cmake_minimum_required(VERSION 3.21)

# vcpkg setup
if(DEFINED ENV{VCPKG_ROOT} AND NOT DEFINED CMAKE_TOOLCHAIN_FILE)
	set(CMAKE_TOOLCHAIN_FILE "$ENV{VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake" CACHE STRING "")
endif()

set(VCPKG_TARGET_TRIPLET "x64-windows-static-md" CACHE STRING "")
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>DLL" CACHE STRING "")

project(
	FPInertia
	VERSION 1.0.0
	LANGUAGES CXX
)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_INTERPROCEDURAL_OPTIMIZATION ON)
set(CMAKE_INTERPROCEDURAL_OPTIMIZATION_DEBUG OFF)

# Output directory - use generator expression to avoid Release/Debug subdirectories
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/Compile/SKSE/Plugins")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/Compile/SKSE/Plugins")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELEASE "${CMAKE_SOURCE_DIR}/Compile/SKSE/Plugins")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY_RELEASE "${CMAKE_SOURCE_DIR}/Compile/SKSE/Plugins")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG "${CMAKE_SOURCE_DIR}/Compile/SKSE/Plugins")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY_DEBUG "${CMAKE_SOURCE_DIR}/Compile/SKSE/Plugins")

# Add CommonLibSSE-NG (disable tests)
set(BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(SKSE_SUPPORT_XBYAK OFF CACHE BOOL "" FORCE)
add_subdirectory(extern/CommonLibSSE CommonLibSSE)

# Find nlohmann_json
find_package(nlohmann_json CONFIG REQUIRED)

# Source files
set(SOURCES
	src/main.cpp
	src/Settings.cpp
	src/Inertia.cpp
	src/Menu.cpp
	src/InertiaPresets.cpp
)

set(HEADERS
	src/PCH.h
	src/Settings.h
	src/Inertia.h
	src/Menu.h
	src/SKSEMenuFramework.h
	src/InertiaPresets.h
)

# Create DLL
add_library(${PROJECT_NAME} SHARED
	${SOURCES}
	${HEADERS}
)

target_compile_features(${PROJECT_NAME} PRIVATE cxx_std_23)

target_compile_definitions(${PROJECT_NAME} PRIVATE
	_UNICODE
	UNICODE
	NOMINMAX
	_CRT_SECURE_NO_WARNINGS
)

target_include_directories(${PROJECT_NAME} PRIVATE
	${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_link_libraries(${PROJECT_NAME} PRIVATE
	CommonLibSSE
	nlohmann_json::nlohmann_json
)

target_precompile_headers(${PROJECT_NAME} PRIVATE
	src/PCH.h
)

if(MSVC)
	target_compile_options(${PROJECT_NAME} PRIVATE
		/sdl
		/utf-8
		/Zi
		/permissive-
		/Zc:preprocessor
		/EHsc
		/W4
		/WX-
		/wd4100  # unreferenced formal parameter
		/wd4189  # local variable initialized but not referenced
	)

	target_link_options(${PROJECT_NAME} PRIVATE
		"$<$<CONFIG:DEBUG>:/INCREMENTAL>"
		"$<$<CONFIG:RELEASE>:/INCREMENTAL:NO;/OPT:REF;/OPT:ICF;/DEBUG:FULL>"
	)
endif()

# Copy INI file
configure_file(
	${CMAKE_CURRENT_SOURCE_DIR}/FPInertia.ini
	${CMAKE_SOURCE_DIR}/Compile/SKSE/Plugins/FPInertia.ini
	COPYONLY
)

