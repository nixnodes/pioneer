cmake_minimum_required(VERSION 3.4)
project(pioneer LANGUAGES CXX)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

include(GNUInstallDirs)

if (CMAKE_COMPILER_IS_GNUCXX)
	add_compile_options(-fdiagnostics-color)
endif (CMAKE_COMPILER_IS_GNUCXX)

if (NOT CMAKE_BUILD_TYPE)
	set(CMAKE_BUILD_TYPE RelWithDebInfo CACHE STRING
		"Choose the type of build, options are: None(CMAKE_CXX_FLAGS or CMAKE_C_FLAGS used) Debug Release RelWithDebInfo MinSizeRel."
		FORCE)
	set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS None Debug Release RelWithDebInfo MinSizeRel)
endif()

# Get the GIT hash of the latest commit
include(FindGit OPTIONAL)
if (GIT_FOUND)
	execute_process(
		COMMAND ${GIT_EXECUTABLE} rev-parse --short HEAD
		WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
		OUTPUT_VARIABLE PROJECT_VERSION_GIT
		OUTPUT_STRIP_TRAILING_WHITESPACE
	)

	add_definitions(-DPIONEER_EXTRAVERSION="${PROJECT_VERSION_GIT}")
endif()

string(TIMESTAMP PROJECT_VERSION "%Y%m%d")
add_definitions(-DPIONEER_VERSION="${PROJECT_VERSION}")

if (NOT PIONEER_DATA_DIR)
	file(TO_NATIVE_PATH ${CMAKE_INSTALL_FULL_DATADIR}/pioneer/data PIONEER_DATA_DIR)
endif(NOT PIONEER_DATA_DIR)
add_definitions(-DPIONEER_DATA_DIR="${PIONEER_DATA_DIR}")

if (MINGW)
	# Enable PRIxYY macros on MinGW
	add_definitions(-D__STDC_FORMAT_MACROS)
endif (MINGW)

list(APPEND SRC_FOLDERS
	src
	src/collider
	src/galaxy
	src/gameui
	src/graphics
	src/graphics/dummy
	src/graphics/gl2
	src/graphics/opengl
	src/gui
	src/scenegraph
	src/terrain
	src/text
	src/ui
)

foreach (each IN LISTS SRC_FOLDERS)
	file(GLOB src_files RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} ${each}/*.cpp)
	list(APPEND CXX_FILES ${src_files})
endforeach (each IN LISTS SRC_FOLDERS)

list(REMOVE_ITEM CXX_FILES
	src/modelcompiler.cpp
	src/tests.cpp
	src/textstress.cpp
	src/uitest.cpp
)

if (WIN32)
	list(APPEND CXX_FILES
		src/win32/FileSystemWin32.cpp
		src/win32/OSWin32.cpp
		src/win32/TextUtils.cpp
	)
else (WIN32)
	list(APPEND CXX_FILES
		src/posix/FileSystemPosix.cpp
		src/posix/OSPosix.cpp
	)
endif (WIN32)

option(USE_SYSTEM_LIBGLEW "Use the system's libglew" OFF)
if (USE_SYSTEM_LIBGLEW)
	find_package(GLEW REQUIRED)
endif (USE_SYSTEM_LIBGLEW)

option(USE_SYSTEM_LIBLUA "Use the system's liblua" OFF)
if (USE_SYSTEM_LIBLUA)
	find_package(Lua 5.2 EXACT REQUIRED)
	include_directories(${LUA_INCLUDE_DIR})
	if (WIN32)
		add_definitions(-DLUA_BUILD_AS_DLL)
	endif (WIN32)
endif (USE_SYSTEM_LIBLUA)

find_package(Freetype REQUIRED)
find_package(OpenGL REQUIRED)
find_package(PkgConfig REQUIRED)
find_package(PNG REQUIRED)
find_package(SDL2 REQUIRED)

pkg_check_modules(ASSIMP REQUIRED assimp)
pkg_check_modules(SDL2_IMAGE REQUIRED SDL2_image)
pkg_check_modules(SIGCPP REQUIRED sigc++-2.0)
pkg_check_modules(VORBISFILE REQUIRED vorbisfile)

include_directories(
	${CMAKE_SOURCE_DIR}/contrib
	${CMAKE_SOURCE_DIR}/src
	${ASSIMP_INCLUDE_DIRS}
	${FREETYPE_INCLUDE_DIRS}
	${OPENGL_INCLUDE_DIRS}
	${PNG_INCLUDE_DIRS}
	${SDL2_INCLUDE_DIRS}
	${SDL2_IMAGE_INCLUDE_DIRS}
	${SIGCPP_INCLUDE_DIRS}
	${VORBISFILE_INCLUDE_DIRS}
)

if (NOT USE_SYSTEM_LIBGLEW)
	add_subdirectory(contrib/glew)
	add_library(GLEW::GLEW ALIAS glew)
endif (NOT USE_SYSTEM_LIBGLEW)
add_subdirectory(contrib/imgui)
add_subdirectory(contrib/jenkins)
add_subdirectory(contrib/json)
add_subdirectory(contrib/PicoDDS)
add_subdirectory(contrib/profiler)
if (NOT USE_SYSTEM_LIBLUA)
	add_subdirectory(contrib/lua)
	set(LUA_LIBRARIES lua)
	include_directories(contrib/lua)
endif (NOT USE_SYSTEM_LIBLUA)

add_executable(${PROJECT_NAME} WIN32 ${CXX_FILES})
target_link_libraries(${PROJECT_NAME} LINK_PRIVATE
	${ASSIMP_LIBRARIES}
	${FREETYPE_LIBRARIES}
	${OPENGL_LIBRARIES}
	${PNG_LIBRARIES}
	${SDL2_LIBRARIES}
	${SDL2_IMAGE_LIBRARIES}
	${SIGCPP_LIBRARIES}
	${VORBISFILE_LIBRARIES}
	${LUA_LIBRARIES}
	GLEW::GLEW
	imgui
	jenkins
	json
	picodds
	profiler
)

if (WIN32)
	target_link_libraries(${PROJECT_NAME} LINK_PRIVATE shlwapi)
endif (WIN32)

set_target_properties(${PROJECT_NAME} PROPERTIES
	CXX_STANDARD 11
	CXX_STANDARD_REQUIRED ON
	CXX_EXTENSIONS ON
)

install(TARGETS ${PROJECT_NAME}
	RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)
install(DIRECTORY data
	DESTINATION ${CMAKE_INSTALL_DATADIR}/pioneer
	PATTERN "listdata.*" EXCLUDE
	PATTERN "Makefile.am" EXCLUDE
)
