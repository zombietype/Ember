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

#include "Runtime.hpp"

namespace Ember
{

class ScopedHandle
{
  HANDLE m_Handle{ INVALID_HANDLE_VALUE };

public:
  ScopedHandle() = default;
  ScopedHandle( HANDLE handle );

  ScopedHandle( ScopedHandle const& other ) = delete;
  ScopedHandle( ScopedHandle&& other ) noexcept;
  ScopedHandle& operator=( ScopedHandle const& other ) = delete;
  ScopedHandle& operator=( ScopedHandle&& other ) noexcept;

  operator HANDLE() const;

  ~ScopedHandle();
};

} // namespace Ember
