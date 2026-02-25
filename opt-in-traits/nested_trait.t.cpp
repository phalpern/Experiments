// nested_trait.t.cpp                                                 -*-C++-*-

#include <iostream>

#if __cplusplus > 201101
# define test_assert(c) static_assert(c, "")
#else
# include <cassert>
# define test_assert(c) assert(c)
#endif

namespace bsl {

template <class T, T Val>
struct integral_constant
{
    typedef integral_constant type;
    static const T value = Val;

    operator T() const { return value; }
};

typedef integral_constant<bool, true>  true_type;
typedef integral_constant<bool, false> false_type;

} // end namespace bsl

namespace bslmf {


struct MatchAnyType
{
    template <class T>
    MatchAnyType(const T&);  // IMPLICIT
};

template <template <class> class Trait, class T = void>
struct TraitWrapper;

template <template <class> class Trait>
struct TraitWrapper<Trait, void>
{
};

template <template <class> class Trait>
struct TraitWrapperBase : TraitWrapper<Trait, void>
{
};

template <template <class> class Trait, class T>
struct TraitWrapper : TraitWrapperBase<Trait>
{
};

template <class T, template <class> class Trait, TraitWrapper<Trait> (T::*)()>
class OpMatch
{
};

template <std::size_t SZ, template <class> class Trait, class T>
struct DetectNestedTrait_Imp2 : bsl::false_type
{
};

template <template <class> class Trait, class T>
struct DetectNestedTrait_Imp2<sizeof(char), Trait, T>
{
    template <class U>
    static char check(OpMatch<U, Trait, &U::operator TraitWrapper<Trait> > *x);
    template <class U>
    static int  check(MatchAnyType);

    enum { value = sizeof(check<T>(0)) == 1 };
};

template <template <class> class Trait, class T>
struct DetectNestedTrait_Imp2<sizeof(short), Trait, T> : bsl::true_type
{
};

template <template <class> class Trait, class T>
struct DetectNestedTrait_Imp
{
    static T& tref();

    // template <class U>
    // static char check(OpMatch<U, Trait, &U::operator TraitWrapper<Trait> > *x);
    template <class U>
    static char check(TraitWrapper<Trait> x, int);
    template <class U>
    static short check(TraitWrapper<Trait, U> x, int);
    template <class U>
    static int check(TraitWrapperBase<Trait> x, int);
    template <class U>
    static long long check(MatchAnyType, ...);

    static_assert(sizeof(check<T>(tref(), 0)) != sizeof(int),
                  "Nested-trait `TYPE` argument must match class name");

    enum { value = DetectNestedTrait_Imp2<sizeof(check<T>(tref(), 0)),
                                          Trait, T>::value };
};

template <template <class> class Trait, class T>
struct DetectNestedTrait :
    bsl::integral_constant<bool, DetectNestedTrait_Imp<Trait, T>::value>
{
};

# define BSLMF_DECLARE_NESTED_TRAIT(TRAIT) \
    operator bslmf::TraitWrapper<TRAIT>()

# define BSLMF_DECLARE_NESTED_TRAIT2(TRAIT, T)    \
    operator bslmf::TraitWrapper<TRAIT, T>()

}  // close package namespace

template <class T>
struct MyTrait : bslmf::DetectNestedTrait<MyTrait, T> { };

template <class T>
struct OtherTrait : bslmf::DetectNestedTrait<OtherTrait, T> { };

struct W
{
    BSLMF_DECLARE_NESTED_TRAIT(MyTrait);
    BSLMF_DECLARE_NESTED_TRAIT(OtherTrait);
};

struct X
{
    BSLMF_DECLARE_NESTED_TRAIT(MyTrait);
};

struct Y
{
    BSLMF_DECLARE_NESTED_TRAIT(OtherTrait);
};

struct Z : X
{
};

template <class T>
struct CRTP_Base
{
    BSLMF_DECLARE_NESTED_TRAIT2(MyTrait, T);
};

struct D : CRTP_Base<D>
{
};

struct E
{
    // Simulate a cut-and-paste error
    BSLMF_DECLARE_NESTED_TRAIT2(MyTrait, D);
};

int main()
{
    test_assert((  bslmf::DetectNestedTrait<MyTrait, W>::value));
    test_assert((  bslmf::DetectNestedTrait<MyTrait, X>::value));
    test_assert((! bslmf::DetectNestedTrait<MyTrait, int>::value));
    test_assert((! bslmf::DetectNestedTrait<MyTrait, Y>::value));
    test_assert((! bslmf::DetectNestedTrait<MyTrait, Z>::value));
    test_assert((  bslmf::DetectNestedTrait<MyTrait, D>::value));
    test_assert((! bslmf::DetectNestedTrait<MyTrait, E>::value));

    test_assert((  MyTrait<W>::value));
    test_assert((  MyTrait<X>::value));
    test_assert((! MyTrait<int>::value));
    test_assert((! MyTrait<Y>::value));
    test_assert((! MyTrait<Z>::value));
    test_assert((  MyTrait<D>::value));
    test_assert((! MyTrait<E>::value));

    test_assert((  OtherTrait<W>::value));
    test_assert((! OtherTrait<X>::value));
    test_assert((! OtherTrait<int>::value));
    test_assert((  OtherTrait<Y>::value));
    test_assert((! OtherTrait<Z>::value));
    test_assert((! OtherTrait<D>::value));
    test_assert((! OtherTrait<E>::value));
}
