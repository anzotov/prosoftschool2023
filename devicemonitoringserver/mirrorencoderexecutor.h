#ifndef MIRRORENCODEREXECUTOR_H
#define MIRRORENCODEREXECUTOR_H

#include "baseencoderexecutor.h"

class MirrorEncoderExecutor final : public BaseEncoderExecutor
{
    std::string encode(const std::string& message) const override final;
    std::string decode(const std::string& message) const override final;
    std::string name() const override final { return "Mirror"; }
};

#endif // MIRRORENCODEREXECUTOR_H