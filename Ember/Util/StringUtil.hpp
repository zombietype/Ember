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

#include <format>
#include <span>
#include <string_view>

#include "DataUtil.hpp"
#include "HelperUtils.hpp"

namespace Ember
{
inline void ToWideChar( std::span<wchar_t> const& buffer, std::string_view const& str )
{
  if ( str.empty() )
  {
    buffer[0] = L'\0';
    return;
  }
  ASSERT( not buffer.empty() );

  size_t     converted;
  auto const res = mbstowcs_s( &converted, DataOf( buffer ), CountOf( buffer ), DataOf( str ), _TRUNCATE );
  ASSERT( not res );
  ASSERT( converted != std::numeric_limits<size_t>::max() ); // Conversion error
  ASSERT( converted < buffer.size() );                       // Buffer size sufficient (includes null terminator)
}

template <class... TTypes>
char const* FormatTo(
    char* out, size_t max_count, std::format_string<std::type_identity_t<TTypes>...> const fmtstr, TTypes&&... args )
{
  auto const res = std::format_to_n( out, max_count, fmtstr, std::forward<TTypes>( args )... );
  if ( ( size_t )res.size < max_count )
    *res.out = '\0';
  else
    *( res.out - 1 ) = '\0';

  return out;
}

template <class... TTypes>
wchar_t const* FormatTo(
    wchar_t*                                                   out,
    size_t                                                     max_count,
    std::wformat_string<std::type_identity_t<TTypes>...> const fmtstr,
    TTypes&&... args )
{
  auto const res = std::format_to_n( out, max_count, fmtstr, std::forward<TTypes>( args )... );
  if ( ( size_t )res.size < max_count )
    *res.out = L'\0';
  else
    *( res.out - 1 ) = L'\0';

  return out;
}

template <size_t TMaxCount, class... TTypes>
char const* FormatTo(
    char ( &out )[TMaxCount], std::format_string<std::type_identity_t<TTypes>...> const fmtstr, TTypes&&... args )
{
  FormatTo( out, TMaxCount, fmtstr, std::forward<TTypes>( args )... );
  return out;
}

template <size_t TMaxCount, class... TTypes>
wchar_t const* FormatTo(
    wchar_t ( &out )[TMaxCount], std::wformat_string<std::type_identity_t<TTypes>...> const fmtstr, TTypes&&... args )
{
  FormatTo( out, TMaxCount, fmtstr, std::forward<TTypes>( args )... );
  return out;
}

} // namespace Ember
