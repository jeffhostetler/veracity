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
 * @file sg_localsettings.c
 */

#include <sg.h>

#if defined(LINUX) | defined(MAC)
#define MACHINE_CONFIG_FILENAME "veracity.conf"
#elif defined(WINDOWS)
#define MACHINE_CONFIG_FILENAME "config.json"
#endif

#if defined(DEBUG)
#define TRACE_LS 0
#else
#define TRACE_LS 0
#endif

void SG_localsettings__update__sz(SG_context * pCtx, const char * psz_path, const char * pValue)
{
	SG_jsondb* p = NULL;
    SG_string* pstr_path = NULL;

	SG_ASSERT(pCtx);
	SG_NONEMPTYCHECK_RETURN(psz_path);

    SG_ERR_CHECK(  SG_STRING__ALLOC(pCtx, &pstr_path)  );
    if ('/' == psz_path[0])
    {
        SG_ERR_CHECK(  SG_string__sprintf(pCtx, pstr_path, "%s", psz_path)  );
    }
    else
    {
        SG_ERR_CHECK(  SG_string__sprintf(pCtx, pstr_path, "%s/%s", SG_LOCALSETTING__SCOPE__MACHINE, psz_path)  );
    }

    SG_ERR_CHECK(  SG_closet__get_localsettings(pCtx, &p)  );
	SG_ERR_CHECK(  SG_jsondb__update__string__sz(pCtx, p, SG_string__sz(pstr_path), SG_TRUE, pValue)  );

fail:
    SG_STRING_NULLFREE(pCtx, pstr_path);
	SG_JSONDB_NULLFREE(pCtx, p);
}

void SG_localsettings__descriptor__update__sz(
	SG_context * pCtx,
	const char * psz_descriptor_name,
	const char * psz_path,
	const char * pValue)
{
	SG_jsondb* p = NULL;
	SG_string* pstr_path = NULL;

	SG_ASSERT(pCtx);
	SG_NONEMPTYCHECK_RETURN(psz_descriptor_name);
	SG_NONEMPTYCHECK_RETURN(psz_path);
	SG_ARGCHECK_RETURN('/' != psz_path[0], psz_path);

	SG_ERR_CHECK(  SG_STRING__ALLOC(pCtx, &pstr_path)  );
	SG_ERR_CHECK(  SG_string__sprintf(pCtx, pstr_path, "%s/%s/%s",
		SG_LOCALSETTING__SCOPE__INSTANCE, psz_descriptor_name, psz_path)  );

	SG_ERR_CHECK(  SG_closet__get_localsettings(pCtx, &p)  );
	SG_ERR_CHECK(  SG_jsondb__update__string__sz(pCtx, p, SG_string__sz(pstr_path), SG_TRUE, pValue)  );

fail:
	SG_STRING_NULLFREE(pCtx, pstr_path);
	SG_JSONDB_NULLFREE(pCtx, p);
}

void SG_localsettings__update__int64(SG_context * pCtx, const char * psz_path, SG_int64 val)
{
	SG_jsondb* p = NULL;
	SG_string* pstr_path = NULL;

	SG_ASSERT(pCtx);
	SG_NONEMPTYCHECK_RETURN(psz_path);

	SG_ERR_CHECK(  SG_STRING__ALLOC(pCtx, &pstr_path)  );
	if ('/' == psz_path[0])
	{
		SG_ERR_CHECK(  SG_string__sprintf(pCtx, pstr_path, "%s", psz_path)  );
	}
	else
	{
		SG_ERR_CHECK(  SG_string__sprintf(pCtx, pstr_path, "%s/%s", SG_LOCALSETTING__SCOPE__MACHINE, psz_path)  );
	}

	SG_ERR_CHECK(  SG_closet__get_localsettings(pCtx, &p)  );

	SG_ERR_CHECK(  SG_jsondb__update__int64(pCtx, p, SG_string__sz(pstr_path), SG_TRUE, val)  );

fail:
	SG_JSONDB_NULLFREE(pCtx, p);
	SG_STRING_NULLFREE(pCtx, pstr_path);
}


void SG_localsettings__update__vhash(SG_context * pCtx, const char * psz_path, const SG_vhash * pValue)
{
	SG_jsondb* p = NULL;
    SG_string* pstr_path = NULL;

    SG_ASSERT(pCtx);
    SG_NONEMPTYCHECK_RETURN(psz_path);
    SG_NULLARGCHECK_RETURN(pValue);

    SG_ERR_CHECK(  SG_STRING__ALLOC(pCtx, &pstr_path)  );
    if ('/' == psz_path[0])
    {
        SG_ERR_CHECK(  SG_string__sprintf(pCtx, pstr_path, "%s", psz_path)  );
    }
    else
    {
        SG_ERR_CHECK(  SG_string__sprintf(pCtx, pstr_path, "%s/%s", SG_LOCALSETTING__SCOPE__MACHINE, psz_path)  );
    }

    SG_ERR_CHECK(  SG_closet__get_localsettings(pCtx, &p)  );

	SG_ERR_CHECK(  SG_jsondb__update__vhash(pCtx, p, SG_string__sz(pstr_path), SG_TRUE, pValue)  );

fail:
	SG_JSONDB_NULLFREE(pCtx, p);
    SG_STRING_NULLFREE(pCtx, pstr_path);
}

void SG_localsettings__update__varray(SG_context * pCtx, const char * psz_path, const SG_varray * pValue)
{
	SG_jsondb* p = NULL;
    SG_string* pstr_path = NULL;

    SG_ASSERT(pCtx);
    SG_NONEMPTYCHECK_RETURN(psz_path);
    SG_NULLARGCHECK_RETURN(pValue);

    SG_ERR_CHECK(  SG_STRING__ALLOC(pCtx, &pstr_path)  );
    if ('/' == psz_path[0])
    {
        SG_ERR_CHECK(  SG_string__sprintf(pCtx, pstr_path, "%s", psz_path)  );
    }
    else
    {
        SG_ERR_CHECK(  SG_string__sprintf(pCtx, pstr_path, "%s/%s", SG_LOCALSETTING__SCOPE__MACHINE, psz_path)  );
    }

    SG_ERR_CHECK(  SG_closet__get_localsettings(pCtx, &p)  );

	SG_ERR_CHECK(  SG_jsondb__update__varray(pCtx, p, SG_string__sz(pstr_path), SG_TRUE, pValue)  );

fail:
	SG_JSONDB_NULLFREE(pCtx, p);
    SG_STRING_NULLFREE(pCtx, pstr_path);
}

void SG_localsettings__varray__empty(SG_context * pCtx, const char* psz_path)
{
    SG_varray* pva = NULL;

    SG_ERR_CHECK(  SG_varray__alloc(pCtx, &pva)  );
    SG_ERR_CHECK(  SG_localsettings__update__varray(pCtx, psz_path, pva)  );

fail:
    SG_VARRAY_NULLFREE(pCtx, pva);
}

void SG_localsettings__reset(SG_context * pCtx, const char* psz_path)
{
    SG_jsondb* p = NULL;
    SG_string* pstr_path = NULL;

    SG_ASSERT(pCtx);
    SG_NONEMPTYCHECK_RETURN(psz_path);

    SG_ERR_CHECK(  SG_STRING__ALLOC(pCtx, &pstr_path)  );
    if ('/' == psz_path[0])
    {
        SG_ERR_CHECK(  SG_string__sprintf(pCtx, pstr_path, "%s", psz_path)  );
    }
    else
    {
        SG_ERR_CHECK(  SG_string__sprintf(pCtx, pstr_path, "%s/%s", SG_LOCALSETTING__SCOPE__MACHINE, psz_path)  );
    }

    SG_ERR_CHECK(  SG_closet__get_localsettings(pCtx, &p)  );
	SG_jsondb__remove(pCtx, p, SG_string__sz(pstr_path));
    SG_ERR_CHECK_CURRENT_DISREGARD(SG_ERR_NOT_FOUND);

fail:
	SG_JSONDB_NULLFREE(pCtx, p);
    SG_STRING_NULLFREE(pCtx, pstr_path);
}

#include "sg_localsettings__filetools.h"

const char * DEFAULT_IGNORES[] =
{
	"*~sg*~",
	".*~sg*~",		// .dotfiles are not matched by the first pattern.
	NULL
};
const char * DEFAULT_BINARY_PATTERNS[] =
{
	"*.lib", "*.a",                                          // static libraries
	"*.dll", "*.dylib", "*.so",                              // dynamic libraries
	"*.exe", "*.out",                                        // executables
	"*.jar",                                                 // Java bytecode
	"*.obj", "*.o",                                          // build intermediates
	"*.jpg", "*.gif", "*.png", "*.ico",                      // images
	"*.swf",                                                 // movies
	"*.zip", "*.tar", "*.gz", "*.tgz", "*.bz2", "*.7z",      // compressed files
	"*.iso", "*.dmg",                                        // disk images
	"*.chm", "*.msi", "*.cab", "*.ocx",                      // Windows
	"*.pdf",                                                 // Adobe
	"*.doc", "*.docx", "*.xls", "*.xlsx", "*.ppt", "*.pptx", // MS Office
	"*.ilk", "*.pdb", "*.idb", "*.aps", "*.bsc", "*.ncb",    // Visual Studio
	"*.vmdk", "*.vmem", "*.vmsd", "*.vmss", "*.nvram",       // VMware
	NULL // default array values need NULL terminators
};
const char * DEFAULT_TEXT_PATTERNS[] =
{
	"*.txt", "*.log", "README", "LICENSE",                 // text
	"*.csv",                                               // formatted text
	"*.xml",                                               // XML
	"*.bas",                                               // BASIC
	"*.pas",                                               // Pascal
	"*.c", "*.h",                                          // C
	"*.cpp", "*.cxx", "*.hpp", "*.hxx",                    // C++
	"*.m",                                                 // Objective-C
	"*.cs",                                                // C#
	"*.java",                                              // Java
	"*.vb", "*.cls", "*.frm",                              // Visual Basic
	"*.dfm", "*.xfm", "*.dof", "*.dpk", "*.dpkw", "*.dpr", // Delphi
	"*.rc",                                                // Windows
	"*.resx", "*.xaml", "*.config",                        // .NET
	"*.def",                                               // COM
	"*.sql",                                               // SQL
	"*.js", "*.json",                                      // JavaScript
	"*.vbs",                                               // VBScript
	"*.pl", "*.pm",                                        // Perl
	"*.py",                                                // Python
	"*.rb",                                                // Ruby
	"*.php", "*.php3", "*.inc",                            // PHP
	"*.asp", "*.aspx", "*.ascx", "*.asmx",                 // ASP
	"*.htm", "*.html", "*.shtml", "*.xhtml",               // HTML
	"*.css",                                               // CSS
	"*.svg",                                               // SVG
	"*.sh", "*.bat", "*.cmd", "*.ps1",                     // shell script
	"Makefile",                                            // make
	"*.mak",                                               // nmake
	"*.cmake",                                             // cmake
	"*.sln", "*.vcproj", "*.vcp", "*.vbp",                 // Visual Studio
	"*.vcxproj", "*.csproj", "*.vbproj",                   // MSBuild
	"*.el",                                                // Emacs Lisp
	"*.bpg",                                               // Borland
	"*.qpr", "*.vct",                                      // FoxPro
	"*.disco",                                             // DISCO
	NULL // default array values need NULL terminators
};

