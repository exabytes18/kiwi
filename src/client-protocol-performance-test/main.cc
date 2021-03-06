#include <iostream>
#include <pthread.h>
#include <signal.h>
#include "client.h"
#include "common/constants.h"


using namespace std;

static void PrintUsage(int argc, char * const argv[]) {
    const char * program_name = (argc > 0) ? (argv[0]) : ("(unknown)");
    cerr << "usage: " << program_name << " server-address" << endl;
}


static int Run(const char * address, const sigset_t termination_signals) {

    // Parse the server address
    SocketAddress server_address = SocketAddress::FromString(address, Constants::DEFAULT_PORT);

    // Initialize the client
    Client client(server_address);

    // Wait for termination signal
    int termination_signal;
    int err = sigwait(&termination_signals, &termination_signal);
    if (err != 0) {
        cerr << "Problem waiting for signal: " << strerror(err) << endl;
        return 1;
    }

    // Bye
    return 0;
}


int main(int argc, char * const argv[]) {
    sigset_t termination_signals;
    sigemptyset(&termination_signals);
    sigaddset(&termination_signals, SIGINT);
    sigaddset(&termination_signals, SIGTERM);

    /*
     * Block termination signals
     * 
     * Note: all threads inherit this sigmask. It's imperative that threads do not
     * modify their sigmask from this inherited version; this ensures that the
     * termination signals are properly blocked and not delivered to random threads.
     * We rely upon this expectation so that we can then deliver the signals in a
     * controlled manner within the main thread.
     */
    int err = pthread_sigmask(SIG_BLOCK, &termination_signals, nullptr);
    if (err != 0) {
        cerr << "Problem blocking signals: " << strerror(err) << endl;
        return 1;
    }

    if (argc != 2) {
        PrintUsage(argc, argv);
        return 1;
    }

    try {
        return Run(argv[1], termination_signals);
    } catch (exception const& e) {
        cerr << "Problem running server: " << e.what() << endl;
        return 1;
    } catch (...) {
        cerr << "Problem running server" << endl;
        return 1;
    }
}
