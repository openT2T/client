
namespace OpenT2T
{

// Defines a minimal interface to a hosted Node.js engine required by OpenT2T.
// Includes methods for initializing, starting, and stopping the Node.js environment,
// as well as calling back and forth between C++ and JavaScript. For simplicity,
// all marshalling of data between the layers is done as JSON. It is assumed that
// any host will have a quality JSON library conveniently available.
class INodeEngine
{
public:
    virtual const char* GetMainScriptFileName() = 0;

    virtual void DefineScriptFile(const char* scriptFileName, const char* scriptCode) = 0;

    virtual void Start(const char* workingDirectory, std::function<void(std::exception_ptr ex)> callback) = 0;

    virtual void Stop(std::function<void(std::exception_ptr ex)> callback) = 0;

    virtual void CallScript(
        const char* scriptCode,
        std::function<void(const char* resultJson, std::exception_ptr ex)> callback) = 0;

    virtual void RegisterCallFromScript(
        const char* scriptFunctionName,
        std::function<void(const char* argsJson)> callback) = 0;
};

}