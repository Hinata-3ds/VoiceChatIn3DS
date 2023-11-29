#ifndef SOCKET_HPP
#define SOCKET_HPP

#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <cstring>
#include <stdexcept>
#include "HinataCodes/Socket.hpp"

namespace CTRPluginFramework
{
    class Socket {
    public:
        Socket() : _socket(-1) {}

        ~Socket() {
            closeSocket();
        }

        int createSocket(int domain, int type, int protocol) {
            _socket = socket(domain, type, protocol);
            if (_socket < 0) {
                throw std::runtime_error("Error creating socket");
            }
            return _socket;
        }

        ssize_t Receive(void *buf, size_t len, int flags) {
            ssize_t receivedSize = recv(_socket, buf, len, flags);
            if (receivedSize < 0) {
                throw std::runtime_error("Error receiving data");
            }
            return receivedSize;
        }

        ssize_t Send(const void *buf, size_t len, int flags) {
            ssize_t sentSize = send(_socket, buf, len, flags);
            if (sentSize < 0) {
                throw std::runtime_error("Error sending data");
            }
            return sentSize;
        }

        int connectSocket(const struct sockaddr *addr, socklen_t addrlen) {
            int result = connect(_socket, addr, addrlen);
            if (result < 0) {
                throw std::runtime_error("Error connecting socket");
            }
            return result;
        }

        int bindSocket(const struct sockaddr *addr, socklen_t addrlen) {
            int result = bind(_socket, addr, addrlen);
            if (result < 0) {
                throw std::runtime_error("Error binding socket");
            }
            return result;
        }

        int listenSocket(int backlog) {
            int result = listen(_socket, backlog);
            if (result < 0) {
                throw std::runtime_error("Error listening on socket");
            }
            return result;
        }

        int acceptConnection(struct sockaddr *addr, socklen_t *addrlen) {
            int newSocket = accept(_socket, addr, addrlen);
            if (newSocket < 0) {
                throw std::runtime_error("Error accepting connection");
            }
            return newSocket;
        }

        int closeSocket() {
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

    private:
        int _socket;
    };
}

#endif
