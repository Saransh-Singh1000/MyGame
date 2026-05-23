#ifndef LOGLEVEL_H
#define LOGLEVEL_H

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

#ifdef __cplusplus
}
#endif

#endif