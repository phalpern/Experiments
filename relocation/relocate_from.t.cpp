/* relocate_construction.t.cpp                                        -*-C++-*-
 *
 * Copyright (C) 2024 Pablo Halpern <phalpern@halpernwightsoftware.com>
 * Distributed under the Boost Software License - Version 1.0
 */

// #include <relocate_construction.h>  // OLD
#include <relocate_from.h>

#include <new>
#include <iostream>
#include <cstring>
#include <string>

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

  // Relocation is not counted as construction or destruction.
  xstd::relocate_default relocate_from(xstd::relocate_ref<counters>);

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

  // Relocate memberwise, without calling the copy constructor or destructor.
  xstd::relocate_default relocate_from(xstd::relocate_ref<X>);

  int value() const { return m_value; }

  friend std::ostream& operator<<(std::ostream& os, const X& obj) {
    return os << '{' << obj.m_value << '}';
  }
};

class Y : public counters<Y>
{
  int m_value;

public:
  explicit Y(int v = 0) : m_value(v) { }
  Y(const Y& other) = default;
  Y(Y&& other) noexcept { m_value = other.m_value; other.m_value = -1; }
  ~Y() { std::cout << "~Y() "; }

  int value() const { return m_value; }

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
    auto b  = xstd::relocate_from(*pa);
    std::cout << "| *pa = " << *pa << ", b = " << b << " | ";
  }
  print_counters<Obj>(std::cout) << std::endl;
}

/// Class with a mix of subobjects for testing `__memberwise_relocate`.  The
/// `Y` subobjects are not trivially relocatable and should be relocated
/// individually; the rest should be skipped.
struct Z : Y, X
{
  int&       m_ref;
  int        m_int;
  unsigned   m_bits : 3;
  const Y    m_cy;
  Y          m_y;

  explicit Z(int& r)
    : Y(1), X(2), m_ref(r), m_int(3), m_bits(5), m_cy(4), m_y(6) { }
};

/// Test `__memberwise_relocate` on `Z`, returning `true` on success.
bool memberwise_test()
{
  union ZBuf { Z z; ZBuf(){} ~ZBuf(){} } from, to;

  std::cout << "Z (memberwise): ";
  int i = 0;
  ::new(&from.z) Z(i);

  int yCtors = Y::ctors(), yDtors = Y::dtors();
  int xCtors = X::ctors(), xDtors = X::dtors();

  // Skipped subobjects must be copied by the caller.
  std::memcpy(static_cast<void*>(&to.z), &from.z, sizeof(Z));
  xstd::__memberwise_relocate(&from.z, &to.z);

  // Each of the three `Y` subobjects is move-constructed and then destroyed;
  // the `X` base has defaulted relocation, so no `X` constructor or
  // destructor is called.
  const Z& z = to.z;
  bool ok = (Y::ctors() - yCtors == 3 && Y::dtors() - yDtors == 3 &&
             X::ctors() == xCtors     && X::dtors() == xDtors     &&
             z.Y::value() == 1 && z.X::value() == 2 && &z.m_ref == &i &&
             z.m_int == 3 && z.m_bits == 5 &&
             z.m_cy.value() == 4 && z.m_y.value() == 6);
  std::cout << "| " << (ok ? "PASS" : "FAIL") << " | ";

  to.z.~Z();
  std::cout << std::endl;
  return ok;
}

