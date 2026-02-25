// nested_trait.t.cpp                                                 -*-C++-*-

#include <cassert>
#include <iostream>

#if __cplusplus > 201101
# include <type_traits>
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

template <template <class> class Trait>
struct TraitWrapper
{
};

#if USE_OPERATOR_METHOD
template <class T, template <class> class Trait, TraitWrapper<Trait> (T::*)()>
class OpWrap
{
};

template <template <class> class Trait, class T>
struct DetectNestedTrait_Imp
{
    template <class U>
    static char check(OpWrap<U, Trait, &U::operator TraitWrapper<Trait> > *x);
    template <class U>
    static int  check(MatchAnyType);

    enum { value = sizeof(check<T>(0)) == 1 };
};

template <template <class> class Trait, class T>
struct DetectNestedTrait :
    bsl::integral_constant<bool, DetectNestedTrait_Imp<Trait, T>::value>
{
};

# define BSLMF_DECLARE_NESTED_TRAIT(TRAIT) \
    operator bslmf::TraitWrapper<TRAIT>()

#else // if ! USE_OPERATOR_METHOD
template <class T, template <class> class Trait,
          bsl::true_type (T::*)(TraitWrapper<Trait>)>
class FnWrap
{
};

template <template <class> class Trait, class T>
struct DetectNestedTrait_Imp
{
    template <class U>
    static char check(FnWrap<U, Trait, &U::declareNestedTrait> *);
    template <class U>
    static int  check(MatchAnyType);

    enum { value = sizeof(check<T>(0)) == 1 };
};

template <template <class> class Trait, class T>
struct DetectNestedTrait :
    bsl::integral_constant<bool, DetectNestedTrait_Imp<Trait, T>::value>
{
};

# define BSLMF_DECLARE_NESTED_TRAIT(TRAIT) \
    bsl::true_type declareNestedTrait(bslmf::TraitWrapper<TRAIT>)

#endif // USE_OPERATOR_METHOD

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

int main()
{
    assert((  bslmf::DetectNestedTrait<MyTrait, W>::value));
    assert((  bslmf::DetectNestedTrait<MyTrait, X>::value));
    assert((! bslmf::DetectNestedTrait<MyTrait, int>::value));
    assert((! bslmf::DetectNestedTrait<MyTrait, Y>::value));
    assert((! bslmf::DetectNestedTrait<MyTrait, Z>::value));

    assert((  MyTrait<W>::value));
    assert((  MyTrait<X>::value));
    assert((! MyTrait<int>::value));
    assert((! MyTrait<Y>::value));
    assert((! MyTrait<Z>::value));

    assert((  OtherTrait<W>::value));
    assert((! OtherTrait<X>::value));
    assert((! OtherTrait<int>::value));
    assert((  OtherTrait<Y>::value));
    assert((! OtherTrait<Z>::value));
}
