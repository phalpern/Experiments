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
//  proxy iterators
//
// Limitations:
//  Range must be an lvalue (but can be const)
//  No support for `auto` variables in C++03 (of course)
//  No support for structured binding loop variables in C++03 (of course)
//  No support for C++20 lifetime extension of subparts of range expression
//  No support for C++23 initialization clause

struct false_type { enum { value = false }; };
struct true_type  { enum { value = true }; };
template <bool Cond, class T = void> struct enable_if;
template <class T> struct enable_if<true, T> { typedef T type; };

template <class>
struct copyable_range : false_type { };

template <class T> struct Decl;

template <class T>
struct Decl<void (&)(T)> { typedef const T type; };

template <class T>
struct Decl<void (&)(T&)> { typedef T& type; };

template <class T>
struct Decl<void (&)(const T&)> { typedef const T& type; };

template <class T> struct __rm_const          { typedef T type; };
template <class T> struct __rm_const<const T> { typedef T type; };

template <class T> T& __unconst(T& v)       { return v; }
template <class T> T& __unconst(const T& v) { return const_cast<T&>(v); }

template <class T>
class _VirtIterBase
{
public:
  virtual T operator*() const = 0;
  virtual void operator++() const = 0;
  virtual bool notAtEnd() const = 0;
};

template <class T, class Range>
class _VirtIter : public _VirtIterBase<T>
{
  Range&                           m_range;
  mutable typename Range::iterator m_current;

public:
  _VirtIter(Range& r) : m_range(r), m_current(__unconst(m_range).begin()) { }

  T operator*() const { return *m_current; }
  void operator++() const { ++m_current; }
  bool notAtEnd() const { return m_current != m_range.end(); }
};

template <class T, class Range>
class _VirtIter<T, const Range> :
  public _VirtIter<T, typename __rm_const<Range>::type>
{
  typedef typename __rm_const<Range>::type  NonconstRange;
  typedef _VirtIter<T, NonconstRange> Base;

public:
  _VirtIter(Range& r) : Base(__unconst(r)) { }
};

template <class T, class A, std::size_t SZ >
class _VirtIter<T, A[SZ]> : public _VirtIterBase<T>
{
  mutable A *m_current;
  A         *m_end;

public:
  _VirtIter(A (&r)[SZ]) : m_current(r), m_end(&r[SZ]) { }

  T operator*() const { return *m_current; }
  void operator++() const { ++m_current; }
  bool notAtEnd() const { return m_current != m_end; }
};

template <class T, class A, std::size_t SZ >
class _VirtIter<T, const A[SZ]> :
  public _VirtIter<T, typename __rm_const<A>::type[SZ]>
{
  typedef typename __rm_const<A>::type NonconstA;
  typedef _VirtIter<T, NonconstA[SZ]>  Base;

public:
  _VirtIter(A (&a)[SZ]) : Base(__unconst(a)) { }
};

template <class T, class Range>
class _VirtIterCp : public _VirtIterBase<T>
{
  Range                            m_range;
  mutable typename Range::iterator m_current;

public:
  _VirtIterCp(const Range& r)
    : m_range(r), m_current(__unconst(m_range).begin()) { }

  T operator*() const { return *m_current; }
  void operator++() const { ++m_current; }
  bool notAtEnd() const { return m_current != m_range.end(); }
};

template <class T, class Range>
_VirtIter<T, Range> mkVirt(Range& r)
{
  return _VirtIter<T, Range>(__unconst(r));
}

template <class T, class Range>
typename enable_if<copyable_range<Range>::value, _VirtIterCp<T, Range> >::type
mkVirt(const Range& r)
{
  return _VirtIterCp<T, Range>(r);
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
