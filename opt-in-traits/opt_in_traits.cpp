#include <type_traits>

namespace xstd {

using namespace std;

enum class trait_heritability {
  default_heritability, not_heritable, heritable
};

template <template <class> class Trait,
          trait_heritability heritability =
              trait_heritability::default_heritability,
          bool Condition = true>
struct opt_in { };

template <template <class> class Trait,
          trait_heritability Heritability =
              trait_heritability::default_heritability,
          bool Condition = true>
using opt_out = opt_in<Trait, Heritability, !Condition>;

template <class Sp, trait_heritability Hy, bool Cond>
struct __opt_in_tuple
{
  // Bundle of opt_in properties for a specific type.
  using scope                                       = Sp;
  constexpr static trait_heritability heritability  = Hy;
  constexpr static bool               condition     = Cond;
};

template <class Tp, template <class> class Trait,
          trait_heritability Hy, bool Cond,
          trait_heritability dflt_Hy, bool dflt_Cond>
struct __get_opt_in_tuple
{
  // Primary template chosen when neither 'opt_in' nor 'opt_out' operator is
  // defined for class 'Tp'.
  using type = __opt_in_tuple<void, dflt_Hy, dflt_Cond>;
};

template <class Tp, template <class> class Trait,
          trait_heritability Hy, bool Cond,
          trait_heritability dflt_Hy, bool dflt_Cond>
requires requires(Tp& t) { t.operator opt_in<Trait, Hy, Cond>(); }
struct __get_opt_in_tuple
{
  // Specialization chosen when either 'opt_in' nor 'opt_out' operator is
  // defined for class 'Tp' or one if it's base classes.
  template <class B> static B getScope(opt_in<Trait, Hy, Cond> (B::*)());

  using scope = decltype(getScope(&Tp::operator opt_in<Trait, heritability,
                                                        Cond>));
  constexpr static trait_heritability heritability =
              (Hy == trait_heritability::default_heritability ? dflt_hy : Hy);

  using type = __opt_in_tuple<scope, heritibility, Cond>;
};

template <class B1, class B2,
          class B1Scope = typename B1::scope,
          class B2Scope = typename B2::scope>
struct __deeper_tuple
{
  // Return opt-in tuple having the more-derived scope

  // If B1 and B2 are different, then they must have differen scopes
  static_assert(is_same_v<B1, B2> || ! is_same_v<B1Scope, B2Scope>,
                "Cannot opt in/out of same trait twice in same scope");
  using type = conditional_t<is_base_of_v<B1Scope, B2Scope>, B2, B1>;
};

template <class B1, class B2>
struct __deeper_tuple<B1, B2, void, void>
{
  // Both B1 and B2 are defaulted; Choose an arbitrary one.
  using type = B1;
};

template <class B1, class B2, class B1Scope>
struct __deeper_tuple<B1, B2, B1Scope, void>
{
  // B2 is defaulted, choose B1.
  using type = B1;
};

template <class B1, class B2, class B2Scope>
struct __deeper_tuple<B1, B2, void, B2Scope>
{
  // B1 is defaulted, choose B2.
  using type = B2;
};

template <class B1, class B2>
using __deeper_tuple_t = typename __deeper_tuple<B1, B2>::type;

template <class Tp, template <class> class Trait, bool AllowInheritance,
          trait_heritability dflt_Hy, bool dflt_Cond>
struct __detect_opt_in_imp
{
  using th = trait_heritability;

  using dt_tuple = typename _get_opt_in_tuple<Tp, trait,
                                              th::default_heritability,
                                              true, dflt Hy, dflt_Cond>;
  using nt_tuple = typename _get_opt_in_tuple<Tp, trait, th::not_heritable
                                              true, dflt Hy, dflt_Cond>;
  using it_tuple = typename _get_opt_in_tuple<Tp, trait, th::not_heritable
                                              true, dflt Hy, dflt_Cond>;
  using df_tuple = typename _get_opt_in_tuple<Tp, trait,
                                              th::default_heritability,
                                              false, dflt Hy, dflt_Cond>;
  using nf_tuple = typename _get_opt_in_tuple<Tp, trait, th::not_heritable
                                              false, dflt Hy, dflt_Cond>;
  using if_tuple = typename _get_opt_in_tuple<Tp, trait, th::not_heritable
                                              false, dflt Hy, dflt_Cond>;

  // find the tuple representing the operator instance at the  most-derived
  // level.
  using deepest = __deeper_tuple_t<
    __deeper_tuple_t<__deeper_tuple_t<dt_tuple, nt_tuple>,
                     __deeper_tuple_t<it_tuple, df_tuple>>,
    __deeper_tuple_t<nf_tuple, if_tuple>>;

  constexpr static trait_heritability heritability = deepest::Heritability;

  ==>
  static_assert(heritability <= opt_in_defaults<Trait>::max_heritability,
                "Cannot declare this trait as heritable");

  constexpr static bool value = deepest::condition;
};

template <class Tp, template <class> class Trait>
using detect_opt_in = bool_constant<__detect_opt_in_imp<Tp, Trait>::value>;

}  // close namespace xstd


namespace proj {

template <class T> struct mytrait : xstd::detect_opt_in<T, mytrait> { };

}  // close namespace proj

namespace xstd {
template <>
struct opt_in_defaults<proj::mytrait>
{
  constexpr static trait_heritability default_heritability = trait_heritability::not_heritable;
  constexpr static trait_heritability max_heritability     = trait_heritability::heritable;
};

}

namespace testproj {

struct W1 { };

struct X1 {
  operator xstd::opt_in<proj::mytrait>();
};

template <class T> struct printtype;

struct Y1 : X1 { };

struct Z1 : Y1 { };

static_assert(! proj::mytrait<W1>::value);
static_assert(  proj::mytrait<X1>::value);
static_assert(! proj::mytrait<Y1>::value);
static_assert(! proj::mytrait<Z1>::value);

struct X2 {
  operator xstd::opt_in<proj::mytrait, xstd::trait_heritability::heritable>();
};

struct Y2 : X2 { };

struct Z2 : Y2 { };

static_assert(  proj::mytrait<X2>::value);
static_assert(  proj::mytrait<Y2>::value);
static_assert(  proj::mytrait<Z2>::value);

struct X3 {
  operator xstd::opt_in<proj::mytrait, xstd::trait_heritability::heritable>();
};

struct Y3 : X3 {
  operator xstd::opt_out<proj::mytrait>();
};

struct Z3 : Y3 { };

static_assert(  proj::mytrait<X3>::value);
static_assert(! proj::mytrait<Y3>::value);
static_assert(! proj::mytrait<Z3>::value);

} // end namespace testproj
