#include <iostream>
#include <pthread.h>
#include <signal.h>
#include <string>
#include "config.h"
#include "server.h"

using namespace std;


static void print_help(char const program_name[]) {
    cerr << "usage: " << program_name << " [config_file_path]" << endl;
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
     * modify their sigmask from this inherited version to ensure that the
     * termination signals are properly blocked and not delivered to random threads.
     * We rely upon this expectation so that we can then deliver the signals in a
     * controlled manner within the main thread.
     */
    int err = pthread_sigmask(SIG_BLOCK, &termination_signals, NULL);
    if (err != 0) {
        cerr << "Problem blocking signals: " << strerror(err) << endl;
        return 1;
    }

    Status s;
    ServerConfig server_config;
    switch (argc) {
        case 0:
        case 1:
            // Use default configuration values.
            break;
        
        case 2:
            // Overload the default configuration values with those from the specified file.
            s = server_config.PopulateFromFile(argv[1]);
            if (!s.ok()) {
                cerr << "Problem reading configuration file (" << argv[1] << "): " << s.ToString() << endl;
                return 1;
            }
            break;
        
        default:
            print_help(argv[0]);
            return 1;
    }

    // Initialize the server
    Server server(server_config);

    // Start the server
    s = server.Start();
    if (!s.ok()) {
        cerr << "Problem starting server: " << s.ToString() << endl;
        return 1;
    }

    // Wait for termination signal
    int termination_signal;
    err = sigwait(&termination_signals, &termination_signal);
    if (err != 0) {
        cerr << "Problem waiting for signal: " << strerror(err) << endl;
        return 1;
    }

    // Bye
    server.Shutdown();
    return 0;
}
