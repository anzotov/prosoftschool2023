#ifndef DUMMYENCODEREXECUTOR_H
#define DUMMYENCODEREXECUTOR_H

#include "baseencoderexecutor.h"

class DummyEncoderExecutor final : public BaseEncoderExecutor
{
    std::string encode(const std::string& message) const override final { return message; }
    std::string decode(const std::string& message) const override final { return message; }
    std::string name() const override final { return "Dummy"; }
};

#endif // DUMMYENCODEREXECUTOR_H