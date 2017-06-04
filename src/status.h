#ifndef KIWI_STATUS_H_
#define KIWI_STATUS_H_

#include <sstream>
#include <string>

using namespace std;

class Status {
public:
    Status() : Status(kOk, kNone, "") {}

    enum Code {
        kOk = 0,
        kIOError = 1,
        kParseError = 2,
        kConfigurationError = 3,
        kStorageError = 4
    };

    enum SubCode {
        kNone = 0,
        kFileNotFound = 1,
        kSyscallError = 2
    };

    /*
     * If you declare a variable as constant, C++ will not let you call any methods
     * on said variable for fear of modification. However, C++ will allow you to still
     * call methods declared as const.
     */
    bool ok() const { return code == kOk; }

    string const ToString() const {
        if (msg == "") {
            return "null";
        } else {
            return msg;
        }
    }

    static Status FileNotFound(string const& msg) { return Status(kIOError, kFileNotFound, msg); }

    static Status IOError(string const& msg) { return Status(kIOError, kNone, msg); }

    static Status ParseError(string const& msg) { return Status(kParseError, kNone, msg); }

    static Status ConfigurationError(string const& msg) { return Status(kConfigurationError, kNone, msg); }

    static Status StorageError(string const& msg) { return Status(kStorageError, kNone, msg); }

private:
    Status(Code code, SubCode subcode, string const& msg) : code(code), subcode(subcode), msg(msg) {}

    Code code;
    SubCode subcode;
    string msg;
};

#endif  // KIWI_STATUS_H_
