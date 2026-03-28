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
# Worker will have to link against these too, so it's guaranteed that it also has these libraries available
target_link_libraries(${PROJ} distributed pagmo)

SCGMS_LINK_COMMON_LIBRARIES(${PROJ})
APPLY_SCGMS_LIBRARY_BUILD_SETTINGS(${PROJ})
CONFIGURE_FILTER_OUTPUT(${PROJ})

# Add the current directory into target's include dirs, anything that links against this lib at build time will be able
# to include our headers
target_include_directories(tproblem_udp PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
