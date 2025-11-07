/* relocate_construction.t.cpp                                        -*-C++-*-
 *
 * Copyright (C) 2024 Pablo Halpern <phalpern@halpernwightsoftware.com>
 * Distributed under the Boost Software License - Version 1.0
 */

#include <relocate_construction.h>
// #include <relocate_from.h>

#include <new>
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

class X : public counters<X>
{
  int        m_value;

public:
  explicit X(int v = 0) : m_value(v) { }
  X(const X& other) : m_value(other.m_value) { }
  ~X() { std::cout << "~X() "; }

  friend std::ostream& operator<<(std::ostream& os, const X& obj) {
    return os << '{' << obj.m_value << '}';
  }
};

namespace xstd {
template <> struct is_trivially_relocatable<X> : true_type { };
} // close namespace xstd

class Y : public counters<Y>
{
  int m_value;

public:
  explicit Y(int v = 0) : m_value(v) { }
  Y(const Y& other) = default;
  Y(Y&& other) noexcept { m_value = other.m_value; other.m_value = -1; }
  ~Y() { std::cout << "~Y() "; }

  friend std::ostream& operator<<(std::ostream& os, const Y& obj) {
    return os << '{' << obj.m_value << '}';
  }
};

template <class Obj>
void simple_test(const char* objnm)
{
  union ObjBuf { Obj buf; ObjBuf(){} ~ObjBuf(){} } ob;

  {
    std::cout << objnm << ": ";
    auto pa = new(&ob.buf) Obj(5);
    auto b  = xstd::relocate_from(pa);
    std::cout << "| *pa = " << *pa << ", b = " << b << " | ";
  }
  print_counters<Obj>(std::cout) << std::endl;
}

int main()
{
  simple_test<int>("int");
  simple_test<X>("X");
  simple_test<Y>("Y");
}

// Local Variables:
// c-basic-offset: 2
// End:
