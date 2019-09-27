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
 * Univerzitni 8
 * 301 00, Pilsen
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

#include <vector>



static constexpr GUID mtrAvg_Abs =	//arithmetic average absolute error
{ 0xd272a84d, 0x50ff, 0x46ce,{ 0x97, 0x7e, 0xc8, 0xe3, 0x68, 0xc3, 0x70, 0x6a } }; 	// {D272A84D-50FF-46CE-977E-C8E368C3706A}

static constexpr GUID mtrMax_Abs =		//maximum absolute error
{ 0x70a34eac, 0xac0a, 0x424f,{ 0xa3, 0x2c, 0x20, 0xfd, 0x51, 0x7b, 0xec, 0xe6 } };	// {70A34EAC-AC0A-424F-A32C-20FD517BECE6}

static constexpr GUID mtrPerc_Abs =		////error at percentil given by TMetricParameters.Threshold (e.g. 25% is 25.0)
{ 0x3c3b19d, 0x4a2, 0x4195,{ 0x8a, 0x77, 0x7f, 0xed, 0xa7, 0x8a, 0x95, 0x39 } };	// {03C3B19D-04A2-4195-8A77-7FEDA78A9539}

static constexpr GUID mtrThresh_Abs =	//number of levels with error greater than TMetricParameters.Threshold (e.g. 20% is 20.0)
{ 0x9ca7aa77, 0xc84b, 0x4998,{ 0xbd, 0x8f, 0xd, 0xe0, 0x9, 0x67, 0xb0, 0x8e } };	// {9CA7AA77-C84B-4998-BD8F-0DE00967B08E}

static constexpr GUID mtrLeal_2010 =	//adapted best fit from Yenny Leal at al. Real-Time Glucose Estimation Algorithm for Continuous Glucose Monitoring Using Autoregressive Models
{ 0xe6d43190, 0x4317, 0x4201,{ 0x87, 0x6a, 0xf, 0xe4, 0xfe, 0x6b, 0x9, 0x90 } };	// {E6D43190-4317-4201-876A-0FE4FE6B0990}

static constexpr GUID mtrAIC =			//Akaike Information Criterion
{ 0x56286dd1, 0x6d95, 0x4c29,{ 0x9c, 0xda, 0x2b, 0x78, 0xbf, 0x7e, 0x5d, 0x16 } };	// {56286DD1-6D95-4C29-9CDA-2B78BF7E5D16}

static constexpr GUID mtrStd_Dev =	//standard deviation, TMetricParameters.Threshold gives percentils (e.g. 5% is 5.0) to cut off from the beginning and the end of sorted differences
{ 0x6c8b6358, 0xdf3e, 0x45e4,{ 0xae, 0x39, 0x9a, 0x55, 0x34, 0x7d, 0xa5, 0x85 } };	// {6C8B6358-DF3E-45E4-AE39-9A55347DA585}

static constexpr GUID mtrCrosswalk =	// the Croswalk - time-order metric from EMBEC 2017
{ 0x1feed1ce, 0x4ab3, 0x42be,{ 0x83, 0x34, 0x77, 0x46, 0x80, 0x27, 0xf, 0x14 } };	// {1FEED1CE-4AB3-42BE-8334-774680270F14}

static constexpr GUID mtrIntegral_CDF =	//integrates the area beneath cumulative distribution function of error - replaces the standard Area Under the Curve
{ 0x7b869d40, 0xb8a5, 0x4109,{ 0x87, 0xac, 0xe4, 0xd9, 0xd7, 0x2b, 0x99, 0x8a } };	// {7B869D40-B8A5-4109-87AC-E4D9D72B998A}

static constexpr GUID mtrAvg_Plus_Bessel_Std_Dev =	//average plus standard deviation with Bessel's correction
{ 0x5faaa53b, 0x2e58, 0x4e0e,{ 0x8b, 0xd0, 0x2c, 0x79, 0xde, 0x90, 0xfe, 0xbb } };	// {5FAAA53B-2E58-4E0E-8BD0-2C79DE90FEBB}



extern "C" HRESULT IfaceCalling do_get_metric_descriptors(glucose::TMetric_Descriptor **begin, glucose::TMetric_Descriptor **end);

extern "C" HRESULT IfaceCalling do_get_filter_descriptors(glucose::TFilter_Descriptor **begin, glucose::TFilter_Descriptor **end);

extern "C" HRESULT IfaceCalling do_create_filter(const GUID *id, glucose::IFilter *output, glucose::IFilter **filter);
