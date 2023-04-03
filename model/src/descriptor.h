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
	
	constexpr GUID model_id = { 0xb387a874, 0x8d1e, 0x460b, { 0xa5, 0xec, 0xba, 0x36, 0xab, 0x75, 0x16, 0xde } };					// {B387A874-8D1E-460B-A5EC-BA36AB7516DE}

	constexpr GUID signal_IG = { 0x55b07d3d, 0xd99, 0x47d0, { 0x8a, 0x3b, 0x3e, 0x54, 0x3c, 0x25, 0xe5, 0xb1 } };					// {55B07D3D-0D99-47D0-8A3B-3E543C25E5B1}
	constexpr GUID signal_BG = { 0x1eee155a, 0x9150, 0x4958, { 0x8a, 0xfd, 0x31, 0x61, 0xb7, 0x3c, 0xf9, 0xfc } };					// {1EEE155A-9150-4958-8AFD-3161B73CF9FC}
	constexpr GUID signal_Delivered_Insulin = { 0xaa402ce3, 0xba4a, 0x457b, { 0xaa, 0x19, 0x1b, 0x90, 0x8b, 0x9b, 0x53, 0xc4 } };	// {AA402CE3-BA4A-457B-AA19-1B908B9B53C4}

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

namespace uva_padova_S2017 {
	
	constexpr GUID model_id = { 0xa5ea1ca6, 0x8a11, 0x4215, { 0x94, 0xb7, 0x70, 0xcf, 0xea, 0x28, 0x4f, 0xc6 } };						// {A5EA1CA6-8A11-4215-94B7-70CFEA284FC6}

	constexpr GUID signal_IG = { 0x7aec7ae, 0x545a, 0x4673, { 0xa7, 0x0, 0x8b, 0x72, 0xcb, 0xd0, 0x92, 0x1d } };						// {07AEC7AE-545A-4673-A700-8B72CBD0921D}
	constexpr GUID signal_BG = { 0x2751a1c7, 0x4e75, 0x4ba6, { 0xbe, 0x5d, 0x7b, 0xff, 0x42, 0x22, 0xbc, 0x9a } };						// {2751A1C7-4E75-4BA6-BE5D-7BFF4222BC9A}
	constexpr GUID signal_Delivered_Insulin = { 0x316503e6, 0x8aa7, 0x422a, { 0x9f, 0xfd, 0xf1, 0x67, 0x75, 0xc, 0x2c, 0xc8 } };		// {316503E6-8AA7-422A-9FFD-F167750C2CC8}
	constexpr GUID signal_IOB = { 0x85fa5fbe, 0xcdce, 0x4236, { 0xa0, 0xee, 0xfb, 0xd5, 0x84, 0x2f, 0x9b, 0xd4 } };						// {85FA5FBE-CDCE-4236-A0EE-FBD5842F9BD4}
	constexpr GUID signal_COB = { 0x9f395f6e, 0x259e, 0x4d73, { 0x9b, 0xeb, 0x3, 0x36, 0xf7, 0xf8, 0xf4, 0xc6 } };						// {9F395F6E-259E-4D73-9BEB-0336F7F8F4C6}

	constexpr size_t model_param_count = 74;

	struct TParameters {
		union {
			struct {
				double Qsto1_0, Qsto2_0, Qgut_0, Gp_0, Gt_0, Ip_0, X_0, I_0, XL_0, Il_0, Isc1_0, Isc2_0, Iid1_0, Iid2_0, Iih_0, Gsc_0;
				double BW, Gb, Ib;
				double kabs, kmax, kmin;
				double beta;
				double Vg, Vi, Vmx, Km0;
				double k2, k1, p2u, m1, m2, m4, m30, ki, kp2, kp3;
				double f, ke1, ke2, Fsnc, Vm0, kd, ka1, ka2, u2ss, kp1;
				double kh1, kh2, kh3, SRHb;
				double n, rho, sigma, delta, xi, kH, Hb;
				double XH_0;
				double Hsc1b, Hsc2b;

				double kir, ka, kaIih;
				double r1, r2, m3, alpha, c;
				double FIih, Ts;

				double b1, b2, a2;
			};
			double vector[model_param_count];
		};
	};

