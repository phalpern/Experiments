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
//  proxy iterators
//
// Limitations:
//  Range must be an lvalue (but can be const), unless the `transient_range`
//    trait is set (manually) to true.
//  No support for `auto` variables in C++03 (of course)
//  No support for structured binding loop variables in C++03 (of course)
//  No support for C++20 lifetime extension of subparts of range expression
//  No support for C++23 initialization clause

template <class T, T V> struct integral_constant { static const T value = V; };
template <class T, T V> const T integral_constant<T, V>::value;

struct false_type : integral_constant<bool, false> { };
struct true_type  : integral_constant<bool, true>  { };

template <bool Cond, class T = void> struct enable_if;
template <class T> struct enable_if<true, T> { typedef T type; };

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

template <std::size_t SZ>
union _AlignedBuf
{
  long double m_ldouble;
  long long   m_ll;
  void*       m_ptr;
  void      (*m_fp)();
  int         _AlignedBuf::*m_mdp;
  char        data[SZ];
};

template <class>
struct transient_range : false_type { };

template <class T, std::size_t begIterSz, std::size_t endIterSz>
class _RngForIter
{
  _AlignedBuf<begIterSz> m_begIter;
  _AlignedBuf<endIterSz> m_endIter;
  T                    (*m_deref)(char *begIterData);
  void                 (*m_increment)(char *begIterData);
  bool                 (*m_iterEQ)(const char *begIterData,
                                   const char *endIterData);

  template <class Iter>
  static T deref(char *begIterData);

  template <class Iter>
  static void increment(char *begIterData);

  template <class begIter, class endIter>
  static bool iterEQ(const char *begIterData, const char *endIterData);

  template <class begIter, class endIter>
  void init(begIter b, endIter e);

public:
  template <class RngArg>
  explicit _RngForIter(RngArg& range) { init(range.begin(), range.end()); }

  template <class RngArg>
  explicit
  _RngForIter(const RngArg& range,
              typename enable_if<transient_range<RngArg>::value, int>::type=0)
    { init(range.begin(), range.end()); }

  // All member functions are `const` because they are invoked via a
  // lifetime-extended reference.
  T operator*() const { return m_deref(__unconst(m_begIter).data); }
  void operator++() const { m_increment(__unconst(m_begIter).data); }
  bool notAtEnd() const { return ! m_iterEQ(m_begIter.data, m_endIter.data); }
};

template <class T, std::size_t begIterSz, std::size_t endIterSz>
template <class begIter, class endIter>
inline
void _RngForIter<T, begIterSz, endIterSz>::init(begIter b, endIter e)
{
  ::new(m_begIter.data) begIter(b);
  ::new(m_endIter.data) endIter(e);

  m_deref     = deref<begIter>;
  m_increment = increment<begIter>;
  m_iterEQ    = iterEQ<begIter, endIter>;
}

template <class T, std::size_t begIterSz, std::size_t endIterSz>
template <class Iter>
T _RngForIter<T, begIterSz, endIterSz>::deref(char *begIterData)
{
  return **reinterpret_cast<Iter*>(begIterData);
}

template <class T, std::size_t begIterSz, std::size_t endIterSz>
template <class Iter>
void _RngForIter<T, begIterSz, endIterSz>::increment(char *begIterData)
{
  ++*reinterpret_cast<Iter*>(begIterData);
}

template <class T, std::size_t begIterSz, std::size_t endIterSz>
template <class begIter, class endIter>
bool _RngForIter<T, begIterSz, endIterSz>::iterEQ(const char *begIterData,
                                                  const char *endIterData)
{
  return (*reinterpret_cast<const begIter*>(begIterData) ==
          *reinterpret_cast<const endIter*>(endIterData));
}

template <class T, std::size_t begIterSz, std::size_t endIterSz, class Range>
_RngForIter<T, begIterSz, endIterSz> mkRngForIter(Range& r)
{
  return _RngForIter<T, begIterSz, endIterSz>(r);
}

template <class T, std::size_t begIterSz, std::size_t endIterSz, class Range>
typename enable_if<transient_range<Range>::value,
                   _RngForIter<T, begIterSz, endIterSz> >::type
mkRngForIter(const Range& r)
{
  return _RngForIter<T, begIterSz, endIterSz>(r);
}

// Adaptor that makes an array behave like a range, with begin() & end()
// methods.
template <class T>
class ArrayAdaptor
{
  T* m_begin;
  T* m_end;

public:
  ArrayAdaptor(T* b, std::size_t sz) : m_begin(b), m_end(b + sz) { }

  T* begin() const { return m_begin; }
  T* end()   const { return m_end; }
};

template <class T> struct transient_range<ArrayAdaptor<T> > : true_type { };

// No-op adaptation of a range.
template <class R>
R& adaptRng(R& r) { return r; }

// No-op adaptation of a range.  This overload will work with rvalue ranges
// and `const` ranges.
template <class R>
const R& adaptRng(const R& r) { return r; }

// Adapt an array to be a range
template <class T, std::size_t Sz>
ArrayAdaptor<T> adaptRng(T (&r)[Sz]) { return ArrayAdaptor<T>(r, Sz); }

#define RANGE_FOR(RangeDecl, ...)                                       \
  for (_RngForIter<Decl<void(&)(UNPAREN(RangeDecl))>::type,             \
         sizeof(adaptRng(__VA_ARGS__).begin()),                         \
         sizeof(adaptRng(__VA_ARGS__).end())>                           \
         __iter(adaptRng(__VA_ARGS__));                                 \
       __iter.notAtEnd(); )                                             \
    for (bool _Top = true; __iter.notAtEnd(); ++__iter, _Top = true)    \
      for (UNPAREN(RangeDecl) = *__iter; _Top; _Top = false)

#endif // ! defined(INCLUDED_RANGE_FOR_CPP03)

// Local Variables:
// c-basic-offset: 2
// End:
