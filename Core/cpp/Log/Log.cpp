#if defined(__ANDROID__)
#include <android/log.h>
#else
#include <iostream>
#endif

void Log(const char* Message){

    if(!Message) return;

#if defined(__ANDROID__)
    __android_log_print(ANDROID_LOG_INFO, "MyApp", "%s", Message);
#else
    std::cout << Message << '\n';
#endif
}