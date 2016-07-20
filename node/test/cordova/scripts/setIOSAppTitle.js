module.exports = function(context) {
    var title = "OpenT2T Client Cordova Tests";
    var plistPath = "platforms/ios/OpenT2TClientTest/OpenT2TClientTest-Info.plist";

    var fs = context.requireCordovaModule('fs');
    var path = context.requireCordovaModule('path');

    var rootdir = context.opts.projectRoot;

    function replaceTextInFile(filename, findText, replaceText) {
        var contents = fs.readFileSync(filename, 'utf8');
        var updatedContents = contents.replace(findText, replaceText);
        fs.writeFileSync(filename, updatedContents, 'utf8');
    }

    replaceTextInFile(path.join(rootdir, plistPath), "${PRODUCT_NAME}", title);
};
