#ifndef COMMANDCENTER_H
#define COMMANDCENTER_H

#include "deviceworkschedule.h"

#include <cstdint>
#include <map>
#include <memory>
#include <vector>

class DeviceMonitoringServer;
class MessageMeterage;
class Message;

/*!
 * \brief Статистика СКО физических параметров от плана
 */
struct DeviationStats
{
    Phase phase;                 ///< Этап плана
    uint64_t firstTimestamp = 0; ///< Временная метка первого измерения для данного этапа плана
    double deviation = 0;        ///< СКО физического параметра от плана
};

/*!
 * \brief Класс командного центра для управления и ведения статистики по физическим параметрам
 */
class CommandCenter final
{
public:
    /*!
     * \brief Установить план работы устройства
     */
    void setSchedule(const DeviceWorkSchedule& schedule);
    /*!
     * \brief Обработать сообщение с измерением
     * \param deviceId - идентификатор устройства
     * \param meterage - сообшение с измерением
     * \return сообшение с ответом
     */
    std::unique_ptr<Message> processMeterage(uint64_t deviceId, MessageMeterage meterage);
    /*!
     * \brief Статистика СКО физических параметров от плана для устройства с идентификатором \a deviceId
     */
    std::vector<DeviationStats> deviationStats(uint64_t deviceId) const;
    /*!
     * \brief Удалить всю известную информацию об устройстве с идентификатором \a deviceId
     */
    void forgetDevice(uint64_t deviceId);

private:
    struct ScheduleInfo
    {
        std::vector<Phase> phases;
        size_t currentPhaseIndex = 0;
    };
    struct StatsInfo
    {
        std::vector<double> squareDiffs;
        std::vector<DeviationStats> deviationStats;
    };
    std::map<uint64_t, ScheduleInfo> m_scheduleInfo;
    std::map<uint64_t, StatsInfo> m_statsInfo;
    std::map<uint64_t, uint64_t> m_lastTimeStamp;
};

#endif // COMMANDCENTER_H