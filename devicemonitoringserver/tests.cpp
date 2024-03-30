#include "tests.h"
#include "commandcenter.h"
#include "devicemock.h"
#include "devicemonitoringserver.h"
#include "deviceworkschedule.h"
#include "dummyencoderexecutor.h"
#include "message.h"
#include "messagecommand.h"
#include "messageencoder.h"
#include "messageerror.h"
#include "messagemeterage.h"
#include "messageserializer.h"
#include "test_runner.h"
#include <servermock/clientconnectionmock.h>
#include <servermock/connectionservermock.h>
#include <servermock/taskqueue.h>

#include <limits>

#define COMPARE_VECTORS_OF_SMART_PTRS(a, b) \
    ASSERT_EQUAL(a.size(), b.size());       \
    for (size_t i = 0; i < a.size(); ++i)   \
    {                                       \
        ASSERT(a[i].get());                 \
        ASSERT(b[i].get());                 \
        ASSERT_EQUAL(*a[i], *b[i]);         \
    }

struct MonitoringServerTest
{
    MonitoringServerTest(uint64_t serverId) :
        serverId(serverId),
        server(new ConnectionServerMock(taskQueue))
    {
        ASSERT(server.messageEncoder().addExecutor(new DummyEncoderExecutor()));
        ASSERT(server.messageEncoder().selectExecutor("Dummy"));
        ASSERT(server.listen(serverId));
    }
    ~MonitoringServerTest()
    {
        for (auto& value : devices)
        {
            delete value.second;
        }
    }
    void connectDevice(uint64_t deviceId)
    {
        devices.insert({ deviceId, new DeviceMock(new ClientConnectionMock(taskQueue)) });
        ASSERT(devices[deviceId]->messageEncoder().addExecutor(new DummyEncoderExecutor()));
        ASSERT(devices[deviceId]->messageEncoder().selectExecutor("Dummy"));
        ASSERT(devices[deviceId]->bind(deviceId));
        ASSERT(devices[deviceId]->connectToServer(serverId));
        while (taskQueue.processTask())
            ;
    }

    uint64_t serverId;
    TaskQueue taskQueue;
    DeviceMonitoringServer server;
    std::map<uint64_t, DeviceMock*> devices;
};

void monitoringServerTestNoSchedule()
{
    MonitoringServerTest test(11u);
    uint64_t deviceId = 111u;
    test.connectDevice(deviceId);
    std::vector<uint8_t> meterages = { 0u };
    test.devices[deviceId]->setMeterages(meterages);
    test.devices[deviceId]->startMeterageSending();
    while (test.taskQueue.processTask())
        ;

    std::vector<std::shared_ptr<Message>> expected = {
        std::shared_ptr<Message>(new MessageError(MessageError::ErrorType::NoSchedule)),
    };
    auto& messages = test.devices[deviceId]->messages();
    COMPARE_VECTORS_OF_SMART_PTRS(expected, messages);
}

void monitoringServerTestNoTimeStamp()
{
    MonitoringServerTest test(11u);
    uint64_t deviceId = 111u;
    test.connectDevice(deviceId);
    DeviceWorkSchedule schedule { deviceId, { { 1u, 0u } } };
    std::vector<uint8_t> meterages = { 0u };
    test.server.setDeviceWorkSchedule(schedule);
    test.devices[deviceId]->setMeterages(meterages);
    test.devices[deviceId]->startMeterageSending();
    while (test.taskQueue.processTask())
        ;

    std::vector<std::shared_ptr<Message>> expected = {
        std::shared_ptr<Message>(new MessageError(MessageError::ErrorType::NoTimestamp)),
    };
    auto& messages = test.devices[deviceId]->messages();
    COMPARE_VECTORS_OF_SMART_PTRS(expected, messages);
}

void monitoringServerTestObsolete()
{
    MonitoringServerTest test(11u);
    uint64_t deviceId = 111u;
    test.connectDevice(deviceId);
    std::vector<uint8_t> meterages = { 0u };
    test.devices[deviceId]->setMeterages(meterages);
    test.devices[deviceId]->startMeterageSending();
    while (test.taskQueue.processTask())
        ;

    test.devices[deviceId]->setMeterages(meterages);
    test.devices[deviceId]->startMeterageSending();
    while (test.taskQueue.processTask())
        ;

    std::vector<std::shared_ptr<Message>> expected = {
        std::shared_ptr<Message>(new MessageError(MessageError::ErrorType::NoSchedule)),
        std::shared_ptr<Message>(new MessageError(MessageError::ErrorType::Obsolete)),
    };
    auto& messages = test.devices[deviceId]->messages();
    COMPARE_VECTORS_OF_SMART_PTRS(expected, messages);
}

