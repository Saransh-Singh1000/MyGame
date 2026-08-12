#include <jni.h>

#include "../../Common/init.h"

JNIEXPORT void JNICALL
Java_org_main_MainActivity_nativeInit(JNIEnv* env, jobject thiz)
{
    (void)env;
    (void)thiz;

    init_window();
}