const struct localsetting_factory_default
{
    const char* psz_path_partial;
    SG_uint16 type;
    const char* psz_val;
    const char** pasz_array;
    const char** pasz_vals;
} g_factory_defaults[] = {
    {
        SG_LOCALSETTING__NEWREPO_HASHMETHOD,
        SG_VARIANT_TYPE_SZ,
        "SHA1/160",
        NULL,
        NULL,
    },
    {
        SG_LOCALSETTING__IGNORES,
        SG_VARIANT_TYPE_VARRAY,
        NULL,
        DEFAULT_IGNORES,
        NULL,
    },
    {
        SG_LOCALSETTING__LOG_LEVEL,
        SG_VARIANT_TYPE_SZ,
        "quiet",
        NULL,
        NULL,
    },
	{
		SG_FILETOOLCONFIG__CLASSES "/" SG_FILETOOLCONFIG__BINARY_CLASS "/" SG_FILETOOLCONFIG__CLASSPATTERNS,
		SG_VARIANT_TYPE_VARRAY,
		NULL,
		DEFAULT_BINARY_PATTERNS,
		NULL,
	},
	{
		SG_FILETOOLCONFIG__BINDINGS "/" SG_MERGETOOL__TYPE "/" SG_FILETOOLCONFIG__BINARY_CLASS,
		SG_VARIANT_TYPE_SZ,
		SG_MERGETOOL__INTERNAL__SKIP,
		NULL,
		NULL,
	},
	{
		SG_FILETOOLCONFIG__BINDINGS "/" SG_DIFFTOOL__TYPE "/" SG_FILETOOLCONFIG__BINARY_CLASS,
		SG_VARIANT_TYPE_SZ,
		SG_DIFFTOOL__INTERNAL__SKIP,
		NULL,
		NULL,
	},
	{
		SG_FILETOOLCONFIG__CLASSES "/" SG_FILETOOLCONFIG__TEXT_CLASS "/" SG_FILETOOLCONFIG__CLASSPATTERNS,
		SG_VARIANT_TYPE_VARRAY,
		NULL,
		DEFAULT_TEXT_PATTERNS,
		NULL,
	},
	{
		SG_FILETOOLCONFIG__BINDINGS "/" SG_MERGETOOL__TYPE "/" SG_FILETOOLCONFIG__TEXT_CLASS "/" SG_MERGETOOL__CONTEXT__MERGE,
		SG_VARIANT_TYPE_SZ,
		SG_MERGETOOL__INTERNAL__MERGE,
		NULL,
		NULL,
	},
	{
		SG_FILETOOLCONFIG__BINDINGS "/" SG_MERGETOOL__TYPE "/" SG_FILETOOLCONFIG__TEXT_CLASS "/" SG_MERGETOOL__CONTEXT__RESOLVE,
		SG_VARIANT_TYPE_SZ,
		SG_MERGETOOL__INTERNAL__DIFFMERGE,
		NULL,
		NULL,
	},
	{
		SG_FILETOOLCONFIG__BINDINGS "/" SG_DIFFTOOL__TYPE "/" SG_FILETOOLCONFIG__TEXT_CLASS "/" SG_DIFFTOOL__CONTEXT__CONSOLE,
		SG_VARIANT_TYPE_SZ,
		SG_DIFFTOOL__INTERNAL__DIFF_IGNORE_EOLS,
		NULL,
		NULL,
	},
	{
		SG_FILETOOLCONFIG__BINDINGS "/" SG_DIFFTOOL__TYPE "/" SG_FILETOOLCONFIG__TEXT_CLASS "/" SG_DIFFTOOL__CONTEXT__GUI,
		SG_VARIANT_TYPE_SZ,
		SG_DIFFTOOL__INTERNAL__DIFFMERGE,
		NULL,
		NULL,
	},
	{
		SG_LOCALSETTING__TORTOISE_REVERT__SAVE_BACKUPS,
        SG_VARIANT_TYPE_SZ,
        "true",
        NULL,
        NULL,
    },

// declare the basic settings for a difftool
#define DEFAULT_DIFFTOOL(TOOLNAME, TOOLPATHS, TOOLARGS)                                           \
{                                                                                                 \
	SG_FILETOOLCONFIG__TOOLS "/" SG_DIFFTOOL__TYPE "/" TOOLNAME "/" SG_FILETOOLCONFIG__TOOLPATH,  \
	SG_VARIANT_TYPE_VARRAY,                                                                       \
	NULL,                                                                                         \
	TOOLPATHS,                                                                                    \
	NULL,                                                                                         \
},                                                                                                \
{                                                                                                 \
	SG_FILETOOLCONFIG__TOOLS "/" SG_DIFFTOOL__TYPE "/" TOOLNAME "/" SG_FILETOOLCONFIG__TOOLARGS,  \
	SG_VARIANT_TYPE_VARRAY,                                                                       \
	NULL,                                                                                         \
	TOOLARGS,                                                                                     \
	NULL,                                                                                         \
},

// declare the flags settings for a difftool
#define DEFAULT_DIFFTOOL_FLAGS_(TOOLNAME, TOOLFLAGS)                                             \
{                                                                                                 \
	SG_FILETOOLCONFIG__TOOLS "/" SG_DIFFTOOL__TYPE "/" TOOLNAME "/" SG_FILETOOLCONFIG__TOOLFLAGS, \
	SG_VARIANT_TYPE_VARRAY,                                                                       \
	NULL,                                                                                         \
	TOOLFLAGS,                                                                                    \
	NULL,                                                                                         \
},

// declare the basic and flags settings for a difftool
#define DEFAULT_DIFFTOOL_FLAGS(TOOLNAME, TOOLPATHS, TOOLARGS, TOOLFLAGS) \
DEFAULT_DIFFTOOL(TOOLNAME, TOOLPATHS, TOOLARGS)                          \
DEFAULT_DIFFTOOL_FLAGS_(TOOLNAME, TOOLFLAGS)

// declare the exit code settings for a difftool
#define DEFAULT_DIFFTOOL_EXIT_(TOOLNAME, EXITCODES, RESULTCODES)                                      \
{                                                                                                     \
	SG_FILETOOLCONFIG__TOOLS "/" SG_DIFFTOOL__TYPE "/" TOOLNAME "/" SG_FILETOOLCONFIG__TOOLEXITCODES, \
	SG_VARIANT_TYPE_VHASH,                                                                            \
	NULL,                                                                                             \
	EXITCODES,                                                                                        \
	RESULTCODES,                                                                                      \
},

// declare the basic and exit code settings for a difftool
#define DEFAULT_DIFFTOOL_EXIT(TOOLNAME, TOOLPATHS, TOOLARGS, EXITCODES, RESULTCODES) \
DEFAULT_DIFFTOOL(TOOLNAME, TOOLPATHS, TOOLARGS)                                      \
DEFAULT_DIFFTOOL_EXIT_(TOOLNAME, EXITCODES, RESULTCODES)

// declare the basic, flags, and exit code settings for a difftool
#define DEFAULT_DIFFTOOL_FLAGS_EXIT(TOOLNAME, TOOLPATHS, TOOLARGS, TOOLFLAGS, EXITCODES, RESULTCODES) \
DEFAULT_DIFFTOOL(TOOLNAME, TOOLPATHS, TOOLARGS)                                                       \
DEFAULT_DIFFTOOL_FLAGS_(TOOLNAME, TOOLFLAGS)                                                          \
DEFAULT_DIFFTOOL_EXIT_(TOOLNAME, EXITCODES, RESULTCODES)

// declare the basic settings for a mergetool
#define DEFAULT_MERGETOOL(TOOLNAME, TOOLPATHS, TOOLARGS)                                          \
{                                                                                                 \
	SG_FILETOOLCONFIG__TOOLS "/" SG_MERGETOOL__TYPE "/" TOOLNAME "/" SG_FILETOOLCONFIG__TOOLPATH, \
	SG_VARIANT_TYPE_VARRAY,                                                                       \
	NULL,                                                                                         \
	TOOLPATHS,                                                                                    \
	NULL,                                                                                         \
},                                                                                                \
{                                                                                                 \
	SG_FILETOOLCONFIG__TOOLS "/" SG_MERGETOOL__TYPE "/" TOOLNAME "/" SG_FILETOOLCONFIG__TOOLARGS, \
	SG_VARIANT_TYPE_VARRAY,                                                                       \
	NULL,                                                                                         \
	TOOLARGS,                                                                                     \
	NULL,                                                                                         \
},

// declare the flags settings for a mergetool
#define DEFAULT_MERGETOOL_FLAGS_(TOOLNAME, TOOLFLAGS)                                              \
{                                                                                                  \
	SG_FILETOOLCONFIG__TOOLS "/" SG_MERGETOOL__TYPE "/" TOOLNAME "/" SG_FILETOOLCONFIG__TOOLFLAGS, \
	SG_VARIANT_TYPE_VARRAY,                                                                        \
	NULL,                                                                                          \
	TOOLFLAGS,                                                                                     \
	NULL,                                                                                          \
},

// declare the basic and flags settings for a mergetool
#define DEFAULT_MERGETOOL_FLAGS(TOOLNAME, TOOLPATHS, TOOLARGS, TOOLFLAGS) \
DEFAULT_MERGETOOL(TOOLNAME, TOOLPATHS, TOOLARGS)                          \
DEFAULT_MERGETOOL_FLAGS_(TOOLNAME, TOOLFLAGS)

// declare the exit code settings for a mergetool
#define DEFAULT_MERGETOOL_EXIT_(TOOLNAME, EXITCODES, RESULTCODES)                                      \
{                                                                                                      \
	SG_FILETOOLCONFIG__TOOLS "/" SG_MERGETOOL__TYPE "/" TOOLNAME "/" SG_FILETOOLCONFIG__TOOLEXITCODES, \
	SG_VARIANT_TYPE_VHASH,                                                                             \
	NULL,                                                                                              \
	EXITCODES,                                                                                         \
	RESULTCODES,                                                                                       \
},

// declare the basic and exit code settings for a mergetool
#define DEFAULT_MERGETOOL_EXIT(TOOLNAME, TOOLPATHS, TOOLARGS, EXITCODES, RESULTCODES) \
DEFAULT_MERGETOOL(TOOLNAME, TOOLPATHS, TOOLARGS)                                      \
DEFAULT_MERGETOOL_EXIT_(TOOLNAME, EXITCODES, RESULTCODES)

// declare the basic, flags, and exit code settings for a mergetool
#define DEFAULT_MERGETOOL_FLAGS_EXIT(TOOLNAME, TOOLPATHS, TOOLARGS, TOOLFLAGS, EXITCODES, RESULTCODES) \
DEFAULT_MERGETOOL(TOOLNAME, TOOLPATHS, TOOLARGS)                                                       \
DEFAULT_MERGETOOL_FLAGS_(TOOLNAME, TOOLFLAGS)                                                          \
DEFAULT_MERGETOOL_EXIT_(TOOLNAME, EXITCODES, RESULTCODES)

// declare a tool in terms of pre-defined platform-specific settings from sg_localsettings__filetools.h
// the tool being defined may use settings from a different tool, effectively aliasing it
// these macros assume a particular naming scheme for variables/#defines
#if defined(WINDOWS)
#define DECLARE_DIFFTOOL_ALIAS(TOOLNAME, TOOLIMPL) DEFAULT_DIFFTOOL(DIFFTOOL__##TOOLNAME, DIFFTOOL__##TOOLIMPL##__PATHS__WINDOWS, DIFFTOOL__##TOOLIMPL##__ARGS__WINDOWS)
#define DECLARE_DIFFTOOL_ALIAS_FLAGS(TOOLNAME, TOOLIMPL) DEFAULT_DIFFTOOL_FLAGS(DIFFTOOL__##TOOLNAME, DIFFTOOL__##TOOLIMPL##__PATHS__WINDOWS, DIFFTOOL__##TOOLIMPL##__ARGS__WINDOWS, DIFFTOOL__##TOOLIMPL##__FLAGS__WINDOWS)
#define DECLARE_DIFFTOOL_ALIAS_EXIT(TOOLNAME, TOOLIMPL) DEFAULT_DIFFTOOL_EXIT(DIFFTOOL__##TOOLNAME, DIFFTOOL__##TOOLIMPL##__PATHS__WINDOWS, DIFFTOOL__##TOOLIMPL##__ARGS__WINDOWS, DIFFTOOL__##TOOLIMPL##__EXITS__WINDOWS, DIFFTOOL__##TOOLIMPL##__RESULTS__WINDOWS)
#define DECLARE_DIFFTOOL_ALIAS_FLAGS_EXIT(TOOLNAME, TOOLIMPL) DEFAULT_DIFFTOOL_FLAGS_EXIT(DIFFTOOL__##TOOLNAME, DIFFTOOL__##TOOLIMPL##__PATHS__WINDOWS, DIFFTOOL__##TOOLIMPL##__ARGS__WINDOWS, DIFFTOOL__##TOOLIMPL##__FLAGS__WINDOWS, DIFFTOOL__##TOOLIMPL##__EXITS__WINDOWS, DIFFTOOL__##TOOLIMPL##__RESULTS__WINDOWS)
#define DECLARE_MERGETOOL_ALIAS(TOOLNAME, TOOLIMPL) DEFAULT_MERGETOOL(MERGETOOL__##TOOLNAME, MERGETOOL__##TOOLIMPL##__PATHS__WINDOWS, MERGETOOL__##TOOLIMPL##__ARGS__WINDOWS)
#define DECLARE_MERGETOOL_ALIAS_FLAGS(TOOLNAME, TOOLIMPL) DEFAULT_MERGETOOL_FLAGS(MERGETOOL__##TOOLNAME, MERGETOOL__##TOOLIMPL##__PATHS__WINDOWS, MERGETOOL__##TOOLIMPL##__ARGS__WINDOWS, MERGETOOL__##TOOLIMPL##__FLAGS__WINDOWS)
#define DECLARE_MERGETOOL_ALIAS_EXIT(TOOLNAME, TOOLIMPL) DEFAULT_MERGETOOL_EXIT(MERGETOOL__##TOOLNAME, MERGETOOL__##TOOLIMPL##__PATHS__WINDOWS, MERGETOOL__##TOOLIMPL##__ARGS__WINDOWS, MERGETOOL__##TOOLIMPL##__EXITS__WINDOWS, MERGETOOL__##TOOLIMPL##__RESULTS__WINDOWS)
#define DECLARE_MERGETOOL_ALIAS_FLAGS_EXIT(TOOLNAME, TOOLIMPL) DEFAULT_MERGETOOL_FLAGS_EXIT(MERGETOOL__##TOOLNAME, MERGETOOL__##TOOLIMPL##__PATHS__WINDOWS, MERGETOOL__##TOOLIMPL##__ARGS__WINDOWS, MERGETOOL__##TOOLIMPL##__FLAGS__WINDOWS, MERGETOOL__##TOOLIMPL##__EXITS__WINDOWS, MERGETOOL__##TOOLIMPL##__RESULTS__WINDOWS)
#elif defined(MAC)
#define DECLARE_DIFFTOOL_ALIAS(TOOLNAME, TOOLIMPL) DEFAULT_DIFFTOOL(DIFFTOOL__##TOOLNAME, DIFFTOOL__##TOOLIMPL##__PATHS__MAC, DIFFTOOL__##TOOLIMPL##__ARGS__MAC)
#define DECLARE_DIFFTOOL_ALIAS_FLAGS(TOOLNAME, TOOLIMPL) DEFAULT_DIFFTOOL_FLAGS(DIFFTOOL__##TOOLNAME, DIFFTOOL__##TOOLIMPL##__PATHS__MAC, DIFFTOOL__##TOOLIMPL##__ARGS__MAC, DIFFTOOL__##TOOLIMPL##__FLAGS__MAC)
#define DECLARE_DIFFTOOL_ALIAS_EXIT(TOOLNAME, TOOLIMPL) DEFAULT_DIFFTOOL_EXIT(DIFFTOOL__##TOOLNAME, DIFFTOOL__##TOOLIMPL##__PATHS__MAC, DIFFTOOL__##TOOLIMPL##__ARGS__MAC, DIFFTOOL__##TOOLIMPL##__EXITS__MAC, DIFFTOOL__##TOOLIMPL##__RESULTS__MAC)
#define DECLARE_DIFFTOOL_ALIAS_FLAGS_EXIT(TOOLNAME, TOOLIMPL) DEFAULT_DIFFTOOL_FLAGS_EXIT(DIFFTOOL__##TOOLNAME, DIFFTOOL__##TOOLIMPL##__PATHS__MAC, DIFFTOOL__##TOOLIMPL##__ARGS__MAC, DIFFTOOL__##TOOLIMPL##__FLAGS__MAC, DIFFTOOL__##TOOLIMPL##__EXITS__MAC, DIFFTOOL__##TOOLIMPL##__RESULTS__MAC)
#define DECLARE_MERGETOOL_ALIAS(TOOLNAME, TOOLIMPL) DEFAULT_MERGETOOL(MERGETOOL__##TOOLNAME, MERGETOOL__##TOOLIMPL##__PATHS__MAC, MERGETOOL__##TOOLIMPL##__ARGS__MAC)
#define DECLARE_MERGETOOL_ALIAS_FLAGS(TOOLNAME, TOOLIMPL) DEFAULT_MERGETOOL_FLAGS(MERGETOOL__##TOOLNAME, MERGETOOL__##TOOLIMPL##__PATHS__MAC, MERGETOOL__##TOOLIMPL##__ARGS__MAC, MERGETOOL__##TOOLIMPL##__FLAGS__MAC)
#define DECLARE_MERGETOOL_ALIAS_EXIT(TOOLNAME, TOOLIMPL) DEFAULT_MERGETOOL_EXIT(MERGETOOL__##TOOLNAME, MERGETOOL__##TOOLIMPL##__PATHS__MAC, MERGETOOL__##TOOLIMPL##__ARGS__MAC, MERGETOOL__##TOOLIMPL##__EXITS__MAC, MERGETOOL__##TOOLIMPL##__RESULTS__MAC)
#define DECLARE_MERGETOOL_ALIAS_FLAGS_EXIT(TOOLNAME, TOOLIMPL) DEFAULT_MERGETOOL_FLAGS_EXIT(MERGETOOL__##TOOLNAME, MERGETOOL__##TOOLIMPL##__PATHS__MAC, MERGETOOL__##TOOLIMPL##__ARGS__MAC, MERGETOOL__##TOOLIMPL##__FLAGS__MAC, MERGETOOL__##TOOLIMPL##__EXITS__MAC, MERGETOOL__##TOOLIMPL##__RESULTS__MAC)
#elif defined(LINUX)
#define DECLARE_DIFFTOOL_ALIAS(TOOLNAME, TOOLIMPL) DEFAULT_DIFFTOOL(DIFFTOOL__##TOOLNAME, DIFFTOOL__##TOOLIMPL##__PATHS__LINUX, DIFFTOOL__##TOOLIMPL##__ARGS__LINUX)
#define DECLARE_DIFFTOOL_ALIAS_FLAGS(TOOLNAME, TOOLIMPL) DEFAULT_DIFFTOOL_FLAGS(DIFFTOOL__##TOOLNAME, DIFFTOOL__##TOOLIMPL##__PATHS__LINUX, DIFFTOOL__##TOOLIMPL##__ARGS__LINUX, DIFFTOOL__##TOOLIMPL##__FLAGS__LINUX)
#define DECLARE_DIFFTOOL_ALIAS_EXIT(TOOLNAME, TOOLIMPL) DEFAULT_DIFFTOOL_EXIT(DIFFTOOL__##TOOLNAME, DIFFTOOL__##TOOLIMPL##__PATHS__LINUX, DIFFTOOL__##TOOLIMPL##__ARGS__LINUX, DIFFTOOL__##TOOLIMPL##__EXITS__LINUX, DIFFTOOL__##TOOLIMPL##__RESULTS__LINUX)
#define DECLARE_DIFFTOOL_ALIAS_FLAGS_EXIT(TOOLNAME, TOOLIMPL) DEFAULT_DIFFTOOL_FLAGS_EXIT(DIFFTOOL__##TOOLNAME, DIFFTOOL__##TOOLIMPL##__PATHS__LINUX, DIFFTOOL__##TOOLIMPL##__ARGS__LINUX, DIFFTOOL__##TOOLIMPL##__FLAGS__LINUX, DIFFTOOL__##TOOLIMPL##__EXITS__LINUX, DIFFTOOL__##TOOLIMPL##__RESULTS__LINUX)
#define DECLARE_MERGETOOL_ALIAS(TOOLNAME, TOOLIMPL) DEFAULT_MERGETOOL(MERGETOOL__##TOOLNAME, MERGETOOL__##TOOLIMPL##__PATHS__LINUX, MERGETOOL__##TOOLIMPL##__ARGS__LINUX)
#define DECLARE_MERGETOOL_ALIAS_FLAGS(TOOLNAME, TOOLIMPL) DEFAULT_MERGETOOL_FLAGS(MERGETOOL__##TOOLNAME, MERGETOOL__##TOOLIMPL##__PATHS__LINUX, MERGETOOL__##TOOLIMPL##__ARGS__LINUX, MERGETOOL__##TOOLIMPL##__FLAGS__LINUX)
#define DECLARE_MERGETOOL_ALIAS_EXIT(TOOLNAME, TOOLIMPL) DEFAULT_MERGETOOL_EXIT(MERGETOOL__##TOOLNAME, MERGETOOL__##TOOLIMPL##__PATHS__LINUX, MERGETOOL__##TOOLIMPL##__ARGS__LINUX, MERGETOOL__##TOOLIMPL##__EXITS__LINUX, MERGETOOL__##TOOLIMPL##__RESULTS__LINUX)
#define DECLARE_MERGETOOL_ALIAS_FLAGS_EXIT(TOOLNAME, TOOLIMPL) DEFAULT_MERGETOOL_FLAGS_EXIT(MERGETOOL__##TOOLNAME, MERGETOOL__##TOOLIMPL##__PATHS__LINUX, MERGETOOL__##TOOLIMPL##__ARGS__LINUX, MERGETOOL__##TOOLIMPL##__FLAGS__LINUX, MERGETOOL__##TOOLIMPL##__EXITS__LINUX, MERGETOOL__##TOOLIMPL##__RESULTS__LINUX)
#endif

// as the platform-specific declarations above, but the tool always uses its own settings
#define DECLARE_DIFFTOOL(TOOLNAME) DECLARE_DIFFTOOL_ALIAS(TOOLNAME, TOOLNAME)
#define DECLARE_DIFFTOOL_FLAGS(TOOLNAME) DECLARE_DIFFTOOL_ALIAS_FLAGS(TOOLNAME, TOOLNAME)
#define DECLARE_DIFFTOOL_EXIT(TOOLNAME) DECLARE_DIFFTOOL_ALIAS_EXIT(TOOLNAME, TOOLNAME)
#define DECLARE_DIFFTOOL_FLAGS_EXIT(TOOLNAME) DECLARE_DIFFTOOL_ALIAS_FLAGS_EXIT(TOOLNAME, TOOLNAME)
#define DECLARE_MERGETOOL(TOOLNAME) DECLARE_MERGETOOL_ALIAS(TOOLNAME, TOOLNAME)
#define DECLARE_MERGETOOL_FLAGS(TOOLNAME) DECLARE_MERGETOOL_ALIAS_FLAGS(TOOLNAME, TOOLNAME)
#define DECLARE_MERGETOOL_EXIT(TOOLNAME) DECLARE_MERGETOOL_ALIAS_EXIT(TOOLNAME, TOOLNAME)
#define DECLARE_MERGETOOL_FLAGS_EXIT(TOOLNAME) DECLARE_MERGETOOL_ALIAS_FLAGS_EXIT(TOOLNAME, TOOLNAME)

// declare tools for each platform
#if defined(WINDOWS)
DECLARE_DIFFTOOL(ARAXIS)
DECLARE_DIFFTOOL_EXIT(BCOMPARE)
DECLARE_DIFFTOOL(DIFFMERGE)
DECLARE_DIFFTOOL_ALIAS(DIFFUSE, DIFFUSE_045)
DECLARE_DIFFTOOL(ECMERGE)
DECLARE_DIFFTOOL_EXIT(KDIFF3)
DECLARE_DIFFTOOL(P4MERGE)
DECLARE_DIFFTOOL_EXIT(TKDIFF)
DECLARE_DIFFTOOL(TORTOISEMERGE)
DECLARE_DIFFTOOL(VIMDIFF)
DECLARE_DIFFTOOL(GVIMDIFF)
DECLARE_MERGETOOL(ARAXIS)
DECLARE_MERGETOOL_EXIT(BCOMPARE)
DECLARE_MERGETOOL_EXIT(DIFFMERGE)
DECLARE_MERGETOOL_ALIAS_FLAGS(DIFFUSE, DIFFUSE_045)
DECLARE_MERGETOOL_EXIT(ECMERGE)
DECLARE_MERGETOOL_EXIT(KDIFF3)
DECLARE_MERGETOOL_FLAGS(P4MERGE)
DECLARE_MERGETOOL_EXIT(TKDIFF)
DECLARE_MERGETOOL(TORTOISEMERGE)
DECLARE_MERGETOOL_FLAGS(VIMDIFF)
DECLARE_MERGETOOL_FLAGS(GVIMDIFF)
#elif defined(MAC)
DECLARE_DIFFTOOL_EXIT(ARAXIS)
DECLARE_DIFFTOOL(DIFFMERGE)
DECLARE_DIFFTOOL_ALIAS(DIFFUSE, DIFFUSE_045)
DECLARE_DIFFTOOL(ECMERGE)
DECLARE_DIFFTOOL_EXIT(KDIFF3)
DECLARE_DIFFTOOL(MELD)
DECLARE_DIFFTOOL_FLAGS(P4MERGE)
DECLARE_DIFFTOOL_EXIT(TKDIFF)
DECLARE_DIFFTOOL(VIMDIFF)
DECLARE_DIFFTOOL(GVIMDIFF)
DECLARE_MERGETOOL(ARAXIS)
DECLARE_MERGETOOL_EXIT(DIFFMERGE)
DECLARE_MERGETOOL_ALIAS_FLAGS(DIFFUSE, DIFFUSE_045)
DECLARE_MERGETOOL_EXIT(ECMERGE)
DECLARE_MERGETOOL_EXIT(KDIFF3)
DECLARE_MERGETOOL_ALIAS(MELD, MELD_132)
DECLARE_MERGETOOL_FLAGS(P4MERGE)
DECLARE_MERGETOOL_EXIT(TKDIFF)
DECLARE_MERGETOOL_FLAGS(VIMDIFF)
DECLARE_MERGETOOL_FLAGS(GVIMDIFF)
#elif defined(LINUX)
DECLARE_DIFFTOOL_EXIT(BCOMPARE)
DECLARE_DIFFTOOL(DIFFMERGE)
DECLARE_DIFFTOOL(DIFFUSE)
DECLARE_DIFFTOOL(DIFFUSE_045)
DECLARE_DIFFTOOL(ECMERGE)
DECLARE_DIFFTOOL_EXIT(KDIFF3)
DECLARE_DIFFTOOL(MELD)
DECLARE_DIFFTOOL(P4MERGE)
DECLARE_DIFFTOOL_EXIT(TKDIFF)
DECLARE_DIFFTOOL(VIMDIFF)
DECLARE_DIFFTOOL(GVIMDIFF)
DECLARE_MERGETOOL_EXIT(BCOMPARE)
DECLARE_MERGETOOL_EXIT(DIFFMERGE)
DECLARE_MERGETOOL_FLAGS(DIFFUSE)
DECLARE_MERGETOOL_FLAGS(DIFFUSE_045)
DECLARE_MERGETOOL_EXIT(ECMERGE)
DECLARE_MERGETOOL_EXIT(KDIFF3)
DECLARE_MERGETOOL_FLAGS(MELD)
DECLARE_MERGETOOL(MELD_132)
DECLARE_MERGETOOL_FLAGS(P4MERGE)
DECLARE_MERGETOOL_EXIT(TKDIFF)
DECLARE_MERGETOOL_FLAGS(VIMDIFF)
DECLARE_MERGETOOL_FLAGS(GVIMDIFF)
#endif

};

