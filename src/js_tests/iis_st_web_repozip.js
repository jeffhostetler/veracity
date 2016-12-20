load("suite_web_repozip.js");

function st_web_repozip_iis() {
    var suite = new suite_web_repozip('http://localhost/veracity');

    suite.setUp = function () {
        suite.suite_setUp();
    }

    return suite;
}
