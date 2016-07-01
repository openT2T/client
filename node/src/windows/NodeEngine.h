#pragma once

namespace OpenT2T
{
    public ref class NodeCallEvent sealed
    {
    internal:
        NodeCallEvent(Platform::String^ functionName, Platform::String^ argsJson);

    public:
        property Platform::String^ FunctionName
        {
            Platform::String^ get();
        }

        property Platform::String^ ArgsJson
        {
            Platform::String^ get();
        }

    private:
        Platform::String^ functionName;
        Platform::String^ argsJson;
    };

    public ref class NodeEngine sealed
    {
    public:
        NodeEngine();
        virtual ~NodeEngine();

        property Platform::String^ MainScriptFileName
        {
            Platform::String^ get();
        }

        void DefineScriptFile(Platform::String^ scriptFileName, Platform::String^ scriptCode);

        Windows::Foundation::IAsyncAction^ StartAsync(Platform::String^ workingDirectory);

        Windows::Foundation::IAsyncAction^ StopAsync();

        Windows::Foundation::IAsyncAction^ CallScriptAsync(Platform::String^ scriptCode);

        void RegisterCallFromScript(Platform::String^ scriptFunctionName);

        event Windows::Foundation::EventHandler<NodeCallEvent^>^ CallFromScript;

    private:
        INodeEngine* node;
    };
}
