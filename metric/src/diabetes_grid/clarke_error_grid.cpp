/**
 * SmartCGMS - continuous glucose monitoring and controlling framework
 * https://diabetes.zcu.cz/
 *
 * Copyright (c) since 2018 University of West Bohemia.
 *
 * Contact:
 * diabetes@mail.kiv.zcu.cz
 * Medical Informatics, Department of Computer Science and Engineering
 * Faculty of Applied Sciences, University of West Bohemia
 * Univerzitni 8, 301 00 Pilsen
 * Czech Republic
 * 
 * 
 * Purpose of this software:
 * This software is intended to demonstrate work of the diabetes.zcu.cz research
 * group to other scientists, to complement our published papers. It is strictly
 * prohibited to use this software for diagnosis or treatment of any medical condition,
 * without obtaining all required approvals from respective regulatory bodies.
 *
 * Especially, a diabetic patient is warned that unauthorized use of this software
 * may result into severe injure, including death.
 *
 *
 * Licensing terms:
 * Unless required by applicable law or agreed to in writing, software
 * distributed under these license terms is distributed on an "AS IS" BASIS, WITHOUT
 * WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *
 * a) This file is available under the Apache License, Version 2.0.
 * b) When publishing any derivative work or results obtained using this software, you agree to cite the following paper:
 *    Tomas Koutny and Martin Ubl, "SmartCGMS as a Testbed for a Blood-Glucose Level Prediction and/or 
 *    Control Challenge with (an FDA-Accepted) Diabetic Patient Simulation", Procedia Computer Science,  
 *    Volume 177, pp. 354-362, 2020
 */

#include "clarke_error_grid.h"

#include <scgms/iface/DeviceIface.h>

// The coordinates were adopted from Matlab CLARKE code
// by Edgar Guevara Codina (codina@REMOVETHIScactus.iico.uaslp.mx)
// File Version 1.2
// March 29 2013
// see http://www.mathworks.com/matlabcentral/fileexchange/20545-clarke-error-grid-analysis/content/clarke.m

// Clarke error grid coordinates
#pragma region ClarkeCoords

const std::vector<TError_Grid_Point> CLARKE_ZONE_A = {
	{ 70.0 * scgms::mgdL_2_mmolL,                                0.0 },
	{ 70.0 * scgms::mgdL_2_mmolL,                                56.0 * scgms::mgdL_2_mmolL },
	{ 400.0 * scgms::mgdL_2_mmolL,                               320.0 * scgms::mgdL_2_mmolL },
	{ 400.0 * scgms::mgdL_2_mmolL,                               400.0 * scgms::mgdL_2_mmolL },
	{ 400.0 * scgms::mgdL_2_mmolL / (1.2 * scgms::mgdL_2_mmolL), 400.0 * scgms::mgdL_2_mmolL },
	{ 70.0 * scgms::mgdL_2_mmolL,                                84.0 * scgms::mgdL_2_mmolL},
	{ 175.0 * scgms::mgdL_2_mmolL / (3.0 * scgms::mgdL_2_mmolL), 70.0 * scgms::mgdL_2_mmolL },
	{ 0.0,                                                       70.0 * scgms::mgdL_2_mmolL },
	{ 0.0,                                                       0.0 },
};

const std::vector<TError_Grid_Point> CLARKE_ZONE_B_Up = {
	{ 70.0 * scgms::mgdL_2_mmolL,                                84.0 * scgms::mgdL_2_mmolL },
	{ 400.0 * scgms::mgdL_2_mmolL / (1.2 * scgms::mgdL_2_mmolL), 400.0 * scgms::mgdL_2_mmolL },
	{ 290.0 * scgms::mgdL_2_mmolL,                               400.0 * scgms::mgdL_2_mmolL },
	{ 70.0 * scgms::mgdL_2_mmolL,                                180.0 * scgms::mgdL_2_mmolL },
};

const std::vector<TError_Grid_Point> CLARKE_ZONE_C_Up = {
	{ 70.0 * scgms::mgdL_2_mmolL,  180.0 * scgms::mgdL_2_mmolL },
	{ 290.0 * scgms::mgdL_2_mmolL, 400.0 * scgms::mgdL_2_mmolL },
	{ 70.0 * scgms::mgdL_2_mmolL,  400.0 * scgms::mgdL_2_mmolL }
};

