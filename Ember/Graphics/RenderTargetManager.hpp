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

#include <Util/DirectXHeaders.hpp>
#include <Util/Runtime.hpp>

namespace Ember
{
class Texture;
class RenderDevice;

class RenderTargetManager
{
  ComPtr<ID3D12Device>         m_D3D12Device;
  ComPtr<ID3D12DescriptorHeap> m_RTVDescriptorHeap;
  ComPtr<ID3D12DescriptorHeap> m_DSVDescriptorHeap;
  uint32_t                     m_RTVDescriptorSize{ 0 };
  uint32_t                     m_DSVDescriptorSize{ 0 };

public:
  static void Create( RenderTargetManager* render_target_manager, ComPtr<ID3D12Device> d3d_device );

  RenderTargetManager() = default;
  RenderTargetManager(
      ComPtr<ID3D12Device>         d3d12_device,
      ComPtr<ID3D12DescriptorHeap> rtv_descriptor_heap,
      uint32_t                     rtv_descriptor_size,
      ComPtr<ID3D12DescriptorHeap> dsv_descriptor_heap,
      uint32_t                     dsv_descriptor_size );

  void RSSetScissorViewport( ID3D12GraphicsCommandList* command_list, uint32_t width, uint32_t height ) const;

  void ClearRenderTargetView(
      ID3D12GraphicsCommandList* command_list, ID3D12Resource* render_target, float const color[] ) const;

  void ClearRenderTargetView(
      ID3D12GraphicsCommandList* command_list, Texture const& render_target, float const color[] ) const;

  void ClearRenderTargetViews(
      ID3D12GraphicsCommandList* command_list,
      uint32_t                   count,
      ID3D12Resource**           render_target,
      float const                color[] ) const;

  void ClearRenderTargetViews(
      ID3D12GraphicsCommandList* command_list,
      uint32_t                   count,
      Texture const*             render_target,
      float const                color[] ) const;

  void ClearDepthStencilView(
      ID3D12GraphicsCommandList* command_list,
      ID3D12Resource*            depth_stencil,
      D3D12_CLEAR_FLAGS          flags,
      float                      depth,
      uint8_t                    stencil ) const;

  void ClearDepthStencilView(
      ID3D12GraphicsCommandList* command_list,
      Texture const&             depth_stencil,
      D3D12_CLEAR_FLAGS          flags,
      float                      depth,
      uint8_t                    stencil ) const;

  void OMSetRenderTargets(
      ID3D12GraphicsCommandList*           command_list,
      uint32_t                             count,
      ID3D12Resource**                     render_targets,
      D3D12_RENDER_TARGET_VIEW_DESC const* rtv_desc,
      ID3D12Resource*                      depth_stencil,
      D3D12_DEPTH_STENCIL_VIEW_DESC const* dsv_desc ) const;

  void OMSetRenderTargets(
      ID3D12GraphicsCommandList* command_list,
      uint32_t                   count,
      Texture const*             render_targets,
      Texture const*             depth_stencil ) const;

  void OMSetRenderTargets(
      ID3D12GraphicsCommandList*           command_list,
      uint32_t                             count,
      Texture const*                       render_targets,
      D3D12_RENDER_TARGET_VIEW_DESC const* rtv_desc,
      Texture const*                       depth_stencil,
      D3D12_DEPTH_STENCIL_VIEW_DESC const* dsv_desc ) const;
};

} // namespace Ember
