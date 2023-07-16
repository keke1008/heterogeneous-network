#include "./response.h"

namespace media::uhf::modem {
    etl::optional<ResponseType>
    deserialize_response_name(const collection::TinyBuffer<uint8_t, 2> &bytes) {
        uint8_t c1 = bytes.get<0>();
        uint8_t c2 = bytes.get<1>();
        if (c1 == 'C' && c2 == 'S') {
            return {ResponseType::CarrierSense};
        }
        if (c1 == 'S' && c2 == 'N') {
            return {ResponseType::GetSerialNumber};
        }
        if (c1 == 'D' && c2 == 'T') {
            return {ResponseType::DataTransmission};
        }
        if (c1 == 'E' && c2 == 'I') {
            return {ResponseType::SetEquipmentId};
        }
        if (c1 == 'D' && c2 == 'I') {
            return {ResponseType::SetDestinationId};
        }
        if (c1 == 'D' && c2 == 'R') {
            return {ResponseType::DataReceiving};
        }
        if (c1 == 'E' && c2 == 'R') {
            return {ResponseType::Error};
        }
        if (c1 == 'I' && c2 == 'R') {
            return {ResponseType::InformationResponse};
        }
        return etl::nullopt;
    } // namespace media::uhf::modem
} // namespace media::uhf::modem
