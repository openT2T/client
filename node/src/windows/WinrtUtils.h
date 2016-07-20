#pragma once

std::string PlatformStringToString(Platform::String^ utf16String)
{
    static stdext::cvt::wstring_convert<std::codecvt_utf8<wchar_t>> convert;
    return convert.to_bytes(utf16String->Data());
}

Platform::String^ StringToPlatformString(const char* utf8String)
{
    if (utf8String == nullptr)
    {
        return nullptr;
    }

    static stdext::cvt::wstring_convert<std::codecvt_utf8<wchar_t>> convert;
    return ref new Platform::String(convert.from_bytes(utf8String).c_str());
}

Platform::String^ StringToPlatformString(const std::string& utf8String)
{
    static stdext::cvt::wstring_convert<std::codecvt_utf8<wchar_t>> convert;
    return ref new Platform::String(convert.from_bytes(utf8String).c_str());
}

// A non-Platform::Exception thrown across a WinRT boundary gets automatically
// converted to Platform::FailureException by default. This gives a slightly better mapping.
Platform::Exception^ ExceptionToPlatformException(std::exception_ptr ex)
{
    if (ex == nullptr)
    {
        return nullptr;
    }

    try
    {
        std::rethrow_exception(ex);
    }
    catch (Platform::Exception^ pex)
    {
        OpenT2T::LogTrace("ExceptionToPlatformException(%s)", PlatformStringToString(pex->Message).c_str());
        return pex;
    }
    catch (const std::out_of_range& stdex)
    {
        OpenT2T::LogTrace("ExceptionToPlatformException(\"%s\")", stdex.what());
        return ref new Platform::OutOfBoundsException(StringToPlatformString(stdex.what()));
    }
    catch (const std::length_error& stdex)
    {
        OpenT2T::LogTrace("ExceptionToPlatformException(\"%s\")", stdex.what());
        return ref new Platform::OutOfBoundsException(StringToPlatformString(stdex.what()));
    }
    catch (const std::invalid_argument& stdex)
    {
        OpenT2T::LogTrace("ExceptionToPlatformException(\"%s\")", stdex.what());
        return ref new Platform::InvalidArgumentException(StringToPlatformString(stdex.what()));
    }
    catch (const std::bad_cast& stdex)
    {
        OpenT2T::LogTrace("ExceptionToPlatformException(\"%s\")", stdex.what());
        return ref new Platform::InvalidCastException(StringToPlatformString(stdex.what()));
    }
    catch (const std::logic_error& stdex)
    {
        OpenT2T::LogTrace("ExceptionToPlatformException(\"%s\")", stdex.what());
        return Platform::Exception::CreateException(E_NOT_VALID_STATE, StringToPlatformString(stdex.what()));
    }
    catch (const std::runtime_error& stdex)
    {
        OpenT2T::LogTrace("ExceptionToPlatformException(\"%s\")", stdex.what());
        return ref new Platform::FailureException(StringToPlatformString(stdex.what()));
    }
    catch (const std::exception& stdex)
    {
        OpenT2T::LogTrace("ExceptionToPlatformException(\"%s\")", stdex.what());
        return ref new Platform::FailureException(StringToPlatformString(stdex.what()));
    }
    catch (...)
    {
        OpenT2T::LogTrace("ExceptionToPlatformException(unknown)");
        return ref new Platform::FailureException();
    }
}

template <typename T>
T ExceptionsToPlatformExceptions(std::function<T()> func)
{
    try
    {
        return func();
    }
    catch (...)
    {
        throw ExceptionToPlatformException(std::current_exception());
    }
}

Windows::Foundation::IAsyncAction^ TaskToAsyncAction(concurrency::task<void> task)
{
    return concurrency::create_async([task]()
    {
        return task.then([](concurrency::task<void> completedTask)
        {
            try
            {
                completedTask.get();
            }
            catch (...)
            {
                throw ExceptionToPlatformException(std::current_exception());
            }
        });
    });
}

template <typename T>
Windows::Foundation::IAsyncOperation<T>^ TaskToAsyncOperation(concurrency::task<T> task)
{
    return concurrency::create_async([task]()
    {
        return task.then([](concurrency::task<T> completedTask)
        {
            try
            {
                return completedTask.get();
            }
            catch (...)
            {
                throw ExceptionToPlatformException(std::current_exception());
            }
        });
    });
}
