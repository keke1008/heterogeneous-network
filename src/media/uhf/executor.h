#pragma once

#include "./modem.h"
#include <nb/lock.h>
#include <nb/result.h>
#include <nb/serial.h>
#include <nb/stream.h>
#include <util/visitor.h>

namespace media::uhf::executor {
    template <typename T, typename Query>
    class QueryExecutor {
        nb::lock::Guard<nb::serial::Serial<T>> serial_;
        Query query_;

      public:
        template <typename... Args>
        QueryExecutor(nb::lock::Guard<nb::serial::Serial<T>> serial, Args &&...args)
            : serial_(std::move(serial)),
              query_(std::move(args)...) {}

        decltype(etl::declval<Query>().response()) execute() {
            nb::stream::pipe(query_, *serial_);
            nb::stream::pipe(*serial_, query_);
            return query_.response();
        }
    };

    template <typename T>
    class GetSerialNumberExecutor : public QueryExecutor<T, modem::GetSerialNumberQuery> {
        using QueryExecutor<T, modem::GetSerialNumberQuery>::QueryExecutor;
    };

    template <typename T>
    class SetEquipmentIdExecutor : QueryExecutor<T, modem::SetEquipmentIdQuery> {
        using QueryExecutor<T, modem::SetEquipmentIdQuery>::QueryExecutor;
    };

    template <typename T>
    class SetDestinationIdExecutor : QueryExecutor<T, modem::SetDestinationIdQuery> {
        using QueryExecutor<T, modem::SetDestinationIdQuery>::QueryExecutor;
    };

    template <typename T>
    class CarrierSenseExecutor : public QueryExecutor<T, modem::CarrierSenseQuery> {
        using QueryExecutor<T, modem::CarrierSenseQuery>::QueryExecutor;
    };

    template <typename T>
    class PacketReceivingExecutor : public QueryExecutor<T, modem::PacketReceivingQuery> {
        using QueryExecutor<T, modem::PacketReceivingQuery>::QueryExecutor;
    };

    template <typename T>
    class PacketTransmissionExecutor {
        nb::lock::Guard<nb::serial::Serial<T>> serial_;
        uint8_t length_;
        nb::stream::HeapStreamReader<uint8_t> data_;
        etl::variant<
            modem::SetDestinationIdQuery,
            modem::CarrierSenseQuery,
            modem::PacketTransmissionQuery>
            queries_;

      public:
        PacketTransmissionExecutor(
            nb::lock::Guard<nb::serial::Serial<T>> serial,
            modem::ModemId &destination_id,
            uint8_t length,
            nb::stream::HeapStreamReader<uint8_t> &&data
        )
            : serial_{etl::move(serial)},
              length_{length},
              data_{etl::move(data)},
              queries_{modem::SetDestinationIdQuery{destination_id}} {}

        inline modem::ModemResult<nb::Empty> execute() {
            etl::visit(
                [&](auto &query) {
                    nb::stream::pipe(query, *serial_);
                    nb::stream::pipe(*serial_, query);
                },
                queries_
            );
            return etl::visit(
                Visitor{
                    [&](modem::SetDestinationIdQuery &query) {
                        return query.get_result().bind_ok([&](nb::Ok<nb::Empty>) {
                            queries_ = modem::CarrierSenseQuery{};
                            return nb::Pending{};
                        });
                    },
                    [&](modem::CarrierSenseQuery &query) {
                        return query.get_result().visit(Visitor{
                            [&](nb::Ok<nb::Empty> &) {
                                queries_ =
                                    modem::PacketTransmissionQuery{length_, etl::move(data_)};
                                return nb::Pending{};
                            },
                            [&](nb::Err<modem::ModemError> &err) {
                                if (err->is_carrier_sense()) {
                                    queries_ = modem::CarrierSenseQuery{};
                                    return nb::Pending{};
                                } else {
                                    return err;
                                }
                            },
                            [&](nb::Pending) { return nb::Pending{}; },
                        });
                    },
                    [&](modem::PacketTransmissionQuery &query) { return query.get_result(); },
                },
                queries_
            );
        }
    };
} // namespace media::uhf::executor
