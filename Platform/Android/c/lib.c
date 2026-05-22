#include <jni.h>

#include "Core.h"

extern "C"
JNIEXPORT void JNICALL
Java_org_main_MainActivity_nativeInit(JNIEnv* env, jobject thiz)
{
    Log(LogLevel::Fatal, "An Unexpected Error Occurred!");
}