const std::vector<TError_Grid_Point>  CLARKE_ZONE_D_Up = {
	{ 0.0,                               70.0 * scgms::mgdL_2_mmolL },
	{ 175 / (3.0 * scgms::mgdL_2_mmolL), 70.0 * scgms::mgdL_2_mmolL },
	{ 70.0 * scgms::mgdL_2_mmolL,        84.0 * scgms::mgdL_2_mmolL },
	{ 70.0 * scgms::mgdL_2_mmolL,        180.0 * scgms::mgdL_2_mmolL },
	{ 0.0,                               180.0 * scgms::mgdL_2_mmolL },
};

const std::vector<TError_Grid_Point> CLARKE_ZONE_E_Up = {
	{ 70.0 * scgms::mgdL_2_mmolL, 180.0 * scgms::mgdL_2_mmolL },
	{ 70.0 * scgms::mgdL_2_mmolL, 400.0 * scgms::mgdL_2_mmolL },
	{ 0.0,                        400.0 * scgms::mgdL_2_mmolL },
	{ 0.0,                        180.0 * scgms::mgdL_2_mmolL },
};

const std::vector<TError_Grid_Point> CLARKE_ZONE_B_Low = {
	{ 130.0 * scgms::mgdL_2_mmolL, 0.0 },
	{ 180.0 * scgms::mgdL_2_mmolL, 70.0 * scgms::mgdL_2_mmolL },
	{ 240.0 * scgms::mgdL_2_mmolL, 70.0 * scgms::mgdL_2_mmolL },
	{ 240.0 * scgms::mgdL_2_mmolL, 180.0 * scgms::mgdL_2_mmolL },
	{ 400.0 * scgms::mgdL_2_mmolL, 180.0 * scgms::mgdL_2_mmolL },
	{ 400.0 * scgms::mgdL_2_mmolL, 320.0 * scgms::mgdL_2_mmolL },
	{ 70.0 * scgms::mgdL_2_mmolL,  56.0 * scgms::mgdL_2_mmolL },
	{ 70.0 * scgms::mgdL_2_mmolL,  0.0 },
};

const std::vector<TError_Grid_Point> CLARKE_ZONE_C_Low = {
	{ 180.0 * scgms::mgdL_2_mmolL, 0.0 },
	{ 180.0 * scgms::mgdL_2_mmolL, 70.0 * scgms::mgdL_2_mmolL },
	{ 130.0 * scgms::mgdL_2_mmolL, 0.0 },
};

const std::vector<TError_Grid_Point>  CLARKE_ZONE_D_Low = {
	{ 240.0 * scgms::mgdL_2_mmolL, 70.0 * scgms::mgdL_2_mmolL },
	{ 400.0 * scgms::mgdL_2_mmolL, 70.0 * scgms::mgdL_2_mmolL },
	{ 400.0 * scgms::mgdL_2_mmolL, 180.0 * scgms::mgdL_2_mmolL },
	{ 240.0 * scgms::mgdL_2_mmolL, 180.0 * scgms::mgdL_2_mmolL },
};

const std::vector<TError_Grid_Point>  CLARKE_ZONE_E_Low = {
	{ 180.0 * scgms::mgdL_2_mmolL, 0.0 },
	{ 400.0 * scgms::mgdL_2_mmolL, 0.0 },
	{ 400.0 * scgms::mgdL_2_mmolL, 70.0 * scgms::mgdL_2_mmolL },
	{ 180.0 * scgms::mgdL_2_mmolL, 70.0 * scgms::mgdL_2_mmolL },
};

#pragma endregion

const TError_Grid Clarke_Error_Grid = {
	{ NError_Grid_Zone::A, CLARKE_ZONE_A },
	{ NError_Grid_Zone::B, CLARKE_ZONE_B_Up },
	{ NError_Grid_Zone::B, CLARKE_ZONE_B_Low },
	{ NError_Grid_Zone::C, CLARKE_ZONE_C_Up },
	{ NError_Grid_Zone::C, CLARKE_ZONE_C_Low },
	{ NError_Grid_Zone::D, CLARKE_ZONE_D_Up },
	{ NError_Grid_Zone::D, CLARKE_ZONE_D_Low },
	{ NError_Grid_Zone::E, CLARKE_ZONE_E_Up },
	{ NError_Grid_Zone::E, CLARKE_ZONE_E_Low },
};
