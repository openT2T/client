
namespace OpenT2T
{

/// Defines a minimal interface to a hosted Node.js engine required by OpenT2T.
/// Includes methods for initializing, starting, and stopping the Node.js environment,
/// as well as calling back and forth between C++ and JavaScript. For simplicity,
/// all marshalling of data between the layers is done as JSON. It is assumed that
/// any host will have a quality JSON library conveniently available. Some methods
/// are async, using a callback function to supply async results/exceptions.
/// Additionally, both sync and async methods may throw std::exceptions directly.
class INodeEngine
{
public:
    /// Injects a script file into the node engine. The script code in the file may later
    /// be executed by requiring the file name.
    virtual void DefineScriptFile(
        std::string scriptFileName, std::string scriptCode) = 0;

    /// Asynchronously starts the node engine, specifying the the working directory that
    /// node modules will be loaded relative to. The callback is invoked when starting
    /// completes; if starting failed, the callback exception argument is non-null.
    virtual void Start(
        std::string workingDirectory,
        std::function<void(std::exception_ptr ex)> callback) = 0;

    /// Asynchronously stops the node engine. The callback is invoked when stopping
    /// completes; if stopping failed, the callback exception argument is non-null.
    virtual void Stop(
        std::function<void(std::exception_ptr ex)> callback) = 0;

    /// Asynchronously evaluates JavaScript code in the node engine. The expression may
    /// require other modules defined via DefineScriptFile or loaded from files relative
    /// to the working directory. The callback is invoked with the result of the evaluation
    /// (in JSON format); if evaluation failed or threw an error, the callback exception
    /// argument is non-null. The callback exception includes the message property from
    /// the JavaScript error object, if any.
    virtual void CallScript(
        std::string scriptCode,
        std::function<void(std::string resultJson, std::exception_ptr ex)> callback) = 0;

    /// Registers a global callback function that can be invoked by JavaScript. The
    /// arguments passed to the callback function are formatted as a JSON array.
    virtual void RegisterCallFromScript(
        std::string scriptFunctionName,
        std::function<void(std::string argsJson)> callback) = 0;
};

}