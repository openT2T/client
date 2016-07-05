#pragma once

std::string PlatformStringToString(Platform::String^ utf16String)
{
    static stdext::cvt::wstring_convert<std::codecvt_utf8<wchar_t>> convert;
    return convert.to_bytes(utf16String->Data());
}

Platform::String^ StringToPlatformString(const char* utf8String)
{
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
    try
    {
        std::rethrow_exception(ex);
    }
    catch (Platform::Exception^ pex)
    {
        return pex;
    }
    catch (const std::out_of_range& stdex)
    {
        return ref new Platform::OutOfBoundsException(StringToPlatformString(stdex.what()));
    }
    catch (const std::length_error& stdex)
    {
        return ref new Platform::OutOfBoundsException(StringToPlatformString(stdex.what()));
    }
    catch (const std::invalid_argument& stdex)
    {
        return ref new Platform::InvalidArgumentException(StringToPlatformString(stdex.what()));
    }
    catch (const std::bad_cast& stdex)
    {
        return ref new Platform::InvalidCastException(StringToPlatformString(stdex.what()));
    }
    catch (const std::exception& stdex)
    {
        return ref new Platform::FailureException(StringToPlatformString(stdex.what()));
    }
    catch (...)
    {
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
