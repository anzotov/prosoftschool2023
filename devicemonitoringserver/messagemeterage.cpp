#include "messagemeterage.h"

#include "bigendian.h"

void MessageMeterage::serialize(std::ostream& os) const
{
    os << signature();
    toBigEndian(os, timeStamp());
    toBigEndian(os, meterage());
}

std::unique_ptr<Message> MessageMeterage::deserialize(std::istream& is)
{
    uint64_t timeStamp;
    uint8_t meterage;
    timeStamp = fromBigEndian<uint64_t>(is);
    meterage = fromBigEndian<uint8_t>(is);
    if (is.fail())
        return {};
    return std::unique_ptr<Message>(new MessageMeterage(timeStamp, meterage));
}
