#pragma once

#include "../ethernet.h"
#include "./interactor.h"

namespace media {
    class EthernetShieldMediaPort {
        memory::Static<net::link::FrameBroker> broker_;
        memory::Static<ethernet::EthernetInteractor> interactor_;

      public:
        EthernetShieldMediaPort() = delete;
        EthernetShieldMediaPort(const EthernetShieldMediaPort &) = delete;
        EthernetShieldMediaPort(EthernetShieldMediaPort &&) = delete;
        EthernetShieldMediaPort &operator=(const EthernetShieldMediaPort &) = delete;
        EthernetShieldMediaPort &operator=(EthernetShieldMediaPort &&) = delete;

        EthernetShieldMediaPort(memory::Static<net::link::LinkFrameQueue> &queue, util::Time &time)
            : broker_{queue},
              interactor_{broker_, time} {}

        inline void initialize(util::Rand &rand) {
            interactor_->initialize(rand);
        }

        inline void initialize_media_port(net::link::MediaPortNumber port) {
            broker_->initialize_media_port(port);
        }

        inline void execute(net::frame::FrameService &fs, util::Time &time) {
            interactor_->execute(fs, time);
        }

        template <nb::AsyncReadableWritable RW>
        inline MediaInteractorRef<RW, false> get_media_interactor_ref() {
            return etl::ref(interactor_);
        }

        template <nb::AsyncReadableWritable RW>
        inline MediaInteractorRef<RW, true> get_media_interactor_cref() const {
            return etl::cref(interactor_);
        }
    };
} // namespace media