/**
 * Build a constant vhash of factory defaults
 * from the above declarations.
 *
 */
static void _alloc__factory__list__vhash(
    SG_context* pCtx,
    SG_vhash** ppvh
    )
{
    SG_uint32 i = 0;
    SG_vhash* pvh = NULL;
    SG_varray* pva = NULL;
    SG_vhash* psvh = NULL;

#if TRACE_LS
	SG_ERR_IGNORE(  SG_console(pCtx, SG_CS_STDERR, "TraceLS: in __factory__list__vhash: begin\n")  );
#endif

    SG_ERR_CHECK(  SG_vhash__alloc(pCtx, &pvh)  );
    for (i=0; i<(sizeof(g_factory_defaults) / sizeof(struct localsetting_factory_default)); i++)
    {
        if (SG_VARIANT_TYPE_SZ == g_factory_defaults[i].type)
        {
            SG_ERR_CHECK(  SG_vhash__slashpath__update__string__sz(pCtx, pvh, g_factory_defaults[i].psz_path_partial, g_factory_defaults[i].psz_val)  );
        }
        else if (SG_VARIANT_TYPE_VARRAY == g_factory_defaults[i].type)
        {
            const char** pp_el = NULL;

            SG_ERR_CHECK(  SG_varray__alloc(pCtx, &pva)  );

            pp_el = g_factory_defaults[i].pasz_array;
            while (*pp_el)
            {
                SG_ERR_CHECK(  SG_varray__append__string__sz(pCtx, pva, *pp_el)  );
                pp_el++;
            }
            SG_ERR_CHECK(  SG_vhash__slashpath__update__varray(pCtx, pvh, g_factory_defaults[i].psz_path_partial, &pva)  );
        }
        else if (SG_VARIANT_TYPE_VHASH == g_factory_defaults[i].type)
        {
            const char** ppKeys = g_factory_defaults[i].pasz_array;
            const char** ppVals = g_factory_defaults[i].pasz_vals;

            SG_ERR_CHECK(  SG_VHASH__ALLOC(pCtx,&psvh)  );

            while (*ppKeys != NULL)
            {
                SG_ASSERT(*ppVals != NULL);
                SG_ERR_CHECK(  SG_vhash__add__string__sz(pCtx, psvh, *ppKeys, *ppVals)  );
                ++ppKeys;
                ++ppVals;
            }
            SG_ASSERT(*ppVals == NULL);
            SG_ERR_CHECK(  SG_vhash__slashpath__update__vhash(pCtx, pvh, g_factory_defaults[i].psz_path_partial, &psvh)  );
            psvh = NULL;
        }
        else
        {
            SG_ERR_THROW(  SG_ERR_NOTIMPLEMENTED  );
        }

        }

#if TRACE_LS
	SG_ERR_IGNORE(  SG_vhash_debug__dump_to_console(pCtx, pvh)  );
	SG_ERR_IGNORE(  SG_console(pCtx, SG_CS_STDERR, "TraceLS: in __factory__list__vhash: return\n")  );
#endif

    *ppvh = pvh;
    pvh = NULL;

fail:
    SG_VHASH_NULLFREE(pCtx, pvh);
    SG_VHASH_NULLFREE(pCtx, psvh);
}

