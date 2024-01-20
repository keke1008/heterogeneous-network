#pragma once

#include "../common.h"
#include "./fixed.h"
#include "./interruption.h"

namespace media::uhf {
    class AsyncCommandSerializer {
        nb::ser::AsyncStaticSpanSerializer prefix_{"@EI"};
        AsyncModemIdSerializer equipment_id_;
        nb::ser::AsyncStaticSpanSerializer suffix_{"\r\n"};

      public:
        explicit AsyncCommandSerializer(ModemId equipment_id) : equipment_id_{equipment_id} {}

        template <nb::AsyncWritable W>
        inline nb::Poll<nb::ser::SerializeResult> serialize(W &writable) {
            POLL_UNWRAP_OR_RETURN(prefix_.serialize(writable));
            POLL_UNWRAP_OR_RETURN(equipment_id_.serialize(writable));
            return suffix_.serialize(writable);
        }

        uint8_t serialized_length() const {
            return prefix_.serialized_length() + equipment_id_.serialized_length() +
                suffix_.serialized_length();
        }
    };

    template <nb::AsyncReadableWritable RW>
    class SetEquipmentIdTask {
        using Task = FixedTask<RW, AsyncCommandSerializer, UhfResponseType::EI, 2>;

        ModemId equipment_id_;
        Task task_;

      public:
        inline SetEquipmentIdTask(ModemId equipment_id)
            : equipment_id_{equipment_id},
              task_{equipment_id} {}

        inline nb::Poll<void> execute(nb::Lock<etl::reference_wrapper<RW>> &rw) {
            POLL_UNWRAP_OR_RETURN(task_.execute(rw));
            return nb::ready();
        }

        inline UhfHandleResponseResult handle_response(UhfResponse<RW> &&res) {
            return task_.handle_response(etl::move(res));
        }

        inline TaskInterruptionResult interrupt() {
            task_ = Task{equipment_id_};
            return TaskInterruptionResult::Interrupted;
        }
    };
} // namespace media::uhf
