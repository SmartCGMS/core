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

# Add "scgms" project - this is a mandatory core module

ADD_SUBDIRECTORY("scgms")

# Add the rest of core modules

SUBDIRLIST(CORE_LIB_DIRS "${CMAKE_CURRENT_SOURCE_DIR}")
FOREACH(subdir ${CORE_LIB_DIRS})
	# ignore git index directory and scgms directory (scgms is already included)
	IF(NOT "${subdir}" STREQUAL ".git" AND NOT "${subdir}" STREQUAL "scgms")
		IF(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/${subdir}/CMakeLists.txt")
			MESSAGE(STATUS "Configuring core module: ${subdir}")
			ADD_SUBDIRECTORY(${subdir})
		ELSE()
			MESSAGE(WARNING "Core module '${subdir}' does not contain CMakeLists.txt file")
		ENDIF()
	ENDIF()
ENDFOREACH()