	const uva_padova_S2017::TParameters lower_bounds = {{{
		//  Qsto1_0, Qsto2_0, Qgut_0, Gp_0, Gt_0, Ip_0, X_0, I_0, XL_0, Il_0, Isc1_0, Isc2_0, Iid1_0, Iid2_0, Iih_0, Gsc_0
		    0,       0,       0,      20,   20,   0,    0,   0,   0,    0,    0,      0,      0,      0,      0,     20,
		//  BW,   Gb,   Ib
		    40.0, 100.0, 80.2,
		//  kabs, kmax, kmin
			0.03564,  0.022661,  0.0095262,
		//  beta,
		    0.2,
		//  Vg,  Vi,    Vmx,   Km0
			1.6385, 0.029655, 0.041075, 180.0,
		//  k2,       k1,       p2u,     m1,     m2,   m4,    m30,  ki,       kp2,    kp3,
			0.047802, 0.054595,0.027729,0.12367, 0.05, 0.001, 0.05, 0.0069266, 0.0017557, 0.0029212,
		//  f,    ke1,     ke2, Fsnc, Vm0, kd,      ka1,    ka2,    u2ss, kp1
		    0.05, 0.00001, 20,  0.1,  0.2, 0.014181, 0.0028953, 0.012974, 0.05, 1.0,
		//  kh1,  kh2,  kh3,  SRHb
		    0.01, 0.01, 0.01, 1.0,
		//  n, rho, sigma, delta, xi, kH, Hb
		    0, 0,   0,     0,     0,  0,  0,
		//  XH_0, Hsc1b, Hsc2b
		    0,    1.0,   1.0,
		//  kir, ka, kaIih
		    0,   0,  0,
		//  r1, r2, m3, alpha, c
		    0,  0,  0,  0,     0,
		//  FIih, Ts
		    0,    0,
		//  b1,      b2,   a2
		    0.00001, 0.01, 2,
	}} };
	const uva_padova_S2017::TParameters default_parameters = {{ {
		//  Qsto1_0, Qsto2_0, Qgut_0,      Gp_0,    Gt_0,    Ip_0,    X_0,     I_0,     XL_0,    Il_0,   Isc1_0, Isc2_0, Iid1_0, Iid2_0, Iih_0, Gsc_0
			499.765, 482.352, 498.9621911, 1000,    999.999, 16.0042, 0.08138, 0.00887, 0.02783, 60.874, 384.52, 331.14, 433.78, 319.05, 493.7, 331.586,
		//  BW,      Gb,      Ib
			73.990, 120.0, 112,
		//  kabs,     kmax, kmin
			0.1591, 0.042084,  0.014086,
		//  beta,
			2.946990633216978,
		//  Vg,      Vi,         Vmx,      Km0
			1.8488, 0.055183, 0.083187, 220.532,
		//  k2,       k1,       p2u,        m1,        m2,     m4,       m30,      ki,         kp2,         kp3,
			0.12     , 0.072423, 0.04397, 0.19216, 0.6763, 0.009774, 0.999007, 0.013891, 0.0049182, 0.0071654,
		//  f,       ke1,        ke2,     Fsnc,  Vm0,     kd,         ka1,        ka2,       u2ss,      kp1
			0.63609, 0.00883182, 223.497, 3.587, 0.2,     0.015764,   0.0038439,  0.017754, 0.2017074, 1,
		//  kh1,     kh2,      kh3,      SRHb
			0.01,    0.01,     0.401678, 3.77879188855556,
		//  n,       rho,         sigma,   delta,       xi,      kH, Hb
			0.94071, 0.487515406, 0.05165, 0.054130087, 0.43875, 0,  200,
		//  XH_0,    Hsc1b,   Hsc2b
			111.9906653991135, 100, 99.18615818834493,
		//  kir,     ka,       kaIih
			0, 0.05490551848298732, 0.04203039297914399,
		//  r1,     r2,        m3,        alpha,   c
			1.993641728083052, 0.2430338970476272, 0.07798578099115527, 1.988343075378012, 1.429239149342998,
		//  FIih,    Ts
			1.697960870575411, 10.37870037948553,
		//  b1,       b2,        a2
			0.06456339523000645, 0.0119872233716361, 18.15879855611503
	}} };
	const uva_padova_S2017::TParameters upper_bounds = {{ {
		//  Qsto1_0, Qsto2_0, Qgut_0, Gp_0, Gt_0, Ip_0, X_0, I_0, XL_0, Il_0, Isc1_0, Isc2_0, Iid1_0, Iid2_0, Iih_0, Gsc_0
		    500,     500,     500,    1000, 1000,  50,   300, 200, 300,  200,  500,    500,    500,    500,    500,   500,
		//  BW,  Gb,  Ib
		    120, 150, 180,
		//  kabs, kmax, kmin
			0.40495, 0.06705,  0.022363,
		//  beta,
		    4.0,
		//  Vg,   Vi,  Vmx, Km0
			2.1143, 0.08331, 0.12971, 248.9074,
		//  k2,     k1,      p2u,      m1,      m2,  m4,  m30, ki,   kp2,  kp3,
			0.2173, 0.10118, 0.066322, 0.32819, 1.0, 1.0, 1.0, 0.023453, 0.010936, 0.01432,
		//  f,   ke1,  ke2,  Fsnc, Vm0, kd,       ka1,  ka2, u2ss, kp1
		    3.0, 0.01, 1000, 5,    20,  0.019723, 0.0052572, 0.026687, 10,   20,
		//  kh1, kh2, kh3, SRHb
		    1.0, 1.0, 1.0, 100.0,
		//  n, rho, sigma, delta, xi,  kH,  Hb
		    3, 1.0, 2.0,   1.0,   0.8, 1.0, 200,
		//  XH_0, Hsc1b, Hsc2b
		    200,  100.0, 100.0,
		//  kir, ka, kaIih
		    2,   2,  2,
		//  r1, r2, m3, alpha, c
		    2,  2,  2,  2,     2,
		//  FIih, Ts
		    2,    20,
		//  b1,  b2,  a2
		    0.2, 1.5, 25,
	}} };
}

namespace samadi_model {

	constexpr GUID model_id = { 0xa7938698, 0x43cd, 0x4ec8, { 0xbb, 0x3a, 0x2a, 0x8d, 0xcb, 0x3d, 0x72, 0x52 } };						// {A7938698-43CD-4EC8-BB3A-2A8DCB3D7252}

	constexpr GUID signal_IG = { 0xbf703a0, 0xec3a, 0x4040, { 0xa0, 0x5a, 0x6b, 0xee, 0x24, 0x79, 0x64, 0xba } };						// {0BF703A0-EC3A-4040-A05A-6BEE247964BA}
	constexpr GUID signal_BG = { 0x6e25b35c, 0xb9af, 0x4282, { 0x98, 0xd8, 0xc6, 0xf, 0xd1, 0x4, 0x27, 0xb0 } };						// {6E25B35C-B9AF-4282-98D8-C60FD10427B0}
	constexpr GUID signal_Delivered_Insulin = { 0x73cd5f5c, 0x263e, 0x47ce, { 0x8b, 0x94, 0x13, 0x10, 0x4d, 0xd5, 0x8a, 0x59 } };		// {73CD5F5C-263E-47CE-8B94-13104DD58A59}
	constexpr GUID signal_IOB = { 0x5ac41cc7, 0x5b14, 0x424d, { 0xb1, 0xfc, 0xb5, 0x11, 0x2, 0xc3, 0x1c, 0xdb } };						// {5AC41CC7-5B14-424D-B1FC-B51102C31CDB}
	constexpr GUID signal_COB = { 0x2d55a02c, 0x5768, 0x45f2, { 0x8c, 0x6d, 0xf9, 0x5e, 0x9e, 0xcb, 0xf1, 0x47 } };						// {2D55A02C-5768-45F2-8C6D-F95E9ECBF147}

	constexpr size_t model_param_count = 43;

	struct TParameters {
		union {
			struct {
				double Q1_0, Q2_0, Gsub_0, S1_0, S2_0, I_0, x1_0, x2_0, x3_0;
				double D1_0, D2_0, DH1_0, DH2_0, E1_0, E2_0, TE_0;
				double k12, ka1, ka2, ka3;
				double kb1, kb2, kb3;
				double ke;
				double Vi, Vg;
				double EGP_0, F01;
				double tmaxi, tau_g, a, t_HR, t_in, n, t_ex, c1, c2;
				double Ag;
				double tmaxG;
				double alpha, beta;
				double BW, HRbase;
			};
			double vector[model_param_count];
		};
	};

	const samadi_model::TParameters lower_bounds = { {{
		//	Q1_0, Q2_0, Gsub_0, S1_0, S2_0, I_0, x1_0, x2_0, x3_0
			5,    5,    1,      0,    0,    0,   0,    0,    0,
		//	D1_0, D2_0, DH1_0, DH2_0, E1_0, E2_0, TE_0
			0,    0,    0,     0,     0,    0,    0,
		//	k12,  ka1,   ka2,  ka3
			0.05, 0.004, 0.05, 0.02,
		//	kb1, kb2, kb3
			0.000005, 0.00001, 0.0005,
		//	ke
			0.175,
		//	Vi, Vg
			0.1, 0.1,
		//	EGP_0, F01
			0.02, 0.005,
		//	tmaxi, tau_g, a,   t_HR, t_in, n, t_ex, c1,  c2
			60,    12,    0.7, 3,    0.5,  2, 150,  450, 80,
		//	Ag
			0.6,
		//	tmaxG
			36,
		//	alpha, beta
			1.3,   0.5,
		//	BW, HRbase
			20, 20
	}} };
	const samadi_model::TParameters default_parameters = { { {
		//	Q1_0, Q2_0, Gsub_0, S1_0, S2_0, I_0, x1_0,   x2_0,   x3_0
			55,   23,   4.7,    0,    0,    0,   0.0296, 0.0047, 0.2996,
		//	D1_0, D2_0, DH1_0, DH2_0, E1_0, E2_0, TE_0
			0,    0,    0,     0,     0,    0,    0,
		//	k12,   ka1,   ka2,  ka3
			0.066, 0.006, 0.06, 0.03,
		//	kb1, kb2,  kb3
			0.000031, 0.0000492, 0.00156,
		//	ke
			0.196,
		//	Vi,   Vg
			0.12, 0.16,
		//	EGP_0,  F01
			0.0161, 0.0097,
		//	tmaxi, tau_g, a,    t_HR, t_in, n, t_ex, c1,  c2
			69.5,  15,    0.77, 5,    1,    3, 200,  500, 100,
		//	Ag
			0.71,
		//	tmaxG
			40.4,
		//	alpha, beta
			1.79,  0.78,
		//	BW, HRbase
			85, 65
	}} };

	const samadi_model::TParameters upper_bounds = { { {
		//	Q1_0, Q2_0, Gsub_0, S1_0, S2_0, I_0, x1_0, x2_0, x3_0
			100,  100,  50,     600,  600,  20,  5,    5,    5,
		//	D1_0, D2_0, DH1_0, DH2_0, E1_0, E2_0, TE_0
			500,  500,  500,   500,   500,  500,  500,
		//	k12,  ka1,   ka2,  ka3
			0.07, 0.007, 0.07, 0.04,
		//	kb1, kb2,   kb3
			0.0001, 0.0001, 0.01,
		//	ke
			0.218,
		//	Vi, Vg
			0.2, 0.2,
		//	EGP_0, F01
			0.035, 0.03,
		//	tmaxi, tau_g, a,    t_HR, t_in, n, t_ex, c1,  c2
			78,    18,    0.85, 7,    1.5,  4, 250,  550, 120,
		//	Ag
			0.8,
		//	tmaxG
			45,
		//	alpha, beta
			2.3,   1.3,
		//	BW,  HRbase
			150, 90
	}} };
}


namespace samadi_gct2_model {

	constexpr GUID model_id = { 0x5752d01f, 0xde31, 0x4eca, { 0xbf, 0x97, 0x64, 0xff, 0x97, 0x1e, 0x33, 0x55 } };		// {5752D01F-DE31-4ECA-BF97-64FF971E3355}


	constexpr GUID signal_IG = { 0xc190ad9e, 0xde82, 0x40d3, { 0xb0, 0xf2, 0x8a, 0xbe, 0x16, 0x68, 0x66, 0xdd } };		// {C190AD9E-DE82-40D3-B0F2-8ABE166866DD}
	constexpr GUID signal_BG = { 0xd57b6630, 0xba55, 0x468a, { 0xad, 0xb3, 0x37, 0xd1, 0xf8, 0xa3, 0xab, 0xb } };		// {D57B6630-BA55-468A-ADB3-37D1F8A3AB0B}
	constexpr GUID signal_Delivered_Insulin = { 0xc4b8017d, 0x205c, 0x4780, { 0x83, 0xec, 0x3e, 0x4c, 0x56, 0x49, 0x42, 0x70 } };	// {C4B8017D-205C-4780-83EC-3E4C56494270}
	constexpr GUID signal_IOB = { 0x76470c10, 0xf4cf, 0x4b5d, { 0xb0, 0x94, 0x1e, 0xa1, 0xe3, 0x37, 0x95, 0xd7 } };		// {76470C10-F4CF-4B5D-B094-1EA1E33795D7}
	constexpr GUID signal_COB = { 0xde20951f, 0x6a89, 0x447d, { 0x99, 0x32, 0x7c, 0xce, 0xb6, 0xc7, 0x16, 0xca } };		// {DE20951F-6A89-447D-9932-7CCEB6C716CA}

	// parameters are the same as in the samadi_model case (TParameters, lower and upper bounds, default params)
}

namespace gct_model {

	constexpr GUID model_id = { 0xc91e7deb, 0x285, 0x4fa0, { 0x83, 0x1e, 0x94, 0xf0, 0xf1, 0xa0, 0x96, 0x2e } };					// {C91E7DEB-0285-4FA0-831E-94F0F1A0962E}

	constexpr GUID signal_IG = { 0xc4d0fa39, 0x120a, 0x49b1, { 0x86, 0x77, 0xd4, 0xe9, 0xcd, 0x5d, 0x59, 0xf9 } };					// {C4D0FA39-120A-49B1-8677-D4E9CD5D59F9}
	constexpr GUID signal_BG = { 0xb45606de, 0x3f1b, 0x4ec4, { 0xbb, 0x83, 0x2e, 0xe9, 0x9e, 0xc8, 0x31, 0x39 } };					// {B45606DE-3F1B-4EC4-BB83-2EE99EC83139}
	constexpr GUID signal_Delivered_Insulin = { 0xe279db89, 0x71c5, 0x4b89, { 0x97, 0xa1, 0x38, 0x17, 0x8, 0xb8, 0x1c, 0x2d } };	// {E279DB89-71C5-4B89-97A1-381708B81C2D}
	constexpr GUID signal_IOB = { 0xfe49fe8e, 0xf819, 0x4323, { 0xb8, 0x4d, 0x7f, 0x9, 0x1, 0xe4, 0xa2, 0x71 } };					// {FE49FE8E-F819-4323-B84D-7F0901E4A271}
	constexpr GUID signal_COB = { 0xb23e1df5, 0xd291, 0x4015, { 0xa0, 0x3d, 0xa9, 0x2e, 0xc3, 0xf1, 0x3c, 0x16 } };					// {B23E1DF5-D291-4015-A03D-A92EC3F13C16}

	constexpr size_t model_param_count = 37;

	/*
	Q1_0 - D2_0 - initial values/quantities

	Vq - distribution volume of glucose molecules in plasma ("volume of plasma")
	Vqsc - distribution volume of glucose molecules in subcutaneous tissue / interstitial fluid
	Vi - insulin distribution volume
	Q1b - basal glucose amount in primary distribution volume
	Gthr - glycosuria threshold [mmol/L]
	GIthr - insulin production glucose threshold (above this level, beta cells start to produce insulin) [mmol/L]

	q12 - transfer rate Q1 <-> Q2, diffusion
	q1sc - transfer rate Q1 <-> Qsc, diffusion
	ix - transfer rate I -> X
	xq1 - moderation rate Q1 -(X)-> sink
	d12 - transfer rate D1 -> D2 ("digestion" rate)
	d2q1 - transfer rate D2 -> Q1 (glucose absorption rate)
	isc2i - transfer rate Isc2 -> I

	q1e - base elimination of Q1 glucose
	q1ee - glucose elimination moderated by exercise
	q1e_thr - glycosuria elimination of Q1 glucose (over threshold)
	xe - elimination rate of X by glucose utilization moderation (coupled with xq1)

	q1p - glucose appearance rate from glycogen and miscellanous sources
	q1pe - glucose appearance moderated 
	ip - insulin production factor (how glucose stimulates insulin production)

	e_pa - exercise-production virtual modulator appearance
	e_ua - exercise-utilization virtual modulator appearance
	e_pe - exercise-production virtual modulator elimination rate
	e_ue - exercise-utilization virtual modulator elimination rate
	q_ep - glucose production modulation rate by exercise
	q_eu - glucose utilization modulation rate by exercise

	Aq - CHO bioavailability (how many % of glucose from meal is absorbed); TODO: this should be a parameter of CHO intake
	t_d - meal absorption time; TODO: this should be a parameter of CHO intake
	t_i - subcutaneous insulin absorption time; TODO: this should be a parameter of insulin dosage
	*/

