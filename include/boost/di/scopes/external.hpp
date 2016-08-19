// Copyright (c) 2012-2016 Krzysztof Jusiak (krzysztof at jusiak dot net)
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
#ifndef BOOST_DI_SCOPES_EXTERNAL_HPP
#define BOOST_DI_SCOPES_EXTERNAL_HPP

#include "boost/di/fwd.hpp"
#include "boost/di/aux_/type_traits.hpp"
#include "boost/di/concepts/creatable.hpp"
#include "boost/di/type_traits/external_traits.hpp"

namespace scopes {

template <class T, class TExpected, class TGiven>
struct arg {
  using type = T;
  using expected = TExpected;
  using given = TGiven;
};

template <class T>
struct wrapper_traits {
  using type = wrappers::unique<class unique, T>;
};

template <class T>
struct wrapper_traits<std::shared_ptr<T>> {
  using type = wrappers::shared<class shared, T>;
};

template <class T>
struct wrapper_traits<std::shared_ptr<T>&> {
  using type = wrappers::shared<class shared, T, std::shared_ptr<T>&>;
};

template <class T>
using wrapper_traits_t = typename wrapper_traits<T>::type;

__BOOST_DI_HAS_TYPE(has_result_type, result_type);

template <class TGiven, class TInjector, class... Ts>
struct is_expr : aux::integral_constant<bool, aux::is_callable_with<TGiven, TInjector, Ts...>::value && !has_result_type<TGiven>::value> {};

template <class U, class TScope>
class external {
public:
  template <class TExpected, class TGiven>
  class scope {
   public:
    template <class T, class TInjector>
    using is_referable = typename TScope::template scope<TExpected, TGiven>::template is_referable<T, TInjector>;

    explicit scope(U object) : object_(object) {}

    template <class TInjector>
    struct provider {
      using injector_t = TInjector;

      template <class TMemory = type_traits::heap>
      decltype(auto) get(const TMemory& = {}) const {
        return object_;
      }

      const TInjector& injector_;
      U object_;
    };

    template <class T, class TName, class TProvider>
    static decltype(typename TScope::template scope<TExpected, TGiven>{}.template try_create<T, TName>(
        aux::declval<provider<typename TProvider::injector_t>>()))
    try_create(const TProvider&);

    template <class T, class TName, class TProvider>
    auto create(const TProvider& pr) {
      using scope = typename TScope::template scope<TExpected, TGiven>;
      return scope{}.template create<T, TName>(provider<typename TProvider::injector_t>{*pr.injector_, object_});
    }

    U object_;
  };
};

struct external_ref {
  template<class, class>
    struct scope {
      template <class T, class TInjector>
      using is_referable = aux::true_type;

      template <class, class, class TProvider>
      static auto try_create(const TProvider& p) -> wrapper_traits_t<decltype(p.get())>;

      template <class, class, class TProvider>
      auto create(const TProvider& p) {
        return wrapper_traits_t<decltype(p.get())>{p.get()};
      }
    };
};

template <class U, class TScope>
struct external_expr {
  template <class TExpected, class TGiven>
  struct scope {
    template <class, class>
    using is_referable =
        aux::integral_constant<bool, !aux::is_callable<TExpected>::value || !has_result_type<TExpected>::value>;

    template <class T, class TInjector>
    struct provider {
      using injector_t = TInjector;

      template <class TMemory = type_traits::heap,
         __BOOST_DI_REQUIRES(aux::always<TMemory>::value &&
                             !is_expr<TGiven, TInjector, const arg<T, TExpected, TGiven>&>::value && !is_expr<TGiven, TInjector>::value &&
                             aux::is_callable<TGiven>::value &&
                             aux::is_callable<TExpected>::value) = 0>
      decltype(auto) get(const TMemory& = {}) const {
        return object_;
      }

      template <class TMemory = type_traits::heap,
        __BOOST_DI_REQUIRES(aux::always<TMemory>::value &&
                            !is_expr<TGiven, TInjector, const arg<T, TExpected, TGiven>&>::value && !is_expr<TGiven, TInjector>::value &&
                            aux::is_callable_with<TGiven>::value &&
                            !aux::is_callable<TExpected>::value) = 0>
      decltype(auto) get(const TMemory& = {}) const {
        return object_();
      }

     template <class TMemory = type_traits::heap,
        __BOOST_DI_REQUIRES(aux::always<TMemory>::value &&
                            is_expr<TGiven, TInjector>::value && !is_expr<TGiven, TInjector, const arg<T, TExpected, TGiven>&>::value) = 0>
      decltype(auto) get(const TMemory& = {}) const {
        return object_(injector_);
      }

      template <class TMemory = type_traits::heap,
        __BOOST_DI_REQUIRES(aux::always<TMemory>::value &&
                            !is_expr<TGiven, TInjector>::value && is_expr<TGiven, TInjector, const arg<T, TExpected, TGiven>&>::value) = 0>
      decltype(auto) get(const TMemory& = {}) const {
        return object_(injector_, arg<T, TExpected, TGiven>{});
      }

      const TInjector& injector_;
      const TGiven& object_;
    };

    explicit scope(const TGiven& object) : object_(object) {}

    template<class T, class TInjector>
    using scope__ = typename type_traits::external_traits_t<
      TScope, decltype(aux::declval<provider<T, TInjector>>().get())
    >::template scope<TExpected, TGiven>;

    template <class T, class TName, class TProvider>
    static decltype(scope__<T, typename TProvider::injector_t>{}.template try_create<T, TName>(
        aux::declval<provider<T, typename TProvider::injector_t>>()))
    try_create(const TProvider&);

    template <class T, class TName, class TProvider>
    auto create(const TProvider& p) {
      using scope = scope__<T, typename TProvider::injector_t>;
      return scope{}.template create<T, TName>(provider<T, typename TProvider::injector_t>{*p.injector_, object_});
    }

    TGiven object_;
  };
};

} // scopes

#endif
