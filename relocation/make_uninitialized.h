/* make_uninitialized.h
 *
 * Copyright (C) 2024 Pablo Halpern <phalpern@halpernwightsoftware.com>
 * Distributed under the Boost Software License - Version 1.0
 */

#ifndef INCLUDED_MAKE_UNINITIALIZED
#define INCLUDED_MAKE_UNINITIALIZED

#include <type_traits>
#include <utility>

namespace xstd {

using namespace std;

/// This dummy class has two constructors; one that does
/// nothing, and one that invokes a user-specified function to initialize the
/// object.  Since this dummy class has no data, itself, no compiler-generated
/// initialization takes place.  By aliasing this type over the address of an
/// object of a different type, we can bypass the other type's constructor and
/// invoke whatever operation we want on the raw storage. This approach might
/// or might not work if the other type is trivially copyable.
struct internal_dummy_type
{
  // non-trivially-copyable dummy type.

  constexpr internal_dummy_type() { }

  template <class F>
  constexpr internal_dummy_type(F&& emplacer)
    { std::forward<F>(emplacer)(this); }
  constexpr ~internal_dummy_type() { }  // not trivial

  constexpr static internal_dummy_type default_factory() { return { }; }

  template <class F>
  constexpr static internal_dummy_type emplace_factory(F&& emplacer)
    { return { std::forward<F>(emplacer) }; }
};

template <class T>
requires (std::is_trivially_default_constructible_v<T>)
inline T make_uninitialized()
{
  union uninit { T v; constexpr uninit() {} };
  return uninit{}.v;
}

template <class T>
requires (std::is_trivially_copyable_v<T> &&
          !std::is_trivially_default_constructible_v<T>)
inline T make_uninitialized()
{
  union Tu { char m_c; T m_t; Tu() { }; ~Tu() { } };
  return Tu().m_t;
}

template <class T>
inline T make_uninitialized()
{
  using T_factory = T (&)();
  return reinterpret_cast<T_factory>(internal_dummy_type::default_factory)();
}

template <class T, class F>
requires (std::is_trivially_copyable_v<T>)
inline T make_uninitialized(F&& emplacer)
{
  union Tu { char m_c; T m_t; Tu() { }; ~Tu() { } };
  Tu ret_u;
  std::forward<F>(emplacer)(&ret_u.m_t);
  return ret_u.m_t;
}

template <class T, class F>
inline T make_uninitialized(F&& emplacer)
{
  using T_factory = T (&)(F&&);
  internal_dummy_type (&factory)(F&&) = internal_dummy_type::emplace_factory<F>;
  return reinterpret_cast<T_factory>(factory)(std::forward<F>(emplacer));
}

} // close namespace xstd

#endif // INCLUDED_MAKE_UNINITIALIZED

// Local Variables:
// c-basic-offset: 2
// End:
