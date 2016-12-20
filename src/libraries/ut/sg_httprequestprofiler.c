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

#include <sg.h>


const char * SG_HTTPREQUESTPROFILER_CATEGORY_NAMES[] =
{
	// Using max 7 chars so as to fit into an 8-char column together with a '\t'
	"JsInit",
	"CookieA",
	"ZingQ",
	"OnPost",
	"ActDisp",
	"CachWr",
	"CachRd",
	"SessRd",
	
	"CachChk",
	"JsMutex",
	"SessWr",
	"SessCl",
	"JsReq",
	"OpenRep",
	"ConfRd",
	"_begin",
	"JsDisp",
	"Auth",
	"OnDisp",
	"_header",
	"_ch_req",
	"_ch_res",
	"Logging",
	"Xfer",
	"Other"
};

SG_STATIC_ASSERT(SG_NrElements(SG_HTTPREQUESTPROFILER_CATEGORY_NAMES) == SG_HTTPREQUESTPROFILER_CATEGORY__COUNT);


#define MAX_SIMULTANEOUS_HTTP_REQUESTS 50
#define MAX_CATEGORY_STACK_HEIGHT 20

struct sg_singlehttprequestprofiledata
{
	SG_thread_id thread;
	SG_int64 requestStartTime;
	
	SG_uint32 categoriesUsed;
	SG_int64 timeSpent[SG_HTTPREQUESTPROFILER_CATEGORY__COUNT];
	
	SG_httprequestprofiler_category category[MAX_CATEGORY_STACK_HEIGHT];
	SG_uint32 categoryStackLevel;
	SG_int64 categoryStartTime;
};

struct sg_aggregatedhttprequestprofiledata
{
	SG_int64 timeSpent[SG_HTTPREQUESTPROFILER_CATEGORY__COUNT];
	SG_uint32 nRequests;
};

void _add_request_to_aggregation(
	struct sg_singlehttprequestprofiledata * r,
	struct sg_aggregatedhttprequestprofiledata * a)
{
	int i;
	for(i=0; i<SG_HTTPREQUESTPROFILER_CATEGORY__COUNT; ++i)
	{
		a->timeSpent[i] += r->timeSpent[i];
	}
	a->nRequests += 1;
}


static struct
{
	SG_mutex mutex; // Protects all other members, including pCtx.
	
	SG_context * pCtx; // Protected by the mutex. Do not use without owning the lock.
	SG_file * pFile;
	
	SG_int64 startupTime;
	struct sg_aggregatedhttprequestprofiledata aggregatedData[SG_HTTPREQUESTPROFILER_CATEGORY__COUNT__SPECIAL][2];
	
	struct sg_singlehttprequestprofiledata requests[MAX_SIMULTANEOUS_HTTP_REQUESTS];
	SG_int64 lastRequestStopTime;
	SG_int32 nRequestsPrintedSinceLastTimestampPrinted;
} * gpHttpRequestProfilerState;


#define LOCK_PROFILER_STATE \
	if(gpHttpRequestProfilerState==NULL) { return; } \
	(void)SG_mutex__lock__bare(&gpHttpRequestProfilerState->mutex); \
	pCtx = gpHttpRequestProfilerState->pCtx;

#define UNLOCK_PROFILER_STATE \
	(void)SG_mutex__unlock__bare(&gpHttpRequestProfilerState->mutex);

#define HANDLE_ERROR_IF_PRESENT \
	if(SG_context__has_err(pCtx)) \
	{ \
		SG_context__err_reset(pCtx); \
		SG_ERR_IGNORE(  SG_file__write__format(pCtx, gpHttpRequestProfilerState->pFile, "# (Http Request Profiler problem of some sort)\n")  ); \
	}


void SG_httprequestprofiler__start_request()
{
	SG_context *pCtx;
	SG_uint32 i;
	
	LOCK_PROFILER_STATE;
	
	for(i=0; i<MAX_SIMULTANEOUS_HTTP_REQUESTS; ++i)
	{
		if(gpHttpRequestProfilerState->requests[i].requestStartTime==0)
		{
			struct sg_singlehttprequestprofiledata * r = &gpHttpRequestProfilerState->requests[i];
			SG_uint32 j;
			
			SG_ERR_CHECK(  SG_thread__get_current_thread(pCtx, &r->thread)  );
			r->categoriesUsed = 0;
			for(j=0; j<SG_HTTPREQUESTPROFILER_CATEGORY__COUNT; ++j)
			{
				r->timeSpent[j] = 0;
			}
			SG_ERR_CHECK(  SG_time__get_milliseconds_since_1970_utc(pCtx, &r->requestStartTime)  );
			r->categoryStackLevel = 0;
			r->category[r->categoryStackLevel] = SG_HTTPREQUESTPROFILER_CATEGORY__OTHER;
			r->categoryStartTime = r->requestStartTime;
			break;
		}
	}
	
fail:
	HANDLE_ERROR_IF_PRESENT;
	UNLOCK_PROFILER_STATE;
}

