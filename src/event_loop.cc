#include <sstream>
#include "event_loop.h"
#include "exceptions.h"
#include "io_utils.h"

using namespace std;


#if defined(HAVE_EPOLL)


    EventLoop::EventLoop(void) {
    }


    EventLoop::~EventLoop(void) {
    }


    void EventLoop::Add(int fd, int filter, void* data) {
    }


    void EventLoop::Update(int fd, int filter, void* data) {
    }


    void EventLoop::Remove(int fd) {
    }

    EventLoop::Notification EventLoop::Poll() {
        return EventLoop::Notification(0, 0);
    }


#elif defined(HAVE_KQUEUE)


    EventLoop::EventLoop(void) {
        kq = kqueue();
        if (kq == -1) {
            throw EventLoopException("Error creating kqueue fd: " + string(strerror(errno)));
        }
    }


    EventLoop::~EventLoop(void) {
        IOUtils::Close(kq);
    }


    void EventLoop::Add(int fd, int filter, void* data) {
        struct kevent event;
        EV_SET(&event, fd, filter, EV_ADD, 0, 0, data);

        int err = kevent(kq, &event, 1, nullptr, 0, nullptr);
        if (err == -1) {
            stringstream ss;
            ss << "Error adding event to kqueue: " << strerror(errno);
            throw EventLoopException(ss.str());
        }

        if (event.flags & EV_ERROR) {
            stringstream ss;
            ss << "Error adding event to kqueue: " << strerror(event.data);
            throw EventLoopException(ss.str());
        }
    }


    void EventLoop::Update(int fd, int filter, void* data) {
    }


    void EventLoop::Remove(int fd) {
    }

    EventLoop::Notification EventLoop::Poll() {
        for (;;) {
            struct kevent tevent;
            int num_events = kevent(kq, nullptr, 0, &tevent, 1, nullptr);
            if (num_events == -1) {
                stringstream ss;
                ss << "kevent(): " << strerror(errno);
                throw EventLoopException(ss.str());
            } else if (num_events > 0) {
                int fd = tevent.ident;
                return EventLoop::Notification(tevent.ident, 0);
            }
        }
    }


#endif


int EventLoop::Notification::GetFD() const {
    return fd;
}


int EventLoop::Notification::GetEvents() const {
    return events;
}
