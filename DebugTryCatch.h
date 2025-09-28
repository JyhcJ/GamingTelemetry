#pragma once

#include "ThreadSafeLogger.h"

#ifdef NDEBUG
// Release 模式：捕获异常并打印日志
#define DEBUG_TRY try
#define DEBUG_CATCH(LOG_MSG) \
        catch (const std::exception& e) { \
            std::cerr << "[Release] " << LOG_MSG << ": " << e.what() << std::endl; \
            LOG_IMMEDIATE(LOG_MSG + std::string(e.what()));\
        } \
        catch (...) { \
            std::cerr << "[Release] " << LOG_MSG << ": Unknown error!" << std::endl; \
            LOG_IMMEDIATE(LOG_MSG+": Unknown error!");\
        }
#else
// Debug 模式：不捕获异常（直接崩溃）
#define DEBUG_TRY
#define DEBUG_CATCH(LOG_MSG)
#endif