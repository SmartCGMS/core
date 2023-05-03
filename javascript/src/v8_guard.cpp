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

#include "v8_guard.h"

#include "../../../common/rtl/FilesystemLib.h"
#include "../../../common/utils/string_utils.h"

std::unique_ptr<v8::Platform> CV8_Guard::sPlatform;
std::mutex CV8_Guard::sInit_Mtx;
size_t CV8_Guard::sInit_Ctr = 0;
size_t CV8_Guard::sInstance_Ctr = 0;

// do not initialize during library load, it may lead to some wrong assumptions by V8
// rather, every subsystem, that needs V8, creates its own CV8_Guard instance with init parameter == true
CV8_Guard gV8_Guard{ false };

CV8_Guard::CV8_Guard(bool init)
{
	std::unique_lock<std::mutex> lck(sInit_Mtx);

	mDisposed = false;

	sInstance_Ctr++;

	if (init)
	{
		if (sInit_Ctr == 0)
		{
                        const auto app_dir = Narrow_WString(Get_Application_Dir());
                        v8::V8::InitializeICUDefaultLocation(app_dir.c_str());
                        v8::V8::InitializeExternalStartupData(app_dir.c_str());
			sPlatform = v8::platform::NewDefaultPlatform();
			v8::V8::InitializePlatform(sPlatform.get());
			v8::V8::Initialize();

			// platform object is released during ShutdownPlatform call below
			sPlatform.release();
		}

		sInit_Ctr++;
	}
}

CV8_Guard::~CV8_Guard()
{
	Dispose();
}

void CV8_Guard::Dispose()
{
	std::unique_lock<std::mutex> lck(sInit_Mtx);

	if (mDisposed)
		return;

	mDisposed = true;

	sInstance_Ctr--;

	if (sInstance_Ctr == 0)
	{
		v8::V8::Dispose();
		//v8::V8::ShutdownPlatform(); // TODO: fix this; causes crash for very unclear reasons

		sInit_Ctr = 0;
	}
}
