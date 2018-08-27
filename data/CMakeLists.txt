CMAKE_MINIMUM_REQUIRED(VERSION 3.0)

SET(PROJ "data")

SET(XLNT_INCLUDE "" CACHE FILEPATH "XLNT header files location")
SET(XLNT_LIBRARY "" CACHE FILEPATH "XLNT library files location")
SET(EXCELFORMAT_INCLUDE "" CACHE FILEPATH "ExcelFormat header files location")
SET(EXCELFORMAT_LIBRARY "" CACHE FILEPATH "ExcelFormat library files location")

CONFIGURE_COMMON(${PROJ})

FILE(GLOB SRC_FILES "src/*.cpp" "src/*.c" "src/*.h" "src/third party/*.h" "src/third party/*.cpp" "src/third party/*.c" "src/fileloader/*.cpp" "src/fileloader/*.h" "src/fileloader/*.c")
IF(WIN32)
	FILE(GLOB SRC_WIN_FILES "src/win/*.cpp" "src/win/*.h" "src/win/*.def")
	SET(SRC_FILES "${SRC_FILES};${SRC_WIN_FILES}")
ENDIF()

ADD_LIBRARY(${PROJ} SHARED ${SRC_FILES};${COMMON_FILES})
CONFIGURE_FILTER_OUTPUT(${PROJ})

IF(WIN32)
	GEN_LIBCONF_D(xlnt "${XLNT_LIBRARY}" TRUE)
ELSE()
	GEN_LIBCONF_D(xlnt "${XLNT_LIBRARY}" FALSE)
ENDIF()
GEN_LIBCONF_D(ExcelFormat "${EXCELFORMAT_LIBRARY}" TRUE)

TARGET_INCLUDE_DIRECTORIES(${PROJ} PUBLIC ${XLNT_INCLUDE} ${EXCELFORMAT_INCLUDE})
TARGET_LINK_LIBRARIES(${PROJ} Qt5::Core Qt5::Sql xlnt ExcelFormat factory)
