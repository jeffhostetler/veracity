/*
Copyright 2010-2013 SourceGear, LLC

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
*/


load("../js_test_lib/vscript_test_lib.js");

function st_list_descriptors2 ()
{
    this.testListDescriptors2 = function ()
    {
        var vhDescList = sg.list_descriptors2();
        var newRepoName = repInfo.repoName + "testListDescriptors2";
        testlib.isNotNull(vhDescList.descriptors);
        testlib.equal(false, vhDescList.has_excludes, "has_excludes should be false");
       /* sg.vv.init_new_repo(['no-wc'], newRepoName);

        sg.exec(vv, "localsettings", "add-to", "server/repos/exclude", newRepoName);
            
        testlib.equal(true, vhDescList.has_excludes, "has_excludes should be true");*/
    }
}
