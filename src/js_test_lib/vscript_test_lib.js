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

//load("vscript_tests.js");
// for JSON stringifying.  I know sglib has this, but this is the version
// used client-side, so it's available here for consistency
load("../js_test_lib/json2.js");


// Veracity specific helper functions


var vv = "vv";

function repoInfo(rName, wPath, fName, userName) {
    this.repoName = rName;
    this.workingPath = wPath;
    this.folderName = fName;
    this.username = userName;

}

//Creates a new repo and fills in the repInfo struct
function createNewRepo(name) {
    //The intent here is to:
    // 1. create a new folder
    // 2. cd to that.
    // 3. create a new repo for that folder    
   
//    var path = "temp";
    var path = tempDir;
    var r = name || "repo";
    
    var foldername = r + "_" + new Date().getTime();
    var dirname = pathCombine(path, foldername);
    if (!sg.fs.exists(dirname)) {
        sg.fs.mkdir_recursive(dirname);
    }
    sg.fs.cd(dirname);
    
    var repoName = foldername;

    sg.vv2.init_new_repo({"repo":repoName});

    var rInfo = new repoInfo(repoName, sg.fs.getcwd(), foldername);
   
    return rInfo;
}

//Like createNewRepo, but also creates and sets a current user
function createNewRepoWithUser(reponame, username) {

	r = createNewRepo(reponame);
	r.username = username || "test@sourcegear.com";

	sg.vv2.whoami({"repo" : r.repoName, "username" : r.username, "create":true});

	return r;
}

function createUsers(arrayOfUsers)
{
	for (index in arrayOfUsers)
	{
		sg.vv2.whoami({"username" : arrayOfUsers[index], "create":true});
	}
}

//creates a folder at the specified path on disk
//if depth is given it will create X number of subfolders
//under the given path.
function createFolderOnDisk(path, depth, dontcheck) {
    echoCmd();

    sg.fs.cd(repInfo.workingPath);

    if (depth) {
        for (var i = 0; i < depth; i++) {
            path += "/" + "NewFolder" + i;
        }
    }

    if (!sg.fs.exists(path)) {
        sg.fs.mkdir_recursive(path);
    }
    if (!dontcheck)
    {
        testlib.existsOnDisk(path);//e(sg.fs.exists(dir), dir + " should exist on disk");
    }
    return path;

}

//Creates a folder on disk and adds it to
//the repo. Path should be reletive to the 
//main repo folder
function addFolder(path) {
    echoCmd();

    createFolderOnDisk(path);
    vscript_test_wc__add(path);
    vscript_test_wc__commit();
    vscript_test_wc__statusEmptyExcludingIgnores(); //(verifyCleanStatus(), "status should be empty");
    testlib.inRepo(path);

}

//rename a file or folder. Path should be reletive to the 
//main repo folder
function renameOnDisk(relPath, newName) {
    echoCmd();
    sg.fs.cd(repInfo.workingPath);
    var parent = getParentPath(relPath);
    var newParent = getParentPath(newName);
    if (newParent != "") {
        if (newParent == parent) {
            newName = getFileNameFromPath(newName);
        } else {
            testlib.ok(false, "-> Invalid newName for renameOnDisk ('" + newName + "')");
            return getFileNameFromPath(relPath);
        }
    }
    var newPath = pathCombine(parent, newName);
    sg.fs.move(relPath, newPath);

    testlib.existsOnDisk(newPath);
    return newName;
}

//move a file or folder. Path should be reletive to the 
//main repo folder
function moveOnDisk(relPath, newPath) {
    echoCmd();
    sg.fs.cd(repInfo.workingPath);
    sg.fs.move(relPath, newPath);
    testlib.existsOnDisk(newPath);
    return newPath;
}

//Calls addremove() then verifies the empty status
function addRemoveAndCommit() {
    echoCmd();
    vscript_test_wc__addremove();
    var hid = vscript_test_wc__commit();
    vscript_test_wc__statusEmptyExcludingIgnores();
    //sleep(1000);	// sleep after a commit so that subsequent operations won't confuse timestamp cache on low resolution filesystems.
	return hid;
}

//Calls addremove() but doesn't commit
function addRemoveNoCommit() {
    echoCmd();
    vscript_test_wc__addremove();
}

