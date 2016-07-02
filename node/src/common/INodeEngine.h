
namespace OpenT2T
{

class INodeEngine
{
public:
    virtual const char* GetMainScriptFileName() = 0;

    virtual void DefineScriptFile(const char* scriptFileName, const char* scriptCode) = 0;

    virtual void Start(const char* workingDirectory, std::function<void(std::exception_ptr ex)> callback) = 0;

    virtual void Stop(std::function<void(std::exception_ptr ex)> callback) = 0;

    virtual void CallScript(const char* scriptCode, std::function<void(std::exception_ptr ex)> callback) = 0;

    virtual void RegisterCallFromScript(
        const char* scriptFunctionName,
        std::function<void(const char* argsJson)> callback) = 0;
};

}