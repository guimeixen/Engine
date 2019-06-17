## This file is a quick tutorial on writing CMakeLists for targeting the Vita
cmake_minimum_required(VERSION 2.8)
cmake_policy(SET CMP0015 NEW)
## This includes the Vita toolchain, must go before project definition
# It is a convenience so you do not have to type 
# -DCMAKE_TOOLCHAIN_FILE=$VITASDK/share/vita.toolchain.cmake for cmake. It is 
# highly recommended that you include this block for all projects.
if(NOT DEFINED CMAKE_TOOLCHAIN_FILE)
  if(DEFINED ENV{VITASDK})
    set(CMAKE_TOOLCHAIN_FILE "$ENV{VITASDK}/share/vita.toolchain.cmake" CACHE PATH "toolchain file")
  else()
    message(FATAL_ERROR "Please define VITASDK to point to your SDK path!")
  endif()
endif()

## Define project parameters here
# Name of the project
project(vitaTestApp)
# This line adds Vita helper macros, must go after project definition in order 
# to build Vita specific artifacts (self/vpk).
include("${VITASDK}/share/vita.cmake" REQUIRED)

## Configuration options for this app
# Display name (under bubble in LiveArea)
set(VITA_APP_NAME "Vita Test App")
# Unique ID must be exactly 9 characters. Recommended: XXXXYYYYY where X = 
# unique string of developer and Y = a unique number for this app
set(VITA_TITLEID  "VI3D00067")
# Optional version string to show in LiveArea's more info screen
set(VITA_VERSION  "01.00")

## Flags and includes for building
# Note that we make sure not to overwrite previous flags
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -O3")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O3 -std=gnu++11 -DGLM_ENABLE_EXPERIMENTAL -DGLM_FORCE_RADIANS -DGLM_FORCE_DEPTH_ZERO_TO_ONE -DVITA")
# Optional. You can specify more param.sfo flags this way.
set(VITA_MKSFOEX_FLAGS "${VITA_MKSFOEX_FLAGS} -d PARENTAL_LEVEL=1")

# Add any additional include paths here
include_directories(
	../Engine
	../
	../include/bullet
)

# Add any additional library paths here
# ${CMAKE_CURRENT_BINARY_DIR} lets you use any library currently being built
link_directories(
  ${CMAKE_CURRENT_BINARY_DIR}
  ../
)

## Build and link
# Add all the files needed to compile here
add_executable(vitaTestApp
  main.cpp
)

# Library to link to (drop the -l prefix). This will mostly be stubs.
target_link_libraries(vitaTestApp
	VitaEngine
	Lua5_3_4
	assimp
	zlib
	BulletDynamics
	BulletCollision
	BulletSoftBody
	LinearMath
	SceGxm_stub
	SceDisplay_stub
	SceCtrl_stub
	SceAppMgr_stub 
)

## Create Vita files
vita_create_self(vitaTestApp.self vitaTestApp)
# The FILE directive lets you add additional files to the VPK, the syntax is 
# FILE src_path dst_path_in_vpk. In this case, we add the LiveArea paths.
vita_create_vpk(vitaTestApp.vpk ${VITA_TITLEID} vitaTestApp.self
  VERSION ${VITA_VERSION}
  NAME ${VITA_APP_NAME}
  FILE sce_sys/icon0.png sce_sys/icon0.png
  FILE sce_sys/livearea/contents/bg.png sce_sys/livearea/contents/bg.png
  FILE sce_sys/livearea/contents/startup.png sce_sys/livearea/contents/startup.png
  FILE sce_sys/livearea/contents/template.xml sce_sys/livearea/contents/template.xml
  FILE Data/test.lua Data/test.lua
  FILE Data/Shaders/GXM/color_v_cg.gxp Data/Shaders/GXM/color_v_cg.gxp
  FILE Data/Shaders/GXM/color_f_cg.gxp Data/Shaders/GXM/color_f_cg.gxp
  FILE Data/Shaders/GXM/texture_v_cg.gxp Data/Shaders/GXM/texture_v_cg.gxp
  FILE Data/Shaders/GXM/texture_f_cg.gxp Data/Shaders/GXM/texture_f_cg.gxp
  FILE Data/Shaders/GXM/text_v_cg.gxp Data/Shaders/GXM/text_v_cg.gxp
  FILE Data/Shaders/GXM/text_f_cg.gxp Data/Shaders/GXM/text_f_cg.gxp
  FILE Data/Materials/color_mat.lua Data/Materials/color_mat.lua
  FILE Data/Materials/texture_mat.lua Data/Materials/texture_mat.lua
  FILE Data/Materials/text_mat.lua Data/Materials/text_mat.lua
  FILE Data/Textures/sand.png Data/Textures/sand.png
  FILE Data/Textures/jorvik.png Data/Textures/jorvik.png
  FILE Data/Textures/jorvik.fnt Data/Textures/jorvik.fnt
  FILE Data/Materials/sand.mat Data/Materials/sand.mat
  FILE Data/Materials/col.mat Data/Materials/col.mat
  FILE Data/Models/trash_can.obj Data/Models/trash_can.obj
  FILE Data/Models/trash_can_d.jpg Data/Models/trash_can_d.jpg
)
