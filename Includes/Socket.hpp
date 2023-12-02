#ifndef SOCKET_HPP
#define SOCKET_HPP


#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <cstring>
#include <stdexcept>
#include <3ds.h>
#include <CTRPluginFramework.hpp>

namespace CTRPluginFramework
{
    class Socket {
    public:
        Socket();
        ~Socket();

        int createSocket(int domain, int type, int protocol);
        ssize_t Receive(void *buf, size_t len, int flags);
        ssize_t Send(const void *buf, size_t len, int flags);
        int connectSocket(const struct sockaddr *addr, socklen_t addrlen);
        int bindSocket(const struct sockaddr *addr, socklen_t addrlen);
        int listenSocket(int backlog);
        Result acceptConnection(struct sockaddr *addr, socklen_t *addrlen);
        int closeSocket();
        int getNewAccept() const;

    private:
        int _socket;
        int _newAccept;
    };
}

#endif