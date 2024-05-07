/* range_for_cpp03.t.cpp                                              -*-C++-*-
 *
 * Copyright (C) 2024 Pablo Halpern <phalpern@halpernwightsoftware.com>
 * Distributed under the Boost Software License - Version 1.0
 */

#include <range_for_cpp03.h>
#include <vector>
#include <iostream>
#include <utility>

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
    std::cout << '(' << p.first << ", " << (*p.second-1) << "), ";
  std::cout << std::endl;

  // TBD: Eventually test proxy iterators (but some changes needed first)
}

// Local Variables:
// c-basic-offset: 2
// End:
