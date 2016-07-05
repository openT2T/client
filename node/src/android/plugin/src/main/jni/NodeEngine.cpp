
#include <functional>
#include <exception>
#include <stdexcept>
#include <jni.h>
#include <android/log.h>

#include "Log.h"
#include "INodeEngine.h"
#include "JXCoreEngine.h"
#include "JniUtils.h"

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
        env->ThrowNew(env->FindClass("java/lang/IllegalStateException"));
    }

    return nodeEngine;
}

extern "C" {

jint JNI_OnLoad(JavaVM *vm, void *reserved)
{
    return JNI_VERSION_1_4;
}

JNIEXPORT void JNICALL Java_io_opent2t_NodeEngine_staticInit(
        JNIEnv* env, jobject thiz)
{
    SetLogLevel(BuildConfig.DEBUG ? LogSeverity::Trace : LogSeverity.Info);

    SetLogHandler([](LogSeverity severity, const char* message)
    {
        const char* logTag = "OpenT2T.NodeEngine.JNI"

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

        __android_log_print(androidLogLevel, logTag, message);
    });
}

JNIEXPORT void JNICALL Java_io_opent2t_NodeEngine_init(
        JNIEnv* env, jobject thiz)
{
    LogTrace("init()");

    INodeEngine* nodeEngine = new JXCoreEngine();
    setNodeEngine(env, thiz, nodeEngine);
}

JNIEXPORT jstring JNICALL Java_io_opent2t_NodeEngine_getMainScriptFileName(
    JNIEnv* env, jobject thiz)
{
    LogTrace("getMainScriptFileName()");

    INodeEngine* nodeEngine = getNodeEngine(env, thiz);
    if (nodeEngine == nullptr)
    {
        return nullptr;
    }

    jstring mainScriptFileName = nullptr;
    try
    {
        const char* mainScriptFileNameChars = nodeEngine->GetMainScriptFileName();
        mainScriptFileName = env->NewStringUTF(mainScriptFileNameChars);
    }
    catch (...)
    {
        env->Throw(exceptionToJavaException(std::current_exception());
    }

    return mainScriptFileName;
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
        }
        catch (...)
        {
            env->Throw(exceptionToJavaException(std::current_exception());
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
        try
        {
            nodeEngine->Start(workingDirectoryChars, [](std::exception_ptr ex)
            {
                if (ex == nullptr)
                {
                    resolvePromise(env, promise, nullptr);
                }
                else
                {
                    rejectPromise(env, promise, exceptionToJavaException(std::current_exception());
                }
            });
        }
        catch (...)
        {
            env->Throw(exceptionToJavaException(std::current_exception());
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
        try
        {
            nodeEngine->Stop([](std::exception_ptr ex)
            {
                if (ex == nullptr)
                {
                    resolvePromise(env, promise, nullptr);
                }
                else
                {
                    rejectPromise(env, promise, exceptionToJavaException(std::current_exception());
                }
            });
        }
        catch (...)
        {
            env->Throw(exceptionToJavaException(std::current_exception());
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
        try
        {
            nodeEngine->CallScript(scriptCodeChars,
                [](const char* resultJson, std::exception_ptr ex)
            {
                if (ex == nullptr)
                {
                    jstring resultJsonString = env->NewStringUTF(resultJson);
                    resolvePromise(env, promise, resultJsonString);
                }
                else
                {
                    rejectPromise(env, promise, exceptionToJavaException(std::current_exception());
                }
            });
        }
        catch (...)
        {
            env->Throw(exceptionToJavaException(std::current_exception());
        }
    }

    env->ReleaseStringUTFChars(scriptCode, scriptCodeChars);

    resolvePromise(env, promise, nullptr);
}

JNIEXPORT void JNICALL Java_io_opent2t_NodeEngine_registerCallFromScript(
    JNIEnv* env, jobject thiz, jstring scriptFunctionName)
{
    const char* scriptFunctionNameChars = env->GetStringUTFChars(scriptFunctionName, JNI_FALSE);

    LogTrace("registerCallFromScript(\"%s\")", scriptFunctionNameChars);

    INodeEngine* nodeEngine = getNodeEngine(env, thiz);
    if (nodeEngine != nullptr)
    {
        try
        {
            nodeEngine->RegisterCallFromScript(scriptFunctionNameChars, [=](const char* argsJson)
            {
                jclass thisClass = env->GetObjectClass(thiz);
                jmethodID raiseCallFromScriptMethod = env->GetMethodID(
                    thisClass, "raiseCallFromScript", "(Ljava/lang/String;Ljava/lang/String;)V");
                jstring argsJsonString = env->NewStringUTF(argsJson);
                env->CallVoidMethod(thiz, raiseCallFromScriptMethod, scriptFunctionName, argsJsonString);
            });
        }
        catch (...)
        {
            env->Throw(exceptionToJavaException(std::current_exception());
        }
    }

    env->ReleaseStringUTFChars(scriptFunctionName, scriptFunctionNameChars);
}

}