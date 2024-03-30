#ifndef MULTIPLY41ENCODEREXECUTOR_H
#define MULTIPLY41ENCODEREXECUTOR_H

#include "baseencoderexecutor.h"

class Multiply41EncoderExecutor final : public BaseEncoderExecutor
{
    std::string encode(const std::string& message) const override final;
    std::string decode(const std::string& message) const override final;
    std::string name() const override final { return "Multiply41"; }
};

#endif // MULTIPLY41ENCODEREXECUTOR_H