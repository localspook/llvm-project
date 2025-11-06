// -*- C++ -*-
//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#ifndef _LIBCPP___MEMORY_POLYMORPHIC_H
#define _LIBCPP___MEMORY_POLYMORPHIC_H

#include <__config>

#include <__concepts/derived_from.h>
#include <__fwd/memory_resource.h>
#include <__memory/addressof.h>
#include <__memory/allocation_guard.h>
#include <__memory/allocator_arg_t.h>
#include <__memory/allocator_traits.h>
#include <__memory/swap_allocator.h>
#include <__type_traits/is_array.h>
#include <__type_traits/is_object.h>
#include <__type_traits/is_same.h>
#include <__utility/exchange.h>
#include <__utility/forward.h>
#include <__utility/in_place.h>
#include <__utility/move.h>
#include <__utility/swap.h>
#include <initializer_list>

#if !defined(_LIBCPP_HAS_NO_PRAGMA_SYSTEM_HEADER)
#  pragma GCC system_header
#endif

#if _LIBCPP_STD_VER >= 26

_LIBCPP_BEGIN_NAMESPACE_STD

template <class _Tp, class _Allocator = allocator<_Tp>>
class _LIBCPP_NO_SPECIALIZATIONS polymorphic {
public:
  using value_type     = _Tp;
  using allocator_type = _Allocator;
  using pointer        = allocator_traits<_Allocator>::pointer;
  using const_pointer  = allocator_traits<_Allocator>::const_pointer;

  static_assert(__check_valid_allocator<allocator_type>::value);
  static_assert(is_same_v<typename allocator_type::value_type, value_type>);
  static_assert(is_object_v<value_type>);
  static_assert(!is_array_v<value_type>);
  static_assert(!is_same_v<value_type, in_place_t>);
  static_assert(!__is_inplace_type<value_type>::value);
  static_assert(std::is_same_v<value_type, remove_cv_t<value_type>>,
                "value_type must not be const or volatile qualified");

  // [polymorphic.ctor], constructors
  _LIBCPP_HIDE_FROM_ABI constexpr explicit polymorphic()
    requires is_default_constructible_v<_Allocator>
      : __p_(__allocate_owned_object<_Tp>(__alloc_)), __vtable_(std::addressof(__vtable_for<_Tp>)) {
    static_assert(is_default_constructible_v<_Tp>);
    static_assert(is_copy_constructible_v<_Tp>);
  }

  _LIBCPP_HIDE_FROM_ABI constexpr explicit polymorphic(allocator_arg_t, const _Allocator& __a)
    requires is_default_constructible_v<_Allocator>
      : __alloc_(__a), __p_(__allocate_owned_object<_Tp>(__alloc_)), __vtable_(std::addressof(__vtable_for<_Tp>)) {
    static_assert(is_default_constructible_v<_Tp>);
    static_assert(is_copy_constructible_v<_Tp>);
  }

  _LIBCPP_HIDE_FROM_ABI constexpr polymorphic(const polymorphic& __other)
      : __alloc_(allocator_traits<_Allocator>::select_on_container_copy_construction(__other.__alloc_)),
        __p_(__other.valueless_after_move() ? nullptr : __other.__vtable_->__clone_(__alloc_, __other.__p_)),
        __vtable_(__other.__vtable_) {}

  _LIBCPP_HIDE_FROM_ABI constexpr polymorphic(allocator_arg_t, const _Allocator& __a, const polymorphic& __other)
      : __alloc_(__a),
        __p_(__other.valueless_after_move() ? nullptr : __other.__vtable_->__clone_(__alloc_, __other.__p_)),
        __vtable_(__other.__vtable_) {}

  _LIBCPP_HIDE_FROM_ABI constexpr polymorphic(polymorphic&& __other) noexcept
      : __alloc_(std::move(__other.__alloc_)),
        __p_(std::exchange(__other.__p_, nullptr)),
        __vtable_(__other.__vtable_) {}

