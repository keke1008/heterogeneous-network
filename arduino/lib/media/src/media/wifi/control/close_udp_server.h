#pragma once

#include "./generic.h"
#include <nb/future.h>
#include <nb/lock.h>
#include <nb/serde.h>

namespace media::wifi {
    template <nb::AsyncReadable R, nb::AsyncWritable W>
    struct CloseUdpServerControl
        : public GenericEmptyResponseControl<R, W, nb::ser::AsyncFlashStringSerializer> {
        explicit CloseUdpServerControl(nb::Promise<bool> &&promise, memory::Static<W> &writable)
            : GenericEmptyResponseControl<R, W, nb::ser::AsyncFlashStringSerializer>{
                  writable,
                  etl::move(promise),
                  WifiResponseMessage::Ok,
                  FLASH_STRING("AT+CIPCLOSE\r\n"),
              } {}
    };
} // namespace media::wifi
