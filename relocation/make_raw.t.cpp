/* make_raw.t.cpp                                           -*-C++-*-
 *
 * Copyright (C) 2024 Pablo Halpern <phalpern@halpernwightsoftware.com>
 * Distributed under the Boost Software License - Version 1.0
 */

#include <make_raw.h>
#include <iostream>
#include <new>
#include <cstdlib>

namespace {

class TestContext
{
  const TestContext* m_prevContext;
  const char*        m_file;
  int                m_line;
  const char*        m_str;
  static const TestContext* s_currContext;

public:
  static const TestContext* currContext() { return s_currContext; }

  TestContext(const char* file, int line, const char* str)
    : m_prevContext(s_currContext)
    , m_file(file), m_line(line), m_str(str)
    {
      s_currContext = this;
    }

  ~TestContext() { s_currContext = m_prevContext; }

  const TestContext* prevContext() const { return m_prevContext; }
  const char* file() const { return m_file; }
  unsigned line() const { return m_line; }
  const char* str() const { return m_str; }
};
const TestContext *TestContext::s_currContext = nullptr;

int errorCount = 0;

#define TEST_ASSERT(c) do {                                             \
    if (! (c)) {                                                        \
      std::cout << __FILE__ << ':' << __LINE__                          \
                << ": Assertion failed: " #c << std::endl;              \
      for (const TestContext* ctx = TestContext::currContext();         \
           ctx; ctx = ctx->prevContext())                               \
        std::cout << ctx->file() << ':' << ctx->line()                  \
                  << ":  Context: " << ctx->str() << std::endl;         \
      ++errorCount;                                                     \
    }                                                                   \
  } while (false)

} // close unnamed namespace

/// Inherit from this struct to turn trivial copyable (if instantiated with
/// `true`) off (if instnatiated with false).
template <bool>
struct TcBase
{
};

/// Specialization for `false` parameter -- not trivially copyable.
template <>
struct TcBase<false>
{
  /// Non-trivial destructor
  constexpr ~TcBase() { }
};

enum TrivCopy : bool { };
enum TrivDfltCtor : bool { };

/// Test class, parameterized on size, trivially copyable, and trivially
/// default constructible.
template <std::size_t SZ, TrivCopy TC, TrivDfltCtor TDC>
struct TestClass : TcBase<bool(TC)>
{
  unsigned m_data[SZ];
};

template <std::size_t SZ, TrivCopy TC>
struct TestClass<SZ, TC, TrivDfltCtor(false)> : TcBase<bool(TC)>
{
  unsigned m_data[SZ];

  constexpr TestClass() {
    for (std::size_t i = 0; i < SZ; ++i)
      m_data[i] = 0xcafebeef;
  }
};

template <std::size_t SZ, TrivCopy TC, TrivDfltCtor TDC>
void testUninit()
{
  using Obj = TestClass<SZ, TC, TDC>;

  unsigned buffer[SZ];
  for (std::size_t i = 0; i < SZ; ++i)
    buffer[i] = 0xdeadbeef;

  auto p = new (buffer) TestClass(xstd::make_raw<Obj>());
  std::cout << "testUninit<" << SZ << ", "
            << "TrivCopy(" << (TC ? "true " : "false") << "), "
            << "TrivDfltCtor(" << (TC ? "true " : "false")
            << ")> uninit element = 0x" << std::hex << p->m_data[0]
            << std::endl;

  // Assert that default constructor is never invoked
  TEST_ASSERT(p->m_data[0]      != 0xcafebeef);
  TEST_ASSERT(p->m_data[SZ - 1] != 0xcafebeef);

  // When not using the trivial-copy optimization, assert that the buffer is
  // left unchanged.
  if constexpr (! bool(TC) || SZ > 16) {
    TEST_ASSERT(p->m_data[0]      == 0xdeadbeef);
    TEST_ASSERT(p->m_data[SZ - 1] == 0xdeadbeef);
  }
}

struct Initializer
{
  template <std::size_t SZ, TrivCopy TC, TrivDfltCtor TDC>
  void operator()(TestClass<SZ, TC, TDC> *p) {
    // Must be called on uninitialized storage.
    TEST_ASSERT(p->m_data[0]      != 0xcafebeef);
    TEST_ASSERT(p->m_data[SZ - 1] != 0xcafebeef);

    for (std::size_t i = 0; i < SZ; ++i)
      p->m_data[i] = 0xdecaf101;
  }
};

template <std::size_t SZ, TrivCopy TC, TrivDfltCtor TDC>
void testInit()
{
  using Obj = TestClass<SZ, TC, TDC>;

  unsigned buffer[SZ];
  for (std::size_t i = 0; i < SZ; ++i)
    buffer[i] = 0xdeadbeef;

  auto p = new (buffer) TestClass(xstd::make_raw<Obj>(Initializer{}));
  std::cout << "testInit<" << SZ << ", "
            << "TrivCopy(" << (TC ? "true " : "false") << "), "
            << "TrivDfltCtor(" << (TC ? "true " : "false")
            << ")> uninit element = 0x" << std::hex << p->m_data[0]
            << std::endl;

  TEST_ASSERT(p->m_data[0]      == 0xdecaf101);
  TEST_ASSERT(p->m_data[SZ - 1] == 0xdecaf101);
}

int main()
{
  testUninit< 1, TrivCopy(false), TrivDfltCtor(false)>();
  testUninit< 1, TrivCopy(false), TrivDfltCtor(true )>();
  testUninit< 1, TrivCopy(true ), TrivDfltCtor(false)>();
  testUninit< 1, TrivCopy(true ), TrivDfltCtor(true )>();
  testUninit<17, TrivCopy(false), TrivDfltCtor(false)>();
  testUninit<17, TrivCopy(false), TrivDfltCtor(true )>();
  testUninit<17, TrivCopy(true ), TrivDfltCtor(false)>();
  testUninit<17, TrivCopy(true ), TrivDfltCtor(true )>();

  testInit< 1, TrivCopy(false), TrivDfltCtor(false)>();
  testInit< 1, TrivCopy(false), TrivDfltCtor(true )>();
  testInit< 1, TrivCopy(true ), TrivDfltCtor(false)>();
  testInit< 1, TrivCopy(true ), TrivDfltCtor(true )>();
  testInit<17, TrivCopy(false), TrivDfltCtor(false)>();
  testInit<17, TrivCopy(false), TrivDfltCtor(true )>();
  testInit<17, TrivCopy(true ), TrivDfltCtor(false)>();
  testInit<17, TrivCopy(true ), TrivDfltCtor(true )>();

  return errorCount;
}

// Local Variables:
// c-basic-offset: 2
// End:
