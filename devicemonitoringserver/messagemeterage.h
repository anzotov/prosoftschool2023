#ifndef MESSAGEMETERAGE_H
#define MESSAGEMETERAGE_H

#include "message.h"

#include <cstdint>
#include <memory>

/*!
 * \brief Класс сообщения с измерением физического параметра
 */
class MessageMeterage final : public Message
{
public:
    /*!
     * \brief Конструктор.
     * \param timeStamp - временная метка измерения
     * \param meterage - величина измерения
     */
    MessageMeterage(uint64_t timeStamp, uint8_t meterage) :
        m_timeStamp(timeStamp), m_meterage(meterage) {}

    /*!
     * \brief Обозначение типа сообщения для целей сериализации
     */
    static constexpr char signature() { return 'm'; }

    /*!
     * \brief Временная метка измерения
     */
    uint64_t timeStamp() const { return m_timeStamp; }
    /*!
     * \brief Величина измерения
     */
    uint8_t meterage() const { return m_meterage; }

    /*!
     * \brief Сериализовать сообщение в поток \a os
     */
    virtual void serialize(std::ostream& os) const override final;
    /*!
     * \brief Десериализовать сообщение из потока \a is
     */
    static std::unique_ptr<Message> deserialize(std::istream& is);

    bool operator==(const Message& other) const
    {
        const MessageMeterage* o = dynamic_cast<const MessageMeterage*>(&other);
        return o && timeStamp() == o->timeStamp() && meterage() == o->meterage();
    }

    void print(std::ostream& os) const override final
    {
        os << "MessageMeterage (timeStamp=" << timeStamp() << ", meterage=" << static_cast<int>(meterage()) << ")";
    }

private:
    const uint64_t m_timeStamp;
    const uint8_t m_meterage;
};

#endif // MESSAGEMETERAGE_H