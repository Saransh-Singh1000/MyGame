#include <jni.h>

#include "Core.h"

JNIEXPORT void JNICALL
Java_org_main_MainActivity_nativeInit(JNIEnv* env, jobject thiz)
{
    Log(Fatal, "An Unexpected Error Occurred!");
}
