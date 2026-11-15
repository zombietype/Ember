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

#include <cassert>

#define DEBUG_BREAK __debugbreak()

#define ERR_FAIL_RET_V( EXPR, RET_VALUE )                                                                              \
  do                                                                                                                   \
  {                                                                                                                    \
    HRESULT hres = ( EXPR );                                                                                           \
    if ( auto x = FAILED( hres ) )                                                                                     \
    {                                                                                                                  \
      _com_error err( hres );                                                                                          \
      LPCTSTR    err_msg = err.ErrorMessage();                                                                         \
      MessageBox( nullptr, L"Warn: " #EXPR, err_msg, MB_OK | MB_ICONWARNING );                                         \
      DEBUG_BREAK;                                                                                                     \
      return RET_VALUE;                                                                                                \
    }                                                                                                                  \
  }                                                                                                                    \
  while ( false )

#define ERR_FAIL_RET_F( EXPR )                                                                                         \
  do                                                                                                                   \
  {                                                                                                                    \
    HRESULT hres = ( EXPR );                                                                                           \
    if ( auto x = FAILED( hres ) )                                                                                     \
    {                                                                                                                  \
      _com_error err( hres );                                                                                          \
      LPCTSTR    err_msg = err.ErrorMessage();                                                                         \
      MessageBox( nullptr, L"Warn: " #EXPR, err_msg, MB_OK | MB_ICONWARNING );                                         \
      DEBUG_BREAK;                                                                                                     \
      return false;                                                                                                    \
    }                                                                                                                  \
  }                                                                                                                    \
  while ( false )

#define ERR_FAIL_RET( EXPR )                                                                                           \
  do                                                                                                                   \
  {                                                                                                                    \
    HRESULT hres = ( EXPR );                                                                                           \
    if ( auto x = FAILED( hres ) )                                                                                     \
    {                                                                                                                  \
      _com_error err( hres );                                                                                          \
      LPCTSTR    err_msg = err.ErrorMessage();                                                                         \
      MessageBox( nullptr, L"Warn: " #EXPR, err_msg, MB_OK | MB_ICONWARNING );                                         \
      DEBUG_BREAK;                                                                                                     \
      return hres;                                                                                                     \
    }                                                                                                                  \
  }                                                                                                                    \
  while ( false )


#define ERR_ABORT( hr )                                                                                                \
  do                                                                                                                   \
  {                                                                                                                    \
    HRESULT hres = ( hr );                                                                                             \
    if ( auto x = FAILED( hres ) )                                                                                     \
    {                                                                                                                  \
      _com_error err( hr );                                                                                            \
      LPCTSTR    err_msg = err.ErrorMessage();                                                                         \
      MessageBox( nullptr, L"Err: " #hr, err_msg, MB_OK | MB_ICONERROR );                                              \
      DEBUG_BREAK;                                                                                                     \
      exit( x );                                                                                                       \
    }                                                                                                                  \
  }                                                                                                                    \
  while ( false )

#define ASSERT( x ) assert( x )
#define ASSERT_M( x, MSG ) assert( x )

#if defined( _DEBUG )
#define ENSURE( x ) assert( x )
#else
#define ENSURE( x )                                                                                                    \
  if ( not( x ) ) std::terminate()
#endif


#define UNREACHABLE std::terminate()
#define UNREACHABLE_M( MSG ) std::terminate()

#define UNIMPLEMENTED std::terminate()
#define UNIMPLEMENTED_M( MSG ) std::terminate()

void ParseArguments( bool* use_warp, uint32_t* client_width, uint32_t* client_height );
