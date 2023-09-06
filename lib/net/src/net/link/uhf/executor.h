#pragma once

#include <etl/circular_buffer.h>

template <typename ReaderWriter>
class UHFExecutor {
    etl::circular_buffer<TransmissionTask<ReaderWriter>, 4> transmission_tasks_;
    etl::circular_buffer<ReceivingTask<ReaderWriter>, 4> reception_tasks_;

  public:
    void execute() {}
};
