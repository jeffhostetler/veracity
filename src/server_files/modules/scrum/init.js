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


(function() {

var def = {
	version: 1,						// veracity API version
	area: "scrum",
	vendor: sg.vendor.SOURCEGEAR,
	grouping: 3,
	hook_code_sync_rev: 30,
	dagnums: {
		"WORK_ITEMS": {
			dagid: 1,
			template: 'sg_ztemplate__wit.json'
		},
		"BUILDS": {
			dagid: 2,
			template: 'sg_ztemplate__builds.json'
		}
	},
	hooks: {
		"ask__wit__add_associations": {
			file: "addWorkItem.js",
			replace: true
		},
		"ask__wit__validate_associations": {
			file: "validateWorkItem.js",
			replace: true        },        "ask__wit__list_items": {            file: "listItems.js",            replace: true        }
	}
};

registerModule(def);

})();
