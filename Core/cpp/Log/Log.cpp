#include "../include/Core.hpp"

#if defined(__ANDROID__)
#include <android/log.h>
#else
#include <iostream>
#endif

#include "include/LogLevel.h"

void Log(LogLevel Level, const char* Message){

    if(!Message) return;

#if defined(__ANDROID__)
    android_LogPriority AndroidLogLevel;

    switch (Level){
        case LogLevel::Info:
            AndroidLogLevel = ANDROID_LOG_INFO;
            break;
        
        case LogLevel::Warning:
            AndroidLogLevel = ANDROID_LOG_WARN;
            break;

        case LogLevel::Error:
            AndroidLogLevel = ANDROID_LOG_ERROR;
            break;

        case LogLevel::Fatal:
            AndroidLogLevel = ANDROID_LOG_FATAL;
            break;
    }

    __android_log_print(AndroidLogLevel, "MyGame", "%s", Message);

#else
    const char* Prefix = "";
    std::ostream* Stream = &std::cout;

    switch(Level){
        case LogLevel::Info:
            break;

        case LogLevel::Warning:
            Prefix = "[WARNING]";
            break;
        
        case LogLevel::Error:
            Prefix = "[ERROR]";
            Stream = &std::cerr;
            break;

        case LogLevel::Fatal:
            Prefix = "[FATAL]";
            Stream = &std::cerr;
            break;
    }

    (*Stream) << Prefix << Message << '\n';

#endif
}