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

#include "parkes_error_grid.h"

#include "../../../../common/iface/DeviceIface.h"


//See also:
//Pfützner, A., Klonoff, D.C., Pardo, S., &Parkes, J.L. (2013).
//Technical Aspects of the Parkes Error Grid.Journal of Diabetes Science and Technology, 7(5), 1275–1281.
//http://www.ncbi.nlm.nih.gov/pmc/articles/PMC3876371/


//Diabetes Type 1 coordinates
#pragma region DiabetesType1
const std::vector<TError_Grid_Point> PARKES_ZONE_A_T1 = {	
	{50.0*scgms::mgdL_2_mmolL, 0.0}, {50.0 * scgms::mgdL_2_mmolL, 30.0 * scgms::mgdL_2_mmolL}, {170.0 * scgms::mgdL_2_mmolL, 145.0 * scgms::mgdL_2_mmolL}, {385.0 * scgms::mgdL_2_mmolL, 300.0 * scgms::mgdL_2_mmolL}, {550.0 * scgms::mgdL_2_mmolL,450.0 * scgms::mgdL_2_mmolL},
	{550.0 * scgms::mgdL_2_mmolL, 550.0 * scgms::mgdL_2_mmolL},
	{430.0 * scgms::mgdL_2_mmolL, 550.0 * scgms::mgdL_2_mmolL}, {280.0 * scgms::mgdL_2_mmolL,380.0 * scgms::mgdL_2_mmolL}, {140.0 * scgms::mgdL_2_mmolL, 170.0 * scgms::mgdL_2_mmolL}, {30.0 * scgms::mgdL_2_mmolL, 50.0 * scgms::mgdL_2_mmolL}, {0.0 , 50.0 * scgms::mgdL_2_mmolL},
	{0.0 , 0.0},
};

const std::vector<TError_Grid_Point> PARKES_ZONE_B_Up_T1 = {
	{0.0 , 50.0 * scgms::mgdL_2_mmolL }, {30.0 * scgms::mgdL_2_mmolL, 50.0 * scgms::mgdL_2_mmolL},{ 140.0 * scgms::mgdL_2_mmolL, 170.0 * scgms::mgdL_2_mmolL },{ 280.0 *scgms::mgdL_2_mmolL,380.0 *scgms::mgdL_2_mmolL},{ 430.0 *scgms::mgdL_2_mmolL, 550.0 *scgms::mgdL_2_mmolL},
	{260.0 *scgms::mgdL_2_mmolL,550.0 * scgms::mgdL_2_mmolL},{70.0 *scgms::mgdL_2_mmolL,110.0 * scgms::mgdL_2_mmolL},{50.0 *scgms::mgdL_2_mmolL,80.0 * scgms::mgdL_2_mmolL},{30.0 *scgms::mgdL_2_mmolL,60.0 * scgms::mgdL_2_mmolL},{0.0 *scgms::mgdL_2_mmolL,60.0 * scgms::mgdL_2_mmolL}
};

const std::vector<TError_Grid_Point> PARKES_ZONE_B_Low_T1 = {
	{ 120.0 *scgms::mgdL_2_mmolL, 0.0 },{ 120.0 *scgms::mgdL_2_mmolL, 30.0 *scgms::mgdL_2_mmolL},{ 260.0 *scgms::mgdL_2_mmolL, 130.0 *scgms::mgdL_2_mmolL},{ 550.0 *scgms::mgdL_2_mmolL, 250.0 *scgms::mgdL_2_mmolL},
	{ 550.0 *scgms::mgdL_2_mmolL,450.0 *scgms::mgdL_2_mmolL},{ 385.0 * scgms::mgdL_2_mmolL, 300.0 *scgms::mgdL_2_mmolL},{ 170.0 *scgms::mgdL_2_mmolL, 145.0 * scgms::mgdL_2_mmolL },{ 50.0 *scgms::mgdL_2_mmolL, 30.0 *scgms::mgdL_2_mmolL},{ 50.0 *scgms::mgdL_2_mmolL, 0.0 },
};

