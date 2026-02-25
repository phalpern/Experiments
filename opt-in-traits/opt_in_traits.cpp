#include <type_traits>

namespace xstd {

using namespace std;

enum class trait_scope { disabled, noninherited, inherited };

template <template <class> class Trait>
struct opt_in_defaults
{
  constexpr static trait_scope default_scope = trait_scope::noninherited;
  constexpr static trait_scope max_scope     = trait_scope::inherited;
};

template <template <class> class Trait,
          trait_scope Scope = opt_in_defaults<Trait>::default_scope>
struct opt_in { };

template <template <class> class Trait>
using opt_out = opt_in<Trait, trait_scope::disabled>;

template <class Tp, template <class> class Trait, trait_scope>
struct __opt_bundle
{
  constexpr static trait_scope Scope = trait_scope::disabled;
  using Targ  = void;
};

template <class Tp, template <class> class Trait, trait_scope Sp>
requires requires(Tp& t) { t.operator opt_in<Trait, Sp>(); }
struct __opt_bundle<Tp, Trait, Sp>
{
  template <class B> static B getTarg(opt_in<Trait, Sp> (B::*)());

  constexpr static trait_scope Scope = Sp;
  using Targ  = decltype(getTarg(&Tp::operator opt_in<Trait, Scope>));
};

template <class B1, class B2,
          class B1Targ = typename B1::Targ, class B2Targ = typename B2::Targ>
struct __deeper_bundle {
  // Return deeper bundle
  static_assert(is_same_v<B1, B2> || ! is_same_v<B1Targ, B2Targ>,
                "Cannot opt in/out of same trait twice");
  using type = conditional_t<is_base_of_v<B1Targ, B2Targ>, B2, B1>;
};

template <class B1, class B2>
struct __deeper_bundle<B1, B2, void, void>
{
  // Neither B1::Targ nor B2::Targ is a base class of the other.  Choose
  // arbitrary one.
  using type = B1;
};

template <class B1, class B2, class B1Targ>
struct __deeper_bundle<B1, B2, B1Targ, void>
{
  // B2Targ is void, choose B1.
  using type = B1;
};

template <class B1, class B2, class B2Targ>
struct __deeper_bundle<B1, B2, void, B2Targ>
{
  // B1Targ is void, choose B2.
  using type = B2;
};

template <class B1, class B2>
using __deeper_bundle_t = typename __deeper_bundle<B1, B2>::type;

template <class Tp, template <class> class Trait>
struct __detect_opt_in_imp
{
  using d_bundle = __opt_bundle<Tp, Trait, trait_scope::disabled>;
  using n_bundle = __opt_bundle<Tp, Trait, trait_scope::noninherited>;
  using i_bundle = __opt_bundle<Tp, Trait, trait_scope::inherited>;

  // find the bundle representing the operator instance at the  most-derived
  // level.
  using deepest = __deeper_bundle_t<__deeper_bundle_t<d_bundle, n_bundle>,
                                    i_bundle>;
  constexpr static trait_scope scope = deepest::Scope;

  static_assert(scope <= opt_in_defaults<Trait>::max_scope,
                "Cannot declare this trait as inherited");

  constexpr static bool value = (scope == trait_scope::disabled  ? false :
                                 scope == trait_scope::inherited ? true :
                                 is_same_v<Tp, typename deepest::Targ>);
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
  constexpr static trait_scope default_scope = trait_scope::noninherited;
  constexpr static trait_scope max_scope     = trait_scope::inherited;
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
  operator xstd::opt_in<proj::mytrait, xstd::trait_scope::inherited>();
};

struct Y2 : X2 { };

struct Z2 : Y2 { };

static_assert(  proj::mytrait<X2>::value);
static_assert(  proj::mytrait<Y2>::value);
static_assert(  proj::mytrait<Z2>::value);

struct X3 {
  operator xstd::opt_in<proj::mytrait, xstd::trait_scope::inherited>();
};

struct Y3 : X3 {
  operator xstd::opt_out<proj::mytrait>();
};

struct Z3 : Y3 { };

static_assert(  proj::mytrait<X3>::value);
static_assert(! proj::mytrait<Y3>::value);
static_assert(! proj::mytrait<Z3>::value);

} // end namespace testproj
