
cmake_minimum_required(VERSION 3.5.1)

project(notes_client)

option(BUILD_DEBUG "Build a debug release" ON)
option(BUILD_APPIMAGE "Build for appimage" OFF)
option(BUILD_REGULAR "Regular build" OFF)
option(WITH_C++FILESYSTEM "If true, std::filesystem::lexically_normal() will be used in the file browser. Else, an approximation with realpath will be tried" TRUE)

#bin type
if(${BUILD_APPIMAGE})

	set(BUILD_VERSION "AppImage")
elseif(${BUILD_REGULAR})

	set(BUILD_VERSION "bin")
else()

	message(FATAL_ERROR "A build type (AppImage or regular) must be defined")
endif()

#release status
if(${BUILD_DEBUG})

	set(CMAKE_BUILD_TYPE Debug)
	set(BUILD_VERSION "${BUILD_VERSION}.debug")
else()
	set(CMAKE_BUILD_TYPE Release)
endif()

#application version
set(MAJOR_VERSION 0)
set(MINOR_VERSION 0)
set(PATCH_VERSION 0)
set(BUILD_VERSION "\"${BUILD_VERSION}\"") #add some quotes to pacify the macro preprocessor

if(${CMAKE_VERSION} VERSION_LESS "3.22.0")

	add_definitions(-DMAJOR_VERSION=${MAJOR_VERSION})
	add_definitions(-DMINOR_VERSION=${MINOR_VERSION})
	add_definitions(-DPATCH_VERSION=${PATCH_VERSION})
	add_definitions(-DBUILD_VERSION=${BUILD_VERSION})
else()

	add_compile_definitions(MAJOR_VERSION=${MAJOR_VERSION})
	add_compile_definitions(MINOR_VERSION=${MINOR_VERSION})
	add_compile_definitions(PATCH_VERSION=${PATCH_VERSION})
	add_compile_definitions(BUILD_VERSION=${BUILD_VERSION})
endif()

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

message("will build ${MAJOR_VERSION}.${MINOR_VERSION}.${PATCH_VERSION}-${BUILD_VERSION}")

#-Werror
add_compile_options(-Wall -Wextra -Wundef -Wcast-align -Wwrite-strings -Wlogical-op -Wmissing-declarations -Wredundant-decls -Wshadow -Woverloaded-virtual -Wno-deprecated -ansi -pedantic -fno-rtti)

include_directories("${CMAKE_CURRENT_LIST_DIR}/include")

add_subdirectory("${PROJECT_SOURCE_DIR}/src")

#bin type
if(${BUILD_APPIMAGE})

	if(${CMAKE_VERSION} VERSION_LESS "3.22.0")

		add_definitions(-DAS_APPIMAGE=1)
	else()

		add_compile_definitions(AS_APPIMAGE=1)
	endif()
elseif(${BUILD_REGULAR})

	if(${CMAKE_VERSION} VERSION_LESS "3.22.0")

		add_definitions(-DAS_REGULAR=1)
	else()

		add_compile_definitions(AS_REGULAR=1)
	endif()
endif()

#add the framework dependencies according to release status
add_library(lm SHARED IMPORTED)
add_library(curlw SHARED IMPORTED)
add_library(tools SHARED IMPORTED)
add_library(appenv SHARED IMPORTED)

set_target_properties(lm PROPERTIES IMPORTED_LOCATION /usr/local/lib/liblm.so)
set_target_properties(curlw PROPERTIES IMPORTED_LOCATION /usr/local/lib/libcurlw.so)
set_target_properties(tools PROPERTIES IMPORTED_LOCATION /usr/local/lib/libtools.so)
set_target_properties(appenv PROPERTIES IMPORTED_LOCATION /usr/local/lib/libappenv.so)

if(${BUILD_DEBUG})

	if(${CMAKE_VERSION} VERSION_LESS "3.12.0")

		add_definitions(-DWITH_DEBUG)
	else()

		add_compile_definitions(WITH_DEBUG)
	endif()

else()

endif()

#extra flag...
if(${WITH_C++FILESYSTEM})
	if(${CMAKE_VERSION} VERSION_LESS "3.12.0")

		add_definitions(-DWITH_FILESYSTEM)
	else()

		add_compile_definitions(WITH_FILESYSTEM)
	endif()
endif()

#Virtual library so we don't have to build again and again for all tests...
add_library(applib STATIC ${SOURCE})
set_target_properties(applib PROPERTIES OUTPUT_NAME applib)

#Configure the executable, we want it into the "build directory" and we want all resources copied
#to the build directory.
add_executable(notes-client src/main.cpp)
target_link_libraries(notes-client applib curlw lm curl tools appenv stdc++fs)

