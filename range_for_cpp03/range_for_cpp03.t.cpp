/* range_for_cpp03.t.cpp                                              -*-C++-*-
 *
 * Copyright (C) 2024 Pablo Halpern <phalpern@halpernwightsoftware.com>
 * Distributed under the Boost Software License - Version 1.0
 */

#include <range_for_cpp03.h>
#include <vector>
#include <iostream>
#include <utility>

class iota
{
  int m_start, m_num, m_stride;

public:
  explicit iota(int e) : m_start(0), m_num(e), m_stride(1) { }
  iota(int s, int n, int d = 1) : m_start(s), m_num(n), m_stride(d) { }

  enum sentinal { };

  class iterator {
    int m_current, m_num, m_stride;
  public:
    explicit iterator(int i, int n, int d)
      : m_current(i), m_num(n), m_stride(d) { }
    int operator*() const { return m_current; }
    iterator& operator++() { m_current += m_stride; --m_num; return *this; }
    friend bool operator==(iterator a, sentinal b) { return 0 == a.m_num; }
    friend bool operator!=(iterator a, sentinal b) { return 0 != a.m_num; }
  };

  iterator begin() const { return iterator(m_start, m_num, m_stride); }
  sentinal end()   const { return sentinal(); }
};

template <>
struct transient_range<iota> : true_type { };

int main()
{
  std::vector<int> v;
  for (int i = 1; i < 128; i <<= 1)
    v.push_back(i);

  RANGE_FOR(int x, v) {
    std::cout << x << ' ';
  }
  std::cout << std::endl;

  RANGE_FOR(int& x, v)
    --x;

  const std::vector<int>& V = v;
  RANGE_FOR(int x, V) {
    std::cout << x << ' ';
  }
  std::cout << std::endl;

  RANGE_FOR(const int& x, v) {
    std::cout << x << ' ';
  }
  std::cout << std::endl;

  RANGE_FOR(const int& x, V) {
    std::cout << x << ' ';
  }
  std::cout << std::endl;

  int a[] = { 1, 2, 3, 5, 7, 11 };
  RANGE_FOR(int x, a) {
    std::cout << x << ' ';
  }
  std::cout << std::endl;

  RANGE_FOR(int& x, a)
    ++x;

  const int (&A)[6] = a;
  RANGE_FOR(const int& x, A)
    std::cout << x << ' ';
  std::cout << std::endl;

  // Test with loop-variable declarations containing a comma.
  std::vector<std::pair<int, int*> > pairvec;
  for (int i = 0; i < 6; ++i)
    pairvec.push_back(std::make_pair(i, &a[i]));

  RANGE_FOR((const std::pair<int, int*>& p), pairvec)
    std::cout << '(' << p.first << ", " << (*p.second-1) << ") ";
  std::cout << std::endl;

  // Test proxy iterators and sentinels
  RANGE_FOR(int x, iota(5))
    std::cout << x << ' ';
  std::cout << std::endl;
  RANGE_FOR(int x, iota(5, 5, -1))
    std::cout << x << ' ';
  std::cout << std::endl;
  RANGE_FOR(int x, iota(9, 5, 0))
    std::cout << x << ' ';
  std::cout << std::endl;
}

// Local Variables:
// c-basic-offset: 2
// End:
