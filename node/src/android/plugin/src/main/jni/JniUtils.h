
void resolvePromise(JNIEnv* env, jobject promise, jobject result)
{
    jclass promiseClass = env->GetObjectClass(promise);
    jmethodID resolveMethod = env->GetMethodID(promiseClass, "resolve", "(Ljava/lang/Object;)V");
    env->CallVoidMethod(promise, resolveMethod, result);
}

void rejectPromise(JNIEnv* env, jobject promise, jthrowable ex)
{
    jclass promiseClass = env->GetObjectClass(promise);
    jmethodID rejectMethod = env->GetMethodID(promiseClass, "reject", "(Ljava/lang/Exception;)V");
    env->CallVoidMethod(promise, rejectMethod, ex);
}

jthrowable newJavaException(JNIEnv* env, const char* exceptionClassName, const char* message)
{
    jthrowable javaException;
    jclass exceptionClass = env->FindClass(exceptionClassName);
    if (message == nullptr)
    {
        jmethodID exceptionConstructor =
            env->GetMethodID(exceptionClass, "<init>", "()V");
        javaException = static_cast<jthrowable>(
            env->NewObject(exceptionClass, exceptionConstructor));
    }
    else
    {
        jmethodID exceptionConstructor =
            env->GetMethodID(exceptionClass, "<init>", "(java/lang/String)V");
        jstring messageString = env->NewStringUTF(message);
        javaException = static_cast<jthrowable>(
            env->NewObject(exceptionClass, exceptionConstructor, messageString));
    }

    return javaException;
}

jthrowable exceptionToJavaException(JNIEnv* env, std::exception_ptr ex)
{
    try
    {
        std::rethrow_exception(ex);
    }
    catch (const std::out_of_range& stdex)
    {
        return newJavaException(env, "java/lang/IndexOutOfBoundsException", stdex.what());
    }
    catch (const std::length_error& stdex)
    {
        return newJavaException(env, "java/lang/IndexOutOfBoundsException", stdex.what());
    }
    catch (const std::invalid_argument& stdex)
    {
        return newJavaException(env, "java/lang/IllegalArgumentException", stdex.what());
    }
    catch (const std::bad_cast& stdex)
    {
        return newJavaException(env, "java/lang/ClassCastException", stdex.what());
    }
    catch (const std::logic_error& stdex)
    {
        return newJavaException(env, "java/lang/IllegalStateException", stdex.what());
    }
    catch (const std::exception& stdex)
    {
        return newJavaException(env, "java/lang/Exception", stdex.what());
    }
    catch (...)
    {
        return newJavaException(env, "java/lang/Exception", nullptr);
    }
}
