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

#pragma once

#include "../../../common/iface/DeviceIface.h"
#include "../../../common/iface/UIIface.h"
#include "../../../common/rtl/hresult.h"
#include "../../../common/rtl/ModelsLib.h"

using namespace scgms::literals;

namespace p559_model
{
	constexpr const GUID model_id = { 0xbe1b3d7c, 0x302f, 0x4d78, { 0x89, 0x92, 0x6a, 0xe0, 0x72, 0xe0, 0x79, 0xa7 } };	// {BE1B3D7C-302F-4D78-8992-6AE072E079A7}
	constexpr const GUID ig_signal_id = { 0xb81ea74d, 0x754b, 0x43a8, { 0x81, 0x1d, 0x6a, 0x85, 0x4f, 0x5, 0xc3, 0x74 } };	// {B81EA74D-754B-43A8-811D-6A854F05C374}

	const size_t param_count = 2;
	const double default_parameters[param_count] = { 6.6, 60.31 };
	const double lower_bound[param_count] = { 2.0, 0.0 };
	const double upper_bound[param_count] = { 22.0, 100.0 };

	struct TParameters {
		union {
			struct {
				double IG0;
				double omega_shift;
			};
			double vector[param_count];
		};
	};
}

namespace aim_ge
{
	constexpr const GUID model_id = { 0xfad48b96, 0x56aa, 0x4f7a, { 0xab, 0x9f, 0x89, 0x12, 0x8d, 0x95, 0x3, 0x3c } };	// {FAD48B96-56AA-4F7A-AB9F-89128D95033C}
	constexpr const GUID ig_id = { 0xc8dcd449, 0x22e6, 0x46db, { 0x9a, 0xa5, 0x1b, 0x23, 0xc6, 0x7c, 0x41, 0x97 } };	// {C8DCD449-22E6-46DB-9AA5-1B23C67C4197}

	constexpr size_t codons_count = 9;
	constexpr size_t param_count = 1 + codons_count;

	struct TParameters {
		union {
			struct
			{
				double IG_0;
				double codons[codons_count];
			};
			double vector[param_count];
		};
	};

	constexpr const double Constant_Lower_Bound = -10.0;
	constexpr const double Constant_Upper_Bound = 10.0;

	const TParameters lower_bounds = {
		2.0,
		0,0,0,0,0,0,0,0,0
	};
	const TParameters default_parameters = {
		6.6,
		0,0,0,0,0,0,0,0,0
	};
	const TParameters upper_bounds = {
		24.0,
		1,1,1,1,1,1,1,1,1
	};

}

namespace data_enhacement
{
	constexpr const GUID model_id = { 0x1f616ae9, 0xcec1, 0x427e, { 0xac, 0xdf, 0xf9, 0x45, 0x74, 0x67, 0x1, 0x3d } };		// {1F616AE9-CEC1-427E-ACDF-F9457467013D}
	constexpr const GUID pos_signal_id = { 0x3b7fa13d, 0x22fb, 0x4159, { 0x88, 0xf, 0xc7, 0x2b, 0x9a, 0xba, 0xc4, 0x34 } };	// {3B7FA13D-22FB-4159-880F-C72B9ABAC434}
	constexpr const GUID neg_signal_id = { 0xce78d527, 0xefc7, 0x4d19, { 0x86, 0x2, 0x39, 0xea, 0xd8, 0x45, 0x6, 0xbe } };	// {CE78D527-EFC7-4D19-8602-39EAD84506BE}

	const size_t param_count = 6;

	struct TParameters {
		union {
			struct {
				double sampling_horizon;
				double response_delay;
				double pos_factor;
				double neg_factor;
				double pos_factor_shift;
				double neg_factor_shift;
			};
			double vector[param_count];
		};
	};

	const TParameters lower_bounds = {
		scgms::One_Minute * 20.0,
		scgms::One_Minute * 10.0,
		0.1,
		2.0,
		0.5,
		0.2,
	};
	const TParameters default_parameters = {
		scgms::One_Minute * 30.0,
		scgms::One_Minute * 15.0,
		0.2,
		3.5,
		1.0,
		0.5,
	};
	const TParameters upper_bounds = {
		scgms::One_Minute * 50.0,
		scgms::One_Minute * 40.0,
		0.5,
		5.0,
		1.5,
		1.0,
	};
}

