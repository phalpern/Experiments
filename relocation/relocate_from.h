/* relocate_from.h                                                    -*-C++-*-
 *
 * Copyright (C) 2024 Pablo Halpern <phalpern@halpernwightsoftware.com>
 * Distributed under the Boost Software License - Version 1.0
 */

#ifndef INCLUDED_RELOCATE_FROM
#define INCLUDED_RELOCATE_FROM

#include <make_raw.h>

#include <utility>
#include <cstring>
#include <memory>

#include <meta>

namespace xstd {

using namespace std;

/// The argument to a "relocation constructor". By the time the relocation
/// function is invoked, memberwise relocation from `*ptr()` has already taken
/// place. Thus `this->ptr()` should be called only for address arithmetic.
template <class T>
class relocate_ref
{
  T* m_ptr;  // Pointer to destroyed object - for arithmetic only

  constexpr relocate_ref(T *p) : m_ptr(p) {}  // Private

public:
  constexpr const T* ptr() const { return m_ptr; }
};

/// Return type of an explicitly defaulted `relocate_from` smf.
class relocate_default { };

/// Return type of an explicitly deleted `relocate_from` smf.
class relocate_delete { };

/// Return `true` if `T` is a move-constructible class type having a
/// user-declared, non-deleted, publicly accessible move or copy constructor;
/// otherwise return `false`.  In particular, return `false` if `T`'s move and
/// copy constructors are implicitly declared (i.e., synthesized by the
/// compiler).  Note that a constructor that is explicitly defaulted on its
/// first declaration (`= default`) is user-declared, so a class having such a
/// constructor yields `true`.
template <class T>
consteval bool __has_user_declared_move()
{
  if constexpr (! is_move_constructible_v<T> ||
                ! (is_class_v<T> || is_union_v<T>)) {
    return false;
  }
  else {
    // Only members accessible from an unprivileged context (i.e., public
    // members) are returned by `members_of`.
    constexpr auto ctx = std::meta::access_context::unprivileged();
    for (std::meta::info mem : members_of(^^remove_cv_t<T>, ctx)) {
      if ((is_move_constructor(mem) || is_copy_constructor(mem)) &&
          is_user_declared(mem) && ! is_deleted(mem)) {
        return true;
      }
    }
    return false;
  }
}

enum class __relocation_def_t {
  deleted, trivial, memberwise, move_destroy, user_provided
};

template <class T>
consteval __relocation_def_t __calc_relocation_type();

/// Return the relocation type of `T`, which must be a non-cv-qualified class
/// or union type whose relocation is explicitly or implicitly defaulted.  The
/// result is `deleted` if any subobject has deleted relocation (or, for a
/// union, if any variant member is not trivially relocatable), `trivial` if
/// every subobject is trivially relocatable and `T` has a trivial destructor,
/// and `memberwise` otherwise.
template <class T>
consteval __relocation_def_t __calc_defaulted_relocation_subtype()
{
  bool is_trivial = true;

  // Recursively invoke `__calc_relocation_type` for subobjects of `T`. If it
  // returns `deleted` for any subobject, return `deleted` here; if it returns
  // something other than `trivial` for any subobject, set `is_trivial` to
  // `false`.
  template for (constexpr std::meta::info sub :
                  std::define_static_array(subobjects_of(
                    ^^T, std::meta::access_context::unchecked())))
  {
    constexpr __relocation_def_t sub_type =
      __calc_relocation_type<typename [:type_of(sub):]>();
    if constexpr (sub_type == __relocation_def_t::deleted) {
      return __relocation_def_t::deleted;
    }
    else if constexpr (sub_type != __relocation_def_t::trivial) {
      // The active member of a union is unknown, so a union having a
      // non-trivially-relocatable variant member cannot be relocated.
      if constexpr (is_union_v<T>) {
        return __relocation_def_t::deleted;
      }
      is_trivial = false;
    }
  }

  // Relocation is never trivial if `T` has a non-trivial destructor.
  if constexpr (! is_trivially_destructible_v<T>) {
    is_trivial = false;
  }

  return (is_trivial ?
          __relocation_def_t::trivial : __relocation_def_t::memberwise);
}

/// Return the relocation type of `T`.  References and scalars (including
/// bit-fields, when `T` is the type of a subobject) are trivially relocatable
/// (their bytes are simply copied); an array relocates element-by-element;
/// cv-qualifiers are ignored; and a class or union is classified by its
/// `relocate_from` member (if any), its user-declared copy or move constructor
/// (if any), and, otherwise, recursively by its subobjects. Note that, at the
/// top level, a reference type is not technically trivially relocatable, but
/// for this internal function, it is easiest to describe it as being TR.
template <class T>
consteval __relocation_def_t __calc_relocation_type() {
  if constexpr (is_reference_v<T> || is_scalar_v<T>) {
    return __relocation_def_t::trivial;
  }
  else if constexpr (is_array_v<T>) {
    constexpr __relocation_def_t elem =
      __calc_relocation_type<remove_extent_t<T>>();
    return (elem == __relocation_def_t::deleted ||
            elem == __relocation_def_t::trivial ?
            elem : __relocation_def_t::memberwise);
  }
  else if constexpr (! is_same_v<T, remove_cv_t<T>>) {
    return __calc_relocation_type<remove_cv_t<T>>();
  }
  else if constexpr (! is_class_v<T> && ! is_union_v<T>) {
    return __relocation_def_t::deleted;  // Not an object type
  }
  else if constexpr (requires(T v) {
    { v.relocate_from(declval<relocate_ref<T>>()) } -> same_as<void>;
  }) {
    return __relocation_def_t::user_provided;
  }
  else if constexpr (requires(T v) {
    { v.relocate_from(declval<relocate_ref<T>>())} -> same_as<relocate_delete>;
  }) {
    return __relocation_def_t::deleted;
  }
  else if constexpr (requires(T v) {
    { v.relocate_from(declval<relocate_ref<T>>())} -> same_as<relocate_default>;
  }) {
    return __calc_defaulted_relocation_subtype<T>();  // Explicitly defaulted
  }
  else if constexpr (requires(T v) {
    v.relocate_from(declval<relocate_ref<T>>());
  }) {
    static_assert(false, "Invalid return type from `relocate_from`");
  }
  else if constexpr (__has_user_declared_move<T>()) {
    if constexpr (std::is_trivially_move_constructible_v<T> &&
                  std::is_trivially_destructible_v<T>) {
      return __relocation_def_t::trivial;
    }
    else {
      return __relocation_def_t::move_destroy;
    }
  }
  else {
    return __calc_defaulted_relocation_subtype<T>();  // Implicitly defaulted
  }
}

/// Derive from `true_type` iff `T` is trivially relocatable.
template <class T>
struct is_trivially_relocatable
  : bool_constant<__calc_relocation_type<T>() == __relocation_def_t::trivial>
{
};

/// Special case: a reference type is never trivially relocatable (though it
/// behaves as if it were when used as a member of a class or union).
template <class T>
struct is_trivially_relocatable<T&> : false_type
{
};

/// Special case: a reference type is never trivially relocatable (though it
/// behaves as if it were when used as a member of a class or union).
template <class T>
struct is_trivially_relocatable<T&&> : false_type
{
};

template <class T>
inline constexpr bool is_trivially_relocatable_v =
    is_trivially_relocatable<T>::value;

template <class T>
void __interior_relocate(T* from_p, T* to_p);

/// Relocate the single subobject `from_obj` into the raw storage occupied by
/// `to_obj` by constructing a `U` from `relocate_from(&from_obj)`.
/// cv-qualifiers on `U` are stripped so that `relocate_from` deduces the
/// unqualified type (and can therefore move) and so that placement new
/// accepts the address.
template <class U>
void __relocate_subobject(U& from_obj, U& to_obj)
{
  using V = remove_cv_t<U>;
  V *from_p = const_cast<V*>(std::addressof(from_obj));
  V *to_p   = const_cast<V*>(std::addressof(to_obj));
  __interior_relocate(from_p, to_p);
}

/// Relocate `*from` into the raw storage at `to` one subobject at a time, in
/// constructor-initializer order, assuming all of the bytes were copied from
/// `*from` to `to` first. Each subobject is relocated using `relocate_from`,
/// except that subobjects of reference type or of trivially relocatable type
/// are skipped. By byte-wise copying the entire objects first, we avoid having
/// to special-case non-addressable members like bit fields and references, and
/// avoid multiple, discontiguous calls to `memcpy` for trivially relocatable
/// subobjects.
template <class T>
void __memberwise_relocate(T* from, T* to)
{
  constexpr auto ctx = std::meta::access_context::unchecked();

  template for (constexpr auto sub :
                  std::define_static_array(subobjects_of(^^T, ctx)))
  {
    using U = typename [:type_of(sub):];
    if constexpr (__calc_relocation_type<U>() != __relocation_def_t::trivial)
      __relocate_subobject(from->[:sub:], to->[:sub:]);
  }
}

/// RAII type to destroy an object of type `T` on destruction of this object.
template <class T, bool trivial = is_trivially_destructible_v<T>>
class __relocate_dtor_guard
{
  T* m_obj_p;

public:
  constexpr __relocate_dtor_guard(T* p) : m_obj_p(p) { }
  constexpr ~__relocate_dtor_guard() { if (m_obj_p) m_obj_p->~T(); }
  __relocate_dtor_guard(const __relocate_dtor_guard&) = delete;
  __relocate_dtor_guard& operator=(const __relocate_dtor_guard&) = delete;
};

/// RAII type to destroy an object of trivially destructible type.
template <class T>
class __relocate_dtor_guard<T, /* trivial = */ true>
{
public:
  constexpr __relocate_dtor_guard(T*) { }
  constexpr ~__relocate_dtor_guard() = default;
  __relocate_dtor_guard(const __relocate_dtor_guard&) = delete;
  __relocate_dtor_guard& operator=(const __relocate_dtor_guard&) = delete;
};

/// Relocate an object. Requires: bytewise copy of `to_p` to `from_p` has
/// already occured. This function can be used at the top level or any
/// subobject level.
template <class T>
void __interior_relocate(T* from_p, T* to_p)
{
  using rd_t = __relocation_def_t;

  constexpr rd_t reloc_type = __calc_relocation_type<T>();
  static_assert(reloc_type != rd_t::deleted,
                "Relocation is deleted for this type");

  if constexpr (reloc_type == rd_t::trivial) {
    // Trivial object was already relocatedf
  }
  else if constexpr (reloc_type == rd_t::memberwise) {
    __memberwise_relocate(from_p, to_p);
  }
  else if constexpr (reloc_type == rd_t::move_destroy) {
    __relocate_dtor_guard guard(from_p);
    ::new (static_cast<void*>(to_p)) T(std::move(*from_p));
  }
  else {
    // User-provided relocation. Perform a memberwise relocation before calling
    // the relocation smf.
    __memberwise_relocate(from_p, to_p);
    to_p->relocate_from(*from_p);
  }
}

template <class T>
T relocate_from(T& from)
{
  using rd_t = __relocation_def_t;

  constexpr rd_t reloc_type = __calc_relocation_type<T>();
  static_assert(reloc_type != rd_t::deleted,
                "Relocation is deleted for this type");

  // Special case optimization: If relocation can occur via trivial move
  // construction, then do that. This mechanism is the only one that works in a
  // `constexpr` context.
  if constexpr (reloc_type == rd_t::trivial &&
                is_trivially_move_constructible_v<T> &&
                is_trivially_destructible_v<T>) {
    return std::move(from);
  }
  else {
    return make_raw<T>([&from](T *to_p) {
      T* from_p = addressof(from);
      std::memcpy(static_cast<void*>(to_p), from_p, sizeof(T));
      __interior_relocate(from_p, to_p);
    });
  }
}

} // close namespace xstd


#endif // ! defined(INCLUDED_RELOCATE_FROM)

// Local Variables:
// c-basic-offset: 2
// End:
