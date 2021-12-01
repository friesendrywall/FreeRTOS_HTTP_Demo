/* 
 * File:   logs.h
 * Author: Erik
 *
 * Created on August 24, 2017, 10:11 AM
 */

#ifndef LOGS_H
#define	LOGS_H

#ifdef	__cplusplus
extern "C" {
#endif
    
#include <stdint.h>
    
    void LogTasks(void *pvParameters);
    void SysLog(int LogLevel, const char* format, ...);

#define LOG_NORMAL 0
#define LOG_DEBUG 1
#define LOG_VERBOSE 2
#define LOG_MAX 3
    
#define DEBUG_PRINT_LEVEL LOG_VERBOSE


#ifdef	__cplusplus
}
#endif

#endif	/* LOGS_H */

