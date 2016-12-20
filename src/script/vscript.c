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


/*

This file, vscript.c, is based on the file js/src/js.c in mozilla's
js-1.8.0-rc1.tar.gz source tarball.

Note: We now build against SpiderMonkey 1.8.5, not 1.8.0. The equivalent file in
the SpiderMonkey 1.8.5 sources is js/src/shell/js.cpp. When we updated veracity
to build against SpiderMonkey 1.8.5 rather than 1.8.0, we chose not to do a
full-blown merge of their changes and ours (thereby making vscript a C++
project). Instead we just deleted most of the unused code and converted the rest
of it to only use SpiderMonkey's public C api. Some of the conversion was manual
and some of it involved copy-pasting from js/src/shell/js.cpp.

Considering one of the main goals was for us to be able to build veracity
against the system library (ie installed by apt-get) rather building
SpiderMonkey ourselves, we pretty much had to do it this way. This is due to the
fact that some of the headers js/src/shell/js.cpp depends on are missing or
broken (because of missing/invalid #includes) in the system-installed library.
The shell in mozilla's source tarball doesn't even build against a
system-installed SpiderMonkey; it only builds when it's part of the SpiderMonkey
source tree.

*/


/* -*- Mode: C; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 4 -*-
 * vim: set ts=8 sw=4 et tw=78:
 *
 * ***** BEGIN LICENSE BLOCK *****
 * Version: MPL 1.1/GPL 2.0/LGPL 2.1
 *
 * The contents of this file are subject to the Mozilla Public License Version
 * 1.1 (the "License"); you may not use this file except in compliance with
 * the License. You may obtain a copy of the License at
 * http://www.mozilla.org/MPL/
 *
 * Software distributed under the License is distributed on an "AS IS" basis,
 * WITHOUT WARRANTY OF ANY KIND, either express or implied. See the License
 * for the specific language governing rights and limitations under the
 * License.
 *
 * The Original Code is Mozilla Communicator client code, released
 * March 31, 1998.
 *
 * The Initial Developer of the Original Code is
 * Netscape Communications Corporation.
 * Portions created by the Initial Developer are Copyright (C) 1998
 * the Initial Developer. All Rights Reserved.
 *
 * Contributor(s):
 *
 * Alternatively, the contents of this file may be used under the terms of
 * either of the GNU General Public License Version 2 or later (the "GPL"),
 * or the GNU Lesser General Public License Version 2.1 or later (the "LGPL"),
 * in which case the provisions of the GPL or the LGPL are applicable instead
 * of those above. If you wish to allow use of your version of this file only
 * under the terms of either the GPL or the LGPL, and not to allow others to
 * use your version of this file under the terms of the MPL, indicate your
 * decision by deleting the provisions above and replace them with the notice
 * and other provisions required by the GPL or the LGPL. If you do not delete
 * the provisions above, a recipient may use your version of this file under
 * the terms of any one of the MPL, the GPL or the LGPL.
 *
 * ***** END LICENSE BLOCK ***** */

/*
 * JS shell.
 */

//#ifdef WINDOWS
//#pragma warning(disable:4706)
//#pragma warning(disable:4214)
//#pragma warning(disable:4996)
//#pragma warning(disable:4267)	// MSVC 2010 64-bit complains about implicit conversions from size_t to anything smaller.
//#pragma warning(disable:4244)	// MSVC 2010 64-bit complains about implicit conversions from size_t to anything smaller.
//#define _CRT_SECURE_NO_DEPRECATE
//#endif

#include <sg.h>

#include <locale.h>
#include <js/jsprf.h>

// Extracted from jscntxt.h which we can't #include due to C++ code and, worse,
// the fact that the file js.msg doesn't get installed along with the
// libmozjs185-1.0.0 in the apt repository. Our version of js.msg matches the
// version from the "js185-1.0.0" source tarball... and it had better match the
// js.msg that the installed libmozjs on a given machine was built with,
// otherwise the error messages might be a little screwy. (Actually, i think we
// only need this here for the value of JSErr_Limit, but still.....)
typedef enum JSErrNum {
#define MSG_DEF(name, number, count, exception, format) \
    name = number,
#include "js.msg"
#undef MSG_DEF
    JSErr_Limit
} JSErrNum;


// js_fgets copy-pasted out of jsscan.cpp in the js185-1.0.0 source tarball.
#if defined(HAVE_GETC_UNLOCKED)
# define fast_getc getc_unlocked
#elif defined(HAVE__GETC_NOLOCK)
# define fast_getc _getc_nolock
#else
# define fast_getc getc
#endif
static int
js_fgets(char *buf, int size, FILE *file)
{
    int n, i, c;
    JSBool crflag;

    n = size - 1;
    if (n < 0)
        return -1;

    crflag = JS_FALSE;
    for (i = 0; i < n && (c = fast_getc(file)) != EOF; i++) {
        buf[i] = (char)c;
        if (c == '\n') {        /* any \n ends a line */
            i++;                /* keep the \n; we know there is room for \0 */
            break;
        }
        if (crflag) {           /* \r not followed by \n ends line at the \r */
            ungetc(c, file);
            break;              /* and overwrite c in buf with \0 */
        }
        crflag = (c == '\r');
    }

    buf[i] = '\0';
    return i;
}

