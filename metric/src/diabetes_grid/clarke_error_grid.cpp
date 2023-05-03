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
 * a) Without a specific agreement, you are not authorized to make or keep any copies of this file.
 * b) For any use, especially commercial use, you must contact us and obtain specific terms and conditions 
 *    for the use of the software.
 * c) When publishing any derivative work or results obtained using this software, you agree to cite the following paper:
 *    Tomas Koutny and Martin Ubl, "SmartCGMS as a Testbed for a Blood-Glucose Level Prediction and/or 
 *    Control Challenge with (an FDA-Accepted) Diabetic Patient Simulation", Procedia Computer Science,  
 *    Volume 177, pp. 354-362, 2020
 */

#include "clarke_error_grid.h"

#include "../../../../common/iface/DeviceIface.h"


// The coordinates were adopted from Matlab CLARKE code
// by Edgar Guevara Codina (codina@REMOVETHIScactus.iico.uaslp.mx)
// File Version 1.2
// March 29 2013
// see http://www.mathworks.com/matlabcentral/fileexchange/20545-clarke-error-grid-analysis/content/clarke.m


//plot([0 400], [0 400], 'k:') % Theoretical 45 regression line
//plot([0 175 / 3], [70 70], 'k-')
//% plot([175 / 3 320], [70 400], 'k-')
//plot([175 / 3 400 / 1.2], [70 400], 'k-') % replace 320 with 400 / 1.2 because 100 * (400 - 400 / 1.2) / (400 / 1.2) = 20 % error
//plot([70 70], [84 400], 'k-')
//plot([0 70], [180 180], 'k-')
//plot([70 290], [180 400], 'k-') % Corrected upper B - C boundary
//% plot([70 70], [0 175 / 3], 'k-')
//plot([70 70], [0 56], 'k-') % replace 175.3 with 56 because 100 * abs(56 - 70) / 70) = 20 % error
//% plot([70 400], [175 / 3 320], 'k-')
//plot([70 400], [56 320], 'k-')
//plot([180 180], [0 70], 'k-')
//plot([180 400], [70 70], 'k-')
//plot([240 240], [70 180], 'k-')
//plot([240 400], [180 180], 'k-')
//plot([130 180], [0 70], 'k-') % Lower B - C boundary slope OK


#pragma region Diabetes Coords
const std::vector<TError_Grid_Point> CLARKE_ZONE_A = {	
	{ 70.0*scgms::mgdL_2_mmolL, 0.0 },{ 70.0 * scgms::mgdL_2_mmolL, 56.0 * scgms::mgdL_2_mmolL },{ 400.0 * scgms::mgdL_2_mmolL, 320.0 * scgms::mgdL_2_mmolL },

	{ 400.0 * scgms::mgdL_2_mmolL, 400.0 * scgms::mgdL_2_mmolL },
	{ 400.0 * scgms::mgdL_2_mmolL / (1.2 * scgms::mgdL_2_mmolL), 400.0 * scgms::mgdL_2_mmolL },{70.0 * scgms::mgdL_2_mmolL,84.0 * scgms::mgdL_2_mmolL}, { 175.0 * scgms::mgdL_2_mmolL / (3.0 * scgms::mgdL_2_mmolL), 70.0 * scgms::mgdL_2_mmolL }, { 0.0, 70.0 * scgms::mgdL_2_mmolL },
	{ 0.0, 0.0 },
};

const std::vector<TError_Grid_Point> CLARKE_ZONE_B_Up = {
	{ 70.0 * scgms::mgdL_2_mmolL, 84.0 * scgms::mgdL_2_mmolL },{ 400.0 * scgms::mgdL_2_mmolL / (1.2 * scgms::mgdL_2_mmolL), 400.0 * scgms::mgdL_2_mmolL },
	{290.0 * scgms::mgdL_2_mmolL, 400.0 * scgms::mgdL_2_mmolL}, {70.0 * scgms::mgdL_2_mmolL, 180.0 * scgms::mgdL_2_mmolL},
};

const std::vector<TError_Grid_Point> CLARKE_ZONE_C_Up = {	
	{ 70.0 * scgms::mgdL_2_mmolL, 180.0 * scgms::mgdL_2_mmolL },{ 290.0 * scgms::mgdL_2_mmolL, 400.0 * scgms::mgdL_2_mmolL },
	{ 70.0 * scgms::mgdL_2_mmolL, 400.0 * scgms::mgdL_2_mmolL}
};