//////////////////////////////////////////////////////////////////

static SG_vhash * gpvhFactoryList = NULL;

void sg_localsettings__global_initialize(SG_context * pCtx)
{
	// Defer allocating factory list until someone needs it.
	// It also solves a chicken-n-egg problem in SG_lib.c

	SG_UNUSED( pCtx );
}

void sg_localsettings__global_cleanup(SG_context * pCtx)
{
	SG_VHASH_NULLFREE(pCtx, gpvhFactoryList);
}

/**
 * Return a handle to the CONSTANT LIST OF BUILTIN SETTINGS.
 * 
 */
void SG_localsettings__factory__list__vhash(
    SG_context* pCtx,
    const SG_vhash** ppvh
    )
{
	SG_UNUSED( pCtx );

	if (!gpvhFactoryList)
		SG_ERR_CHECK_RETURN(  _alloc__factory__list__vhash(pCtx, &gpvhFactoryList)  );

	*ppvh = gpvhFactoryList;
}

//////////////////////////////////////////////////////////////////

void SG_localsettings__descriptor__get__sz(
	SG_context * pCtx,
	const char * psz_descriptor_name,
	const char * psz_path,
	char ** ppszValue)
{
	SG_jsondb* p = NULL;
	SG_string* pstr_path = NULL;

	SG_ASSERT(pCtx);
	SG_NONEMPTYCHECK_RETURN(psz_descriptor_name);
	SG_NONEMPTYCHECK_RETURN(psz_path);
	SG_ARGCHECK_RETURN('/' != psz_path[0], psz_path);

	SG_ERR_CHECK(  SG_STRING__ALLOC(pCtx, &pstr_path)  );
	SG_ERR_CHECK(  SG_string__sprintf(pCtx, pstr_path, "%s/%s/%s",
		SG_LOCALSETTING__SCOPE__INSTANCE, psz_descriptor_name, psz_path)  );

	SG_ERR_CHECK(  SG_closet__get_localsettings(pCtx, &p)  );
#if TRACE_LS
	SG_ERR_IGNORE(  SG_console(pCtx, SG_CS_STDERR, "TraceLS: in __descriptor__get__sz: %s\n",
							   SG_string__sz(pstr_path))  );
#endif
	SG_ERR_CHECK(  SG_jsondb__get__sz(pCtx, p, SG_string__sz(pstr_path), ppszValue)  );

fail:
	SG_STRING_NULLFREE(pCtx, pstr_path);
	SG_JSONDB_NULLFREE(pCtx, p);
}

static void _system_wide_config__get__pathname(SG_context* pCtx, SG_bool bCreateDir, SG_pathname** ppPathname)
{
	SG_pathname* pPathMachineConfig = NULL;
	SG_ERR_CHECK(  SG_PATHNAME__ALLOC__MACHINE_CONFIG_DIRECTORY(pCtx, &pPathMachineConfig)  );

	if (bCreateDir)
	{
		SG_fsobj__mkdir_recursive__pathname(pCtx, pPathMachineConfig);
		SG_ERR_CHECK_CURRENT_DISREGARD(SG_ERR_DIR_ALREADY_EXISTS);
	}

	SG_ERR_CHECK(  SG_pathname__append__from_sz(pCtx, pPathMachineConfig, MACHINE_CONFIG_FILENAME)  );

	*ppPathname = pPathMachineConfig;
	return;

fail:
	SG_PATHNAME_NULLFREE(pCtx, pPathMachineConfig);
}

