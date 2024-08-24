#ifndef EVENT_LOOP_HPP_
#define EVENT_LOOP_HPP_

#include <functional>
#include <mutex>
#include <vector>
#include <list>
#include <map>
#include <sys/epoll.h>

class EventLoop
{
public:
    EventLoop();
    ~EventLoop();
    using EventHandler = std::function<void (const int fd, const unsigned int events)>;
    void addMonitoredFd(const int fd, const unsigned int events, const EventHandler& eh);
    void addMonitoredFd(const int fd, const unsigned int events, EventHandler&& eh);
    void deleteMonitoredFd(const int fd);
    using Callback = std::function<void ()>;
    void postCallback(const Callback& callback);
    void run();
    void stop();

    EventLoop(const EventLoop&) = delete;
    EventLoop(EventLoop&&) = delete;
    EventLoop& operator = (const EventLoop&) = delete;
    EventLoop& operator = (EventLoop&&) = delete;

private:
    template <typename T>
    void addMonitoredFdImpl(const int fd, const unsigned int events, T&& eh);
    void initWakeupEvent();
    void waitAndHandleEvents();
    void executeHandler(const epoll_event& e);
    void wakeup();
    void wakeupHandler(const int fd, const unsigned int events);
    void executeCallbacks();
    bool isCallbackExist();
    std::list<Callback> popCallbacks();

    bool stopped;
    int epoll_fd;
    int wakeup_fd;
    std::mutex mutex;
    std::list<Callback> callbacks;
    std::vector<epoll_event> ebuffer;
    std::map<int, EventHandler> handlers;
};

#endif
