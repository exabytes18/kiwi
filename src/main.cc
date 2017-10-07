#include <iostream>
#include <pthread.h>
#include <signal.h>
#include "server.h"

using namespace std;


static void print_usage(int argc, char * const argv[]) {
    const char * program_name = (argc > 0) ? (argv[0]) : ("(unknown)");
    cerr << "usage: " << program_name << " config_file_path" << endl;
}


static int run(const char * config_file_path, const sigset_t termination_signals) {
    // Read the configuration file
    ServerConfig server_config = ServerConfig::ParseFromFile(config_file_path);

    // Initialize the storage
    Storage storage(server_config);

    // Start the server
    Server server(server_config, storage);
    server.Start();

    // Wait for termination signal
    int termination_signal;
    int err = sigwait(&termination_signals, &termination_signal);
    if (err != 0) {
        cerr << "Problem waiting for signal: " << strerror(err) << endl;
        return 1;
    }

    // Bye
    server.Shutdown();
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
     * Note: all threads inherit this sigmask. It's critical that threads do not
     * modify their sigmask from this inherited version; this ensures that the
     * termination signals are properly blocked and not delivered to random threads.
     * We rely upon this expectation so that we can then deliver the signals in a
     * controlled manner within the main thread.
     */
    int err = pthread_sigmask(SIG_BLOCK, &termination_signals, NULL);
    if (err != 0) {
        cerr << "Problem blocking signals: " << strerror(err) << endl;
        return 1;
    }

    if (argc != 2) {
        print_usage(argc, argv);
        return 1;
    }

    try {
        return run(argv[1], termination_signals);
    } catch (const exception& e) {
        cerr << "Problem running server: " << e.what() << endl;
        return 1;
    } catch (...) {
        cerr << "Problem running server" << endl;
        return 1;
    }
}
