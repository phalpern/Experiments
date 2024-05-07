/* range_for_cpp03.h                                                  -*-C++-*-
 *
 * Copyright (C) 2024 Pablo Halpern <phalpern@halpernwightsoftware.com>
 * Distributed under the Boost Software License - Version 1.0
 */

#ifndef INCLUDED_RANGE_FOR_CPP03
#define INCLUDED_RANGE_FOR_CPP03

#include <cstdlib>
#include <unparen.h>

// Implement range-based for loop that works with C++03
//
// Supports:
//  standard containers
//  C-style arrays
//  const ranges
//  range-like classes that lack `iterator` types
//  TBD: proxy iterators
//
// Limitations:
//  Range must be an lvalue (but can be const)
//  No support for `auto` variables in C++03 (of course)
//  No support for structured binding loop variables in C++03 (of course)
//  No support for C++20 lifetime extension of subparts of range expression
//  No support for C++23 initialization clause

template <class T> struct Decl;

// TBD: Make `Decl` return `T` here
template <class T>
struct Decl<void (&)(T)> { typedef const T type; };

// TBD: Make `Decl` return `T&` here
template <class T>
struct Decl<void (&)(T&)> { typedef T type; };

// TBD: Make `Decl` return `const T&` here
template <class T>
struct Decl<void (&)(const T&)> { typedef const T type; };

template <class T> struct __rm_const          { typedef T type; };
template <class T> struct __rm_const<const T> { typedef T type; };

template <class T> T& __unconst(T& v)       { return v; }
template <class T> T& __unconst(const T& v) { return const_cast<T&>(v); }

// TBD: Change `T&` to `DerefT` and allow it to either an lvalue ref or an
// rvalue non-ref.
template <class T>
class _VirtIterBase
{
public:
  virtual T& operator*() const = 0;
  virtual void operator++() const = 0;
  virtual bool notAtEnd() const = 0;
};

// TBD: Change `T&` to `DerefT` and allow it to either an lvalue ref or an
// rvalue non-ref.
template <class T, class Range>
class _VirtIter : public _VirtIterBase<T>
{
  Range&                           m_range;
  mutable typename Range::iterator m_current;

public:
  _VirtIter(Range& r) : m_range(r), m_current(__unconst(m_range).begin()) { }

  T& operator*() const { return *m_current; }
  void operator++() const { ++m_current; }
  bool notAtEnd() const { return m_current != m_range.end(); }
};

template <class T, class Range>
class _VirtIter<T, const Range> :
  public _VirtIter<const T, typename __rm_const<Range>::type>
{
  typedef typename __rm_const<Range>::type  NonconstRange;
  typedef _VirtIter<const T, NonconstRange> Base;

public:
  _VirtIter(Range& r) : Base(__unconst(r)) { }
};

template <class T, class A, std::size_t SZ >
class _VirtIter<T, A[SZ]> : public _VirtIterBase<T>
{
  mutable T *m_current;
  T         *m_end;

public:
  _VirtIter(T (&r)[SZ]) : m_current(r), m_end(&r[SZ]) { }

  T& operator*() const { return *m_current; }
  void operator++() const { ++m_current; }
  bool notAtEnd() const { return m_current != m_end; }
};

template <class T, class A, std::size_t SZ >
class _VirtIter<T, const A[SZ]> :
  public _VirtIter<const T, typename __rm_const<T>::type[SZ]>
{
  typedef typename __rm_const<T>::type      NonconstT;
  typedef _VirtIter<const T, NonconstT[SZ]> Base;

public:
  _VirtIter(const T (&a)[SZ]) : Base(__unconst(a)) { }
};

template <class T, class Range>
_VirtIter<T, Range> mkVirt(Range& r)
{
  return _VirtIter<T, Range>(__unconst(r));
}

#define RANGE_FOR(RangeDecl, ...)                                            \
  for (const _VirtIterBase<Decl<void(&)(UNPAREN(RangeDecl))>::type>& _Iter = \
         mkVirt<Decl<void(&)(UNPAREN(RangeDecl))>::type>(__VA_ARGS__);  \
       _Iter.notAtEnd(); )                                              \
    for (bool _Top = true; _Iter.notAtEnd(); ++_Iter, _Top = true)      \
      for (UNPAREN(RangeDecl) = *_Iter; _Top; _Top = false)

#endif // ! defined(INCLUDED_RANGE_FOR_CPP03)

// Local Variables:
// c-basic-offset: 2
// End:
