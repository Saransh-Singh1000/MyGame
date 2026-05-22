#include <stdlib.h>

#include "../include/Core.h"
#include "include/LogLevel.h"

#if defined(__ANDROID__)
#include <android/log.h>

#else
#include <stdio.h>
#endif

#define TAG "MyGame"

#define COLOR_RESET "\x1b[0m"
#define COLOR_INFO "\x1b[32m"
#define COLOR_WARNING "\x1b[33m"
#define COLOR_ERROR "\x1b[31m"
#define COLOR_FAULT "\x1b[35m"


void Log(LogLevel Level, const char* Message){
#if defined(__ANDROID__)
    switch (Level){
        case Debug:
            __android_log_print(ANDROID_LOG_DEBUG, TAG, "%s", Message);
             break;
        case Info:
            __android_log_print(ANDROID_LOG_INFO, TAG, "%s", Message);
            break;
        case Warning:
            __android_log_print(ANDROID_LOG_WARN, TAG, "%s", Message);
            break;
        case Error:
            __android_log_print(ANDROID_LOG_ERROR, TAG, "%s", Message);
            break;
        case Fault:
            __android_log_print(ANDROID_LOG_FATAL, TAG, "%s", Message);

            fflush(stdout);
            fflush(stderr);

            abort();
    }
#else
    char TimeBuffer[64];
    GetTime(TimeBuffer, sizeof(TimeBuffer));

    switch (Level) {
        case Debug:
            printf("[%s] [DEBUG] %s\n", TimeBuffer, Message);
            break;
        case Info:
            printf(COLOR_INFO "[%s] [INFO] %s\n" COLOR_RESET, TimeBuffer, Message);
            break;
        case Warning:
            printf(COLOR_WARNING "[%s] [WARNING] %s\n" COLOR_RESET, TimeBuffer, Message);
            break;
        case Error:
            printf(COLOR_ERROR "[%s] [ERROR] %s\n" COLOR_RESET, TimeBuffer, Message);
            break;
        case Fault:
            printf(COLOR_FAULT "[%s] [FAULT] %s\n" COLOR_RESET, TimeBuffer, Message);

            fflush(stdout);
            fflush(stderr);

            abort();
    }
#endif
}