	struct TParameters {
		union {
			struct {
				// initial quantities
				double Q1_0, Q2_0, Qsc_0, I_0, Isc_0, X_0, D1_0, D2_0;
				// patient quantity and base parameters
				double Vq, Vqsc, Vi, Q1b, Gthr, GIthr;
				// transfer parameters
				double q12, q1sc, ix, xq1, d12, d2q1, isc2i;
				// elimination parameters
				double q1e, q1ee, q1e_thr, xe;
				// production parameters
				double q1p, q1pe, ip;
				// exercise-related parameters
				double e_pa, e_ua, e_pe, e_ue, q_ep, q_eu;
				// misc parameters
				double Ag, t_d, t_i;
			};
			double vector[model_param_count];
		};
	};

	const gct_model::TParameters lower_bounds = { {{
		//	Q1_0, Q2_0, Qsc_0, I_0, Isc_0, X_0, D1_0, D2_0
			0,    0,    0,     0,   0,     0,   0,    0,
		//	Vq,  Vqsc, Vi, Q1b, Gthr, GIthr,
			3,   1,    3,  50,  8.0,  4.0,
		//	q12,  q1sc, ix,  xq1,   d12,  d2q1, isc2i
			0.01, 1.4,  1.4, 0.001, 14.0, 14.0, 0.05,
		//	q1e,   q1ee,  q1e_thr, xe,
			0.144, 0.144, 0.001,   0.01,
		//	q1p,     q1pe,   ip,
			0.00001, 0.0001, 0.0,
		//	e_pa, e_ua, e_pe, e_ue, q_ep, q_eu
			2000, 500,  2000,  500, 300,  100,
		//	Ag,  t_d,    t_i
			0.5, 10_min, 5_min
	}} };

	const gct_model::TParameters default_parameters = { { {
		//	Q1_0, Q2_0, Qsc_0, I_0,   Isc_0, X_0, D1_0, D2_0
			135,  65,   450,   4e-12, 16.2,  2.6, 0,    54.8,
		//	Vq,  Vqsc, Vi, Q1b, Gthr, GIthr,
			30,  25,   80, 240, 8.0, 5.0,
		//	q12,  q1sc, ix,  xq1,     d12,   d2q1,  isc2i
			0.01, 8,    3.4, 0.07697, 144.0, 144.0, 0.1,
		//	q1e,     q1ee,    q1e_thr, xe,
			0.38519, 0.38519, 0.8,     0.01,
		//	q1p,  q1pe, ip,
			0.01, 0.1,  0.0,
		//	e_pa, e_ua, e_pe, e_ue, q_ep, q_eu
			9000, 2000, 7425, 2000, 1800, 300,
		//	Ag,   t_d,    t_i
			0.98, 44_min, 119_min
	}} };

	const gct_model::TParameters upper_bounds = { { {
		//	Q1_0, Q2_0, Qsc_0, I_0, Isc_0, X_0, D1_0, D2_0
			500,  500,  500,   500, 500,   500, 200,  200,
		//	Vq,  Vqsc, Vi, Q1b,  Gthr, GIthr,
			60,  60,   80, 1000, 14.0, 8.0,
		//	q12, q1sc, ix,    xq1, d12,   d2q1,  isc2i
			0.9, 24.0, 144.0, 3.0, 144.0, 144.0, 0.5,
		//	q1e,  q1ee, q1e_thr, xe,
			14.4, 14.4, 0.8,     2.0,
		//	q1p,  q1pe, ip
			0.01, 0.1,  0.05,
		//	e_pa,  e_ua,  e_pe, e_ue, q_ep, q_eu
			12000, 12000, 9000, 9000, 2500, 1800,
		//	Ag,   t_d,    t_i
			0.98, 50_min, 120_min
	}} };
}

namespace gct2_model {

	constexpr GUID model_id = { 0x68c39029, 0x4637, 0x4f9e, { 0x93, 0xa3, 0xcb, 0x2f, 0x5a, 0x34, 0x10, 0x12 } };					// {68C39029-4637-4F9E-93A3-CB2F5A341012}

	constexpr GUID signal_IG = { 0x31b6c36f, 0x589b, 0x4c08, { 0xa3, 0x54, 0xc8, 0xa4, 0x61, 0xb7, 0x1e, 0xf6 } };					// {31B6C36F-589B-4C08-A354-C8A461B71EF6}
	constexpr GUID signal_BG = { 0xf2e61657, 0x3b3c, 0x467a, { 0xad, 0x44, 0xc5, 0x6b, 0xae, 0x29, 0x27, 0x69 } };					// {F2E61657-3B3C-467A-AD44-C56BAE292769}
	constexpr GUID signal_Delivered_Insulin = { 0x42b624fa, 0x5971, 0x433e, { 0x95, 0x35, 0xd2, 0x74, 0xce, 0x60, 0x9, 0x6a } };	// {42B624FA-5971-433E-9535-D274CE60096A}
	constexpr GUID signal_IOB = { 0x1613dabc, 0x1aed, 0x46b0, { 0x98, 0x10, 0x7f, 0xd, 0xba, 0xc8, 0xb1, 0x80 } };					// {1613DABC-1AED-46B0-9810-7F0DBAC8B180}
	constexpr GUID signal_COB = { 0x4a1d62fe, 0x9273, 0x4a73, { 0xa6, 0xfe, 0xb3, 0xcd, 0xe6, 0x4a, 0x2a, 0x9d } };					// {4A1D62FE-9273-4A73-A6FE-B3CDE64A2A9D}