#ifdef XP_UNIX
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#endif

#if defined(XP_WIN) || defined(XP_OS2)
#include <io.h>     /* for _isatty() */
#define isatty _isatty
#define fileno _fileno
#endif

typedef enum JSShellExitCode {
    EXITCODE_RUNTIME_ERROR      = 3,
    EXITCODE_FILE_NOT_FOUND     = 4,
    EXITCODE_OUT_OF_MEMORY      = 5
} JSShellExitCode;

size_t gStackChunkSize = 8192;

/* Assume that we can not use more than 5e5 bytes of C stack by default. */
static size_t gMaxStackSize = 500000;
static jsuword gStackBase;

static size_t gScriptStackQuota = JS_DEFAULT_SCRIPT_STACK_QUOTA;

int gExitCode = 0;
JSBool gQuitting = JS_FALSE;
FILE *gErrFile = NULL;
FILE *gOutFile = NULL;

static JSBool reportWarnings = JS_TRUE;
static JSBool compileOnly = JS_FALSE;

typedef enum JSShellErrNum {
#define MSG_DEF(name, number, count, exception, format) \
    name = number,
#include "jsshell.msg"
#undef MSG_DEF
    JSShellErr_Limit
#undef MSGDEF
} JSShellErrNum;

#ifdef EDITLINE
JS_BEGIN_EXTERN_C
extern char     *readline(const char *prompt);
extern void     add_history(char *line);
JS_END_EXTERN_C
#endif

static JSBool
GetLine(SG_context *pCtx, JSContext *cx, char *bufp, SG_uint32 bufremaining, FILE *file, const char *prompt) {
#ifndef EDITLINE
    SG_UNUSED(cx);
#endif

#ifdef EDITLINE
    /*
     * Use readline only if file is stdin, because there's no way to specify
     * another handle.  Are other filehandles interactive?
     */
    if (file == stdin) {
        char *linep = readline(prompt);
        if (!linep)
            return JS_FALSE;
        if (linep[0] != '\0')
            add_history(linep);
        SG_strcpy(pCtx, bufp, bufremaining, linep);
        if(SG_context__has_err(pCtx))
        {
            SG_context__err_reset(pCtx);
            return JS_FALSE;
        }
        JS_free(cx, linep);
        bufp += strlen(bufp);
        *bufp++ = '\n';
        *bufp = '\0';
    } else
#endif
    {
        char line[256];
        fprintf(gOutFile, "%s", prompt);
        fflush(gOutFile);
        if (!fgets(line, sizeof line, file))
            return JS_FALSE;
        SG_strcpy(pCtx, bufp, bufremaining, line);
        if(SG_context__has_err(pCtx))
        {
            SG_context__err_reset(pCtx);
            return JS_FALSE;
        }
    }
    return JS_TRUE;
}

static void
SetContextOptions(JSContext *cx)
{
    jsuword stackLimit;

    if (gMaxStackSize == 0) {
        /*
         * Disable checking for stack overflow if limit is zero.
         */
        stackLimit = 0;
    } else {
#if JS_STACK_GROWTH_DIRECTION > 0
        stackLimit = gStackBase + gMaxStackSize;
#else
        stackLimit = gStackBase - gMaxStackSize;
#endif
    }
    JS_SetThreadStackLimit(cx, stackLimit);
    JS_SetScriptStackQuota(cx, gScriptStackQuota);
}

static void
Process(JSContext *cx, JSObject *obj, char *filename, JSBool forceTTY)
{
	SG_context * pCtx = NULL;
	char * sz = NULL;
	
    JSBool ok, hitEOF;
    JSObject *script;
    jsval result;
    JSString *str;
    char buffer[4096];
    char *bufp;
    int lineno;
    int startline;

    SetContextOptions(cx);

    if (!forceTTY && filename != NULL && strlen(filename) != 0) {
        script = JS_CompileFile(cx, obj, filename);
        if (script) {
            if (!compileOnly)
                (void)JS_ExecuteScript(cx, obj, script, &result);
        }

        return;
    }

	(void)SG_context__alloc(&pCtx);

    /* It's an interactive filehandle; drop into read-eval-print loop. */
    lineno = 1;
    hitEOF = JS_FALSE;
    do {
        bufp = buffer;
        *bufp = '\0';

        /*
         * Accumulate lines until we get a 'compilable unit' - one that either
         * generates an error (before running out of source) or that compiles
         * cleanly.  This should be whenever we get a complete statement that
         * coincides with the end of a line.
         */
        startline = lineno;
        do {
            if (!GetLine(pCtx, cx, bufp, (SG_uint32)(sizeof(buffer)-(bufp-buffer)), stdin, startline == lineno ? "vscript> " : "")) {
                hitEOF = JS_TRUE;
                break;
            }
            bufp += strlen(bufp);
            lineno++;
        } while (!JS_BufferIsCompilableUnit(cx, obj, buffer, strlen(buffer)));

        /* Clear any pending exception from previous failed compiles.  */
        JS_ClearPendingException(cx);
        script = JS_CompileScript(cx, obj, buffer, strlen(buffer), "typein",
                                  startline);
        if (script) {
            if (!compileOnly) {
                ok = JS_ExecuteScript(cx, obj, script, &result);
                if (ok && !JSVAL_IS_VOID(result)) {
                    str = JS_ValueToString(cx, result);
                    if (str)
                    {
                    	SG_ERR_IGNORE(  sg_jsglue__jsstring_to_sz(pCtx, cx, str, &sz)  );
                        fprintf(gOutFile, "%s\n", sz);
                        SG_NULLFREE(pCtx, sz);
                    }
                }
            }
        }
    } while (!hitEOF && !gQuitting);
    SG_CONTEXT_NULLFREE(pCtx);
    fprintf(gOutFile, "\n");
    return;
}

static int
usage(void)
{
    extern const char *vscript_usage;

    fprintf(gErrFile, "vscript %s.%s.%s.%s%s\n\n", MAJORVERSION, MINORVERSION, REVVERSION, BUILDNUMBER, DEBUGSTRING);
    fputs(vscript_usage, gErrFile);
    return 2;
}

static struct {
    const char  *name;
    uint32      flag;
} js_options[] = {
    {"strict",          JSOPTION_STRICT},
    {"werror",          JSOPTION_WERROR},
    {"atline",          JSOPTION_ATLINE},
    {"xml",             JSOPTION_XML},
    {"relimit",         JSOPTION_RELIMIT},
    {"anonfunfix",      JSOPTION_ANONFUNFIX},
    {NULL,              0}
};

extern JSClass global_class;

static int
ProcessArgs(JSContext *cx, JSObject *obj, char **argv, int argc)
{
    int i, j, length;
    JSObject *argsObj;
    char *filename = NULL;
    JSBool isInteractive = JS_TRUE;
    JSBool forceTTY = JS_FALSE;

    /*
     * Scan past all optional arguments so we can create the arguments object
     * before processing any -f options, which must interleave properly with
     * -v and -w options.  This requires two passes, and without getopt, we'll
     * have to keep the option logic here and in the second for loop in sync.
     */
    for (i = 0; i < argc; i++) {
        if (argv[i][0] != '-' || argv[i][1] == '\0') {
            ++i;
            break;
        }
        switch (argv[i][1]) {
          case 'b':
          case 'c':
          case 'f':
          case 'e':
          case 'v':
          case 'S':
            ++i;
            break;
          default:;
        }
    }

    /*
     * Create arguments early and define it to root it, so it's safe from any
     * GC calls nested below, and so it is available to -f <file> arguments.
     */
    argsObj = JS_NewArrayObject(cx, 0, NULL);
    if (!argsObj)
        return 1;
    if (!JS_DefineProperty(cx, obj, "arguments", OBJECT_TO_JSVAL(argsObj),
                           NULL, NULL, 0)) {
        return 1;
    }

    length = argc - i;
    for (j = 0; j < length; j++) {
        JSString *str = JS_NewStringCopyZ(cx, argv[i++]);
        if (!str)
            return 1;
        if (!JS_DefineElement(cx, argsObj, j, STRING_TO_JSVAL(str),
                              NULL, NULL, JSPROP_ENUMERATE)) {
            return 1;
        }
    }

    for (i = 0; i < argc; i++) {
        if (argv[i][0] != '-' || argv[i][1] == '\0') {
            filename = argv[i++];
            isInteractive = JS_FALSE;
            break;
        }

        switch (argv[i][1]) {
        case 'v':
            if (++i == argc)
                return usage();

            JS_SetVersion(cx, (JSVersion) atoi(argv[i]));
            break;

        case 'w':
            reportWarnings = JS_TRUE;
            break;

        case 'W':
            reportWarnings = JS_FALSE;
            break;

        case 's':
            JS_ToggleOptions(cx, JSOPTION_STRICT);
            break;

        case 'E':
            JS_ToggleOptions(cx, JSOPTION_RELIMIT);
            break;

        case 'x':
            JS_ToggleOptions(cx, JSOPTION_XML);
            break;

        case 'o':
            if (++i == argc)
                return usage();

            for (j = 0; js_options[j].name; ++j) {
                if (strcmp(js_options[j].name, argv[i]) == 0) {
                    JS_ToggleOptions(cx, js_options[j].flag);
                    break;
                }
            }
            break;

        case 'c':
            /* set stack chunk size */
            gStackChunkSize = atoi(argv[++i]);
            break;

        case 'f':
            if (++i == argc)
                return usage();

            Process(cx, obj, argv[i], JS_FALSE);

            /*
             * XXX: js -f foo.js should interpret foo.js and then
             * drop into interactive mode, but that breaks the test
             * harness. Just execute foo.js for now.
             */
            isInteractive = JS_FALSE;
            break;

        case 'e':
        {
            jsval rval;

            if (++i == argc)
                return usage();

            /* Pass a filename of -e to imitate PERL */
            JS_EvaluateScript(cx, obj, argv[i], SG_STRLEN(argv[i]),
                              "-e", 1, &rval);

            isInteractive = JS_FALSE;
            break;

        }
        case 'C':
            compileOnly = JS_TRUE;
            isInteractive = JS_FALSE;
            break;

        case 'i':
            isInteractive = forceTTY = JS_TRUE;
            break;

        case 'S':
            if (++i == argc)
                return usage();

            /* Set maximum stack size. */
            gMaxStackSize = atoi(argv[i]);
            break;

#ifdef MOZ_SHARK
        case 'k':
            JS_ConnectShark();
            break;
#endif
        default:
            return usage();
        }
    }

    if (filename || isInteractive)
        Process(cx, obj, filename, forceTTY);
    return gExitCode;
}

static JSBool
Load(JSContext *cx, uintN argc, jsval *vp)
{
    SG_context *pCtx = NULL;
    JSObject *thisobj = JS_THIS_OBJECT(cx, vp);
    jsval *argv = JS_ARGV(cx, vp);
    uintN i;
    if (!thisobj)
        return JS_FALSE;

	SG_context__alloc(&pCtx);
	if (pCtx==NULL)
		return JS_FALSE;

    for (i = 0; i < argc; i++) {
        JSString *str = JS_ValueToString(cx, argv[i]);
        char *filename = NULL;
    	uint32 oldopts;
    	JSObject *scriptObj;
        if (!str) {
        	SG_CONTEXT_NULLFREE(pCtx);
            return JS_FALSE;
        }
        argv[i] = STRING_TO_JSVAL(str);
        sg_jsglue__jsstring_to_sz(pCtx, cx, str, &filename);
        if (SG_context__has_err(pCtx)) {
        	SG_CONTEXT_NULLFREE(pCtx);
            return JS_FALSE;
        }
        errno = 0;
        oldopts = JS_GetOptions(cx);
        JS_SetOptions(cx, oldopts | JSOPTION_COMPILE_N_GO | JSOPTION_NO_SCRIPT_RVAL);
        scriptObj = JS_CompileFile(cx, thisobj, filename);
        SG_NULLFREE(pCtx, filename);
        JS_SetOptions(cx, oldopts);
        if (!scriptObj) {
        	SG_CONTEXT_NULLFREE(pCtx);
            return JS_FALSE;
        }

        if (!compileOnly && !JS_ExecuteScript(cx, thisobj, scriptObj, NULL)) {
        	SG_CONTEXT_NULLFREE(pCtx);
            return JS_FALSE;
        }
    }

	SG_CONTEXT_NULLFREE(pCtx);
    return JS_TRUE;
}

/*
 * function readline()
 * Provides a hook for scripts to read a line from stdin.
 */
static JSBool
ReadLine(JSContext *cx, uintN argc, jsval *vp)
{
#define BUFSIZE 256
    FILE *from;
    char *buf, *tmp;
    size_t bufsize, buflength, gotlength;
    JSBool sawNewline;
    JSString *str;

	SG_UNUSED(argc);

    from = stdin;
    buflength = 0;
    bufsize = BUFSIZE;
    buf = (char *) JS_malloc(cx, bufsize);
    if (!buf)
        return JS_FALSE;

    sawNewline = JS_FALSE;
    while ((gotlength =
            js_fgets(buf + buflength, (int)(bufsize - buflength), from)) > 0) {
        buflength += gotlength;

        /* Are we done? */
        if (buf[buflength - 1] == '\n') {
            buf[buflength - 1] = '\0';
            sawNewline = JS_TRUE;
            break;
        } else if (buflength < bufsize - 1) {
            break;
        }

        /* Else, grow our buffer for another pass. */
        bufsize *= 2;
        if (bufsize > buflength) {
            tmp = (char *) JS_realloc(cx, buf, bufsize);
        } else {
            JS_ReportOutOfMemory(cx);
            tmp = NULL;
        }

        if (!tmp) {
            JS_free(cx, buf);
            return JS_FALSE;
        }

        buf = tmp;
    }

    /* Treat the empty string specially. */
    if (buflength == 0) {
        *vp = feof(from) ? JSVAL_NULL : JS_GetEmptyStringValue(cx);
        JS_free(cx, buf);
        return JS_TRUE;
    }

    /* Shrink the buffer to the real size. */
    tmp = (char *) JS_realloc(cx, buf, buflength);
    if (!tmp) {
        JS_free(cx, buf);
        return JS_FALSE;
    }

    buf = tmp;

    /*
     * Turn buf into a JSString. Note that buflength includes the trailing null
     * character.
     */
    str = JS_NewStringCopyN(cx, buf, sawNewline ? buflength - 1 : buflength);
    JS_free(cx, buf);
    if (!str)
        return JS_FALSE;

    *vp = STRING_TO_JSVAL(str);
    return JS_TRUE;
}

static JSBool
Print(JSContext *cx, uintN argc, jsval *vp)
{
    jsval *argv;
    uintN i;
    JSString *str;
    char *bytes;

    argv = JS_ARGV(cx, vp);
    for (i = 0; i < argc; i++) {
        str = JS_ValueToString(cx, argv[i]);
        if (!str)
            return JS_FALSE;
        bytes = JS_EncodeString(cx, str);
        if (!bytes)
            return JS_FALSE;
        fprintf(gOutFile, "%s%s", i ? " " : "", bytes);
        JS_free(cx, bytes);
    }

    fputc('\n', gOutFile);
    fflush(gOutFile);

    JS_SET_RVAL(cx, vp, JSVAL_VOID);
    return JS_TRUE;
}

static JSBool
Help(JSContext *cx, uintN argc, jsval *vp);

static JSBool
Quit(JSContext *cx, uintN argc, jsval *vp)
{
    JS_ConvertArguments(cx, argc, JS_ARGV(cx, vp), "/ i", &gExitCode);

    gQuitting = JS_TRUE;
    return JS_FALSE;
}

static JSFunctionSpec shell_functions[] = {
    JS_FN("load",           Load,           1,0),
    JS_FN("readline",       ReadLine,       0,0),
    JS_FN("print",          Print,          0,0),
    JS_FN("help",           Help,           0,0),
    JS_FN("quit",           Quit,           0,0),
#ifdef MOZ_SHARK
    JS_FN("startShark",      js_StartShark,      0,0),
    JS_FN("stopShark",       js_StopShark,       0,0),
    JS_FN("connectShark",    js_ConnectShark,    0,0),
    JS_FN("disconnectShark", js_DisconnectShark, 0,0),
#endif
    JS_FS_END
};

static const char shell_help_header[] =
"Command                  Description\n"
"=======                  ===========\n";

static const char *const shell_help_messages[] = {
"load(['foo.js' ...])     Load files named by string arguments",
"readline()               Read a single line from stdin",
"print([exp ...])         Evaluate and print expressions",
"help([name ...])         Display usage and help messages",
"quit()                   Quit the shell",
#ifdef MOZ_SHARK
"startShark()             Start a Shark session.\n"
"                         Shark must be running with programatic sampling.",
"stopShark()              Stop a running Shark session.",
"connectShark()           Connect to Shark.\n"
"                         The -k switch does this automatically.",
"disconnectShark()        Disconnect from Shark.",
#endif
};

/* Help messages must match shell functions. */
JS_STATIC_ASSERT(JS_ARRAY_LENGTH(shell_help_messages) + 1 ==
                 JS_ARRAY_LENGTH(shell_functions));

#ifdef DEBUG
static void
CheckHelpMessages(void)
{
    const char *const *m;
    const char *lp;

    /* Each message must begin with "function_name(" prefix. */
    for (m = shell_help_messages; m != JS_ARRAY_END(shell_help_messages); ++m) {
        lp = strchr(*m, '(');
        JS_ASSERT(lp);
        JS_ASSERT(memcmp(shell_functions[m - shell_help_messages].name,
                         *m, lp - *m) == 0);
    }
}
#else
# define CheckHelpMessages() ((void) 0)
#endif

static JSBool
Help(JSContext *cx, uintN argc, jsval *vp)
{
    uintN i;
    SG_UNUSED(cx);
    SG_UNUSED(argc);
    fprintf(gOutFile, "%s\n", JS_GetImplementationVersion());
    fputs(shell_help_header, gOutFile);
    for (i = 0; shell_functions[i].name; i++)
        fprintf(gOutFile, "%s\n", shell_help_messages[i]);
    JS_SET_RVAL(cx, vp, JSVAL_VOID);
    return JS_TRUE;
}

#ifdef JSD_LOWLEVEL_SOURCE
/*
 * This facilitates sending source to JSD (the debugger system) in the shell
 * where the source is loaded using the JSFILE hack in jsscan. The function
 * below is used as a callback for the jsdbgapi JS_SetSourceHandler hook.
 * A more normal embedding (e.g. mozilla) loads source itself and can send
 * source directly to JSD without using this hook scheme.
 */
static void
SendSourceToJSDebugger(const char *filename, uintN lineno,
                       jschar *str, size_t length,
                       void **listenerTSData, JSDContext* jsdc)
{
    JSDSourceText *jsdsrc = (JSDSourceText *) *listenerTSData;

    if (!jsdsrc) {
        if (!filename)
            filename = "typein";
        if (1 == lineno) {
            jsdsrc = JSD_NewSourceText(jsdc, filename);
        } else {
            jsdsrc = JSD_FindSourceForURL(jsdc, filename);
            if (jsdsrc && JSD_SOURCE_PARTIAL !=
                JSD_GetSourceStatus(jsdc, jsdsrc)) {
                jsdsrc = NULL;
            }
        }
    }
    if (jsdsrc) {
        jsdsrc = JSD_AppendUCSourceText(jsdc,jsdsrc, str, length,
                                        JSD_SOURCE_PARTIAL);
    }
    *listenerTSData = jsdsrc;
}
#endif /* JSD_LOWLEVEL_SOURCE */

JSErrorFormatString jsShell_ErrorFormatString[JSErr_Limit] = {
#define MSG_DEF(name, number, count, exception, format) \
    { format, count, JSEXN_ERR } ,
#include "jsshell.msg"
#undef MSG_DEF
};

static void
my_ErrorReporter(JSContext *cx, const char *message, JSErrorReport *report)
{
    int i, j, k, n;
    char *prefix, *tmp;
    const char *ctmp;

    if (!report) {
        fprintf(gErrFile, "%s\n", message);
        return;
    }

    /* Conditionally ignore reported warnings. */
    if (JSREPORT_IS_WARNING(report->flags) && !reportWarnings)
        return;

    prefix = NULL;
    if (report->filename)
        prefix = JS_smprintf("%s:", report->filename);
    if (report->lineno) {
        tmp = prefix;
        prefix = JS_smprintf("%s%u: ", tmp ? tmp : "", report->lineno);
        JS_free(cx, tmp);
    }
    if (JSREPORT_IS_WARNING(report->flags)) {
        tmp = prefix;
        prefix = JS_smprintf("%s%swarning: ",
                             tmp ? tmp : "",
                             JSREPORT_IS_STRICT(report->flags) ? "strict " : "");
        JS_free(cx, tmp);
    }

    /* embedded newlines -- argh! */
    while ((ctmp = strchr(message, '\n')) != 0) {
        ctmp++;
        if (prefix)
            fputs(prefix, gErrFile);
        fwrite(message, 1, ctmp - message, gErrFile);
        message = ctmp;
    }

    /* If there were no filename or lineno, the prefix might be empty */
    if (prefix)
        fputs(prefix, gErrFile);
    fputs(message, gErrFile);

    if (!report->linebuf) {
        fputc('\n', gErrFile);
        goto out;
    }

    /* report->linebuf usually ends with a newline. */
    n = SG_STRLEN(report->linebuf);
    fprintf(gErrFile, ":\n%s%s%s%s",
            prefix,
            report->linebuf,
            (n > 0 && report->linebuf[n-1] == '\n') ? "" : "\n",
            prefix);
    n = (int)(report->tokenptr - report->linebuf);
    for (i = j = 0; i < n; i++) {
        if (report->linebuf[i] == '\t') {
            for (k = (j + 8) & ~7; j < k; j++) {
                fputc('.', gErrFile);
            }
            continue;
        }
        fputc('.', gErrFile);
        j++;
    }
    fputs("^\n", gErrFile);
 out:
    if (!JSREPORT_IS_WARNING(report->flags)) {
        if (report->errorNumber == JSMSG_OUT_OF_MEMORY) {
            gExitCode = EXITCODE_OUT_OF_MEMORY;
        } else {
            gExitCode = EXITCODE_RUNTIME_ERROR;
        }
    }
    JS_free(cx, prefix);
}

#if defined(SHELL_HACK) && defined(DEBUG) && defined(XP_UNIX)
static JSBool
Exec(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
    JSFunction *fun;
    const char *name, **nargv;
    uintN i, nargc;
    JSString *str;
    pid_t pid;
    int status;

    fun = JS_ValueToFunction(cx, argv[-2]);
    if (!fun)
        return JS_FALSE;
    if (!fun->atom)
        return JS_TRUE;
    name = JS_GetStringBytes(ATOM_TO_STRING(fun->atom));
    nargc = 1 + argc;
    nargv = JS_malloc(cx, (nargc + 1) * sizeof(char *));
    if (!nargv)
        return JS_FALSE;
    nargv[0] = name;
    for (i = 1; i < nargc; i++) {
        str = JS_ValueToString(cx, argv[i-1]);
        if (!str) {
            JS_free(cx, nargv);
            return JS_FALSE;
        }
        nargv[i] = JS_GetStringBytes(str);
    }
    nargv[nargc] = 0;
    pid = fork();
    switch (pid) {
      case -1:
        perror("js");
        break;
      case 0:
        (void) execvp(name, (char **)nargv);
        perror("js");
        exit(127);
      default:
        while (waitpid(pid, &status, 0) < 0 && errno == EINTR)
            continue;
        break;
    }
    JS_free(cx, nargv);
    return JS_TRUE;
}
#endif

static JSBool
global_enumerate(JSContext *cx, JSObject *obj)
{
    return JS_EnumerateStandardClasses(cx, obj);
}

static JSBool
global_resolve(JSContext *cx, JSObject *obj, jsid id, uintN flags,
               JSObject **objp)
{
    if ((flags & JSRESOLVE_ASSIGNING) == 0) {
        JSBool resolved;

        if (!JS_ResolveStandardClass(cx, obj, id, &resolved))
            return JS_FALSE;
        if (resolved) {
            *objp = obj;
            return JS_TRUE;
        }
    }

#if defined(SHELL_HACK) && defined(DEBUG) && defined(XP_UNIX)
    if ((flags & (JSRESOLVE_QUALIFIED | JSRESOLVE_ASSIGNING)) == 0) {
        /*
         * Do this expensive hack only for unoptimized Unix builds, which are
         * not used for benchmarking.
         */
        char *path, *comp, *full;
        const char *name;
        JSBool ok, found;
        JSFunction *fun;

        if (!JSVAL_IS_STRING(id))
            return JS_TRUE;
        path = getenv("PATH");
        if (!path)
            return JS_TRUE;
        path = JS_strdup(cx, path);
        if (!path)
            return JS_FALSE;
        name = JS_GetStringBytes(JSVAL_TO_STRING(id));
        ok = JS_TRUE;
        for (comp = strtok(path, ":"); comp; comp = strtok(NULL, ":")) {
            if (*comp != '\0') {
                full = JS_smprintf("%s/%s", comp, name);
                if (!full) {
                    JS_ReportOutOfMemory(cx);
                    ok = JS_FALSE;
                    break;
                }
            } else {
                full = (char *)name;
            }
            found = (access(full, X_OK) == 0);
            if (*comp != '\0')
                free(full);
            if (found) {
                fun = JS_DefineFunction(cx, obj, name, Exec, 0,
                                        JSPROP_ENUMERATE);
                ok = (fun != NULL);
                if (ok)
                    *objp = obj;
                break;
            }
        }
        JS_free(cx, path);
        return ok;
    }
#else
    return JS_TRUE;
#endif
}

