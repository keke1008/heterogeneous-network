#pragma once

#include "./generic.h"
#include <nb/future.h>
#include <nb/lock.h>
#include <nb/serde.h>

namespace media::wifi {
    template <nb::AsyncReadable R, nb::AsyncWritable W>
    struct CloseUdpServerControl
        : public GenericEmptyResponseControl<R, W, nb::ser::AsyncStaticSpanSerializer> {
        explicit CloseUdpServerControl(nb::Promise<bool> &&promise, memory::Static<W> &writable)
            : GenericEmptyResponseControl<R, W, nb::ser::AsyncStaticSpanSerializer>{
                  writable, etl::move(promise), WifiResponseMessage::Ok, "AT+CIPCLOSE\r\n"
              } {}
    };
} // namespace media::wifi
