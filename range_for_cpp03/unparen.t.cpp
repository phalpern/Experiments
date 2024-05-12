/* unparen.cpp                                                        -*-C++-*-
 *
 * Copyright (C) 2024 Pablo Halpern <phalpern@halpernwightsoftware.com>
 * Distributed under the Boost Software License - Version 1.0
 */

// Compile using the preprocessor only as:
// g++ -std=c++03 -I. -Wall -E unparen.t.cpp

#include <unparen.h>

UNPAREN(a)
UNPAREN(a, b, c)
UNPAREN((a, b, c))
UNPAREN((a + b) * c)
UNPAREN(((a + b) * c))

#define DO(X, Y) { UNPAREN(X) : UNPAREN(Y) }
DO(A, P)
DO((A, B), P)
DO(A, (P, Q))
DO((A, B), (P, Q))

// Local Variables:
// c-basic-offset: 2
// End:
