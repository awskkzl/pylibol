/*********************************************************************************
*     File Name           :     platform_win32.h
*     Created By          :     yuewu
*     Creation Date       :     [2015-10-23 13:33]
*     Last Modified       :     [2015-12-03 14:49]
*     Description         :     platform specific functions for windows
**********************************************************************************/

#ifndef SOL_UTIL_PLATFORM_WIN32_H__
#define SOL_UTIL_PLATFORM_WIN32_H__

#include <windows.h>

namespace sol {

inline FILE* open_file(const char* path, const char* mode) {
  FILE* file;
  errno_t ret = fopen_s(&file, path, mode);
  if (ret != 0) {
    return nullptr;
  }
  return file;
}

}  // namespace sol

#endif  // SOL_UTIL_PLATFORM_WIN32_H__
