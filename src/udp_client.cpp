#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "./udp_client.h"

namespace AsyncBsdSocketLib
{
    UdpClient::UdpClient(
        std::string ipAddress, uint16_t port) : NetworkSocket(ipAddress, port),
                                                mIsMulticast{false},
                                                mShareAddress{false}
    {
    }

    UdpClient::UdpClient(
        std::string ipAddress,
        uint16_t port,
        std::string nicIpAddress,
        std::string multicastIpAddress,
        bool shareAddress) : NetworkSocket(ipAddress, port),
                             mNicIpAddress{nicIpAddress},
                             mMulticastIpAddress{multicastIpAddress},
                             mIsMulticast{true},
                             mShareAddress{shareAddress}
    {
    }

    int UdpClient::Connection() const noexcept
    {
        return mDescriptor;
    }

    bool UdpClient::TrySetup() noexcept
    {
        mDescriptor = socket(AF_INET, SOCK_DGRAM, 0);

        bool _result = (mDescriptor >= 0);

        if (_result && mIsMulticast && mShareAddress)
        {
            int _enable = 1;

            // Enable re-use address flag
            _result =
                (setsockopt(
                     mDescriptor,
                     SOL_SOCKET,
                     SO_REUSEADDR,
                     (const void *)(&_enable),
                     sizeof(_enable)) > -1);
        }

        if (_result)
        {
            struct sockaddr_in _address;
            _address.sin_addr.s_addr = inet_addr(mIpAddress.c_str());
            _address.sin_family = AF_INET;
            _address.sin_port = htons(mPort);

            //! \remark Binding the socket to the port
            _result =
                (bind(
                     mDescriptor,
                     (struct sockaddr *)&_address,
                     sizeof(_address)) == 0);
        }

        // Set multicast socket option if the binding was successful
        if (mIsMulticast && _result)
        {
            struct in_addr _nicAddress;
            _nicAddress.s_addr = inet_addr(mNicIpAddress.c_str());

            // Enable sending multicast traffic through the NIC
            _result =
                (setsockopt(
                     mDescriptor,
                     IPPROTO_IP, IP_MULTICAST_IF,
                     (const void *)(&_nicAddress),
                     sizeof(_nicAddress)) > -1);

            if (_result)
            {
                struct ip_mreq _multicastGroup;
                _multicastGroup.imr_interface.s_addr =
                    inet_addr(mNicIpAddress.c_str());
                _multicastGroup.imr_multiaddr.s_addr =
                    inet_addr(mMulticastIpAddress.c_str());

                // Subscribe to the multicast group for receiving the multicast traffic
                _result =
                    (setsockopt(mDescriptor,
                                IPPROTO_IP, IP_ADD_MEMBERSHIP,
                                (const void *)(&_multicastGroup),
                                sizeof(_multicastGroup)) > -1);
            }
        }

        return _result;
    }

    template <std::size_t N>
    ssize_t UdpClient::Send(
        const std::array<uint8_t, N> &buffer,
        std::string ipAddress,
        uint16_t port) const noexcept
    {
        // No flag for sending
        const int _flags = 0;

        struct sockaddr_in _destinationAddress;
        _destinationAddress.sin_addr.s_addr = inet_addr(ipAddress.c_str());
        _destinationAddress.sin_family = AF_INET;
        _destinationAddress.sin_port = htons(port);

        ssize_t _result =
            sendto(
                mDescriptor,
                buffer.size,
                N,
                _flags,
                (struct sockaddr *)&_destinationAddress,
                sizeof(_destinationAddress));

        return _result;
    }

    template <std::size_t N>
    ssize_t UdpClient::Receive(
        std::array<uint8_t, N> &buffer,
        std::string &ipAddress,
        uint16_t &port) const noexcept
    {
        // No flag for receiving
        const int _flags = 0;

        struct sockaddr_in _sourceAddress;
        int _sourceAddressLength = sizeof(_sourceAddress);

        ssize_t _result =
            recvfrom(
                mDescriptor,
                buffer.data,
                N,
                _flags,
                (struct sockaddr *)&_sourceAddress,
                &_sourceAddressLength);

        // Convert IP address
        char _ipAddress[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &(_sourceAddress.sin_addr), _ipAddress, INET_ADDRSTRLEN);
        ipAddress = std::string(_ipAddress);

        // Convert port number
        port = htons(_sourceAddress.sin_port);

        return _result;
    }
}
