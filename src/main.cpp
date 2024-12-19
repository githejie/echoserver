#include <iostream>
#include <memory>
#include <sys/types.h>
#include <sys/socket.h>
#include <string.h>
#include <netdb.h>
#include <unistd.h>
#include <csignal>
#include "EventLoop.hpp"

std::shared_ptr<EventLoop> loop;

void handle_signal(int signal) {
    if (signal == SIGTERM) {
        std::cout << "Received SIGTERM, stopping the loop..." << std::endl;
        loop->stop();
    }
}

int main(int argc, char* argv[])
{
    if (argc != 3)
    {
        std::cerr << "Usage: " << argv[0] << " <IP> <PORT>" << std::endl;
        return -1;
    }

    const char* ip = argv[1];
    const char* port = argv[2];

    addrinfo *result = nullptr;
    int ret = getaddrinfo(ip, port, nullptr, &result);
    if (ret != 0)
    {
        std::cerr << "getaddrinfo failed: "
            << ((ret == EAI_SYSTEM) ? strerror(errno) : gai_strerror(ret)) << std::endl;
        return -1;
    }

    int listen_fd = socket(result->ai_family, SOCK_STREAM, 0);
    if (listen_fd == -1) {
        std::cerr << "socket creation failed: " << strerror(errno) << std::endl;
        freeaddrinfo(result);
        return -1;
    }

    if (bind(listen_fd, result->ai_addr, result->ai_addrlen) == -1) {
        std::cerr << "bind failed: " << strerror(errno) << std::endl;
        close(listen_fd);
        freeaddrinfo(result);
        return -1;
    }
    freeaddrinfo(result);

    if (listen(listen_fd, SOMAXCONN) == -1) {
        std::cerr << "listen failed: " << strerror(errno) << std::endl;
        close(listen_fd);
        return -1;
    }
    std::cout << "listen fd " << listen_fd << std::endl;

    loop = std::make_shared<EventLoop>();

    // Register the SIGTERM handler
    std::signal(SIGTERM, handle_signal);

    loop->addMonitoredFd(listen_fd, EPOLLIN, [=](const int listen_fd, const unsigned int events)
    {
        (void)events;
        int client_fd = accept4(listen_fd, nullptr, nullptr, SOCK_NONBLOCK|SOCK_CLOEXEC);
        if (client_fd == -1)
        {
            std::cerr << "accept4 failed: " << strerror(errno) << std::endl;
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
