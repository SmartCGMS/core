CMAKE_MINIMUM_REQUIRED(VERSION 3.0)

SET(PROJ "controllers")

CONFIGURE_COMMON(${PROJ})

FILE(GLOB SRC_FILES "src/descriptor.cpp" "src/descriptor.h"
					"src/betapid/*.cpp" "src/betapid/*.c" "src/betapid/*.h"
					"src/lgs/*.cpp" "src/lgs/*.c" "src/lgs/*.h"
					"src/absorption/*.cpp" "src/absorption/*.c" "src/absorption/*.h"
					"src/activity_detection/*.cpp" "src/activity_detection/*.c" "src/activity_detection/*.h"
)

IF(WIN32)
	FILE(GLOB SRC_WIN_FILES "src/win/*.cpp" "src/win/*.h" "src/win/*.def")
	SET(SRC_FILES "${SRC_FILES};${SRC_WIN_FILES}")
ENDIF()

SCGMS_ADD_LIBRARY(${PROJ} SHARED ${SRC_FILES};${COMMON_FILES})
CONFIGURE_FILTER_OUTPUT(${PROJ})

TARGET_LINK_LIBRARIES(${PROJ})