void sg_httprequestprofiler__stop_start(
	SG_bool stopCategory,
	SG_httprequestprofiler_category startCategory)
{
	SG_context *pCtx;
	SG_int64 stopTime = 0;
	SG_thread_id currentThreadId = 0;
	SG_uint32 i;
	
	(void)SG_time__get_milliseconds_since_1970_utc__err(&stopTime);
	
	LOCK_PROFILER_STATE;
	
	SG_ERR_CHECK(  SG_thread__get_current_thread(pCtx, &currentThreadId)  );
	
	for(i=0; i<MAX_SIMULTANEOUS_HTTP_REQUESTS; ++i)
	{
		SG_bool eq = SG_FALSE;
		SG_ERR_CHECK(  SG_thread__threads_equal(pCtx, currentThreadId, gpHttpRequestProfilerState->requests[i].thread, &eq)  );
		if(eq)
		{
			struct sg_singlehttprequestprofiledata * r = &gpHttpRequestProfilerState->requests[i];
			
			r->timeSpent[r->category[r->categoryStackLevel]] += (stopTime - r->categoryStartTime);
			
			if(stopCategory)
			{
				--r->categoryStackLevel;
				SG_ASSERT(r->categoryStackLevel < MAX_CATEGORY_STACK_HEIGHT);
			}
	
			if(startCategory!=SG_HTTPREQUESTPROFILER_CATEGORY__NOT_APPLICABLE)
			{
				++r->categoryStackLevel;
				SG_ASSERT(r->categoryStackLevel < MAX_CATEGORY_STACK_HEIGHT);
				r->category[r->categoryStackLevel] = startCategory;
				if(startCategory < SG_HTTPREQUESTPROFILER_CATEGORY__COUNT__SPECIAL)
				{
					r->categoriesUsed |= (1 << startCategory);
				}
			}
	
			SG_ERR_CHECK(  SG_time__get_milliseconds_since_1970_utc(pCtx, &r->categoryStartTime)  );
			break;
		}
	}
	
fail:
	HANDLE_ERROR_IF_PRESENT;
	UNLOCK_PROFILER_STATE;
}

void SG_httprequestprofiler__start(SG_httprequestprofiler_category category)
{
	sg_httprequestprofiler__stop_start(SG_FALSE, category);
}
void SG_httprequestprofiler__stop()
{
	sg_httprequestprofiler__stop_start(SG_TRUE, SG_HTTPREQUESTPROFILER_CATEGORY__NOT_APPLICABLE);
}
void SG_httprequestprofiler__switch(SG_httprequestprofiler_category new_category)
{
	sg_httprequestprofiler__stop_start(SG_TRUE, new_category);
}

