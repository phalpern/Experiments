/* relocatatable.h                                                    -*-C++-*-
 *
 * Copyright (C) 2024 Pablo Halpern <phalpern@halpernwightsoftware.com>
 * Distributed under the Boost Software License - Version 1.0
 */

#ifndef INCLUDED_RELOCATATABLE
#define INCLUDED_RELOCATATABLE

#include <type_traits>
#include <concepts>

namespace xstd
{

using namespace std;

// template <class T>
// struct relocation_ref
// {
//   T& m_ref;
// };

template <class T>
struct defaulted_relocation_ref
{
};

/// Trait to determine eligibility for trivial relocation.
/// TBD: the correct way to implement this trait is using reflection, but
/// for the moment, is is true for trivially move-constructible types and for
/// types having a function declaration, `static T& is_eligible_for_TR();`.
template <class T>
inline constexpr bool is_eligible_for_TR_v =
  (is_trivially_move_constructible_v<T> && is_trivially_destructible_v<T>) ||
  requires { { T::is_eligible_for_TR() } -> same_as<T>; };

template <class T>
struct is_eligible_for_TR : bool_constant<is_eligible_for_TR_v<T>> { };

template <class T>
inline constexpr bool is_trivially_relocatable_v =
  is_eligible_for_TR_v<T> &&
  ((is_trivially_move_constructible_v<T> && is_trivially_destructible_v<T>) ||
   requires (defaulted_relocation_ref<T> rr) { T(rr); });

template <class T>
struct is_trivially_relocatable : bool_constant<is_trivially_relocatable_v<T>>
{
};

} // end namespace xstd

#endif // ! defined(INCLUDED_RELOCATATABLE)

// Local Variables:
// c-basic-offset: 2
// End:
