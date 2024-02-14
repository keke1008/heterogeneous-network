#pragma once

#include <etl/type_traits.h>
#include <etl/utility.h>

namespace tl {
    namespace private_invoke_result {
        template <typename T, typename = void>
        struct invoke_result_impl {};

        template <typename F, typename... Args>
        struct invoke_result_impl<
            F(Args...),
            etl::void_t<decltype(etl::declval<F>()(etl::declval<Args>()...))>> {
            using type = decltype(etl::declval<F>()(etl::declval<Args>()...));
        };

    } // namespace private_invoke_result

    template <typename F, typename... Args>
    struct invoke_result : private_invoke_result::invoke_result_impl<F && (Args && ...)> {};

    template <typename F, typename... Args>
    using invoke_result_t = typename invoke_result<F, Args...>::type;

    namespace private_is_invocable {
        template <typename T, typename = void>
        struct is_invocable_impl : etl::false_type {};

        template <typename F, typename... Args>
        struct is_invocable_impl<
            F(Args...),
            etl::void_t<decltype(etl::declval<F>()(etl::declval<Args>()...))>> : etl::true_type {};
    } // namespace private_is_invocable

    template <typename F, typename... Args>
    struct is_invocable : private_is_invocable::is_invocable_impl<F && (Args && ...)> {};

    template <typename F, typename... Args>
    inline constexpr bool is_invocable_v = is_invocable<F, Args...>::value;

    namespace private_underlying_type {
        template <uint8_t ByteSize>
        struct unsigned_integral_type_by_size {};

        template <>
        struct unsigned_integral_type_by_size<1> {
            using type = uint8_t;
        };

        template <>
        struct unsigned_integral_type_by_size<2> {
            using type = uint16_t;
        };

        template <>
        struct unsigned_integral_type_by_size<4> {
            using type = uint32_t;
        };

        template <>
        struct unsigned_integral_type_by_size<8> {
            using type = uint64_t;
        };

        template <uint8_t ByteSize>
        using unsigned_integral_type_by_size_t =
            typename unsigned_integral_type_by_size<ByteSize>::type;
    } // namespace private_underlying_type

    template <typename T>
    struct underlying_type {
        static_assert(etl::is_enum_v<T>, "underlying_type must be an enum");
        using type = etl::conditional_t<
            etl::is_signed_v<T>,
            etl::make_signed_t<
                private_underlying_type::unsigned_integral_type_by_size_t<sizeof(T)>>,
            private_underlying_type::unsigned_integral_type_by_size_t<sizeof(T)>>;
    };

    template <typename T>
    using underlying_type_t = typename underlying_type<T>::type;
} // namespace tl
