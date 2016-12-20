## Copyright 2010-2013 SourceGear, LLC
## 
## Licensed under the Apache License, Version 2.0 (the "License");
## you may not use this file except in compliance with the License.
## You may obtain a copy of the License at
## 
## http://www.apache.org/licenses/LICENSE-2.0
## 
## Unless required by applicable law or agreed to in writing, software
## distributed under the License is distributed on an "AS IS" BASIS,
## WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
## See the License for the specific language governing permissions and
## limitations under the License.


set(CMAKE_CXX_FLAGS "${CMAKE_C_FLAGS}")
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG}")
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE}")
set(CMAKE_CXX_FLAGS_MINSIZEREL "${CMAKE_C_FLAGS_MINSIZEREL}")
set(CMAKE_CXX_FLAGS_RELWITHDEBINFO "${CMAKE_C_FLAGS_RELWITHDEBINFO}")

set(OVERLAYDIR "OverlayHandlers")
set(OVERLAYPrefix "VeracityOverlay")
file(GLOB OVERLAYS ${OVERLAYDIR}/*.cpp ${OVERLAYDIR}/*.h ${OVERLAYDIR}/*.rgs)
#${CMAKE_CURRENT_BINARY_DIR}
INCLUDE_DIRECTORIES(${CMAKE_CURRENT_BINARY_DIR} ${CMAKE_CURRENT_SOURCE_DIR})

source_group("Generated" FILES ${CMAKE_CURRENT_BINARY_DIR}/VeracityTortoise_i.h ${CMAKE_CURRENT_BINARY_DIR}/VeracityTortoise_i.c)
ADD_CUSTOM_COMMAND(
	OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/VeracityTortoise_i.h ${CMAKE_CURRENT_BINARY_DIR}/VeracityTortoise_i.c
	DEPENDS VeracityTortoise.idl
	COMMAND ${CMAKE_COMMAND} -E remove  "${CMAKE_CURRENT_BINARY_DIR}/VeracityTortoise_i.h" "${CMAKE_CURRENT_BINARY_DIR}/VeracityTortoise_i.c" "${CMAKE_CURRENT_BINARY_DIR}/VeracityTortoise_p.c"
	COMMAND midl /D "_DEBUG" /W1 /nologo /char signed /env win32 /Oicf /h "${CMAKE_CURRENT_BINARY_DIR}/VeracityTortoise_i.h" /iid "${CMAKE_CURRENT_BINARY_DIR}/VeracityTortoise_i.c" /proxy "${CMAKE_CURRENT_BINARY_DIR}/VeracityTortoise_p.c" /tlb "${CMAKE_CURRENT_BINARY_DIR}/VeracityTortoise.tlb" /robust ${CMAKE_CURRENT_SOURCE_DIR}/VeracityTortoise.idl
	)
#message("Updating version number for VeracityTortoise.dll")

#This replaces the version numbers in the rc file with the ones that we get from cmake.
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/VeracityTortoise.rc.in ${CMAKE_CURRENT_BINARY_DIR}/VeracityTortoise.rc @ONLY)

add_library(VeracityTortoise SHARED Commands.cpp
	Commands.h compreg.cpp
	compreg.h dllmain.cpp
	VeracityCacheMessaging.h VeracityCacheMessaging.cpp
	GlobalHelpers.h GlobalHelpers.cpp
	dllmain.h ICommandBase.cpp ICommandBase.h
	resource.h stdafx.cpp stdafx.h
	VistaIconConverter.cpp VistaIconConverter.h
	targetver.h VeracityContextMenu.cpp VeracityContextMenu.h
	VeracityContextMenu.rgs VeracityTortoise.cpp VeracityTortoise.def  VeracityTortoise.idl
	VeracityTortoise.rc.in
	VeracityCopyHook.cpp VeracityCopyHook.h
	VeracityCopyHook.rgs
	icons_and_registry.rc
	${OVERLAYS}
	${CMAKE_CURRENT_BINARY_DIR}/VeracityTortoise_i.h ${CMAKE_CURRENT_BINARY_DIR}/VeracityTortoise_i.c 
	${CMAKE_CURRENT_BINARY_DIR}/VeracityTortoise.rc VeracityTortoise.rgs 
	)
source_group("OverlayHandlers" FILES GLOB ${OVERLAYS})
GET_TARGET_PROPERTY(LIB_NAME VeracityTortoise LOCATION)

set_target_properties(VeracityTortoise PROPERTIES LINKER_LANGUAGE CXX)
set_target_properties(VeracityTortoise PROPERTIES FOLDER "Windows/Tortoise")

IF(TORTOISE_DEV)
	set_property(TARGET VeracityTortoise APPEND PROPERTY COMPILE_DEFINITIONS  TORTOISE_DEV) #look at dllmain.cpp for what this controls.
	IF(ALLOW_TORTOISE_QDIR_DEBUG)
		set_property(TARGET VeracityTortoise APPEND PROPERTY COMPILE_DEFINITIONS  ALLOW_TORTOISE_QDIR_DEBUG) #look at dllmain.cpp for what this controls.
	endif () # ALLOW_TORTOISE_QDIR_DEBUG
	
endif () # TORTOISE_DEV

target_link_libraries(VeracityTortoise
	sglib sghash #sgtemplates
	sg_fs3
	#${SG_THIRDPARTY_LIBRARIES}
	#pcrt4 shell32
	)
#Uncomment this to get the dll to register itself after build.
#ADD_CUSTOM_COMMAND(
#	TARGET VeracityTortoise
#	POST_BUILD COMMAND regsvr32.exe /s ${LIB_NAME}
#	)
