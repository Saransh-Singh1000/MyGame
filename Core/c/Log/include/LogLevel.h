#pragma once

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    Debug,
    Info,
    Warning,
    Error,
    Fatal,
} LogLevel;

void Log(LogLevel level, const char* message);

#ifdef __cplusplus
}
#endif