namespace cgp_pred
{
	constexpr size_t NumOfForecastValues = 4; // TMP: use 4, as for t+30, t+60, t+90, t+120 // 24 = 120 minutes, 1 value per 5 minutes

	constexpr size_t MatrixWidth = 20;
	constexpr size_t MatrixLength = 10;
	constexpr size_t Arity = 2;

	constexpr size_t TotalConstantCount = 4;

	constexpr const GUID model_id = { 0x987afc1f, 0xe42, 0x40b1, { 0xae, 0xe6, 0xad, 0x57, 0x38, 0xdc, 0x67, 0x1d } };// {987AFC1F-0E42-40B1-AEE6-AD5738DC671D}
	extern const GUID calculated_signal_ids[NumOfForecastValues];

	const size_t param_count = MatrixWidth * MatrixLength * (Arity+1) + NumOfForecastValues + TotalConstantCount;

	struct TParameters {
		union {
			struct
			{
				struct {
					double function;
					double operands[Arity];
				} nodes[MatrixWidth * MatrixLength];

				double outputs[NumOfForecastValues];
				double constants[TotalConstantCount];
			};
			double vector[param_count];
		};
	};

	struct TParameters_Converted {
		union {
			struct
			{
				struct {
					size_t function;
					size_t operands[Arity];
				} nodes[MatrixWidth * MatrixLength];

				size_t outputs[NumOfForecastValues];
				double constants[TotalConstantCount];
			};
			double vector[param_count];
		};
	};

	template<typename T, size_t size>
	constexpr std::array<T, size> generateArray(T val)
	{
		std::array<T, size> result{};
		for (size_t i = 0; i < size; ++i)
			result[i] = val;
		return result;
	}

	const std::array<double, param_count> lower_bounds{ generateArray<double, param_count>(0) };
	const std::array<double, param_count> default_parameters{ generateArray<double, param_count>(0.5) };
	const std::array<double, param_count> upper_bounds{ generateArray<double, param_count>(1.0) };
}

namespace bases_pred
{
	constexpr const GUID model_id = { 0x6bcaf380, 0x292a, 0x4aa8, { 0x85, 0x4a, 0x87, 0xe5, 0xa5, 0xba, 0x32, 0xc9 } };		// {6BCAF380-292A-4AA8-854A-87E5A5BA32C9}
	constexpr const GUID ig_signal_id = { 0xe20fb63c, 0x9028, 0x4593, { 0x8f, 0x96, 0x6, 0xa7, 0xc5, 0xb, 0x66, 0xff } };	// {E20FB63C-9028-4593-8F96-06A7C50B66FF}

	constexpr size_t Base_Functions_CHO = 4;
	constexpr size_t Base_Functions_Ins = 4;
	constexpr size_t Base_Functions_PA = 3;
	constexpr size_t Base_Functions_PatternPred = 0;
	constexpr size_t Base_Functions_Count = Base_Functions_CHO + Base_Functions_Ins + Base_Functions_PA + Base_Functions_PatternPred;

	constexpr double Prediction_Horizon = 60.0_min;

	const size_t param_count = Base_Functions_Count * 3 + 8 + 3;

	struct TParameters {
		union {
			struct {
				double curWeight;
				double baseAvgTimeWindow;
				double baseAvgOffset;
				double carbContrib;
				double insContrib;
				double paContrib;
				double carbPast;
				double insPast;
				struct {
					double amplitude;
					double tod_offset;
					double variance;
				} baseFunction[Base_Functions_Count];
				double c;
				double k;
				double h;
			};
			double vector[param_count];
		};
	};

