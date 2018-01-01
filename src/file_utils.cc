#include <errno.h>
#include <fcntl.h>
#include <sstream>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include "exceptions.h"
#include "file_utils.h"
#include "io_utils.h"

#include <iostream>

using namespace std;


string FileUtils::ReadFile(string const& path) {
    stringstream ss;

    int fd = open(path.c_str(), O_RDONLY);
    if (fd == -1) {
        switch (errno) {
            case ENOENT:
                throw FileNotFoundException("File not found: " + path);

            default:
                throw IOException("Error reading file: " + string(strerror(errno)));
        }
    }

    char buf[4096];
    for (;;) {
        ssize_t num_read = read(fd, buf, sizeof(buf));
        if (num_read == 0) {
            break;
        }
        if (num_read == -1) {
            IOUtils::Close(fd);
            throw IOException("Error reading file: " + string(strerror(errno)));
        }
        ss.write(buf, num_read);
    }

    IOUtils::Close(fd);
    return ss.str();
}
