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

#include "../../../common/iface/UIIface.h"
#include "../../../common/rtl/hresult.h"
#include "../../../common/rtl/ModelsLib.h"


namespace diffusion_v2_model {
	constexpr GUID signal_Diffusion_v2_Blood = { 0xd96a559b, 0xe247, 0x41e0,{ 0xbd, 0x8e, 0x78, 0x8d, 0x20, 0xdb, 0x9a, 0x70 } }; // {D96A559B-E247-41E0-BD8E-788D20DB9A70}
	constexpr GUID signal_Diffusion_v2_Ist = { 0x870ddbd6, 0xdaf1, 0x4877,{ 0xa8, 0x9e, 0x5e, 0x7b, 0x2, 0x8d, 0xa6, 0xc7 } };  // {870DDBD6-DAF1-4877-A89E-5E7B028DA6C7}


	const double default_parameters[param_count] = { 1.091213, -0.015811, -0.174114, 0.0233101854166667, -2.6E-6, 0.0185995368055556 };

	struct TParameters {
		union {
			struct {
				double p, cg, c, dt, k, h;
			};
			double vector[param_count];
		};
	};
}


namespace steil_rebrin {
	constexpr GUID signal_Steil_Rebrin_Blood = { 0x287b9bb8, 0xb73b, 0x4485,{ 0xbe, 0x20, 0x2c, 0x8c, 0x40, 0x98, 0x3b, 0x16 } }; // {287B9BB8-B73B-4485-BE20-2C8C40983B16}

	//DelFavero 2014 flaw of the Steil-Rebrin model
	const double default_parameters[param_count] = { 0.00576459, 1.0 / 1.02164072, 0.0, 0.0 };

	struct TParameters {
		union {
			struct {
				double tau, alpha, beta, gamma;	//and g is considered == 1.0
			};
			double vector[param_count];
		};
	};
}

namespace steil_rebrin_diffusion_prediction {
	constexpr GUID signal_Steil_Rebrin_Diffusion_Prediction = { 0xf997edf4, 0x357c, 0x4cb2, { 0x8d, 0x7c, 0xf6, 0x81, 0xa3, 0x76, 0xe1, 0x7c } };

	//const double default_parameters[param_count] = { 1.091213, -0.015811, -0.174114, 0.0233101854166667, 1.02164072, 1.0 / 0.00576459  };
	const double default_parameters[param_count] = { 0.00116362, -7.20151e-11, 0.0794574, 0.010417, 850.437, 0.00630974 };	//15 minutes prediction

	struct TParameters {
		union {
			struct {
				double p, cg, c, dt, inv_g, tau;
			};
			double vector[param_count];
		};
	};
}


namespace diffusion_prediction {
	constexpr GUID signal_Diffusion_Prediction = { 0x43fcd03d, 0xb8bc, 0x497d, { 0x9e, 0xab, 0x3d, 0x1e, 0xeb, 0x3e, 0xbb, 0x5c } };

	const double default_parameters[param_count] = { 1.091213, -0.015811, -0.174114, 0.0233101854166667, 1.065754, -0.004336, 0.167069, 0.0247337965277778 - 0.0233101854166667 };

	struct TParameters {
		union {
			struct {
				double p, cg, c, dt;
			} retrospective, predictive;
			double vector[param_count];
		};
	};
}

namespace constant_model {

	constexpr GUID signal_Constant = { 0xfc616dd, 0x7bf, 0x49ea, { 0xbf, 0x2c, 0xfc, 0xa1, 0x39, 0x42, 0x9f, 0x5 } };

	const size_t param_count = 1;
	const double default_parameters[param_count] = { 6.66 };

	struct TParameters {
		union {
			struct {
				double c;
			};
			double vector[param_count];
		};
	};
}

namespace bergman_model {
	
	constexpr GUID model_id = { 0x8114b2a6, 0xb4b2, 0x4c8d, { 0xa0, 0x29, 0x62, 0x5c, 0xbd, 0xb6, 0x82, 0xef } };

	constexpr GUID signal_Bergman_BG = { 0xe1cd07ef, 0xb079, 0x4911, { 0xb7, 0x9b, 0xd2, 0x3, 0x48, 0x61, 0x1, 0xc8 } };				// {E1CD07EF-B079-4911-B79B-D203486101C8}
	constexpr GUID signal_Bergman_IG = { 0x582cfe11, 0x68b9, 0x4e5e, { 0xb5, 0xb, 0x19, 0x78, 0xc0, 0x77, 0xa8, 0x4e } };				// {582CFE11-68B9-4E5E-B50B-1978C077A84E}
	constexpr GUID signal_Bergman_IOB = { 0x9cddc004, 0x12ca, 0x4380, { 0x96, 0xff, 0x83, 0x19, 0xa8, 0xce, 0x79, 0xa8 } };				// {9CDDC004-12CA-4380-96FF-8319A8CE79A8}
	constexpr GUID signal_Bergman_COB = { 0x3bd439f0, 0x8b5a, 0x4004, { 0x9b, 0x70, 0x24, 0xe5, 0xe2, 0x52, 0x3b, 0x7a } };				// {3BD439F0-8B5A-4004-9B70-24E5E2523B7A}
	constexpr GUID signal_Bergman_Basal_Insulin = { 0xfc556839, 0xd6c0, 0x4646, { 0xa3, 0x46, 0x8, 0x21, 0xd2, 0x5f, 0x7e, 0x29 } };	// {FC556839-D6C0-4646-A346-0821D25F7E29}
	constexpr GUID signal_Bergman_Insulin_Activity = { 0x755cfd08, 0x2b12, 0x43b6, { 0xa4, 0x55, 0x58, 0x6, 0x15, 0x68, 0x44, 0x6e } };	// {755CFD08-2B12-43B6-A455-58061568446E}

	constexpr size_t model_param_count = 30;

	struct TParameters {
		union {
			struct {
				double p1, p2, p3, p4;
				double k12, k21;
				double Vi;
				double BodyWeight;
				double VgDist;
				double d1rate, d2rate;
				double irate;
				double Qb, Ib;
				double Q10, Q20, X0, I0, D10, D20, Isc0, Gsc0;
				double BasalRate0;
				double p, cg, c, dt, k, h;
				double Ag;
			};
			double vector[model_param_count];
		};
	};

	constexpr bergman_model::TParameters lower_bounds = {{ {
		0.005, 0.005, 5.0e-07, 0.1,					// p1, p2, p3, p4
		0.001, 0.001,								// k12, k21
		8.0,										// Vi
		10.0,										// BodyWeight
		0.05,										// VgDist
		0.01, 0.01,									// d1rate, d2rate
		0.01,										// irate
		0.0, 0.0,									// Qb, Ib
		0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0,		// Q10, Q20, X0, I0, D10, D20, Isc0, Gsc0
		0.0,										// BasalRate0
		0, -0.5, -10.0, 0,							// p, cg, c, dt
		-1.0, 0.0,									// k, h
		0.05										// Ag
	}} };
	constexpr bergman_model::TParameters default_parameters = {{ {
		0.028735, 0.028344, 5.035e-05, 0.3,			// p1, p2, p3, p4
		0.1, 0.1,									// k12, k21
		12.0,										// Vi
		70.0,										// BodyWeight
		0.22,										// VgDist
		0.05, 0.05,									// d1rate, d2rate
		0.04,										// irate
		95.0, 9.2,									// Qb, Ib
		220.0, 100.0, 0, 0, 0, 0, 0, 95.0,			// Q10, Q20, X0, I0, D10, D20, Isc0, Gsc0
		0,											// BasalRate0
		0.929, -0.037, 0.0, 0.0233101854166667,		// p, cg, c, dt
		0, 0.0185995368055556,						// k, h
		0.35										// Ag
	}} };
	constexpr bergman_model::TParameters upper_bounds = {{ {
		0.1, 0.1, 0.05, 1.0,										// p1, p2, p3, p4
		0.3, 0.3,													// k12, k21
		18.0,														// Vi
		100.0,														// BodyWeight
		1.0,														// VgDist
		1.0, 1.0,													// d1rate, d2rate
		1.0,														// irate
		200.0, 20.0,												// Qb, Ib
		300.0, 300.0, 100.0, 200.0, 150.0, 150.0, 50.0, 300.0,		// Q10, Q20, X0, I0, D10, D20, Isc0, Gsc0
		5.0,														// BasalRate0
		2.0, 0.0, 5.0, 1.0 / (24.0),								// p, cg, c, dt
		0.0, 1 / (24.0),											// k, h
		1.5															// Ag
	}} };
}

namespace uva_padova_S2013 {
	
	constexpr GUID model_id = { 0xb387a874, 0x8d1e, 0x460b, { 0xa5, 0xec, 0xba, 0x36, 0xab, 0x75, 0x16, 0xde } };						// {B387A874-8D1E-460B-A5EC-BA36AB7516DE}

	constexpr GUID signal_UVa_Padova_IG = { 0x55b07d3d, 0xd99, 0x47d0, { 0x8a, 0x3b, 0x3e, 0x54, 0x3c, 0x25, 0xe5, 0xb1 } };			// {55B07D3D-0D99-47D0-8A3B-3E543C25E5B1}
	constexpr GUID signal_UVa_Padova_BG = { 0x1eee155a, 0x9150, 0x4958, { 0x8a, 0xfd, 0x31, 0x61, 0xb7, 0x3c, 0xf9, 0xfc } };			// {1EEE155A-9150-4958-8AFD-3161B73CF9FC}
	constexpr GUID signal_UVa_Padova_Delivered_Insulin = { 0xaa402ce3, 0xba4a, 0x457b, { 0xaa, 0x19, 0x1b, 0x90, 0x8b, 0x9b, 0x53, 0xc4 } };	// {AA402CE3-BA4A-457B-AA19-1B908B9B53C4}

	constexpr size_t model_param_count = 61;

	struct TParameters {
		union {
			struct {
				double Qsto1_0, Qsto2_0, Qgut_0, Gp_0, Gt_0, Ip_0, X_0, I_0, XL_0, Il_0, Isc1_0, Isc2_0, Gs_0;
				double BW, Gb, Ib;
				double kabs, kmax, kmin;
				double b, d;
				double Vg, Vi, Vmx, Km0;
				double k2, k1, p2u, m1, m2, m4, m30, ki, kp2, kp3;
				double f, ke1, ke2, Fsnc, Vm0, kd, ksc, ka1, ka2, u2ss, kp1;

				double kh1, kh2, kh3, SRHb;
				double n, rho, sigma1, sigma2, delta, xi, kH, Hb;
				double XH_0;
				double Hsc1b, Hsc2b;
			};
			double vector[model_param_count];
		};
	};

	constexpr uva_padova_S2013::TParameters lower_bounds = {{{
		//  Qsto1_0, Qsto2_0, Qgut_0, Gp_0, Gt_0, Ip_0, X_0, I_0, XL_0, Il_0, Isc1_0, Isc2_0, Gs_0
		    0,       0,       0,      20,   20,   0,    0,   0,   0,    0,    0,      0,      20,
		//  BW,   Gb,   Ib
		    10.0, 30.0, 10.2,
		//  kabs, kmax, kmin
		    0.0,  1.0,  0.0,
		//  b,   d
		    0.2, 0.05,
		//  Vg,  Vi,    Vmx,   Km0
		    0.4, 0.005, 0.005, 100.0,
		//  k2,   k1,    p2u,   m1,   m2,   m4,    m30,  ki,     kp2,    kp3,
		    0.01, 0.005, 0.001, 0.01, 0.05, 0.001, 0.05, 0.0001, 0.0001, 0.002,
		//  f,    ke1,     ke2, Fsnc, Vm0, kd,    ksc,   ka1,    ka2,    u2ss, kp1
		    0.05, 0.00001, 20,  0.1,  0.2, 0.002, 0.008, 0.0001, 0.0005, 0.05, 1.0,
		//  kh1,  kh2,  kh3,  SRHb
		    0.01, 0.01, 0.01, 1.0,
		//  n, rho, sigma1, sigma2, delta, xi, kH, Hb
		    0, 0,   0,      0,      0,     0,  0,  0,
		//  XH_0, Hsc1b, Hsc2b
		    0,    1.0,   1.0

	}} };
	constexpr uva_padova_S2013::TParameters default_parameters = {{ {
		//  Qsto1_0, Qsto2_0, Qgut_0, Gp_0,       Gt_0,          Ip_0,      X_0, I_0,    XL_0,   Il_0,          Isc1_0,        Isc2_0,        Gs_0
		    0,       0,       0,      265.370112, 162.457097269, 5.5043265, 0,   100.25, 100.25, 3.20762505142, 72.4341762342, 141.153779328, 265.370112,
		//  BW,     Gb,     Ib
		    102.32, 138.56, 100.25,
		//  kabs,    kmax,     kmin
		    0.08906, 0.046122, 0.0037927,
		//  b,       d
		    0.70391, 0.21057,
		//  Vg,     Vi,       Vmx,      Km0
		    1.9152, 0.054906, 0.031319, 253.52,
		//  k2,       k1,       p2u,      m1,      m2,             m4,             m30,     ki,        kp2,     kp3,
		    0.087114, 0.058138, 0.027802, 0.15446, 0.225027424083, 0.090010969633, 0.23169, 0.0046374, 0.00469, 0.01208,
		//  f,   ke1,    ke2, Fsnc, Vm0,          kd,     ksc,    ka1,    ka2,    u2ss,         kp1
		    0.9, 0.0005, 339, 1,    3.2667306607, 0.0152, 0.0766, 0.0019, 0.0078, 1.2386244136, 4.73140582528,
		//  kh1,  kh2,  kh3,  SRHb
		    0.05, 0.05, 0.05, 10.0,

		// not identified, guessed:

		//  n,    rho,  sigma1, sigma2, delta, xi,   kH,   Hb
		    0.95, 0.12, 0.4,    0.3,    0.08,  0.02, 0.05, 30.0,
		//  XH_0, Hsc1b, Hsc2b
		    0,    15.0,  15.0
	}} };
	constexpr uva_padova_S2013::TParameters upper_bounds = {{ {
		//  Qsto1_0, Qsto2_0, Qgut_0, Gp_0, Gt_0, Ip_0, X_0, I_0, XL_0, Il_0, Isc1_0, Isc2_0, Gs_0
		    500,     500,     500,    500,  500,  50,   300, 200, 300,  200,  500,    500,    500,
		//  BW,  Gb,  Ib
		    250, 300, 200,
		//  kabs, kmax, kmin
		    0.8,  1.0,  0.5,
		//  b,   d
		    2.0, 2.0,
		//  Vg,   Vi,  Vmx, Km0
		    10.0, 2.0, 0.5, 500,
		//  k2,  k1,  p2u, m1,  m2,  m4,  m30, ki,   kp2,  kp3,
		    0.6, 0.2, 0.2, 0.9, 1.0, 1.0, 1.0, 0.05, 0.02, 0.5,
		//  f,   ke1,  ke2,  Fsnc, Vm0, kd,  ksc, ka1,  ka2, u2ss, kp1
		    3.0, 0.01, 1000, 5,    20,  0.8, 0.9, 0.05, 0.1, 10,   20,
		//  kh1, kh2, kh3, SRHb
		    1.0, 1.0, 1.0, 100.0,
		//  n, rho, sigma1, sigma2, delta, xi,  kH,  Hb
		    3, 1.0, 2.0,    2.0,    1.0,   0.8, 1.0, 200,
		//  XH_0, Hsc1b, Hsc2b
		    200,  100.0, 100.0
	}} };
}


