/**
 * `etl::variant`はコピーやムーブ可能かどうか正しく判定できないため，
 * このファイルでは，`etl::variant`を置き換える`tl::Variant`を実装する．
 *
 */

#pragma once
#include <etl/variant.h>

#include <etl/placement_new.h>
#include <etl/type_select.h>
#include <etl/utility.h>
#include <log.h>
#include <util/type_traits.h>
#include <util/visitor.h>

namespace tl {
    namespace variant {
        template <uint8_t Index, typename... Ts>
        union Storage;

        template <uint8_t Index>
        union Storage<Index> {};

        template <uint8_t Index, typename T, typename... Ts>
        union Storage<Index, T, Ts...> {
          private:
            using Next = Storage<Index + 1, Ts...>;

            T value_;
            Next next_;

          public:
            Storage &operator=(const Storage &) = delete;
            Storage &operator=(Storage &&) = delete;

            ~Storage() {}

            Storage() {}

            template <
                typename U,
                typename = etl::
                    enable_if_t<etl::is_one_of_v<etl::decay_t<U>, T, Ts...>, etl::decay_t<U> *>>
            Storage(U &&value) : value_{etl::forward<U>(value)} {}

            template <typename U, typename = etl::enable_if_t<etl::is_one_of_v<U, T, Ts...>, U *>>
            inline U *as_ptr() {
                static_assert(etl::is_one_of_v<U, T, Ts...>, "Type not in variant");
                return reinterpret_cast<U *>(&value_);
            }

            inline void *as_void_ptr() {
                return reinterpret_cast<void *>(&value_);
            }

            inline void *as_void_ptr() const {
                return const_cast<Storage *>(this)->as_void_ptr();
            }
        };

        template <typename... Ts>
        struct Visitor : Ts... {
            using Ts::operator()...;
        };

        template <typename... Ts>
        struct Inner {
            Storage<0, Ts...> storage_;

            template <typename T>
            inline T *as_ptr() {
                return storage_.template as_ptr<T>();
            }

          public:
            Inner &operator=(const Inner &) = delete;
            Inner &operator=(Inner &&) = delete;

            ~Inner() {}

            template <typename T>
            Inner(T &&value) {
                using Type = etl::decay_t<T>;
                new (as_ptr<Type>()) Type{etl::forward<Type>(value)};
            }

            template <typename T>
            inline void set(T &&value) {
                using Type = etl::decay_t<T>;
                new (as_ptr<Type>()) Type{etl::forward<T>(value)};
            }

            template <typename T, typename... Args>
            inline void emplace(Args &&...args) {
                new (as_ptr<T>()) T{etl::forward<Args>(args)...};
            }

            template <typename T>
            inline auto &get() {
                return *as_ptr<T>();
            }

            template <typename T>
            inline const auto &get() const {
                return *as_ptr<T>();
            }

            using DestroyFn = auto (*)(void *) -> void;

            template <typename Arg>
            inline static void do_destroy(void *arg) {
                reinterpret_cast<Arg *>(arg)->~Arg();
            }

            inline void destroy(uint8_t index) {
                DestroyFn table[] = {reinterpret_cast<DestroyFn>(do_destroy<Ts>)...};
                table[index](storage_.as_void_ptr());
            }

            using CopyFn = auto (*)(const void *arg) -> Inner;

            template <typename Arg>
            inline static Inner do_copy(void *arg) {
                return Inner{*reinterpret_cast<Arg *>(arg)};
            }

            inline Inner copy(uint8_t index) const & {
                CopyFn table[] = {reinterpret_cast<CopyFn>(do_copy<Ts>)...};
                return table[index](storage_.as_void_ptr());
            }

            using MoveFn = auto (*)(void *arg) -> Inner;

            template <typename Arg>
            inline static Inner do_move(void *arg) {
                return Inner{etl::move(*reinterpret_cast<Arg *>(arg))};
            }

            inline Inner move(uint8_t index) && {
                MoveFn table[] = {reinterpret_cast<MoveFn>(do_move<Ts>)...};
                return table[index](storage_.as_void_ptr());
            }

            template <typename R, typename Visitor>
            using VisitFn = auto (*)(Visitor *, void *) -> R;

