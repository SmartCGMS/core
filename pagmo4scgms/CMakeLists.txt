CMAKE_MINIMUM_REQUIRED(VERSION 3.10)

SET(PROJ "pagmo4scgms")


message(${CMAKE_CURRENT_LIST_DIR})
message("${CMAKE_CURRENT_LIST_DIR}/cmake/")
message("${CMAKE_CURRENT_LIST_DIR}/src/external")
message("HERE HERE HERE")

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/cmake/")
# Download all external dependencies into the /src/external directory
set(FETCHCONTENT_BASE_DIR "${CMAKE_CURRENT_LIST_DIR}/src/external")

# Suppress CMP0135 CMake FetchContent warning
if(POLICY CMP0135)
    cmake_policy(SET CMP0135 NEW)
endif()

# Download pagmo dependencies
include(GetBoost)
include(GetTBB)

include(GetPagmo)


#FILE(GLOB SRC_FILES "src/*.cpp" "src/*.c" "src/*.h")
#FILE(GLOB_RECURSE SRC_EXT_FILES "src/external/*.cpp" "src/external/*.c" "src/external/*.h")
#IF(WIN32)
#    FILE(GLOB SRC_WIN_FILES "src/win/*.cpp" "src/win/*.h" "src/win/*.def")
#    SET(SRC_FILES "${SRC_FILES};${SRC_WIN_FILES}")
#ENDIF()

#SCGMS_ADD_LIBRARY(${PROJ} SHARED ${SRC_FILES} ${SRC_EXT_FILES})
#SCGMS_LINK_COMMON_LIBRARIES(${PROJ})
#APPLY_SCGMS_LIBRARY_BUILD_SETTINGS(${PROJ})
#CONFIGURE_FILTER_OUTPUT(${PROJ})
#
#TARGET_INCLUDE_DIRECTORIES(${PROJ}
#        PUBLIC "src/external/"
#        PUBLIC "src/external/api/"
#        PUBLIC "src/external/util/"
#        PUBLIC "src/external/algs/"
#)
