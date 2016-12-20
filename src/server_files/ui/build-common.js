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

var buildColors =
{
	green : 1,
	red : 1,
	gray : 1,
	yellow : 1,
	white : 1
};

function getRec(recs, recid)
{
	for (var i = 0; i < recs.length; i++)
	{
		if (recs[i].recid == recid)
			return recs[i];
	}
	throw(recid + " not found.");
}

function getName(recs, recid)
{
	for (var i = 0; i < recs.length; i++)
	{
		if (recs[i].recid == recid)
			return recs[i].name;
	}
	throw(recid + " not found.");
}

function getDateTime(nMS)
{
	var d = new Date(parseInt(nMS));
	return shortDateTime(d, true);
}

function getDuration(nMS)
{
	var sReturn, sComp, nDur;

	nDur = nMS % 1000;
	sReturn = "s";

	// Strip off last component
	nMS -= nDur;
	nMS /= 1000;

	nDur = nMS % 60;
	if (nDur)
		sReturn = nDur.toString() + sReturn;
	else
		sReturn = "0" + sReturn;

	// Strip off last component
	nMS -= nDur;
	nMS /= 60;

	nDur = nMS % 60;
	if (nDur)
		sReturn = nDur.toString() + "m " + sReturn;

	// Strip off last component
	nMS -= nDur;
	nMS /= 60;

	if (nMS)
		sReturn = nMS.toString() + "h " + sReturn;

	return sReturn;
}

function sortBuildRecsByLastUpdateDesc(a, b)
{
	if (a.updated < b.updated)
		return 1;
	if (a.updated > b.updated)
		return -1;
	return 0;
}

function canGetEtaString(buildRec)
{
	if (buildRec.avg_duration === undefined || buildRec.eta_start === undefined)
		return false;

	return true;
}

function getEtaString(buildRec)
{
	var nowMS;
	var etaMS;

	if (canGetEtaString(buildRec) == false)
		return null;

	nowMS = new Date().getTime();
	etaMS = parseInt(buildRec.eta_start) + parseInt(buildRec.avg_duration);

	if (nowMS < etaMS)
		return getDuration(etaMS - nowMS);

	return getDuration(nowMS - etaMS) + " ago";
}

function getStartedFinishedLabel(buildRec, statusRec)
{
	if (statusRec.temporary == 1)
	{
		if (buildRec.eta_start !== undefined)
			return "Started:";
		else
			return "On:";
	}
	else
	{
		if (buildRec.eta_finish !== undefined)
			return "Finished:";
		else
			return "On:";
	}
}

function getStartedFinishedValue(buildRec, statusRec)
{
	if (statusRec.temporary == 1)
	{
		if (buildRec.eta_start !== undefined)
			return getDateTime(buildRec.eta_start);
		else
			return getDateTime(buildRec.updated);
	}
	else
	{
		if (buildRec.eta_finish !== undefined)
			return getDateTime(buildRec.eta_finish);
		else
			return getDateTime(buildRec.updated);
	}
}

function getEtaDurationLabel(buildRec, statusRec)
{
	if (statusRec.temporary == 1)
	{
		if (canGetEtaString(buildRec) == true)
			return "ETA:";
	}
	else if (buildRec.eta_finish !== undefined)
		return "Duration:";
	return null;
}

function getEtaDurationValue(buildRec, statusRec)
{
	if (statusRec.temporary == 1)
	{
		return getEtaString(buildRec);
	}
	else
	{
		if (buildRec.eta_start !== undefined && buildRec.eta_finish !== undefined)
		{
			return getDuration(buildRec.eta_finish - buildRec.eta_start);
		}
	}
	return null;
}
