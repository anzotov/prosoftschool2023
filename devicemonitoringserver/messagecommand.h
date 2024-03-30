#ifndef MESSAGECOMMAND_H
#define MESSAGECOMMAND_H

#include "message.h"

#include <cstdint>
#include <memory>

/*!
 * \brief Класс сообщения с командой для корректировки физического параметра
 */
class MessageCommand final : public Message
{
public:
    /*!
     * \brief Конструктор.
     * \param command - величина для коррекции физического параметра
     */
    MessageCommand(int8_t command) :
        m_command(command) {}

    /*!
     * \brief Обозначение типа сообщения для целей сериализации
     */
    static constexpr char signature() { return 'c'; }

    /*!
     * \brief Величина для коррекции физического параметра
     */
    int8_t command() const { return m_command; }

    /*!
     * \brief Сериализовать сообщение в поток \a os
     */
    virtual void serialize(std::ostream& os) const override final;
    /*!
     * \brief Десериализовать сообщение из потока \a is
     */
    static std::unique_ptr<Message> deserialize(std::istream& is);

    /*!
    * \brief Вывод сообщения в виде строки для отладки
    */
    bool operator==(const Message& other) const
    {
        const MessageCommand* o = dynamic_cast<const MessageCommand*>(&other);
        return o && command() == o->command();
    }

    void print(std::ostream& os) const override final
    {
        os << "MessageCommand (command=" << command() << ")";
    }

private:
    const int8_t m_command;
};

#endif // MESSAGECOMMAND_H