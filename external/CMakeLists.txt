CMAKE_MINIMUM_REQUIRED(VERSION 3.0)

# currently supporting only Win32 target
IF(WIN32)

	SET(PROJ "external")

	CONFIGURE_COMMON(${PROJ})

	FILE(GLOB SRC_FILES "src/*.cpp" "src/*.c" "src/*.h")
	FILE(GLOB_RECURSE DMMS_LIB_FILES "src/dmms_lib/*.cpp" "src/dmms_lib/*.c" "src/dmms_lib/*.h")
	FILE(GLOB_RECURSE DMMS_PROC_FILES "src/dmms_proc/*.cpp" "src/dmms_proc/*.c" "src/dmms_proc/*.h")
	FILE(GLOB_RECURSE MATLAB_SRC_FILES "src/matlab/*.cpp" "src/matlab/*.c" "src/matlab/*.h")
	IF(WIN32)
		FILE(GLOB SRC_WIN_FILES "src/win/*.cpp" "src/win/*.h" "src/win/*.def")
		SET(SRC_FILES "${SRC_FILES};${SRC_WIN_FILES}")
	ENDIF()

	SET(SRC_FILES "${SRC_FILES};${DMMS_LIB_FILES};${DMMS_PROC_FILES};${MATLAB_SRC_FILES}")

	SET(EXTERNAL_UTILS_SRC )
	ADD_UTILS_FILE(EXTERNAL_UTILS_SRC "string_utils.h")
	ADD_UTILS_FILE(EXTERNAL_UTILS_SRC "string_utils.cpp")

	FIND_PACKAGE(DmmsLib)

	ADD_LIBRARY(${PROJ} SHARED ${SRC_FILES};${COMMON_FILES};${EXTERNAL_UTILS_SRC})
	CONFIGURE_FILTER_OUTPUT(${PROJ})

	TARGET_LINK_LIBRARIES(${PROJ} factory ${CMAKE_CURRENT_SOURCE_DIR}/src/matlab/libmx_win64_ms.lib)
	TARGET_INCLUDE_DIRECTORIES(${PROJ} PUBLIC ${DMMSLIB_INCLUDE_DIRS})

ENDIF()
