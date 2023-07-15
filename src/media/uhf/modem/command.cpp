#include "./command.h"

namespace media::uhf::modem {
    collection::TinyBuffer<uint8_t, 2> serialize_command_name(CommandName command_name) {
        switch (command_name) {
        case CommandName::CarrierSense:
            return {'C', 'S'};
        case CommandName::SerialNumber:
            return {'S', 'N'};
        case CommandName::DataTransmission:
            return {'D', 'T'};
        case CommandName::SetEquipmentId:
            return {'E', 'I'};
        case CommandName::SetDestinationId:
            return {'D', 'I'};
        }
    }
} // namespace media::uhf::modem
