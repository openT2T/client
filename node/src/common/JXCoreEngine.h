
namespace OpenT2T
{

class JXCoreEngine : public INodeEngine
{
public:
    JXCoreEngine();
    ~JXCoreEngine();

    void DefineScriptFile(std::string scriptFileName, std::string scriptCode) override;

    void Start(std::string workingDirectory, std::function<void(std::exception_ptr ex)> callback) override;

    void Stop(std::function<void(std::exception_ptr ex)> callback) override;

    void CallScript(
        std::string scriptCode,
        std::function<void(std::string resultJson, std::exception_ptr ex)> callback) override;

    void RegisterCallFromScript(
        std::string scriptFunctionName,
        std::function<void(std::string argsJson)> callback) override;

private:
    static std::once_flag _initOnce;
    WorkItemDispatcher _dispatcher;
    std::unordered_map<std::string, std::string> _initialScriptMap;
    bool _started;
    void* _callScriptFunction;
};

}