CMAKE_MINIMUM_REQUIRED(VERSION 3.10)
SET(PROJ "pagmo4scgms")

message(AUTHOR_WARNING "Make sure that you've installed Boost and Intel TBB before using the Pagmo library.")

message(AUTHOR_WARNING "Both Boost and Intel TBB must be in your env PATH variable (e.g. C:\\Program Files (x86)\\Intel\\oneAPI\\tbb\\2022.2\\bin) or else pagmo.dll won't load.'")

set(PAGMO_WITH_EIGEN3 ON)
add_subdirectory(src/external/pagmo)

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
# The pagmo.dll file need to be inside /compiled/ dir TODO: remove this and use APPLY_SCGMS_LIBRARY_BUILD_SETTINGS instead?
set_target_properties(pagmo
    PROPERTIES
    LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/compiled/"
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/compiled/"
)

SCGMS_LINK_COMMON_LIBRARIES(${PROJ})
APPLY_SCGMS_LIBRARY_BUILD_SETTINGS(${PROJ})
CONFIGURE_FILTER_OUTPUT(${PROJ})