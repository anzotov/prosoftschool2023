#ifndef ROT3ENCODEREXECUTOR_H
#define ROT3ENCODEREXECUTOR_H

#include "baseencoderexecutor.h"

class Rot3EncoderExecutor final : public BaseEncoderExecutor
{
    std::string encode(const std::string& message) const override final;
    std::string decode(const std::string& message) const override final;
    std::string name() const override final { return "ROT3"; }
};

#endif // ROT3ENCODEREXECUTOR_H