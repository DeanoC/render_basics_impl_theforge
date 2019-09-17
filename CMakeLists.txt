cmake_minimum_required(VERSION 3.13 FATAL_ERROR)
get_directory_property(hasParent PARENT_DIRECTORY)
if(NOT hasParent)
	get_filename_component(PARENT_DIR ${CMAKE_CURRENT_SOURCE_DIR} DIRECTORY)
	set_property(GLOBAL PROPERTY GLOBAL_FETCHDEPS_BASE ${PARENT_DIR}/al2o3 )
	set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/out_libs)
	set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/out_bin)
	set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/out_bin)
	set(LIB_BASE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/src/libs/)
	include(FetchContent)
	FetchContent_Declare( al2o3 GIT_REPOSITORY https://github.com/DeanoC/al2o3 GIT_TAG master )
	FetchContent_GetProperties(al2o3)
	if(NOT al2o3_POPULATED)
		FetchContent_Populate(al2o3)
		add_subdirectory(${al2o3_SOURCE_DIR} ${al2o3_BINARY_DIR})
	endif()
endif ()

SET_MIN_VERSIONS()
set(BaseInterfaceName render_basics)
set(ImplName theforge)
set(LibName ${BaseInterfaceName}_impl_${ImplName})
project(${LibName})

set(Interface
		api.h
		)
set(Src
		api.cpp
		blitencoder.cpp
		buffer.cpp
		computeencoder.cpp
		framebuffer.cpp
		graphicsencoder.cpp
		stock.cpp
		view.cpp
		visualdebug.cpp
		visdebug.hpp
		)

set(Deps
		al2o3_platform
		al2o3_cadt
		al2o3_os
		render_basics_interface
		gfx_theforge
		gfx_shadercompiler
		gfx_imgui_al2o3_theforge_bindings
		)
ADD_LINK_TIME_IMPL(${BaseInterfaceName} ${ImplName} "${Interface}" "${Src}" "${Deps}")