	const TParameters lower_bounds = {
		0.05,
		scgms::One_Hour * 6.0,
		-3.0,
		0.01,
		0.01,
		0.01,
		0,
		0,

		-10.0, 0.0, 0.05,
		-10.0, 0.0, 0.05,
		-10.0, 0.0, 0.05,
		-10.0, 0.0, 0.05,
		-10.0, 0.0, 0.05,
		-10.0, 0.0, 0.05,
		-10.0, 0.0, 0.05,
		-10.0, 0.0, 0.05,
		-10.0, 0.0, 0.05,
		-10.0, 0.0, 0.05,
		-10.0, 0.0, 0.05,
		/*-10.0, 0.0, 0.05,
		-10.0, 0.0, 0.05,
		-10.0, 0.0, 0.05,
		-10.0, 0.0, 0.05,*/
		-5, -1, scgms::One_Minute * 5,
	};
	const TParameters default_parameters = {
		0.3,
		scgms::One_Hour * 12.0,
		0.0,
		0.5,
		0.5,
		0.5,
		scgms::One_Hour,
		scgms::One_Hour,

		3, 0.3, 0.2,
		5, 0.5, 0.2,
		4, 0.7, 0.2,
		4, 0.7, 0.2,
		4, 0.7, 0.2,
		4, 0.7, 0.2,
		4, 0.7, 0.2,
		4, 0.7, 0.2,
		4, 0.7, 0.2,
		4, 0.7, 0.2,
		4, 0.7, 0.2,
		/*4, 0.7, 0.2,
		4, 0.7, 0.2,
		4, 0.7, 0.2,
		4, 0.7, 0.2,*/
		0, -0.07, scgms::One_Minute * 5,
	};
	const TParameters upper_bounds = {
		0.95,
		scgms::One_Hour * 24.0,
		3.0,
		2.0,
		2.0,
		2.0,
		scgms::One_Hour * 2.0,
		scgms::One_Hour * 2.0,

		10.0, 0.99, 0.5,
		10.0, 0.99, 0.5,
		10.0, 0.99, 0.5,
		10.0, 0.99, 0.5,
		10.0, 0.99, 0.5,
		10.0, 0.99, 0.5,
		10.0, 0.99, 0.5,
		10.0, 0.99, 0.5,
		10.0, 0.99, 0.5,
		10.0, 0.99, 0.5,
		10.0, 0.99, 0.5,
		/*10.0, 0.99, 0.5,
		10.0, 0.99, 0.5,
		10.0, 0.99, 0.5,
		10.0, 0.99, 0.5,*/
		5, 0, scgms::One_Minute * 20,
	};
}

namespace bases_standalone
{
	constexpr const GUID model_id = { 0x862fa905, 0x19da, 0x4e2f, { 0x9c, 0xc2, 0x2c, 0x2c, 0x8c, 0x80, 0x68, 0x9f } };// {862FA905-19DA-4E2F-9CC2-2C2C8C80689F}
	constexpr const GUID ig_signal_id = { 0x22609887, 0xd4a9, 0x44a7, { 0xad, 0xa, 0xd0, 0x51, 0x94, 0xf1, 0x34, 0xc0 } };// {22609887-D4A9-44A7-AD0A-D05194F134C0}

	constexpr size_t Base_Functions_CHO = 4;
	constexpr size_t Base_Functions_Ins = 4;
	constexpr size_t Base_Functions_PA = 3;
	constexpr size_t Base_Functions_Count = Base_Functions_CHO + Base_Functions_Ins + Base_Functions_PA;

	const size_t param_count = Base_Functions_Count * 3 + 9 + 4 + 2 + 6;

	struct TParameters {
		union {
			struct {
				double curWeight;
				double baseWeight;
				double baseAvgTimeWindow;
				double baseAvgOffset;
				double carbContrib;
				double insContrib;
				double paContrib;
				double carbPast;
				double insPast;
				struct {
					double amplitude;
					double tod_offset;
					double variance;
				} baseFunction[Base_Functions_Count];
				double c;
				double baseValue;
				double k1;
				double h1;
				double initCarbs, initIns;
				double Ag, t_maxI, t_maxD, vi, ke, pa_decay;
			};
			double vector[param_count];
		};
	};

	const TParameters lower_bounds = {
		0.05,					// curWeight
		0.05,					// baseWeight
		scgms::One_Hour * 6.0,	// baseAvgTimeWindow
		-3.0,					// baseAvgOffset
		0.0001,					// carbContrib
		0.001,					// insContrib
		0.001,					// paContrib
		-scgms::One_Minute * 15,// carbPast
		-scgms::One_Minute * 15,// insPast

		// CHO bases
		-2.0, 0.1, 0.05,
		-2.0, 0.1, 0.05,
		-2.0, 0.1, 0.05,
		-2.0, 0.1, 0.05,
		// Insulin bases
		-2.0, 0.1, 0.05,
		-2.0, 0.1, 0.05,
		-2.0, 0.1, 0.05,
		-2.0, 0.1, 0.05,
		// PA bases
		-2.0, 0.1, 0.05,
		-2.0, 0.1, 0.05,
		-2.0, 0.1, 0.05,
		// c, baseValue, k1, h1
		0, 3.0, -1, 0,
		// initCarbs, initIns
		0, 0,
		// Ag, t_maxI, t_maxD, vi, ke, pa_decay
		0.75, 35, 30, 0.10, 0.12, 0.4,
	};
	const TParameters default_parameters = {
		0.3,					// curWeight
		0.2,					// baseWeight
		scgms::One_Hour * 12.0,	// baseAvgTimeWindow
		0.0,					// baseAvgOffset
		0.002,					// carbContrib
		0.5,					// insContrib
		0.1,					// paContrib
		0,						// carbPast
		0,						// insPast

		// CHO bases
		1, 0.7, 0.2,
		1, 0.7, 0.2,
		1, 0.7, 0.2,
		1, 0.7, 0.2,
		// Insulin bases
		-1, 0.7, 0.2,
		-1, 0.7, 0.2,
		-1, 0.7, 0.2,
		-1, 0.7, 0.2,
		// PA bases
		1, 0.7, 0.2,
		1, 0.7, 0.2,
		1, 0.7, 0.2,
		// c, baseValue, k1, h1
		6, 12.0, 0, 1,
		// initCarbs, initIns
		0, 0,
		// Ag, t_maxI, t_maxD, vi, ke, pa_decay
		0.85, 55, 40, 0.12, 0.138, 0.6,
	};
	const TParameters upper_bounds = {
		0.75,					// curWeight
		0.7,					// baseWeight
		scgms::One_Hour * 24.0,	// baseAvgTimeWindow
		3.0,					// baseAvgOffset
		1.0,					// carbContrib
		2.0,					// insContrib
		0.5,					// paContrib
		scgms::One_Minute * 15.0,	// carbPast
		scgms::One_Minute * 15.0,	// insPast

		// CHO bases
		2.0, 0.9, 0.3,
		2.0, 0.9, 0.3,
		2.0, 0.9, 0.3,
		2.0, 0.9, 0.3,
		// Insulin bases
		-0.2, 0.9, 0.3,
		-0.2, 0.9, 0.3,
		-0.2, 0.9, 0.3,
		-0.2, 0.9, 0.3,
		// PA bases
		2.0, 0.9, 0.3,
		2.0, 0.9, 0.3,
		2.0, 0.9, 0.3,
		// c, baseValue, k1, h1
		15, 25.0, 1, 10,
		// initCarbs, initIns
		50, 10,
		// Ag, t_maxI, t_maxD, vi, ke, pa_decay
		0.9, 65, 50, 0.145, 0.15, 0.85,
	};
}


namespace gege
{
	constexpr const GUID model_id = { 0x7be59b46, 0xb5db, 0x412c, { 0x96, 0x16, 0x30, 0xe7, 0x88, 0x32, 0xf2, 0xf0 } };		// {7BE59B46-B5DB-412C-9616-30E78832F2F0}

	constexpr const GUID ibr_id = { 0xac22fa49, 0x154f, 0x435e, { 0x9e, 0xdc, 0x36, 0xb, 0x87, 0x69, 0xec, 0x8b } };		// {AC22FA49-154F-435E-9EDC-360B8769EC8B}
	constexpr const GUID bolus_id = { 0x6741202c, 0x247d, 0x4430, { 0xaf, 0x45, 0x17, 0xf2, 0xc0, 0xcf, 0x73, 0xa9 } };		// {6741202C-247D-4430-AF45-17F2C0CF73A9}

	constexpr size_t param_count = 60;

	struct TParameters {
		union {
			struct {
				double codons[param_count];
				// may be split to some logical units in the future (e.g.; codons coding the grammar and parameters of every node)
			};
			double vector[param_count];
		};
	};

	const TParameters lower_bounds = {
		0,0,0,0,0,0,0,0,0,0,
		0,0,0,0,0,0,0,0,0,0,
		0,0,0,0,0,0,0,0,0,0,
		0,0,0,0,0,0,0,0,0,0,
		0,0,0,0,0,0,0,0,0,0,
		0,0,0,0,0,0,0,0,0,0,
	};
	const TParameters default_parameters = {
		0.5,0.5,0.5,0.5,0.5,0.5,0.5,0.5,0.5,0.5,
		0.5,0.5,0.5,0.5,0.5,0.5,0.5,0.5,0.5,0.5,
		0.5,0.5,0.5,0.5,0.5,0.5,0.5,0.5,0.5,0.5,
		0.5,0.5,0.5,0.5,0.5,0.5,0.5,0.5,0.5,0.5,
		0.5,0.5,0.5,0.5,0.5,0.5,0.5,0.5,0.5,0.5,
		0.5,0.5,0.5,0.5,0.5,0.5,0.5,0.5,0.5,0.5,
	};
	const TParameters upper_bounds = {
		1,1,1,1,1,1,1,1,1,1,
		1,1,1,1,1,1,1,1,1,1,
		1,1,1,1,1,1,1,1,1,1,
		1,1,1,1,1,1,1,1,1,1,
		1,1,1,1,1,1,1,1,1,1,
		1,1,1,1,1,1,1,1,1,1,
	};

}