const std::vector<TError_Grid_Point> PARKES_ZONE_C_Up_T1 = {
	{0.0 ,60.0 *scgms::mgdL_2_mmolL},{ 30.0 *scgms::mgdL_2_mmolL,60.0 *scgms::mgdL_2_mmolL},{ 50.0 *scgms::mgdL_2_mmolL,80.0 *scgms::mgdL_2_mmolL},{ 70.0 *scgms::mgdL_2_mmolL,110.0 *scgms::mgdL_2_mmolL}, { 260.0 *scgms::mgdL_2_mmolL,550.0 *scgms::mgdL_2_mmolL},
	{ 125.0 * scgms::mgdL_2_mmolL, 550.0 *scgms::mgdL_2_mmolL},{ 80.0 *scgms::mgdL_2_mmolL, 215.0 * scgms::mgdL_2_mmolL },{ 50.0 *scgms::mgdL_2_mmolL, 125.0 * scgms::mgdL_2_mmolL },{ 25.0 * scgms::mgdL_2_mmolL, 100.0 *scgms::mgdL_2_mmolL},{0.0 , 100.0 *scgms::mgdL_2_mmolL},
};

const std::vector<TError_Grid_Point> PARKES_ZONE_C_Low_T1 = {
	{ 250.0 *scgms::mgdL_2_mmolL, 0.0 },{ 250.0 *scgms::mgdL_2_mmolL, 40.0 *scgms::mgdL_2_mmolL},{ 550.0 *scgms::mgdL_2_mmolL, 150.0 *scgms::mgdL_2_mmolL},
	{ 550.0 *scgms::mgdL_2_mmolL, 250.0 *scgms::mgdL_2_mmolL},{ 260.0 *scgms::mgdL_2_mmolL, 130.0 *scgms::mgdL_2_mmolL},{ 120.0 *scgms::mgdL_2_mmolL, 30.0 *scgms::mgdL_2_mmolL},{ 120.0 *scgms::mgdL_2_mmolL, 0.0 },
	
};

const std::vector<TError_Grid_Point> PARKES_ZONE_D_Up_T1 = {
	{0.0 , 100.0 *scgms::mgdL_2_mmolL},{ 25.0 * scgms::mgdL_2_mmolL, 100.0 *scgms::mgdL_2_mmolL},{ 50.0 *scgms::mgdL_2_mmolL, 125.0 * scgms::mgdL_2_mmolL },{ 80.0 *scgms::mgdL_2_mmolL, 215.0 * scgms::mgdL_2_mmolL },{ 125.0 * scgms::mgdL_2_mmolL, 550.0 *scgms::mgdL_2_mmolL},
	{ 50.0 *scgms::mgdL_2_mmolL,550.0 *scgms::mgdL_2_mmolL},{ 35.0 * scgms::mgdL_2_mmolL,155.0 * scgms::mgdL_2_mmolL },{0.0 ,150.0 *scgms::mgdL_2_mmolL},
	
};

const std::vector<TError_Grid_Point> PARKES_ZONE_D_Low_T1 = {
	{550.0 *scgms::mgdL_2_mmolL, 0.0},
	{ 550.0 *scgms::mgdL_2_mmolL, 150.0 *scgms::mgdL_2_mmolL},{ 250.0 *scgms::mgdL_2_mmolL, 40.0 *scgms::mgdL_2_mmolL},	{ 250.0 *scgms::mgdL_2_mmolL, 0.0 },
};

const std::vector<TError_Grid_Point> PARKES_ZONE_E_Up_T1 = {	
	{0.0 ,150.0 *scgms::mgdL_2_mmolL},{ 35.0 * scgms::mgdL_2_mmolL,155.0 * scgms::mgdL_2_mmolL },{ 50.0 *scgms::mgdL_2_mmolL,550.0 *scgms::mgdL_2_mmolL},
	{0.0 , 550.0 * scgms::mgdL_2_mmolL},
};
#pragma endregion

//Diabetes Type 2 coordinates
#pragma region DiabetesType2
const std::vector<TError_Grid_Point> PARKES_ZONE_A_T2 = {	
	{ 50.0 *scgms::mgdL_2_mmolL, 0.0 },{ 50.0 *scgms::mgdL_2_mmolL, 30.0 *scgms::mgdL_2_mmolL},{ 90.0 *scgms::mgdL_2_mmolL, 80.0 *scgms::mgdL_2_mmolL},{ 330.0 *scgms::mgdL_2_mmolL, 230.0 *scgms::mgdL_2_mmolL},{ 550.0 *scgms::mgdL_2_mmolL,450.0 *scgms::mgdL_2_mmolL},
	{ 550.0 *scgms::mgdL_2_mmolL, 550.0 *scgms::mgdL_2_mmolL},
	{ 440.0 *scgms::mgdL_2_mmolL, 550.0 *scgms::mgdL_2_mmolL},{230.0 *scgms::mgdL_2_mmolL,330.0 * scgms::mgdL_2_mmolL},{ 30.0 *scgms::mgdL_2_mmolL, 50.0 *scgms::mgdL_2_mmolL},{0.0 , 50.0 *scgms::mgdL_2_mmolL},
	{0.0 , 0.0 },
};

const std::vector<TError_Grid_Point> PARKES_ZONE_B_Up_T2 = {
	{0.0 , 50.0 *scgms::mgdL_2_mmolL},{ 30.0 *scgms::mgdL_2_mmolL, 50.0 *scgms::mgdL_2_mmolL},{ 230.0 *scgms::mgdL_2_mmolL,330.0 *scgms::mgdL_2_mmolL},{ 440.0 *scgms::mgdL_2_mmolL, 550.0 *scgms::mgdL_2_mmolL},
	{ 280.0 *scgms::mgdL_2_mmolL,550.0 *scgms::mgdL_2_mmolL},{ 30.0 *scgms::mgdL_2_mmolL,60.0 *scgms::mgdL_2_mmolL},{0.0 ,60.0 *scgms::mgdL_2_mmolL}
};

const std::vector<TError_Grid_Point> PARKES_ZONE_B_Low_T2 = {
	{ 90.0 *scgms::mgdL_2_mmolL, 0.0 },{ 260.0 *scgms::mgdL_2_mmolL, 130.0 *scgms::mgdL_2_mmolL},{ 550.0 *scgms::mgdL_2_mmolL, 250.0 *scgms::mgdL_2_mmolL},
	{ 550.0 *scgms::mgdL_2_mmolL,450.0 *scgms::mgdL_2_mmolL},{ 330.0 *scgms::mgdL_2_mmolL, 230.0 *scgms::mgdL_2_mmolL},{ 90.0 *scgms::mgdL_2_mmolL, 80.0 *scgms::mgdL_2_mmolL},{ 50.0 *scgms::mgdL_2_mmolL, 30.0 *scgms::mgdL_2_mmolL},{ 50.0 *scgms::mgdL_2_mmolL, 0.0 },
};

const std::vector<TError_Grid_Point> PARKES_ZONE_C_Up_T2 = {
	{0.0 ,60.0 *scgms::mgdL_2_mmolL},{ 30.0 *scgms::mgdL_2_mmolL,60.0 *scgms::mgdL_2_mmolL},{ 280.0 *scgms::mgdL_2_mmolL,550.0 *scgms::mgdL_2_mmolL},
	{ 125.0 * scgms::mgdL_2_mmolL, 550.0 *scgms::mgdL_2_mmolL},{ 35.0 * scgms::mgdL_2_mmolL, 90.0 *scgms::mgdL_2_mmolL},{ 25.0 * scgms::mgdL_2_mmolL, 80.0 *scgms::mgdL_2_mmolL},{0.0 , 80.0 *scgms::mgdL_2_mmolL},
};

