CMAKE_MINIMUM_REQUIRED(VERSION 3.10)
SET(PROJ "pagmo4scgms")

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/cmake/")
# Download all external dependencies into the /src/external directory
set(FETCHCONTENT_BASE_DIR "${CMAKE_CURRENT_LIST_DIR}/src/external")

message(AUTHOR_WARNING "Make sure that you've installed Boost and Intel TBB before using the Pagmo library.")
include(GetPagmo)

# This needs to be added or Pagmo won't compile with newest MSVC (14.44)
if(MSVC)
    target_compile_definitions(pagmo PRIVATE _SILENCE_ALL_MS_EXT_DEPRECATION_WARNINGS)
    target_compile_options(pagmo PRIVATE /W2 /EHsc)
endif()

file(GLOB SRC_FILES "src/*.cpp" "src/*.c" "src/*.h")

IF(WIN32)
    FILE(GLOB SRC_WIN_FILES "src/win/*.cpp" "src/win/*.h" "src/win/*.def")
    SET(SRC_FILES "${SRC_FILES};${SRC_WIN_FILES}")
ENDIF()

SCGMS_ADD_LIBRARY(${PROJ} SHARED ${SRC_FILES})
# Add Pagmo as a dependency of the "pagmo4scgms" (variable ${PROJ}) target created by the command above
target_link_libraries(${PROJ} pagmo)

SCGMS_LINK_COMMON_LIBRARIES(${PROJ})
APPLY_SCGMS_LIBRARY_BUILD_SETTINGS(${PROJ})
CONFIGURE_FILTER_OUTPUT(${PROJ})