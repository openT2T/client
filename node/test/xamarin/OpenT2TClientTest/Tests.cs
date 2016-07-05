using System;
using System.Collections;
using System.Collections.Generic;
using System.Diagnostics;
using System.Linq;
using System.Reflection;
using System.Threading.Tasks;
using Xamarin.Forms;

using OpenT2T;

namespace OpenT2T.Test
{
    public class Tests
    {
        TestLogView log;
        int passCount = 0;
        int failCount = 0;
        string currentTest;

        public Tests(TestLogView log)
        {
            this.log = log;
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

            currentTest = "NodeEngine.CallScriptAsync()";
            string message = "\"Hello from node!\"";
            await ExpectResultAsync(message, () => node.CallScriptAsync("function () { return " + message + "; }()"));
        }

        void Expect(Action action)
        {
            try
            {
                action();
            }
            catch (Exception ex)
            {
                Assert(false, GetExceptionMessage(ex));
                return;
            }

            Assert(true, "(no exception)");
        }

        async Task ExpectAsync(Func<Task> action)
        {
            try
            {
                await action();
            }
            catch (Exception ex)
            {
                Assert(false, GetExceptionMessage(ex));
                return;
            }

            Assert(true, "(no exception)");
        }

        void ExpectResult<T>(T expected, Func<T> action)
        {
            T result;
            try
            {
                result = action();
            }
            catch (Exception ex)
            {
                Assert(false, GetExceptionMessage(ex));
                return;
            }

            AssertEquals(expected, result);
        }

        void ExpectResult<T>(Func<T> action, Func<T, bool> test)
        {
            T result;
            try
            {
                result = action();
            }
            catch (Exception ex)
            {
                Assert(false, GetExceptionMessage(ex));
                return;
            }

            Assert(test(result), typeof(T).Name);
        }

        async Task ExpectResultAsync<T>(T expected, Func<Task<T>> action)
        {
            T result;
            try
            {
                result = await action();
            }
            catch (Exception ex)
            {
                Assert(false, GetExceptionMessage(ex));
                return;
            }

            AssertEquals(expected, result);
        }

        async Task ExpectResultAsync<T>(Func<Task<T>> action, Func<T, bool> test)
        {
            T result;
            try
            {
                result = await action();
            }
            catch (Exception ex)
            {
                Assert(false, GetExceptionMessage(ex));
                return;
            }

            Assert(test(result), typeof(T).Name);
        }

        void ExpectException(Action action)
        {
            try
            {
                action();
            }
            catch (Exception ex)
            {
                Assert(true, GetExceptionMessage(ex));
                return;
            }

            Assert(false, "Expected exception but none was thrown.");
        }

        async Task ExpectExceptionAsync(Func<Task> action)
        {
            try
            {
                await action();
            }
            catch (Exception ex)
            {
                Assert(true, GetExceptionMessage(ex));
                return;
            }

            Assert(false, "Expected exception but none was thrown.");
        }

        void AssertEquals<T>(T expected, T actual)
        {
            if (expected is IList)
            {
                if (actual is IList)
                {
                    IList expectedList = (IList)(Object)expected;
                    IList actualList = (IList)(Object)actual;
                    bool listsAreEqual = (((actualList == null) == (expectedList == null)) &&
                        (expectedList == null || (actualList.Count == expectedList.Count &&
                        actualList.Cast<Object>().SequenceEqual(expectedList.Cast<Object>()))));
                    if (listsAreEqual)
                    {
                        Assert(true, "[" + String.Join(", ", actualList.Cast<Object>()) + "]");
                    }
                    else
                    {
                        Assert(false, "actual: " + "[" + String.Join(", ", actualList.Cast<Object>()) + "]" + ", " +
                            "expected: " + "[" + String.Join(", ", expectedList.Cast<Object>()) + "]");
                    }
                }
                else
                {
                    Assert(false, "actual: " + (actual == null ? "(null)" : actual.GetType().Name) + ", " +
                        "expected: IList");
                }
            }
            else
            {
                if (Object.Equals(actual, expected))
                {
                    Assert(true, actual.ToString());
                }
                else
                {
                    Assert(false, "actual: " + actual + ", expected: " + expected);
                }
            }
        }

        void Assert(bool assertion, string assertionString)
        {
            if (assertion)
            {
                LogSuccess("PASSED: " + currentTest + " - " + assertionString);
                passCount++;
            }
            else
            {
                LogError("FAILED: " + currentTest + " - " + assertionString);
                failCount++;
            }
        }

        void Log(string message)
        {
            Debug.WriteLine(message);
            this.log.AppendLine(message);
        }

        void LogImportant(string message)
        {
            Debug.WriteLine(message);
            this.log.AppendLine(message, Color.Black, true);
        }

        void LogSuccess(string message)
        {
            Debug.WriteLine(message);
            this.log.AppendLine(message, Color.Green, false);
        }

        void LogError(string message)
        {
            Debug.WriteLine(message);
            this.log.AppendLine(message, Color.Red, false);
        }

        static string GetExceptionMessage(Exception ex)
        {
            if (ex.GetType().Name == "NSErrorException")
            {
                Object nserror = null;
                TypeInfo nserrorExceptionType = ex.GetType().GetTypeInfo();
                PropertyInfo errorProperty = nserrorExceptionType.GetDeclaredProperty("Error");
                if (errorProperty != null)
                {
                    nserror = errorProperty.GetValue(ex);
                }
                else
                {
                    // This seems to be a bug in the Xamarin runtime.
                    // Error is a field instead of a property on iOS.
                    FieldInfo errorField = nserrorExceptionType.GetDeclaredField("error");
                    if (errorField != null)
                    {
                        nserror = errorField.GetValue(ex);
                    }
                }

                return "NSError: " + nserror;
            }
            else
            {
                return ex.GetType().Name + ": " + ex.Message;
            }
        }
    }
}
