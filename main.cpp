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
    listen(listen_fd, SOMAXCONN);
    std::cout << "listen fd " << listen_fd << std::endl;
    for (;;)
    {
        int client_fd = accept4(listen_fd, nullptr, nullptr, SOCK_NONBLOCK|SOCK_CLOEXEC);
        if (client_fd == -1)
        {
            std::cout << "accept4 failed: " << strerror(errno) << std::endl;
            return -1;
        }
        std::cout << "new client fd " << client_fd << std::endl;
        loop->postCallback([=]()
            {
                loop->addMonitoredFd(client_fd, EPOLLIN, [=](const int fd, const unsigned int events)
                    {
                        if (events & EPOLLIN)
                        {
                            std::cout << "handler client fd " << fd << std::endl;
                            char buf[1024];
                            do {
                                int n = recv(fd, buf, sizeof(buf), 0);
                                if (n <= 0)
                                    break;
                                if (send(fd, buf, n, 0) < 0)
                                    break;
                                return;
                            } while(0);
                        }
                        std::cout << "delete client fd " << fd << std::endl;
                        loop->deleteMonitoredFd(fd);
                        close(fd);
                    });
            });
    }

    freeaddrinfo(result);
    close(listen_fd);
    return 0;
}