/// Compile-time tests for `__has_user_declared_move` and
/// `__calc_relocation_type`.
namespace trait_tests {

using xstd::__has_user_declared_move;
using xstd::__relocation_def_t;
using enum xstd::__relocation_def_t;

template <class T>
constexpr __relocation_def_t R = xstd::__calc_relocation_type<T>();

struct Implicit        { std::string s; };
struct UserMove        { UserMove(UserMove&&); };
struct UserCopy        { UserCopy(const UserCopy&); };
struct DefaultedMove   { DefaultedMove(DefaultedMove&&) = default; };
class  PrivateMove     { PrivateMove(PrivateMove&&); public: PrivateMove(int); };
class  PrivCopyPubMove {
  PrivCopyPubMove(const PrivCopyPubMove&);
public:
  PrivCopyPubMove(PrivCopyPubMove&&);
};
struct DeletedMove     { DeletedMove(DeletedMove&&) = delete; };
struct DeletedCopyOnly { DeletedCopyOnly(const DeletedCopyOnly&) = delete; };
struct OnlyDtor        { ~OnlyDtor(); };  // Implicit copy, no move
struct HasUM           { int i; UserMove m; };
union  UTriv           { int i; float f; };

// `__has_user_declared_move`
static_assert(! __has_user_declared_move<int>());
static_assert(! __has_user_declared_move<Implicit>());
static_assert(  __has_user_declared_move<UserMove>());
static_assert(! __has_user_declared_move<const UserMove>());  // Not movable
static_assert(  __has_user_declared_move<UserCopy>());
static_assert(  __has_user_declared_move<const UserCopy>());
static_assert(  __has_user_declared_move<DefaultedMove>());
static_assert(! __has_user_declared_move<PrivateMove>());
static_assert(  __has_user_declared_move<PrivCopyPubMove>());
static_assert(! __has_user_declared_move<DeletedMove>());
static_assert(! __has_user_declared_move<DeletedCopyOnly>());
static_assert(! __has_user_declared_move<OnlyDtor>());
static_assert(! __has_user_declared_move<HasUM>());
static_assert(! __has_user_declared_move<UTriv>());

struct Del {
  xstd::relocate_delete relocate_from(xstd::relocate_ref<Del>);
};
struct Plain       { int i; double d; int& r; unsigned b : 3; };
struct HasConstUM  { const UserMove m[2]; };
struct HasDel      { Del d; };
struct DerivedDel  : Plain, Del { };
struct DerivedUM   : Plain, UserMove { };
struct Nested      { HasUM h; Plain p; };
struct NestedDel   { Nested n; HasDel d; };
union  UNon        { UserMove m; int i; UNon(); ~UNon(); };
struct ArrPlain    { Plain p[3]; };
struct UserDtor    { int i; ~UserDtor(); };
struct HasUserDtor { UserDtor d; int j; };
struct DefaultedRel {
  int i;
  ~DefaultedRel();
  xstd::relocate_default relocate_from(xstd::relocate_ref<DefaultedRel>);
};
struct UMTrivDtor  { UMTrivDtor(UMTrivDtor&&) = default; int i; };

// `__calc_relocation_type`
static_assert(R<int>                   == trivial);
static_assert(R<int&>                  == trivial);
static_assert(R<int[4]>                == trivial);
static_assert(R<void>                  == deleted);
static_assert(R<UserMove>              == move_destroy);
static_assert(R<UserMove[2]>           == memberwise);
static_assert(R<const UserMove[2][3]>  == memberwise);
static_assert(R<UMTrivDtor>            == trivial);
static_assert(R<Del>                   == deleted);
static_assert(R<Plain>                 == trivial);
static_assert(R<const Plain>           == trivial);
static_assert(R<ArrPlain>              == trivial);
static_assert(R<HasUM>                 == memberwise);
static_assert(R<HasConstUM>            == memberwise);
static_assert(R<HasDel>                == deleted);
static_assert(R<DerivedDel>            == deleted);
static_assert(R<DerivedUM>             == memberwise);
static_assert(R<Nested>                == memberwise);
static_assert(R<NestedDel>             == deleted);
static_assert(R<UTriv>                 == trivial);
static_assert(R<UNon>                  == deleted);
static_assert(R<UserDtor>              == memberwise);
static_assert(R<HasUserDtor>           == memberwise);
static_assert(R<DefaultedRel>          == memberwise);

} // close namespace trait_tests

int main()
{
  simple_test<int>("int");
  simple_test<X>("X");
  simple_test<Y>("Y");

  if (! memberwise_test())
    return 1;
}

// Local Variables:
// c-basic-offset: 2
// End:
