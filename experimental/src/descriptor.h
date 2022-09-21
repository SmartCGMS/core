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
 * a) For non-profit, academic research, this software is available under the
 *      GPLv3 license.
 * b) For any other use, especially commercial use, you must contact us and
 *       obtain specific terms and conditions for the use of the software.
 * c) When publishing work with results obtained using this software, you agree to cite the following paper:
 *       Tomas Koutny and Martin Ubl, "Parallel software architecture for the next generation of glucose
 *       monitoring", Procedia Computer Science, Volume 141C, pp. 279-286, 2018
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
		for (int i = 0; i < size; ++i)
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

extern "C" HRESULT IfaceCalling do_get_model_descriptors(scgms::TModel_Descriptor **begin, scgms::TModel_Descriptor **end);
extern "C" HRESULT IfaceCalling do_get_signal_descriptors(scgms::TSignal_Descriptor * *begin, scgms::TSignal_Descriptor * *end);
extern "C" HRESULT IfaceCalling do_get_filter_descriptors(scgms::TFilter_Descriptor **begin, scgms::TFilter_Descriptor **end);
extern "C" HRESULT IfaceCalling do_create_discrete_model(const GUID *model_id, scgms::IModel_Parameter_Vector *parameters, scgms::IFilter *output, scgms::IDiscrete_Model **model);
extern "C" HRESULT IfaceCalling do_create_filter(const GUID *id, scgms::IFilter *output, scgms::IFilter **filter);