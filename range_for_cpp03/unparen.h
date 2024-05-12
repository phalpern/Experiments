/* unparen.h                                                          -*-C++-*-
 *
 * Copyright (C) 2024 Pablo Halpern <phalpern@halpernwightsoftware.com>
 * Distributed under the Boost Software License - Version 1.0
 */

#ifndef INCLUDED_UNPAREN
#define INCLUDED_UNPAREN

// If invoked with an argument sequence beginning with an open parenthesis,
// remove that (outermost) parenthesis and its matching closing parenthesis,
// unchanged.  Only one pair of parentheses are removed.  Note that if the
// arguments begins with a parenthesized phrase, but the entire argument list
// is not parenthesized, only the first set of parentheses are removed.
// Example invocations:
//..
//  +------------------------+-------------+--------------------------------+
//  | Invocation             | Expansion   | Notes                          |
//  +------------------------+-------------+--------------------------------+
//  | UNPAREN(a, b)          | a, b        |                                |
//  | UNPAREN((a, b))        | a, b        |                                |
//  | UNPAREN((a + b) * c)   | a + b * c   | Probably not what was intended |
//  | UNPAREN(((a + b) * c)) | (a + b) * c | Workaround for previous line   |
//  +------------------------+-------------+--------------------------------+
//..
#define UNPAREN(...) UNPAREN_CLEANUP(UNPAREN_NORMALIZE __VA_ARGS__)

// Whether invoked as `UNPAREN_NORMALIZE(ARGS, ...)` (with parentheses) or
// `UNPAREN_NORMALIZE ARGS, ...` (without parentheses), the resulting
// *normalized* expansion is `UNPAREN_NORMALIZE ARGS, ...`.  Note that the
// second form is not actually a macro expansion but rather the result of *not*
// expanding a function-like macro (because of the absence of parentheses).
#define UNPAREN_NORMALIZE(...) UNPAREN_NORMALIZE __VA_ARGS__

// Invoked as `UNPAREN_CLEANUP(UNPAREN_NORMALIZE ARGS, ...)` and expand it to
// `UNPAREN_CLEANUP(UNPAREN_REMOVE_UNPAREN_NORMALIZE ARGS, ...)`, which, in
// turn, expands to simply `ARGS, ...` (because
// `UNPAREN_REMOVE_UNPAREN_NORMALIZE` expands to nothing).  The indirection
// through `UNPAREN_CLEANUP2` is needed because the splice operator (`##`) is
// eager and it is necessary to expand any macros within the argument list
// before applying the splice.
#define UNPAREN_CLEANUP(...) UNPAREN_CLEANUP2(__VA_ARGS__)
#define UNPAREN_CLEANUP2(...) UNPAREN_REMOVE_ ## __VA_ARGS__

// Expand to nothing.  This expression-like macro is "invoked" as
// `UNPAREN_UNPAREN_NORMALIZE ARGS, ...` and leaves only `ARGS, ...`.
#define UNPAREN_REMOVE_UNPAREN_NORMALIZE

#endif // ! defined(INCLUDED_UNPAREN)

// Local Variables:
// c-basic-offset: 2
// End:
