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

SET(PROJ "scgms")
CONFIGURE_COMMON(${PROJ})

FILE(GLOB FACTORY_SRC "src/*.cpp" "src/*.c" "src/*.h")
IF(WIN32)
	FILE(GLOB FACTORY_WIN_SRC "src/win/*.cpp" "src/win/*.def")
	SET(FACTORY_SRC "${FACTORY_SRC};${FACTORY_WIN_SRC}")
ENDIF()

SET(FACTORY_RTL_SRC )

ADD_RTL_FILE(FACTORY_RTL_SRC "Dynamic_Library.h")
ADD_RTL_FILE(FACTORY_RTL_SRC "Dynamic_Library.cpp")
ADD_RTL_FILE(FACTORY_RTL_SRC "FilesystemLib.h")
ADD_RTL_FILE(FACTORY_RTL_SRC "FilesystemLib.cpp")
ADD_RTL_FILE(FACTORY_RTL_SRC "hresult.h")
ADD_RTL_FILE(FACTORY_RTL_SRC "hresult.c")
ADD_RTL_FILE(FACTORY_RTL_SRC "rattime.h")
ADD_RTL_FILE(FACTORY_RTL_SRC "rattime.cpp")
ADD_RTL_FILE(FACTORY_RTL_SRC "referencedImpl.h")
ADD_RTL_FILE(FACTORY_RTL_SRC "referencedImpl.cpp")
ADD_RTL_FILE(FACTORY_RTL_SRC "DeviceLib.h")
ADD_RTL_FILE(FACTORY_RTL_SRC "DeviceLib.cpp")
ADD_RTL_FILE(FACTORY_RTL_SRC "UILib.h")
ADD_RTL_FILE(FACTORY_RTL_SRC "UILib.cpp")
ADD_RTL_FILE(FACTORY_RTL_SRC "FilterLib.h")
ADD_RTL_FILE(FACTORY_RTL_SRC "FilterLib.cpp")
ADD_RTL_FILE(FACTORY_RTL_SRC "scgmsLib.h")
ADD_RTL_FILE(FACTORY_RTL_SRC "scgmsLib.cpp")
ADD_RTL_FILE(FACTORY_RTL_SRC "SolverLib.h")
ADD_RTL_FILE(FACTORY_RTL_SRC "SolverLib.cpp")

SET(FACTORY_UTILS_SRC )
ADD_UTILS_FILE(FACTORY_UTILS_SRC "winapi_mapping.h")
ADD_UTILS_FILE(FACTORY_UTILS_SRC "winapi_mapping.c")
ADD_UTILS_FILE(FACTORY_UTILS_SRC "string_utils.h")
ADD_UTILS_FILE(FACTORY_UTILS_SRC "string_utils.cpp")
ADD_UTILS_FILE(FACTORY_UTILS_SRC "ConvertUTF.h")
ADD_UTILS_FILE(FACTORY_UTILS_SRC "ConvertUTF.cpp")
ADD_UTILS_FILE(FACTORY_UTILS_SRC "DebugHelper.h")
ADD_UTILS_FILE(FACTORY_UTILS_SRC "DebugHelper.cpp")

SCGMS_ADD_LIBRARY(${PROJ} SHARED ${FACTORY_SRC};${COMMON_IFACE_FILES};${COMMON_LANG_FILES};${COMMON_RESOURCE_FILES};${FACTORY_RTL_SRC};${FACTORY_UTILS_SRC})
