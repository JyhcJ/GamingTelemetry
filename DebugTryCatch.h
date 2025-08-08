#pragma once

#include "ThreadSafeLogger.h"

#ifdef NDEBUG
// Release ģʽ�������쳣����ӡ��־
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
// Debug ģʽ���������쳣��ֱ�ӱ�����
#define DEBUG_TRY
#define DEBUG_CATCH(LOG_MSG)
#endif