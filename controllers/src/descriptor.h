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

#pragma once

#include "../../../common/iface/UIIface.h"
#include "../../../common/rtl/hresult.h"
#include "../../../common/rtl/ModelsLib.h"


namespace basal_2_bolus {

	constexpr GUID filter_id = { 0xebf3a9e5, 0x5b00, 0x43ef, { 0xb6, 0x64, 0xde, 0x30, 0xcc, 0xd1, 0xa3, 0x7d } }; // {EBF3A9E5-5B00-43EF-B664-DE30CCD1A37D}

	constexpr size_t model_param_count = 2;
	const double default_parameters[model_param_count] = { 0.05, 5.0 * scgms::One_Minute };	//https://www.ncbi.nlm.nih.gov/pmc/articles/PMC4455475/

	struct TParameters {
		union {
			struct {
				double minimum_amount;		//e.g.; the insulin pump motor delivers 0.05 IU/hour
				double period;				//every 5 minutes
			};
			double vector[model_param_count];
		};
	};
}

namespace sensor {

	constexpr GUID filter_id = { 0x3c2f37ab, 0x92c7, 0x4903, { 0x8d, 0x89, 0x8c, 0x71, 0xca, 0xf8, 0xec, 0xb9 } }; // {3C2F37AB-92C7-4903-8D89-8C71CAF8ECB9}

	extern const wchar_t* rsNoise_Level;
	extern const wchar_t* rsCalibration_Signal_Id;
	extern const wchar_t* rsCalibration_Min_Value_Count;
	extern const wchar_t* rsPrecalibrated;
	extern const wchar_t* rsSensor_Drift_Per_Day;

}

namespace iob {
	constexpr size_t param_count = 2;
	const double default_parameters[param_count] = { 75.0*scgms::One_Minute, 180.0*scgms::One_Minute };

	struct TParameters {
		union {
			struct {
				double peak;
				double dia;
			};
			double vector[param_count];
		};
	};
}

namespace cob {
	constexpr size_t param_count = 2;
	const double default_parameters[param_count] = { 75.0*scgms::One_Minute, 180.0*scgms::One_Minute };

	struct TParameters {
		union {
			struct {
				double peak;
				double dia;	// TODO: rework parameters to fit COB calc!!! for now we wrongly retain IOB params
			};
			double vector[param_count];
		};
	};
}

namespace betapid_insulin_regulation {
	constexpr const GUID betapid_signal_id = { 0xd7eeda17, 0x781b, 0x438d, { 0xb9, 0x9b, 0x12, 0x81, 0xb9, 0x71, 0x33, 0x6e } };						// {D7EEDA17-781B-438D-B99B-1281B971336E}
	constexpr const GUID betapid2_signal_id = { 0x4b574705, 0x3832, 0x478d, { 0xa3, 0xae, 0xb6, 0xd8, 0x27, 0x74, 0x1f, 0xd } };						// {4B574705-3832-478D-A3AE-B6D827741F0D}

	const size_t param_count = 6;
	const double default_parameters[param_count] = { 2.0, 0.028, 0.24, 0.36, 0.0667, 1.3 };

	struct TParameters {
		union {
			struct {
				double kp, ki, kd, isf, csr, basal_insulin_rate;
			};
			double vector[param_count];
		};
	};
}

namespace betapid3_insulin_regulation
{
	constexpr const GUID betapid3_signal_id = { 0xda20eb00, 0xe8bc, 0x42a7, { 0x8e, 0x11, 0xbf, 0xc9, 0xa7, 0xf7, 0x79, 0x5 } };						// {DA20EB00-E8BC-42A7-8E11-BFC9A7F77905}

	const size_t param_count = 5;
	const double default_parameters[param_count] = { 2.0, 0.028, 0.24, 0.95, 1.3 };

	struct TParameters {
		union {
			struct {
				double k, ki, kd, kidecay, basal_insulin_rate;
			};
			double vector[param_count];
		};
	};
}

namespace lgs_basal_insulin
{
	constexpr const GUID lgs_basal_insulin_signal_id = { 0x6957caf0, 0x797, 0x442b, { 0xa0, 0x0, 0xe3, 0x94, 0xbb, 0x1e, 0x7d, 0x3d } };	// {6957CAF0-0797-442B-A000-E394BB1E7D3D}

	const size_t param_count = 3;

	const double lower_bound[param_count] = { 0.0, 0.0, 0.0 };
	const double default_parameters[param_count] = { 1.3, 4.0, 30.0 * (1.0 / (24.0*60.0)) };
	const double upper_bound[param_count] = { 3.0, 8.0, 120.0 * (1.0 / (24.0*60.0)) };

	struct TParameters {
		union {
			struct {
				double basal_insulin_rate;
				double suspend_threshold;
				double suspend_duration;
			};
			double vector[param_count];
		};
	};
}

namespace physical_activity_detection
{
	constexpr const GUID signal_id = { 0x5596068a, 0x4acf, 0x4e3f, { 0x9a, 0x98, 0xda, 0x6b, 0x9f, 0xe5, 0xe1, 0x9f } };	// {5596068A-4ACF-4E3F-9A98-DA6B9FE5E19F}

	const size_t param_count = 3;

	const double lower_bound[param_count] = { 30.0, 2.0, 15.0 };
	const double default_parameters[param_count] = { 65.0, 10.0, 20.0 };
	const double upper_bound[param_count] = { 100.0, 15.0, 30.0 };

	struct TParameters {
		union {
			struct {
				double heartrate_resting;
				double eda_threshold, eda_max;
			};
			double vector[param_count];
		};
	};
}