namespace flr_ge
{
	constexpr const GUID model_id = { 0xbf37b3fc, 0xb794, 0x46fc, { 0xae, 0xd0, 0xb, 0xf1, 0xea, 0x83, 0xdd, 0x4b } };		// {BF37B3FC-B794-46FC-AED0-0BF1EA83DD4B}
	constexpr const GUID ibr_id = { 0xa2f6b465, 0x9f78, 0x421f, { 0x83, 0x98, 0x96, 0x7d, 0xc9, 0xd7, 0xc, 0xd8 } };		// {A2F6B465-9F78-421F-8398-967DC9D70CD8}
	constexpr const GUID bolus_id = { 0xde4ca4f0, 0xddcf, 0x4974, { 0x8d, 0xab, 0x1b, 0x9e, 0xd, 0xb9, 0x45, 0xef } };		// {DE4CA4F0-DDCF-4974-8DAB-1B9E0DB945EF}

	constexpr size_t rules_count = 20;
	constexpr size_t rule_size = 4;
	constexpr size_t constants_count = 20; // should be more

	constexpr size_t param_count = rules_count * rule_size + constants_count;

	struct TParameters {
		union {
			struct {
				double rules[rules_count * rule_size];
				double constants[constants_count];
			};
			double vector[param_count];
		};
	};

	constexpr const double Constant_Lower_Bound = -10.0;
	constexpr const double Constant_Upper_Bound = 10.0;

	const TParameters lower_bounds = {
		// rules
		0,0,0,0,0,0,0,0,0,0,
		0,0,0,0,0,0,0,0,0,0,
		0,0,0,0,0,0,0,0,0,0,
		0,0,0,0,0,0,0,0,0,0,
		0,0,0,0,0,0,0,0,0,0,
		0,0,0,0,0,0,0,0,0,0,
		0,0,0,0,0,0,0,0,0,0,
		0,0,0,0,0,0,0,0,0,0,
		// constants
		Constant_Lower_Bound,Constant_Lower_Bound,Constant_Lower_Bound,Constant_Lower_Bound,Constant_Lower_Bound,Constant_Lower_Bound,Constant_Lower_Bound,Constant_Lower_Bound,Constant_Lower_Bound,Constant_Lower_Bound,
		Constant_Lower_Bound,Constant_Lower_Bound,Constant_Lower_Bound,Constant_Lower_Bound,Constant_Lower_Bound,Constant_Lower_Bound,Constant_Lower_Bound,Constant_Lower_Bound,Constant_Lower_Bound,Constant_Lower_Bound,
	};
	const TParameters default_parameters = {
		// rules
		0,0,0,0,0,0,0,0,0,0,
		0,0,0,0,0,0,0,0,0,0,
		0,0,0,0,0,0,0,0,0,0,
		0,0,0,0,0,0,0,0,0,0,
		0,0,0,0,0,0,0,0,0,0,
		0,0,0,0,0,0,0,0,0,0,
		0,0,0,0,0,0,0,0,0,0,
		0,0,0,0,0,0,0,0,0,0,
		// constants
		0,0,0,0,0,0,0,0,0,0,
		0,0,0,0,0,0,0,0,0,0,
	};
	const TParameters upper_bounds = {
		// rules
		1,1,1,1,1,1,1,1,1,1,
		1,1,1,1,1,1,1,1,1,1,
		1,1,1,1,1,1,1,1,1,1,
		1,1,1,1,1,1,1,1,1,1,
		1,1,1,1,1,1,1,1,1,1,
		1,1,1,1,1,1,1,1,1,1,
		1,1,1,1,1,1,1,1,1,1,
		1,1,1,1,1,1,1,1,1,1,
		// constants
		Constant_Upper_Bound,Constant_Upper_Bound,Constant_Upper_Bound,Constant_Upper_Bound,Constant_Upper_Bound,Constant_Upper_Bound,Constant_Upper_Bound,Constant_Upper_Bound,Constant_Upper_Bound,Constant_Upper_Bound,
		Constant_Upper_Bound,Constant_Upper_Bound,Constant_Upper_Bound,Constant_Upper_Bound,Constant_Upper_Bound,Constant_Upper_Bound,Constant_Upper_Bound,Constant_Upper_Bound,Constant_Upper_Bound,Constant_Upper_Bound,
	};

}

