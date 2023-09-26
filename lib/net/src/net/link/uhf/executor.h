#pragma once

#include "../frame.h"
#include "./task.h"
#include <etl/circular_buffer.h>
#include <etl/optional.h>
#include <nb/poll.h>
#include <nb/stream/types.h>
#include <util/rand.h>
#include <util/tuple.h>
#include <util/visitor.h>

namespace net::link::uhf {
    class UHFExecutor {
        etl::optional<Task> task_;
        nb::stream::ReadableWritableStream &stream_;

      public:
        UHFExecutor(nb::stream::ReadableWritableStream &stream) : stream_{stream} {}

        inline bool is_supported_address_type(AddressType type) const {
            return type == AddressType::UHF;
        }

        nb::Poll<void> include_route_information() {
            if (task_.has_value()) {
                return nb::pending;
            }

            auto task = IncludeRouteInformationTask{};
            task_.emplace(etl::move(task));
            return nb::ready();
        }

        inline nb::Poll<util::Tuple<nb::Future<DataWriter>, nb::Future<bool>>>
        transmit(ModemId dest, uint8_t length) {
            if (task_.has_value()) {
                return nb::pending;
            }

            auto [f_result, p_result] = nb::make_future_promise_pair<bool>();
            auto [f_body, p_body] = nb::make_future_promise_pair<DataWriter>();
            auto task = DataTransmissionTask{dest, length, etl::move(p_body), etl::move(p_result)};
            task_.emplace(etl::move(task));
            return util::Tuple{etl::move(f_body), etl::move(f_result)};
        }

        inline nb::Poll<nb::Future<SerialNumber>> get_serial_number() {
            if (task_.has_value()) {
                return nb::pending;
            }

            auto [f, p] = nb::make_future_promise_pair<SerialNumber>();
            auto task = GetSerialNumberTask{etl::move(p)};
            task_.emplace(etl::move(task));
            return etl::move(f);
        }

        inline nb::Poll<nb::Future<void>> set_equipment_id(ModemId equipment_id) {
            if (task_.has_value()) {
                return nb::pending;
            }

            auto [f, p] = nb::make_future_promise_pair<void>();
            auto task = SetEquipmentIdTask{equipment_id, etl::move(p)};
            task_.emplace(etl::move(task));
            return nb::ready(etl::move(f));
        }

        nb::Poll<FrameReception> execute(util::Time &time, util::Rand &rand) {
            if (!task_.has_value()) {
                if (stream_.readable_count() == 0) {
                    return nb::pending;
                }

                // 構造化束縛するとコンパイラがセグフォる
                auto pair = nb::make_future_promise_pair<DataReader>();
                auto body_future = etl::move(pair.first);
                auto body_promise = etl::move(pair.second);

                auto [source_future, source_promise] = nb::make_future_promise_pair<Address>();
                auto task = DataReceivingTask{etl::move(body_promise), etl::move(source_promise)};
                task.poll(stream_);
                task_.emplace(etl::move(task));
                return nb::ready(FrameReception{
                    .body = etl::move(body_future),
                    .source = etl::move(source_future),
                });
            }

            auto &task = task_.value();
            POLL_UNWRAP_OR_RETURN(etl::visit(
                util::Visitor{
                    [&](DataTransmissionTask &task) { return task.poll(stream_, time, rand); },
                    [&](auto &task) { return task.poll(stream_); }},
                task
            ));
            task_ = etl::nullopt;
            return nb::pending;
        }
    };
} // namespace net::link::uhf
