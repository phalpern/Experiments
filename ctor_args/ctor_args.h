/* ctor_args.h                                                        -*-C++-*-
 *
 * Copyright (C) 2024 Pablo Halpern <phalpern@halpernwightsoftware.com>
 * Distributed under the Boost Software License - Version 1.0
 */

#include <tuple>
#include <array>
#include <type_traits>

namespace xstd
{
using namespace std;

constexpr size_t max_ctor_args = 5;

// Can hold a pack of up to `max_ctor_args` ctor arguments for `Tp`
template <class Tp>
struct ctor_args
{
  using value_type = Tp;

  // Sadly, these constructors cannot be constexpr because they use
  // type erasure.
  template <class... Args>
  requires constructible_from<Tp, Args&&...>
  ctor_args(Args&&... args) : p_make_value(make_value<Args...>)
  {
    static_assert(sizeof...(args) <= max_ctor_args,
                  "Exceeded number of supported ctor args");
    std::construct_at(static_cast<tuple<Args&&...>*>((void*)buffer.bytes),
                      std::forward<Args>(args)...);
  }

  template <class F>
  requires constructible_from<Tp, invoke_result_t<const F>>
  ctor_args(F&& f) : p_make_value(lazy_value<F>)
  {
    static_assert(sizeof(F) <= sizeof(buffer),
                  "Exceeded maximum size for creation function");
    std::construct_at(static_cast<F*>((void*)buffer.bytes),
                      std::forward<F>(f));
  }

  constexpr Tp operator()() const { return p_make_value(&buffer); }

private:
  template <class... Args>
  static Tp make_value(const void *p_arg_pack);

  template <class F>
  static Tp lazy_value(const void *f);

  union buf {
    array<tuple<int&&>, max_ctor_args> sizer;  // Set size and alignment
    char bytes[sizeof(sizer)];
    constexpr buf() {}
  };

  Tp   (*p_make_value)(const void *p_arg_pack);
  buf  buffer;
};

template <class Tp>
template <class... Args>
Tp ctor_args<Tp>::make_value(const void *p_arg_pack)
{
  using tp_arg_pack = const tuple<Args&&...>;
  return make_from_tuple<Tp>(*static_cast<tp_arg_pack*>(p_arg_pack));
}

template <class Tp>
template <class F>
Tp ctor_args<Tp>::lazy_value(const void *f)
{
  return (*static_cast<const F*>(f))();
}

} // close namespace xstd

// Local Variables:
// c-basic-offset: 2
// End:
