#include <jni.h>

#include "api.hpp"

extern "C"
JNIEXPORT void JNICALL
Java_org_main_MainActivity_nativeInit(JNIEnv* env, jobject thiz)
{
    Log("Hello World");
}