cmake_minimum_required(VERSION 3.25)

project(setup)

add_executable(setup WIN32 res/setup.rc main.cpp settings.cpp hw.cpp)

if (MSVC_VERSION GREATER_EQUAL 1300)
	target_include_directories(setup PRIVATE
		$ENV{WindowsSdkDir}/Include/$ENV{WindowsSDKVersion}/shared
	)
endif()

# The Steam setup.exe was built with Visual C++ 2008 (MSVCR90.dll), Release, /GS and whole program optimization
if (MSVC_VERSION EQUAL 1500)
	target_compile_options(setup PRIVATE /O2 /GL /GS)
	target_link_options(setup PRIVATE /LTCG)
endif()

target_include_directories(setup PRIVATE $ENV{DXSDK_DIR}/Include)

target_link_directories(setup PRIVATE $ENV{DXSDK_DIR}/Lib)
target_link_libraries(setup PRIVATE comctl32 d3d8 ddraw dxguid)
