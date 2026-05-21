#include "../include/Core.hpp"

#if defined(__ANDROID__)
#include <android/log.h>
#else
#include <iostream>
#include <iomanip>
#include <sstream>
#include <ctime>
#endif

#include "include/LogLevel.h"

#if !defined(__ANDROID__)
static const char* Color(LogLevel level) {
    switch (level) {
        case LogLevel::Info:    return "\033[37m";
        case LogLevel::Warn: return "\033[33m"; 
        case LogLevel::Error:   return "\033[31m";
        case LogLevel::Fatal:   return "\033[91m"; 
    }
    return "\033[0m";
}

static const char* Prefix(LogLevel level) {
    switch (level) {
        case LogLevel::Info:    return "[INFO]";
        case LogLevel::Warn: return "[WARN]";
        case LogLevel::Error:   return "[ERROR]";
        case LogLevel::Fatal:   return "[FATAL]";
    }
    return "";
}

static std::string Time() {
    std::time_t now = std::time(nullptr);
    std::tm* local = std::localtime(&now);

    std::ostringstream oss;
    oss << std::setfill('0')
        << std::setw(2) << local->tm_hour << ":"
        << std::setw(2) << local->tm_min << ":"
        << std::setw(2) << local->tm_sec;

    return oss.str();
}
#endif

void Log(LogLevel Level, const char* Message){

    if(!Message) return;

#if defined(__ANDROID__)

    android_LogPriority AndroidLogLevel;

    switch (Level){
        case LogLevel::Info:
            AndroidLogLevel = ANDROID_LOG_INFO;
            break;

        case LogLevel::Warn:
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

    std::ostream* Stream = &std::cout;

    switch(Level){
        case LogLevel::Info:
            break;

        case LogLevel::Warn:
            break;

        case LogLevel::Error:
        case LogLevel::Fatal:
            Stream = &std::cerr;
            break;
    }

    (*Stream)
        << Color(Level)
        << "[" << Time() << "]"
        << Prefix(Level)
        << Message
        << "\033[0m"
        << '\n';

#endif
}