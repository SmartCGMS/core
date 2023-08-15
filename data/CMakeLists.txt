# SmartCGMS - continuous glucose monitoring and controlling framework
# https://diabetes.zcu.cz/
#
# Copyright (c) since 2018 University of West Bohemia.
#
# Contact:
# diabetes@mail.kiv.zcu.cz
# Medical Informatics, Department of Computer Science and Engineering
# Faculty of Applied Sciences, University of West Bohemia
# Univerzitni 8, 301 00 Pilsen
# Czech Republic
# 
# 
# Purpose of this software:
# This software is intended to demonstrate work of the diabetes.zcu.cz research
# group to other scientists, to complement our published papers. It is strictly
# prohibited to use this software for diagnosis or treatment of any medical condition,
# without obtaining all required approvals from respective regulatory bodies.
#
# Especially, a diabetic patient is warned that unauthorized use of this software
# may result into severe injure, including death.
#
#
# Licensing terms:
# Unless required by applicable law or agreed to in writing, software
# distributed under these license terms is distributed on an "AS IS" BASIS, WITHOUT
# WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
#
# a) Without a specific agreement, you are not authorized to make or keep any copies of this file.
# b) For any use, especially commercial use, you must contact us and obtain specific terms and conditions 
#    for the use of the software.
# c) When publishing any derivative work or results obtained using this software, you agree to cite the following paper:
#    Tomas Koutny and Martin Ubl, "SmartCGMS as a Testbed for a Blood-Glucose Level Prediction and/or 
#    Control Challenge with (an FDA-Accepted) Diabetic Patient Simulation", Procedia Computer Science,  
#    Volume 177, pp. 354-362, 2020

CMAKE_MINIMUM_REQUIRED(VERSION 3.0)

SET(PROJ "data")

# component: .xls and .xlsx file parsers and loaders; NOTE: requires xlnt and ExcelFormat libraries
SET(COMPONENT_EXCEL_LOADERS TRUE CACHE BOOL "Build .xls and .xlsx file loading support")

IF(COMPONENT_EXCEL_LOADERS)
	SET(XLNT_INCLUDE "" CACHE FILEPATH "XLNT header files location")
	SET(XLNT_LIBRARY "" CACHE FILEPATH "XLNT library files location")
	SET(EXCELFORMAT_INCLUDE "" CACHE FILEPATH "ExcelFormat header files location")
	SET(EXCELFORMAT_LIBRARY "" CACHE FILEPATH "ExcelFormat library files location")
ENDIF()

CONFIGURE_COMMON(${PROJ})

FILE(GLOB SRC_FILES_TOP "src/*.cpp" "src/*.c" "src/*.h")
FILE(GLOB SRC_FILES_THIRDPARTY "src/third party/*.h" "src/third party/*.cpp" "src/third party/*.c")
FILE(GLOB SRC_FILES_FILELOADER "src/fileloader/*.cpp" "src/fileloader/*.h" "src/fileloader/*.c" "misc/formats/*.c")
SET(SRC_FILES ${SRC_FILES_TOP} ${SRC_FILES_THIRDPARTY} ${SRC_FILES_FILELOADER})

IF(WIN32)
	FILE(GLOB SRC_WIN_FILES "src/win/*.cpp" "src/win/*.h" "src/win/*.def")
	SET(SRC_FILES "${SRC_FILES};${SRC_WIN_FILES}")
ENDIF()

SOURCE_GROUP(src FILES ${SRC_FILES_TOP})
SOURCE_GROUP(thirdparty FILES ${SRC_FILES_THIRDPARTY})
SOURCE_GROUP(fileloader FILES ${SRC_FILES_FILELOADER})

SCGMS_ADD_LIBRARY(${PROJ} SHARED ${SRC_FILES})
TARGET_LINK_LIBRARIES(${PROJ} scgms-common)
CONFIGURE_FILTER_OUTPUT(${PROJ})

SET(REQUIRED_LIBS )

IF(COMPONENT_EXCEL_LOADERS)
	IF(WIN32)
		GEN_LIBCONF_D(xlnt "${XLNT_LIBRARY}" TRUE)
	ELSE()
		GEN_LIBCONF_D(xlnt "${XLNT_LIBRARY}" FALSE)
	ENDIF()
	GEN_LIBCONF_D(ExcelFormat "${EXCELFORMAT_LIBRARY}" TRUE)

	TARGET_INCLUDE_DIRECTORIES(${PROJ} PUBLIC ${XLNT_INCLUDE} ${EXCELFORMAT_INCLUDE})
	LIST(APPEND REQUIRED_LIBS xlnt ExcelFormat)
ELSE()
	TARGET_COMPILE_DEFINITIONS(${PROJ} PUBLIC -DNO_BUILD_EXCELSUPPORT)
ENDIF()

TARGET_LINK_LIBRARIES(${PROJ} ${REQUIRED_LIBS})