namespace ge_model {
	//grammatical evolution model
	constexpr GUID model_id = { 0x39932e74, 0x7fc3, 0x4965, { 0x9e, 0x55, 0xee, 0x33, 0xf6, 0x3, 0x5a, 0xcb } };

	//single output signal
	constexpr GUID ge_signal_id = { 0x2c8c1272, 0x4b01, 0x4c66, { 0xb6, 0xb3, 0x31, 0xc4, 0xf7, 0x34, 0x99, 0x5a } };

	constexpr size_t ge_variables_count = 20;
	constexpr size_t max_instruction_count = 200;
	constexpr size_t model_param_count = ge_variables_count + max_instruction_count;

	struct TParameters {
		union {
			struct {
				//initial state
				double data[ge_variables_count]; //ge-specific variables
				double instructions[max_instruction_count];
			};
			double vector[model_param_count];
		};
	};

}

namespace insulin_bolus
{
	const size_t param_count = 1;
	const double default_parameters[param_count] = { 0.0666 };

	struct TParameters {
		union {
			struct {
				double csr;
			};
			double vector[param_count];
		};
	};
}

namespace const_isf
{
	constexpr const GUID const_isf_signal_id = { 0x9e76e1d9, 0x7dec, 0x4ffd, { 0xa3, 0xd0, 0xad, 0x4e, 0x4e, 0xc0, 0x31, 0x32 } };	// {9E76E1D9-7DEC-4FFD-A3D0-AD4E4EC03132}

	const size_t param_count = 1;
	const double default_parameters[param_count] = { 0.36 };

	struct TParameters {
		union {
			struct {
				double isf;
			};
			double vector[param_count];
		};
	};
}

namespace const_cr {
	constexpr const GUID const_cr_signal_id = { 0xb09dea1f, 0xae22, 0x4e27, { 0x90, 0x25, 0xc7, 0x69, 0xc2, 0x7d, 0x50, 0x74 } };	// {B09DEA1F-AE22-4E27-9025-C769C27D5074}

	const size_t param_count = 1;
	const double default_parameters[param_count] = { 0.0667 };

	struct TParameters {
		union {
			struct {
				double cr;
			};
			double vector[param_count];
		};
	};
}