void SG_httprequestprofiler__stop_request()
{
	SG_context *pCtx;
	SG_int64 stopTime = 0, lastStopTime = SG_INT64_MAX;
	SG_thread_id currentThreadId = 0;
	SG_uint32 i;
	
	(void)SG_time__get_milliseconds_since_1970_utc__err(&stopTime);
	
	LOCK_PROFILER_STATE;
	
	lastStopTime = gpHttpRequestProfilerState->lastRequestStopTime;
	gpHttpRequestProfilerState->lastRequestStopTime = stopTime;
	
	SG_ERR_CHECK(  SG_thread__get_current_thread(pCtx, &currentThreadId)  );
	
	for(i=0; i<MAX_SIMULTANEOUS_HTTP_REQUESTS; ++i)
	{
		SG_bool eq = SG_FALSE;
		SG_ERR_CHECK(  SG_thread__threads_equal(pCtx, currentThreadId, gpHttpRequestProfilerState->requests[i].thread, &eq)  );
		if(eq)
		{
			struct sg_singlehttprequestprofiledata * r = &gpHttpRequestProfilerState->requests[i];
			SG_uint32 j;
			SG_int_to_string_buffer tmp;
			
			r->timeSpent[SG_HTTPREQUESTPROFILER_CATEGORY__OTHER] += (stopTime - r->categoryStartTime);
			
			for(j=0; j<SG_HTTPREQUESTPROFILER_CATEGORY__COUNT__SPECIAL; ++j)
			{
				if(r->categoriesUsed & (1 << j))
				{
					_add_request_to_aggregation(r, &gpHttpRequestProfilerState->aggregatedData[j][1]);
				}
				else
				{
					_add_request_to_aggregation(r, &gpHttpRequestProfilerState->aggregatedData[j][0]);
				}
			}
			
			if(r->requestStartTime >= lastStopTime+10000)
			{
				SG_time tm1, tm2;
				SG_ERR_IGNORE(  SG_time__decode__local(pCtx, lastStopTime, &tm1)  );
				SG_ERR_IGNORE(  SG_time__decode__local(pCtx, r->requestStartTime, &tm2)  );
				SG_ERR_IGNORE( SG_file__write__format(pCtx, gpHttpRequestProfilerState->pFile,
					"# (%02d:%02d:%02d %d/%d -> %02d:%02d:%02d %d/%d)\n",
					tm1.hour, tm1.min, tm1.sec, tm1.month, tm1.mday,
					tm2.hour, tm2.min, tm2.sec, tm2.month, tm2.mday) );
				gpHttpRequestProfilerState->nRequestsPrintedSinceLastTimestampPrinted = 0;
			}
			if(gpHttpRequestProfilerState->nRequestsPrintedSinceLastTimestampPrinted==99)
			{
				SG_time tm;
				SG_ERR_IGNORE(  SG_time__decode__local(pCtx, stopTime, &tm)  );
				SG_ERR_IGNORE( SG_file__write__format(pCtx, gpHttpRequestProfilerState->pFile,
					"# %02d:%02d:%02d\n",
					tm.hour, tm.min, tm.sec) );
				gpHttpRequestProfilerState->nRequestsPrintedSinceLastTimestampPrinted = 0;
			}

			SG_ERR_IGNORE(  SG_file__write__format(pCtx, gpHttpRequestProfilerState->pFile, "%s\t%d", SG_int64_to_sz(r->requestStartTime, tmp), r->categoriesUsed)  );
			for(j=0; j<SG_HTTPREQUESTPROFILER_CATEGORY__COUNT; ++j)
			{
				SG_ERR_IGNORE(  SG_file__write__format(pCtx, gpHttpRequestProfilerState->pFile, "\t%s", SG_int64_to_sz(r->timeSpent[j], tmp))  );
			}
			SG_ERR_IGNORE(  SG_file__write__format(pCtx, gpHttpRequestProfilerState->pFile, "\t%s\n", SG_int64_to_sz(stopTime, tmp))  );
			
			gpHttpRequestProfilerState->nRequestsPrintedSinceLastTimestampPrinted += 1;
			
			r->requestStartTime = 0;
			break;
		}
	}
	
fail:
	HANDLE_ERROR_IF_PRESENT;
	UNLOCK_PROFILER_STATE;
}