void monitoringServerTestCommand()
{
    MonitoringServerTest test(11u);
    uint64_t deviceId = 111u;
    test.connectDevice(deviceId);
    DeviceWorkSchedule schedule { deviceId, {
                                            { 0u, 0u },
                                            { 1u, 0u },
                                            { 2u, 100u },
                                            } };
    std::vector<uint8_t> meterages = { 0u, 100u, 0u };
    test.server.setDeviceWorkSchedule(schedule);
    test.devices[deviceId]->setMeterages(meterages);
    test.devices[deviceId]->startMeterageSending();
    while (test.taskQueue.processTask())
        ;

    std::vector<std::shared_ptr<Message>> expected = {
        std::shared_ptr<Message>(new MessageCommand(0)),
        std::shared_ptr<Message>(new MessageCommand(-100)),
        std::shared_ptr<Message>(new MessageCommand(100)),
    };
    auto& messages = test.devices[deviceId]->messages();
    COMPARE_VECTORS_OF_SMART_PTRS(expected, messages);
}

void monitoringServerTestTwoDevices()
{
    MonitoringServerTest test(11u);
    uint64_t deviceId1 = 111u, deviceId2 = 654u;
    test.connectDevice(deviceId1);
    test.connectDevice(deviceId2);
    test.server.setDeviceWorkSchedule({ deviceId1, {
                                                   { 0u, 0u },
                                                   { 1u, 0u },
                                                   { 2u, 3u },
                                                   } });
    test.server.setDeviceWorkSchedule({ deviceId2, {
                                                   { 1u, 100u },
                                                   { 2u, 50u },
                                                   { 3u, 0u },
                                                   } });
    test.devices[deviceId1]->setMeterages({ 0u, 1u, 2u });
    test.devices[deviceId2]->setMeterages({ 0u, 0u, 50u, 100u });
    test.devices[deviceId1]->startMeterageSending();
    test.devices[deviceId2]->startMeterageSending();
    while (test.taskQueue.processTask())
        ;

    std::vector<std::shared_ptr<Message>> expected1 = {
        std::shared_ptr<Message>(new MessageCommand(0)),
        std::shared_ptr<Message>(new MessageCommand(-1)),
        std::shared_ptr<Message>(new MessageCommand(1)),
    };
    auto& messages1 = test.devices[deviceId1]->messages();
    COMPARE_VECTORS_OF_SMART_PTRS(expected1, messages1);
    std::vector<std::shared_ptr<Message>> expected2 = {
        std::shared_ptr<Message>(new MessageError(MessageError::ErrorType::NoTimestamp)),
        std::shared_ptr<Message>(new MessageCommand(100)),
        std::shared_ptr<Message>(new MessageCommand(0)),
        std::shared_ptr<Message>(new MessageCommand(-100)),
    };
    auto& messages2 = test.devices[deviceId2]->messages();
    COMPARE_VECTORS_OF_SMART_PTRS(expected2, messages2);
}

void monitoringServerCryptoPositiveTest()
{
    MonitoringServerTest test(11u);
    ASSERT(test.server.messageEncoder().selectExecutor("ROT3"));
    uint64_t deviceId = 111u;
    test.connectDevice(deviceId);
    ASSERT(test.devices[deviceId]->messageEncoder().selectExecutor("ROT3"));
    DeviceWorkSchedule schedule { deviceId, {
                                            { 0u, 0u },
                                            { 1u, 0u },
                                            { 2u, 3u },
                                            } };
    std::vector<uint8_t> meterages = { 0u, 1u, 2u };
    test.server.setDeviceWorkSchedule(schedule);
    test.devices[deviceId]->setMeterages(meterages);
    test.devices[deviceId]->startMeterageSending();
    while (test.taskQueue.processTask())
        ;

    std::vector<std::shared_ptr<Message>> expected = {
        std::shared_ptr<Message>(new MessageCommand(0)),
        std::shared_ptr<Message>(new MessageCommand(-1)),
        std::shared_ptr<Message>(new MessageCommand(1)),
    };
    auto& messages = test.devices[deviceId]->messages();
    COMPARE_VECTORS_OF_SMART_PTRS(expected, messages);
}

void monitoringServerCryptoNegativeTest()
{
    MonitoringServerTest test(11u);
    ASSERT(test.server.messageEncoder().selectExecutor("ROT3"));
    uint64_t deviceId = 111u;
    test.connectDevice(deviceId);
    ASSERT(test.devices[deviceId]->messageEncoder().selectExecutor("Mirror"));
    DeviceWorkSchedule schedule { deviceId, {
                                            { 0u, 0u },
                                            { 1u, 0u },
                                            { 2u, 3u },
                                            } };
    std::vector<uint8_t> meterages = { 0u, 1u, 2u };
    test.server.setDeviceWorkSchedule(schedule);
    test.devices[deviceId]->setMeterages(meterages);
    test.devices[deviceId]->startMeterageSending();
    while (test.taskQueue.processTask())
        ;

    ASSERT(test.devices[deviceId]->messages().empty());
}

void messageSerializationTest()
{
    std::vector<std::shared_ptr<Message>> messages;

    ASSERT_EQUAL(MessageSerializer::deserialize("").get(), nullptr);
    ASSERT_EQUAL(MessageSerializer::deserialize("12345").get(), nullptr);
    ASSERT_EQUAL(MessageSerializer::deserialize("e").get(), nullptr);
    ASSERT_EQUAL(MessageSerializer::deserialize("eA").get(), nullptr);
    ASSERT_EQUAL(MessageSerializer::deserialize("m").get(), nullptr);
    ASSERT_EQUAL(MessageSerializer::deserialize("mA").get(), nullptr);
    ASSERT_EQUAL(MessageSerializer::deserialize("c").get(), nullptr);

    std::vector<std::shared_ptr<Message>> expected = {
        std::shared_ptr<Message>(new MessageCommand(0)),
        std::shared_ptr<Message>(new MessageCommand(1)),
        std::shared_ptr<Message>(new MessageCommand(-1)),
        std::shared_ptr<Message>(new MessageCommand(123)),
        std::shared_ptr<Message>(new MessageCommand(std::numeric_limits<int8_t>::max())),
        std::shared_ptr<Message>(new MessageCommand(std::numeric_limits<int8_t>::min())),
        std::shared_ptr<Message>(new MessageMeterage(12345u, 123u)),
        std::shared_ptr<Message>(new MessageMeterage(std::numeric_limits<uint64_t>::min(), std::numeric_limits<uint8_t>::min())),
        std::shared_ptr<Message>(new MessageMeterage(std::numeric_limits<uint64_t>::max(), std::numeric_limits<uint8_t>::min())),
        std::shared_ptr<Message>(new MessageMeterage(std::numeric_limits<uint64_t>::max(), std::numeric_limits<uint8_t>::max())),
        std::shared_ptr<Message>(new MessageMeterage(std::numeric_limits<uint64_t>::min(), std::numeric_limits<uint8_t>::max())),
        std::shared_ptr<Message>(new MessageError(MessageError::ErrorType::NoSchedule)),
        std::shared_ptr<Message>(new MessageError(MessageError::ErrorType::NoTimestamp)),
        std::shared_ptr<Message>(new MessageError(MessageError::ErrorType::Obsolete)),
    };

    for (auto& ptr : expected)
    {
        messages.push_back(std::move(MessageSerializer::deserialize(MessageSerializer::serialize(*ptr))));
    }

    COMPARE_VECTORS_OF_SMART_PTRS(expected, messages);
}

void messageEncoderEmptyTest()
{
    MessageEncoder encoder;
    ASSERT(encoder.encode("123").empty());
    ASSERT(encoder.decode("123").empty());
}

void messageEncoderDummyTest()
{
    MessageEncoder encoder;
    ASSERT(encoder.addExecutor(new DummyEncoderExecutor()));
    ASSERT(encoder.selectExecutor("Dummy"));
    std::string message;
    const int start = std::numeric_limits<unsigned char>::min(),
              end = std::numeric_limits<unsigned char>::max() + 1;
    const size_t len = end - start;
    for (int i = start; i < end; ++i)
    {
        message += i;
    }
    ASSERT_EQUAL(len, message.length());

    auto encoded = encoder.encode(message);
    ASSERT_EQUAL(len, encoded.length());
    ASSERT_EQUAL(message, encoded);

    auto decoded = encoder.decode(encoded);
    ASSERT_EQUAL(len, decoded.length());

    ASSERT_EQUAL(message, decoded);
}

void messageEncoderNegativeTest()
{
    MessageEncoder encoder;
    ASSERT_EQUAL(false, encoder.addExecutor(nullptr));
    ASSERT_EQUAL(false, encoder.selectExecutor("123"));
    std::string message = "123";
    ASSERT(encoder.encode(message).empty());
    ASSERT(encoder.decode(message).empty());
}

void messageEncoderAddTest()
{
    class TestEncoderExecutor : public BaseEncoderExecutor
    {
    public:
        TestEncoderExecutor(const std::string& message) :
            test_message(message) {}
        std::string encode(const std::string&) const override final { return test_message; }
        std::string decode(const std::string&) const override final { return test_message; }
        std::string name() const override final { return "Test"; }

    private:
        std::string test_message;
    };
    MessageEncoder encoder;
    ASSERT(encoder.addExecutor(new TestEncoderExecutor("123")));
    ASSERT(encoder.selectExecutor("Test"));
    ASSERT(encoder.addExecutor(new DummyEncoderExecutor()));
    ASSERT_EQUAL(std::string("123"), encoder.encode("test"));
    ASSERT_EQUAL(std::string("123"), encoder.decode("test"));
}

void messageEncoderDoubleAddTest()
{
    class TestEncoderExecutor : public BaseEncoderExecutor
    {
    public:
        TestEncoderExecutor(const std::string& message) :
            test_message(message) {}
        std::string encode(const std::string&) const override final { return test_message; }
        std::string decode(const std::string&) const override final { return test_message; }
        std::string name() const override final { return "Test"; }

    private:
        std::string test_message;
    };
    MessageEncoder encoder;
    ASSERT(encoder.addExecutor(new TestEncoderExecutor("123")));
    ASSERT(encoder.selectExecutor("Test"));
    ASSERT_EQUAL(std::string("123"), encoder.encode("test"));
    ASSERT_EQUAL(std::string("123"), encoder.decode("test"));
    ASSERT(encoder.addExecutor(new TestEncoderExecutor("456")));
    ASSERT_EQUAL(std::string("456"), encoder.encode("test"));
    ASSERT_EQUAL(std::string("456"), encoder.decode("test"));
}

void messageEncoderRot3Test()
{
    MessageEncoder encoder;
    ASSERT(encoder.selectExecutor("ROT3"));
    std::string message;
    const int start = std::numeric_limits<unsigned char>::min(),
              end = std::numeric_limits<unsigned char>::max() + 1;
    const size_t len = end - start;
    for (int i = start; i < end; ++i)
    {
        message += i;
    }
    ASSERT_EQUAL(len, message.length());

    auto encoded = encoder.encode(message);
    ASSERT_EQUAL(len, encoded.length());
    for (size_t i = 0; i < len; ++i)
    {
        int enc = message[i] + 3;
        if (enc >= end)
        {
            enc -= end - start;
        }
        ASSERT_EQUAL(static_cast<char>(enc), encoded[i]);
    }

    auto decoded = encoder.decode(encoded);
    ASSERT_EQUAL(len, decoded.length());

    ASSERT_EQUAL(message, decoded);
}

void messageEncoderMirrorTest()
{
    MessageEncoder encoder;
    ASSERT(encoder.selectExecutor("Mirror"));
    std::string message;
    const int start = std::numeric_limits<unsigned char>::min(),
              end = std::numeric_limits<unsigned char>::max() + 1;
    const size_t len = end - start;
    for (int i = start; i < end; ++i)
    {
        message += i;
    }
    ASSERT_EQUAL(len, message.length());

    auto encoded = encoder.encode(message);
    ASSERT_EQUAL(len * 3, encoded.length());
    std::vector<uint8_t> results;
    for (size_t i = 0; i < len; ++i)
    {
        unsigned char ch = message[i];
        if (ch == 0 || ch == 9 || ch == 90 || ch == 91 || ch == 100 || ch == 132 || ch == 159 || ch == 255)
        {
            results.push_back(encoded[3 * i]);
            results.push_back(encoded[3 * i + 1]);
            results.push_back(encoded[3 * i + 2]);
        }
    }
    std::vector<uint8_t> expected = {
        0,
        0,
        0,
        1,
        0,
        9,
        2,
        0,
        9,
        2,
        0,
        19,
        3,
        0,
        1,
        3,
        0,
        231,
        3,
        3,
        183,
        3,
        2,
        40,
    };
    ASSERT_EQUAL(expected, results);

    auto decoded = encoder.decode(encoded);
    ASSERT_EQUAL(len, decoded.length());

    ASSERT_EQUAL(message, decoded);
}

void messageEncoderMultiply41Test()
{
    MessageEncoder encoder;
    ASSERT(encoder.selectExecutor("Multiply41"));
    std::string message;
    const int start = std::numeric_limits<unsigned char>::min(),
              end = std::numeric_limits<unsigned char>::max() + 1;
    const size_t len = end - start;
    for (int i = start; i < end; ++i)
    {
        message += i;
    }
    ASSERT_EQUAL(len, message.length());

    auto encoded = encoder.encode(message);
    ASSERT_EQUAL(len * 2, encoded.length());
    std::vector<uint8_t> results;
    for (size_t i = 0; i < len; ++i)
    {
        unsigned char ch = message[i];
        if (ch == 0 || ch == 1 || ch == 6 || ch == 7 || ch == 255)
        {
            results.push_back(encoded[2 * i]);
            results.push_back(encoded[2 * i + 1]);
        }
    }
    std::vector<uint8_t> expected = {
        0,
        0,
        0,
        41,
        0,
        246,
        1,
        31,
        40,
        215,
    };
    ASSERT_EQUAL(expected, results);

    auto decoded = encoder.decode(encoded);
    ASSERT_EQUAL(len, decoded.length());

    ASSERT_EQUAL(message, decoded);
}

void commandCenterNoScheduleTest()
{
    CommandCenter center;
    uint64_t deviceId = 123u;
    std::vector<std::shared_ptr<Message>> messages;
    MessageMeterage meterage = MessageMeterage(0u, 0u);
    messages.push_back(std::move(center.processMeterage(deviceId, meterage)));
    std::vector<std::shared_ptr<Message>> expected = {
        std::shared_ptr<Message>(new MessageError(MessageError::ErrorType::NoSchedule)),
    };
    COMPARE_VECTORS_OF_SMART_PTRS(expected, messages);
}

void commandCenterNoTimestampTest()
{
    CommandCenter center;
    uint64_t deviceId = 123u;
    std::vector<std::shared_ptr<Message>> messages;
    MessageMeterage meterage = MessageMeterage(0u, 0u);
    center.setSchedule({ deviceId, { { 1u, 0u } } });
    messages.push_back(std::move(center.processMeterage(deviceId, meterage)));
    std::vector<std::shared_ptr<Message>> expected = {
        std::shared_ptr<Message>(new MessageError(MessageError::ErrorType::NoTimestamp)),
    };
    COMPARE_VECTORS_OF_SMART_PTRS(expected, messages);
}

void commandCenterCommandZeroTest()
{
    CommandCenter center;
    uint64_t deviceId = 123u;
    std::vector<std::shared_ptr<Message>> messages;
    MessageMeterage meterage = MessageMeterage(0u, 0u);
    center.setSchedule({ deviceId, { { 0u, 0u } } });
    messages.push_back(std::move(center.processMeterage(deviceId, meterage)));
    std::vector<std::shared_ptr<Message>> expected = {
        std::shared_ptr<Message>(new MessageCommand(0)),
    };
    COMPARE_VECTORS_OF_SMART_PTRS(expected, messages);
}

void commandCenterCommandUpTest()
{
    CommandCenter center;
    uint64_t deviceId = 123u;
    std::vector<std::shared_ptr<Message>> messages;
    MessageMeterage meterage = MessageMeterage(0u, 49u);
    center.setSchedule({ deviceId, { { 0u, 100u } } });
    messages.push_back(std::move(center.processMeterage(deviceId, meterage)));
    std::vector<std::shared_ptr<Message>> expected = {
        std::shared_ptr<Message>(new MessageCommand(51)),
    };
    COMPARE_VECTORS_OF_SMART_PTRS(expected, messages);
}

void commandCenterCommandDownTest()
{
    CommandCenter center;
    uint64_t deviceId = 123u;
    std::vector<std::shared_ptr<Message>> messages;
    MessageMeterage meterage = MessageMeterage(0u, 100u);
    center.setSchedule({ deviceId, { { 0u, 49u } } });
    messages.push_back(std::move(center.processMeterage(deviceId, meterage)));
    std::vector<std::shared_ptr<Message>> expected = {
        std::shared_ptr<Message>(new MessageCommand(-51)),
    };
    COMPARE_VECTORS_OF_SMART_PTRS(expected, messages);
}

