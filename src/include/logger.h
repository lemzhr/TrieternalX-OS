
#ifndef LOGGER_H
#define LOGGER_H

#include "types.h"

enum LogLevel
{
    LOG_LEVEL_DEBUG,
    LOG_LEVEL_INFO,
    LOG_LEVEL_WARNING,
    LOG_LEVEL_ERROR
};

void logger_init();
void klog(LogLevel level, const char* component, const char* message);

#define klog_debug(component, message) klog(LOG_LEVEL_DEBUG, component, message)
#define klog_info(component, message) klog(LOG_LEVEL_INFO, component, message)
#define klog_warn(component, message) klog(LOG_LEVEL_WARNING, component, message)
#define klog_err(component, message) klog(LOG_LEVEL_ERROR, component, message)

#endif
