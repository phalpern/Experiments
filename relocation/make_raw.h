/* make_raw.h
 *
 * Copyright (C) 2024 Pablo Halpern <phalpern@halpernwightsoftware.com>
 * Distributed under the Boost Software License - Version 1.0
 */

#ifndef INCLUDED_MAKE_RAW
#define INCLUDED_MAKE_RAW

/// Construct an object without necessarily invoking its constructor, leaving
/// it either uninitialized or initialized via an external operation such as
/// `memcpy` or a simulated constructor. If the object's type is not trivially
/// destructible, invoking the destructor on the incompletely initialized
/// object is UB. The implementation of this component depends on multiple
/// types of UB and makes certain implementation assumptions.

#include <type_traits>
#include <utility>

namespace xstd {

using namespace std;

namespace internal {

/// The largest size for objects were the specialized `make_raw` operations
/// yield optimization benefits. In generic code, trivial initialization
/// results in an unnecessary zero-initialization and returning a trivially
/// copyable object results in unnecessary byte copies into a register (or
/// worse, into another memory location). Below this size threshold, however,
/// these costs are outweighed by the benefits of avoiding a function
/// invocation through an indirect pointer.
/// TBD: The correct value for this constant should be determined by timing
/// measurements specific to each platform. The value here is the typical size
/// of an object that will fit into the largest SIMD register, though some
/// architectures have even larger vector sizes.
constexpr std::size_t max_opt_size = 64;

/// An uninitialized buffer such that:
///  * `sizeof(obj_buffer<T>) == sizeof(T)`
///  * `alignof(obj_buffer<T>) == alignof(T)`
///  * `is_trivially_copyable_v<obj_buffer<T>> == `is_trivially_copyable_v<T>`
/// This primary template is instantiated for non-trivially copyable `T`.
template <class T, bool = is_trivially_copyable_v<T>>
union obj_buffer
{
  char m_c[sizeof(T)];
  T    m_t;

  /// No-op, but non-trivial default constructor.
  constexpr obj_buffer() { }

  /// No-op, but non-trivial destructor, which prevents this type from being
  /// trivially copyable.
  constexpr ~obj_buffer() { }
};

/// An uninitialized buffer such that:
///  * `sizeof(obj_buffer<T>) == sizeof(T)`
///  * `alignof(obj_buffer<T>) == alignof(T)`
///  * `is_trivially_copyable_v<obj_buffer<T>> == `is_trivially_copyable_v<T>`
/// This specialization is instantiated for trivially copyable `T`.
template <class T>
union obj_buffer<T, true>
{
  char m_c[sizeof(T)];
  T    m_t;

  /// No-op, but non-trivial default constructor.
  constexpr obj_buffer() { }

  // Implicit destructor is trivial
};

/// An object intended to alias an uninitialized `T` and intercept the
/// construction process.
///
/// Important attributes are that:
///  * `sizeof(shadow_init<T>) == sizeof(T)`
///  * `alignof(shadow_init<T>) == alignof(T)`
///  * `is_trivially_copyable_v<shadow_init<T>> == `is_trivially_copyable_v<T>`
///
/// This shadow class has two constructors: a default constructor that does
/// nothing, and one that invokes a user-specified function to initialize the
/// object. By type punning `T` over an object of this class after
/// construction, we can bypass `T`'s constructor and invoke whatever operation
/// we want on the raw storage (or no operation at all).
template <class T>
class shadow_init
{
  obj_buffer<T> m_tu;

  /// No-op (but not trivial) default constructor
  constexpr shadow_init() { }

  /// This constructor calls the specified `emplacer` on the address of this
  /// object (which has no other initialization). The `emplacer` typically puts
  /// a valid object of some other type at the this address.
  template <class F>
  constexpr explicit shadow_init(F&& emplacer)
    requires (std::is_invocable_v<F&&, T*>)
    { std::forward<F>(emplacer)(&m_tu.m_t); }

public:
  /// Returns a default-constructed object. This factory-function version of
  /// the default constructor exists so that its address can be taken and cast
  /// to a different factory type.
  constexpr static shadow_init default_factory() { return { }; }

