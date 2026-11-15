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

#include "Buffer.hpp"

#include "RenderDevice.hpp"
#include "Util/FlatMap.hpp"
#include "Util/HelperUtils.hpp"

namespace Ember
{

struct BufferImpl
{
  struct Handles
  {
    BindlessManager* Bindless; // Not owned; To be used with the handles
    SRVHandle        AsSRV;
    UAVHandle        AsUAV;

    /*
     * Note about book-keeping:
     * A buffer can be used as:
     * 1. constant buffer (CBV)
     * 2. readonly storage buffer (SRV)
     * 3. read-write storage buffer (SRV+UAV)
     * Thus, if we have SRV then we are definitely not a constant buffer.
     *
     * So, we use validity of SRVHandle to determine if the buffer is a constant buffer or not.
     * And union stores UAV or CBV.
     */

    bool IsConstantBuffer() const noexcept
    {
      return not AsSRV;
    }

    Handles() : Bindless{ nullptr }, AsSRV{}, AsUAV{}
    {}

    Handles( BindlessManager* const bindless, SRVHandle const srv, UAVHandle const uav = {} )
      : Bindless{ bindless }, AsSRV{ srv }, AsUAV{ uav }
    {}

    Handles( Handles&& other ) noexcept : Bindless{ other.Bindless }, AsSRV{ other.AsSRV }, AsUAV{ other.AsUAV }
    {
      other.Bindless = nullptr;
      other.AsSRV    = {};
      other.AsUAV    = {};
    }

    Handles& operator=( Handles&& other ) noexcept
    {
      if ( this == &other ) return *this;
      std::swap( Bindless, other.Bindless );
      std::swap( AsSRV, other.AsSRV );
      std::swap( AsUAV, other.AsUAV );

      return *this;
    }

    Handles( Handles const& )            = delete;
    Handles& operator=( Handles const& ) = delete;

    ~Handles()
    {
      if ( not Bindless ) return;

      Bindless->Free( AsSRV );
      ASSERT( not IsConstantBuffer() );
      Bindless->Free( AsUAV );
    }
  };

  ComPtr<ID3D12Resource>      Resource;
  ComPtr<D3D12MA::Allocation> Allocation;
  D3D12_GPU_VIRTUAL_ADDRESS   GPUAddress;
  uint64_t                    Size;
  Buffer::Type                Type;
  Handles                     Handles;
};
} // namespace Ember

Ember::Buffer::Buffer( std::shared_ptr<BufferImpl> impl ) : m_Impl{ std::move( impl ) }
{}

void Ember::Buffer::Write( size_t const offset, size_t const size, void const* data ) const
{
  if ( size == 0 ) return;

  D3D12_RANGE const empty_read_range = { 0, 0 };
  D3D12_RANGE const write_range      = { offset, offset + size };

  byte*             mapped;
  ERR_ABORT( m_Impl->Resource->Map( 0, &empty_read_range, ( void** )&mapped ) );

  memcpy( mapped + offset, data, size );

  m_Impl->Resource->Unmap( 0, &write_range );
}

ID3D12Resource* Ember::Buffer::GetBuffer() const noexcept
{
  ASSERT( m_Impl );
  return m_Impl->Resource.Get();
}

uint64_t Ember::Buffer::GetSize() const noexcept
{
  return m_Impl ? m_Impl->Size : 0;
}

Ember::Buffer::Type Ember::Buffer::GetType() const noexcept
{
  ASSERT( m_Impl );
  return m_Impl->Type;
}

Ember::SRVHandle Ember::Buffer::GetSRVHandle() const
{
  if ( not m_Impl ) return {};
  ASSERT( GetType() == Type::kStorageBuffer or GetType() == Type::kAccelerationStructure );

  return m_Impl->Handles.AsSRV;
}

Ember::UAVHandle Ember::Buffer::GetUAVHandle() const
{
  if ( not m_Impl ) return {};
  ASSERT( GetType() == Type::kStorageBuffer );
  ASSERT( not m_Impl->Handles.IsConstantBuffer() );

  auto handle = m_Impl->Handles.AsUAV;
  ASSERT( handle );

  return handle;
}

void Ember::Buffer::SetName( LPCWSTR const name ) const
{
  ERR_ABORT( m_Impl->Resource->SetName( name ) );
}

D3D12_GPU_VIRTUAL_ADDRESS Ember::Buffer::GetGPUVirtualAddress() const
{
  return m_Impl->GPUAddress;
}

uintptr_t Ember::Buffer::GetPtrID() const
{
  return ( uintptr_t )m_Impl.get();
}

