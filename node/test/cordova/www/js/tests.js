var runTests = function () {
    var currentTest = null;
    var passCount;
    var failCount;
    var opent2t;
    var log;

    function run(plugin, logHandler) {
        passCount = 0;
        failCount = 0;
        opent2t = plugin;
        log = logHandler;

        if (!plugin) {
            log("Test plugin not found!");
            return;
        }

        log("Test plugin found.");

        return testNodeEngine()
        .then(
            function () {
                log("");
                logImportant("RESULTS: " + passCount + " passed, " + failCount + " failed.");
                log("");
            },
            function (error) {
                log("");
                logError("ERROR: " + error.message);
                log("");
            });
    }

    function testNodeEngine() {
        log("");
        log("Testing node engine...");

        currentTest = "new NodeEngine()";
        var node = new opent2t.NodeEngine();
        assert(node != null, "constructor");

        currentTest = "node.startAsync()";
        return node.startAsync(".")
        .then(expectResult(undefined), handleError).then(function () {

        currentTest = "node.callScriptAsync()";
        var message = '"Hello from node!"';
        return node.callScriptAsync("(function () { return " + message + "; })()")
        .then(expectResult(message), handleError).then(function () {

        currentTest = "node.registerCallFromScript()";
        var callbackArgs = null;
        node.addCallFromScriptListener(function (e) { callbackArgs = e.argsJson; });
        node.registerCallFromScript("testTest");
        return node.callScriptAsync("testTest(null, 0, 'test');")
        .then(expectResult(undefined), handleError).then(function () {

        assert(stringify(callbackArgs) == "[null,0,\"test\"]", "[null,0,\"test\"]");

        currentTest = "node.callScriptAsync(error)";
        return node.callScriptAsync("throw new Error('test');")
        .then(expectNoResult(), handleExpectedError).then(function () {

        currentTest = "node.defineScriptFile()";
        callbackArgs = null;
        node.DefineScriptFile("test.js", "testTest(123);");
        return node.callScriptAsync("require('test.js');")
        .then(expectResult(undefined), handleError).then(function () {

        });});});});});
    }

    function expectNoResult() {
        return expect(function (result) { return false; }, "(exception)");
    }

    function expectResult(expected) {
        return expect(function (result) { return result === expected; }, stringify(expected));
    }

    function expectResult2(expected) {
        return expect(function (result) { return stringify(result) === stringify(expected); }, stringify(expected));
    }

    function expect(expectation, expectationString) {
        return function(result) {
            if (expectation(result)) {
                logSuccess("PASSED: " + currentTest + " - " + expectationString);
                passCount++;
            } else {
                logError("FAILED: " + currentTest +
                    " - actual: " + stringify(result) + ", expected: " + expectationString);
                failCount++;
            }
        };
    }

    function assert(assertion, assertionString) {
        if (assertion) {
            logSuccess("PASSED: " + currentTest + " - " + assertionString);
            passCount++;
        } else {
            logError("FAILED: " + currentTest + " - " + assertionString);
            failCount++;
        }
    }

    function handleError(error) {
        logError("FAILED: " + currentTest + " - exception: " +
            (error.message || stringify(error)));
        failCount++;
    }

    function handleExpectedError(error) {
        logSuccess("PASSED: " + currentTest + " - expected exception: " +
            (error.message || stringify(error)));
        passCount++;
    }

    function logSuccess(message) {
        log(message, "color: green");
    }

    function logError(message) {
        log(message, "color: red");
    }

    function logImportant(message) {
        log(message, "font-weight: bold; font-size: 120%");
    }

    function stringify(obj) {
        try {
            return JSON.stringify(obj);
        } catch (e) {
            return e.message || JSON.stringify(e);
        }
    }

    return run;
}();
