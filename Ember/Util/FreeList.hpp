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

struct FreeList
{
  struct Node
  {
    Node* Next;
    Node* Prev;
  };

  struct Iterator
  {
    Node*     Iter;

    Iterator& operator++();
    bool      operator==( Iterator const& other ) const;
    Node&     operator*();
  };

private:
  Node m_Head;
  Node m_Tail;

public:
  FreeList();

  void               PushBack( Node* node );
  void               PushFront( Node* node );
  Node*              PopFront();
  void               Clear();
  [[nodiscard]] bool Empty() const;

  Iterator           begin();
  Iterator           end();

  FreeList( FreeList&& ) noexcept;
  FreeList& operator=( FreeList&& ) noexcept;

  FreeList( FreeList const& )            = delete;
  FreeList& operator=( FreeList const& ) = delete;

  ~FreeList()                            = default;
};
