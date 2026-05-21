#include <jni.h>

#include "Core.hpp"

extern "C"
JNIEXPORT void JNICALL
Java_org_main_MainActivity_nativeInit(JNIEnv* env, jobject thiz)
{
    Log(LogLevel::Info, "Hello World");
}