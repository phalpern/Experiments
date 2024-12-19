/* conditionally_trivial.cpp                                          -*-C++-*-
 *
 * Copyright (C) 2024 Pablo Halpern <phalpern@halpernwightsoftware.com>
 * Distributed under the Boost Software License - Version 1.0
 */

#include <type_traits>

/// This empty struct is trivial iff `C` is `true`.
// template <bool C>
// struct maybe_trivial
// {
//   maybe_trivial(const maybe_trivial&) noexcept requires ( C) = default;
//   maybe_trivial(const maybe_trivial&) noexcept requires (!C) { }
// };

/// This empty struct is trivial iff `C` is `true`.
template <bool C> struct maybe_trivial        {                      };
template <>       struct maybe_trivial<false> { ~maybe_trivial() { } };

static_assert(  std::is_trivially_move_constructible_v<maybe_trivial<true>>);
static_assert(! std::is_trivially_move_constructible_v<maybe_trivial<false>>);

static_assert(  std::is_trivial_v<maybe_trivial<true>>);
static_assert(! std::is_trivial_v<maybe_trivial<false>>);

template <bool C> struct Foo { ~Foo() requires(! C) { } };

static_assert(  std::is_destructible_v<Foo<true>>);
static_assert(! std::is_trivially_destructible_v<Foo<false>>);

#if 0

template <class T>
class optional memberwise_trivially_relocatable :
  private maybe_trivial<is_trivially_relocatable_v<T>>
{
  bool            m_engaged;
  byte alignas(T) m_buffer[sizeof(T)];

 public:
  // ...
  optional(optional&& other) : m_engaged(other.m_engaged)
  {
    if (m_engaged)
      ::new (m_buffer) T(std::move(*other));
  }

  // ...
};

#endif

// Local Variables:
// c-basic-offset: 2
// End:
