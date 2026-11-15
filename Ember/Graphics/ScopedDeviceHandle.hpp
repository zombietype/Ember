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

#include <concepts>
#include <type_traits>
#include "BindlessManager.hpp"
#include "DeviceHandle.hpp"

namespace Ember
{

class ScopedHandlePair
{
  BindlessManager* m_Bindless{ nullptr };
  SRVHandle        m_SRV;
  UAVHandle        m_UAV;

public:
  ScopedHandlePair() = default;
  ScopedHandlePair( BindlessManager* bindless, SRVHandle srv, UAVHandle uav = {} );

  [[nodiscard]] SRVHandle GetSRV() const noexcept;
  [[nodiscard]] UAVHandle GetUAV() const noexcept;

  ScopedHandlePair( ScopedHandlePair&& other ) noexcept;
  ScopedHandlePair& operator=( ScopedHandlePair&& other ) noexcept;
  ScopedHandlePair( ScopedHandlePair const& other )            = delete;
  ScopedHandlePair& operator=( ScopedHandlePair const& other ) = delete;
  ~ScopedHandlePair();
};


} // namespace Ember
