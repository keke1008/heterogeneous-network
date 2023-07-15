#include "./response.h"

namespace media::uhf::modem {
    etl::optional<ResponseName>
    deserialize_response_name(const collection::TinyBuffer<uint8_t, 2> &bytes) {
        uint8_t c1 = bytes.get<0>();
        uint8_t c2 = bytes.get<1>();
        if (c1 == 'C' && c2 == 'S') {
            return {ResponseName::CarrierSense};
        }
        if (c1 == 'S' && c2 == 'N') {
            return {ResponseName::SerialNumber};
        }
        if (c1 == 'D' && c2 == 'T') {
            return {ResponseName::DataTransmission};
        }
        if (c1 == 'E' && c2 == 'I') {
            return {ResponseName::EquipmentId};
        }
        if (c1 == 'D' && c2 == 'I') {
            return {ResponseName::DestinationId};
        }
        if (c1 == 'D' && c2 == 'R') {
            return {ResponseName::DataReceived};
        }
        if (c1 == 'E' && c2 == 'R') {
            return {ResponseName::Error};
        }
        if (c1 == 'I' && c2 == 'R') {
            return {ResponseName::InformationResponse};
        }
        return etl::nullopt;
    } // namespace media::uhf::modem
} // namespace media::uhf::modem
