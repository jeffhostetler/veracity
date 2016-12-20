@ECHO OFF
REM # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # #
REM # Copyright 2010-2013 SourceGear, LLC
REM # 
REM # Licensed under the Apache License, Version 2.0 (the "License");
REM # you may not use this file except in compliance with the License.
REM # You may obtain a copy of the License at
REM # 
REM # http://www.apache.org/licenses/LICENSE-2.0
REM # 
REM # Unless required by applicable law or agreed to in writing, software
REM # distributed under the License is distributed on an "AS IS" BASIS,
REM # WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
REM # See the License for the specific language governing permissions and
REM # limitations under the License.
REM # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # #

REM c_test.bat
REM usage: c_test.bat c_test_name ${CMAKE_CURRENT_SOURCE_DIR} ${MY_RELEXE_DIR}
REM

REM %~1 is %1 with surrounding quotes removed
SET TEST=%~1
ECHO TEST is %TEST%

SET SRCDIR=%~2
ECHO SRCDIR is %SRCDIR%

SET RELEXEDIR=%~3
ECHO RELEXEDIR is %RELEXEDIR%

REM %cd% is like `pwd`
SET BINDIR=%cd%
ECHO BINDIR is %BINDIR%

SET TEMPDIR=%BINDIR%\TEMP\%TEST%
ECHO TEMPDIR is %TEMPDIR%

For /f "tokens=2-4 delims=/ " %%a in ('date /t') do (set mydate=%%c-%%a-%%b)
For /f "tokens=1-2 delims=/:" %%a in ('time /t') do (set mytime=%%a%%b)

SET SGCLOSET=
SET SGCLOSET=%TEMPDIR%\.sgcloset-test.%mydate%;%mytime%
IF "%SGCLOSET%"=="" GOTO :NO_SGCLOSET_LOCATION
ECHO SGCLOSET is %SGCLOSET%

REM We need vv to be in our path.
SET PATH=%BINDIR%\..\src\cmd\%RELEXEDIR%;%PATH%
ECHO Path is %PATH%

ECHO Running test %TEST%...
%RELEXEDIR%\%TEST% "%SRCDIR%"
SET RESULT_TEST=%ERRORLEVEL%

EXIT %RESULT_TEST%

:NO_SGCLOSET_LOCATION
ECHO ## %TEST% FAILED to find a location for .sgcloset-test
SET LOCALAPPDATA
SET USERPROFILE
EXIT 1

:SERVER_FILES_CANNOT_BE_DEFAULT
ECHO ## %TEST% FAILED due to default server/files setting
vv config show server/files
EXIT 1
