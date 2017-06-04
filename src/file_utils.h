#ifndef KIWI_FILE_UTILS_H_
#define KIWI_FILE_UTILS_H_

#include <string>
#include "status.h"

using namespace std;

namespace FileUtils {
    Status ReadFile(string const path, string& result);
}

#endif  // KIWI_FILE_UTILS_H_