void sg_httprequestprofiler__dump_all(SG_bool finalResults)
{
	SG_context *pCtx;
	SG_int64 dumpTime = 0;
	SG_time tm1, tm2;
	SG_int32 b, h, i;
	SG_int_to_string_buffer tmp;
	
	LOCK_PROFILER_STATE;
	
	(void)SG_time__get_milliseconds_since_1970_utc__err(&dumpTime);
	SG_ERR_IGNORE(  SG_time__decode__local(pCtx, dumpTime, &tm2)  );
	
	if(dumpTime >= gpHttpRequestProfilerState->lastRequestStopTime+10000)
	{
		SG_ERR_IGNORE(  SG_time__decode__local(pCtx, gpHttpRequestProfilerState->lastRequestStopTime, &tm1)  );
		SG_ERR_IGNORE( SG_file__write__format(pCtx, gpHttpRequestProfilerState->pFile,
			"# (%02d:%02d:%02d %d/%d -> %02d:%02d:%02d %d/%d)\n",
			tm1.hour, tm1.min, tm1.sec, tm1.month, tm1.mday,
			tm2.hour, tm2.min, tm2.sec, tm2.month, tm2.mday) );
	}
	
	SG_ERR_IGNORE(  SG_time__decode__local(pCtx, gpHttpRequestProfilerState->startupTime, &tm1)  );
	
	if(finalResults)
	{
		SG_ERR_IGNORE( SG_file__write__format(pCtx, gpHttpRequestProfilerState->pFile,
			"# Dumping stats from %02d:%02d:%02d %d/%d to %02d:%02d:%02d %d/%d/%d for clean shutdown.\n",
			tm1.hour, tm1.min, tm1.sec, tm1.month, tm1.mday,
			tm2.hour, tm2.min, tm2.sec, tm2.month, tm2.mday, tm2.year) );
	}
	else
	{
		SG_ERR_IGNORE( SG_file__write__format(pCtx, gpHttpRequestProfilerState->pFile,
			"# Dumping stats from %02d:%02d:%02d %d/%d to %02d:%02d:%02d %d/%d.\n",
			tm1.hour, tm1.min, tm1.sec, tm1.month, tm1.mday,
			tm2.hour, tm2.min, tm2.sec, tm2.month, tm2.mday) );
	}
	
	if(gpHttpRequestProfilerState->aggregatedData[0][0].nRequests==0 && gpHttpRequestProfilerState->aggregatedData[0][1].nRequests==0)
	{
		SG_ERR_IGNORE(  SG_file__write__format(pCtx, gpHttpRequestProfilerState->pFile, "# -> No requests served!\n")  );
	}
	else
	{
		SG_ERR_IGNORE(  SG_file__write__format(pCtx, gpHttpRequestProfilerState->pFile, "# ->      Category")  );
		for(i=0; i<SG_HTTPREQUESTPROFILER_CATEGORY__COUNT; ++i)
		{
			SG_ERR_IGNORE(  SG_file__write__format(pCtx, gpHttpRequestProfilerState->pFile, "\t%s", SG_HTTPREQUESTPROFILER_CATEGORY_NAMES[i])  );
		}
		SG_ERR_IGNORE(  SG_file__write__format(pCtx, gpHttpRequestProfilerState->pFile, "\tTotal\n")  );
	
		for(h=0; h<SG_HTTPREQUESTPROFILER_CATEGORY__COUNT__SPECIAL; ++h)
		{
			for(b=1; b>=0; --b)
			{
				SG_uint64 total = 0;
				SG_ERR_IGNORE(  SG_file__write__format(pCtx, gpHttpRequestProfilerState->pFile, "# ->       %s%s",
					b?"":"!",
					SG_HTTPREQUESTPROFILER_CATEGORY_NAMES[h])  );
				for(i=0; i<SG_HTTPREQUESTPROFILER_CATEGORY__COUNT; ++i)
				{
					total += gpHttpRequestProfilerState->aggregatedData[h][b].timeSpent[i];
					SG_ERR_IGNORE(  SG_file__write__format(pCtx, gpHttpRequestProfilerState->pFile, "\t%s",
						((h==i && !b)?"---":
						 SG_int64_to_sz(gpHttpRequestProfilerState->aggregatedData[h][b].timeSpent[i], tmp)
						))  );
				}
				SG_ERR_IGNORE(  SG_file__write__format(pCtx, gpHttpRequestProfilerState->pFile, "\t%s", SG_uint64_to_sz(total, tmp))  );
				SG_ERR_IGNORE(  SG_file__write__format(pCtx, gpHttpRequestProfilerState->pFile, " (%s requests)\n", SG_int64_to_sz(gpHttpRequestProfilerState->aggregatedData[h][b].nRequests, tmp))  );
			}
		}
	}
	
	SG_zero(gpHttpRequestProfilerState->aggregatedData);
	gpHttpRequestProfilerState->startupTime = dumpTime;
	gpHttpRequestProfilerState->lastRequestStopTime = dumpTime;
	
	UNLOCK_PROFILER_STATE;
}

void SG_httprequestprofiler__dump_all()
{
	sg_httprequestprofiler__dump_all(SG_FALSE);
}


