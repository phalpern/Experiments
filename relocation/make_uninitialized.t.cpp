/* make_uninitialized.t.cpp                                           -*-C++-*-
 *
 * Copyright (C) 2024 Pablo Halpern <phalpern@halpernwightsoftware.com>
 * Distributed under the Boost Software License - Version 1.0
 */

#include <make_uninitialized.h>
#include <iostream>
#include <new>

/// Trivially default constructible and trivially copyable
class W
{
  int m_value;

public:
  friend std::ostream& operator<<(std::ostream& os, const W& obj)
  {
    return os << "W(" << std::hex << obj.m_value << ')';
  }
};

/// Not trivially default constructible but trivially copyable
class X
{
  int m_value;

public:
  explicit X(int v = 0) : m_value(v) { }
  X(const X& other) = default;
  ~X() = default;

  friend std::ostream& operator<<(std::ostream& os, const X& obj)
  {
    return os << "X(" << std::hex << obj.m_value << ')';
  }
};

/// Trivially default constructible but not trivially copyable
class Y
{
  int m_value;

public:
  Y() = default;
  explicit Y(int v) : m_value(v) { }
  Y(const Y& other) : m_value(other.m_value) { }
  ~Y() { std::cout << "~Y() "; }

  friend std::ostream& operator<<(std::ostream& os, const Y& obj)
  {
    return os << "Y(" << std::hex << obj.m_value << ')';
  }
};

/// Neither trivially default constructible nor trivially copyable
class Z
{
  int m_value;

public:
  explicit Z(int v = 0) : m_value(v) { }
  Z(const Z& other) : m_value(other.m_value) { }
  ~Z() { std::cout << "~Z() "; }

  friend std::ostream& operator<<(std::ostream& os, const Z& obj)
  {
    return os << "Z(" << std::hex << obj.m_value << ')';
  }
};

union UU
{
  int m_int;
  W   m_w;
  X   m_x;
  Y   m_y;
  Z   m_z;

  UU() : m_int(0xdeadbeef) { }
  ~UU() { }
};

int main()
{
  UU a;

  std::cout << "Before int init: " << std::hex << a.m_int << "\n";

  new (&a.m_int) int(xstd::make_uninitialized<int>());
  std::cout << "After int init:  " << std::hex << a.m_int << "\n";

  a.m_int = 0xdeadbeef;
  new (&a.m_w) W(xstd::make_uninitialized<W>());
  std::cout << "After W init:    " << std::hex << a.m_w << "\n";

  a.m_int = 0xdeadbeef;
  new (&a.m_x) X(xstd::make_uninitialized<X>());
  std::cout << "After X init:    " << std::hex << a.m_x << "\n";

  a.m_int = 0xdeadbeef;
  new (&a.m_y) Y(xstd::make_uninitialized<Y>());
  std::cout << "After Y init:    " << std::hex << a.m_y << "\n";

  a.m_int = 0xdeadbeef;
  new (&a.m_z) Z(xstd::make_uninitialized<Z>());
  std::cout << "After Z init:    " << std::hex << a.m_z << "\n";
}

// Local Variables:
// c-basic-offset: 2
// End:
