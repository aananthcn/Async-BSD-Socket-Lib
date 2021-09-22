#include <gtest/gtest.h>
#include <algorithm>
#include <iterator>
#include "../src/udp_client.h"

namespace AsyncBsdSocketLib
{
    class UdpMulticastTest : public testing::Test
    {
    protected:
        const std::string cMulticastAddress = "224.0.0.0";
        const std::string cLoopbackNic = "127.0.0.1";
        const uint16_t cAlicePort = 8092;
        const uint16_t cBobPort = 8093;

        UdpClient Alice;
        UdpClient Bob;
        bool SuccessfulSetup;

        UdpMulticastTest() : Alice(
                                 cMulticastAddress,
                                 cAlicePort,
                                 cLoopbackNic,
                                 cMulticastAddress),
                             Bob(
                                 cMulticastAddress,
                                 cBobPort,
                                 cLoopbackNic,
                                 cMulticastAddress)
        {
        }

        void SetUp()
        {
            SuccessfulSetup = Alice.TrySetup();

            if (SuccessfulSetup)
            {
                SuccessfulSetup = Bob.TrySetup();
            }
        }

        void TearDown()
        {
            Bob.TryClose();
            Alice.TryClose();
        }
    };

    TEST_F(UdpMulticastTest, AliceBobTalk)
    {
        EXPECT_TRUE(SuccessfulSetup);
    }

    TEST_F(UdpMulticastTest, Echo)
    {
        const std::size_t cBufferSize = 16;

        const std::array<uint8_t, cBufferSize> _sendBuffer{
            0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
            0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f};

        // Evaluate sending
        ssize_t _sentBytes =
            Alice.Send(_sendBuffer, cMulticastAddress, cBobPort);
        EXPECT_GT(_sentBytes, 0);

        std::array<uint8_t, cBufferSize> _receiveBuffer;
        std::string _senderIpAddress;
        uint16_t _senderPort;

        ssize_t _receivedBytes =
            Bob.Receive(_receiveBuffer, _senderIpAddress, _senderPort);

        // Evalute receiving and the sender
        EXPECT_GT(_receivedBytes, 0);
        EXPECT_EQ(_senderIpAddress, cLoopbackNic);
        EXPECT_EQ(_senderPort, cAlicePort);

        // Evaluate echo data
        bool _areEqual =
            std::equal(
                std::cbegin(_sendBuffer),
                std::cend(_sendBuffer),
                std::cbegin(_receiveBuffer));
        EXPECT_TRUE(_areEqual);
    }
}