namespace pattern_prediction {
	const GUID filter_id = { 0xa730a576, 0xe84d, 0x4834, { 0x82, 0x6f, 0xfa, 0xee, 0x56, 0x4e, 0x6a, 0xbd } };  // {A730A576-E84D-4834-826F-FAEE564E6ABD}
	const GUID calc_id = { 0x16f8ebcc, 0xafca, 0x4cd3, { 0x9c, 0x8a, 0xe1, 0xa5, 0x17, 0x89, 0xef, 0xdc } }; // {16F8EBCC-AFCA-4CD3-9C8A-E1A51789EFDC}

	constexpr const GUID signal_Pattern_Prediction = { 0x4f9d0e51, 0x65e3, 0x4aaf, { 0xa3, 0x87, 0xd4, 0xd, 0xee, 0xe0, 0x72, 0x50 } }; 		// {4F9D0E51-65E3-4AAF-A387-D40DEEE07250}


	constexpr double Low_Threshold = 3.0;			//mmol/L below which a medical attention is needed
	constexpr double High_Threshold = 13.0;			//dtto above
	constexpr size_t Internal_Bound_Count = 30;

	constexpr double Band_Size = (High_Threshold - Low_Threshold) / static_cast<double>(Internal_Bound_Count);						//must imply relative error <= 10% 
	constexpr double Inv_Band_Size = 1.0 / Band_Size;		//abs(Low_Threshold-Band_Size)/Low_Threshold 
	constexpr double Half_Band_Size = 0.5 / Inv_Band_Size;
	constexpr size_t Band_Count = 2 + Internal_Bound_Count;
	//1 band < mLow_Threshold, n bands in between, and 1 band >=mHigh_Threshold	

	enum class NPattern_Dir : uint8_t {
		negative = 0,
		zero,
		positive,
		count
	};

	constexpr size_t param_count = 1 + Band_Count * static_cast<size_t>(NPattern_Dir::count) * static_cast<size_t>(NPattern_Dir::count);

	struct TParameters {
		union {
			struct {
				double dt;
				std::array<
					std::array<
						std::array<double, static_cast<size_t>(NPattern_Dir::count)>, 
						static_cast<size_t>(NPattern_Dir::count)>, 
					Band_Count> bands;
			};
			std::array<double, param_count> vector;
		};
	};
	
	extern const TParameters default_parameters;
}

namespace const_neural_net {

	constexpr GUID calc_id = { 0xe34c6737, 0x1637, 0x48d1, { 0x9a, 0x53, 0xe8, 0xba, 0xd, 0xa, 0x5, 0x60 } };	// {E34C6737-1637-48D1-9A53-E8BA0D0A0560}

	constexpr GUID signal_Neural_Net_Prediction = { 0x43607228, 0x6d87, 0x44a4, { 0x84, 0x35, 0xe4, 0xf3, 0xc5, 0xda, 0x62, 0xdd } };	// {43607228-6D87-44A4-8435-E4F3C5DA62DD}

	
	constexpr size_t layers_count = 4;
	constexpr double band_offset = 4.0;
	constexpr double band_size = 1.0 / 3.0;	//mmol/L
	constexpr size_t input_count = 5;
	constexpr std::array<size_t, layers_count> layers_size = {5, 5, 5, 5};

	constexpr size_t count_param_count() {
		size_t result = input_count * layers_size[0];

		for (size_t i = 1; i < layers_size.size() - 1; i++)
			result += layers_size[i - 1] * layers_size[i];

		return result;
	}

	constexpr size_t param_count = count_param_count() + 1;	//+ dt
	struct TParameters {
		union {
			struct {				
				double dt;
				std::array<double, input_count    * layers_size[0]> weight0;
				std::array<double, layers_size[0] * layers_size[1]> weight1;
				std::array<double, layers_size[1] * layers_size[2]> weight2;
				std::array<double, layers_size[2] * layers_size[3]> weight3;
			};
			std::array<double, param_count> vector;
		};
	};	

	extern const TParameters default_parameters;
}


extern "C" HRESULT IfaceCalling do_get_model_descriptors(scgms::TModel_Descriptor **begin, scgms::TModel_Descriptor **end);
extern "C" HRESULT IfaceCalling do_get_signal_descriptors(scgms::TSignal_Descriptor * *begin, scgms::TSignal_Descriptor * *end);
extern "C" HRESULT IfaceCalling do_get_filter_descriptors(scgms::TFilter_Descriptor **begin, scgms::TFilter_Descriptor **end);
extern "C" HRESULT IfaceCalling do_create_discrete_model(const GUID *model_id, scgms::IModel_Parameter_Vector *parameters, scgms::IFilter *output, scgms::IDiscrete_Model **model);
extern "C" HRESULT IfaceCalling do_create_filter(const GUID *id, scgms::IFilter *output, scgms::IFilter **filter);
