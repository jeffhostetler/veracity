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

/**
 *
 * @file sg_jsglue__private_sg_config.h
 *
 * @details Set configuration-related properties in "sg.config.{...}".
 *
 */

//////////////////////////////////////////////////////////////////

#ifndef H_SG_JSGLUE__PRIVATE_SG_CONFIG_H
#define H_SG_JSGLUE__PRIVATE_SG_CONFIG_H

BEGIN_EXTERN_C;

//////////////////////////////////////////////////////////////////

/**
 * Set properties on the global "sg" symbol for various
 * compile-time configuration options and values.  These
 * will allow any consumer of the JS API to ask certain
 * questions about sglib.
 *
 * Most of these values should be considered constants
 * (and some have the READONLY property bit set).
 *
 * Others might be overwritten by the test harness
 * and/or the build machine.  (But just because the
 * test harness changes a value doesn't mean that sglib
 * can support it (without a recompile).)
 */
static void sg_jsglue__install_sg_config(SG_context * pCtx,
										 JSContext * cx,
										 JSObject * pjsobj_sg)
{
	JSObject * pjsobj_config = NULL;
	JSObject * pjsobj_version = NULL;
	SG_uint32 nrBits = (sizeof(void *) * 8);
	const char * pszVersion = NULL;

	SG_JS_NULL_CHECK(  pjsobj_config  = JS_DefineObject(cx, pjsobj_sg,     "config",  NULL, NULL, 0)  );	// sg.config.[...]
	SG_JS_NULL_CHECK(  pjsobj_version = JS_DefineObject(cx, pjsobj_config, "version", NULL, NULL, 0)  );	// sg.config.version.[...]

	// Define CONSTANTS equivalent to:
	//     var sg.config.version.major       = "1";
	//     var sg.config.version.minor       = "0";
	//     var sg.config.version.rev         = "0";
	//     var sg.config.version.buildnumber = "1000";

	SG_ERR_CHECK(  sg_jsglue__define_property__sz(pCtx, cx, pjsobj_version, "major",        MAJORVERSION)  );
	SG_ERR_CHECK(  sg_jsglue__define_property__sz(pCtx, cx, pjsobj_version, "minor",        MINORVERSION)  );
	SG_ERR_CHECK(  sg_jsglue__define_property__sz(pCtx, cx, pjsobj_version, "rev",          REVVERSION  )  );
	SG_ERR_CHECK(  sg_jsglue__define_property__sz(pCtx, cx, pjsobj_version, "buildnumber",  BUILDNUMBER )  );

	// Define CONSTANTS equivalent to:
	//     var sg.config.debug = true;

#if defined(DEBUG)
	SG_ERR_CHECK(  sg_jsglue__define_property__bool(pCtx, cx, pjsobj_config, "debug",  SG_TRUE )  );
#else
	SG_ERR_CHECK(  sg_jsglue__define_property__bool(pCtx, cx, pjsobj_config, "debug",  SG_FALSE )  );
#endif

	// Define CONSTANTS equivalent to:
	//     var sg.config.bits     = "64";
	//     var sg.config.platform = "MAC";

	SG_ERR_CHECK(  sg_jsglue__define_property__sz(pCtx, cx, pjsobj_config, "bits", ((nrBits == 64) ? "64" : "32"))  );
	SG_ERR_CHECK(  sg_jsglue__define_property__sz(pCtx, cx, pjsobj_config, "platform", SG_PLATFORM )  );

	// Define CONSTANTS equivalent to:
	//     var sg.config.version.string      = "1.0.0.1000 [64-bit MAC] (Debug)";
	//     var sg.config.version.string      = "1.0.0.1000 [32-bit MAC]";
	SG_ERR_CHECK(  SG_lib__version(pCtx, &pszVersion)  );
	SG_ERR_CHECK(  sg_jsglue__define_property__sz(pCtx, cx, pjsobj_version, "string", pszVersion)  );

	// TODO 2012/10/15 See sg.version in sg_jsglue__install_scripting_api()
	// TODO            for a duplication of properties "sg.config.version.string"
	// TODO            and "sg.version".
	// TODO 
	// TODO            And SG_JSGLUE_METHOD_PROTOTYPE(sg, platform) (which defines
	// TODO            sg.platform()) for a duplication of property "sg.config.platform".

fail:
	return;
}

//////////////////////////////////////////////////////////////////

END_EXTERN_C;

#endif//H_SG_JSGLUE__PRIVATE_SG_CONFIG_H
