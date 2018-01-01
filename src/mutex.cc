#include <iostream>
#include "mutex.h"

using namespace std;


Mutex::Mutex(void) {
    int err = pthread_mutex_init(&mutex, nullptr);
    if (err != 0) {
        cerr << "pthread_mutex_init(): " << strerror(err) << endl;
    }
}


Mutex::~Mutex(void) {
    int err = pthread_mutex_destroy(&mutex);
    if (err != 0) {
        cerr << "pthread_mutex_destroy(): " << strerror(err) << endl;
    }
}


void Mutex::Lock(void) noexcept {
    int err = pthread_mutex_lock(&mutex);
    if (err != 0) {
        cerr << "pthread_mutex_lock(): " << strerror(err) << endl;
        abort();
    }
}


void Mutex::Unlock(void) noexcept {
    int err = pthread_mutex_unlock(&mutex);
    if (err != 0) {
        cerr << "pthread_mutex_unlock(): " << strerror(err) << endl;
        abort();
    }
}
