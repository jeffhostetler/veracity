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
 * @file sg_localsettings_typedefs.h
 *
 * @details Typedefs for working with SG_localsettings__ routines.
 */

#ifndef H_SG_LOCALSETTINGS_TYPEDEFS_H
#define H_SG_LOCALSETTINGS_TYPEDEFS_H

BEGIN_EXTERN_C;

//////////////////////////////////////////////////////////////////////


/*
 * Here are some #define's for the names of local settings.
 *
 * These can be used to specify the setting when calling an initialize,
 * get, set, or reset function in SG_localsettings. (The parameter name
 * is always pSettingName.)
 *
 * This is not meant to be comprehensive list of all settings, just
 * those that are used by sglib and are relatively public. Please make
 * #defines for client-specific settings elsewhere, or else just use
 * plain old strings without #defines.
 */
#define SG_LOCALSETTING__IGNORES                   "ignores"
#define SG_LOCALSETTING__NEWREPO_DRIVER            "new_repo/driver"
#define SG_LOCALSETTING__NEWREPO_CONNECTSTRING     "new_repo/connect_string"
#define SG_LOCALSETTING__NEWREPO_HASHMETHOD        "new_repo/hash_method"
#define SG_LOCALSETTING__PATHS                     "paths"
#define SG_LOCALSETTING__PATHS_DEFAULT             "paths/default"
#define SG_LOCALSETTING__SYNC_TARGETS			   "sync_targets"
#define SG_LOCALSETTING__SERVER_DEFAULT_REPO       "server/repos/default"
#define SG_LOCALSETTING__SERVER_REPO_INCLUDES      "server/repos/include"
#define SG_LOCALSETTING__SERVER_REPO_EXCLUDES      "server/repos/exclude"
#define SG_LOCALSETTING__SERVER_FILES              "server/files"
#define SG_LOCALSETTING__SERVER_SSJS_MUTABLE       "server/ssjs_mutable"
#define SG_LOCALSETTING__SERVER_HOSTNAME           "server/hostname"
#define SG_LOCALSETTING__SERVER_SCHEME             "server/scheme"
#define SG_LOCALSETTING__SERVER_APPLICATIONROOT    "server/application_root"
#define SG_LOCALSETTING__SERVER_DEBUG_DELAY        "server/debug_delay"
#define SG_LOCALSETTING__SERVER_ENABLE_DIAGNOSTICS "server/enable_diagnostics"
#define SG_LOCALSETTING__SERVER_REMOTE_AJAX_LIBS      "server/remote_ajax_libs"
#define SG_LOCALSETTING__SERVER_READONLY           "server/readonly"
#define SG_LOCALSETTING__SERVER_CLONE_ALLOWED      "server/clone_allowed"
#define SG_LOCALSETTING__USERID                    "whoami/userid"
#define SG_LOCALSETTING__USERNAME                  "whoami/username"
#define SG_LOCALSETTING__VERIFY_SSL_CERTS          "network/verify_ssl_certs"
#define SG_LOCALSETTING__LOG_PATH                  "log/path"
#define SG_LOCALSETTING__LOG_LEVEL                 "log/level"
#define SG_LOCALSETTING__TORTOISE_HISTORY_FILTER_DEFAULTS	"Tortoise/History/FilterDefaults"
#define SG_LOCALSETTING__TORTOISE_REVERT__SAVE_BACKUPS	"Tortoise/revert__save_backups"
#define SG_LOCALSETTING__TORTOISE_EXPLORER__HIDE_MENU_IF_NO_WORKING_COPY	"Tortoise/explorer/hide_menu_if_no_working_copy"

#define SG_LOCALSETTING__SCOPE__DEFAULT  "/default"
#define SG_LOCALSETTING__SCOPE__MACHINE  "/machine"
#define SG_LOCALSETTING__SCOPE__ADMIN    "/admin"
#define SG_LOCALSETTING__SCOPE__REPO     "/repo"
#define SG_LOCALSETTING__SCOPE__INSTANCE "/instance"

#define SG_FILETOOLCONFIG__CLASSES       "fileclasses"
#define SG_FILETOOLCONFIG__CLASSPATTERNS "patterns"
#define SG_FILETOOLCONFIG__BINARY_CLASS  ":binary"
#define SG_FILETOOLCONFIG__TEXT_CLASS    ":text"
#define SG_FILETOOLCONFIG__TOOLS         "filetools"
#define SG_FILETOOLCONFIG__TOOLPATH      "path"
#define SG_FILETOOLCONFIG__TOOLARGS      "args"
#define SG_FILETOOLCONFIG__TOOLSTDIN     "stdin"
#define SG_FILETOOLCONFIG__TOOLSTDOUT    "stdout"
#define SG_FILETOOLCONFIG__TOOLSTDERR    "stderr"
#define SG_FILETOOLCONFIG__TOOLFLAGS     "flags"
#define SG_FILETOOLCONFIG__TOOLEXITCODES "exit"
#define SG_FILETOOLCONFIG__BINDINGS      "filetoolbindings"

//////////////////////////////////////////////////////////////////

END_EXTERN_C;

#endif//H_SG_LOCALSETTINGS_TYPEDEFS_H
