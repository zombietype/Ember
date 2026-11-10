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
#include <utility>

#include "Util/HelperUtils.hpp"

namespace Ember
{
template <std::derived_from<IUnknown> T>
class Com
{
public:
  using Type = T;

private:
  Type* m_Internal = nullptr;

public:
  Com() noexcept = default;

  Com( nullptr_t ) noexcept
  {}

  explicit Com( T* ptr ) noexcept : m_Internal{ ptr }
  {
    ASSERT( m_Internal );
    m_Internal->AddRef();
  }

  Com( Com const& other ) = delete;
  Com( Com&& other ) noexcept : m_Internal{ std::exchange( other.m_Internal, nullptr ) }
  {}

  Com& operator=( Com const& other ) = delete;
  Com& operator=( Com&& other ) noexcept
  {
    if ( this == &other ) return *this;
    if ( m_Internal ) m_Internal->Release();
    m_Internal = std::exchange( other.m_Internal, nullptr );
    return *this;
  }

  Type** WritePtr() noexcept
  {
    if ( m_Internal ) m_Internal->Release();
    return &m_Internal;
  }

  Type* Get() noexcept
  {
    ASSERT( m_Internal );
    return m_Internal;
  }

  Type const* Get() const noexcept
  {
    ASSERT( m_Internal );
    return m_Internal;
  }

  Type* operator->() noexcept
  {
    ASSERT( m_Internal );
    return m_Internal;
  }

  Type const* operator->() const noexcept
  {
    ASSERT( m_Internal );
    return m_Internal;
  }

  Type& operator*() noexcept
  {
    ASSERT( m_Internal );
    return *m_Internal;
  }

  HRESULT As( std::derived_from<IUnknown> auto* other ) noexcept
  {
    ASSERT( m_Internal );
    ASSERT( not other );
    return m_Internal->QueryInterface( IID_PPV_ARGS( &other ) );
  }

  template <std::derived_from<IUnknown> Other>
  HRESULT As( Com<Other>& other ) noexcept
  {
    ASSERT( m_Internal );
    return m_Internal->QueryInterface( IID_PPV_ARGS( other.write_ptr() ) );
  }

  template <std::derived_from<IUnknown> Other>
  Com<Other> As() noexcept
  {
    ASSERT( m_Internal );

    Com<Other> result;
    ERR_ABORT( As( result ) );

    return result;
  }

  Com Clone() const noexcept
  {
    ASSERT( m_Internal );
    return Com{ m_Internal };
  }

  T* Leak() noexcept
  {
    ASSERT( m_Internal );
    return std::exchange( m_Internal, nullptr );
  }

  ~Com() noexcept
  {
    if ( not m_Internal ) return;

    m_Internal->Release();
    m_Internal = nullptr;
  }
};
} // namespace Ember