static void _system_wide_config__get__vhash(SG_context* pCtx, SG_vhash** ppvh)
{
	SG_pathname* pPathMachineConfig = NULL;
	SG_bool bMachineConfigFileExists = SG_FALSE;
	SG_vhash* pvhMachineConfig = NULL;

	SG_ERR_CHECK(  _system_wide_config__get__pathname(pCtx, SG_FALSE, &pPathMachineConfig)  );
	SG_ERR_CHECK(  SG_fsobj__exists__pathname(pCtx, pPathMachineConfig, &bMachineConfigFileExists, NULL, NULL)  );
	if (bMachineConfigFileExists)
	{
		SG_ERR_CHECK(  SG_vfile__slurp(pCtx, pPathMachineConfig, &pvhMachineConfig)  );
		*ppvh = pvhMachineConfig;
	}

	SG_PATHNAME_NULLFREE(pCtx, pPathMachineConfig);

	return;

fail:
	SG_PATHNAME_NULLFREE(pCtx, pPathMachineConfig);
	SG_VHASH_NULLFREE(pCtx, pvhMachineConfig);
}

static void _system_wide_config__get__value(
	SG_context* pCtx, 
	const char* pszFullpath,
	SG_variant** ppvValue)
{
	SG_vhash* pvhMachineConfig = NULL;
	const SG_variant* pv = NULL;

	SG_ERR_CHECK(  _system_wide_config__get__vhash(pCtx, &pvhMachineConfig)  );

	if (pvhMachineConfig)
	{
		const char* pszRefSafePath = pszFullpath;
		if ('/' == pszRefSafePath[0])
			pszRefSafePath++;
		SG_ERR_CHECK(  SG_vhash__slashpath__get__variant(pCtx, pvhMachineConfig, pszRefSafePath, &pv)  );
		if (pv)
			SG_ERR_CHECK(  SG_variant__copy(pCtx, pv, ppvValue)  );
		else
			*ppvValue = NULL;
	}

	/* Common cleanup */
fail:
	SG_VHASH_NULLFREE(pCtx, pvhMachineConfig);
}

/**
 * Retrieves the value of a setting, given its fully-scoped name.
 */
static void _get_variant(
	SG_context*  pCtx,       //< [in] [out] Error and context info.
	SG_jsondb*   pSettings,  //< [in] Settings database to retrieve the value from.
	const char*  szFullName, //< [in] Fully-scoped name of the setting to retrieve.
	SG_variant** ppValue     //< [out] The retrieve value.  Caller owns this value.
	                         //<       If set to NULL, then no value was found for the given setting.
	)
{
	SG_variant* pValue              = NULL;
	const SG_vhash* pDefaults       = NULL;
	SG_uint32   uDefaultScopeLength = 0u;

#if TRACE_LS
	SG_ERR_IGNORE(  SG_console(pCtx, SG_CS_STDERR, "TraceLS: in _get_variant: %s\n", szFullName)  );
#endif

	// check if the fully-scoped name indicates a default hard-coded setting
	uDefaultScopeLength = SG_STRLEN(SG_LOCALSETTING__SCOPE__DEFAULT);
	if (strncmp(szFullName, SG_LOCALSETTING__SCOPE__DEFAULT, uDefaultScopeLength) == 0)
	{
		const SG_variant* pHashValue = NULL;

		// look up the default setting
		SG_ERR_CHECK(  SG_localsettings__factory__list__vhash(pCtx, &pDefaults)  );
		SG_ERR_CHECK(  SG_vhash__slashpath__get__variant(pCtx, pDefaults, szFullName + uDefaultScopeLength + 1u, &pHashValue)  );
		if (pHashValue != NULL)
		{
			SG_ERR_CHECK(  SG_variant__copy(pCtx, pHashValue, &pValue)  );
		}
	}
	else
	{
		SG_bool bHasValue = SG_FALSE;

		// look up the full name directly in our database
		SG_ERR_CHECK(  SG_jsondb__has(pCtx, pSettings, szFullName, &bHasValue)  );
		if (bHasValue == SG_TRUE)
		{
			SG_ERR_CHECK(  SG_jsondb__get__variant(pCtx, pSettings, szFullName, &pValue)  );
		}
		else
		{
			// if it wasn't there, try the system-wide config file
			SG_ERR_CHECK(  _system_wide_config__get__value(pCtx, szFullName, &pValue)  );
		}
	}

#if TRACE_LS
	SG_ERR_IGNORE(  SG_console(pCtx, SG_CS_STDERR, "TraceLS: in _get_variant: return\n")  );
#endif

	// return whatever we found
	*ppValue = pValue;
	pValue = NULL;

fail:
	SG_VARIANT_NULLFREE(pCtx, pValue);
}

#if defined(DEBUG)
#define _GET_VARIANT(pCtx,pSettings,szFullname,ppValue)		SG_STATEMENT(  SG_variant * _p = NULL; \
																		   _get_variant(pCtx,pSettings,szFullname,&_p);	\
																		   _sg_mem__set_caller_data(_p,__FILE__,__LINE__,"SG_variant");	\
																		   *(ppValue) = _p;  )
#else
#define _GET_VARIANT(pCtx,pSettings,szFullname,ppValue)		_get_variant(pCtx,pSettings,szFullname,ppValue)
#endif


/**
 * Type of a helper function that gets a setting value from a specific scope.
 */
typedef void (*_get_variant__scope)(
	SG_context*  pCtx,      //< [in] [out] Error and context info.
	SG_jsondb*   pSettings, //< [in] Settings database to retrieve the value from.
	const char*  szName,    //< [in] The non-scoped name of the setting to build a full name for.
	SG_repo*     pRepo,     //< [in] The repo to build the scoped name for.
	SG_variant** ppValue,   //< [out] The retrieved value.  Caller owns this value.
	                        //<       If set to NULL, then no value was found for the given setting name.
	SG_string**  ppFullName //< [out] The fully-scoped name of the setting.
	                        //<       If NULL, then the name will not be returned.
	                        //<       If non-NULL, caller owns the returned value.
	);

/**
 * A _get_variant__scope function that gets values from the "instance" scope.
 */
static void _get_variant__instance(
	SG_context*  pCtx,
	SG_jsondb*   pSettings,
	const char*  szName,
	SG_repo*     pRepo,
	SG_variant** ppValue,
	SG_string**  ppFullName
	)
{
	const char* szDescriptor = NULL;
	SG_string*  pFullName    = NULL;

	SG_NULLARGCHECK(pSettings);
	SG_NULLARGCHECK(szName);
	SG_NULLARGCHECK(ppValue);

	if (pRepo == NULL)
	{
		if (ppFullName != NULL)
		{
			*ppFullName = NULL;
		}
		return;
	}

	SG_STRING__ALLOC(pCtx, &pFullName);

	SG_ERR_CHECK(  SG_repo__get_descriptor_name(pCtx, pRepo, &szDescriptor)  );
	SG_ERR_CHECK(  SG_string__sprintf(pCtx, pFullName, "%s/%s/%s", SG_LOCALSETTING__SCOPE__INSTANCE, szDescriptor, szName)  );
	SG_ERR_CHECK(  _GET_VARIANT(pCtx, pSettings, SG_string__sz(pFullName), ppValue)  );

	if (ppFullName != NULL)
	{
		*ppFullName = pFullName;
		pFullName = NULL;
	}

fail:
	SG_STRING_NULLFREE(pCtx, pFullName);
}

/**
 * A _get_variant__scope function that gets value from the "repo" scope.
 */
static void _get_variant__repo(
	SG_context*  pCtx,
	SG_jsondb*   pSettings,
	const char*  szName,
	SG_repo*     pRepo,
	SG_variant** ppValue,
	SG_string**  ppFullName
	)
{
	char*      szRepoId  = NULL;
	SG_string* pFullName = NULL;

	SG_NULLARGCHECK(pSettings);
	SG_NULLARGCHECK(szName);
	SG_NULLARGCHECK(ppValue);

	if (pRepo == NULL)
	{
		if (ppFullName != NULL)
		{
			*ppFullName = NULL;
		}
		return;
	}

	SG_STRING__ALLOC(pCtx, &pFullName);

	SG_ERR_CHECK(  SG_repo__get_repo_id(pCtx, pRepo, &szRepoId)  );
	SG_ERR_CHECK(  SG_string__sprintf(pCtx, pFullName, "%s/%s/%s", SG_LOCALSETTING__SCOPE__REPO, szRepoId, szName)  );
	SG_ERR_CHECK(  _GET_VARIANT(pCtx, pSettings, SG_string__sz(pFullName), ppValue)  );

	if (ppFullName != NULL)
	{
		*ppFullName = pFullName;
		pFullName = NULL;
	}

fail:
	SG_NULLFREE(pCtx, szRepoId);
	SG_STRING_NULLFREE(pCtx, pFullName);
}

/**
 * A _get_variant__scope function that gets value from the "admin" scope.
 */
static void _get_variant__admin(
	SG_context*  pCtx,
	SG_jsondb*   pSettings,
	const char*  szName,
	SG_repo*     pRepo,
	SG_variant** ppValue,
	SG_string**  ppFullName
	)
{
	char*      szAdminId = NULL;
	SG_string* pFullName = NULL;

	SG_NULLARGCHECK(pSettings);
	SG_NULLARGCHECK(szName);
	SG_NULLARGCHECK(ppValue);

	if (pRepo == NULL)
	{
		if (ppFullName != NULL)
		{
			*ppFullName = NULL;
		}
		return;
	}

	SG_STRING__ALLOC(pCtx, &pFullName);

	SG_ERR_CHECK(  SG_repo__get_admin_id(pCtx, pRepo, &szAdminId)  );
	SG_ERR_CHECK(  SG_string__sprintf(pCtx, pFullName, "%s/%s/%s", SG_LOCALSETTING__SCOPE__ADMIN, szAdminId, szName)  );
	SG_ERR_CHECK(  _GET_VARIANT(pCtx, pSettings, SG_string__sz(pFullName), ppValue)  );

	if (ppFullName != NULL)
	{
		*ppFullName = pFullName;
		pFullName = NULL;
	}

fail:
	SG_NULLFREE(pCtx, szAdminId);
	SG_STRING_NULLFREE(pCtx, pFullName);
}

/**
 * A _get_variant__scope function that gets value from the "machine" scope.
 */
static void _get_variant__machine(
	SG_context*  pCtx,
	SG_jsondb*   pSettings,
	const char*  szName,
	SG_repo*     pRepo,
	SG_variant** ppValue,
	SG_string**  ppFullName
	)
{
	SG_string* pFullName = NULL;

	SG_NULLARGCHECK(pSettings);
	SG_NULLARGCHECK(szName);
	SG_UNUSED(pRepo);
	SG_NULLARGCHECK(ppValue);

	SG_STRING__ALLOC(pCtx, &pFullName);

	SG_ERR_CHECK(  SG_string__sprintf(pCtx, pFullName, "%s/%s", SG_LOCALSETTING__SCOPE__MACHINE, szName)  );
	SG_ERR_CHECK(  _GET_VARIANT(pCtx, pSettings, SG_string__sz(pFullName), ppValue)  );

	if (ppFullName != NULL)
	{
		*ppFullName = pFullName;
		pFullName = NULL;
	}

fail:
	SG_STRING_NULLFREE(pCtx, pFullName);
}

/**
 * A _get_variant__scope function that gets value from the "default" scope.
 */
