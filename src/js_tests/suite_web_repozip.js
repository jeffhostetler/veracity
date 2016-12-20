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

function suite_web_repozip() {

  
    this.csid1 = "";
    this.csid2 = "";

    this.suite_setUp = function() {
        var folder = createFolderOnDisk("stWebRepoZip");
        var folder2 = createFolderOnDisk("stWebRepoZip2");
        var file = createFileOnDisk(pathCombine(folder, "file1.txt"), 39);
        var file2 = createFileOnDisk(pathCombine(folder, "file2.txt"), 9);
        var file3 = createFileOnDisk(pathCombine(folder2, "file3.txt"), 5);
        var file4 = createFileOnDisk(pathCombine(folder2, "file4.txt"), 10);
        var folder3 = createFolderOnDisk(pathCombine(folder, "folder3"));
        var foo = createFileOnDisk(pathCombine(folder3, "foo.txt"), 10);
        var empty = createFolderOnDisk("empty");
        var empty2 = createFolderOnDisk(pathCombine(folder3, "empty2"));
        
        vscript_test_wc__addremove();
        this.csid1 = vscript_test_wc__commit();
        /*stWebRepoZip
        /file1.txt
        /file2.txt
        stWebRepoZip2
        /file3.txt
        /file4.txt        
        */

        var file5 = createFileOnDisk(pathCombine(folder, "file5.txt"), 15);
        insertInFile(file, 20);
        vscript_test_wc__remove(file3);
        vscript_test_wc__move(file2, folder2);
        vscript_test_wc__rename(file4, "file4_rename");
        vscript_test_wc__add(file5);
        this.csid2 = vscript_test_wc__commit();

        /*stWebRepoZip
            /folder3
                /foo
        /file1.txt
        /file5.txt
        stWebRepoZip2
        /file2.txt
        /file4_rename        
        */
    };

    //no longer supported
   /* this.testZipNoCSID = function() {

        var url = this.rootUrl + "/repos/" + repInfo.repoName + "/zip/";

        var zipName = "testZipNoCSID.zip";
        var o = curl("-o", zipName, url);
        if (!testlib.equal("200 OK", o.status)) {
            print("response not 200");
            return;
        }

        if(sg.platform()!="WINDOWS")
        {
            o = sg.exec("unzip", zipName, "-d", "testZipNoCSID");
            print(o.stdout);

            sg.fs.cd(pathCombine(repInfo.workingPath, "testZipNoCSID"));
            /*stWebRepoZip
                /folder3
                    /empty
                    /foo
            /file1.txt
            /file5.txt
            stWebRepoZip2
            /file2.txt
            /file4_rename   
            empty     
            

            testlib.existsOnDisk("stWebRepoZip/folder3/foo.txt");
            testlib.existsOnDisk("stWebRepoZip/file1.txt");
            testlib.existsOnDisk("stWebRepoZip/file5.txt");
            testlib.existsOnDisk("stWebRepoZip2/file2.txt");
            testlib.existsOnDisk("stWebRepoZip2/file4_rename");
            testlib.existsOnDisk("empty");
            testlib.existsOnDisk("stWebRepoZip/folder3/empty2");

            sg.fs.cd(repInfo.workingPath);
            deleteFolderOnDisk("testZipNoCSID");
        }
    };*/

    this.testZipCSID1 = function() {

        sg.fs.cd(repInfo.workingPath);

        var url = this.rootUrl + "/repos/" + repInfo.repoName + "/zip/" + this.csid1 + ".zip";

        var zipName = this.csid1 + ".zip";
        var o = curl("-o", zipName, url);
        if (!testlib.equal("200 OK", o.status)) {
            print("response not 200");
            return;
        }


        if(sg.platform()!="WINDOWS")
        {
            o = sg.exec("unzip", zipName, "-d", zipName+"-unzipped");
            print(o.stdout);

            sg.fs.cd(zipName+"-unzipped");
            /*stWebRepoZip
                 /folder3
                    /empty
                    /foo.txt
            /file1.txt
            /file2.txt
            stWebRepoZip2
            /file3.txt
            /file4.txt 
            empty       
            */

            testlib.existsOnDisk("stWebRepoZip/folder3/foo.txt");
            testlib.existsOnDisk("stWebRepoZip/file1.txt");
            testlib.existsOnDisk("stWebRepoZip/file2.txt");
            testlib.existsOnDisk("stWebRepoZip2/file3.txt");
            testlib.existsOnDisk("stWebRepoZip2/file4.txt");
            testlib.existsOnDisk("empty");
            testlib.existsOnDisk("stWebRepoZip/folder3/empty2");

            sg.fs.cd(repInfo.workingPath);
            deleteFolderOnDisk(zipName+"-unzipped");
        }

    };

}
