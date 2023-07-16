#pragma once

#include "./executor/data_receiving.h"
#include "./executor/data_transmission.h"
#include "./executor/get_serial_number.h"
#include "./executor/set_equipment_id.h"
#include <nb/serial.h>

namespace media::uhf::executor {
    template <typename ReaderWriter>
    using Executor = etl::variant<
        etl::monostate,
        GetSerialNumberExecutor<ReaderWriter>,
        SetEquipmentIdExecutor<ReaderWriter>,
        DataTransmissionExecutor<ReaderWriter>,
        DataReceivingExecutor<ReaderWriter>>;

    template <typename ReaderWriter>
    using Task = etl::variant<
        GetSerialNumberTask<ReaderWriter>,
        SetEquipmentIdTask<ReaderWriter>,
        DataTransmissionTask<ReaderWriter>,
        DataReceivingTask<ReaderWriter>>;
} // namespace media::uhf::executor