void commandCenterObsoleteTest()
{
    CommandCenter center;
    uint64_t deviceId = 123u;
    std::vector<std::shared_ptr<Message>> messages;
    messages.push_back(std::move(center.processMeterage(deviceId, MessageMeterage(0u, 100u))));
    messages.push_back(std::move(center.processMeterage(deviceId, MessageMeterage(0u, 100u))));
    center.setSchedule({ deviceId, { { 2u, 50u } } });
    messages.push_back(std::move(center.processMeterage(deviceId, MessageMeterage(1u, 100u))));
    messages.push_back(std::move(center.processMeterage(deviceId, MessageMeterage(1u, 100u))));
    messages.push_back(std::move(center.processMeterage(deviceId, MessageMeterage(2u, 100u))));
    messages.push_back(std::move(center.processMeterage(deviceId, MessageMeterage(2u, 100u))));
    std::vector<std::shared_ptr<Message>> expected = {
        std::shared_ptr<Message>(new MessageError(MessageError::ErrorType::NoSchedule)),
        std::shared_ptr<Message>(new MessageError(MessageError::ErrorType::Obsolete)),
        std::shared_ptr<Message>(new MessageError(MessageError::ErrorType::NoTimestamp)),
        std::shared_ptr<Message>(new MessageError(MessageError::ErrorType::Obsolete)),
        std::shared_ptr<Message>(new MessageCommand(-50)),
        std::shared_ptr<Message>(new MessageError(MessageError::ErrorType::Obsolete)),
    };
    COMPARE_VECTORS_OF_SMART_PTRS(expected, messages);
}

void commandCenterTwoDevicesTest()
{
    CommandCenter center;
    MessageMeterage meterage = MessageMeterage(0u, 100u);
    uint64_t deviceId1 = 111u, deviceId2 = 222u;
    std::vector<std::shared_ptr<Message>> messages1, messages2;
    center.setSchedule({ deviceId1, { { 0u, 10u } } });
    center.setSchedule({ deviceId2, { { 0u, 50u } } });
    messages1.push_back(std::move(center.processMeterage(deviceId1, meterage)));
    messages1.push_back(std::move(center.processMeterage(deviceId1, meterage)));
    messages2.push_back(std::move(center.processMeterage(deviceId2, meterage)));

    std::vector<std::shared_ptr<Message>> expected1 = {
        std::shared_ptr<Message>(new MessageCommand(-90)),
        std::shared_ptr<Message>(new MessageError(MessageError::ErrorType::Obsolete)),
    };
    COMPARE_VECTORS_OF_SMART_PTRS(expected1, messages1);

    std::vector<std::shared_ptr<Message>> expected2 = {
        std::shared_ptr<Message>(new MessageCommand(-50)),
    };
    COMPARE_VECTORS_OF_SMART_PTRS(expected2, messages2);
}

void commandCenterUnsortedScheduleTest()
{
    CommandCenter center;
    uint64_t deviceId = 123u;
    std::vector<std::shared_ptr<Message>> messages;
    center.setSchedule({ deviceId, { { 3u, 30u }, { 0u, 0u }, { 2u, 20u }, { 1u, 10u } } });
    messages.push_back(std::move(center.processMeterage(deviceId, MessageMeterage(0u, 100u))));
    messages.push_back(std::move(center.processMeterage(deviceId, MessageMeterage(1u, 100u))));
    messages.push_back(std::move(center.processMeterage(deviceId, MessageMeterage(2u, 100u))));
    messages.push_back(std::move(center.processMeterage(deviceId, MessageMeterage(3u, 100u))));
    messages.push_back(std::move(center.processMeterage(deviceId, MessageMeterage(4u, 100u))));
    std::vector<std::shared_ptr<Message>> expected = {
        std::shared_ptr<Message>(new MessageCommand(-100)),
        std::shared_ptr<Message>(new MessageCommand(-90)),
        std::shared_ptr<Message>(new MessageCommand(-80)),
        std::shared_ptr<Message>(new MessageCommand(-70)),
        std::shared_ptr<Message>(new MessageCommand(-70)),
    };
    COMPARE_VECTORS_OF_SMART_PTRS(expected, messages);
}

