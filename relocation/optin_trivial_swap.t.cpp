/* opt_in_trivial_swap.cpp                                            -*-C++-*-
 *
 * Copyright (C) 2024 Pablo Halpern <phalpern@halpernwightsoftware.com>
 * Distributed under the Boost Software License - Version 1.0
 */

#include <algorithm>
#include <iostream>
#include <cassert>

namespace xstd {

using namespace std;

template <class T>
class swap_value_representations
{
    friend T;

    // Private; callable from within `T` only.
    static void apply(T* a, T* b)
        { std::swap(*a, *b); cout << "trivial swap\n"; }

    // Not constructible
    swap_value_representations() = delete;

    // No public members
};

}  // close namespace xstd

class X
{
    int m_i;

  public:
    explicit X(int i = 0) : m_i(i) { }

    int value() const { return m_i; }

    void swap(X& b) { xstd::swap_value_representations<X>::apply(this, &b); }
};

void swap(X& a, X& b) { a.swap(b); }

int main()
{
    X x1(1), x2(2);

    // xstd::swap_value_representations<X>::apply(&x1, &x2);
    // assert(x1.value() == 2);
    // assert(x2.value() == 1);

    using std::swap;
    swap(x1, x2);
    assert(x1.value() == 2);
    assert(x2.value() == 1);
}


// Local Variables:
// c-basic-offset: 2
// End:
