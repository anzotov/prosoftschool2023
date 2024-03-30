#include "message.h"

#include <messagecommand.h>
#include <messageerror.h>
#include <messagemeterage.h>

template <class T, class... Ts>
static std::unique_ptr<Message> deserialize_typed(std::istream& is)
{
    if (is.peek() == T::signature())
    {
        is.get();
        return T::deserialize(is);
    }
    else
    {
        if constexpr (sizeof...(Ts))
            return deserialize_typed<Ts...>(is);
    }
    return {};
}

std::unique_ptr<Message> Message::deserialize(std::istream& is)
{
    return deserialize_typed<MessageError, MessageMeterage, MessageCommand>(is);
}