static void _get_variant__default(
	SG_context*  pCtx,
	SG_jsondb*   pSettings,
	const char*  szName,
	SG_repo*     pRepo,
	SG_variant** ppValue,
	SG_string**  ppFullName
	)
{
	SG_string*        pFullName = NULL;
	const SG_vhash*   pDefaults = NULL;
	const SG_variant* pValue    = NULL;

	SG_UNUSED(pSettings);
	SG_NULLARGCHECK(szName);
	SG_UNUSED(pRepo);
	SG_NULLARGCHECK(ppValue);

#if TRACE_LS
	SG_ERR_IGNORE(  SG_console(pCtx, SG_CS_STDERR, "TraceLS: in _get_variant__default: %s\n",
							   szName)  );
#endif

	/* This routine retrieves "factory defaults" so it doesn't look for the system-wide config file. */

	SG_ERR_CHECK(  SG_STRING__ALLOC(pCtx, &pFullName)  );

	SG_ERR_CHECK(  SG_string__sprintf(pCtx, pFullName, "%s/%s", SG_LOCALSETTING__SCOPE__DEFAULT, szName)  );
	SG_ERR_CHECK(  SG_localsettings__factory__list__vhash(pCtx, &pDefaults)  );
	SG_ERR_CHECK(  SG_vhash__slashpath__get__variant(pCtx, pDefaults, szName, &pValue)  );
	if (pValue == NULL)
	{
		*ppValue = NULL;
	}
	else
	{
		SG_ERR_CHECK(  SG_variant__copy(pCtx, pValue, ppValue)  );
	}

	if (ppFullName != NULL)
	{
		*ppFullName = pFullName;
		pFullName = NULL;
	}

#if TRACE_LS
	SG_ERR_IGNORE(  SG_console(pCtx, SG_CS_STDERR, "TraceLS: in _get_variant__default: return\n")  );
#endif

fail:
	SG_STRING_NULLFREE(pCtx, pFullName);
}

// list of functions that retrieve a value from a particular scope.
// they are listed in the order that we should try them when looking
// for a setting's value (most peronsonalized to most general).
static _get_variant__scope gafScopes[] = {
	_get_variant__instance,
	_get_variant__repo,
	_get_variant__admin,
	_get_variant__machine,
	_get_variant__default,
};

// this table must match the one above.
static const char * gaszScopeShortName[] = {
	SG_LOCALSETTING__SCOPE__INSTANCE,
	SG_LOCALSETTING__SCOPE__REPO,
	SG_LOCALSETTING__SCOPE__ADMIN,
	SG_LOCALSETTING__SCOPE__MACHINE,
	SG_LOCALSETTING__SCOPE__DEFAULT,
};

void SG_localsettings__get__variant(
	SG_context*  pCtx,
	const char*  szName,
	SG_repo*     pRepo,
	SG_variant** ppValue,
	SG_string**  ppFullName
	)
{
	SG_jsondb*  pSettings = NULL;
	SG_string*  pFullName = NULL;
	SG_variant* pValue    = NULL;

	SG_ASSERT(pCtx);
	SG_NONEMPTYCHECK_RETURN(szName);
	SG_NULLARGCHECK(ppValue);

#if TRACE_LS
	SG_ERR_IGNORE(  SG_console(pCtx, SG_CS_STDERR, "TraceLS: in SG_localsettings__get_variant: %s\n",
							   szName)  );
#endif

	// load the settings
	SG_ERR_CHECK(  SG_closet__get_localsettings(pCtx, &pSettings)  );

	if (szName[0] == '/')
	{
		// if they gave us a name that starts with a slash
		// assume that it's a fully-scoped name and just look it up directly
		SG_ERR_CHECK(  SG_STRING__ALLOC__SZ(pCtx, &pFullName, szName)  );
		SG_ERR_CHECK(  _GET_VARIANT(pCtx, pSettings, szName, &pValue)  );
	}
	else
	{
		// otherwise, we'll have to run through our scopes
		// and use the first value we find
		SG_uint32 uIndex = 0u;

		// check each scope until we run out or find a value.
		// we start with /instance ... /default so that the
		// most-personalized are found first.
		for (uIndex = 0u; uIndex < SG_NrElements(gafScopes) && pValue == NULL; ++uIndex)
		{
			// get the value for the current scope
			_get_variant__scope fGetValue = gafScopes[uIndex];
			SG_ERR_CHECK(  fGetValue(pCtx, pSettings, szName, pRepo, &pValue, &pFullName)  );

			// if we didn't find one, make sure and free the returned scoped name
			// since we own it, but won't be using it for anything
			if (pValue == NULL)
			{
				SG_STRING_NULLFREE(pCtx, pFullName);
			}
		}
	}

#if TRACE_LS
	SG_ERR_IGNORE(  SG_console(pCtx, SG_CS_STDERR, "TraceLS: in SG_localsettings__get_variant: return\n")  );
#endif

	// return whatever value we found (even just NULL)
	*ppValue = pValue;
	pValue = NULL;

	// if they want the full name where we found the value, return that too
	// but only if we actually found a value
	if (ppFullName != NULL && *ppValue != NULL)
	{
		*ppFullName = pFullName;
		pFullName = NULL;
	}

fail:
	SG_VARIANT_NULLFREE(pCtx, pValue);
	SG_STRING_NULLFREE(pCtx, pFullName);
	SG_JSONDB_NULLFREE(pCtx, pSettings);
}

void SG_localsettings__get__sz(SG_context * pCtx, const char * psz_path, SG_repo* pRepo, char** ppsz, SG_string** ppstr_where_found)
{
    char* psz_val = NULL;
    SG_string* pstr_path = NULL;
    SG_variant* pv = NULL;

    SG_ERR_CHECK(  SG_localsettings__get__variant(pCtx, psz_path, pRepo, &pv, &pstr_path)  );
    if (pv)
    {
        if (SG_VARIANT_TYPE_SZ == pv->type)
        {
            psz_val = (char*) pv->v.val_sz;
            pv->v.val_sz = NULL;
        }
    }

    *ppsz = psz_val;
    psz_val = NULL;

    if (ppstr_where_found)
    {
        *ppstr_where_found = pstr_path;
        pstr_path = NULL;
    }

fail:
    SG_VARIANT_NULLFREE(pCtx, pv);
    SG_STRING_NULLFREE(pCtx, pstr_path);
    SG_NULLFREE(pCtx, psz_val);
}

void SG_localsettings__get__varray(SG_context * pCtx, const char * psz_path, SG_repo* pRepo, SG_varray** ppva, SG_string** ppstr_where_found)
{
    SG_varray* pva = NULL;
    SG_string* pstr_path = NULL;
    SG_variant* pv = NULL;

    SG_ERR_CHECK(  SG_localsettings__get__variant(pCtx, psz_path, pRepo, &pv, &pstr_path)  );
    if (pv)
    {
        if (SG_VARIANT_TYPE_VARRAY == pv->type)
        {
            pva = pv->v.val_varray;
            pv->v.val_varray = NULL;
        }
    }

    *ppva = pva;
    pva = NULL;

    if (ppstr_where_found)
    {
        *ppstr_where_found = pstr_path;
        pstr_path = NULL;
    }

fail:
    SG_VARIANT_NULLFREE(pCtx, pv);
    SG_STRING_NULLFREE(pCtx, pstr_path);
    SG_VARRAY_NULLFREE(pCtx, pva);
}

void SG_localsettings__get__vhash(SG_context * pCtx, const char * psz_path, SG_repo* pRepo, SG_vhash** ppvh, SG_string** ppstr_where_found)
{
    SG_vhash* pvh = NULL;
    SG_string* pstr_path = NULL;
    SG_variant* pv = NULL;

    SG_ERR_CHECK(  SG_localsettings__get__variant(pCtx, psz_path, pRepo, &pv, &pstr_path)  );
    if (pv)
    {
        if (SG_VARIANT_TYPE_VHASH == pv->type)
        {
            pvh = pv->v.val_vhash;
            pv->v.val_vhash = NULL;
        }
    }

    *ppvh = pvh;
    pvh = NULL;

    if (ppstr_where_found)
    {
        *ppstr_where_found = pstr_path;
        pstr_path = NULL;
    }

fail:
    SG_VARIANT_NULLFREE(pCtx, pv);
    SG_STRING_NULLFREE(pCtx, pstr_path);
    SG_VHASH_NULLFREE(pCtx, pvh);
}

//////////////////////////////////////////////////////////////////

/**
 * SG_varray_foreach_callback that copies the given value to another array,
 * if the value is not found in any of another set of arrays.
 * If the value being added is itself an array,
 * then it is recursed and its individual values are added.
 */
static SG_varray_foreach_callback _add_to_varray_if__flatten;

static void _add_to_varray_if__flatten(
	SG_context*       pCtx,        //< [in] [out] Error and context info.
	void*             pCallerData, //< [in] 
	const SG_varray*  pArray,      //< [in] The array being iterated.
	SG_uint32         uIndex,      //< [in] The current index in the array being iterated.
	const SG_variant* pValue       //< [in] The current value in the array being iterated.
	)
{
	SG_varray * pvaCallerData = (SG_varray *)pCallerData;

	SG_NULLARGCHECK(pCallerData);
	SG_UNUSED(pArray);
	SG_UNUSED(uIndex);
	SG_NULLARGCHECK(pValue);

	switch (pValue->type)
	{
	case SG_VARIANT_TYPE_VARRAY:
		// if the value is itself an array,
		// then recursively iterate through it and add each of its values
		// effectively flattening all arrays into the output
		SG_ERR_CHECK(  SG_varray__foreach(pCtx, pValue->v.val_varray, _add_to_varray_if__flatten, pCallerData)  );
		break;
	case SG_VARIANT_TYPE_SZ:
		SG_ERR_CHECK(  SG_varray__appendcopy__variant(pCtx, pvaCallerData, pValue)  );
		break;
	default:
		// the only other possibility *should* be hashes
		// but just in case we end up with a localsetting that's an int or a double or something,
		// we'll just ignore any other type
		break;
	}

fail:
	return;
}

//////////////////////////////////////////////////////////////////

/**
 * SG_vhash_foreach_callback that copies the given key/value pair to another vhash.
 * Existing keys in the destination will NOT be replaced with the new value.
 */
static void _add_to_vhash__unique(
	SG_context*       pCtx,        //< [in] [out] Error and context info.
	void*             pCallerData, //< [in] The vhash to add values to.
	const SG_vhash*   pHash,       //< [in] The hash being iterated.
	const char*       szKey,       //< [in] The current key in the hash being iterated.
	const SG_variant* pValue       //< [in] The current value in the hash being iterated.
	)
{
	SG_vhash* pDestination = (SG_vhash*)pCallerData;
	SG_bool   bHasKey      = SG_TRUE;

	SG_NULLARGCHECK(pCallerData);
	SG_UNUSED(pHash);
	SG_NULLARGCHECK(szKey);
	SG_NULLARGCHECK(pValue);

	SG_ERR_CHECK(  SG_vhash__has(pCtx, pDestination, szKey, &bHasKey)  );
	if (bHasKey == SG_FALSE)
	{
		SG_ERR_CHECK(  SG_vhash__addcopy__variant(pCtx, pDestination, szKey, pValue)  );
	}

fail:
	return;
}

