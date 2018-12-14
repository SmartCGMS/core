CMAKE_MINIMUM_REQUIRED(VERSION 3.0)

SET(PROJ "matlab")

CONFIGURE_COMMON(${PROJ})

FILE(GLOB SRC_FILES "src/*.cpp" "src/*.c" "src/*.h")
IF(WIN32)
	FILE(GLOB SRC_WIN_FILES "src/win/*.cpp" "src/win/*.h" "src/win/*.def")
	SET(SRC_FILES "${SRC_FILES};${SRC_WIN_FILES}")
ENDIF()

FIND_PACKAGE(Matlab COMPONENTS MX_LIBRARY MAT_LIBRARY ENG_LIBRARY CPPENG_LIBRARY CPPDA_LIBRARY)

IF (Matlab_FOUND)
	SET(CMAKE_INSTALL_RPATH "${CMAKE_INSTALL_RPATH};${Matlab_CPPLIBRARIES_RPATH}")

	ADD_LIBRARY(${PROJ} SHARED ${SRC_FILES};${COMMON_FILES})
	CONFIGURE_FILTER_OUTPUT(${PROJ})

	TARGET_INCLUDE_DIRECTORIES(${PROJ} PUBLIC ${Matlab_INCLUDE_DIRS})
	TARGET_LINK_LIBRARIES(${PROJ} factory ${Matlab_LIBRARIES})
ELSE()
	MESSAGE(WARNING "Matlab libraries was not found, matlab solver will not be built.")
ENDIF()