namespace ann
{
	constexpr const GUID model_id = { 0xdde6dd27, 0xc7b7, 0x4f53, { 0xa6, 0x23, 0x34, 0x5, 0x5b, 0x9e, 0x51, 0xbb } };	// {DDE6DD27-C7B7-4F53-A623-34055B9E51BB}
	constexpr const GUID signal_id = { 0x37d8c64c, 0x79cb, 0x4b6b, { 0xa6, 0x0, 0xcd, 0x7, 0x4, 0x46, 0x2d, 0x5e } };	// {37D8C64C-79CB-4B6B-A600-CD0704462D5E}

	constexpr const size_t ig_history = 6;
	constexpr const size_t ig_trend_history = 2;
	constexpr const size_t iob_history = 6;
	constexpr const size_t cob_history = 6;
	constexpr const size_t pa_history = 3;

	constexpr const size_t inputs_size = ig_history + ig_trend_history + iob_history + cob_history + pa_history; // 6x IG, 2x IoB, 2x CoB, 2x PA
	constexpr const size_t hidden_1_size = 4;
	constexpr const size_t hidden_2_size = 4;

	constexpr const size_t param_count = inputs_size * hidden_1_size + 1 + hidden_1_size * hidden_2_size + 1 + hidden_2_size + 1;

	struct TParameters {
		union {
			struct {
				double w_input_to_1[inputs_size * hidden_1_size + 1];
				double w_1_to_2[hidden_1_size * hidden_2_size + 1];
				double w_2_to_output[hidden_2_size + 1];
			};
			double vector[param_count];
		};
	};

	const std::array<double, param_count> lower_bounds = { []() constexpr { 
		std::array<double, param_count> tmp{};
		for (size_t i = 0; i < param_count; i++)
			tmp[i] = -1;
		return tmp;
	}()};

	const std::array<double, param_count> default_parameters = { []() constexpr {
		std::array<double, param_count> tmp{};
		for (size_t i = 0; i < param_count; i++)
			tmp[i] = 0.1;
		return tmp;
	}() };

	const std::array<double, param_count> upper_bounds = { []() constexpr {
		std::array<double, param_count> tmp{};
		for (size_t i = 0; i < param_count; i++)
			tmp[i] = 1.0;
		return tmp;
	}() };

}

namespace ideg {

	namespace riskfinder {
		constexpr const GUID id = { 0x2662aafb, 0x5009, 0x4e54, { 0xb2, 0x3, 0x46, 0xd1, 0x14, 0xe5, 0x4b, 0x49 } }; // {2662AAFB-5009-4E54-B203-46D114E54B49}

		constexpr const GUID risk_hyper_signal_id = { 0xa348cf3f, 0x2667, 0x40c2, { 0x8e, 0x8e, 0x76, 0x4d, 0xe5, 0xc2, 0x95, 0x91 } }; // {A348CF3F-2667-40C2-8E8E-764DE5C29591}
		constexpr const GUID risk_hypo_signal_id = { 0xfe4ed8db, 0xc1c0, 0x411c, { 0x85, 0xe5, 0x78, 0xa4, 0x49, 0xf3, 0xf1, 0x8a } };  // {FE4ED8DB-C1C0-411C-85E5-78A449F3F18A}
		constexpr const GUID risk_avg_signal_id = { 0xa42391dc, 0x398a, 0x4e0e, { 0x9a, 0xe8, 0x65, 0x5e, 0xca, 0xd3, 0x7b, 0x4c } };   // {A42391DC-398A-4E0E-9AE8-655ECAD37B4C}

		extern const wchar_t* rsFind_Hyper;
		extern const wchar_t* rsFind_Hypo;
	}

	namespace logstopper {
		constexpr const GUID id = { 0x7ff5a2f5, 0xe37e, 0x4464, { 0xb5, 0x4a, 0xaf, 0x94, 0xe, 0xbe, 0xde, 0x40 } }; // {7FF5A2F5-E37E-4464-B54A-AF940EBEDE40}

		extern const wchar_t* rsStop_Time;
	}
}

