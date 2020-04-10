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

#include "v8_support.h"

#ifdef WIN32
#pragma comment(lib, "secur32.lib")
#pragma comment(lib, "winmm.lib")
#pragma comment(lib, "dmoguids.lib")
#pragma comment(lib, "wmcodecdspuuid.lib")
#pragma comment(lib, "msdmo.lib")
#pragma comment(lib, "Strmiids.lib")
#pragma comment(lib, "dbghelp.lib")
#endif

#include <string>
#include <iostream>

#include "../../../common/rtl/FilesystemLib.h"
#include "../../../common/utils/string_utils.h"

namespace v8_support
{
	v8::MaybeLocal<v8::String> Read_File(v8::Isolate* isolate, const std::wstring& name)
	{
		std::wifstream t(name);
		if (!t.is_open())
			return v8::MaybeLocal<v8::String>();
		std::wstring contents((std::istreambuf_iterator<wchar_t>(t)), std::istreambuf_iterator<wchar_t>());

		const std::string narrowed_contents = Narrow_WString(contents);

		v8::MaybeLocal<v8::String> result = v8::String::NewFromUtf8(isolate, narrowed_contents.c_str(), v8::NewStringType::kNormal, static_cast<int>(narrowed_contents.size()));
		return result;
	}

	v8::MaybeLocal<v8::Value> Execute_String_Unchecked(v8::Isolate* isolate, v8::Local<v8::String> source, v8::Local<v8::Value> name)
	{
		v8::HandleScope handle_scope(isolate);
		v8::TryCatch try_catch(isolate);
		v8::ScriptOrigin origin(name);
		v8::Local<v8::Context> context(isolate->GetCurrentContext());
		v8::Local<v8::Script> script;

		if (!v8::Script::Compile(context, source, &origin).ToLocal(&script))
			return v8::MaybeLocal<v8::Value>();

		v8::Local<v8::Value> result;
		if (!script->Run(context).ToLocal(&result))
			return v8::MaybeLocal<v8::Value>();

		return result;
	}

	CExecution_Environment::CExecution_Environment()
	{
		//
	}

	CExecution_Environment::~CExecution_Environment()
	{
		mContext.Empty();
		mGlobal.Empty();
		mIsolate->Dispose();
	}

	bool CExecution_Environment::Initialize()
	{
		mCreate_Params.array_buffer_allocator = v8::ArrayBuffer::Allocator::NewDefaultAllocator();

		mIsolate = v8::Isolate::New(mCreate_Params);

		// context create scope
		{
			v8::Locker locker(mIsolate);

			v8::Isolate::Scope isolate_scope(mIsolate);
			v8::HandleScope handle_scope(mIsolate);

			v8::Local<v8::ObjectTemplate> global = v8::ObjectTemplate::New(mIsolate);
			global->Set(v8::String::NewFromUtf8Literal(mIsolate, "require"), v8::FunctionTemplate::New(mIsolate, v8_nodejs::Require));
			mGlobal.Reset(mIsolate, global);

			v8::Local<v8::Context> context = v8::Context::New(mIsolate, nullptr, global);
			mContext.Reset(mIsolate, context);
		}

		return true;
	}

	bool CExecution_Environment::Run_And_Fetch(const std::string& program, std::string& target)
	{
		v8::Locker isolateLocker(mIsolate);
		v8::Isolate::Scope isolate_scope(mIsolate);
		v8::HandleScope handle_scope(mIsolate);

		auto vresult = Run_Program(program);

		if (vresult.IsEmpty())
			return false;

		target = std::string{ *v8::String::Utf8Value(mIsolate, vresult) };

		return true;
	}

	bool CExecution_Environment::Run_And_Fetch(const std::string& program, std::vector<double>& target)
	{
		v8::Locker isolateLocker(mIsolate);
		v8::Isolate::Scope isolate_scope(mIsolate);
		v8::HandleScope handle_scope(mIsolate);

		auto vresult = Run_Program(program);

		if (vresult.IsEmpty())
			return false;

		auto ares = v8::Local<v8::Array>::Cast(vresult);

		auto len = ares->Length();

		double val;
		target.clear();
		for (decltype(len) i = 0; i < len; i++) {

			try
			{
				val = ares->Get(mIsolate->GetCurrentContext(), i).ToLocalChecked()->ToNumber(mIsolate->GetCurrentContext()).ToLocalChecked()->Value();
			}
			catch (...)
			{
				val = std::numeric_limits<double>::quiet_NaN();
			}

			target.push_back(val);
		}

		return true;
	}

	bool ConvertFromV8(v8::Isolate* isolate, v8::Local<v8::Value> input,
		std::string* result) {
		v8::String::Utf8Value utf8(isolate, input);
		*result = *utf8;
		return true;
	}

	std::string V8ToString(v8::Isolate* isolate, v8::Local<v8::Value> value) {
		if (value.IsEmpty())
			return std::string();
		std::string result;
		if (!ConvertFromV8(isolate, value, &result))
			return std::string();
		return result;
	}

	v8::Local<v8::String> GetSourceLine(v8::Isolate* isolate,
		v8::Local<v8::Message> message) {
		auto maybe = message->GetSourceLine(isolate->GetCurrentContext());
		v8::Local<v8::String> source_line;
		return maybe.ToLocal(&source_line) ? source_line : v8::String::Empty(isolate);
	}


	v8::Local<v8::Value> CExecution_Environment::Run_Program(const std::string& str, const std::string& origin_name)
	{
		v8::TryCatch try_catch(mIsolate);

		v8::Local<v8::Context> context(mContext.Get(mIsolate));

		v8::Context::Scope context_scope(context);

		v8::Local<v8::String> source = v8::String::NewFromUtf8(mIsolate, str.c_str()).ToLocalChecked();

		v8::Local<v8::String> name = v8::String::NewFromUtf8(mIsolate, origin_name.c_str()).ToLocalChecked();
		v8::ScriptOrigin origin(name);
		v8::Local<v8::Script> script;

		if (!v8::Script::Compile(context, source, &origin).ToLocal(&script) || try_catch.HasCaught())
		{
			std::wstring exception;

			v8::Local<v8::Message> message = try_catch.Message();
			exception += Widen_Char((V8ToString(mIsolate, message->Get()) + "\n" + V8ToString(mIsolate, GetSourceLine(mIsolate, message))).c_str());

			v8::Local<v8::StackTrace> trace = message->GetStackTrace();
			if (!trace.IsEmpty())
			{
				int len = trace->GetFrameCount();
				for (int i = 0; i < len; ++i) {
					v8::Local<v8::StackFrame> frame = trace->GetFrame(mIsolate, i);
					exception += Widen_Char(V8ToString(mIsolate, frame->GetScriptName()).c_str()) + L":"
						+ std::to_wstring(frame->GetLineNumber()) + L":" + std::to_wstring(frame->GetColumn()) + L": "
						+ Widen_Char(V8ToString(mIsolate, frame->GetFunctionName()).c_str()) + L"\n";
				}
			}

			std::wcout << exception << std::endl;

			/*std::wstring exception;
			v8::String::Utf8Value utf8(mIsolate, try_catch.Exception());
			exception = Widen_Char(*utf8);
			std::wcout << *utf8 << std::endl;

			auto trace = v8::StackTrace::CurrentStackTrace(mIsolate, 64);
			if (!trace.IsEmpty())
			{
				v8::String::Utf8Value utf8_2(mIsolate, trace.);
				exception += L", " + Widen_Char(*utf8_2);
				std::wcout << *utf8_2 << std::endl;
			}*/

			/*v8::Local<v8::Value> strace;
			if (!try_catch.StackTrace(context).IsEmpty() && try_catch.StackTrace(context).ToLocal(&strace))
			{
				v8::String::Utf8Value utf8_2(mIsolate, strace);
				exception += L", " + Widen_Char(*utf8_2);
			}*/

			mPending_Exceptions.push_back(exception);
		}
		else
		{
			v8::Local<v8::Value> result;
			if (!script->Run(context).ToLocal(&result))
			{
				std::wstring exception;
				v8::String::Utf8Value utf8(mIsolate, try_catch.Exception());
				exception = Widen_Char(*utf8);

				/*auto trace = v8::StackTrace::CurrentStackTrace(mIsolate, 64);
				if (!trace.IsEmpty())
				{
					v8::String::Utf8Value utf8_2(mIsolate, trace);
					exception += L", " + Widen_Char(*utf8_2);
				}*/

				/*v8::Local<v8::Value> strace;
				if (!try_catch.StackTrace(context).IsEmpty() && try_catch.StackTrace(context).ToLocal(&strace))
				{
					v8::String::Utf8Value utf8_2(mIsolate, strace);
					exception += L", " + Widen_Char(*utf8_2);
				}*/

				mPending_Exceptions.push_back(exception);
			}
			else
				return result;
		}

		return {};
	}

	v8::Local<v8::Value> CExecution_Environment::Locked_Run_Program(const std::string& str, const std::string& origin_name)
	{
		v8::Locker isolateLocker(mIsolate);
		v8::Isolate::Scope isolate_scope(mIsolate);
		v8::HandleScope handle_scope(mIsolate);

		return Run_Program(str, origin_name);
	}

	v8::Local<v8::Value> CExecution_Environment::Run_Program_From_File(const std::string& path)
	{
		std::ifstream t(path);
		std::string str((std::istreambuf_iterator<char>(t)), std::istreambuf_iterator<char>());

		return Run_Program(str, path);
	}

	v8::Local<v8::Value> CExecution_Environment::Locked_Run_Program_From_File(const std::string& path)
	{
		std::ifstream t(path);
		std::string str((std::istreambuf_iterator<char>(t)), std::istreambuf_iterator<char>());

		return Locked_Run_Program(str, path);
	}

	const std::list<std::wstring>& CExecution_Environment::Get_Exceptions_List() const
	{
		return mPending_Exceptions;
	}

	void CExecution_Environment::Flush_Exceptions()
	{
		mPending_Exceptions.clear();
	}
}

namespace v8_nodejs
{
	void Require(const v8::FunctionCallbackInfo<v8::Value>& args)
	{
		if (args.Length() != 1)
			return;

		v8::HandleScope handle_scope(args.GetIsolate());

		const auto arg0 = args[0];
		v8::String::Utf8Value filename(args.GetIsolate(), arg0);

		if (!*filename)
		{
			std::string exception{ "require(): Invalid filename" };
			args.GetIsolate()->ThrowException(v8::String::NewFromUtf8(args.GetIsolate(), exception.c_str()).ToLocalChecked());
			return;
		}

		std::wstring filename_w = Widen_Char(*filename);

		auto basefilename = v8::StackTrace::CurrentStackTrace(args.GetIsolate(), 1, v8::StackTrace::kScriptName)->GetFrame(args.GetIsolate(), 0)->GetScriptName();
		std::string basefn{ *v8::String::Utf8Value(args.GetIsolate(), basefilename) };

		std::filesystem::path base;
		std::filesystem::path scriptpath{ filename_w };

		if (scriptpath.is_relative())
		{
			if (basefn != "unnamed" && !basefn.empty())
			{
				base = basefn;
				base = base.parent_path();
			}
			else
			{
				base = Get_Application_Dir();
			}
			base /= filename_w;
			base = base.make_preferred();
			base = std::filesystem::absolute(base);
		}
		else
		{
			base = scriptpath.make_preferred();
			base = std::filesystem::absolute(base);
		}

		filename_w = base.wstring();
		if (!base.has_extension())
			filename_w += L".js";

		v8::Local<v8::String> source;
		if (!v8_support::Read_File(args.GetIsolate(), filename_w).ToLocal(&source))
		{
			std::string exception{ "require(): could not load file " };
			exception += Narrow_WString(filename_w);

			args.GetIsolate()->ThrowException(v8::String::NewFromUtf8(args.GetIsolate(), exception.c_str()).ToLocalChecked());
			return;
		}

		auto result = v8_support::Execute_String_Unchecked(args.GetIsolate(), source, arg0);

		v8::Local<v8::Value> result_local;

		if (result.IsEmpty() || !result.ToLocal(&result_local))
		{
			std::string exception{ "require(): could not execute file " };
			exception += Narrow_WString(filename_w);

			args.GetIsolate()->ThrowException(v8::String::NewFromUtf8(args.GetIsolate(), exception.c_str()).ToLocalChecked());
			return;
		}

		args.GetReturnValue().Set(result_local);
	}
}