void commandCenterDublicateScheduleTest()
{
    CommandCenter center;
    uint64_t deviceId = 123u;
    std::vector<std::shared_ptr<Message>> messages;
    center.setSchedule({ deviceId, { { 0u, 0u }, { 0u, 0u }, { 0u, 0u }, { 1u, 10u } } });
    messages.push_back(std::move(center.processMeterage(deviceId, MessageMeterage(0u, 100u))));
    messages.push_back(std::move(center.processMeterage(deviceId, MessageMeterage(1u, 100u))));
    messages.push_back(std::move(center.processMeterage(deviceId, MessageMeterage(2u, 100u))));
    std::vector<std::shared_ptr<Message>> expected = {
        std::shared_ptr<Message>(new MessageCommand(-100)),
        std::shared_ptr<Message>(new MessageCommand(-90)),
        std::shared_ptr<Message>(new MessageCommand(-90)),
    };
    COMPARE_VECTORS_OF_SMART_PTRS(expected, messages);
}

void commandCenterNewScheduleTest()
{
    CommandCenter center;
    uint64_t deviceId = 123u;
    std::vector<std::shared_ptr<Message>> messages;
    center.setSchedule({ deviceId, { { 0u, 100u }, { 1u, 50u } } });
    messages.push_back(std::move(center.processMeterage(deviceId, MessageMeterage(0u, 99u))));
    messages.push_back(std::move(center.processMeterage(deviceId, MessageMeterage(1u, 100u))));
    messages.push_back(std::move(center.processMeterage(deviceId, MessageMeterage(2u, 50u))));
    center.setSchedule({ deviceId, { { 3u, 0u } } });
    messages.push_back(std::move(center.processMeterage(deviceId, MessageMeterage(3u, 50u))));
    center.setSchedule({ deviceId, { { 0u, 100u }, { 1u, 50u }, { 3u, 0u }, { 4u, 10u } } });
    messages.push_back(std::move(center.processMeterage(deviceId, MessageMeterage(4u, 40u))));
    center.setSchedule({ deviceId, { { 10u, 0u } } });
    messages.push_back(std::move(center.processMeterage(deviceId, MessageMeterage(5u, 50u))));
    messages.push_back(std::move(center.processMeterage(deviceId, MessageMeterage(5u, 50u))));
    center.setSchedule({ deviceId, {} });
    messages.push_back(std::move(center.processMeterage(deviceId, MessageMeterage(6u, 50u))));
    messages.push_back(std::move(center.processMeterage(deviceId, MessageMeterage(6u, 50u))));
    std::vector<std::shared_ptr<Message>> expected = {
        std::shared_ptr<Message>(new MessageCommand(1)),
        std::shared_ptr<Message>(new MessageCommand(-50)),
        std::shared_ptr<Message>(new MessageCommand(0)),
        std::shared_ptr<Message>(new MessageCommand(-50)),
        std::shared_ptr<Message>(new MessageCommand(-30)),
        std::shared_ptr<Message>(new MessageError(MessageError::ErrorType::NoTimestamp)),
        std::shared_ptr<Message>(new MessageError(MessageError::ErrorType::Obsolete)),
        std::shared_ptr<Message>(new MessageError(MessageError::ErrorType::NoSchedule)),
        std::shared_ptr<Message>(new MessageError(MessageError::ErrorType::Obsolete)),
    };
    COMPARE_VECTORS_OF_SMART_PTRS(expected, messages);
}

void commandCenterDeviationTest()
{
    CommandCenter center;
    uint64_t deviceId = 123u;
    std::vector<std::shared_ptr<Message>> messages;
    center.setSchedule({ deviceId, { { 0u, 50u }, { 3u, 90u } } });
    messages.push_back(std::move(center.processMeterage(deviceId, MessageMeterage(0u, 100u))));
    messages.push_back(std::move(center.processMeterage(deviceId, MessageMeterage(1u, 0u))));
    messages.push_back(std::move(center.processMeterage(deviceId, MessageMeterage(2u, 100u))));
    messages.push_back(std::move(center.processMeterage(deviceId, MessageMeterage(3u, 0u))));
    messages.push_back(std::move(center.processMeterage(deviceId, MessageMeterage(4u, 50u))));
    messages.push_back(std::move(center.processMeterage(deviceId, MessageMeterage(5u, 100u))));

    const auto& deviations = center.deviationStats(deviceId);
    std::vector<DeviationStats> expected = { { { 0u, 50u }, 0u, 50.0 }, { { 3u, 90u }, 3u, 57.15 } };
    ASSERT_EQUAL(expected.size(), deviations.size());
    for (size_t i = 0; i < expected.size(); ++i)
    {
        ASSERT_WITH_THRESHOLD(expected[i].deviation, deviations[i].deviation, 1.0);
        ASSERT_EQUAL(expected[i].phase.timeStamp, deviations[i].phase.timeStamp);
        ASSERT_EQUAL(expected[i].phase.value, deviations[i].phase.value);
    }
}

