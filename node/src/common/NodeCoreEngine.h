
namespace OpenT2T
{

/// Implementation of the INodeEngine interface using the Node hosting APIs.
class NodeCoreEngine : public INodeEngine
{
public:
    NodeCoreEngine();
    ~NodeCoreEngine();

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
    void CallScriptInternal(
        std::string scriptCode,
        std::function<void(std::string resultJson, std::exception_ptr ex)> callback);

    void RegisterCallFromScriptInternal(
        std::string scriptFunctionName,
        std::function<void(std::string argsJson)> callback);

    /// Tracks whether Node's one-time initialization has been invoked.
    static std::once_flag _initOnce;

    /// Working directory used for the one-time initialization.
    static std::string _workingDirectory;

    /// Dispatches calls to a thread dedicated to the Node engine instance.
    WorkItemDispatcher _dispatcher;

    /// Tracks script files that are defined before the engine is started.
    std::unordered_map<std::string, std::string> _initialScriptMap;

    /// Tracks call-from-script functions that are registered before the engine is started.
    std::unordered_map<std::string, std::function<void(std::string)>> _initialCallFromScriptMap;

    /// Tracks whether the engine has been started.
    bool _started;

    /// Pointer to a JS_Value representing a JavaScript function used to evaluate script code in the engine.
    void* _callScriptFunction;
};

}
