#include "../include/Core.h"

#include <time.h>

void GetTime(char* Buffer, size_t Size){
    time_t Now = time(NULL);
    struct tm* T = localtime(&Now);
    
    strftime(Buffer, Size, "%Y-%m-%d %H:%M:%S", T);
}