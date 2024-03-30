#include "messageerror.h"

#include <optional>

void MessageError::serialize(std::ostream& os) const
{
    os << signature();
    switch (errorType())
    {
    case MessageError::ErrorType::NoSchedule:
        os.put('s');
        break;
    case MessageError::ErrorType::NoTimestamp:
        os.put('t');
        break;
    case MessageError::ErrorType::Obsolete:
        os.put('o');
        break;
    }
}

std::unique_ptr<Message> MessageError::deserialize(std::istream& is)
{
    char ch = is.get();
    if (is.fail())
        return {};
    switch (ch)
    {
    case 's':
        return std::unique_ptr<Message>(new MessageError(MessageError::ErrorType::NoSchedule));
    case 't':
        return std::unique_ptr<Message>(new MessageError(MessageError::ErrorType::NoTimestamp));
    case 'o':
        return std::unique_ptr<Message>(new MessageError(MessageError::ErrorType::Obsolete));
    }
    return {};
}