const std::vector<TError_Grid_Point> PARKES_ZONE_C_Low_T2 = {
	{ 250.0 *scgms::mgdL_2_mmolL, 0.0 },{ 250.0 *scgms::mgdL_2_mmolL, 40.0 *scgms::mgdL_2_mmolL},{410.0 *scgms::mgdL_2_mmolL,110.0}, { 550.0 *scgms::mgdL_2_mmolL, 160.0 *scgms::mgdL_2_mmolL},
	{ 550.0 *scgms::mgdL_2_mmolL, 250.0 *scgms::mgdL_2_mmolL},{ 260.0 *scgms::mgdL_2_mmolL, 130.0 *scgms::mgdL_2_mmolL},{ 90.0 *scgms::mgdL_2_mmolL, 0.0 },

};

const std::vector<TError_Grid_Point> PARKES_ZONE_D_Up_T2 = {
	{0.0 , 80.0 *scgms::mgdL_2_mmolL},{ 25.0 * scgms::mgdL_2_mmolL, 80.0 *scgms::mgdL_2_mmolL},{ 35.0 * scgms::mgdL_2_mmolL, 90.0 *scgms::mgdL_2_mmolL},{ 125.0 * scgms::mgdL_2_mmolL, 550.0 *scgms::mgdL_2_mmolL},
	{ 50.0 *scgms::mgdL_2_mmolL,550.0 *scgms::mgdL_2_mmolL},{ 35.0 * scgms::mgdL_2_mmolL,200.0 *scgms::mgdL_2_mmolL},{0.0 ,200.0 *scgms::mgdL_2_mmolL},

};

const std::vector<TError_Grid_Point> PARKES_ZONE_D_Low_T2 = {	
	{ 550.0 *scgms::mgdL_2_mmolL, 160.0 *scgms::mgdL_2_mmolL},{410.0 *scgms::mgdL_2_mmolL,110.0}, { 250.0 *scgms::mgdL_2_mmolL, 40.0 *scgms::mgdL_2_mmolL},{ 250.0 *scgms::mgdL_2_mmolL, 0.0},
	{ 550.0 *scgms::mgdL_2_mmolL, 0.0 },
};

const std::vector<TError_Grid_Point> PARKES_ZONE_E_Up_T2 = {
	{0.0,200.0 *scgms::mgdL_2_mmolL},{ 35.0 * scgms::mgdL_2_mmolL,200.0 *scgms::mgdL_2_mmolL},{ 50.0 *scgms::mgdL_2_mmolL,550.0 *scgms::mgdL_2_mmolL},
	{0.0 , 550.0 *scgms::mgdL_2_mmolL},
};
#pragma endregion

const TError_Grid Parkes_Error_Grid_Type_1 = {

	{ NError_Grid_Zone::A, PARKES_ZONE_A_T1},
	
	{ NError_Grid_Zone::B, PARKES_ZONE_B_Up_T1},
	{ NError_Grid_Zone::C, PARKES_ZONE_C_Up_T1},
	{ NError_Grid_Zone::D, PARKES_ZONE_D_Up_T1},
	{ NError_Grid_Zone::E, PARKES_ZONE_E_Up_T1},

	{ NError_Grid_Zone::B, PARKES_ZONE_B_Low_T1},
	{ NError_Grid_Zone::C, PARKES_ZONE_C_Low_T1},
	{ NError_Grid_Zone::D, PARKES_ZONE_D_Low_T1},
};

const TError_Grid Parkes_Error_Grid_Type_2 = {
	{ NError_Grid_Zone::A, PARKES_ZONE_A_T2},

	{ NError_Grid_Zone::B, PARKES_ZONE_B_Up_T2},
	{ NError_Grid_Zone::C, PARKES_ZONE_C_Up_T2},
	{ NError_Grid_Zone::D, PARKES_ZONE_D_Up_T2},
	{ NError_Grid_Zone::E, PARKES_ZONE_E_Up_T2},

	{ NError_Grid_Zone::B, PARKES_ZONE_B_Low_T2},
	{ NError_Grid_Zone::C, PARKES_ZONE_C_Low_T2},
	{ NError_Grid_Zone::D, PARKES_ZONE_D_Low_T2},
};

