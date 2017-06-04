#include <errno.h>
#include <fcntl.h>
#include <sstream>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include "file_utils.h"

#include <iostream>
using namespace std;

// Moveable, but not copyable (like unique_ptr or fstream).
class ReadableFile {
public:
    ReadableFile() : ReadableFile(-1) {}
    ReadableFile(int fd) : fd(fd) {}

    Status ReadRemaining(string& result) {
        stringstream ss;

        char buf[1024];
        for (;;) {
            ssize_t num_read = read(fd, buf, sizeof(buf));
            if (num_read == 0) {
                break;
            }
            if (num_read == -1) {
                return Status::IOError("Error reading file: " + string(strerror(errno)));
            }
            ss.write(buf, num_read);
        }

        result = ss.str();
        return Status();
    }

    ReadableFile(ReadableFile&& other) : fd(other.fd) {
        other.fd = -1;
    }

    ReadableFile& operator=(ReadableFile&& rhs) {
        // Check if self-move.
        if (this != &rhs) {
            // Close our file descriptor because we're about to overwrite it.
            if (fd != -1) {
                close(fd);
            }

            // Steal the file descriptor from the old object
            fd = rhs.fd;

            // Set the rhs file descriptor to -1; his destructor is still going to be invoked
            // (for every successful constructor call, there's an equal and opposite destructor
            // call) and we don't want the rhs destructor to close our file descriptor on us.
            rhs.fd = -1;
        }
        return *this;
    }

    ~ReadableFile() {
        if (fd != -1) {
            close(fd);
        }
    }

    static Status Open(string const path, ReadableFile& result) {
        int fd = open(path.c_str(), O_RDONLY);
        if (fd == -1) {
            switch (errno) {
                case ENOENT:
                    return Status::FileNotFound("File not found: " + path);

                default:
                    return Status::IOError("Error reading file: " + string(strerror(errno)));
            }
        }

        result = ReadableFile(fd);
        return Status();
    }

private:
    int fd;

    // Copying ReadableFile objects doesn't make much sense because we can't share file descriptors,
    // so let's delete the copy constructor and copy assignment methods.
    ReadableFile(ReadableFile const& file) = delete;
    ReadableFile& operator=(ReadableFile const& rhs) = delete;
};


Status FileUtils::ReadFile(string const path, string& result) {
    ReadableFile file;

    Status s = ReadableFile::Open(path, file);
    if (!s.ok()) {
        return s;
    }

    return file.ReadRemaining(result);
}
