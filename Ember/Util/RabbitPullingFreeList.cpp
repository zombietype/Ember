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

#include "RabbitPullingFreeList.hpp"

#include "HelperUtils.hpp"

Ember::RabbitPullingFreeList::RabbitPullingFreeList( uint32_t const max_allowed ) : m_MaxAllowed{ max_allowed }
{
  // Sentinel
  ASSERT( max_allowed != UINT32_MAX );
}

uint32_t Ember::RabbitPullingFreeList::Allocate()
{
  if ( not m_Recycled.empty() )
  {
    uint32_t const index = m_Recycled.front();
    m_Recycled.pop_front();
    return index;
  }

  ASSERT( m_MaxReached < m_MaxAllowed );

  uint32_t const index = m_MaxReached++;

  return index;
}

void Ember::RabbitPullingFreeList::Free( uint32_t const index )
{
  ASSERT( std::ranges::find( m_Recycled, index ) == m_Recycled.end() );
  m_Recycled.push_back( index );
}

uint32_t Ember::RabbitPullingFreeList::InUse() const
{
  return m_MaxReached - ( uint32_t )m_Recycled.size();
}