	constexpr size_t model_param_count = 39;

	/*
	Q1_0 - D1_0 - initial values/quantities

	Vq - distribution volume of glucose molecules in plasma ("volume of plasma")
	Vqsc - distribution volume of glucose molecules in subcutaneous tissue / interstitial fluid
	Vi - insulin distribution volume
	Q1b - basal glucose amount in primary distribution volume
	Gthr - glycosuria threshold [mmol/L]
	GIthr - insulin production glucose threshold (above this level, beta cells start to produce insulin) [mmol/L]

	q12 - transfer rate of diffusion between Q1 and Q2
	q1sc - transfer rate of diffusion between Q1 and Qsc
	ix - transfer rate I -> X
	xq1 - moderation rate Q1 -(X)-> sink
	iscimod - transfer rate Isc -> I, bundled with local degradation (proposed by Hovorka, http://doi.org/10.1109/TBME.2004.839639 )

	q1e - base elimination of Q1 glucose
	q1ee - glucose elimination moderated by exercise
	q1e_thr - glycosuria elimination of Q1 glucose (over threshold)
	xe - elimination rate of X by glucose utilization moderation (coupled with xq1)

	q1p - glucose appearance rate from glycogen and miscellanous sources
	q1pe - glucose appearance moderated by exercise effects
	q1pi - glucose appearance inhibition by insulin presence
	ip - insulin production factor (how glucose stimulates insulin production)

	e_pa - exercise-production virtual modulator appearance
	e_ua - exercise-utilization virtual modulator appearance
	e_pe - exercise-production virtual modulator elimination rate
	e_ue - exercise-utilization virtual modulator elimination rate
	q_ep - glucose production modulation rate by exercise
	q_eu - glucose utilization modulation rate by exercise

	e_lta - long-term excercise effect virtual modulator appearance
	e_lte - long-term excercise effect virtual modulator elimination rate
	e_Si - long-term exercise insulin sensitivity coefficient

	Aq - CHO bioavailability (how many % of glucose from meal is absorbed); TODO: this should be a parameter of CHO intake
	t_d - meal absorption time; TODO: this should be a parameter of CHO intake
	t_i - subcutaneous insulin absorption time; TODO: this should be a parameter of insulin dosage
	t_id - bolus spreading - a fraction of a single bolus will be dosed every minute of this time interval
	*/

	struct TParameters {
		union {
			struct {
				// initial quantities
				double Q1_0, Q2_0, Qsc_0, I_0, Isc_0, X_0, D1_0;
				// patient quantity and base parameters
				double Vq, Vqsc, Vi, Q1b, Gthr, GIthr;
				// transfer parameters
				double q12, q1sc, ix, xq1, iscimod;
				// elimination parameters
				double q1e, q1ee, q1e_thr, xe;
				// production parameters
				double q1p, q1pe, q1pi, ip;
				// exercise-related parameters
				double e_pa, e_ua, e_pe, e_ue, q_ep, q_eu;
				// exercise-related long-term parameters
				double e_lta, e_lte, e_Si;
				// misc parameters
				double Ag, t_d, t_i, t_id;
			};
			double vector[model_param_count];
		};
	};

	const gct2_model::TParameters lower_bounds = { {{
		//	Q1_0, Q2_0, Qsc_0, I_0, Isc_0, X_0, D1_0,
			0,    0,    0,     0,   0,     0,   0,
		//	Vq,  Vqsc, Vi, Q1b, Gthr, GIthr,
			3,   1,    3,  50,  8.0,  4.0,
		//	q12,  q1sc, ix, xq1, iscimod
			0.5, 1.4,   1,  5,   0.1,
		//	q1e,   q1ee,  q1e_thr, xe,
			0.144, 0.144, 0.001,   0.1,
		//	q1p,     q1pe,   q1pi,    ip,
			0.00001, 0.0001, 0.00001, 0.0,
		//	e_pa, e_ua, e_pe, e_ue, q_ep, q_eu
			100,  100,  100,  100,  50,   50,
		//	e_lta, e_lte, e_Si
			1,     0.2,   0.02,
		//	Ag,  t_d,    t_i
			0.5, 10_min, 5_min,

		//	t_id
			1_min,
	}} };

	const gct2_model::TParameters default_parameters = { { {
		//	Q1_0, Q2_0, Qsc_0, I_0, Isc_0, X_0, D1_0,
			350,  65,   250,   0,   0,     0,   0,
		//	Vq,  Vqsc, Vi, Q1b, Gthr, GIthr,
			8,   5,    10, 240, 8.0, 5.0,
		//	q12, q1sc, ix, xq1, iscimod
			0.8, 8,    5,  45,  0.7,
		//	q1e,     q1ee,    q1e_thr, xe,
			0.38519, 0.38519, 0.8,     0.3,
		//	q1p,  q1pe, q1pi,  ip,
			0.01, 0.1,  0.001, 0.0,
		//	e_pa, e_ua, e_pe, e_ue, q_ep, q_eu
			200, 200, 342, 200, 180, 100,
		//	e_lta, e_lte, e_Si
			20,    0.5,   0.03,
		//	Ag,   t_d,    t_i
			0.98, 44_min, 60_min,

		//	t_id
			5_min,
	}} };

	const gct2_model::TParameters upper_bounds = { { {
		//	Q1_0, Q2_0, Qsc_0, I_0, Isc_0, X_0, D1_0,
			500,  500,  500,   500, 500,   500, 200,
		//	Vq,  Vqsc, Vi, Q1b,  Gthr, GIthr,
			10,  10,   20, 1000, 14.0, 8.0,
		//	q12, q1sc, ix,  xq1,   iscimod
			1.5, 24.0, 50,  120.0, 1.0,
		//	q1e,  q1ee, q1e_thr, xe,
			14.4, 14.4, 0.8,     2.0,
		//	q1p,  q1pe, q1pi, ip
			0.01, 0.1,  0.1,  0.05,
		//	e_pa, e_ua, e_pe, e_ue, q_ep, q_eu
			500,  500,  500,  500,  250,  250,
		//	e_lta, e_lte, e_Si
			50,    10,    0.1,
		//	Ag,   t_d,  t_i
			0.98, 2_hr, 5_hr,

		//	t_id
			2_hr,
	}} };
}

namespace gct3_model {

	constexpr GUID model_id = { 0xe8e30624, 0xc632, 0x44a1, { 0x8d, 0xdd, 0xed, 0xe2, 0x29, 0x93, 0x5, 0x70 } };					// {E8E30624-C632-44A1-8DDD-EDE229930570}

	constexpr GUID signal_IG = { 0x4111f7bc, 0x360c, 0x4bc5, { 0xbb, 0xd9, 0xda, 0xc, 0x78, 0x61, 0xcb, 0x49 } };					// {4111F7BC-360C-4BC5-BBD9-DA0C7861CB49}
	constexpr GUID signal_BG = { 0xe1b1847d, 0x7741, 0x4550, { 0xaa, 0xf2, 0x56, 0xca, 0x57, 0x71, 0x3a, 0x6 } };					// {E1B1847D-7741-4550-AAF2-56CA57713A06}
	constexpr GUID signal_Delivered_Insulin = { 0x8cd7d226, 0x4bbd, 0x49fc, { 0x9a, 0xe2, 0x86, 0x9, 0x4, 0xbc, 0xa0, 0x8d } };		// {8CD7D226-4BBD-49FC-9AE2-860904BCA08D}
	constexpr GUID signal_IOB = { 0x905b72fd, 0x54ff, 0x4592, { 0x92, 0x43, 0x4e, 0xfa, 0xcc, 0xd3, 0x4f, 0xde } };					// {905B72FD-54FF-4592-9243-4EFACCD34FDE}
	constexpr GUID signal_COB = { 0x60da8b75, 0x1195, 0x4d20, { 0x81, 0xb3, 0x3c, 0xed, 0x30, 0x18, 0xd1, 0xdf } };					// {60DA8B75-1195-4D20-81B3-3CED3018D1DF}

	constexpr size_t model_param_count = 44;

	/*
	Q1_0 - D1_0 - initial values/quantities

	Vq - distribution volume of glucose molecules in plasma ("volume of plasma")
	Vqsc - distribution volume of glucose molecules in subcutaneous tissue / interstitial fluid
	Vi - insulin distribution volume
	Q1b - basal glucose amount in primary distribution volume
	Gthr - glycosuria threshold [mmol/L]
	GIthr - insulin production glucose threshold (above this level, beta cells start to produce insulin) [mmol/L]

	q12 - transfer rate of diffusion between Q1 and Q2
	q1sc - transfer rate of diffusion between Q1 and Qsc
	ix - transfer rate I -> X
	xq1 - moderation rate Q1 -(X)-> sink
	iscimod - transfer rate Isc -> I, bundled with local degradation (proposed by Hovorka, http://doi.org/10.1109/TBME.2004.839639 )

	q1e - base elimination of Q1 glucose
	q1ee - glucose elimination moderated by exercise
	q1e_thr - glycosuria elimination of Q1 glucose (over threshold)
	xe - elimination rate of X by glucose utilization moderation (coupled with xq1)

	q1p - glucose appearance rate from glycogen and miscellanous sources
	q1pe - glucose appearance moderated by exercise effects
	q1pi - glucose appearance inhibition by insulin presence
	ip - insulin production factor (how glucose stimulates insulin production)

	e_pa - exercise-production virtual modulator appearance
	e_ua - exercise-utilization virtual modulator appearance
	e_pe - exercise-production virtual modulator elimination rate
	e_ue - exercise-utilization virtual modulator elimination rate
	q_ep - glucose production modulation rate by exercise
	q_eu - glucose utilization modulation rate by exercise

	e_lta - long-term excercise effect virtual modulator appearance
	e_lte - long-term excercise effect virtual modulator elimination rate
	e_Si - long-term exercise insulin sensitivity coefficient

	Aq - CHO bioavailability (how many % of glucose from meal is absorbed); TODO: this should be a parameter of CHO intake
	t_d - meal absorption time; TODO: this should be a parameter of CHO intake
	t_i - subcutaneous insulin absorption time; TODO: this should be a parameter of insulin dosage
	t_id - bolus spreading - a fraction of a single bolus will be dosed every minute of this time interval
	*/

	struct TParameters {
		union {
			struct {
				// initial quantities
				double Q1_0, Q2_0, Qsc_0, I_0, Isc_0, X_0, D1_0;
				// patient quantity and base parameters
				double Vq, Vqsc, Vi, Q1b, Gthr, GIthr;
				// transfer parameters
				double q12, q1sc, ix, xq1, iscimod;
				// elimination parameters
				double q1e, q1ee, q1e_thr, xe;
				// production parameters
				double q1p, q1pe, q1pi, ip;
				// exercise-related parameters
				double e_pa, e_ua, e_pe, e_ue, q_ep, q_eu;
				// exercise-related long-term parameters
				double e_lta, e_lte, e_Si;
				// misc parameters
				double Ag, t_d, t_i, t_id;
				// effect of IG on absorption
				double dqscm, iqscm;
				// circadian insulin response
				double ci_0, ci_1, ci_2;
			};
			double vector[model_param_count];
		};
	};

	const TParameters lower_bounds = { {{
		//	Q1_0, Q2_0, Qsc_0, I_0, Isc_0, X_0, D1_0,
			0,    0,    0,     0,   0,     0,   0,
		//	Vq,  Vqsc, Vi, Q1b, Gthr, GIthr,
			3,   2,    3,  50,  10.0,  4.0,
		//	q12,  q1sc, ix, xq1, iscimod
			0.5, 1.4,   1,  3,   0.9,
		//	q1e,   q1ee,  q1e_thr, xe,
			0.044, 0.044, 0.001,   0.1,
		//	q1p,     q1pe,   q1pi,    ip,
			0.00001, 0.0001, 0.00001, 0.0,
		//	e_pa, e_ua, e_pe, e_ue, q_ep, q_eu
			100,  100,  100,  100,  50,   50,
		//	e_lta, e_lte, e_Si
			1,     0.2,   0.02,
		//	Ag,  t_d,    t_i
			0.8, 10_min, 5_min,

		//	t_id
			1_min,
		//	dqscm, iqscm
			-1,    -1,
		//	ci_0, ci_1, ci_2
			-1,   -1,   -1
	}} };

	const TParameters default_parameters = { { {
		//	Q1_0, Q2_0, Qsc_0, I_0, Isc_0, X_0, D1_0,
			350,  65,   250,   0,   0,     0,   0,
		//	Vq,  Vqsc, Vi, Q1b, Gthr, GIthr,
			8,   5,    10, 240, 8.0, 5.0,
		//	q12, q1sc, ix, xq1, iscimod
			0.8, 8,    5,  45,  0.99,
		//	q1e,     q1ee,    q1e_thr, xe,
			0.38519, 0.38519, 0.8,     0.3,
		//	q1p,  q1pe, q1pi,  ip,
			0.01, 0.1,  0.001, 0.0,
		//	e_pa, e_ua, e_pe, e_ue, q_ep, q_eu
			200, 200, 342, 200, 180, 100,
		//	e_lta, e_lte, e_Si
			20,    0.5,   0.03,
		//	Ag,   t_d,    t_i
			0.98, 44_min, 60_min,

		//	t_id
			5_min,
		//	dqscm, iqscm
			0,     0,
		//	ci_0, ci_1, ci_2
			0,    0,    0,
	}} };

	const TParameters upper_bounds = { { {
		//	Q1_0, Q2_0, Qsc_0, I_0, Isc_0, X_0, D1_0,
			500,  500,  500,   500, 500,   500, 200,
		//	Vq,  Vqsc, Vi, Q1b,  Gthr, GIthr,
			10,  10,   20, 1000, 14.0, 8.0,
		//	q12, q1sc, ix,  xq1,   iscimod
			8,   80.0, 100, 120.0, 1.0,
		//	q1e,  q1ee, q1e_thr, xe,
			14.4, 14.4, 0.8,     3.0,
		//	q1p,  q1pe, q1pi, ip
			0.01, 0.1,  0.1,  0.05,
		//	e_pa, e_ua, e_pe, e_ue, q_ep, q_eu
			500,  500,  500,  500,  250,  250,
		//	e_lta, e_lte, e_Si
			50,    10,    0.1,
		//	Ag,   t_d,  t_i
			0.95, 2_hr, 3_hr,

		//	t_id
			2_hr,
		//	dqscm, iqscm
			1,     1,
		//	ci_0, ci_1, ci_2
			1,    1,    1
	}} };
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


extern "C" HRESULT IfaceCalling do_get_model_descriptors(scgms::TModel_Descriptor **begin, scgms::TModel_Descriptor **end);
extern "C" HRESULT IfaceCalling do_get_signal_descriptors(scgms::TSignal_Descriptor * *begin, scgms::TSignal_Descriptor * *end);
extern "C" HRESULT IfaceCalling do_get_filter_descriptors(scgms::TFilter_Descriptor **begin, scgms::TFilter_Descriptor **end);
extern "C" HRESULT IfaceCalling do_create_discrete_model(const GUID *model_id, scgms::IModel_Parameter_Vector *parameters, scgms::IFilter *output, scgms::IDiscrete_Model **model);
extern "C" HRESULT IfaceCalling do_create_filter(const GUID *id, scgms::IFilter *output, scgms::IFilter **filter);