void SG_localsettings__get__collapsed_vhash(
	SG_context* pCtx,
	const char* szName,
	SG_repo*    pRepo,
	SG_vhash**  ppHash
	)
{
	SG_jsondb*  pSettings                          = NULL;
	SG_uint32   uIndex                             = 0u;
	SG_variant* apValues[SG_NrElements(gafScopes)] = {NULL,};
	SG_vhash*   pHash                              = NULL;
	SG_string*  pFullPath                          = NULL;
	SG_variant* pSystemWideValue                   = NULL;

	SG_NULLARGCHECK(szName);
	SG_NULLARGCHECK(ppHash);

	// load the settings
	SG_ERR_CHECK(  SG_closet__get_localsettings(pCtx, &pSettings)  );

	// allocate our output hash
	SG_ERR_CHECK(  SG_VHASH__ALLOC(pCtx, &pHash)  );

	// run through and get the named value from each scope
	for (uIndex = 0; uIndex < SG_NrElements(gafScopes); ++uIndex)
	{
		_get_variant__scope fGetValue = gafScopes[uIndex];
		fGetValue(pCtx, pSettings, szName, pRepo, apValues + uIndex, NULL);
	}

	// run through each scope, from highest priority to lowest
	for (uIndex = 0u; uIndex < SG_NrElements(gafScopes); ++uIndex)
	{
		SG_variant* pValue     = apValues[uIndex];
		SG_vhash*   pValueHash = NULL;

		// if there was no value at this scope, move on
		if (pValue == NULL)
		{
			continue;
		}

		// if this value isn't a vhash, ignore it
		if (pValue->type != SG_VARIANT_TYPE_VHASH)
		{
			continue;
		}

		// get the hash value
		SG_ERR_CHECK(  SG_variant__get__vhash(pCtx, pValue, &pValueHash)  );

		// iterate through each value in the hash and add it to the output
		// as long as the key doesn't already exist in the output
		SG_ERR_CHECK(  SG_vhash__foreach(pCtx, pValueHash, _add_to_vhash__unique, pHash)  );
	}

	// lastly, merge in the system-wide config file's "machine"-scoped value
	SG_ERR_CHECK(  SG_string__alloc__format(pCtx, &pFullPath, "/machine/%s", szName)  );
	_system_wide_config__get__value(pCtx, SG_string__sz(pFullPath), &pSystemWideValue);
	if(!SG_CONTEXT__HAS_ERR(pCtx) && pSystemWideValue!=NULL && pSystemWideValue->type==SG_VARIANT_TYPE_VHASH)
	{
		SG_ERR_CHECK(  SG_vhash__foreach(pCtx, pSystemWideValue->v.val_vhash, _add_to_vhash__unique, pHash)  );
	}
	else
	{
		SG_context__err_reset(pCtx);
	}

	// return the output hash
	*ppHash = pHash;
	pHash = NULL;

fail:
	for (uIndex = 0u; uIndex < SG_NrElements(gafScopes); ++uIndex)
	{
		SG_VARIANT_NULLFREE(pCtx, apValues[uIndex]);
	}
	SG_VHASH_NULLFREE(pCtx, pHash);
	SG_JSONDB_NULLFREE(pCtx, pSettings);
	SG_STRING_NULLFREE(pCtx, pFullPath);
	SG_VARIANT_NULLFREE(pCtx, pSystemWideValue);
	return;
}

typedef struct 
{
	SG_vhash* pvhResultCurrent;
} localsettings_list_state;

static void _localsettings_list_cb(
	SG_context* pCtx, 
	void* ctx, 
	const SG_vhash* pvhMachineConfig, 
	const char* putf8Key, 
	const SG_variant* pvMachineSetting)
{
	localsettings_list_state* pState = (localsettings_list_state*)ctx;
	SG_string* pstrCurrentVhashPath = NULL;

	SG_UNUSED(pvhMachineConfig);

	if (SG_VARIANT_TYPE_VHASH == pvMachineSetting->type)
	{
		SG_bool bHas = SG_FALSE;
		SG_vhash* pvhRefPrev = pState->pvhResultCurrent;
		SG_ERR_CHECK(  SG_vhash__has(pCtx, pState->pvhResultCurrent, putf8Key, &bHas)  );
		if (bHas)
			SG_ERR_CHECK(  SG_vhash__get__vhash(pCtx, pState->pvhResultCurrent, putf8Key, &pState->pvhResultCurrent)  );
		else
		{
			SG_vhash* pvhTmp;
			SG_ERR_CHECK(  SG_VHASH__ALLOC(pCtx, &pvhTmp)  );
			pState->pvhResultCurrent = pvhTmp;
			SG_ERR_CHECK(  SG_vhash__add__vhash(pCtx, pvhRefPrev, putf8Key, &pvhTmp)  );
		}

		SG_ERR_CHECK(  SG_vhash__foreach(pCtx, pvMachineSetting->v.val_vhash, _localsettings_list_cb, ctx)  );
		
		pState->pvhResultCurrent = pvhRefPrev;
	}
	else
	{
		/* NOTE: As written, varrays won't be combined. If a varray 
		 * setting is defined in both the machine and user config 
		 * files, the machine values will be ignored. */

		SG_bool bHas = SG_FALSE;

		SG_ERR_CHECK(  SG_vhash__has(pCtx, pState->pvhResultCurrent, putf8Key, &bHas)  );
		if (!bHas)
		{
			/* User settings don't have an override. Add the machine setting. */
			SG_ERR_CHECK(  SG_vhash__addcopy__variant(pCtx, pState->pvhResultCurrent, 
				putf8Key, pvMachineSetting)  );
		}
		
		SG_STRING_NULLFREE(pCtx, pstrCurrentVhashPath);
	}

	return;

fail:
	SG_STRING_NULLFREE(pCtx, pstrCurrentVhashPath);
}

void SG_localsettings__list__vhash(SG_context * pCtx, SG_vhash** ppResult)
{
	SG_jsondb* p = NULL;
	SG_vhash* pvhUserConfig = NULL;
	SG_vhash* pvhMachineConfig = NULL;

	localsettings_list_state state;
	state.pvhResultCurrent = NULL;
	
	SG_NULLARGCHECK_RETURN(ppResult);

	SG_ERR_CHECK(  SG_closet__get_localsettings(pCtx, &p)  );
	SG_ERR_CHECK(  SG_jsondb__get__vhash(pCtx, p, "/", &pvhUserConfig)  );
	
	SG_ERR_CHECK(  _system_wide_config__get__vhash(pCtx, &pvhMachineConfig)  );
	if (!pvhMachineConfig)
	{
		SG_RETURN_AND_NULL(pvhUserConfig, ppResult);
	}
	else
	{
		state.pvhResultCurrent = pvhUserConfig;
		SG_ERR_CHECK(  SG_vhash__foreach(pCtx, pvhMachineConfig, _localsettings_list_cb, &state)  );

		SG_RETURN_AND_NULL(pvhUserConfig, ppResult);
		pvhUserConfig = NULL;
	}

	/* fall through */
fail:
	SG_JSONDB_NULLFREE(pCtx, p);
	SG_VHASH_NULLFREE(pCtx, pvhUserConfig);
	SG_VHASH_NULLFREE(pCtx, pvhMachineConfig);
}

void SG_localsettings__varray__append(SG_context * pCtx, const char* psz_path, const char* pValue)
{
	SG_jsondb* p = NULL;
	SG_string* pstr_path = NULL;

	SG_ASSERT(pCtx);
	SG_NONEMPTYCHECK_RETURN(psz_path);

	SG_ERR_CHECK(  SG_STRING__ALLOC(pCtx, &pstr_path)  );
	if ('/' == psz_path[0])
	{
		SG_ERR_CHECK(  SG_string__sprintf(pCtx, pstr_path, "%s/#", psz_path)  );
	}
	else
	{
		SG_ERR_CHECK(  SG_string__sprintf(pCtx, pstr_path, "%s/%s/#", SG_LOCALSETTING__SCOPE__MACHINE, psz_path)  );
	}

	SG_ERR_CHECK(  SG_closet__get_localsettings(pCtx, &p)  );

	SG_ERR_CHECK(  SG_jsondb__update__string__sz(pCtx, p, SG_string__sz(pstr_path), SG_TRUE, pValue)  );

fail:
	SG_JSONDB_NULLFREE(pCtx, p);
	SG_STRING_NULLFREE(pCtx, pstr_path);
}

void SG_localsettings__varray__remove_first_match(SG_context * pCtx, const char* psz_path, const char* psz_val)
{
	SG_jsondb* p = NULL;
    SG_string* pstr_path_element = NULL;
    SG_varray* pva = NULL;
    SG_uint32 ndx = 0;
    SG_uint32 count = 0;
    SG_uint32 i = 0;
    SG_bool b_found = SG_FALSE;
    SG_string* pstr_path_found = NULL;

	SG_ASSERT(pCtx);
	SG_NONEMPTYCHECK_RETURN(psz_path);

    SG_ERR_CHECK(  SG_closet__get_localsettings(pCtx, &p)  );
    SG_ERR_CHECK(  SG_localsettings__get__varray(pCtx, psz_path, NULL, &pva, &pstr_path_found)  );
    if (
        pva != NULL &&
        strncmp(SG_string__sz(pstr_path_found), SG_LOCALSETTING__SCOPE__DEFAULT, SG_STRLEN(SG_LOCALSETTING__SCOPE__DEFAULT)) != 0
    )
    {
        SG_ERR_CHECK(  SG_varray__count(pCtx, pva, &count)  );
        for (i=0; i<count; i++)
        {
            const char* psz = NULL;

            SG_ERR_CHECK(  SG_varray__get__sz(pCtx, pva, i, &psz)  );
            if (0 == strcmp(psz, psz_val))
            {
                b_found = SG_TRUE;
                ndx = i;
                break;
            }
        }
        if (b_found)
        {
            SG_ERR_CHECK(  SG_STRING__ALLOC(pCtx, &pstr_path_element)  );
            SG_ERR_CHECK(  SG_string__sprintf(pCtx, pstr_path_element, "%s/%d", SG_string__sz(pstr_path_found), ndx)  );
            SG_ERR_CHECK(  SG_jsondb__remove(pCtx, p, SG_string__sz(pstr_path_element))  );
        }
    }

fail:
    SG_VARRAY_NULLFREE(pCtx, pva);
    SG_STRING_NULLFREE(pCtx, pstr_path_found);
    SG_STRING_NULLFREE(pCtx, pstr_path_element);
	SG_JSONDB_NULLFREE(pCtx, p);
}

void SG_localsettings__import(
	SG_context * pCtx,
	SG_vhash* pvh /**< Caller still owns this */
	)
{
	SG_bool    bHasDefaults = SG_FALSE;
    SG_jsondb* p            = NULL;

	// remove any "defaults" section from the settings to import
	// It would usually exist if they're importing from a file that was
	// exported using the --verbose flag.
	SG_ERR_CHECK(  SG_vhash__has(pCtx, pvh, SG_LOCALSETTING__SCOPE__DEFAULT + 1, &bHasDefaults)  );
	if (bHasDefaults == SG_TRUE)
	{
		SG_ERR_CHECK(  SG_log__report_info(pCtx, "Ignoring section: %s", SG_LOCALSETTING__SCOPE__DEFAULT + 1)  );
		SG_ERR_CHECK(  SG_vhash__remove(pCtx, pvh, SG_LOCALSETTING__SCOPE__DEFAULT + 1)  );
	}

    SG_ERR_CHECK(  SG_closet__get_localsettings(pCtx, &p)  );
    SG_ERR_CHECK(  SG_jsondb__update__vhash(pCtx, p, "/", SG_TRUE, pvh)  );

fail:
    SG_JSONDB_NULLFREE(pCtx, p);
}