  /// Returns an object initialized using `emplacer`. This factory-function
  /// version of the emplace constructor exists so that its address can be
  /// taken and cast to a different factory type.
  template <class F>
  constexpr static shadow_init emplace_factory(F&& emplacer)
    requires (std::is_invocable_v<F&&, T*>)
    { return shadow_init{std::forward<F>(emplacer)}; }
};

}  // close namespace `xstd::internal`

/// Returns an object of type `T` without initializing it. This overload is
/// selected when `T` is neither trivially default constructible nor trivially
/// copyable.
template <class T>
inline T make_raw()
{
  using shadow = internal::shadow_init<T>;

  // UB: Cast a factory returning a shadow to a factory returning a `T`. When
  // the resulting `factory` is invoked, the compiler is fooled into thinking
  // that it is calling a function returning `T`, when it is in fact calling a
  // function returning a shadow object.
  // ASSUMPTION: types `T ()` and `shadow ()` have the same ABI.
  auto& factory = reinterpret_cast<T (&)()>(shadow::default_factory);

  // Invoke the spoofed factory function and return the resulting `T`
  // object. Materialization rules will prevent temporaries from getting
  // created.
  return factory();
}

/// Returns an object of type `T` without initializing it. This optimized
/// version is selected when `T` is is trivially default constructible.  The
/// default constructor is invoked, which might zero-initialize the object in
/// some cases; to avoid losing the performance benefit due to this zero
/// initialization, this optimization is restricted to objects within the
/// optimization size threshold.
template <class T>
requires (std::is_trivially_default_constructible_v<T> &&
          sizeof(T) <= internal::max_opt_size)
constexpr inline T make_raw()
{
  return T{};
}

/// Returns an object of type `T` without initializing it. This optimized
/// version is selected when `T` is trivially copyable (and might, therefore,
/// be returned in registers). Unless the compiler is able to see through and
/// optimize out the return statement, this function will yield a a byte copy
/// of `sizeof(T)` bytes, typically from the stack into registers for small
/// objects or from the stack into the return value for larger objects; to
/// avoid losing the performance benefit due to this byte copy, this
/// optimization is restricted to objects within the optimization size
/// threshold.
template <class T>
requires (std::is_trivially_copyable_v<T> &&
          !std::is_trivially_default_constructible_v<T> &&
          sizeof(T) <= internal::max_opt_size)
inline T make_raw()
{
  // Return the uninitialized `T` member of a `obj_buffer` object. Note that,
  // if `T` is large, this might be a slightly expensive operation involving a
  // `memcpy`, though hopefully the compiler would optimize that out.
  return internal::obj_buffer<T>{}.m_t;  // UB: `obj_buffer` has no active elem
}

/// Returns an object of type `T` initializing it using `emplacer`. This
/// overload is selected when `T` is not trivially copyable.
template <class T, class F>
inline T make_raw(F&& emplacer)
{
  using shadow = internal::shadow_init<T>;

  // UB: Cast a factory returning a shadow to a factory returning a `T`. When
  // the resulting `factory` is invoked, the compiler is fooled into thinking
  // that it is calling a function returning `T`, when it is in fact calling a
  // function returning a shadow object.
  // ASSUMPTION: types `T (F&&)` and `shadow (F&&)` have the same ABI.
  auto& shadow_factory = shadow::template emplace_factory<F>;
  auto& factory = reinterpret_cast<T (&)(F&&)>(shadow_factory);

  // Invoke the spoofed factory function and return the resulting `T`
  // object. Materialization rules will prevent temporaries from getting
  // created.
  return factory(std::forward<F>(emplacer));
}

/// Returns an object of type `T` initializing it using `emplacer`. This
/// optimized version is selected when `T` is trivially copyable (and might,
/// therefore, be returned in registers). Unless the compiler is able to see
/// through and optimize out the return statement, this function will yield a a
/// byte copy of `sizeof(T)` bytes, typically from the stack into registers for
/// small objects or from the stack into the return value for larger objects;
/// to avoid losing the performance benefit due to this byte copy, this
/// optimization is restricted to objects within the optimization size
/// threshold.
template <class T, class F>
requires (std::is_trivially_copyable_v<T> &&
          sizeof(T) <= internal::max_opt_size)
inline T make_raw(F&& emplacer)
{
  internal::obj_buffer<T> ret_u;
  std::forward<F>(emplacer)(&ret_u.m_t);  // UB: `ret_u` has no active element
  return ret_u.m_t;  // Trivial copy of `m_t`
}


} // close namespace xstd

#endif // INCLUDED_MAKE_RAW

// Local Variables:
// c-basic-offset: 2
// End:
