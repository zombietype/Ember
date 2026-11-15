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

#pragma once

#include <memory_resource>
#include <string>
#include <unordered_map>

#include <Graphics/CommandList.hpp>
#include <Graphics/Queue.hpp>
#include <Graphics/Texture.hpp>

namespace Ember
{
class RenderDevice;

enum class ColorSpaceOverride
{
  kNone,
  kLinear,
  kSrgb,
};

class TextureLoader
{
  struct UploadBatch
  {
    using BarrierList = std::pmr::deque<std::pair<Texture, D3D12_RESOURCE_STATES>>;
    BarrierList                               Barriers;
    std::pmr::deque<Com<D3D12MA::Allocation>> Allocations;
    Queue::Receipt                            Receipt;

    UploadBatch() = default;
    explicit UploadBatch( Queue::Receipt receipt, std::pmr::polymorphic_allocator<> const& pool_allocator );
    void PushTextureStateChange( Texture dest, D3D12_RESOURCE_STATES const final_state );
    void Flush( std::vector<D3D12_RESOURCE_BARRIER>* barriers );

    UploadBatch( UploadBatch const& other )                = delete;
    UploadBatch( UploadBatch&& other ) noexcept            = default;
    UploadBatch& operator=( UploadBatch const& other )     = delete;
    UploadBatch& operator=( UploadBatch&& other ) noexcept = default;
  };

  using TextureCache = std::pmr::unordered_map<std::pmr::string, Texture>;

  RenderDevice*                          m_RenderDevice;

  std::pmr::unsynchronized_pool_resource m_CachePool;
  TextureCache                           m_Cache;
  std::mutex                             m_LoadLock;

  // Upload
  std::pmr::unsynchronized_pool_resource m_InFlightPool;
  std::shared_ptr<Queue>                 m_CopyContext;
  std::vector<UploadBatch>               m_UploadBatches;
  uint32_t                               m_CurrentUploadBatch{ 0 };
  CommandList                            m_CurrentCommandList;
  uint32_t                               m_CurrentUploadBatchSize{ 0 };
  Com<D3D12MA::Allocator>                m_StagingAllocator;

  std::vector<D3D12_RESOURCE_BARRIER>    m_PendingBarriers;


  //
  bool TryLoadImpl(
      Texture* texture, char const* id, DirectX::ScratchImage scratch_image, D3D12_RESOURCE_STATES final_state );

public:
  TextureLoader() = default;

  TextureLoader(
      RenderDevice*           render_device,
      Com<D3D12MA::Allocator> allocator,
      std::shared_ptr<Queue>  copy_context,
      uint32_t                upload_frame_count );

  static bool Create(
      TextureLoader*         loader,
      RenderDevice*          render_device,
      std::shared_ptr<Queue> compute_context,
      uint32_t               upload_frame_count );

  // Loaders
  bool TryLoadTexture(
      Texture*           texture,
      char const*        filename,
      ColorSpaceOverride color_space_override = ColorSpaceOverride::kNone,
      D3D12_RESOURCE_STATES final_state       = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE );

  bool TryLoadTextureFromData(
      Texture*           texture,
      char const*        id,
      size_t             data_size,
      byte const*        data,
      ColorSpaceOverride color_space_override = ColorSpaceOverride::kNone,
      D3D12_RESOURCE_STATES final_state       = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE );

  Queue::Receipt EndBatch();

  void           Update();
  void           FlushBarriers( CommandList* command_list );
};

} // namespace Ember
