#include <sys/eventfd.h>
#include <unistd.h>
#include "EventLoop.hpp"

EventLoop::EventLoop():
    stopped(false),
    epoll_fd(epoll_create1(EPOLL_CLOEXEC)),
    wakeup_fd(eventfd(0, EFD_NONBLOCK|EFD_CLOEXEC))
{
    initWakeupEvent();
}

EventLoop::~EventLoop()
{
    close(epoll_fd);
    close(wakeup_fd);
}

void EventLoop::run()
{
    while (!stopped)
    {
        waitAndHandleEvents();
        executeCallbacks();
    }
}

void EventLoop::stop()
{
    postCallback([this](){ stopped = true; });
}

void EventLoop::initWakeupEvent()
{
    addMonitoredFd(wakeup_fd, EPOLLIN, std::bind(&EventLoop::wakeupHandler,
                                                 this,
                                                 std::placeholders::_1,
                                                 std::placeholders::_2));
}

void EventLoop::waitAndHandleEvents()
{
    ebuffer.resize(handlers.size());
    const int count(epoll_wait(epoll_fd, ebuffer.data(), ebuffer.size(), -1));
    if (count <= 0)
        return;

    ebuffer.resize(count);
    for (const auto& event : ebuffer)
        if (event.data.fd != -1)
            executeHandler(event);
}

void EventLoop::executeHandler(const epoll_event& e)
{
    handlers.at(e.data.fd)(e.data.fd, e.events);
}

void EventLoop::addMonitoredFd(const int fd, const unsigned int events, const EventHandler& eh)
{
    addMonitoredFdImpl(fd, events, eh);
}

void EventLoop::addMonitoredFd(const int fd, const unsigned int events, EventHandler&& eh)
{
    addMonitoredFdImpl(fd, events, std::move(eh));
}

template <typename T>
void EventLoop::addMonitoredFdImpl(const int fd, const unsigned int events, T&& eh)
{
    if (handlers.find(fd) != handlers.end())
        return;

    epoll_event e = {};
    e.events = events;
    e.data.fd = fd;
    epoll_ctl(epoll_fd, EPOLL_CTL_ADD, fd, &e);
    handlers.emplace(fd, std::forward<T>(eh));
}

void EventLoop::deleteMonitoredFd(const int fd)
{
    if (handlers.find(fd) == handlers.end())
        return;

    for (auto& event : ebuffer)
        if (event.data.fd == fd)
            event.data.fd = -1;

    epoll_ctl(epoll_fd, EPOLL_CTL_DEL, fd, nullptr);
    handlers.erase(fd);
}

void EventLoop::wakeup()
{
    uint64_t one = 1;
    write(wakeup_fd, &one, sizeof(one));
}

void EventLoop::wakeupHandler(const int fd, const unsigned int events)
{
    if (events & EPOLLIN)
    {
        uint64_t times;
        read(fd, &times, sizeof(times));
    }
}

void EventLoop::postCallback(const Callback& callback)
{
    std::lock_guard<std::mutex> guard(mutex);
    callbacks.push_back(callback);
    wakeup();
}

std::list<EventLoop::Callback> EventLoop::popCallbacks()
{
    std::lock_guard<std::mutex> gurad(mutex);
    std::list<EventLoop::Callback> extracted;
    std::swap(callbacks, extracted);
    return extracted;
}

bool EventLoop::isCallbackExist()
{
    std::lock_guard<std::mutex> gurad(mutex);
    return !callbacks.empty();
}

void EventLoop::executeCallbacks()
{
    if (isCallbackExist())
    {
        auto extractedCallbacks(popCallbacks());
        for (const auto& callback : extractedCallbacks)
            callback();
    }
}
