
namespace OpenT2T
{

class JXCoreEngine : public INodeEngine
{
public:
    JXCoreEngine();

    const char* GetMainScriptFileName() override;

    void DefineScriptFile(const char* scriptFileName, const char* scriptCode) override;

    void Start(const char* workingDirectory, std::function<void(std::exception_ptr ex)> callback) override;

    void Stop(std::function<void(std::exception_ptr ex)> callback) override;

    void CallScript(
        const char* scriptCode,
        std::function<void(const char* resultJson, std::exception_ptr ex)> callback) override;

    void RegisterCallFromScript(
        const char* scriptFunctionName,
        std::function<void(const char* argsJson)> callback) override;
};

}