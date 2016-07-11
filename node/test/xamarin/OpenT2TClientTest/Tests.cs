using System;
using System.Threading.Tasks;

using OpenT2T;

namespace OpenT2T.Test
{
    public class Tests : TestClass
    {
        public Tests(TestLogView log) : base(log)
        {
        }

        public async void Run()
        {
            log.Clear();

            await Task.Run(async () =>
            {
                try
                {
                    await TestNodeEngine();

                    Log("");
                    LogImportant("RESULTS: " + passCount + " passed, " + failCount + " failed.");
                }
                catch (Exception ex)
                {
                    LogError("ERROR: " + ex.ToString());
                }
            });
        }

        async Task TestNodeEngine()
        {
            Log("");
            Log("Testing node engine...");

            NodeEngine node = null;

            currentTest = "new NodeEngine()";
            Expect(() => node = new NodeEngine());

            currentTest = "NodeEngine.StartAsync()";
            await ExpectAsync(() => node.StartAsync("."));

            currentTest = "NodeEngine.CallScriptAsync()";
            string message = "\"Hello from node!\"";
            await ExpectResultAsync(message, () => node.CallScriptAsync("(function () { return " + message + "; })()"));

            currentTest = "NodeEngine.RegisterCallFromScript()";
            string args = null;
            node.CallFromScript += (source, e) => { args = e.ArgsJson; };
            Expect(() => node.RegisterCallFromScript("testTest"));
            await ExpectAsync(() => node.CallScriptAsync("testTest(null, 0, 'test');"));
            ExpectResult("[null,0,\"test\"]", () => args);

            currentTest = "NodeEngine.CallScriptAsync(error)";
            await ExpectExceptionAsync(() => node.CallScriptAsync("throw new Error('test');"));
        }
    }
}
