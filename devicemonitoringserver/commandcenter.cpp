#include "commandcenter.h"

#include "messagecommand.h"
#include "messageerror.h"
#include "messagemeterage.h"

#include <algorithm>
#include <cmath>
#include <numeric>

void CommandCenter::setSchedule(const DeviceWorkSchedule& schedule)
{
    auto& scheduleInfo = m_scheduleInfo[schedule.deviceId];
    scheduleInfo = {};
    scheduleInfo.phases = schedule.schedule;
    std::sort(scheduleInfo.phases.begin(),
              scheduleInfo.phases.end(),
              [](const Phase& phaseA, const Phase& phaseB) { return phaseA.timeStamp < phaseB.timeStamp; });
}

std::unique_ptr<Message> CommandCenter::processMeterage(uint64_t deviceId, MessageMeterage meterage)
{
    auto currentTimeStamp = meterage.timeStamp();

    auto lastTimeStampIt = m_lastTimeStamp.find(deviceId);
    if (lastTimeStampIt != m_lastTimeStamp.end() && lastTimeStampIt->second >= currentTimeStamp)
        return std::unique_ptr<Message>(new MessageError(MessageError::ErrorType::Obsolete));
    m_lastTimeStamp[deviceId] = currentTimeStamp;

    auto& scheduleInfo = m_scheduleInfo[deviceId];
    auto& statsInfo = m_statsInfo[deviceId];
    if (scheduleInfo.phases.empty())
        return std::unique_ptr<Message>(new MessageError(MessageError::ErrorType::NoSchedule));

    while (scheduleInfo.currentPhaseIndex + 1 < scheduleInfo.phases.size()
           && scheduleInfo.phases[scheduleInfo.currentPhaseIndex + 1].timeStamp <= currentTimeStamp)
    {
        ++scheduleInfo.currentPhaseIndex;
    }

    auto currentPhase = scheduleInfo.phases[scheduleInfo.currentPhaseIndex];

    if (currentPhase.timeStamp > currentTimeStamp)
        return std::unique_ptr<Message>(new MessageError(MessageError::ErrorType::NoTimestamp));

    int command = currentPhase.value - meterage.meterage();

    auto lastDeviationStatIt = statsInfo.deviationStats.rbegin();
    if (lastDeviationStatIt == statsInfo.deviationStats.rend()
        || lastDeviationStatIt->phase.timeStamp != currentPhase.timeStamp
        || lastDeviationStatIt->phase.value != currentPhase.value)
    {
        statsInfo.squareDiffs.clear();
        statsInfo.deviationStats.push_back({ currentPhase, currentTimeStamp, 0.0 });
        lastDeviationStatIt = statsInfo.deviationStats.rbegin();
    }
    statsInfo.squareDiffs.push_back(command * command);
    double squareDiffsSum = std::accumulate(statsInfo.squareDiffs.cbegin(), statsInfo.squareDiffs.cend(), 0.0);
    double deviation = std::sqrt(squareDiffsSum / statsInfo.squareDiffs.size());
    lastDeviationStatIt->deviation = deviation;

    return std::unique_ptr<Message>(new MessageCommand(command));
}

std::vector<DeviationStats> CommandCenter::deviationStats(uint64_t deviceId) const
{
    auto statsInfoIt = m_statsInfo.find(deviceId);
    if (statsInfoIt != m_statsInfo.cend())
        return statsInfoIt->second.deviationStats;
    else
        return {};
}

void CommandCenter::forgetDevice(uint64_t deviceId)
{
    m_scheduleInfo.erase(deviceId);
    m_statsInfo.erase(deviceId);
    m_lastTimeStamp.erase(deviceId);
}
