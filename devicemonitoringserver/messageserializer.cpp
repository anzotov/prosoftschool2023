#include "messageserializer.h"
#include "message.h"
#include "messagecommand.h"
#include "messageerror.h"
#include "messagemeterage.h"

#include <sstream>

std::string MessageSerializer::serialize(const Message& message)
{
    std::ostringstream os(std::ios_base::binary);
    message.serialize(os);
    return os.str();
}

std::unique_ptr<Message> MessageSerializer::deserialize(const std::string& string)
{
    std::istringstream is(string, std::ios_base::binary);
    return Message::deserialize(is);
}