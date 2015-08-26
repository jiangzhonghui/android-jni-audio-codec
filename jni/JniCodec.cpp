#include <stdio.h>

#include "jni.h"

extern int registerAmrCodec(JNIEnv *env);
extern int registerG711Codec(JNIEnv *env);

jint JNI_OnLoad(JavaVM *vm, void *unused)
{
    JNIEnv *env = NULL;
    vm->GetEnv((void **)&env, JNI_VERSION_1_4);
	
	registerAmrCodec(env);
	registerG711Codec(env);	
	
    return JNI_VERSION_1_4;
}