//Calls commit and verifies the empty status
function commit() {
    echoCmd();
    vscript_test_wc__commit();
    vscript_test_wc__statusEmptyExcludingIgnores();    

    //sleep(1000);	// sleep after a commit so that subsequent operations won't confuse timestamp cache on low resolution filesystems.
}

//Delete a file on disk. Path should be reletive to the 
//main repo folder
function deleteFileOnDisk(relPath) {
    echoCmd();
    sg.fs.cd(repInfo.workingPath);
    sg.fs.remove(relPath);
    testlib.notOnDisk(relPath);

}

//Delete a folder on disk. Path should be reletive to the 
//main repo folder
function deleteFolderOnDisk(relPath) {
    echoCmd();
    sg.fs.cd(repInfo.workingPath);
    sg.fs.rmdir_recursive(relPath);
    testlib.notOnDisk(relPath);

}

//Creates a folder with a bunch of folders and files underneath
function createFolderRecursive_Fixed(relPath, maxNumFolders, maxNumFilesInFolder) {
    echoCmd();
    var numFolders = maxNumFolders;
    var numFilesInFolder = maxNumFilesInFolder;

    var fsFolders = [];
    var fsFiles = [];

    var baseFolder = createFolderOnDisk(relPath);

    fsFolders.push(baseFolder);
    for (var i = 0; i < numFolders; i++) {
       
        var folder = createFolderOnDisk(pathCombine(baseFolder, "folder_" + i), null, true);
       
        for (var j = 0; j < numFilesInFolder; j++) {
            var file = createFileOnDisk(pathCombine(folder, "file_" + j + ".txt"), 100, null, true);
            fsFiles.push(file);
        }
        fsFolders.push(folder);
    }
    return [baseFolder, fsFolders.concat(fsFiles)];

}

//Creates a folder with a bunch of folders and files underneath
function createFolderWithSubfoldersAndFiles(relPath, maxNumFolders, maxNumFilesInFolder) {
    echoCmd();
    var numFolders = maxNumFolders || Math.floor(Math.random() * 50);
    var numFilesInFolder = maxNumFilesInFolder || 100;

    var fsFolders = [];
    var fsFiles = [];

    var baseFolder = createFolderOnDisk(relPath);

    fsFolders.push(baseFolder);
    for (var i = 0; i < numFolders; i++) {

        var rand = Math.floor(Math.random() * fsFolders.length);
        var folder = createFolderOnDisk(pathCombine(fsFolders[rand], "folder_" + i), null, true);
        var rand2 = Math.floor(Math.random() * numFilesInFolder);
       
        for (var j = 0; j < rand2; j++) {
            var file = createFileOnDisk(pathCombine(folder, "file_" + j + ".txt"), null, null, true);
            fsFiles.push(file);
        }
        fsFolders.push(folder);
    }
    return [baseFolder, fsFolders.concat(fsFiles)];

}

//Creates a file. Parent folder must exist. Path should be reletive to the 
//main repo folder
function createFileOnDisk(relPath, nlines, text, dontcheck) {
    echoCmd();
    sg.fs.cd(repInfo.workingPath);
    
    var numlines = nlines || Math.floor(Math.random() * 25) + 1;
    var someText = text || "file contents for version 1 - " + Math.floor(Math.random() * numlines);
    var parentFullPath;


    if (!sg.fs.exists(relPath)) {
        sg.file.write(relPath, someText + "\n");
    }

    for (var i = 0; i < numlines; i++) {
        sg.file.append(relPath, someText + "\n");
    }

    if (!dontcheck)
    {
        testlib.existsOnDisk(relPath);
    }

    return relPath;

}

//Creates a file with random contents.
function createRandomFileOnDisk(path, numbytes) {
    echoCmd();
    palette = "0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ- .!?\n";
    
    sg.file.write(path, "");
    written = 0;
    while(written<numbytes){
        var buffer = "";
        while(buffer.length<1024*1024&&buffer.length<numbytes)
            buffer+=palette[Math.round(Math.random()*palette.length)%palette.length];
        sg.file.append(path, buffer);
        written+=buffer.length
    }
}

