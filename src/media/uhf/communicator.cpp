#include "./communicator.h"
#include "collection/tiny_buffer.h"

#include <util/progmem.h>
#include <util/tuple.h>

namespace media::uhf {
    // clang-format off
    const PROGMEM util::Tuple<uint8_t, uint8_t, ResponseName> response_names[] = {
        {'C', 'S', ResponseName::CarrierSense},
        {'S', 'N', ResponseName::GetSerialNumber},
        {'E', 'I', ResponseName::SetEquipmentId},
        {'D', 'T', ResponseName::DataTransmission},
        {'D', 'R', ResponseName::DataReceiving},
        {'E', 'R', ResponseName::Error},
        {'I', 'R', ResponseName::Information},
    }; // clang-format on

    ResponseName parse_response_name(const collection::TinyBuffer<uint8_t, 2> &name) {
        char c1 = name.get<0>(), c2 = name.get<1>();
        for (auto &response_name : response_names) {
            if (util::get<0>(response_name) == c1 && util::get<1>(response_name) == c2) {
                return util::get<2>(response_name);
            }
        }
        return ResponseName::Other;
    }
} // namespace media::uhf
