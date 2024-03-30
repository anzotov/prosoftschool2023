#ifndef MESSAGESERIALIZER_H
#define MESSAGESERIALIZER_H

#include <memory>
#include <string>

class Message;

/*!
 * \brief Класс для сериализации сообщений
 */
class MessageSerializer
{
public:
    MessageSerializer() = delete;
    /*!
     * \brief Сериализовать сообщение \a message
     */
    static std::string serialize(const Message& message);
    /*!
     * \brief Десериализовать сообщение из строки \a string
     */
    static std::unique_ptr<Message> deserialize(const std::string& string);
};

#endif // MESSAGESERIALIZER_H