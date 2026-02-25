/* member_relocate_to.h                                               -*-C++-*-
 *
 * Copyright (C) 2024 Pablo Halpern <phalpern@halpernwightsoftware.com>
 * Distributed under the Boost Software License - Version 1.0
 */

/// This header implmenents traits and a relocation functions consistent with
/// P2786R13, but using a different syntax for warranting trivial
/// relocatability. In this experiment, the presence of a member, `relocate_at`
/// provides a hook for _user defined_ relocation whereas a
/// `default_relocate_at` member function indicates that the relocation should
/// be automatically generated. For types that are eligible for trivial
/// relocation, `default_relocate_at` makes the type trivially relocatable,
/// whereas for types that are not eligible, `default_relocate_at` causes
/// relocation to be expressed as a move-destroy combination.
///
/// This approach is superior to the `T(default_relocation_ref<T>)` constructor
/// in that it does not require a header when declaring the aspirationally
/// trivially relocatable class.

#ifndef INCLUDED_MEMBER_RELOCATE_AT
#define INCLUDED_MEMBER_RELOCATE_AT

#include <memory>

namespace xstd {

using namespace std;

/// Trait to determine eligibility for trivial relocation.  TBD: In the absence
/// of compiler support, this trait is mostly user-warranted. The correct
/// way to implement this trait is using reflection or a builtin, but for the
/// moment, is is true for trivially move-constructible types and for types
/// having a function declaration, `static T& is_eligible_for_TR();`.
template <class T>
inline constexpr bool __is_eligible_for_TR_v =
  (is_trivially_move_constructible_v<T> && is_trivially_destructible_v<T>) ||
  requires { { T::is_eligible_for_TR() } -> same_as<T>; };

template <class T>
struct __is_eligible_for_TR : bool_constant<__is_eligible_for_TR_v<T>> { };

/// A trivially relocatable type is either implicitly trivially relocatable or
/// is eligible for TR _and_ has a `default_relocate_at` member function (no
/// body necessary).  A class having a `default_relocate_at` member function
/// but is not eligible for TR, will be relocated using move-destroy. If it
/// also has a deleted or throwing move constructor, it will not be
/// relocatable.
template <class T>
inline constexpr bool is_trivially_relocatable_v =
  is_eligible_for_TR_v<T> &&
  ((is_trivially_move_constructible_v<T> && is_trivially_destructible_v<T>) ||
   requires requires (T& from, T* to) { from.default_relocate_at(to); })

template <class T>
struct is_trivially_relocatable : bool_constant<is_trivially_relocatable_v<T>>
{
};

/// Relocate an object whose of type having a `relocate_at` member function.
/// We mandate that member `relocate_at` be `noexcept`.
/// A member `relocate_at` can use any allowed mechanism, including private
/// constructors leaving the object in an incomplete state, to achieve general
/// relocation. The order of ending member object lifetimes remains a problem,
/// however. Possibly, this approach could be combined with the `relocate_from`
/// mechanism, perhaps with some variation of `relocate_from` that delays
/// calling the destructor/vacuous destructor.
template <class T>
requires requires (T& from, T* to) { from.relocate_at(to); }
constexpr T& relocate_at(T* to, T& from) noexcept
{
  static_assert(noexcept(noexcept(from.relocate_at(to))),
                "Member `relocate_at` must be `noexcept`");
  from.relocate_at(to);
  return to;
}

/// Relocate a trivially-relocatable type. If a type is both generally
/// relocatable and trivially relocatable, this will deliberately result in an
/// overload ambiguity. Perhaps TR should take precidence, so that a class that
/// might not be TR can supply both a defaulted and not-defaulted
/// `relocate_at`.
template <class T>
requires (is_trivially_relocatable_v<T>)
/* constexpr? */ T& relocate_at(T* to, T& from) noexcept
{
  trivially_relocate(addressof(from), addressof(from) + 1, to);
  return to;
}

/// Relocate a nothrow-movable type by move-construction of the new item
/// followed by destruction of the old. This overload is called for types that
/// are neither trivially relocatable nor have a `relocate_at` member function.
template <class T>
requires (is_nothrow_move_constructible_v<T> && is_nothrow_destructible_v<T>)
constexpr T& relocate_at(T* to, T& from) noexcept
{
  to = construct_at(to, std::move(from));
  from.~T();
  return to;
}

/// TBD: Would it be possible, using reflection, to implement
/// memberwise-relocation for classes that are not eligible for trivial
/// relocatation but do have a `default_relocate_at` member function? I don't
/// think it is possible to generate a member-initialization list, but possibly
/// using `make_uninitialized` (with some UB thrown in).

template <class T>
requires (is_trivially_relocatable<T>)
T* relocate(T* start, T* finish, T* dest)
{
  trivially_relocate(start, finish, dest);
}

template <class T>
requires (! is_trivially_relocatable<T>)
constexpr T* relocate(T* start, T* finish, T* dest)
{
  if (dest == start)
    return finish;
  else if (start <= dest && dest < finish) {
    dest += (finish - start);
    for (T* cursor = dest; finish != start; )
      relocate_at(--cursor--, *--finish);
  }
  else {
    while (start != finish)
      relocate_at(dest++, *start++);
  }

  return dest;
}

} // close namespace xstd

#endif // ! defined(INCLUDED_MEMBER_RELOCATE_AT)

// Local Variables:
// c-basic-offset: 2
// End:
