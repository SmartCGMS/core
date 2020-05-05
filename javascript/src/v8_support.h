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

#include <libplatform/libplatform.h>
#include <v8.h>

#include <string>
#include <list>

#include "v8_guard.h"

namespace v8_support
{
	v8::MaybeLocal<v8::String> Read_File(v8::Isolate* isolate, const std::wstring& name);
	v8::MaybeLocal<v8::Value> Execute_String_Unchecked(v8::Isolate* isolate, v8::Local<v8::String> source, v8::Local<v8::Value> name);

	class CExecution_Environment
	{
		private:
			// initialize V8 with class instance, if needed
			CV8_Guard mV8_Guard{ true };

			v8::Isolate* mIsolate;
			v8::Global<v8::Context> mContext;
			v8::Global<v8::ObjectTemplate> mGlobal;
			v8::Isolate::CreateParams mCreate_Params;

			std::list<std::wstring> mPending_Exceptions;

		public:
			CExecution_Environment();
			virtual ~CExecution_Environment();

			bool Initialize();

			v8::Local<v8::Value> Locked_Run_Program(const std::string& str, const std::string& origin_name = "unnamed");
			v8::Local<v8::Value> Run_Program(const std::string& str, const std::string& origin_name = "unnamed");
			v8::Local<v8::Value> Locked_Run_Program_From_File(const std::string& path);
			v8::Local<v8::Value> Run_Program_From_File(const std::string& path);

			const std::list<std::wstring>& Get_Exceptions_List() const;
			void Flush_Exceptions();

			// the following methods are specialized to what we really need

			bool Run_And_Fetch(const std::string& program, std::string& target);
			bool Run_And_Fetch(const std::string& program, std::vector<double>& target);
	};
}

namespace v8_nodejs
{
	void Require(const v8::FunctionCallbackInfo<v8::Value>& args);
}
