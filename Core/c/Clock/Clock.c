#include "../include/Core.h"

#include <time.h>
#include <string.h>

void GetTime(char* Buffer, size_t Size){
    time_t Now = time(NULL);
    
    struct tm T;
    memset(&T, 0, sizeof(struct tm));

#if defined(_WIN32)
    localtime_s(&T, &Now);
#else
    localtime_r(&Now, &T);
#endif
    
    strftime(Buffer, Size, "%Y-%m-%d %H:%M:%S", &T);
}