#include <iostream>
#include <memory>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <string.h>
#include "EventLoop.hpp"

int main()
{
    addrinfo *result = nullptr;
    int ret = getaddrinfo("127.0.0.1", "2233", nullptr, &result);
    if (ret != 0)
    {
        std::cout << "getaddrinfo failed: " 
            << ((ret == EAI_SYSTEM) ? strerror(errno) : gai_strerror(ret)) << std::endl;
        return -1;
    }

    auto loop = std::make_shared<EventLoop>();
    int listen_fd = socket(result->ai_family, SOCK_STREAM, 0);
    bind(listen_fd, result->ai_addr, result->ai_addrlen);
    freeaddrinfo(result);
    listen(listen_fd, SOMAXCONN);
    std::cout << "listen fd " << listen_fd << std::endl;

    loop->addMonitoredFd(listen_fd, EPOLLIN, [=](const int listen_fd, const unsigned int events)
    {
        int client_fd = accept4(listen_fd, nullptr, nullptr, SOCK_NONBLOCK|SOCK_CLOEXEC);
        if (client_fd == -1)
        {
            std::cout << "accept4 failed: " << strerror(errno) << std::endl;
            loop->stop();
            loop->deleteMonitoredFd(listen_fd);
            close(listen_fd);
            return;
        }

        std::cout << "new client fd " << client_fd << std::endl;
        loop->addMonitoredFd(client_fd, EPOLLIN, [=](const int client_fd, const unsigned int events)
        {
            if (events & EPOLLIN)
            {
                std::cout << "handler client fd " << client_fd << std::endl;
                char buf[1024];
                do {
                    int n = recv(client_fd, buf, sizeof(buf), 0);
                    if (n <= 0)
                        break;
                    if (send(client_fd, buf, n, 0) < 0)
                        break;
                    return;
                } while(0);
            }
            std::cout << "delete client fd " << client_fd << std::endl;
            loop->deleteMonitoredFd(client_fd);
            close(client_fd);
        });
    });

    loop->run();
    return 0;
}
