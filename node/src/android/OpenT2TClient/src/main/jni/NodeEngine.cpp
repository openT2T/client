// Native methods for the NodeEngine Java class

#include <cstdlib>
#include <exception>
#include <functional>
#include <memory>
#include <mutex>
#include <queue>
#include <stdexcept>
#include <string>
#include <thread>
#include <unordered_map>

#include <jni.h>
#include <android/log.h>

#include "Log.h"
#include "AsyncQueue.h"
#include "WorkItemDispatcher.h"
#include "INodeEngine.h"
#include "NodeCoreEngine.h"
#include "JniUtils.h"

using namespace OpenT2T;

void setNodeEngine(JNIEnv* env, jobject thiz, INodeEngine* nodeEngine)
{
    jclass thisClass = env->GetObjectClass(thiz);
    jfieldID nodeField = env->GetFieldID(thisClass, "node", "J");
    env->SetLongField(thiz, nodeField, reinterpret_cast<long>(nodeEngine));
}

INodeEngine* getNodeEngine(JNIEnv* env, jobject thiz)
{
    jclass thisClass = env->GetObjectClass(thiz);
    jfieldID nodeField = env->GetFieldID(thisClass, "node", "J");
    INodeEngine* nodeEngine = reinterpret_cast<INodeEngine*>(env->GetLongField(thiz, nodeField));

    if (nodeEngine == nullptr)
    {
        env->ThrowNew(
                env->FindClass("java/lang/IllegalStateException"),
                "Node engine not initialized");
    }

    return nodeEngine;
}

JavaVM* jvm;

