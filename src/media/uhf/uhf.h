#pragma once

#include "./common.h"
#include "./executor.h"
#include "./modem.h"
#include <etl/circular_buffer.h>
#include <etl/optional.h>
#include <media/common.h>
#include <memory/circular_buffer.h>
#include <memory/pair_shared.h>
#include <nb/future.h>
#include <nb/lock.h>
#include <nb/serial.h>
#include <util/visitor.h>

namespace media::uhf {
    template <typename RawSerial>
    class UHF {
        inline constexpr static size_t TASK_BUFFER_SIZE = 3;

        using Serial = nb::serial::Serial<RawSerial>;

        nb::lock::Lock<Serial> serial_;
        etl::circular_buffer<uhf::executor::Task<Serial>, TASK_BUFFER_SIZE> tasks_;
        uhf::executor::Executor<Serial> executor_{etl::monostate{}};

      public:
        explicit UHF(nb::lock::Lock<Serial> &&serial) : serial_{etl::move(serial)} {}

        nb::Future<uhf::modem::SerialNumber> get_serial_number() {
            auto [future, promise] = nb::make_future_promise_pair<uhf::modem::SerialNumber>();
            tasks_.push(uhf::executor::Task<Serial>{
                uhf::executor::GetSerialNumberTask<Serial>{etl::move(promise)}});
            return etl::move(future);
        }

        nb::Future<nb::Empty> set_equipment_id() {}

        nb::Future<uhf::common::DataWriter<Serial>>
        send_data(uhf::common::ModemId destination, size_t length) {
            auto [future, promise] =
                nb::make_future_promise_pair<uhf::common::DataWriter<Serial>>();
            tasks_.push_back(uhf::executor::DataTransmissionTask<Serial>{
                etl::move(promise), destination, length});
            return etl::move(future);
        }

        nb::Future<etl::optional<uhf::common::DataReader<Serial>>> receive_data() {
            auto [future, promise] =
                nb::make_future_promise_pair<etl::optional<uhf::common::DataReader<Serial>>>();
            tasks_.push_back(uhf::executor::DataReceivingTask<Serial>{etl::move(promise)});
            return etl::move(future);
        }

        void execute() {
            if (etl::holds_alternative<etl::monostate>(executor_)) {
                if (tasks_.empty()) {
                    return;
                }
                auto serial = serial_.lock();
                if (!serial.has_value()) {
                    return;
                }
                executor_ = etl::visit(
                    [&](uhf::executor::GetSerialNumberTask<Serial> &&task) {
                        return task.into_executor(etl::move(serial));
                    },
                    [&](uhf::executor::SetEquipmentIdExecutor<Serial> &&task) {
                        return task.into_executor(etl::move(serial));
                    },
                    [&](uhf::executor::DataTransmissionTask<Serial> &&task) {
                        return task.into_executor(etl::move(serial));
                    },
                    [&](uhf::executor::DataReceivingTask<Serial> &&task) {
                        return task.into_executor(etl::move(serial));
                    },
                    tasks_.front()
                );
                tasks_.pop();
            }

            etl::visit(
                util::Visitor{
                    [&](auto &exe) {
                        auto result = exe.execute();
                        if (result.is_ready()) {
                            executor_ = etl::monostate{};
                        }
                    },
                    [&](etl::monostate) { /* unreachable */ },
                },
                executor_
            );
        }
    };
} // namespace media::uhf
