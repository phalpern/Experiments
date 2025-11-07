/* to_rvalue.cpp                                                      -*-C++-*-
 *
 * Copyright (C) 2024 Pablo Halpern <phalpern@halpernwightsoftware.com>
 * Distributed under the Boost Software License - Version 1.0
 */

////// OLD implementation of `relocate_from`

#ifndef INCLUDE_RELOCATE_CONSTRUCTION
#define INCLUDE_RELOCATE_CONSTRUCTION

#include <type_traits>
#include <utility>
#include <cstring>

namespace xstd {

using namespace std;

template <class T>
struct is_trivially_relocatable
  : conjunction<is_trivially_copy_constructible<T>,
                is_trivially_destructible<T>>
{
};

template <class T>
inline constexpr bool is_trivially_relocatable_v =
  is_trivially_relocatable<T>::value;

template <class T>
struct relocate_internals
{
  static_assert(! (is_trivially_copy_constructible_v<T> &&
                   is_trivially_destructible_v<T>),
                "capture_imp is not for trivially copy-constructible types");

  /// This not-trivially-copyable class has a constructor that returns `this`
  /// though an output parameter but has no other effect. An object of this
  /// class is used as a stand-in for a trivially relocatable but not
  /// trivially copyable type `T`, as a way to delay construction of `T`.
  class self_capture
  {
    char m_padding[sizeof(T)];
  public:
    self_capture(void* &p) { p = this; }
    ~self_capture() { }
  };

  static self_capture capture_return_address(void *&p) {
    return { p };
  }

  static T get_return_address(T* &p) {
    using cap = T (&)(T *&);
    cap capture = reinterpret_cast<cap>(capture_return_address);
    return capture(p);
  }

  /// The destructor for this `struct` performs a trivial relocation from
  /// `src` to `dest`.
  struct trivial_relocator
  {
    T *src;
    T *dest;

    trivial_relocator(T *s) : src(s) { }
    ~trivial_relocator() {
      std::memcpy((void*) dest, (void*) src, sizeof(T));
      std::memset((void*) src, 0, sizeof(T));  // For debugging only
    }
  };

  /// The destructor of this `struct` invokes the destructor for `obj`.
  struct destroyer
  {
    T *obj;
    destroyer(T *o) : obj(o) { }
    ~destroyer() { obj->~T(); }
  };
};

/// This overload of `relocate_from` simply uses the trivial copy constructor.
template <class T>
requires (is_trivially_copy_constructible_v<T> &&
          is_trivially_destructible_v<T>)
T relocate_from(T *p)
{
  return *p;
}

/// This overload  of `relocate_from` trivially relocates from `*p` to the
/// return-value object.
template <class T>
requires (is_trivially_relocatable_v<T> &&
          ! (is_trivially_copy_constructible_v<T> &&
             is_trivially_destructible_v<T>))
T relocate_from(T *p)
{
  typename relocate_internals<T>::trivial_relocator tr(p);
  return relocate_internals<T>::get_return_address(tr.dest);
}

/// This overload of `relocate_from` non-trivially reloates from `*p` to the
/// return-value object.
template <class T>
requires (is_nothrow_move_constructible_v<T> && !is_trivially_relocatable_v<T>)
T relocate_from(T *p)
{
  typename relocate_internals<T>::destroyer tr(p);
  return std::move(*p);
}

} // close namespace xstd

#endif // INCLUDE_RELOCATE_CONSTRUCTION

// Local Variables:
// c-basic-offset: 2
// End:
