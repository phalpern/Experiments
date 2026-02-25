/* borrow.cpp                                                         -*-C++-*-
 *
 * Copyright (C) 2024 Pablo Halpern <phalpern@halpernwightsoftware.com>
 * Distributed under the Boost Software License - Version 1.0
 */

#include <new>
#include <utility>
#include <cassert>

// For now, we'll use cassert, but for compile-time checking, we'll need
// something better.
#define RustAssert assert

template <class T>
class RustObj

template <class T>
class RustRef;

template <class T>
class RustObj<const T>
{
  friend class RustRef<const T>;

  using CT = const T;

protected:
  bool m_live:1      = true;
  bool m_mutating:1  = false;
  bool m_usecount:14 = 0;
  union contents
  {
    contents() { }
    T  m_value;
  };

  constexpr void *voidify(T& r)
  {
    return const_cast<std::remove_const_t<T>*>(std::addressof(r));
  }

public:
  template <class... Args>
  constexpr RustObj(Args&&... args)
  {
    ::new (voidify(m_value)) T(std::forward<Args>(args)...);
  }

  constexpr RustObj(const RustObj& rhs)
  {
    RustAssert(! rhs.m_mutating);
    ::new(voidify(m_value)) T(rhs.m_value);
  }

  constexpr RustObj(RustObj&& rhs)
  {
    RustAssert(! rhs.m_mutating);
    ::new(voidify(m_value)) T(std::move(rhs.m_value));
  }

  constexpr ~RustObj()
  {
    RustAssert(0 == m_usecount);
    if (m_live) m_value.~T();
  }

  constexpr void drop()
  {
    RustAssert(m_live && 0 == m_usecount);
    m_live = false;
    m_value.~T();
  }

  constexpr operator RustRef<const T>()  { return { this }; }
  constexpr RustRef<const T> operator+() { return { this }; }

  template <class F, class... Args>
  constexpr auto apply(F&& f, Args&&... args)
  {
    return (+*this).apply(std::forward<F>(f), std::forward<Args>(args)...);
  }
};

// Mutable object
template <class T>
class RustObj : RustObj<const T>
{
  friend class RustRef<T>;

public:
  template <class... Args>
  constexpr RustObj(Args&&... args)
    : RustObj<const T>(std::forward<Args>(args)...)
  {
    ::new (voidify(m_value)) T(std::forward<Args>(args)...);
  }

  constexpr RustObj(const RustObj& rhs)
  {
    RustAssert(! rhs.m_mutating);
    ::new(voidify(m_value)) T(rhs.m_value);
  }

  constexpr RustObj(RustObj&& rhs)
  {
    RustAssert(! rhs.m_mutating);
    ::new(voidify(m_value)) T(std::move(rhs.m_value));
  }

  constexpr ~RustObj()
  {
    RustAssert(0 == m_usecount);
    if (m_live) m_value.~T();
  }

  constexpr void drop()
  {
    RustAssert(m_live && 0 == m_usecount);
    m_live = false;
    m_value.~T();
  }

  constexpr operator RustRef<const T>()  { return { this }; }
  constexpr RustRef<const T> operator+() { return { this }; }

  constexpr RustRef<T> operator&() requires(! std::is_const_v<T>)
    { return { this }; }

  template <class F, class... Args>
  constexpr auto apply(F&& f, Args&&... args)
  {
    return (+*this).apply(std::forward<F>(f), std::forward<Args>(args)...);
  }

  template <class F, class... Args>
  constexpr auto mapply(F&& f, Args&&... args)
    require(! std::is_const_v<T>)
  {
    return (&*this).apply(std::forward<F>(f), std::forward<Args>(args)...);
  }
};

// Mutating reference
template <class T>
class RustRef
{
  friend class RustObj<T>;

  RustObj<T>* m_obj_p;

  // Private creators
  constexpr RustRef(RustObj<T> *obj_p) : m_obj_p(obj_p)
  {
    RustAssert(obj_p->m_live && 0 == obj_p->m_usecount);
    ++obj_p->m_usecount;
    obj_p->m_mutating = true;
  }

public:
  constexpr RustRef(RustRef&& rhs) : m_obj_p(rhs.m_obj_p)
  {
    RustAssert(m_obj_p && m_obj_p->m_live);
    rhs.m_obj_p = nullptr;
  }

  constexpr ~RustRef()
  {
    if (m_obj_p) {
      m_obj_p->m_mutating = false;
      --m_obj_p->m_usecount;
    }
  }

  constexpr void drop()
  {
    RustAssert(m_obj_p);
    m_obj_p->m_mutating = false;
    --m_obj_p->m_usecount;
    m_obj_p = nullptr;
  }

  template <class F, class... Args>
  constexpr auto apply(F&& f, Args&&... args)
  {
    RustAssert(m_obj_p && m_obj_p->m_live);
    return std::forward<F>(f)(const_cast<const T&>(m_obj_p->m_value),
                              std::forward<Args>(args)...);
  }

  template <class F, class... Args>
  constexpr auto mapply(F&& f, Args&&... args)
  {
    RustAssert(m_obj_p && m_obj_p->m_live);
    return std::forward<F>(f)(m_obj_p->m_value, std::forward<Args>(args)...);
  }
};

template <class T>
class RustRef<const T>
{
  using CT = const T;

  friend class RustObj<CT>;

  RustObj<CT>* m_obj_p;

  // Private creators
  constexpr RustRef(RustObj<T> *obj_p) : m_obj_p(obj_p)
  {
    RustAssert(obj_p->m_live &&
               0 == obj_p->m_usecount && ! m_obj_p->m_mutating);
    ++obj_p->m_usecount;
    obj_p->m_mutating = true;
  }

public:
  constexpr RustRef(const RustRef& rhs) : m_obj_p(rhs.m_obj_p)
  {
    RustAssert(m_obj_p && m_obj_p->m_live && ! m_obj_p->m_mutating);
    ++m_obj_p->m_usecount;
  }

  constexpr RustRef(RustRef&& rhs) : m_obj_p(rhs.m_obj_p)
  {
    RustAssert(m_obj_p && m_obj_p->m_live && ! m_obj_p->m_mutating);
    rhs.m_obj_p = nullptr;
  }

  constexpr ~RustRef()
  {
    if (m_obj_p) {
      --m_obj_p->m_usecount;
    }
  }

  constexpr void drop()
  {
    RustAssert(m_obj_p);
    --m_obj_p->m_usecount;
    m_obj_p = nullptr;
  }

  template <class F, class... Args>
  constexpr auto apply(F&& f, Args&&... args)
  {
    RustAssert(m_obj_p && m_obj_p->m_live);
    return std::forward<F>(f)(m_obj_p->m_value, std::forward<Args>(args)...);
  }
};


// Local Variables:
// c-basic-offset: 2
// End:
