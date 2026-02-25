/* friend_access.t.cpp                                                -*-C++-*-
 *
 * Copyright (C) 2024 Pablo Halpern <phalpern@halpernwightsoftware.com>
 * Distributed under the Boost Software License - Version 1.0
 */

#include <iostream>
#include <type_traits>

template <class T> inline constexpr bool is_trivial_fooable_v = false;

template <class T>
class TrivialFoo
{
  friend T;

  TrivialFoo(T *a, T* b) requires(!is_trivial_fooable_v<T>)
    { std::cout << "private TrivialFoo\n"; }

public:
  TrivialFoo(T *a, T* b) requires(is_trivial_fooable_v<T>)
    { std::cout << "public TrivialFoo\n"; }
};

template <class T>
TrivialFoo(T* a, T* b) -> TrivialFoo<T>;

namespace internal {
template <class T>
void do_Foo(T*, T*) { std::cout << "internal::do_Foo\n"; }
}

template <class T>
void Foo(T* a, T* b) { using internal::do_Foo; do_Foo(a, b); }

class X
{
  friend void do_Foo(X* a, X* b)
  {
    std::cout << "Start friend do_Foo\n";
    TrivialFoo(a, b);
    std::cout << "End friend do_Foo\n";
  }
};

class Y
{
};

template <> inline constexpr bool is_trivial_fooable_v<Y> = true;

int main()
{
  X x1, x2;
  Foo(&x1, &x2);
  // TrivialFoo(&x1, &x2);  // Disallowed: X is not trivial_fooable

  Y y1, y2;
  Foo(&y1, &y2);
  TrivialFoo(&y1, &y2);
}

// Local Variables:
// c-basic-offset: 2
// End:
