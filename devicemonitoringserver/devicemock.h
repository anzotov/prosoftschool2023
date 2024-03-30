#ifndef DEVICE_H
#define DEVICE_H

#include "common.h"
#include "message.h"
#include "messageencoder.h"
#include "messageserializer.h"

#include <cstdint>
#include <memory>
#include <string>
#include <vector>

class AbstractClientConnection;

/*!
 * \brief Класс, эмитирующий устройство.
 */
class DeviceMock
{
    NON_COPYABLE(DeviceMock)
public:
    /*!
     * \brief Конструктор.
     * \param clientConnection - владеющий указатель на объект класса клиента
     */
    DeviceMock(AbstractClientConnection* clientConnection);
    ~DeviceMock();

    /*!
     * \brief Назначить устройству идентификатор.
     * \param deviceId - идентификатор устройства
     * \return false в случае ошибки
     */
    bool bind(uint64_t deviceId);
    /*!
     * \brief Подключить устройство к серверу.
     * \param serverId - идентификатор сервера
     * \return false в случае ошибки
     */
    bool connectToServer(uint64_t serverId);
    /*!
     * \brief Установить тестовый список измерений устройства.
     * \param measurements - список измерений
     */
    void setMeterages(std::vector<uint8_t> meterages);
    /*!
     * \brief Начать отправку измерений.
     */
    void startMeterageSending();
    /*!
     * \brief Список полученных сообщений.
     */
    const std::vector<std::unique_ptr<Message>>& messages() const;
    /*!
     * \brief Ссылка на объект MessageEncoder для управления параметрами шифрования.
     */
    MessageEncoder& messageEncoder();

private:
    /*!
     * \brief Отправить следующее измерение.
     */
    void sendNextMeterage();
    /*!
     * \brief Обработчик установления соединения.
     */
    void onConnected();
    /*!
     * \brief Обработчик разрыва соединения.
     */
    void onDisconnected();
    /*!
     * \brief Отправить сообщение.
     */
    void sendMessage(const std::string& message) const;
    /*!
     * \brief Обработчик получения нового сообщения от сервера.
     * \param message - сообщение
     */
    void onMessageReceived(const std::string& message);

private:
    AbstractClientConnection* m_clientConnection = nullptr;
    std::vector<uint8_t> m_meterages;
    uint64_t m_timeStamp = 0;
    std::vector<std::unique_ptr<Message>> m_messages;
    MessageEncoder m_encoder;
};

#endif // DEVICE_H
