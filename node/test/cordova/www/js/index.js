
var app = {
    initialize: function() {
        this.bindEvents();
    },

    bindEvents: function() {
        document.addEventListener('deviceready', this.onDeviceReady, false);
    },

    onDeviceReady: function() {
        console.log("Received deviceready event.");

        document.getElementById('testButton').addEventListener('click', app.onTestButtonClick);

        // Automatically run the tests on launch.
        app.onTestButtonClick();
    },

    onTestButtonClick: function() {
        var logView = document.getElementById("testLog");
        logView.innerHTML = "";

        runTests(
            window.opent2t,
            function(message, style) {
                console.log(message);
                logView.innerHTML +=
                    "<span style='" + (style || "") + "'>" + app.encodeHtml(message) + "</span><br/>";
                logView.scrollTop = logView.scrollHeight;
            });
    },

    encodeHtml: function(text) {
        var escaper = document.createElement("div");
        escaper.appendChild(document.createTextNode(text));
        return escaper.innerHTML;
    }
};

app.initialize();