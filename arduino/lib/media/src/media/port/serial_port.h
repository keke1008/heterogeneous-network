#pragma once

#include "../detector.h"
#include "../serial.h"
#include "../uhf.h"
#include "../wifi.h"
#include "./interactor.h"

namespace media {
    template <nb::AsyncReadableWritable RW>
    class SerialPortMediaPort {
        memory::Static<net::link::FrameBroker> broker_;
        etl::variant<
            MediaDetector<RW>,
            memory::Static<uhf::UhfInteractor<RW>>,
            memory::Static<wifi::WifiInteractor<RW>>,
            memory::Static<serial::SerialInteractor<RW>>>
            state_;

      public:
        SerialPortMediaPort() = delete;
        SerialPortMediaPort(const SerialPortMediaPort &) = delete;
        SerialPortMediaPort(SerialPortMediaPort &&) = delete;
        SerialPortMediaPort &operator=(const SerialPortMediaPort &) = delete;
        SerialPortMediaPort &operator=(SerialPortMediaPort &&) = delete;

        SerialPortMediaPort(
            memory::Static<RW> &serial,
            memory::Static<net::link::MeasuredLinkFrameQueue> &queue,
            util::Time &time
        )
            : broker_{queue},
              state_{MediaDetector<RW>{serial, time}} {}

        inline void initialize_media_port(net::link::MediaPortNumber port) {
            broker_->initialize_media_port(port);
        }

        void execute(net::frame::FrameService &fs, util::Time &time, util::Rand &rand) {
            if (etl::holds_alternative<MediaDetector<RW>>(state_)) {
                MediaDetector<RW> &detector = etl::get<MediaDetector<RW>>(state_);
                auto poll_media_type = detector.poll(time);
                if (poll_media_type.is_pending()) {
                    return;
                }
                auto &rw = *detector.stream();

                switch (poll_media_type.unwrap()) {
                case net::link::MediaType::UHF: {
                    state_.template emplace<memory::Static<uhf::UhfInteractor<RW>>>(rw, broker_);
                    break;
                }
                case net::link::MediaType::Wifi: {
                    state_.template emplace<memory::Static<wifi::WifiInteractor<RW>>>(
                        rw, broker_, time
                    );
                    break;
                }
                case net::link::MediaType::Serial: {
                    state_.template emplace<memory::Static<serial::SerialInteractor<RW>>>(
                        rw, broker_
                    );
                    break;
                }
                }
            }

            etl::visit(
                util::Visitor{
                    [&](MediaDetector<RW> &) {},
                    [&](memory::Static<uhf::UhfInteractor<RW>> &media) {
                        media->execute(fs, time, rand);
                    },
                    [&](memory::Static<wifi::WifiInteractor<RW>> &media) {
                        media->execute(fs, time);
                    },
                    [&](memory::Static<serial::SerialInteractor<RW>> &media) {
                        media->execute(fs, time);
                    },
                },
                state_
            );
        }

        inline MediaInteractorRef<RW, false> get_media_interactor_ref() {
            return etl::visit<MediaInteractorRef<RW, false>>(
                util::Visitor{
                    [](MediaDetector<RW> &) { return etl::monostate{}; },
                    [](auto &media) { return etl::ref(media); },
                },
                state_
            );
        }

        inline MediaInteractorRef<RW, true> get_media_interactor_cref() const {
            return etl::visit<MediaInteractorRef<RW, true>>(
                util::Visitor{
                    [](const MediaDetector<RW> &) { return etl::monostate{}; },
                    [](const auto &media) { return etl::cref(media); },
                },
                state_
            );
        }
    };
} // namespace media