void SG_localsettings__split_full_name(
	SG_context* pCtx,
	const char* szFullName,
	SG_uint32* uSplitIndex,
	SG_string* pScopeName,
	SG_string* pSettingName
	)
{
	SG_uint32 uSlashCount = 0;
	SG_uint32 uCurrent = 0;

	SG_ARGCHECK(szFullName[0] == '/', szFullName);

	// this is basically implemented by locating the Nth '/' character
	// where N depends on which scope the name is in

	// TODO 2011/07/15 since SG_LOCALSETTING__SCOPE__{...} are CONSTANTS, we
	// TODO            could precompute the lengths and avoid the strlen()'s
	// TODO            here.
	if (0 == strncmp(szFullName, SG_LOCALSETTING__SCOPE__INSTANCE, strlen(SG_LOCALSETTING__SCOPE__INSTANCE)))
	{
		uSlashCount = 2;
	}
	else if (0 == strncmp(szFullName, SG_LOCALSETTING__SCOPE__REPO, strlen(SG_LOCALSETTING__SCOPE__REPO)))
	{
		uSlashCount = 2;
	}
	else if (0 == strncmp(szFullName, SG_LOCALSETTING__SCOPE__ADMIN, strlen(SG_LOCALSETTING__SCOPE__ADMIN)))
	{
		uSlashCount = 2;
	}
	else if (0 == strncmp(szFullName, SG_LOCALSETTING__SCOPE__MACHINE, strlen(SG_LOCALSETTING__SCOPE__MACHINE)))
	{
		uSlashCount = 1;
	}
	else
	{
		uSlashCount = 1;
	}

	// iterate through the string until we find the slash that we're looking for
	while (szFullName[uCurrent] != '\0' && uSlashCount > 0u)
	{
		// the first time through, this will always skip the leading slash character
		// this is why uSlashCount is one less than it seems like it should be
		uCurrent = uCurrent + 1u;

		// if this is a slash, update our count
		if (szFullName[uCurrent] == '/')
		{
			uSlashCount = uSlashCount - 1;
		}
	}

	// if the caller wants the index, fill it in
	if (uSplitIndex != NULL)
	{
		*uSplitIndex = uCurrent;
	}

	// if the caller wants the scope name, fill it in
	if (pScopeName != NULL)
	{
		SG_ERR_CHECK(  SG_string__set__buf_len(pCtx, pScopeName, (const SG_byte*)szFullName, uCurrent)  );
	}

	// if the caller wants the setting name, fill it in
	if (pSettingName != NULL)
	{
		SG_ERR_CHECK(  SG_string__set__sz(pCtx, pSettingName, szFullName + uCurrent + 1)  );
	}

fail:
	return;
}

/**
 * Structure used by provide_matching_values as pCallerData.
 */
typedef struct
{
	const char*                        szPattern;   //< The pattern to match names against.  See SG_localsettings__foreach.
	SG_localsettings_foreach_callback* pCallback;   //< The callback to provide matching values to.
	void*                              pCallerData; //< Caller-specific data to send back to pCallback.
	SG_string*                         pPrefix;     //< The prefix of the current value.  Used to built fully-qualified names.
}
provide_matching_values__data;

/**
 * Provides each value from a vhash whose name matches a pattern to another callback.
 * Values that are themselves vhashes are recursed into.
 * Each value's name is prefixed such that it's fully-qualified when passed to the callback.
 * Intended for use as a SG_vhash_foreach_callback.
 */
static void provide_matching_values(
	SG_context*       pCtx,        //< Error and context information.
	void*             pCallerData, //< An allocated instance of provide_matching_values__data.
	const SG_vhash*   pHash,       //< The hash that the current value is from.
	const char*       szName,      //< The name of the current value.
	const SG_variant* pValue       //< The current value.
	)
{
	SG_string* pFullName = NULL;
	SG_string* pScopeName = NULL;
	SG_string* pSettingName = NULL;
	SG_uint32 uValueSize = 0u;
	provide_matching_values__data* pData = NULL;

	SG_UNUSED(pHash);

	SG_NULLARGCHECK_RETURN(pCallerData);
	pData = (provide_matching_values__data*) pCallerData;

	// build the full name of this value from the incoming prefix and name
	SG_ERR_CHECK(  SG_STRING__ALLOC__COPY(pCtx, &pFullName, pData->pPrefix)  );
	SG_ERR_CHECK(  SG_string__append__sz(pCtx, pFullName, "/")  );
	SG_ERR_CHECK(  SG_string__append__sz(pCtx, pFullName, szName)  );

	// if this is a vhash, get its size
	if (pValue->type == SG_VARIANT_TYPE_VHASH)
	{
		SG_ERR_CHECK(  SG_vhash__count(pCtx, pValue->v.val_vhash, &uValueSize)  );
	}

	// if this is a vhash with children, then recurse into it
	// otherwise provide it to the callback
	if (uValueSize > 0u)
	{
		// use our full name as the prefix during recursion
		// to accomplish that, we'll swap pData->pPrefix and pFullName
		// that way if an error occurs during recursion, everything will still be freed
		SG_string* pTemp = NULL;
		pTemp = pData->pPrefix;
		pData->pPrefix = pFullName;
		pFullName = pTemp;
		SG_ERR_CHECK(  SG_vhash__foreach(pCtx, pValue->v.val_vhash, provide_matching_values, pData)  );
		pTemp = pData->pPrefix;
		pData->pPrefix = pFullName;
		pFullName = pTemp;
	}
	else
	{
		// if we have a pattern that starts with a slash
		// then match it against the start of the full name
		// if the name doesn't match, skip this one
		if (pData->szPattern != NULL && pData->szPattern[0] == '/')
		{
			if (strncmp(SG_string__sz(pFullName), pData->szPattern, strlen(pData->szPattern)) != 0)
			{
				goto fail;
			}
		}

		// split our full name into a scope name and a local name
		SG_ERR_CHECK(  SG_STRING__ALLOC(pCtx, &pScopeName)  );
		SG_ERR_CHECK(  SG_STRING__ALLOC(pCtx, &pSettingName)  );
		SG_ERR_CHECK(  SG_localsettings__split_full_name(pCtx, SG_string__sz(pFullName), NULL, pScopeName, pSettingName)  );

		// if we have a pattern that doesn't start with a slash
		// then match it against just the local name of the setting
		// if the name doesn't match, skip this one
		if (pData->szPattern != NULL && pData->szPattern[0] != '/')
		{
			if (strstr(SG_string__sz(pSettingName), pData->szPattern) == NULL)
			{
				goto fail;
			}
		}

		// send the data to the callback
		pData->pCallback(pCtx, SG_string__sz(pFullName), SG_string__sz(pScopeName), SG_string__sz(pSettingName), pValue, pData->pCallerData);
	}

fail:
	SG_STRING_NULLFREE(pCtx, pFullName);
	SG_STRING_NULLFREE(pCtx, pScopeName);
	SG_STRING_NULLFREE(pCtx, pSettingName);
}

void SG_localsettings__foreach(
	SG_context* pCtx,
	const char* szPattern,
	SG_bool bIncludeDefaults,
	SG_localsettings_foreach_callback* pCallback,
	void* pCallerData
	)
{
	SG_vhash* pValues = NULL;
	const SG_vhash* pDefaults = NULL;
	provide_matching_values__data ProvideMatchingValuesData = {NULL, NULL, NULL, NULL};

	SG_UNUSED(pCallback);
	SG_UNUSED(pCallerData);

	// get the settings
	SG_ERR_CHECK(  SG_localsettings__list__vhash(pCtx, &pValues)  );

	// if defaults were requested, get those too
	if (bIncludeDefaults)
	{
		SG_ERR_CHECK(  SG_localsettings__factory__list__vhash(pCtx, &pDefaults)  );
		SG_ERR_CHECK(  SG_vhash__addcopy__vhash(pCtx, pValues, SG_LOCALSETTING__SCOPE__DEFAULT + 1, pDefaults)  ); // +1 to remove the slash at the beginning
	}

	// sort the settings
	SG_ERR_CHECK(  SG_vhash__sort(pCtx, pValues, SG_TRUE, SG_vhash_sort_callback__increasing)  );

	// setup our callback data
	SG_ERR_CHECK(  SG_STRING__ALLOC(pCtx, &(ProvideMatchingValuesData.pPrefix))  );
	ProvideMatchingValuesData.szPattern = szPattern;
	ProvideMatchingValuesData.pCallback = pCallback;
	ProvideMatchingValuesData.pCallerData = pCallerData;

	// iterate through the vhash
	SG_ERR_CHECK(  SG_vhash__foreach(pCtx, pValues, provide_matching_values, &ProvideMatchingValuesData)  );

fail:
	SG_VHASH_NULLFREE(pCtx, pValues);
	SG_STRING_NULLFREE(pCtx, ProvideMatchingValuesData.pPrefix);
}

/**
 * Update a string value in the system-wide configuration store.
 */
void SG_localsettings__system_wide__update_sz(
	SG_context * pCtx,
	const char * psz_path,
	const char * pszValue)
{
	SG_pathname* pPathnameSystemConfig = NULL;
	SG_vfile* pvf = NULL;
	SG_vhash* pvh = NULL;

	SG_NULLARGCHECK_RETURN(psz_path);
	SG_NULLARGCHECK_RETURN(pszValue);
	
	SG_ERR_CHECK(  _system_wide_config__get__pathname(pCtx, SG_TRUE, &pPathnameSystemConfig)  );
	SG_ERR_CHECK(  SG_vfile__begin(pCtx, pPathnameSystemConfig, SG_FILE_RDWR | SG_FILE_OPEN_OR_CREATE, &pvh, &pvf)  );
	if (!pvh)
		SG_ERR_CHECK(  SG_VHASH__ALLOC(pCtx, &pvh)  );
	SG_ERR_CHECK(  SG_vhash__slashpath__update__string__sz(pCtx, pvh, psz_path, pszValue)  );
	SG_ERR_CHECK(  SG_vfile__end__pretty_print_NOT_for_repo_storage(pCtx, &pvf, pvh)  );

	SG_PATHNAME_NULLFREE(pCtx, pPathnameSystemConfig);
	SG_VHASH_NULLFREE(pCtx, pvh);

	return;

fail:
	SG_PATHNAME_NULLFREE(pCtx, pPathnameSystemConfig);
	SG_ERR_IGNORE(  SG_vfile__abort(pCtx, &pvf)  );
	SG_VHASH_NULLFREE(pCtx, pvh);
}

//////////////////////////////////////////////////////////////////

/**
 * Return a VHASH (keyed by scope) containing a VARRAY with
 * the values stored in the config.  Values may come from
 * either localsettings-proper or from the system-wide settings
 * as appropriate for the key type.
 */
void SG_localsettings__get_varray_at_each_scope(SG_context * pCtx,
												const char * pszName,
												SG_repo * pRepo,
												SG_vhash ** ppVHashOfVArray)
{
	SG_jsondb* pSettings = NULL;
	SG_vhash * pvhResult = NULL;
	SG_variant * pVariant_k = NULL;
	SG_varray * pva_k = NULL;
	SG_uint32 uIndex = 0;

	SG_NONEMPTYCHECK_RETURN( pszName );
	SG_NULLARGCHECK_RETURN( pRepo );
	SG_NULLARGCHECK_RETURN( ppVHashOfVArray );

	SG_ERR_CHECK(  SG_closet__get_localsettings(pCtx, &pSettings)  );

	SG_ERR_CHECK(  SG_VHASH__ALLOC(pCtx, &pvhResult)  );

	// for each scope level:
	//    get the value of 'name' at that level
	//    (since the value can be a string, an array of string, or really anything
	//     the user may have set from the command line.)
	//    convert the value(s) into a flattened varray for this level
	//    add this varray to the vhash result.

	for (uIndex = 0; uIndex < SG_NrElements(gafScopes); ++uIndex)
	{
		_get_variant__scope fGetValue = gafScopes[uIndex];

		SG_ERR_CHECK(  fGetValue(pCtx, pSettings, pszName, pRepo, &pVariant_k, NULL)  );
		if (pVariant_k)
		{
			SG_ERR_CHECK(  SG_VARRAY__ALLOC(pCtx, &pva_k)  );
			SG_ERR_CHECK(  _add_to_varray_if__flatten(pCtx, pva_k, NULL, 0, pVariant_k)  );
			SG_ERR_CHECK(  SG_vhash__add__varray(pCtx, pvhResult, gaszScopeShortName[uIndex], &pva_k)  );

			SG_VARRAY_NULLFREE(pCtx, pva_k);
			SG_VARIANT_NULLFREE(pCtx, pVariant_k);
		}
	}

	*ppVHashOfVArray = pvhResult;
	pvhResult = NULL;

fail:
	SG_VARRAY_NULLFREE(pCtx, pva_k);
	SG_VARIANT_NULLFREE(pCtx, pVariant_k);
	SG_VHASH_NULLFREE(pCtx, pvhResult);
	SG_JSONDB_NULLFREE(pCtx, pSettings);
}

