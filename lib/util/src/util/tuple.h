#pragma once

#include <etl/utility.h>
#include <stdint.h>

namespace util {
    template <typename... Ts>
    class Tuple;

    template <typename... Ts>
    Tuple(Ts...) -> Tuple<Ts...>;

    template <>
    class Tuple<> {};

    namespace private_tuple_last {
        template <typename T, typename... Ts>
        struct tuple_last {
            using type = typename tuple_last<Ts...>::type;
        };

        template <typename T>
        struct tuple_last<T> {
            using type = T;
        };

        template <typename... Ts>
        using tuple_last_t = typename tuple_last<Ts...>::type;
    } // namespace private_tuple_last

    namespace private_tuple_get {
        template <uint8_t I, typename Head, typename... Tail>
        struct tuple_type;

        template <uint8_t I, typename... Ts>
        using tuple_type_t = typename tuple_type<I, Ts...>::type;

        template <uint8_t I, typename Head, typename... Tail>
        struct tuple_type {
            using type = tuple_type_t<I - 1, Tail...>;
        };

        template <typename Head, typename... Tail>
        struct tuple_type<0, Head, Tail...> {
            using type = Tuple<Head, Tail...>;
        };
    } // namespace private_tuple_get

    template <uint8_t I, typename... Ts>
    inline constexpr decltype(auto) get(Tuple<Ts...> &tuple) {
        static_assert(I < sizeof...(Ts), "Index out of range");
        return static_cast<private_tuple_get::tuple_type_t<I, Ts...> &>(tuple).head();
    }

    template <uint8_t I, typename... Ts>
    inline constexpr decltype(auto) get(const Tuple<Ts...> &tuple) {
        static_assert(I < sizeof...(Ts), "Index out of range");
        return static_cast<const private_tuple_get::tuple_type_t<I, Ts...> &>(tuple).head();
    }

    template <uint8_t I, typename... Ts>
    inline constexpr decltype(auto) get(Tuple<Ts...> &&tuple) {
        static_assert(I < sizeof...(Ts), "Index out of range");
        return static_cast<private_tuple_get::tuple_type_t<I, Ts...> &&>(tuple).head();
    }

    template <uint8_t I, typename... Ts>
    inline constexpr decltype(auto) get(const Tuple<Ts...> &&tuple) {
        static_assert(I < sizeof...(Ts), "Index out of range");
        return static_cast<private_tuple_get::tuple_type_t<I, Ts...> &&>(tuple).head();
    }

    namespace private_tuple_apply {
        template <typename T, typename Tuple, size_t... I>
        constexpr decltype(auto) apply_impl(T &&f, Tuple &&t, etl::index_sequence<I...>) {
            return f(get<I>(etl::forward<Tuple>(t))...);
        }
    } // namespace private_tuple_apply

    template <typename F, typename TTuple>
    constexpr decltype(auto) apply(F &&f, TTuple &&t) {
        return private_tuple_apply::apply_impl(
            etl::forward<F>(f), etl::forward<TTuple>(t),
            etl::make_index_sequence<etl::decay_t<TTuple>::size>()
        );
    }

    template <typename Head, typename... Tail>
    class Tuple<Head, Tail...> : public Tuple<Tail...> {
        Head head_;

      public:
        static inline constexpr uint8_t size = sizeof...(Tail) + 1;

        Tuple() = default;

        template <typename T, typename... Ts>
        Tuple(T &&head, Ts &&...tail)
            : Tuple<Tail...>{etl::forward<Ts>(tail)...},
              head_{etl::forward<T>(head)} {}

        Head &head() {
            return head_;
        }

        const Head &head() const {
            return head_;
        }

        auto &last() {
            return static_cast<Tuple<private_tuple_last::tuple_last_t<Head, Tail...>> &>(*this)
                .head();
        }

        const auto &last() const {
            return static_cast<const Tuple<private_tuple_last::tuple_last_t<Head, Tail...>> &>(*this
            )
                .head();
        }
    };
} // namespace util

namespace std {
    template <typename T>
    struct tuple_size;

    template <typename... Ts>
    struct tuple_size<util::Tuple<Ts...>> {
        static constexpr uint8_t value = util::Tuple<Ts...>::size;
    };

    template <typename... Ts>
    struct tuple_size<const util::Tuple<Ts...>> {
        static constexpr uint8_t value = util::Tuple<Ts...>::size;
    };

    template <size_t I, typename T>
    class tuple_element;

    template <typename T, typename... Ts>
    class tuple_element<0, util::Tuple<T, Ts...>> {
      public:
        using type = T;
    };

    template <size_t I, typename T, typename... Ts>
    class tuple_element<I, util::Tuple<T, Ts...>> {
      public:
        using type = typename tuple_element<I - 1, util::Tuple<Ts...>>::type;
    };

    template <typename T, typename... Ts>
    class tuple_element<0, const util::Tuple<T, Ts...>> {
      public:
        using type = const T;
    };

    template <size_t I, typename T, typename... Ts>
    class tuple_element<I, const util::Tuple<T, Ts...>> {
      public:
        using type = typename tuple_element<I - 1, const util::Tuple<Ts...>>::type;
    };
} // namespace std
