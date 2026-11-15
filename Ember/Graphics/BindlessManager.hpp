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

#include <array>
#include <mutex>

#include "Com.hpp"
#include "DeviceHandle.hpp"
#include "Util/DirectXHeaders.hpp"
#include "Util/RabbitPullingFreeList.hpp"

namespace Ember
{

class BindlessManager
{
  ID3D12Device2*            m_Device;

  RabbitPullingFreeList     m_ResourceFreeList;
  Com<ID3D12DescriptorHeap> m_ResourceDescriptorHeap;
  std::mutex                m_ResourceDescriptorLock;
  uint32_t                  m_ResourceDescriptorIncrement;

  RabbitPullingFreeList     m_SamplerFreeList;
  Com<ID3D12DescriptorHeap> m_SamplerDescriptorHeap;
  std::mutex                m_SamplerDescriptorLock;
  uint32_t                  m_SamplerDescriptorIncrement;

public:
  BindlessManager() = default;
  BindlessManager(
      ID3D12Device2&            device,
      Com<ID3D12DescriptorHeap> resource_descriptor_heap,
      uint32_t                  resource_descriptor_increment,
      uint32_t                  max_resources,
      Com<ID3D12DescriptorHeap> sampler_descriptor_heap,
      uint32_t                  sampler_descriptor_increment,
      uint32_t                  max_samplers );

  static void Create( BindlessManager* bindless, ID3D12Device2& device, uint32_t max_resources, uint32_t max_samplers );

  [[nodiscard]] SRVHandle CreateDescriptorHandle(
      ID3D12Resource* resource, D3D12_SHADER_RESOURCE_VIEW_DESC const& srv_desc );
  [[nodiscard]] UAVHandle CreateDescriptorHandle(
      ID3D12Resource* resource, D3D12_UNORDERED_ACCESS_VIEW_DESC const& uav_desc );
  [[nodiscard]] UAVHandle CreateDescriptorHandle(
      ID3D12Resource* resource, ID3D12Resource* counter, D3D12_UNORDERED_ACCESS_VIEW_DESC const& uav_desc );
  [[nodiscard]] RawDescriptorHandle AllocateRawDescriptor(
      D3D12_CPU_DESCRIPTOR_HANDLE* cpu_desc, D3D12_GPU_DESCRIPTOR_HANDLE* gpu_desc );

  [[nodiscard]] SamplerHandle                        CreateSamplerHandle( D3D12_SAMPLER_DESC const& sampler_desc );

  void                                               Free( SRVHandle handle );
  void                                               Free( UAVHandle handle );
  void                                               Free( RawDescriptorHandle handle );
  void                                               Free( SamplerHandle handle );

  [[nodiscard]] std::array<ID3D12DescriptorHeap*, 2> GetBindlessDescriptorHeaps();

  BindlessManager( BindlessManager const& other )                = delete;
  BindlessManager( BindlessManager&& other ) noexcept            = delete;
  BindlessManager& operator=( BindlessManager const& other )     = delete;
  BindlessManager& operator=( BindlessManager&& other ) noexcept = delete;
  ~BindlessManager();
};
} // namespace Ember