Ember::Buffer Ember::BufferManager::Wrap( BufferImpl&& impl )
{
  return Buffer{ std::allocate_shared<BufferImpl>(
      std::pmr::polymorphic_allocator<>( &m_MemoryPool ), std::forward<BufferImpl>( impl ) ) };
}

Ember::BufferManager::BufferManager(
    ComPtr<ID3D12Device2> device, ComPtr<D3D12MA::Allocator> gpu_allocator, BindlessManager* bindless_manager )
  : m_Bindless{ bindless_manager }, m_Device{ std::move( device ) }, m_GpuAllocator{ std::move( gpu_allocator ) }
{}

namespace
{

void AllocateBufferImpl(
    [[maybe_unused]] ID3D12Device2* device, // Only used when running for RenderDoc
    D3D12MA::Allocator*             allocator,
    uint64_t const                  size,
    D3D12MA::Allocation**           allocation,
    ID3D12Resource**                resource,
    D3D12_RESOURCE_FLAGS const      flags  = D3D12_RESOURCE_FLAG_NONE,
    D3D12_RESOURCE_STATES const init_state = D3D12_RESOURCE_STATE_COMMON,
    bool const use_rebar_heap              = true )
{
  CD3DX12_RESOURCE_DESC const buffer_desc = CD3DX12_RESOURCE_DESC::Buffer( size, flags );

  auto const                  heap_type   = use_rebar_heap ? D3D12_HEAP_TYPE_GPU_UPLOAD : D3D12_HEAP_TYPE_DEFAULT;
  // TODO: Buffer Allocation can fail. Handle by returning value.
#if not defined( RENDERDOC_COMPAT )
  D3D12MA::ALLOCATION_DESC const allocation_desc = {
    .Flags    = D3D12MA::ALLOCATION_FLAG_NONE,
    .HeapType = heap_type,
  };

  ERR_ABORT( allocator->CreateResource(
      &allocation_desc, &buffer_desc, init_state, nullptr, allocation, IID_PPV_ARGS( resource ) ) );
#else
  auto const heap_properties = CD3DX12_HEAP_PROPERTIES{ heap_type };
  ERR_ABORT( device->CreateCommittedResource(
      &heap_properties,
      D3D12_HEAP_FLAG_ALLOW_ALL_BUFFERS_AND_TEXTURES,
      &buffer_desc,
      init_state,
      nullptr,
      IID_PPV_ARGS( resource ) ) );
#endif
}
} // namespace

Ember::Buffer Ember::BufferManager::CreateVertexBuffer( uint32_t const size, uint32_t const stride )
{
  ComPtr<ID3D12Resource>      buffer;
  ComPtr<D3D12MA::Allocation> allocation;
  AllocateBufferImpl( m_Device.Get(), m_GpuAllocator.Get(), size, &allocation, &buffer );

  auto const virtual_address = buffer->GetGPUVirtualAddress();

  return Wrap( {
      .Resource   = std::move( buffer ),
      .Allocation = std::move( allocation ),
      .GPUAddress = virtual_address,
      .Size       = size,
      .Type       = Buffer::Type::kVertexBuffer,
  } );
}

Ember::Buffer Ember::BufferManager::CreateIndexBuffer( uint32_t const size, DXGI_FORMAT const format )
{
  ComPtr<ID3D12Resource>      buffer;
  ComPtr<D3D12MA::Allocation> allocation;
  AllocateBufferImpl( m_Device.Get(), m_GpuAllocator.Get(), size, &allocation, &buffer );

  auto const virtual_address = buffer->GetGPUVirtualAddress();
  return Wrap( {
      .Resource   = std::move( buffer ),
      .Allocation = std::move( allocation ),
      .GPUAddress = virtual_address,
      .Size       = size,
      .Type       = Buffer::Type::kIndexBuffer,
  } );
}

Ember::Buffer Ember::BufferManager::CreateStorageBuffer( uint32_t const size, uint32_t const stride )
{
  ASSERT( size % stride == 0 );

  ComPtr<ID3D12Resource>      buffer;
  ComPtr<D3D12MA::Allocation> allocation;
  AllocateBufferImpl( m_Device.Get(), m_GpuAllocator.Get(), size, &allocation, &buffer );

  auto const srv_desc        = CD3DX12_SHADER_RESOURCE_VIEW_DESC::StructuredBuffer( size / stride, stride );

  auto const srv_handle      = m_Bindless->CreateDescriptorHandle( buffer.Get(), srv_desc );
  auto const virtual_address = buffer->GetGPUVirtualAddress();

  return Wrap( {
      .Resource   = std::move( buffer ),
      .Allocation = std::move( allocation ),
      .GPUAddress = virtual_address,
      .Size       = size,
      .Type       = Buffer::Type::kStorageBuffer,
      .Handles    = { m_Bindless, srv_handle },
  } );
}

