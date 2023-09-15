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

#include <scgms/rtl/FilterLib.h>
#include <scgms/rtl/referencedImpl.h>
#include <scgms/rtl/FilesystemLib.h>

#include "fileloader/Structures.h"
#include "fileloader/FormatAdapter.h"
#include "fileloader/Extractor.h"
#include "fileloader/file_format_rules.h"

#include <memory>
#include <thread>
#include <list>

#pragma warning( push )
#pragma warning( disable : 4250 ) // C4250 - 'class1' : inherits 'class2::member' via dominance

using TValue_Vector = std::vector<CMeasured_Values_At_Single_Time>;
// segment begin and end (indexes in given array/vector)
using TSegment_Limits = std::pair<size_t, size_t>;

/*
 * Filter class for loading and extracting file, and sending values to chain
 */
class CFile_Reader : public scgms::CBase_Filter {
protected:
	// original filename from configuration
	filesystem::path mFileName;
		
	double mMaximum_IG_Interval = 12.0 * scgms::One_Minute;	//maximum allowed interval between to IGs

	// do we need to send shutdown after last value?
	bool mShutdownAfterLast = false;
	// minimum values in segment
	size_t mMinimum_Required_IGs = 0;
	// require both BG values in a segment
	bool mRequire_BG = false;

	// reader thread
	std::unique_ptr<std::thread> mReaderThread;
	CFile_Format_Rules mFile_Format_Rules;	

	// reader main method
	void Run_Reader();	
	// send event to filter chain
	bool Send_Event(scgms::NDevice_Event_Code code, double device_time, uint64_t segment_id, const GUID& signalId = Invalid_GUID, const double value = 0.0, const std::wstring& winfo = L"");	
	// extracts files to value vector container
	TValue_Vector Extract();
	// resolves segments of given value vector
	std::list<TSegment_Limits> Resolve_Segments(const TValue_Vector& src) const;
	
protected:
	virtual HRESULT Do_Execute(scgms::UDevice_Event event) override final;
	HRESULT Do_Configure(scgms::SFilter_Configuration configuration, refcnt::Swstr_list& error_description) override final;
public:
	CFile_Reader(scgms::IFilter *output);
	virtual ~CFile_Reader();
};

#pragma warning( pop )
