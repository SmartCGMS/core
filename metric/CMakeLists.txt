CMAKE_MINIMUM_REQUIRED(VERSION 3.0)

SET(PROJ "metric")

CONFIGURE_COMMON(${PROJ})

FILE(GLOB SRC_FILES "src/*.cpp" "src/*.c" "src/*.h" "src/diabetes_grid/*.h" "src/diabetes_grid/*.c" "src/diabetes_grid/*.cpp")
IF(WIN32)
	FILE(GLOB SRC_WIN_FILES "src/win/*.cpp" "src/win/*.h" "src/win/*.def")
	SET(SRC_FILES "${SRC_FILES};${SRC_WIN_FILES}")
ENDIF()

SET(METRIC_UTILS_SRC )
ADD_UTILS_FILE(METRIC_UTILS_SRC "math_utils.h")
ADD_UTILS_FILE(METRIC_UTILS_SRC "math_utils.cpp")

SCGMS_ADD_LIBRARY(${PROJ} SHARED ${SRC_FILES};${COMMON_FILES};${METRIC_UTILS_SRC})
CONFIGURE_FILTER_OUTPUT(${PROJ})

TARGET_LINK_LIBRARIES(${PROJ} scgms)
