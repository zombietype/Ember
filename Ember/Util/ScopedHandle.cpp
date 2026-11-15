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

#include "ScopedHandle.hpp"

#include <utility>

Ember::ScopedHandle::ScopedHandle( HANDLE const handle ) : m_Handle{ handle }
{}

Ember::ScopedHandle::ScopedHandle( ScopedHandle&& other ) noexcept : m_Handle{ other.m_Handle }
{
  other.m_Handle = INVALID_HANDLE_VALUE;
}

Ember::ScopedHandle& Ember::ScopedHandle::operator=( ScopedHandle&& other ) noexcept
{
  if ( this == &other ) return *this;
  std::swap( m_Handle, other.m_Handle );
  return *this;
}

Ember::ScopedHandle::operator HANDLE() const
{
  return m_Handle;
}

Ember::ScopedHandle::~ScopedHandle()
{
  ::CloseHandle( m_Handle );
}
