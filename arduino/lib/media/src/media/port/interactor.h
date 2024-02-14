#pragma once

#include "../ethernet.h"
#include "../serial.h"
#include "../uhf.h"
#include "../wifi.h"

namespace media {
    template <typename T, bool Const>
    using StaticRef = etl::conditional_t<
        Const,
        etl::reference_wrapper<const memory::Static<T>>,
        etl::reference_wrapper<memory::Static<T>>>;

    template <nb::AsyncReadableWritable RW, bool Const>
    class MediaInteractorRef {
        etl::variant<
            etl::monostate,
            StaticRef<uhf::UhfInteractor<RW>, Const>,
            StaticRef<wifi::WifiInteractor<RW>, Const>,
            StaticRef<serial::SerialInteractor<RW>, Const>,
            StaticRef<ethernet::EthernetInteractor, Const>>
            interactor_;

      public:
        MediaInteractorRef() = delete;
        MediaInteractorRef(const MediaInteractorRef &) = default;
        MediaInteractorRef(MediaInteractorRef &&) = default;
        MediaInteractorRef &operator=(const MediaInteractorRef &) = default;
        MediaInteractorRef &operator=(MediaInteractorRef &&) = default;

        MediaInteractorRef(etl::monostate) : interactor_{etl::monostate{}} {}

        template <typename T>
        MediaInteractorRef(T &&interactor) : interactor_{etl::ref(interactor)} {}

        template <typename T>
        MediaInteractorRef(const T &interactor) : interactor_{etl::cref(interactor)} {}

        template <typename T>
        inline T visit(auto &&f) const {
            return etl::visit<T>(
                util::Visitor{
                    [&](const etl::monostate &s) -> T { return f(s); },
                    [&](const auto &media) -> T { return f(*media.get()); },
                },
                interactor_
            );
        }

        template <typename T>
        inline decltype(auto) visit(auto &&f) {
            return etl::visit<T>(
                util::Visitor{
                    [&](etl::monostate &s) -> T { return f(s); },
                    [&](auto &media) -> T { return f(*media.get()); },
                },
                interactor_
            );
        }

        template <typename T>
        inline bool holds_alternative() const {
            return etl::holds_alternative<StaticRef<T, Const>>(interactor_);
        }

        template <typename T>
        inline T &get() {
            return *etl::get<StaticRef<T, Const>>(interactor_).get();
        }
    };
} // namespace media