            template <typename R, typename Visitor, typename Arg>
            inline static R do_visit(Visitor &&visitor, void *arg) {
                return visitor(*reinterpret_cast<Arg *>(arg));
            }

            template <typename R, typename Visitor>
            inline constexpr auto visit(uint8_t index, Visitor &&visitor) & -> R {
                VisitFn<R, Visitor> table[] = {
                    reinterpret_cast<VisitFn<R, Visitor>>(do_visit<R, Visitor, Ts>)...,
                };
                return table[index](&visitor, storage_.as_void_ptr());
            }

            template <typename R, typename Visitor>
            using ConstVisitFn = auto (*)(const Visitor *, void *) -> R;

            template <typename R, typename Visitor>
            inline constexpr auto visit(uint8_t index, Visitor &&visitor) const & -> R {
                ConstVisitFn<R, Visitor> table[] = {
                    reinterpret_cast<ConstVisitFn<R, Visitor>>(do_visit<R, const Visitor, Ts>)...,
                };
                return table[index](&visitor, storage_.as_void_ptr());
            }

            template <typename R, typename Visitor, typename Arg>
            inline static R do_move_visit(Visitor &&visitor, void *arg) {
                return visitor(etl::move(*reinterpret_cast<Arg *>(arg)));
            }

            template <typename R, typename Visitor>
            inline constexpr auto visit(uint8_t index, Visitor &&visitor) && -> R {
                VisitFn<R, Visitor> table[] = {
                    reinterpret_cast<VisitFn<R, Visitor>>(do_move_visit<R, Visitor, Ts>)...,
                };
                return table[index](&visitor, storage_.as_void_ptr());
            }
        };

        struct VisitAutoReturn {};

        template <typename R, typename F, typename... Ts>
        struct VisitReturn {
            using type = etl::conditional_t<
                etl::is_same_v<R, VisitAutoReturn>,
                etl::common_type_t<util::invoke_result_t<F, Ts>...>,
                etl::decay_t<R>>;
        };

        template <typename R, typename F, typename... Ts>
        using VisitReturnType = typename VisitReturn<R, F, Ts...>::type;

        template <typename T>
        constexpr uint8_t index_of() {
            PANIC();
            return 0;
        }

        template <typename T, typename U, typename... Ts>
        constexpr uint8_t index_of() {
            if constexpr (etl::is_same_v<T, U>) {
                return 0;
            } else {
                return 1 + index_of<T, Ts...>();
            }
        }

        template <typename T, typename... Ts>
        struct IndexOf : etl::integral_constant<uint8_t, index_of<T, Ts...>()> {};

        template <uint8_t Index, typename... Ts>
        struct TypeAt;

        template <uint8_t Index, typename T, typename... Ts>
        struct TypeAt<Index, T, Ts...> {
            using type = typename TypeAt<Index - 1, Ts...>::type;
        };

        template <typename T, typename... Ts>
        struct TypeAt<0, T, Ts...> {
            using type = T;
        };
    } // namespace variant

    template <typename... Ts>
    class Variant {
        using Inner = variant::Inner<Ts...>;

        uint8_t index_;
        Inner inner_;

        template <uint8_t>
        using TypeAt = typename variant::TypeAt<0, Ts...>::type;

        template <typename T>
        static constexpr uint8_t IndexOf = variant::index_of<T, Ts...>();

      public:
        Variant() : index_{0}, inner_{TypeAt<0>{}} {}

        template <typename T, typename = etl::enable_if_t<etl::is_one_of_v<T, Ts...>>>
        inline Variant(T &&value)
            : index_{IndexOf<etl::decay_t<T>>},
              inner_{etl::forward<T>(value)} {}

        inline Variant(const Variant &other)
            : index_{other.index_},
              inner_{other.inner_.copy(other.index_)} {}

        inline Variant(Variant &&other)
            : index_{other.index_},
              inner_{etl::move(other.inner_).move(other.index_)} {}

        inline Variant &operator=(const Variant &other) {
            inner_.destroy(index_);
            index_ = other.index_;
            inner_ = other.inner_.copy(index_);
            return *this;
        }

        inline Variant &operator=(Variant &&other) {
            inner_.destroy(index_);
            index_ = other.index_;
            inner_ = etl::move(other.inner_).move(index_);
            return *this;
        }

