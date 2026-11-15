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

#include <deque>

namespace Ember
{

// Freelist that returns indexes.
// Prefers recycled indexes, else pulls a new rabbit (index) out of a hat.
class RabbitPullingFreeList
{
  std::deque<uint32_t> m_Recycled{};
  uint32_t             m_MaxReached{ 0 };
  uint32_t             m_MaxAllowed{ 0 };

public:
  RabbitPullingFreeList() = default;
  explicit RabbitPullingFreeList( uint32_t max_allowed );

  uint32_t               Allocate();
  void                   Free( uint32_t index );

  [[nodiscard]] uint32_t InUse() const;
};

} // namespace Ember