Ember::Buffer Ember::BufferManager::CreateRawStorageBuffer( uint64_t size )
{
  uint64_t constexpr static kStride = 4;

  //
  size = ( size % kStride == 0 ) ? size : size + ( kStride - ( size % kStride ) );

  ComPtr<ID3D12Resource>      buffer;
  ComPtr<D3D12MA::Allocation> allocation;
  AllocateBufferImpl( m_Device.Get(), m_GpuAllocator.Get(), size, &allocation, &buffer );

  auto srv_desc              = CD3DX12_SHADER_RESOURCE_VIEW_DESC::RawBuffer( ( UINT )( size / kStride ), 0 );
  srv_desc.Format            = DXGI_FORMAT_R32_TYPELESS;

  auto const srv_handle      = m_Bindless->CreateDescriptorHandle( buffer.Get(), srv_desc );
  auto const virtual_address = buffer->GetGPUVirtualAddress();

  return Wrap( {
      .Resource   = std::move( buffer ),
      .Allocation = std::move( allocation ),
      .GPUAddress = virtual_address,
      .Size       = size,
      .Type       = Buffer::Type::kStorageBuffer,
      .Handles    = { m_Bindless, srv_handle },
  } );
}

Ember::Buffer Ember::BufferManager::CreateReadWriteBuffer( uint32_t size, uint32_t stride )
{
  ASSERT( size % stride == 0 );

  ComPtr<ID3D12Resource>      buffer;
  ComPtr<D3D12MA::Allocation> allocation;
  AllocateBufferImpl(
      m_Device.Get(), m_GpuAllocator.Get(), size, &allocation, &buffer, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS );

  auto const srv_desc        = CD3DX12_SHADER_RESOURCE_VIEW_DESC::StructuredBuffer( size / stride, stride );
  auto const uav_desc        = CD3DX12_UNORDERED_ACCESS_VIEW_DESC::StructuredBuffer( size / stride, stride );

  auto const srv_handle      = m_Bindless->CreateDescriptorHandle( buffer.Get(), srv_desc );
  auto const uav_handle      = m_Bindless->CreateDescriptorHandle( buffer.Get(), uav_desc );
  auto const virtual_address = buffer->GetGPUVirtualAddress();

  return Wrap( {
      .Resource   = std::move( buffer ),
      .Allocation = std::move( allocation ),
      .GPUAddress = virtual_address,
      .Size       = size,
      .Type       = Buffer::Type::kStorageBuffer,
      .Handles    = { m_Bindless, srv_handle, uav_handle },
  } );
}

Ember::Buffer Ember::BufferManager::CreateConstantBuffer( uint32_t const size )
{
  uint32_t const              padded_size = ( 256 - ( size % 256 ) ) + size;

  ComPtr<ID3D12Resource>      buffer;
  ComPtr<D3D12MA::Allocation> allocation;
  AllocateBufferImpl( m_Device.Get(), m_GpuAllocator.Get(), padded_size, &allocation, &buffer );

  D3D12_GPU_VIRTUAL_ADDRESS const addr = buffer->GetGPUVirtualAddress();
  return Wrap( {
      .Resource   = std::move( buffer ),
      .Allocation = std::move( allocation ),
      .GPUAddress = addr,
      .Size       = size,
      .Type       = Buffer::Type::kConstantBuffer,
  } );
}

Ember::Buffer Ember::BufferManager::CreateASBuffer( uint64_t const size )
{
  ComPtr<ID3D12Resource>      buffer;
  ComPtr<D3D12MA::Allocation> allocation;
  AllocateBufferImpl(
      m_Device.Get(),
      m_GpuAllocator.Get(),
      size,
      &allocation,
      &buffer,
      D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS,
      D3D12_RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE,
      false );

  auto const addr       = buffer->GetGPUVirtualAddress();
  auto const srv_desc   = CD3DX12_SHADER_RESOURCE_VIEW_DESC::RaytracingAccelStruct( addr );
  auto const srv_handle = m_Bindless->CreateDescriptorHandle( nullptr, srv_desc );

  return Wrap( {
      .Resource   = std::move( buffer ),
      .Allocation = std::move( allocation ),
      .GPUAddress = addr,
      .Size       = size,
      .Type       = Buffer::Type::kAccelerationStructure,
      .Handles    = { m_Bindless, srv_handle },
  } );
}