        template <typename T, typename = etl::enable_if_t<etl::is_one_of_v<etl::decay_t<T>, Ts...>>>
        inline Variant &operator=(T &&value) {
            inner_.destroy(index_);
            index_ = IndexOf<etl::decay_t<T>>;
            inner_.set(etl::forward<T>(value));
            return *this;
        }

        inline ~Variant() {
            inner_.destroy(index_);
        }

        inline constexpr uint8_t index() const {
            return index_;
        }

        template <typename T>
        inline constexpr bool holds_alternative() const {
            return index_ == IndexOf<T>;
        }

        template <typename T>
        inline constexpr T &get() {
            if (!holds_alternative<T>()) {
                PANIC();
            }
            return inner_.template get<T>();
        }

        template <typename T>
        inline constexpr const T &get() const {
            if (!holds_alternative<T>()) {
                PANIC();
            }
            return inner_.template get<T>();
        }

        template <typename T, typename... Args>
        inline auto emplace(Args &&...args) -> etl::enable_if_t<
            etl::conjunction_v<etl::is_one_of<T, Ts...>, etl::is_constructible<T, Args...>>> {
            using Type = etl::decay_t<T>;
            inner_.destroy(index_);
            index_ = IndexOf<Type>;
            inner_.template emplace<Type>(etl::forward<Args>(args)...);
        }

        template <
            typename R = variant::VisitAutoReturn,
            typename... Fs,
            typename = etl::enable_if_t<etl::conjunction_v<
                util::is_invocable<variant::Visitor<etl::decay_t<Fs>...>, Ts &>...>>>
        inline constexpr variant::VisitReturnType<R, variant::Visitor<etl::decay_t<Fs>...>, Ts &...>
        visit(Fs &&...f) & {
            using Visitor = variant::Visitor<etl::decay_t<Fs>...>;
            using ReturnType = variant::VisitReturnType<R, Visitor, Ts &...>;
            return inner_.template visit<ReturnType, Visitor>(
                index_, Visitor{etl::forward<Fs>(f)...}
            );
        }

        template <
            typename R = variant::VisitAutoReturn,
            typename... Fs,
            typename = etl::enable_if_t<etl::conjunction_v<
                util::is_invocable<variant::Visitor<etl::decay_t<Fs>...>, const Ts &>...>>>
        inline constexpr variant::
            VisitReturnType<R, variant::Visitor<etl::decay_t<Fs>...>, const Ts &...>
            visit(Fs &&...f) const & {
            using Visitor = variant::Visitor<etl::decay_t<Fs>...>;
            using ReturnType = variant::VisitReturnType<R, Visitor, const Ts &...>;
            return inner_.template visit<ReturnType, Visitor>(
                index_, Visitor{etl::forward<Fs>(f)...}
            );
        }

        template <
            typename R = variant::VisitAutoReturn,
            typename... Fs,
            typename = etl::enable_if_t<etl::conjunction_v<
                util::is_invocable<variant::Visitor<etl::decay_t<Fs>...>, Ts &&>...>>>
        inline constexpr variant::
            VisitReturnType<R, variant::Visitor<etl::decay_t<Fs>...>, Ts &&...>
            visit(Fs &&...f) && {
            using Visitor = variant::Visitor<etl::decay_t<Fs>...>;
            using ReturnType = variant::VisitReturnType<R, Visitor, Ts &&...>;
            return etl::move(inner_).template visit<ReturnType, Visitor>(
                index_, Visitor{etl::forward<Fs>(f)...}
            );
        }
    };

    template <typename T, typename... Ts>
    constexpr bool holds_alternative(const Variant<Ts...> &v) {
        return v.template holds_alternative<T>();
    }

    template <typename T, typename... Ts>
    constexpr T &get(Variant<Ts...> &v) {
        return v.template get<T>();
    }

    template <typename T, typename... Ts>
    constexpr const T &get(const Variant<Ts...> &v) {
        return v.template get<T>();
    }

    template <typename R = variant::VisitAutoReturn, typename F, typename... Ts>
    constexpr decltype(auto) visit(F &&f, Variant<Ts...> &v) {
        return v.template visit<R, F>(etl::forward<F>(f));
    }

    struct Monostate {};
} // namespace tl
