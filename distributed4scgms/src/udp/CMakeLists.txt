CMAKE_MINIMUM_REQUIRED(VERSION 3.10)
SET(PROJ "tproblem_udp")

file(GLOB SRC_FILES "./*.cpp" "./*.c" "./*.h")

IF(WIN32)
    FILE(GLOB SRC_WIN_FILES "./win/*.cpp" "./win/*.h" "./win/*.def")
    SET(SRC_FILES "${SRC_FILES};${SRC_WIN_FILES}")
ENDIF()

SCGMS_ADD_LIBRARY(${PROJ} SHARED ${SRC_FILES})

# We need to link against distributed so we can derive from udp_base
# And also link against pagmo so we can access boost::serialize
# distributed4scgms is needed because of CRemap
target_link_libraries(${PROJ} distributed pagmo distributed4scgms)

SCGMS_LINK_COMMON_LIBRARIES(${PROJ})
APPLY_SCGMS_LIBRARY_BUILD_SETTINGS(${PROJ})
CONFIGURE_FILTER_OUTPUT(${PROJ})