//Creates a copy of the given file in
//the existing directory with a random extension
function createTempFile(file) {
    echoCmd();
    sg.fs.cd(repInfo.workingPath);
    var text = sg.file.read(file);
      
    var tmpFile = file + new Date().getTime();

    sg.file.write(tmpFile, text);
    sleep(100); //do this so the file names are unique on fast machines
    return tmpFile;
}

//Creates a copy of the given file on disk.
function copyFileOnDisk(srcFilePath, destFilePath) {
    echoCmd();
    sg.fs.cd(repInfo.workingPath);
    var text = sg.file.read(srcFilePath);
    sg.file.write(destFilePath, text);
    return destFilePath;
}

//Perform a random file edit
function doRandomEditToFile(path) {
    echoCmd();
    var method = Math.floor(Math.random() * 4);
    var rand = Math.floor(Math.random() * 100) + 1;
    switch (method) {
        case 0:
            appendToFile(path, rand);
            break;
        case 1:
            insertInFile(path, rand);
            break;
        case 2:
            replaceInFile(path, rand);
            break;
        case 3:
            deleteInFile(path);
            break;
        default: 
            appendToFile(path, rand);
            break;
            

    }    
	return method;

}

//Appends some lines to the end of a file. numLines and text optional
function appendToFile(relPath, numLines, text) {
    echoCmd();
    sg.fs.cd(repInfo.workingPath);
    var nl = numLines || Math.floor(Math.random() * 100) + 1;
    
    var newText = text || "file contents appended for new version " + Math.floor(Math.random() * nl);
    if (!sg.fs.exists(relPath)) {
        sg.file.write(relPath, newText + "\n");
    }

    for (var i = 0; i < nl; i++) {
        sg.file.append(relPath, newText + "\n");

    }
}

//Inserts some text to a random place in a file. numLines and text optional.
function insertInFile(relPath, numLines, text) {
    echoCmd();
    var insertText = [];

    sg.fs.cd(repInfo.workingPath);

    var nl = numLines || Math.floor(Math.random() * 100) + 1;
    
    var newText = text || "file contents inserted for new version " + Math.floor(Math.random() * nl);
    
    
    if (!sg.fs.exists(relPath)) {
        sg.file.write(relPath, "");       
    }
    
    for (var i = 0; i < nl; i++) {
        insertText[i] = newText + " " + i;  
    }
    var fullText = sg.file.read(relPath);
    var lines = fullText.split("\n");
    
    var lineNumber = Math.floor(Math.random() * lines.length);
    var start = lines.slice(0, lineNumber);
    var end = lines.slice(lineNumber + 1, lines.length);
    
    var newLines = start.concat(insertText, end);
    sg.file.write(relPath, newLines.join("\n"));

}

//Replaces some text at a random place in a file. numLines and text optional.
function replaceInFile(relPath, numLines, text){
    echoCmd();
    var insertText = [];

    var nl = numLines || Math.floor(Math.random() * 100) + 1;
    sg.fs.cd(repInfo.workingPath);
    var newText = text || "file contents replaced for new version " + Math.floor(Math.random() * nl);
    
    
    if (!sg.fs.exists(relPath)) {
        sg.file.write(relPath, "");       
    }
    
    for (var i = 0; i < nl; i++) {
        insertText[i] = newText + " " + i;  
    }
    var fullText = sg.file.read(relPath);
    var lines = fullText.split("\n");
    
    var lineNumber = Math.floor(Math.random() * lines.length) + 1;
    lines[lineNumber] = newText;
    
    
    sg.file.write(relPath, lines.join("\n"));

}

//deletes some random text in the file
function deleteInFile(relPath) {
    echoCmd();

    sg.fs.cd(repInfo.workingPath);
      
    if (!sg.fs.exists(relPath)) {
      
        sg.file.write(relPath, "");
    }
    var fullText = sg.file.read(relPath);
    var lines = fullText.split("\n");

    var lineNumber = Math.floor(Math.random() * lines.length);
    var numLines = Math.floor(Math.random() * (lines.length - lineNumber));
    var newLines = lines.splice(lineNumber, numLines);
   
    sg.file.write(relPath, newLines.join("\n"));

}

//compares 2 files in your working copy 
//to verify they are the same
function compareFiles(path1, path2) {
    echoCmd();

    var p1;
    var p2;

    if (!sg.fs.exists(path1)) {
        p1 = pathCombine(repInfo.workingPath, path1);

    }
    else
        p1 = path1;
        
    if (!sg.fs.exists(path2)){
        p2 = pathCombine(repInfo.workingPath, path2);
    }
    else 
        p2 = path2;

   
    if (sg.fs.length(p1) !=  sg.fs.length(p2))
            return false;
            
    var fullText1 = sg.file.read(p1);
    var fullText2 = sg.file.read(p2);

    for (var i=0; i<fullText1.length; i++){
        if (fullText1[i] != fullText2[i])
            return false;

    }
    return true;
}


//verifies that a given path exists in the repo
function verifyInRepo(repoName, path) {
    echoCmd();
    var inRepo = false;
    path = path.trim("/");
    path = "@/" + path;
    var repo = sg.open_repo(repoName)
    // TODO 4/6/10 BUGBUG We should not assume the leaf/head; use current baseline.
    var array = repo.fetch_dag_leaves(sg.dagnum.VERSION_CONTROL);
    function recursetree(prefix, hid) {
        if (!inRepo) {
            var treenode = repo.fetch_json(hid);
            if (treenode == null || treenode.tne == null)
                return;
            if (prefix.length > 0)
                prefix += "/"
            for (var i in treenode.tne) {
                var repPath = prefix + treenode.tne[i].name;
               
		// TODO 4/6/10 BUGBUG We cannot assume case folding.
                if (path.toLowerCase() == repPath.toLowerCase()) {
                  
                    inRepo = true;

                }
                //print(prefix + treenode.tne[i].name)
                if (treenode.tne[i].type == 2)
                    recursetree(prefix + treenode.tne[i].name, treenode.tne[i].hid);
            }
        }
    }
    if (array != null)
        var dagnode = repo.fetch_dagnode(sg.dagnum.VERSION_CONTROL, array[0]);
   
    var changeset = repo.fetch_json(dagnode.hid);
    var arrOfItems = new Array();    
    
    recursetree("", changeset.tree.root);
    repo.close();
    return inRepo;

}


function reportTreeObjectStatus(objPath, noPrint)  // DEPRECATED
{
    echoCmd();
    return vscript_test_wc__reportTreeObjectStatus(objPath, noPrint);
}


//some generic helper functions
var MAX_DUMP_DEPTH = 10;
String.prototype.trim = function(chars) {
    var str = this.rtrim(chars);
    str = str.ltrim(chars);
    return str;

}
String.prototype.ltrim = function(chars) {
    chars = chars || "\\s";
    return this.replace(new RegExp("^[" + chars + "]+", "g"), "");
}

String.prototype.rtrim = function(chars) {
    chars = chars || "\\s";
    return this.replace(new RegExp("[" + chars + "]+$", "g"), "");
}

if (!Array.prototype.indexOf) {
    Array.prototype.indexOf = function(obj, fromIndex) {
        if (fromIndex == null) {
            fromIndex = 0;
        } else if (fromIndex < 0) {
            fromIndex = Math.max(0, this.length + fromIndex);
        }
        for (var i = fromIndex, j = this.length; i < j; i++) {
            if (this[i] === obj)
                return i;
        }
        return -1;
    };
}

function getParentPath(path) {
    
    var parent = path.rtrim("/");
    // path.rtrim("\\");

    var index = parent.lastIndexOf("/");

    if (index < 0) {

        return "";
    }

    try {
        parent = parent.slice(0, index);
    }
    catch (r) {
        print(r);
    }
    return parent;

}

function getFileNameFromPath(path) {
    var parent = path.rtrim("/");
 
    var index = parent.lastIndexOf("/");

    var file = path.slice(index + 1, path.length);
   
    return file;

}

function pathCombine(path1, path2) {

    path1 = path1.rtrim("/");
    path2 = path2.ltrim("/");
    return (path1 + "/" + path2).rtrim("/");

}


function sleep(delay) {
    echoCmd();
    sg.sleep_ms(delay);
}

// prints the command being executed if in verbose mode
// echoCmd(true) will always print (except if failuresOnly)
function echoCmd(alwaysPrint) {
    if ((testlib.verbose || alwaysPrint) && !testlib.failuresOnly) {
        cmdStr = formatFunctionString(getTestName(echoCmd.caller.toString()), echoCmd.caller.arguments);
        indentLine("=> " + cmdStr);
    }
}

//gets the name of the test from the function string
function getTestName(functionString) {
    var ownName = functionString.substr('function '.length);
    ownName = ownName.substr(0, ownName.indexOf('('));
    this.type = ownName;
    return ownName;
}

//helper function to format a function call when you need to pass it in
//as a string
function formatFunctionString(functionName, args) {
    var qArgs = [];
    for (var i = 0; i < args.length; i++) {
        if (args[i] != undefined) {
            qArgs.push("\"" + args[i] + "\"");
        }
    }
    var s = functionName + "(" + qArgs.join(",") + ")";
    return s;
    

}

//indents a line and optionally prints it
function indentLine(line, noPrint) {
    var l = "\t" + line;
    if (!noPrint)
        print(l);
    return l;

}

function dumpObj(obj, name, indent, depth) {
    if (depth > MAX_DUMP_DEPTH) {
        return indent + name + ": <Maximum Depth Reached>\n";
    }
    if (typeof obj == "object") {
        var child = null;
        var output = indent + name + "\n";
        indent += "\t";
        for (var item in obj) {
            try {
                child = obj[item];
            } catch (e) {
                child = "<Unable to Evaluate>";
            }
            if (typeof child == "object") {
                output += dumpObj(child, item, indent, depth + 1);
            } else {
                output += indent + item + ": " + child + "\n";
            }
        }
        return output;
    } else {
        return obj;
    }
}


function dumpTree(headerMsg) // DEPRECATED
{
    if (arguments.length == 0)
        headerMsg = "-> PendingTree dump:";
    vscript_test_wc__dumpTree(headerMsg);
}


function dumpRepo()
{
    var repo = sg.open_repo(repInfo.repoName)
    var array = repo.fetch_dag_leaves(sg.dagnum.VERSION_CONTROL);
    function printtree(prefix,  hid)
    {
        var treenode = repo.fetch_json(hid);
        if (treenode == null || treenode.tne == null)
            return;
        if (prefix.length > 0)
            prefix += "/"
        for (var i in treenode.tne)
        {
            print(prefix + treenode.tne[i].name)
            if (treenode.tne[i].type == 2)
                printtree(prefix + treenode.tne[i].name, treenode.tne[i].hid);
        }
    }
    if (array != null)
        //print("fetched dag leaves:  " + array)
    var dagnode = repo.fetch_dagnode(sg.dagnum.VERSION_CONTROL, array[0]);
    if ( dagnode != null)
    {
        //print("fetched dag node:  " + dumpObj(dagnode, "dagnode", 0, 0))
        //print("gen:  " + dagnode.gen)
    }
    var changeset = repo.fetch_json(dagnode.hid);
        //print("fetched changeset :  " + dumpObj(changeset, "changeset", 0, 0))
    var arrOfItems = new Array();
    printtree("", changeset.root);
    repo.close()
}



// Dump the ActionLog from various pendingtree operations like REVERT.
function dumpActionLog(headerMsg, actionLog)
{
    print(dumpObj(actionLog, "ActionLog(" + headerMsg + "):", "\t", 0));
}


// Syntax candy to create a WD-less repo for use by most zing tests.
function new_zing_repo(reponame)
{
    sg.vv2.init_new_repo({"repo":reponame, "no-wc":true});

    sg.vv2.whoami({"repo" : reponame, "username" : "zingtests@sourcegear.com", "create":true});
    
}

// All the submodule tests need this to get a user created
function whoami_testing(reponame)
{
	sg.vv2.whoami({"repo" : reponame, "username" : "testing@sourcegear.com", "create":true});
}

function newRecord(rectype, repo, fields)
{
	// create a sprint
	var ztx = null;
	var recid = null;
	
	try
	{
		var zs = new zingdb(repo, sg.dagnum.WORK_ITEMS);
		ztx = zs.begin_tx();
		var zrec = ztx.new_record(rectype);
		
		for ( var f in fields )
			zrec[f] = fields[f];

		recid = zrec.recid;

		vv.txcommit(ztx);
		ztx = null;

		return(recid);
	}
	finally
	{
		if (ztx)
			ztx.abort();
	}
}

// Execute 'curl', gathering headers and output.
// The last argument passed in is assumed to be the url, and gets encodeURI() called on it.
var curl = function() {
	var a = arguments;
	if(a.length>15) throw "too many args passed to curl";
	if(a.length==0) throw "too few args passed to curl";
	a[a.length-1] = encodeURI(a[a.length-1]);

	if (!sg.fs.exists(tempDir))
		sg.fs.mkdir(tempDir);

	var hpath = pathCombine(tempDir,"curl-headers");
	var opath = pathCombine(tempDir,"curl-output");

	sg.file.write(hpath, '');
	sg.file.write(opath, '');

	sg.sleep_ms(1);
	var o;
	if     (a.length==15) o=sg.exec("curl", "-D", hpath, a[0], a[1], a[2], a[3], a[4], a[5], a[6], a[7], a[8], a[9], a[10], a[11], a[12], a[13], "-o", opath, a[14]);
	else if(a.length==14) o=sg.exec("curl", "-D", hpath, a[0], a[1], a[2], a[3], a[4], a[5], a[6], a[7], a[8], a[9], a[10], a[11], a[12], "-o", opath, a[13]);
	else if(a.length==13) o=sg.exec("curl", "-D", hpath, a[0], a[1], a[2], a[3], a[4], a[5], a[6], a[7], a[8], a[9], a[10], a[11], "-o", opath, a[12]);
	else if(a.length==12) o=sg.exec("curl", "-D", hpath, a[0], a[1], a[2], a[3], a[4], a[5], a[6], a[7], a[8], a[9], a[10], "-o", opath, a[11]);
	else if(a.length==11) o=sg.exec("curl", "-D", hpath, a[0], a[1], a[2], a[3], a[4], a[5], a[6], a[7], a[8], a[9], "-o", opath, a[10]);
	else if(a.length==10) o=sg.exec("curl", "-D", hpath, a[0], a[1], a[2], a[3], a[4], a[5], a[6], a[7], a[8], "-o", opath, a[9]);
	else if(a.length==9)  o=sg.exec("curl", "-D", hpath, a[0], a[1], a[2], a[3], a[4], a[5], a[6], a[7], "-o", opath, a[8]);
	else if(a.length==8)  o=sg.exec("curl", "-D", hpath, a[0], a[1], a[2], a[3], a[4], a[5], a[6], "-o", opath, a[7]);
	else if(a.length==7)  o=sg.exec("curl", "-D", hpath, a[0], a[1], a[2], a[3], a[4], a[5], "-o", opath, a[6]);
	else if(a.length==6)  o=sg.exec("curl", "-D", hpath, a[0], a[1], a[2], a[3], a[4], "-o", opath, a[5]);
	else if(a.length==5)  o=sg.exec("curl", "-D", hpath, a[0], a[1], a[2], a[3], "-o", opath, a[4]);
	else if(a.length==4)  o=sg.exec("curl", "-D", hpath, a[0], a[1], a[2], "-o", opath, a[3]);
	else if(a.length==3)  o=sg.exec("curl", "-D", hpath, a[0], a[1], "-o", opath, a[2]);
	else if(a.length==2)  o=sg.exec("curl", "-D", hpath, a[0], "-o", opath, a[1]);
	else                  o=sg.exec("curl", "-D", hpath, "-o", opath, a[0]);
	sg.sleep_ms(1);

	o.headers = sg.file.read(hpath);
	o.body = sg.file.read(opath);
	o.http_status = o.headers.slice(o.headers.indexOf(" ")+1, o.headers.indexOf("\r"));
	if(o.http_status=="100 Continue"){
		var tmp = o.headers.slice(o.headers.indexOf("\r\n\r\n")+4);
		o.http_status = tmp.slice(tmp.indexOf(" ")+1, tmp.indexOf("\r"));
	}
	o.status = ((o.exit_status==0)?o.http_status:("curl error "+o.exit_status));
	return o;
}
