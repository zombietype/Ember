// ===========================================================================
//
// Copyright 2026 Anish Bhobe
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
// http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//
// ===========================================================================

#include "TextureLoader.hpp"

#include <filesystem>
#include <span>

#include <Graphics/RenderDevice.hpp>
#include <Util/DataUtil.hpp>
#include <Util/DirectXHeaders.hpp>
#include <Util/HelperUtils.hpp>
#include <Util/Profiling.hpp>
#include <Util/StringUtil.hpp>
#include "MipMapGenerator.hpp"

Ember::TextureLoader::UploadBatch::UploadBatch(
    Queue::Receipt receipt, std::pmr::polymorphic_allocator<> const& pool_allocator )
  : Barriers{ pool_allocator }, Allocations{ pool_allocator }, Receipt{ std::move( receipt ) }
{}

void Ember::TextureLoader::UploadBatch::PushTextureStateChange( Texture dest, D3D12_RESOURCE_STATES const final_state )
{
  Barriers.emplace_back( dest, final_state );
}

void Ember::TextureLoader::UploadBatch::Flush( std::vector<D3D12_RESOURCE_BARRIER>* barriers )
{
  std::ranges::transform(
      Barriers,
      std::back_inserter( *barriers ),
      []( auto const& pair )
      {
        D3D12_RESOURCE_BARRIER const transition =
            CD3DX12_RESOURCE_BARRIER::Transition( pair.first.GetTexture(), pair.first.GetCurrentState(), pair.second );
        pair.first.SetCurrentState( pair.second );
        return transition;
      } );

  Barriers.clear();
  Allocations.clear();
}

Ember::TextureLoader::TextureLoader(
    RenderDevice*           render_device,
    Com<D3D12MA::Allocator> allocator,
    std::shared_ptr<Queue>  copy_context,
    uint32_t const          upload_frame_count )
  : m_RenderDevice{ render_device }
  , m_CopyContext{ std::move( copy_context ) }
  , m_StagingAllocator{ std::move( allocator ) }
{
  m_UploadBatches.reserve( upload_frame_count );
  Queue::Receipt initial = m_CopyContext->CreateReceipt();
  for ( int i = 0; i < ( int )upload_frame_count; ++i )
  {
    m_UploadBatches.emplace_back( initial, &m_InFlightPool );
  }
  m_CurrentCommandList = m_CopyContext->GetCommandList();
}

bool Ember::TextureLoader::Create(
    TextureLoader*         loader,
    RenderDevice*          render_device,
    std::shared_ptr<Queue> compute_context,
    uint32_t const         upload_frame_count )
{
  if ( compute_context->GetCommandListType() != D3D12_COMMAND_LIST_TYPE_COMPUTE )
  {
    OutputDebugStringA( "TextureLoader requires a compute context for mip-map generation. " );
    return false;
  }

  D3D12MA::ALLOCATOR_DESC const alloc_desc = {
    .Flags    = D3D12MA_RECOMMENDED_ALLOCATOR_FLAGS,
    .pDevice  = render_device->GetDevice(),
    .pAdapter = render_device->GetAdapter(),
  };
  Com<D3D12MA::Allocator> alloc;
  ERR_FAIL_RET_F( D3D12MA::CreateAllocator( &alloc_desc, alloc.WritePtr() ) );

  // We need COMPUTE instead of COPY due to the mip-mapping.
  // Ideally, we want to kick the job to an async compute queue.

  new ( loader ) TextureLoader{
    render_device,
    std::move( alloc ),
    std::move( compute_context ),
    upload_frame_count,
  };

  return true;
}

bool Ember::TextureLoader::TryLoadImpl(
    Texture* texture, char const* id, DirectX::ScratchImage scratch_image, D3D12_RESOURCE_STATES final_state )
{
  PIXScopedEvent( m_CurrentCommandList.Get(), PIX_COLOR_DEFAULT, "TextureLoader::TryLoadImpl %s", id );

  // Mipmapping
  {
    DirectX::ScratchImage mipmapped;
    DirectX::GenerateMipMaps(
        scratch_image.GetImages(),
        scratch_image.GetImageCount(),
        scratch_image.GetMetadata(),
        DirectX::TEX_FILTER_DEFAULT,
        0,
        mipmapped );

    scratch_image = std::move( mipmapped );
  }

  std::span const images{ scratch_image.GetImages(), scratch_image.GetImageCount() };

  auto const&     metadata = scratch_image.GetMetadata();

  new ( texture ) Texture{ m_RenderDevice->CreateTexture2D( {
      .Format    = metadata.format,
      .Width     = ( uint32_t )metadata.width,
      .Height    = ( uint32_t )metadata.height,
      .InitState = D3D12_RESOURCE_STATE_COPY_DEST,
  } ) };

  wchar_t wide_id[512];
  MultiByteToWideChar( CP_UTF8, MB_ERR_INVALID_CHARS, id, -1, wide_id, 512 );
  ERR_FAIL_RET_V( texture->GetTexture()->SetName( wide_id ), false );

  uint64_t const           req_size = GetRequiredIntermediateSize( texture->GetTexture(), 0, CountOf( images ) );

  Com<D3D12MA::Allocation> staging_alloc;
  {
    CD3DX12_RESOURCE_DESC const    resource_desc   = CD3DX12_RESOURCE_DESC::Buffer( req_size );

    D3D12MA::ALLOCATION_DESC const allocation_desc = {
      .Flags    = D3D12MA::ALLOCATION_FLAG_NONE,
      .HeapType = D3D12_HEAP_TYPE_UPLOAD,
    };

    ERR_FAIL_RET_V(
        m_StagingAllocator->CreateResource(
            &allocation_desc,
            &resource_desc,
            D3D12_RESOURCE_STATE_GENERIC_READ,
            nullptr,
            staging_alloc.WritePtr(),
            IID_NULL,
            nullptr ),
        false );
  }

  auto lock_guard = std::lock_guard( m_LoadLock );
  // [1] Needs to run on a single thread from here.

  static std::vector<D3D12_SUBRESOURCE_DATA> subresources;
  subresources.clear(); // Doesn't release resource, so we get to keep reusing allocation.
  for ( auto& image : images )
  {
    subresources.push_back( {
        .pData      = image.pixels,
        .RowPitch   = ( LONG_PTR )image.rowPitch,
        .SlicePitch = ( LONG_PTR )image.slicePitch,
    } );
  }

  ++m_CurrentUploadBatchSize;

  UpdateSubresources(
      m_CurrentCommandList.Get(),
      texture->GetTexture(),
      staging_alloc->GetResource(),
      0,
      0,
      CountOf( subresources ),
      DataOf( subresources ) );

  m_UploadBatches[m_CurrentUploadBatch].PushTextureStateChange( *texture, final_state );
  m_UploadBatches[m_CurrentUploadBatch].Allocations.push_back( std::move( staging_alloc ) );

  m_Cache[id] = *texture;

  return true;
}