void SG_httprequestprofiler__global_init(SG_context *pCtx)
{
	char *szConfigSetting = NULL;
	SG_bool full = SG_FALSE;
	SG_error err = SG_ERR_OK;
	SG_string *pFilename = NULL;
	SG_pathname *pPath = NULL;
	SG_time tm;
	SG_enumeration i;
	
	if(gpHttpRequestProfilerState!=NULL)
		return;
	
	SG_ERR_CHECK(  SG_localsettings__get__sz(pCtx, "server/hrp", NULL, &szConfigSetting, NULL)  );
	full = szConfigSetting && strcmp(szConfigSetting, "full")==0;
	SG_NULLFREE(pCtx, szConfigSetting);
	
	if(!full)
		return;
	
	SG_ERR_CHECK(  SG_alloc1(pCtx, gpHttpRequestProfilerState)  );
	
	err = SG_context__alloc(&gpHttpRequestProfilerState->pCtx);
	if(!SG_IS_OK(err))
	{
		SG_ERR_THROW(err);
	}
	
	SG_ERR_CHECK(  SG_mutex__init(pCtx, &gpHttpRequestProfilerState->mutex)  );
	
	(void)SG_time__get_milliseconds_since_1970_utc__err(&gpHttpRequestProfilerState->startupTime);
	gpHttpRequestProfilerState->lastRequestStopTime = gpHttpRequestProfilerState->startupTime;
	SG_ERR_IGNORE(  SG_time__decode__local(pCtx, gpHttpRequestProfilerState->startupTime, &tm)  );
	
	SG_ERR_CHECK( SG_string__alloc__format(pCtx, &pFilename, "hrp-%04d-%02d-%02d.log", tm.year, tm.month, tm.mday) );
	
	SG_ERR_CHECK(  SG_pathname__alloc__log_directory(pCtx, &pPath)  );
	SG_ERR_CHECK(  SG_pathname__append__from_string(pCtx, pPath, pFilename)  );
	SG_STRING_NULLFREE(pCtx, pFilename);
	
	SG_ERR_CHECK(  SG_file__open__pathname(pCtx, pPath, SG_FILE_OPEN_OR_CREATE|SG_FILE_WRONLY, 0644, &gpHttpRequestProfilerState->pFile)  );
	SG_ERR_CHECK(  SG_file__seek_end(pCtx, gpHttpRequestProfilerState->pFile, NULL)  );
	
	SG_ERR_IGNORE( SG_file__write__format(pCtx, gpHttpRequestProfilerState->pFile,
		"# Http Request Profiler starting up at %02d:%02d:%02d %d/%d/%d.\n",
		tm.hour, tm.min, tm.sec, tm.month, tm.mday, tm.year) );
	
	SG_ERR_IGNORE(  SG_file__write__format(pCtx, gpHttpRequestProfilerState->pFile, "# Start-Time   Category")  );
	for(i=0; i<SG_HTTPREQUESTPROFILER_CATEGORY__COUNT; ++i)
	{
		SG_ERR_IGNORE(  SG_file__write__format(pCtx, gpHttpRequestProfilerState->pFile, "\t%s", SG_HTTPREQUESTPROFILER_CATEGORY_NAMES[i])  );
	}
	SG_ERR_IGNORE(  SG_file__write__format(pCtx, gpHttpRequestProfilerState->pFile, "\tEnd-Time\n")  );
	SG_PATHNAME_NULLFREE(pCtx, pPath);
	
	return;
fail:
	SG_NULLFREE(pCtx, szConfigSetting);
	SG_STRING_NULLFREE(pCtx, pFilename);
	SG_PATHNAME_NULLFREE(pCtx, pPath);
	
	if(gpHttpRequestProfilerState!=NULL)
	{
		SG_FILE_NULLCLOSE(pCtx, gpHttpRequestProfilerState->pFile);
		SG_mutex__destroy(&gpHttpRequestProfilerState->mutex);
		SG_CONTEXT_NULLFREE(gpHttpRequestProfilerState->pCtx);
		SG_NULLFREE(pCtx, gpHttpRequestProfilerState);
	}
}

void SG_httprequestprofiler__global_cleanup()
{
	if(gpHttpRequestProfilerState!=NULL)
	{
		SG_context *pCtx = gpHttpRequestProfilerState->pCtx;
		sg_httprequestprofiler__dump_all(SG_TRUE);
		SG_FILE_NULLCLOSE(pCtx, gpHttpRequestProfilerState->pFile);
		SG_mutex__destroy(&gpHttpRequestProfilerState->mutex);
		SG_NULLFREE(pCtx, gpHttpRequestProfilerState);
		SG_CONTEXT_NULLFREE(pCtx);
	}
}