void commandCenterDeviationNewScheduleTest()
{
    CommandCenter center;
    uint64_t deviceId = 123u;
    std::vector<std::shared_ptr<Message>> messages;
    center.setSchedule({ deviceId, { { 0u, 50u }, { 3u, 90u } } });
    messages.push_back(std::move(center.processMeterage(deviceId, MessageMeterage(0u, 100u))));
    messages.push_back(std::move(center.processMeterage(deviceId, MessageMeterage(1u, 0u))));
    messages.push_back(std::move(center.processMeterage(deviceId, MessageMeterage(2u, 100u))));
    messages.push_back(std::move(center.processMeterage(deviceId, MessageMeterage(3u, 80u))));
    messages.push_back(std::move(center.processMeterage(deviceId, MessageMeterage(4u, 100u))));
    messages.push_back(std::move(center.processMeterage(deviceId, MessageMeterage(5u, 80u))));
    center.setSchedule({ deviceId, { { 0u, 0u }, { 6u, 30u } } });
    messages.push_back(std::move(center.processMeterage(deviceId, MessageMeterage(6u, 0u))));
    messages.push_back(std::move(center.processMeterage(deviceId, MessageMeterage(7u, 60u))));
    messages.push_back(std::move(center.processMeterage(deviceId, MessageMeterage(8u, 0u))));
    center.setSchedule({ deviceId, { { 0u, 0u } } });
    messages.push_back(std::move(center.processMeterage(deviceId, MessageMeterage(9u, 50u))));

    const auto& deviations = center.deviationStats(deviceId);
    std::vector<DeviationStats> expected = {
        { { 0u, 50u }, 0u, 50.0 },
        { { 3u, 90u }, 3u, 10.0 },
        { { 6u, 30u }, 6u, 30.0 },
        { { 0u, 0u }, 9u, 50.0 },
    };
    ASSERT_EQUAL(expected.size(), deviations.size());
    for (size_t i = 0; i < expected.size(); ++i)
    {
        ASSERT_WITH_THRESHOLD(expected[i].deviation, deviations[i].deviation, 1.0);
        ASSERT_EQUAL(expected[i].phase.timeStamp, deviations[i].phase.timeStamp);
        ASSERT_EQUAL(expected[i].phase.value, deviations[i].phase.value);
    }
}

void commandCenterForgetTest()
{
    CommandCenter center;
    uint64_t deviceId = 123u;
    std::vector<std::shared_ptr<Message>> messages;
    MessageMeterage meterage = MessageMeterage(0u, 0u);
    center.setSchedule({ deviceId, { { 0u, 1u } } });
    messages.push_back(std::move(center.processMeterage(deviceId, meterage)));
    messages.push_back(std::move(center.processMeterage(deviceId, meterage)));

    std::vector<std::shared_ptr<Message>> expectedMsg = {
        std::shared_ptr<Message>(new MessageCommand(1)),
        std::shared_ptr<Message>(new MessageError(MessageError::ErrorType::Obsolete)),
    };
    COMPARE_VECTORS_OF_SMART_PTRS(expectedMsg, messages);

    auto deviations = center.deviationStats(deviceId);
    std::vector<DeviationStats> expectedDev = { { { 0u, 1u }, 0u, 1.0 } };
    ASSERT_EQUAL(expectedDev.size(), deviations.size());
    for (size_t i = 0; i < expectedDev.size(); ++i)
    {
        ASSERT_WITH_THRESHOLD(expectedDev[i].deviation, deviations[i].deviation, 1.0);
        ASSERT_EQUAL(expectedDev[i].phase.timeStamp, deviations[i].phase.timeStamp);
        ASSERT_EQUAL(expectedDev[i].phase.value, deviations[i].phase.value);
    }

    center.forgetDevice(deviceId);
    messages.push_back(std::move(center.processMeterage(deviceId, meterage)));

    expectedMsg.push_back(std::shared_ptr<Message>(new MessageError(MessageError::ErrorType::NoSchedule)));
    COMPARE_VECTORS_OF_SMART_PTRS(expectedMsg, messages);

    deviations = center.deviationStats(deviceId);
    ASSERT_EQUAL(0u, deviations.size());
}
