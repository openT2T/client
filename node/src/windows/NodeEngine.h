#pragma once

namespace OpenT2T
{
    /// <summary>
    /// Event raised when a registered function is called by script.
    /// </summary>
    public ref class NodeCallEvent sealed
    {
    internal:
        NodeCallEvent(Platform::String^ functionName, Platform::String^ argsJson);

    public:
        /// <summary>
        /// Name of the function that was called by script.
        /// </summary>
        property Platform::String^ FunctionName
        {
            Platform::String^ get();
        }

        /// <summary>
        /// JSON-serialized array of arguments passed by the script.
        /// </summary>
        property Platform::String^ ArgsJson
        {
            Platform::String^ get();
        }

    private:
        Platform::String^ functionName;
        Platform::String^ argsJson;
    };

    /// <summary>
    /// APIs for interacting with a node engine hosted in the application.
    /// Wrapper around the cross-platform C++ INodeEngine interface.
    /// (See INodeEngine.h for interface documentation.)
    /// </summary>
    public ref class NodeEngine sealed
    {
    public:
        NodeEngine();
        virtual ~NodeEngine();

        void DefineScriptFile(Platform::String^ scriptFileName, Platform::String^ scriptCode);

        Windows::Foundation::IAsyncAction^ StartAsync(Platform::String^ workingDirectory);

        Windows::Foundation::IAsyncAction^ StopAsync();

        Windows::Foundation::IAsyncOperation<Platform::String^>^ CallScriptAsync(Platform::String^ scriptCode);

        void RegisterCallFromScript(Platform::String^ scriptFunctionName);

        event Windows::Foundation::EventHandler<NodeCallEvent^>^ CallFromScript;

    private:
        INodeEngine* node;
    };
}
