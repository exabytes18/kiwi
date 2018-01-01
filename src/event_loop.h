#ifndef KIWI_EVENT_LOOP_H_
#define KIWI_EVENT_LOOP_H_

#include "config.h"
#if defined(HAVE_EPOLL)
    #include <sys/epoll.h>
#elif defined(HAVE_KQUEUE)
    #include <sys/event.h>
#endif

using namespace std;


class EventLoop {
public:
    #if defined(HAVE_EPOLL)
        static const int EVENT_READ = EPOLLIN;
        static const int EVENT_WRITE = EPOLLOUT;
    #elif defined(HAVE_KQUEUE)
        static const int EVENT_READ = EVFILT_READ;
        static const int EVENT_WRITE = EVFILT_WRITE;
    #endif

    class Notification {
    public:
        Notification(int fd, int events) : fd(fd), events(events) {}
        Notification(const Notification& mE)            = default;
        Notification(Notification&& mE)                 = default;
        Notification& operator=(const Notification& mE) = default;
        Notification& operator=(Notification&& mE)      = default;
        int GetFD() const;
        int GetEvents() const;

    private:
        int fd;
        int events;
    };

    EventLoop(void);
    ~EventLoop(void);
    void Add(int fd, int filter, void* data);
    void Update(int fd, int filter, void* data);
    void Remove(int fd);
    Notification Poll();


private:
    #if defined(HAVE_EPOLL)
        int epoll_fd;
    #elif defined(HAVE_KQUEUE)
        int kq;
    #endif
};

#endif  // KIWI_EVENT_LOOP_H_