  _LIBCPP_HIDE_FROM_ABI constexpr polymorphic(allocator_arg_t, const _Allocator& __a, polymorphic&& __other) noexcept(
      allocator_traits<_Allocator>::is_always_equal::value)
      : __alloc_(__a),
        __p_(__alloc_ == __other.__alloc_ ? std::exchange(__other.__p_, nullptr)
                                          : __other.__vtable_->__move_(__alloc_, __other.__p_)),
        __vtable_(__other.__vtable_) {}

  template <class _U = _Tp, class _UU = remove_cvref_t<_U>>
    requires(!is_same_v<_UU, polymorphic> && derived_from<_UU, _Tp> && is_constructible_v<_UU, _U> &&
             is_copy_constructible_v<_UU> && !__is_inplace_type<_UU>::value && is_default_constructible_v<_Allocator>)
  _LIBCPP_HIDE_FROM_ABI constexpr explicit polymorphic(_U&& __u)
      : __p_(__allocate_owned_object<_UU>(__alloc_, std::forward<_U>(__u))),
        __vtable_(std::addressof(__vtable_for<_UU>)) {}

  template <class _U = _Tp, class _UU = remove_cvref_t<_U>>
    requires(!is_same_v<_UU, polymorphic> && derived_from<_UU, _Tp> && is_constructible_v<_UU, _U> &&
             is_copy_constructible_v<_UU> && !__is_inplace_type<_UU>::value)
  _LIBCPP_HIDE_FROM_ABI constexpr explicit polymorphic(allocator_arg_t, const _Allocator& __a, _U&& __u)
      : __alloc_(__a),
        __p_(__allocate_owned_object<_UU>(__alloc_, std::forward<_U>(__u))),
        __vtable_(std::addressof(__vtable_for<_UU>)) {}

  template <class _U, class... _Ts>
    requires(is_same_v<remove_cvref_t<_U>, _U> && derived_from<_U, _Tp> && is_constructible_v<_U, _Ts...> &&
             is_copy_constructible_v<_U> && is_default_constructible_v<_Allocator>)
  _LIBCPP_HIDE_FROM_ABI constexpr explicit polymorphic(in_place_type_t<_U>, _Ts&&... __ts)
      : __p_(__allocate_owned_object<_U>(__alloc_, std::forward<_Ts>(__ts)...)),
        __vtable_(std::addressof(__vtable_for<_U>)) {}

  template <class _U, class... _Ts>
    requires(is_same_v<remove_cvref_t<_U>, _U> && derived_from<_U, _Tp> && is_constructible_v<_U, _Ts...> &&
             is_copy_constructible_v<_U>)
  _LIBCPP_HIDE_FROM_ABI constexpr explicit polymorphic(
      allocator_arg_t, const _Allocator& __a, in_place_type_t<_U>, _Ts&&... __ts)
      : __alloc_(__a),
        __p_(__allocate_owned_object<_U>(__alloc_, std::forward<_Ts>(__ts)...)),
        __vtable_(std::addressof(__vtable_for<_U>)) {}

  template <class _U, class _I, class... _Us>
    requires(is_same_v<remove_cvref_t<_U>, _U> && derived_from<_U, _Tp> &&
             is_constructible_v<_U, initializer_list<_I>&, _Us...> && is_copy_constructible_v<_U> &&
             is_default_constructible_v<_Allocator>)
  _LIBCPP_HIDE_FROM_ABI constexpr explicit polymorphic(in_place_type_t<_U>, initializer_list<_I> __ilist, _Us&&... __us)
      : __p_(__allocate_owned_object<_U>(__alloc_, __ilist, std::forward<_Us>(__us)...)),
        __vtable_(std::addressof(__vtable_for<_U>)) {}