JSClass global_class = {
    "global", JSCLASS_NEW_RESOLVE | JSCLASS_GLOBAL_FLAGS,
    JS_PropertyStub,  JS_PropertyStub,
    JS_PropertyStub,  JS_StrictPropertyStub,
    global_enumerate, (JSResolveOp) global_resolve,
    JS_ConvertStub,   JS_FinalizeStub,
    JSCLASS_NO_OPTIONAL_MEMBERS
};

static JSBool
ContextCallback(JSContext *cx, uintN contextOp)
{
    if (contextOp == JSCONTEXT_NEW) {
        JS_SetErrorReporter(cx, my_ErrorReporter);
        JS_SetVersion(cx, JSVERSION_LATEST);
        SetContextOptions(cx);
    }
    return JS_TRUE;
}

static void _set_up_logging(
	SG_context * pCtx,
	SG_log_console__data * pcLogStdData,
	SG_log_text__data * pcLogFileData,
	SG_log_text__writer__daily_path__data * pcLogFileWriterData)
{
	// Code coppied from _my_main() in sg.c

	char * szLogLevel = NULL;
	char * szLogPath = NULL;
	SG_uint32							  logFileFlags = SG_LOG__FLAG__HANDLER_TYPE__ALL;
	// find the appropriate log path
	SG_ERR_CHECK(  SG_localsettings__get__sz(pCtx, SG_LOCALSETTING__LOG_PATH, NULL, &szLogPath, NULL)  );


	// get the configured log level
	SG_ERR_CHECK(  SG_localsettings__get__sz(pCtx, SG_LOCALSETTING__LOG_LEVEL, NULL, &szLogLevel, NULL)  );

	// register the stdout logger
	SG_ERR_CHECK(  SG_log_console__set_defaults(pCtx, pcLogStdData)  );
	SG_ERR_CHECK(  SG_log_console__register(pCtx, pcLogStdData, NULL, SG_LOG__FLAG__HANDLER_TYPE__NORMAL)  );

	// register the file logger
	SG_ERR_CHECK(  SG_log_text__set_defaults(pCtx, pcLogFileData)  );
	pcLogFileData->fWriter             = SG_log_text__writer__daily_path;
	pcLogFileData->pWriterData         = pcLogFileWriterData;
	pcLogFileData->szRegisterMessage   = NULL;
	pcLogFileData->szUnregisterMessage = NULL;
	if (szLogLevel != NULL)
	{
		if (SG_stricmp(szLogLevel, "quiet") == 0)
		{
			logFileFlags = SG_LOG__FLAG__HANDLER_TYPE__QUIET;
			pcLogFileData->bLogVerboseOperations = SG_FALSE;
			pcLogFileData->bLogVerboseValues     = SG_FALSE;
			pcLogFileData->szVerboseFormat       = NULL;
			pcLogFileData->szInfoFormat          = NULL;
		}
		else if (SG_stricmp(szLogLevel, "normal") == 0)
		{
			logFileFlags = SG_LOG__FLAG__HANDLER_TYPE__NORMAL;
			pcLogFileData->bLogVerboseOperations = SG_FALSE;
			pcLogFileData->bLogVerboseValues     = SG_FALSE;
			pcLogFileData->szVerboseFormat       = NULL;
		}
		else if (SG_stricmp(szLogLevel, "verbose") == 0)
		{
			logFileFlags = SG_LOG__FLAG__HANDLER_TYPE__ALL;
			pcLogFileData->szRegisterMessage   = "---- vscript started logging ----";
			pcLogFileData->szUnregisterMessage = "---- vscript stopped logging ----";
		}
	}
	logFileFlags |= SG_LOG__FLAG__DETAILED_MESSAGES;
	SG_ERR_CHECK(  SG_log_text__writer__daily_path__set_defaults(pCtx, pcLogFileWriterData)  );
	pcLogFileWriterData->bReopen          = SG_FALSE;
	pcLogFileWriterData->ePermissions     = 0666;
	if (szLogPath != NULL)
		SG_ERR_CHECK(  SG_PATHNAME__ALLOC__SZ(pCtx, &pcLogFileWriterData->pBasePath, szLogPath)  );
	else
		SG_ERR_CHECK(  SG_PATHNAME__ALLOC__LOG_DIRECTORY(pCtx, &pcLogFileWriterData->pBasePath)  );
	pcLogFileWriterData->szFilenameFormat = "vscript-%d-%02d-%02d.log";
	SG_ERR_CHECK(  SG_log_text__register(pCtx, pcLogFileData, NULL, logFileFlags)  );

fail:
	SG_context__err_reset(pCtx);

	SG_NULLFREE(pCtx, szLogPath);
	SG_NULLFREE(pCtx, szLogLevel);
}

static void _clean_up_logging(
	SG_context * pCtx,
	SG_log_console__data * pcLogStdData,
	SG_log_text__data * pcLogFileData,
	SG_log_text__writer__daily_path__data * pcLogFileWriterData)
{
	SG_ERR_IGNORE(  SG_log_text__unregister(pCtx, pcLogFileData)  );
	SG_ERR_IGNORE(  SG_log_console__unregister(pCtx, pcLogStdData)  );
	SG_PATHNAME_NULLFREE(pCtx, pcLogFileWriterData->pBasePath);
}

static int _vscript_context_teardown(SG_context **ppCtx)
{
    SG_context *pCtx = *ppCtx;
    int result = 0;

	//////////////////////////////////////////////////////////////////
	// Post-processing for errors captured in SG_context.
	if (SG_context__has_err(pCtx))
	{
		SG_context__err_to_console(pCtx, SG_CS_STDERR);
		SG_context__err_reset(pCtx);
	}

    *ppCtx = NULL;

    SG_lib__global_cleanup(pCtx);

    SG_CONTEXT_NULLFREE(pCtx);

    return(result);
}


static void _checkSkipModules(SG_context *pCtx, int *argc, char **argv, SG_bool *skipModules)
{
    int		i = 0;
    int		newArgc = 0;
    *skipModules = SG_FALSE;

    SG_UNUSED(pCtx);

    for ( i = 0; i < *argc; ++i )
    {
        if (strcmp(argv[i], "--no-modules") == 0)
        {
            *skipModules = SG_TRUE;
        }
        else
        {
            argv[newArgc] = argv[i];
            ++newArgc;
        }
    }

    *argc = newArgc;
}


int
main(int argc, char **argv)
{
    int stackDummy;
    SG_error err;
    SG_context * pCtx = NULL;
    JSRuntime *rt;
    JSContext *cx;
    SG_log_console__data cLogStdData;
    SG_log_text__data cLogFileData;
    SG_log_text__writer__daily_path__data cLogFileWriterData;
    JSObject *glob;
    int result;
    SG_bool skipModules = SG_FALSE;

    SG_zero(cLogStdData);
    SG_zero(cLogFileData);
    SG_zero(cLogFileWriterData);

    CheckHelpMessages();
    setlocale(LC_ALL, "");

    gStackBase = (jsuword)&stackDummy;

#ifdef XP_OS2
   /* these streams are normally line buffered on OS/2 and need a \n, *
    * so we need to unbuffer then to get a reasonable prompt          */
    setbuf(stdout,0);
    setbuf(stderr,0);
#endif

    gErrFile = stderr;
    gOutFile = stdout;

    argc--;
    argv++;

    err = SG_context__alloc(&pCtx);
    if (SG_IS_ERROR(err))
        return 1;
    SG_lib__global_initialize(pCtx);
    if (SG_context__has_err(pCtx))
    {
        _vscript_context_teardown(&pCtx);
        return 1;
    }

    _checkSkipModules(pCtx, &argc, argv, &skipModules);
    if (SG_context__has_err(pCtx))
    {
        _vscript_context_teardown(&pCtx);
        return 1;
    }

    SG_jscore__new_runtime(pCtx, ContextCallback, shell_functions, skipModules, &rt);
    if (SG_context__has_err(pCtx))
    {
        _vscript_context_teardown(&pCtx);
        return 1;
    }

    SG_jscore__new_context(pCtx, &cx, &glob, NULL);
    if (SG_context__has_err(pCtx))
    {
        _vscript_context_teardown(&pCtx);
        return 1;
    }

    JS_SetGlobalObject(cx, glob);

    //////////////////////////////////////////////////////////////////
    // Allocate and initialize the SG_context.
    _set_up_logging(pCtx, &cLogStdData, &cLogFileData, &cLogFileWriterData);

    result = ProcessArgs(cx, glob, argv, argc);

    _clean_up_logging(pCtx, &cLogStdData, &cLogFileData, &cLogFileWriterData);

    // TODO 2011/09/30 The following call removes "pCtx" from
    // TODO            the private data in the "cx".  This was
    // TODO            done for neatness before we destroy it.
    // TODO            And this has worked fine for years.
    // TODO
    // TODO            I have commented this out as an experiment
    // TODO            so that the "pCtx" is still available for
    // TODO            use in any GC/Finalize callbacks.
    // TODO
    // TODO SG_jsglue__set_sg_context(NULL, cx);
    JS_EndRequest(cx);
    JS_DestroyContext(cx);

    // End of SG_context post-processing.
    //////////////////////////////////////////////////////////////////

    result = result + _vscript_context_teardown(&pCtx);

    return result;
}
