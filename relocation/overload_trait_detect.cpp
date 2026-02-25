/* overload_trait_detect.cpp                                          -*-C++-*-
 *
 * Copyright (C) 2024 Pablo Halpern <phalpern@halpernwightsoftware.com>
 * Distributed under the Boost Software License - Version 1.0
 */

#include <type_traits>

template <class T> struct is_trivially_relocatable;

class trivially_relocatable
{
  private:
    trivially_relocatable() = delete;
    trivially_relocatable(const trivially_relocatable&) = delete;
    void operator=(const trivially_relocatable&) = delete;

    struct magic_token { };
    constexpr trivially_relocatable(magic_token, int) { }

    template <class T> friend struct is_trivially_relocatable;
};

template <class T>
struct is_trivially_relocatable :
    std::bool_constant<requires { T({trivially_relocatable::magic_token(), 0}, 0); }>
{
};

template <class T>
constexpr inline bool is_trivially_relocatable_v = is_trivially_relocatable<T>::value;

struct X
{
    X(trivially_relocatable, ...);

//    void operator=(replaceable);
};

struct Y
{
    template <class... Args>
    Y(Args&&...a) { }
};

struct Z
{
    Z(...) { }
};

static_assert(  is_trivially_relocatable_v<X>);
static_assert(! is_trivially_relocatable_v<Y>);
static_assert(! is_trivially_relocatable_v<Z>);

// Y y({trivially_relocatable::magic_token(), 0}, 0);

// Local Variables:
// c-basic-offset: 2
// End:
