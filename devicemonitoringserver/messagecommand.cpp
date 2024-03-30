#include "messagecommand.h"

#include "bigendian.h"

#include <functional>
#include <optional>

void MessageCommand::serialize(std::ostream& os) const
{
    os << signature();
    toBigEndian(os, command());
}

std::unique_ptr<Message> MessageCommand::deserialize(std::istream& is)
{
    int command;
    command = fromBigEndian<int8_t>(is);
    if (is.fail())
        return {};
    return std::unique_ptr<Message>(new MessageCommand(command));
}
