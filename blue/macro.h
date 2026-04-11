#ifndef __BLUE_MACRO_H__
#define __BLUE_MACRO_H__
// 自定义宏
#include <string>
#include <assert.h>
#include "util.h"

// x : 条件
#define BLUE_ASSERT(x)                                                                      \
    if (!(x))                                                                               \
    {                                                                                       \
        BLUE_LOG_ERROR(BLUE_LOG_MASSAGE_ROOT()) << "ASSERTION : " #x                        \
                                                << "\nbacktrace\n"                          \
                                                << blue::BacktraceToString(100, 2, "    "); \
        assert(x);                                                                          \
    }
// x : 条件 w : 额外打印的信息
#define BLUE_ASSERT2(x, w)                                                                  \
    if (!(x))                                                                               \
    {                                                                                       \
        BLUE_LOG_ERROR(BLUE_LOG_MASSAGE_ROOT()) << "ASSERTION : " #x                        \
                                                << "\n"                                     \
                                                << w                                        \
                                                << "\nbacktrace\n"                          \
                                                << blue::BacktraceToString(100, 2, "    "); \
        assert(x);                                                                          \
    }
#endif // __BLUE_MACRO_H__