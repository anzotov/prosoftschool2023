#ifndef MESSAGE_H
#define MESSAGE_H

#include <functional>
#include <memory>
#include <ostream>

/*!
 * \brief Абстрактный базовый класс для сообщений
 */
class Message
{
public:
    virtual ~Message() = default;

    /*!
     * \brief Десериализовать сообщение из входного потока \a is
     */
    static std::unique_ptr<Message> deserialize(std::istream& is);

    /*!
     * \brief Сериализовать сообщение в поток \a os
     */
    virtual void serialize(std::ostream& os) const = 0;

    /*!
    * \brief Вывод сообщения в виде строки для отладки
    */
    virtual void print(std::ostream& os) const = 0;
    virtual bool operator==(const Message& other) const = 0;
    bool operator!=(const Message& other) const
    {
        return !(*this == other);
    }
};

inline std::ostream& operator<<(std::ostream& os, const Message& message)
{
    message.print(os);
    return os;
}

#endif // MESSAGE_H