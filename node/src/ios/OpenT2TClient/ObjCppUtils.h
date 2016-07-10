
void ExceptionToNSError(std::exception_ptr ex, NSError** outError)
{
    if (ex != nullptr)
    {
        // TODO: Use other domains/error codes for some exceptions.
        NSString* domain = @"OpenT2T";
        NSInteger code = 1;
        NSString* description = nil;

        try
        {
            std::rethrow_exception(ex);
        }
        catch (const std::out_of_range& stdex)
        {
            description = [NSString stringWithUTF8String: stdex.what()];
        }
        catch (const std::length_error& stdex)
        {
            description = [NSString stringWithUTF8String: stdex.what()];
        }
        catch (const std::invalid_argument& stdex)
        {
            description = [NSString stringWithUTF8String: stdex.what()];
        }
        catch (const std::bad_cast& stdex)
        {
            description = [NSString stringWithUTF8String: stdex.what()];
        }
        catch (const std::logic_error& stdex)
        {
            description = [NSString stringWithUTF8String: stdex.what()];
        }
        catch (const std::exception& stdex)
        {
            description = [NSString stringWithUTF8String: stdex.what()];
        }
        catch (...)
        {
        }

        NSDictionary* userInfo;
        if (description != nil)
        {
            OpenT2T::LogTrace("ExceptionToNSError(\"%s\")", description.UTF8String);
            userInfo = @{ NSLocalizedDescriptionKey : description };
        }
        else
        {
            OpenT2T::LogTrace("ExceptionToNSError(\"\")");
            userInfo = @{};
        }

        if (outError != nil)
        {
            *outError = [[NSError alloc] initWithDomain: domain code: code userInfo: userInfo];
        }
    }
}

bool ValidateArgumentNotNull(const char* methodName, const char* argName, NSObject* arg, NSError** outError)
{
    if (arg == nil)
    {
        std::string message = std::string() +
            "Invalid argument: method '" + methodName + "' argument '" + argName + "' may not be null.";
        OpenT2T::LogError(message.c_str());
        if (outError)
        {
            ExceptionToNSError(std::make_exception_ptr(std::invalid_argument(message)), outError);
        }
        return false;
    }

    return true;
}