  template <class _U, class _I, class... _Us>
    requires(is_same_v<remove_cvref_t<_U>, _U> && derived_from<_U, _Tp> &&
             is_constructible_v<_U, initializer_list<_I>&, _Us...> && is_copy_constructible_v<_U>)
  _LIBCPP_HIDE_FROM_ABI constexpr explicit polymorphic(
      allocator_arg_t, const _Allocator& __a, in_place_type_t<_U>, initializer_list<_I> __ilist, _Us&&... __us)
      : __alloc_(__a),
        __p_(__allocate_owned_object<_U>(__alloc_, __ilist, std::forward<_Us>(__us)...)),
        __vtable_(std::addressof(__vtable_for<_U>)) {}

  // [polymorphic.dtor], destructor
  _LIBCPP_HIDE_FROM_ABI constexpr ~polymorphic() {
    if (!valueless_after_move()) {
      __vtable_->__destroy_(__alloc_, __p_);
    }
  }

  // [polymorphic.assign], assignment
  _LIBCPP_HIDE_FROM_ABI constexpr polymorphic& operator=(const polymorphic& __other) {
    if (std::addressof(__other) == this)
      return *this;

    static constexpr bool __allocator_needs_updating =
        allocator_traits<_Allocator>::propagate_on_container_copy_assignment::value;
    if (__other.valueless_after_move()) {
      this->~polymorphic();
      __p_ = nullptr;
    } else {
      pointer __new_p =
          __other.__vtable_->__clone_(__allocator_needs_updating ? __other.__alloc_ : __alloc_, __other.__p_);
      this->~polymorphic();
      __p_      = __new_p;
      __vtable_ = __other.__vtable_;
    }

    if constexpr (__allocator_needs_updating)
      __alloc_ = __other.__alloc_;

    return *this;
  }

  _LIBCPP_HIDE_FROM_ABI constexpr polymorphic& operator=(polymorphic&& __other) noexcept(
      allocator_traits<_Allocator>::propagate_on_container_move_assignment::value ||
      allocator_traits<_Allocator>::is_always_equal::value) {
    if (std::addressof(__other) == this)
      return *this;

    static constexpr bool __allocator_needs_updating =
        allocator_traits<_Allocator>::propagate_on_container_move_assignment::value;
    if constexpr (allocator_traits<_Allocator>::is_always_equal::value) {
      std::swap(__p_, __other.__p_);
      std::swap(__vtable_, __other.__vtable_);
      __other.~polymorphic();
      __other.__p_ = nullptr;
    } else if (__alloc_ == __other.__alloc_) {
      std::swap(__p_, __other.__p_);
      std::swap(__vtable_, __other.__vtable_);
      __other.~polymorphic();
      __other.__p_ = nullptr;
    } else if (__other.valueless_after_move()) {
      this->~polymorphic();
      __p_ = nullptr;
    } else {
      pointer __new_p =
          __other.__vtable_->__move_(__allocator_needs_updating ? __other.__alloc_ : __alloc_, __other.__p_);
      this->~polymorphic();
      __p_      = __new_p;
      __vtable_ = __other.__vtable_;
    }

    if constexpr (__allocator_needs_updating)
      __alloc_ = __other.__alloc_;

    return *this;
  }

  // [polymorphic.obs], observers
  [[nodiscard]] _LIBCPP_HIDE_FROM_ABI constexpr const _Tp& operator*() const noexcept {
    _LIBCPP_ASSERT_VALID_ELEMENT_ACCESS(
        !valueless_after_move(), "operator* called on a valueless std::polymorphic object");
    return *__p_;
  }

  [[nodiscard]] _LIBCPP_HIDE_FROM_ABI constexpr _Tp& operator*() noexcept {
    _LIBCPP_ASSERT_VALID_ELEMENT_ACCESS(
        !valueless_after_move(), "operator* called on a valueless std::polymorphic object");
    return *__p_;
  }

