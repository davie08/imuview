#include <jni.h>
#include <string>

extern "C" JNIEXPORT jstring JNICALL
Java_com_toynext_imuview_MainActivity_getVersion(
        JNIEnv* env,
        jobject /* this */) {
    std::string hello = "Hello from C++";
    return env->NewStringUTF(hello.c_str());
}