extern "C" {

jint JNI_OnLoad(JavaVM* vm, void* reserved)
{
    jvm = vm;
    return JNI_VERSION_1_4;
}

JNIEXPORT void JNICALL Java_io_opent2t_NodeEngine_staticInit(
        JNIEnv* env, jobject thiz)
{
#if DEBUG
    OpenT2T::logLevel = LogSeverity::Trace;
#else
    OpenT2T::logLevel = LogSeverity::Info;
#endif

    OpenT2T::logHandler = [](LogSeverity severity, const char* message)
    {
        const char* logTag = "OpenT2T.NodeEngine.JNI";

        int androidLogLevel;
        switch (severity)
        {
            case LogSeverity::Error:
                androidLogLevel = ANDROID_LOG_ERROR;
                break;
            case LogSeverity::Warning:
                androidLogLevel = ANDROID_LOG_WARN;
                break;
            case LogSeverity::Info:
                androidLogLevel = ANDROID_LOG_INFO;
                break;
            case LogSeverity::Verbose:
            case LogSeverity::Trace:
            default:
                androidLogLevel = ANDROID_LOG_ERROR;
                break;
        }

        __android_log_write(androidLogLevel, logTag, message);
    };
}

JNIEXPORT void JNICALL Java_io_opent2t_NodeEngine_init(
        JNIEnv* env, jobject thiz)
{
    LogTrace("init()");

    INodeEngine* nodeEngine = new NodeCoreEngine();
    setNodeEngine(env, thiz, nodeEngine);
}

JNIEXPORT void JNICALL Java_io_opent2t_NodeEngine_defineScriptFile(
    JNIEnv* env, jobject thiz, jstring scriptFileName, jstring scriptCode)
{
    const char* scriptFileNameChars = env->GetStringUTFChars(scriptFileName, JNI_FALSE);
    const char* scriptCodeChars = env->GetStringUTFChars(scriptCode, JNI_FALSE);
    LogTrace("defineScriptFile(\"%s\", \"...\")", scriptFileNameChars);

    INodeEngine* nodeEngine = getNodeEngine(env, thiz);
    if (nodeEngine != nullptr)
    {
        try
        {
            nodeEngine->DefineScriptFile(scriptFileNameChars, scriptCodeChars);
            LogTrace("defineScriptFile succeeded");
        }
        catch (...)
        {
            LogError("defineScriptFile failed");
            env->Throw(exceptionToJavaException(env, std::current_exception()));
        }
    }

    env->ReleaseStringUTFChars(scriptFileName, scriptFileNameChars);
    env->ReleaseStringUTFChars(scriptCode, scriptCodeChars);
}

JNIEXPORT void JNICALL Java_io_opent2t_NodeEngine_start(
    JNIEnv* env, jobject thiz, jobject promise, jstring workingDirectory)
{
    const char* workingDirectoryChars = env->GetStringUTFChars(workingDirectory, JNI_FALSE);
    LogTrace("start(\"%s\")", workingDirectoryChars);

    INodeEngine* nodeEngine = getNodeEngine(env, thiz);
    if (nodeEngine != nullptr)
    {
        promise = env->NewGlobalRef(promise);
        try
        {
            nodeEngine->Start(workingDirectoryChars, [=](std::exception_ptr ex)
            {
                JNIEnv* env;
                jvm->AttachCurrentThread(&env, nullptr);

                if (ex == nullptr)
                {
                    LogTrace("start succeeded");
                    resolvePromise(env, promise, nullptr);
                }
                else
                {
                    LogError("start failed");
                    rejectPromise(
                            env,
                            promise,
                            exceptionToJavaException(env, ex));
                }

                env->DeleteGlobalRef(promise);
                jvm->DetachCurrentThread();
            });
        }
        catch (...)
        {
            LogError("start failed");
            env->Throw(exceptionToJavaException(env, std::current_exception()));
            env->DeleteGlobalRef(promise);
        }
    }

    env->ReleaseStringUTFChars(workingDirectory, workingDirectoryChars);
}

JNIEXPORT void JNICALL Java_io_opent2t_NodeEngine_stop(
    JNIEnv* env, jobject thiz, jobject promise)
{
    LogTrace("stop()");

    INodeEngine* nodeEngine = getNodeEngine(env, thiz);
    if (nodeEngine != nullptr)
    {
        promise = env->NewGlobalRef(promise);
        try
        {
            nodeEngine->Stop([=](std::exception_ptr ex)
            {
                JNIEnv* env;
                jvm->AttachCurrentThread(&env, nullptr);

                if (ex == nullptr)
                {
                    LogTrace("stop succeeded");
                    resolvePromise(env, promise, nullptr);
                }
                else
                {
                    LogError("stop failed");
                    rejectPromise(
                            env,
                            promise,
                            exceptionToJavaException(env, ex));
                }

                env->DeleteGlobalRef(promise);
                jvm->DetachCurrentThread();
            });
        }
        catch (...)
        {
            LogError("stop failed");
            env->Throw(exceptionToJavaException(env, std::current_exception()));
            env->DeleteGlobalRef(promise);
        }
    }
}

JNIEXPORT void JNICALL Java_io_opent2t_NodeEngine_callScript(
    JNIEnv* env, jobject thiz, jobject promise, jstring scriptCode)
{
    const char* scriptCodeChars = env->GetStringUTFChars(scriptCode, JNI_FALSE);
    LogTrace("callScript(\"%s\")", scriptCodeChars);

    INodeEngine* nodeEngine = getNodeEngine(env, thiz);
    if (nodeEngine != nullptr)
    {
        promise = env->NewGlobalRef(promise);
        try
        {
            nodeEngine->CallScript(scriptCodeChars,
                [=](std::string resultJson, std::exception_ptr ex)
            {
                JNIEnv* env;
                jvm->AttachCurrentThread(&env, nullptr);

                if (ex == nullptr)
                {
                    LogTrace("callScript succeeded");
                    jstring resultJsonString = env->NewStringUTF(resultJson.c_str());
                    resolvePromise(env, promise, resultJsonString);
                }
                else
                {
                    LogError("callScript failed");
                    rejectPromise(
                            env,
                            promise,
                            exceptionToJavaException(env, ex));
                }

                env->DeleteGlobalRef(promise);
                jvm->DetachCurrentThread();
            });
        }
        catch (...)
        {
            LogError("callScript failed");
            env->Throw(exceptionToJavaException(env, std::current_exception()));
            env->DeleteGlobalRef(promise);
        }
    }

    env->ReleaseStringUTFChars(scriptCode, scriptCodeChars);
}

JNIEXPORT void JNICALL Java_io_opent2t_NodeEngine_registerCallFromScript(
    JNIEnv* env, jobject thiz, jstring scriptFunctionName)
{
    const char* scriptFunctionNameChars = env->GetStringUTFChars(scriptFunctionName, JNI_FALSE);
    LogTrace("registerCallFromScript(\"%s\")", scriptFunctionNameChars);

    INodeEngine* nodeEngine = getNodeEngine(env, thiz);
    if (nodeEngine != nullptr)
    {
        scriptFunctionName = reinterpret_cast<jstring>(env->NewGlobalRef(scriptFunctionName));
        thiz = env->NewGlobalRef(thiz);
        try
        {
            nodeEngine->RegisterCallFromScript(scriptFunctionNameChars, [=](std::string argsJson)
            {
                JNIEnv* env;
                jvm->AttachCurrentThread(&env, nullptr);

                const char* scriptFunctionNameChars =
                        env->GetStringUTFChars(scriptFunctionName, JNI_FALSE);
                LogTrace("callFromScript(\"%s\")", scriptFunctionNameChars);
                env->ReleaseStringUTFChars(scriptFunctionName, scriptFunctionNameChars);

                jclass thisClass = env->GetObjectClass(thiz);
                jmethodID raiseCallFromScriptMethod = env->GetMethodID(
                    thisClass, "raiseCallFromScript", "(Ljava/lang/String;Ljava/lang/String;)V");
                jstring argsJsonString = env->NewStringUTF(argsJson.c_str());
                env->CallVoidMethod(
                        thiz, raiseCallFromScriptMethod, scriptFunctionName, argsJsonString);
                if (env->ExceptionOccurred())
                {
                    LogError("raiseCallFromScript threw exception");
                    env->ExceptionClear();
                }

                jvm->DetachCurrentThread();
            });
        }
        catch (...)
        {
            LogError("registerCallFromScript failed");
            env->Throw(exceptionToJavaException(env, std::current_exception()));
            env->DeleteGlobalRef(scriptFunctionName);
        }
    }

    LogTrace("registerCallFromScript succeeded");
    env->ReleaseStringUTFChars(scriptFunctionName, scriptFunctionNameChars);
}

}
