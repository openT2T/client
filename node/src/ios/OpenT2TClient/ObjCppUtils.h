
void ExceptionToNSError(std::exception_ptr ex, NSError** outError)
{
    if (outError)
    {
        // TODO: Use other domains/error codes for some exceptions.
        try
        {
            std::rethrow_exception(ex);
        }
        catch (const std::out_of_range& stdex)
        {
            NSDictionary* userInfo = @{NSLocalizedDescriptionKey : [NSString stringWithUTF8String: stdex.what()]};
            *outError = [[NSError alloc] initWithDomain: @"OpenT2T" code: 1 userInfo: userInfo];
        }
        catch (const std::length_error& stdex)
        {
            NSDictionary* userInfo = @{NSLocalizedDescriptionKey : [NSString stringWithUTF8String: stdex.what()]};
            *outError = [[NSError alloc] initWithDomain: @"OpenT2T" code: 1 userInfo: userInfo];
        }
        catch (const std::invalid_argument& stdex)
        {
            NSDictionary* userInfo = @{NSLocalizedDescriptionKey : [NSString stringWithUTF8String: stdex.what()]};
            *outError = [[NSError alloc] initWithDomain: @"OpenT2T" code: 1 userInfo: userInfo];
        }
        catch (const std::bad_cast& stdex)
        {
            NSDictionary* userInfo = @{NSLocalizedDescriptionKey : [NSString stringWithUTF8String: stdex.what()]};
            *outError = [[NSError alloc] initWithDomain: @"OpenT2T" code: 1 userInfo: userInfo];
        }
        catch (const std::logic_error& stdex)
        {
            NSDictionary* userInfo = @{NSLocalizedDescriptionKey : [NSString stringWithUTF8String: stdex.what()]};
            *outError = [[NSError alloc] initWithDomain: @"OpenT2T" code: 1 userInfo: userInfo];
        }
        catch (const std::exception& stdex)
        {
            NSDictionary* userInfo = @{NSLocalizedDescriptionKey : [NSString stringWithUTF8String: stdex.what()]};
            *outError = [[NSError alloc] initWithDomain: @"OpenT2T" code: 1 userInfo: userInfo];
        }
        catch (...)
        {
            NSDictionary* userInfo = @{};
            *outError = [[NSError alloc] initWithDomain: @"OpenT2T" code: 1 userInfo: userInfo];
        }
    }
}