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

/// This dummy class has two constructors; one that does
/// nothing, and one that invokes a user-specified function to initialize the
/// object.  Since this dummy class has no data itself, no compiler-generated
/// initialization takes place. By aliasing this type over the address of an
/// object of a different type, we can bypass the other type's constructor and
/// invoke whatever operation we want on the raw storage. This approach might
/// or might not work if the other type is trivially copyable, since trivially
/// copyable objects are often returned in registers, so `this` would not
/// actually alias the intended object. In fact, this type is deliberately not
/// trivially copyable so that it can correctly alias non-trivially copyable
/// types.
struct internal_dummy_type
{
  /// No-op, but non-trivial default constructor.
  constexpr internal_dummy_type() { }

  /// This constructor calls the specified `emplacer` on the address of this
  /// object (which has no other initialization). The `emplacer` typically puts
  /// a valid object of some other type at the this address.
  template <class F>
  constexpr explicit internal_dummy_type(F&& emplacer)
    requires (std::is_invocable_v<F&&, void*>)
    { std::forward<F>(emplacer)(this); }

  /// No-op, but non-trivial destructor.
  constexpr ~internal_dummy_type() { }

  /// Returns a default-constructed object. This factory-function version of
  /// the default constructor exists so that its address can be taken and cast
  /// to a different factory type.
  constexpr static internal_dummy_type default_factory() { return { }; }

  /// Returns an object initialized using `emplacer`. This factory-function
  /// version of the emplace constructor exists so that its address can be
  /// taken and cast to a different factory type.
  template <class F>
  constexpr static internal_dummy_type emplace_factory(F&& emplacer)
    requires (std::is_invocable_v<F&&, void*>)
    { return { std::forward<F>(emplacer) }; }
};


/// Returns an object of type `T` without initializing it. This overload is
/// selected when `T` is trivially default constructible. Note that the default
/// constructor is invoked, which might zero-initialize the object in some
/// cases.
template <class T>
requires (std::is_trivially_default_constructible_v<T>)
constexpr inline T make_raw()
{
  return T{};
}

/// Returns an object of type `T` without initializing it. This overload is
/// selected when `T` is trivially copyable. Note that if the move constructor
/// is not optimized out of the return statement, this function will result in
/// a byte copy of `sizeof(T)` bytes.
template <class T>
requires (std::is_trivially_copyable_v<T> &&
          !std::is_trivially_default_constructible_v<T>)
inline T make_raw()
{
  // `Tu` is a union containing a `T` but doesn't initialize the `T`.
  union Tu { char m_c; T m_t; Tu() { }; ~Tu() { } };

  // Return the uninitialized `T` member of a `Tu` object. Note that, if `T` is
  // large, this might be a slightly expensive operation involving a `memcpy`,
  // though hopefully the compiler would optimize that out.
  return Tu().m_t;
}

/// Returns an object of type `T` without initializing it. This overload is
/// selected when `T` is neither trivially default constructible nor trivially
/// copyable.
template <class T>
inline T make_raw()
{
  // The `T_factory` type is a reference to function taking no arguments and
  // returning `T`.
  using T_factory = T (&)();

  // Cast the dummy factory function to `T_factory` and invoke it. This
  // sequence fools the compiler into thinking that it is calling a function
  // returning `T`, when it is in fact returning an (uninitialized) dummy
  // object.
  return reinterpret_cast<T_factory>(internal_dummy_type::default_factory)();
}

/// Returns an object of type `T` initializing it using `emplacer`. This
/// overload is selected when `T` is trivially copyable (and might, therefore,
/// be returned in registers).
template <class T, class F>
requires (std::is_trivially_copyable_v<T>)
inline T make_raw(F&& emplacer)
{
  union Tu { char m_c; T m_t; Tu() { }; ~Tu() { } };
  Tu ret_u;
  std::forward<F>(emplacer)(&ret_u.m_t);
  return ret_u.m_t;
}

/// Returns an object of type `T` initializing it using `emplacer`. This
/// overload is selected when `T` is not trivially copyable.
template <class T, class F>
inline T make_raw(F&& emplacer)
{
  // The `T_factory` type is a reference to function taking an `F&&` argument
  // and returning `T`.
  using T_factory = T (&)(F&&);

  // Cast the dummy factory function to `T_factory`. This cast fools the
  // compiler into thinking that `factory` is a function returning `T`, when it
  // is in fact returning a dummy object.
  internal_dummy_type (&factory)(F&&) = internal_dummy_type::emplace_factory<F>;

  // Invoke the factory after casting and return the result
  return reinterpret_cast<T_factory>(factory)(std::forward<F>(emplacer));
}

} // close namespace xstd

#endif // INCLUDED_MAKE_RAW

// Local Variables:
// c-basic-offset: 2
// End:
