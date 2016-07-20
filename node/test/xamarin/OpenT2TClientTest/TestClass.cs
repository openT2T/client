using System;
using System.Collections;
using System.Diagnostics;
using System.Linq;
using System.Reflection;
using System.Threading.Tasks;
using Xamarin.Forms;

namespace OpenT2T.Test
{
    public class TestClass
    {
        protected TestLogView log;
        protected int passCount = 0;
        protected int failCount = 0;
        protected string currentTest;

        public TestClass(TestLogView log)
        {
            this.log = log;
        }

        public void Expect(Action action)
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

        public async Task ExpectAsync(Func<Task> action)
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

        public void ExpectResult<T>(T expected, Func<T> action)
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

        public void ExpectResult<T>(Func<T> action, Func<T, bool> test)
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

        public async Task ExpectResultAsync<T>(T expected, Func<Task<T>> action)
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

        public async Task ExpectResultAsync<T>(Func<Task<T>> action, Func<T, bool> test)
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

        public void ExpectException(Action action)
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

        public async Task ExpectExceptionAsync(Func<Task> action)
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

        public void AssertEquals<T>(T expected, T actual)
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

        public void Assert(bool assertion, string assertionString)
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

        public void Log(string message)
        {
            Debug.WriteLine(message);
            this.log.AppendLine(message);
        }

        public void LogImportant(string message)
        {
            Debug.WriteLine(message);
            this.log.AppendLine(message, Color.Black, true);
        }

        public void LogSuccess(string message)
        {
            Debug.WriteLine(message);
            this.log.AppendLine(message, Color.Green, false);
        }

        public void LogError(string message)
        {
            Debug.WriteLine(message);
            this.log.AppendLine(message, Color.Red, false);
        }

        public static string GetExceptionMessage(Exception ex)
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