const std::vector<TError_Grid_Point>  CLARKE_ZONE_D_Up = {
	{ 0.0, 70.0 * scgms::mgdL_2_mmolL },{ 175 / (3.0 * scgms::mgdL_2_mmolL), 70.0 * scgms::mgdL_2_mmolL },{ 70.0 * scgms::mgdL_2_mmolL,84.0 * scgms::mgdL_2_mmolL },
	{70.0 * scgms::mgdL_2_mmolL, 180.0 * scgms::mgdL_2_mmolL}, {0.0, 180.0 * scgms::mgdL_2_mmolL},
};

const std::vector<TError_Grid_Point> CLARKE_ZONE_E_Up = {
	{ 70.0 * scgms::mgdL_2_mmolL, 180.0 * scgms::mgdL_2_mmolL },
	{ 70.0 * scgms::mgdL_2_mmolL, 400.0 * scgms::mgdL_2_mmolL },{ 0.0, 400.0 * scgms::mgdL_2_mmolL },
	{ 0.0, 180.0 * scgms::mgdL_2_mmolL },
};

const std::vector<TError_Grid_Point> CLARKE_ZONE_B_Low = {
	{130.0 * scgms::mgdL_2_mmolL,0.0}, {180.0 * scgms::mgdL_2_mmolL,70.0 * scgms::mgdL_2_mmolL}, {240.0 * scgms::mgdL_2_mmolL,70.0 * scgms::mgdL_2_mmolL}, {240.0 * scgms::mgdL_2_mmolL,180.0 * scgms::mgdL_2_mmolL}, {400.0 * scgms::mgdL_2_mmolL, 180.0 * scgms::mgdL_2_mmolL},
	{400.0 * scgms::mgdL_2_mmolL,320.0 * scgms::mgdL_2_mmolL}, {70.0 * scgms::mgdL_2_mmolL, 56.0 * scgms::mgdL_2_mmolL}, { 70.0 * scgms::mgdL_2_mmolL, 0.0 },
};

const std::vector<TError_Grid_Point> CLARKE_ZONE_C_Low = {
	{180.0 * scgms::mgdL_2_mmolL, 0.0}, { 180.0 * scgms::mgdL_2_mmolL,70.0 * scgms::mgdL_2_mmolL },{ 130.0 * scgms::mgdL_2_mmolL,0.0 },
};

const std::vector<TError_Grid_Point>  CLARKE_ZONE_D_Low = {
	{ 240.0 * scgms::mgdL_2_mmolL,70.0 * scgms::mgdL_2_mmolL },{ 400.0 * scgms::mgdL_2_mmolL, 70.0 * scgms::mgdL_2_mmolL },{ 400.0 * scgms::mgdL_2_mmolL,180.0 * scgms::mgdL_2_mmolL },{240.0 * scgms::mgdL_2_mmolL,180.0 * scgms::mgdL_2_mmolL},
};

const std::vector<TError_Grid_Point>  CLARKE_ZONE_E_Low = {
	{ 180.0 * scgms::mgdL_2_mmolL, 0.0 },{ 400.0 * scgms::mgdL_2_mmolL, 0.0 },{ 400.0 * scgms::mgdL_2_mmolL,70.0 * scgms::mgdL_2_mmolL },{ 180.0 * scgms::mgdL_2_mmolL, 70.0 * scgms::mgdL_2_mmolL },
};


#pragma endregion

const TError_Grid Clarke_Error_Grid = {

	{ NError_Grid_Zone::A, CLARKE_ZONE_A},
	{ NError_Grid_Zone::B, CLARKE_ZONE_B_Up},
	{ NError_Grid_Zone::B, CLARKE_ZONE_B_Low},
	{ NError_Grid_Zone::C, CLARKE_ZONE_C_Up},
	{ NError_Grid_Zone::C, CLARKE_ZONE_C_Low},
	{ NError_Grid_Zone::D, CLARKE_ZONE_D_Up},
	{ NError_Grid_Zone::D, CLARKE_ZONE_D_Low},
	{ NError_Grid_Zone::E, CLARKE_ZONE_E_Up},
	{ NError_Grid_Zone::E, CLARKE_ZONE_E_Low },
};