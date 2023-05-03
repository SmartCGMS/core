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

IF(COMPONENT_MATLAB_CONNECTOR)

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

		SCGMS_ADD_LIBRARY(${PROJ} SHARED ${SRC_FILES};${COMMON_FILES})
		CONFIGURE_FILTER_OUTPUT(${PROJ})

		TARGET_INCLUDE_DIRECTORIES(${PROJ} PUBLIC ${Matlab_INCLUDE_DIRS})
		TARGET_LINK_LIBRARIES(${PROJ} scgms ${Matlab_LIBRARIES})

		# Matlab R2018a+ contains some deprecated functions and approaches
		TARGET_COMPILE_OPTIONS(${PROJ} PUBLIC "-D_SILENCE_ALL_CXX17_DEPRECATION_WARNINGS")
	ELSE()
		MESSAGE(WARNING "Matlab libraries was not found, matlab solver will not be built.")
	ENDIF()

ENDIF()