  [[nodiscard]] _LIBCPP_HIDE_FROM_ABI constexpr const_pointer operator->() const noexcept {
    _LIBCPP_ASSERT_VALID_ELEMENT_ACCESS(
        !valueless_after_move(), "operator-> called on a valueless std::polymorphic object");
    return __p_;
  }

  [[nodiscard]] _LIBCPP_HIDE_FROM_ABI constexpr pointer operator->() noexcept {
    _LIBCPP_ASSERT_VALID_ELEMENT_ACCESS(
        !valueless_after_move(), "operator-> called on a valueless std::polymorphic object");
    return __p_;
  }

  [[nodiscard]] _LIBCPP_HIDE_FROM_ABI constexpr bool valueless_after_move() const noexcept { return !__p_; }
  [[nodiscard]] _LIBCPP_HIDE_FROM_ABI constexpr allocator_type get_allocator() const noexcept { return __alloc_; }

  // [polymorphic.swap], swap
  _LIBCPP_HIDE_FROM_ABI constexpr void
  swap(polymorphic& __other) noexcept(allocator_traits<_Allocator>::propagate_on_container_swap::value ||
                                      allocator_traits<_Allocator>::is_always_equal::value) {
    _LIBCPP_ASSERT_COMPATIBLE_ALLOCATOR(
        allocator_traits<_Allocator>::propagate_on_container_swap::value || get_allocator() == __other.get_allocator(),
        "swapping std::polymorphic objects with different allocators");
    std::swap(__p_, __other.__p_);
    std::swap(__vtable_, __other.__vtable_);
    std::__swap_allocator(__alloc_, __other.__alloc_);
  }

  _LIBCPP_HIDE_FROM_ABI friend constexpr void
  swap(polymorphic& __lhs, polymorphic& __rhs) noexcept(noexcept(__lhs.swap(__rhs))) {
    __lhs.swap(__rhs);
  }

private:
  struct __vtable {
    pointer (*__clone_)(_Allocator&, const_pointer);
    pointer (*__move_)(_Allocator&, pointer);
    void (*__destroy_)(_Allocator&, pointer) noexcept;
  };

  template <class _U>
  static constexpr __vtable __vtable_for = {
      .__clone_ =
          [](_Allocator& __a, const_pointer __p) static {
            return __allocate_owned_object<_U>(__a, static_cast<const _U&>(std::to_address(__p)));
          },
      .__move_ =
          [](_Allocator& __a, pointer __p) static {
            return __allocate_owned_object<_U>(__a, static_cast<_U&&>(std::to_address(__p)));
          },
      .__destroy_ =
          [](_Allocator& __a, pointer __p) static noexcept {
            allocator_traits<_Allocator>::destroy(__a, static_cast<_U*>(__p));
            allocator_traits<_Allocator>::deallocate(__a, static_cast<_U*>(__p), 1);
          }};

  template <class _U, class... _Us>
  _LIBCPP_HIDE_FROM_ABI static constexpr pointer __allocate_owned_object(_Allocator& __a, _Us&&... __us) {
    using __rebound_allocator = __rebind_alloc<allocator_traits<_Allocator>, _U>;
    __allocation_guard<__rebound_allocator> __guard(__a, 1);
    allocator_traits<__rebound_allocator>::construct(__a, __guard.__get(), std::forward<_Us>(__us)...);
    return __guard.__release_ptr();
  }

  _LIBCPP_NO_UNIQUE_ADDRESS _Allocator __alloc_ = _Allocator();
  pointer __p_;
  __vtable* __vtable_;
};

namespace pmr {

template <class _Tp>
using polymorphic _LIBCPP_AVAILABILITY_PMR = polymorphic<_Tp, polymorphic_allocator<_Tp>>;

} // namespace pmr

_LIBCPP_END_NAMESPACE_STD

#endif // _LIBCPP_STD_VER >= 26

#endif // _LIBCPP___MEMORY_POLYMORPHIC_H
