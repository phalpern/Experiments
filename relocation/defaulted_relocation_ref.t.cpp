/* relocatable.t.cpp                                                  -*-C++-*-
 *
 * Copyright (C) 2024 Pablo Halpern <phalpern@halpernwightsoftware.com>
 * Distributed under the Boost Software License - Version 1.0
 */

#include <defaulted_relocation_ref.h>
#include <iostream>

/// CRTP Class that counts constructors and destructors for `T`
template <class T>
class counters
{
  static int s_ctors; // Number of ctor calls
  static int s_dtors; // Number of dtor calls

public:
  counters() { ++s_ctors; }
  counters(const counters&) { ++s_ctors; }
  ~counters() { ++s_dtors; }

  static int ctors() { return s_ctors; }
  static int dtors() { return s_dtors; }

  static std::ostream& print_counters(std::ostream& os) {
    return os << "ctors() = " << ctors() << ", dtors() = " << dtors() << ' ';
  }
};

template <class T> int counters<T>::s_ctors = 0;
template <class T> int counters<T>::s_dtors = 0;

template <class T>
requires requires(std::ostream& os) { T::print_counters(os); }
std::ostream& print_counters(std::ostream& os)
{
  return T::print_counters(os);
}

template <class T>
std::ostream& print_counters(std::ostream& os)
{
  return os;
}

// Trivially copyable, hence TR
struct W
{
  int v;
};

// Not trivially destructible, hence not TR.
// TBD, this should be eligible for TR, but we don't have the reflection
// mechanism to detect that in this version.
struct X : counters<X>
{
  int v;

public:
  ~X() { }
};

// Declared eligible for TR, but does not have a defaulted relocation ctor,
// hence not TR.
class Y : counters<Y>
{
  int v;

public:
  static Y is_eligible_for_TR();

  ~Y() { }
};

// Declared eligible for TR and has a defaulted relocation ctor, hence TR.
class Z : counters<Z>
{
  int v;

public:
  static Z is_eligible_for_TR();

  Z();
  Z(xstd::defaulted_relocation_ref<Z>);

  ~Z() { }
};

template <class T>
class CTR : counters<CTR<T>>
{
  T m_v;

public:
  static CTR is_eligible_for_TR();

  CTR(const T& v = {}) : m_v(v) { }
  CTR(xstd::defaulted_relocation_ref<CTR>)
    requires (xstd::is_trivially_relocatable_v<T>);
};

static_assert(  xstd::is_eligible_for_TR_v<int>);
static_assert(  xstd::is_eligible_for_TR_v<W>);
static_assert(! xstd::is_eligible_for_TR_v<X>);
static_assert(  xstd::is_eligible_for_TR_v<Y>);
static_assert(  xstd::is_eligible_for_TR_v<Z>);

static_assert(  xstd::is_trivially_relocatable_v<int>);
static_assert(  xstd::is_trivially_relocatable_v<W>);
static_assert(! xstd::is_trivially_relocatable_v<X>);
static_assert(! xstd::is_trivially_relocatable_v<Y>);
static_assert(  xstd::is_trivially_relocatable_v<Z>);

static_assert(  xstd::is_trivially_relocatable_v<CTR<int>>);
static_assert(  xstd::is_trivially_relocatable_v<CTR<W>>);
static_assert(! xstd::is_trivially_relocatable_v<CTR<X>>);
static_assert(! xstd::is_trivially_relocatable_v<CTR<Y>>);
static_assert(  xstd::is_trivially_relocatable_v<CTR<Z>>);

int main()
{
}

// Local Variables:
// c-basic-offset: 2
// End:
