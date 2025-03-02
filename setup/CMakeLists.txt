cmake_minimum_required(VERSION 3.25)

project(rw_data)

add_executable(setup WIN32 res/setup.rc main.cpp settings.cpp hw.cpp)
set_source_files_properties(res/setup.rc PROPERTIES COMPILE_FLAGS "/dNOT_WS_VISIBLE")

if (MSVC_VERSION GREATER_EQUAL 1300)
	target_include_directories(setup PRIVATE
		$ENV{WindowsSdkDir}/Include/$ENV{WindowsSDKVersion}/shared
	)
endif()

target_include_directories(setup PRIVATE $ENV{DXSDK_DIR}/Include)

target_link_directories(setup PRIVATE $ENV{DXSDK_DIR}/Lib)
target_link_libraries(setup PRIVATE comctl32 d3d8 ddraw dxguid)