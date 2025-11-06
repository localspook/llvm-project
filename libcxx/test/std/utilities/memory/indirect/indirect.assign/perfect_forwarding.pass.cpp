//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

// REQUIRES: std-at-least-c++26

// <memory>

// template <class T, class Allocator = std::allocator<T>> class indirect;

// template<class U = T>
//   constexpr indirect& operator=(U&& u);

#include <cassert>
#include <type_traits>
#include <memory>

#include "test_convertible.h"
#include "test_allocator.h"
#include "min_allocator.h"
#include "archetypes.h"

struct MoveConstructibleOnly {
  MoveConstructibleOnly(MoveConstructibleOnly&&) {}
};

constexpr void test_assignment_sfinae() {
  { // Assignment isn't enabled if T is constructible but not assignable from the RHS type.
    using I = std::indirect<MoveConstructibleOnly>;
    static_assert(!std::is_assignable_v<I&, MoveConstructibleOnly&>);
    static_assert(!std::is_assignable_v<I&, MoveConstructibleOnly&&>);
  }
  { // Assignment isn't enabled if T is assignable but not constructible from the RHS type.
    using I = std::indirect<TestTypes::MoveAssignOnly>;
    static_assert(!std::is_assignable_v<I&, TestTypes::MoveAssignOnly&>);
    static_assert(!std::is_assignable_v<I&, TestTypes::MoveAssignOnly&&>);
  }
  {
    using I = std::indirect<TestTypes::MoveOnly>;
    static_assert(!std::is_assignable_v<I&, TestTypes::MoveOnly&>);
    static_assert(std::is_assignable_v<I&, TestTypes::MoveOnly&&>);
  }
}

void test_assignment_throws() {
#ifndef TEST_HAS_NO_EXCEPTIONS
  struct CopyingThrows {
    CopyingThrows() = default;
    CopyingThrows(const CopyingThrows&) { throw 42; }
    CopyingThrows& operator=(const CopyingThrows&) { throw 42; }
  };

  std::indirect<CopyingThrows> i;
  try {
    i = CopyingThrows();
    assert(false);
  } catch (const int& e) {
    assert(e == 42);
  } catch (...) {
    assert(false);
  }

#endif
}

constexpr void test_assignment() {
  {
    test_allocator_statistics stats = {};
    std::indirect<int, test_allocator<int>> i(std::allocator_arg, test_allocator<int>(&stats));
    assert(stats.construct_count == 0);
    std::same_as<std::indirect<int, test_allocator<int>>&> decltype(auto) ret = (i = 10);
    // assert(stats.construct_count == 1); TODO
    assert(&ret == &i);
    assert(!i.valueless_after_move());
    assert(*i == 10);
  }
}

constexpr bool test() {
  test_assignment_sfinae();
  test_assignment();

  return true;
}

int main(int, char**) {
  test_assignment_throws();
  test();
  static_assert(test());
  return 0;
}
