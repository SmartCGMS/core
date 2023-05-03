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

SET(PROJ "drawing")

CONFIGURE_COMMON(${PROJ})

FILE(GLOB SRC_FILES "src/*.cpp" "src/*.c" "src/*.h")
IF(WIN32)
    FILE(GLOB SRC_WIN_FILES "src/win/*.cpp" "src/win/*.h" "src/win/*.def")
    SET(SRC_FILES "${SRC_FILES};${SRC_WIN_FILES}")
ENDIF()

SET(DRAWING_UTILS_SRC )
ADD_UTILS_FILE(DRAWING_UTILS_SRC "drawing/IRenderer.h")
ADD_UTILS_FILE(DRAWING_UTILS_SRC "drawing/Drawing.cpp")
ADD_UTILS_FILE(DRAWING_UTILS_SRC "drawing/Drawing.h")
ADD_UTILS_FILE(DRAWING_UTILS_SRC "drawing/SVGRenderer.cpp")
ADD_UTILS_FILE(DRAWING_UTILS_SRC "drawing/SVGRenderer.h")

FILE(GLOB_RECURSE SRC_DRAWING_FILES "src/drawing/*.cpp" "src/drawing/*.c" "src/drawing/*.h")

SCGMS_ADD_LIBRARY(${PROJ} SHARED ${SRC_FILES};${COMMON_FILES};${SRC_DRAWING_FILES};${DRAWING_UTILS_SRC})
CONFIGURE_FILTER_OUTPUT(${PROJ})

TARGET_LINK_LIBRARIES(${PROJ} scgms)