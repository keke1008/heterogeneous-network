#pragma once

#include <etl/variant.h>

#include "./task/data_receiving.h"
#include "./task/data_transmission.h"
#include "./task/get_serial_number.h"
#include "./task/include_route_information.h"
#include "./task/set_equipment_id.h"

namespace net::link::uhf {
    using Task = etl::variant<
        DataReceivingTask,
        DataTransmissionTask,
        GetSerialNumberTask,
        SetEquipmentIdTask,
        IncludeRouteInformationTask>;
} // namespace net::link::uhf
