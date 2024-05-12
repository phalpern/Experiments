/* ctor_args.t.cpp                                                    -*-C++-*-
 *
 * Copyright (C) 2024 Pablo Halpern <phalpern@halpernwightsoftware.com>
 * Distributed under the Boost Software License - Version 1.0
 */

#include <ctor_args.h>
#include <string>
#include <iostream>
#include <cassert>

using namespace std::string_literals;

struct TestType
{
  std::string_view m_s = {};
  int         m_i = 0;
  void       *m_p = nullptr;

  constexpr TestType() : m_s("default") { }
  constexpr explicit TestType(std::string_view&& s) : m_s(std::move(s)) { }
  constexpr explicit TestType(int i, void *p = nullptr) : m_s("none"), m_i(i), m_p(p) { }
  constexpr TestType(int i, const std::string_view& s) : m_s(s), m_i(i) { }
};

int lastIdx = -1;

template <class... Args>
TestType emplace(int idx, xstd::ctor_args<TestType> args)
{
  lastIdx = idx;
  return args();
}

void verify(TestType t, int idx, std::string_view s, int i, const void *p)
{
  assert(lastIdx == idx);
  assert(t.m_s == s);
  assert(t.m_i == i);
  assert(t.m_p == p);
}

struct TTWrapper
{
  TestType m_tt;

  TTWrapper(int, xstd::ctor_args<TestType> args) : m_tt(args()) { }
};

int main()
{
  short v = 6;

  verify(emplace(1, {}), 1, "default", 0, nullptr);
  verify(emplace(2, { "hello" }), 2, "hello", 0, nullptr);
  verify(emplace(3, { 99 }), 3, "none", 99, nullptr);
  verify(emplace(4, { 88, &v }), 4, "none", 88, &v);
  verify(emplace(5, { 77, "goodbye" }), 5, "goodbye", 77, nullptr);
  verify(emplace(6, [v] -> TestType { return { v * 11, "lazy" }; }),
         6, "lazy", 66, nullptr);

  TTWrapper x(0, { 5 });
  assert(x.m_tt.m_i == 5);
}

// Local Variables:
// c-basic-offset: 2
// End:
