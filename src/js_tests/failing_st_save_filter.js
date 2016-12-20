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


/*
 "filter": 
        {
            "merge_type": "field",
            "fields" :
            {
                "name": 
                {            
                    "datatype": "string",
                    "constraints": 
                    {
                      "required": true,
                      "unique": true
                    }
                },
                  
                "who": 
                {
                    "datatype": "userid",   
                    "constraints": 
                    {
                      "required": true,
                      "defaultvalue" : "whoami"                  
                    }                
                },
                "assignee":
                {
                    "datatype": "string"
                },
                "status":
                {
                    "datatype": "string"
                },
                "priority":
                {
                    "datatype": "string"
                },
                "reporter":
                {
                    "datatype": "string"
                },
                "verifier":
                {
                    "datatype": "string"                  
                },
                "datefrom":
                {
                    "datatype": "datetime",
                    "constraints" :
                    {
                        "required" : false
                    }
                },
                "dateto":
                {
                    "datatype": "datetime",
                    "constraints" :
                    {
                        "required" : false
                    }
                },
                "groupedby":
                {
                    "datatype": "string",
                    "constraints" :
                    {
                        "required" : false,
                        "allowed" :
                        [
                            "Status",
                            "Assignee",
                            "Priority",
                            "Reporter",
                            "Verifyer"                            
                        ],
                        
                        "defaultvalue": "Status"
                    }
                }               


            }  
        },*/

function st_save_filter()
{
    var fields = [];
    var filterField = function (n, dn)
    {
        this.name = n;
        this.displayname = dn;

    }
    function setupFields(zs)
    {
        var ztx = zs.begin_tx();
        var template = ztx.get_template();
        ztx.commit();

        for (var f in template.rectypes.item.fields)
        {               
            var field = template.rectypes.item.fields[f];
            var ff;
            if (field.form && field.form.label)
            {
                ff = new filterField(f, field.form.label);              
            }
            else
            {
                ff = new filterField(f, f);
            }
            fields.push(ff);
        }
    }
    this.testSaveFilterTemplate = function ()
    {

        var repo = sg.open_repo(repInfo.repoName);
        var zs = new zingdb(repo, sg.dagnum.WORK_ITEMS);

        setupFields(zs);

        var ztx = zs.begin_tx();
        var zrec = ztx.new_record("filter");
        var user = repInfo.userName;
        var groupedby = "status";
        var sortby = "priority";
        var columns = "id,id,name,assingee,reporter,priority";
        zrec.name = "filter1";
        zrec.user = user;
        zrec.columns = columns;
        zrec.groupedby = groupedby;
        zrec.sort = sortby;
        var fid = zrec.recid;
        var zcrit = ztx.new_record("filtercriteria");
        zcrit.name = zrec.name;
        zcrit.fieldid = "assignee";
        zcrit.value = user;
        var zcrit2 = ztx.new_record("filtercriteria");
        zcrit2.name = zrec.name;
        zcrit2.fieldid = "status";
        zcrit2.value = "open";

        zcrit.filter = zrec;
        zcrit2.filter = zrec;

        ztx.commit();

        var recs = zs.query("filter", ["*", "!"]);
        testlib.ok(recs.length == 1, "1 record");

        testlib.ok(recs[0].name == "filter1", "name should be filter1");
        testlib.ok(recs[0].groupedby == groupedby, "grouped by should be status");
        testlib.ok(recs[0].sort == sortby, "sort by should be priority");
        testlib.ok(recs[0].user == user, "user should be " + user);
        testlib.ok(recs[0].columns == columns, "columns should be " + columns);
        testlib.ok(recs[0].criteria.length == 2, "criteria length should be 2");

        repo.close();
    }
    this.testGetFilterByName = function ()
    {
        var repo = sg.open_repo(repInfo.repoName);
        var zs = new zingdb(repo, sg.dagnum.WORK_ITEMS);

        setupFields(zs);

        var ztx = zs.begin_tx();
        var zrec = ztx.new_record("filter");
        var user = repInfo.userName;
        var groupedby = "status";
        var sortby = "priority";
        var columns = "id,id,name,assingee,reporter,priority";
        zrec.name = "filter2";
        zrec.user = user;
        zrec.columns = columns;
        zrec.groupedby = groupedby;
        zrec.sort = sortby;
        var fid = zrec.recid;
        var zcrit = ztx.new_record("filtercriteria");
        zcrit.name = zrec.name;
        zcrit.fieldid = "assignee";
        zcrit.value = user;
        var zcrit2 = ztx.new_record("filtercriteria");
        zcrit2.name = zrec.name;
        zcrit2.fieldid = "status";
        zcrit2.value = "open";

        zcrit.filter = zrec;
        zcrit2.filter = zrec;

        ztx.commit();

        recs = zs.query("filter", ["*", "!"], "name == 'filter2'");
        testlib.ok(recs.length == 1, "1 record");
        testlib.ok(recs[0].name == "filter2", "name should be filter2");
       
        repo.close();       

        testlib.ok(recs.length == 1, "1 record named filter2");


    }
    this.testGetFilterByUser = function ()
    {
        var repo = sg.open_repo(repInfo.repoName);
        var zs = new zingdb(repo, sg.dagnum.WORK_ITEMS);

        setupFields(zs);

        var ztx = zs.begin_tx();
        var zrec = ztx.new_record("filter");
        var user = repInfo.userName;
        var groupedby = "status";
        var sortby = "priority";
        var columns = "id,id,name,assingee,reporter,priority";
        zrec.name = "filter3";
        zrec.user = user;
        zrec.columns = columns;
        zrec.groupedby = groupedby;
        zrec.sort = sortby;
        var fid = zrec.recid;
        var zcrit = ztx.new_record("filtercriteria");
        zcrit.name = zrec.name;
        zcrit.fieldid = "assignee";
        zcrit.value = user;
        var zcrit2 = ztx.new_record("filtercriteria");
        zcrit2.name = zrec.name;
        zcrit2.fieldid = "status";
        zcrit2.value = "open";

        zcrit.filter = zrec;
        zcrit2.filter = zrec;

        ztx.commit();

        recs = zs.query("filter", ["*", "!"], "user == '" + user + "'");
       
        repo.close();

        testlib.ok(recs.length == 3, "3 records for user");

    }

}
