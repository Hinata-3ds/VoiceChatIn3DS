#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <cstring>
#include <stdexcept>
#include "types.h"
#include "Socket.hpp"

namespace CTRPluginFramework
{
    Socket::Socket() : _socket(-1), _newAccept(-1) {}

    Socket::~Socket() {
        closeSocket();
    }

    int Socket::createSocket(int domain, int type, int protocol) {
        _socket = socket(domain, type, protocol);
        if (_socket < 0) {
            throw std::runtime_error("Error creating socket");
        }
        return _socket;
    }

    ssize_t Socket::Receive(void *buf, size_t len, int flags) {
        ssize_t receivedSize = recv(_socket, buf, len, flags);
        if (receivedSize < 0) {
            throw std::runtime_error("Error receiving data");
        }
        return receivedSize;
    }

    ssize_t Socket::Send(const void *buf, size_t len, int flags) {
        ssize_t sentSize = send(_socket, buf, len, flags);
        if (sentSize < 0) {
            throw std::runtime_error("Error sending data");
        }
        return sentSize;
    }

    int Socket::connectSocket(const struct sockaddr *addr, socklen_t addrlen) {
        int result = connect(_socket, addr, addrlen);
        if (result < 0) {
            throw std::runtime_error("Error connecting socket");
        }
        return result;
    }

    int Socket::bindSocket(const struct sockaddr *addr, socklen_t addrlen) {
        int result = bind(_socket, addr, addrlen);
        if (result < 0) {
            throw std::runtime_error("Error binding socket");
        }
        return result;
    }

    int Socket::listenSocket(int backlog) {
        int result = listen(_socket, backlog);
        if (result < 0) {
            throw std::runtime_error("Error listening on socket");
        }
        return result;
    }

    Result Socket::acceptConnection(struct sockaddr *addr, socklen_t *addrlen) {
        _newAccept = accept(_socket, addr, addrlen);
        if (_newAccept < 0) {
            throw std::runtime_error("Error accepting connection");
            return _newAccept;
        }
        closeSocket(); // Close the previous socket before creating a new one.
        _socket = _newAccept; // Use the accepted socket for subsequent operations.
        return _newAccept;
    }

    int Socket::closeSocket() {
        if (_socket >= 0) {
            int result = close(_socket);
            _socket = -1;
            if (result < 0) {
                throw std::runtime_error("Error closing socket");
            }
            return result;
        }
        return 0;
    }

    int Socket::getNewAccept() const {
        return _newAccept;
    }
}
