/* relocate_from.h                                                    -*-C++-*-
 *
 * Copyright (C) 2024 Pablo Halpern <phalpern@halpernwightsoftware.com>
 * Distributed under the Boost Software License - Version 1.0
 */

#ifndef INCLUDED_RELOCATE_FROM
#define INCLUDED_RELOCATE_FROM

#include <make_uninitialized.h>

#include <utility>
#include <cstring>

namespace xstd {

using namespace std;

template <class T>
struct is_trivially_relocatable
    : conjunction<is_trivially_move_constructible<T>,
                  is_trivially_destructible<T>>
{
};

template <class T>
inline constexpr bool is_trivially_relocatable_v =
    is_trivially_relocatable<T>::value;

/// This overload of `relocate_from` simply uses the trivial move constructor.
template <class T>
requires (is_trivially_move_constructible_v<T> &&
          is_trivially_destructible_v<T>)
T relocate_from(T *p)
{
  return std::move(*p);
}

/// This overload  of `relocate_from` trivially relocates from `*p` to the
/// return-value object.
template <class T>
requires (is_trivially_relocatable_v<T> &&
          ! (is_trivially_move_constructible_v<T> &&
             is_trivially_destructible_v<T>))
T relocate_from(T *p)
{
  struct  trivial_relocator
  {
    void *m_from;
    void operator()(void *to) { std::memcpy(to, m_from, sizeof(T)); }
  };

  return make_uninitialized<T>(trivial_relocator{p});
}

/// This overload of `relocate_from` non-trivially relocates from `*p` to the
/// return-value object.
template <class T>
requires (is_nothrow_move_constructible_v<T> && !is_trivially_relocatable_v<T>)
T relocate_from(T *p)
{
  /// The destructor of this `struct` invokes the destructor for `obj`.
  struct destroyer
  {
    T *obj;
    ~destroyer() { obj->~T(); }
  };

  destroyer tr{p};
  return std::move(*p);
}

} // close namespace xstd


#endif // ! defined(INCLUDED_RELOCATE_FROM)

// Local Variables:
// c-basic-offset: 2
// End:
