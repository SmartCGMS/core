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
# a) This file is available under the Apache License, Version 2.0.
# b) When publishing any derivative work or results obtained using this software, you agree to cite the following paper:
#    Tomas Koutny and Martin Ubl, "SmartCGMS as a Testbed for a Blood-Glucose Level Prediction and/or 
#    Control Challenge with (an FDA-Accepted) Diabetic Patient Simulation", Procedia Computer Science,  
#    Volume 177, pp. 354-362, 2020

CMAKE_MINIMUM_REQUIRED(VERSION 3.0)

SET(PROJ "external")

CONFIGURE_COMMON(${PROJ})

FILE(GLOB SRC_FILES "src/*.cpp" "src/*.c" "src/*.h")
SOURCE_GROUP("src" FILES ${SRC_FILES})

FILE(GLOB_RECURSE NETWORK_SRC_FILES "src/network/*.cpp" "src/network/*.c" "src/network/*.h")

# currently supporting only Win32 target
IF(WIN32)
	FILE(GLOB_RECURSE DMMS_LIB_FILES "src/dmms_lib/*.cpp" "src/dmms_lib/*.c" "src/dmms_lib/*.h")
	FILE(GLOB_RECURSE DMMS_PROC_FILES "src/dmms_proc/*.cpp" "src/dmms_proc/*.c" "src/dmms_proc/*.h")
	
	SOURCE_GROUP("src\\dmms_lib" FILES ${DMMS_LIB_FILES})
	SOURCE_GROUP("src\\dmms_proc" FILES ${DMMS_PROC_FILES})
	
	IF(WIN32)
		FILE(GLOB SRC_WIN_FILES "src/win/*.cpp" "src/win/*.h" "src/win/*.def")
		SET(SRC_FILES "${SRC_FILES};${SRC_WIN_FILES}")
	ENDIF()

	SET(SRC_FILES "${SRC_FILES};${DMMS_LIB_FILES};${DMMS_PROC_FILES};${MATLAB_SRC_FILES}")
	
	FIND_PACKAGE(DmmsLib)
ENDIF()

SOURCE_GROUP("src\\network" FILES ${NETWORK_SRC_FILES})

SCGMS_ADD_LIBRARY(${PROJ} SHARED ${SRC_FILES};${NETWORK_SRC_FILES})
TARGET_LINK_LIBRARIES(${PROJ} scgms-common)
CONFIGURE_FILTER_OUTPUT(${PROJ})

IF(WIN32)
	TARGET_INCLUDE_DIRECTORIES(${PROJ} PUBLIC ${DMMSLIB_INCLUDE_DIRS})
ENDIF()
	
IF(WIN32)
	TARGET_LINK_LIBRARIES(${PROJ} Ws2_32)
ELSE()
	TARGET_COMPILE_DEFINITIONS(${PROJ} PUBLIC NO_BUILD_DMMS)
ENDIF()
