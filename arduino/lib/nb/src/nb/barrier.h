#pragma once

#include <memory/pair_ptr.h>
#include <nb/poll.h>

namespace nb {
    namespace barrier {
        class Internal : public memory::PairPtr<Internal, Internal> {};
    } // namespace barrier

    class BarrierController {
        barrier::Internal internal_;

      public:
        BarrierController() = delete;
        BarrierController(const BarrierController &) = delete;
        BarrierController(BarrierController &&) = default;
        BarrierController &operator=(const BarrierController &) = delete;
        BarrierController &operator=(BarrierController &&) = default;

        explicit BarrierController(barrier::Internal &&internal) : internal_{etl::move(internal)} {}

        inline void release() {
            internal_.unpair();
        }
    };

    class Barrier {
        barrier::Internal internal_;

      public:
        Barrier() = delete;
        Barrier(const Barrier &) = delete;
        Barrier(Barrier &&) = default;
        Barrier &operator=(const Barrier &) = delete;
        Barrier &operator=(Barrier &&) = default;

        explicit Barrier(barrier::Internal &&internal) : internal_{etl::move(internal)} {}

        static inline Barrier dangling() {
            return Barrier{barrier::Internal{nullptr}};
        }

        inline nb::Poll<void> poll_wait() {
            return internal_.has_pair() ? nb::pending : nb::ready();
        }

        inline etl::optional<BarrierController> make_controller() {
            if (internal_.has_pair()) {
                return etl::nullopt;
            }

            barrier::Internal internal{&internal_};
            return etl::optional(BarrierController{etl::move(internal)});
        }
    };

    inline etl::pair<Barrier, BarrierController> make_barrier() {
        barrier::Internal internal1{nullptr};
        barrier::Internal internal2{&internal1};
        internal1.unsafe_set_pair(&internal2);

        return {
            Barrier{etl::move(internal1)},
            BarrierController{etl::move(internal2)},
        };
    }
} // namespace nb