bool Ember::TextureLoader::TryLoadTexture(
    Texture*                    texture,
    char const*                 filename,
    ColorSpaceOverride const    color_space_override,
    D3D12_RESOURCE_STATES const final_state )
{
  auto const it = m_Cache.find( filename );
  if ( it != m_Cache.end() )
  {
    new ( texture ) Texture{ it->second };
    return true;
  }

  std::filesystem::path const file_path( filename );
  if ( not exists( file_path ) ) return false;
  if ( not file_path.has_extension() ) return false;

  wchar_t wide_filename[512];
  MultiByteToWideChar( CP_UTF8, MB_ERR_INVALID_CHARS, filename, -1, wide_filename, 512 );

  DirectX::TexMetadata  metadata;
  DirectX::ScratchImage scratch_image;

  if ( file_path.extension() == ".hdr" )
  {
    ERR_FAIL_RET_F( DirectX::LoadFromHDRFile( wide_filename, &metadata, scratch_image ) );
  }
  else
  {
    // TODO: This is dicey. Might wanna support "R" format etc.

    DirectX::WIC_FLAGS const flags =
        ( color_space_override == ColorSpaceOverride::kSrgb ? DirectX::WIC_FLAGS_DEFAULT_SRGB
                                                            : DirectX::WIC_FLAGS_NONE ) |
        DirectX::WIC_FLAGS_FORCE_RGB;
    ERR_FAIL_RET_F( LoadFromWICFile( wide_filename, flags, &metadata, scratch_image ) );
  }

  return TryLoadImpl( texture, filename, std::move( scratch_image ), final_state );
}

bool Ember::TextureLoader::TryLoadTextureFromData(
    Texture*                    texture,
    char const*                 id,
    size_t const                data_size,
    byte const*                 data,
    ColorSpaceOverride const    color_space_override,
    D3D12_RESOURCE_STATES const final_state )
{
  auto const it = m_Cache.find( id );
  if ( it != m_Cache.end() )
  {
    new ( texture ) Texture{ it->second };
    return true;
  }

  DirectX::TexMetadata     metadata;
  DirectX::ScratchImage    scratch_image;
  DirectX::WIC_FLAGS const flags = ( color_space_override == ColorSpaceOverride::kSrgb ? DirectX::WIC_FLAGS_DEFAULT_SRGB
                                                                                       : DirectX::WIC_FLAGS_NONE ) |
                                   DirectX::WIC_FLAGS_FORCE_RGB;
  ERR_FAIL_RET_V( DirectX::LoadFromWICMemory( data, data_size, flags, &metadata, scratch_image ), false );

  return TryLoadImpl( texture, id, std::move( scratch_image ), final_state );
}

Ember::Queue::Receipt Ember::TextureLoader::EndBatch()
{
  m_UploadBatches[m_CurrentUploadBatch].Receipt = m_CopyContext->Submit( std::move( m_CurrentCommandList ) );

  Queue::Receipt const batch_receipt            = m_UploadBatches[m_CurrentUploadBatch].Receipt;

  m_CurrentUploadBatch++;
  m_CurrentUploadBatch %= m_UploadBatches.size();

  m_CopyContext->WaitOn( m_UploadBatches[m_CurrentUploadBatch].Receipt );

  auto lock_guard = std::lock_guard( m_LoadLock );

  m_UploadBatches[m_CurrentUploadBatch].Flush( &m_PendingBarriers );

  m_CurrentCommandList     = m_CopyContext->GetCommandList();
  m_CurrentUploadBatchSize = 0;

  return batch_receipt;
}

void Ember::TextureLoader::Update()
{
  // TODO: Ideally, I don't want this as a check on 'Update' but instead it should be a task on a thread pool.
  for ( auto& batch : m_UploadBatches )
  {
    ASSERT_M(
        batch.Barriers.empty() or batch.Receipt.IsValid(), "Either no textures in the batch, or the batch is ended." );
    if ( not batch.Barriers.empty() and batch.Receipt.IsComplete() )
    {
      auto lock_guard = std::lock_guard( m_LoadLock );

      batch.Flush( &m_PendingBarriers );
    }
  }
}

void Ember::TextureLoader::FlushBarriers( CommandList* command_list )
{
  if ( m_PendingBarriers.empty() ) return;

  auto lock_guard = std::lock_guard( m_LoadLock );

  command_list->ResourceBarrier( m_PendingBarriers );

  m_PendingBarriers.clear();
}
