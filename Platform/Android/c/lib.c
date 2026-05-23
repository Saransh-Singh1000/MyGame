#include <jni.h>

#include "Core.h"

JNIEXPORT void JNICALL
Java_org_main_MainActivity_nativeInit(JNIEnv* env, jobject thiz)
{
    Log(Debug, "Hello, World!");
    Log(Info, "Hello, World!");
    Log(Warning, "Hello, World!");
    Log(Error, "Hello, World!");
    Log(Fatal, "Hello, World!");
}
