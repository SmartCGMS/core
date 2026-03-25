CMAKE_MINIMUM_REQUIRED(VERSION 3.10)
SET(PROJ "external4scgms")

set(DISTRIBUTED_SOLVER_RUNTIME_OUTPUT_DIR "${CMAKE_BINARY_DIR}/compiled/")

add_subdirectory(src/external/Pagmo-solver-distributed)

file(GLOB SRC_FILES "src/*.cpp" "src/*.c" "src/*.h")

IF(WIN32)
    FILE(GLOB SRC_WIN_FILES "src/win/*.cpp" "src/win/*.h" "src/win/*.def")
    SET(SRC_FILES "${SRC_FILES};${SRC_WIN_FILES}")
ENDIF()

SCGMS_ADD_LIBRARY(${PROJ} SHARED ${SRC_FILES})

target_link_libraries(${PROJ} distributed)

SCGMS_LINK_COMMON_LIBRARIES(${PROJ})
APPLY_SCGMS_LIBRARY_BUILD_SETTINGS(${PROJ})
CONFIGURE_FILTER_OUTPUT(${PROJ})