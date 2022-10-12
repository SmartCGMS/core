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

#include "../../../common/rtl/FilterLib.h"
#include "../../../common/rtl/referencedImpl.h"
#include "../../../common/rtl/FilesystemLib.h"
#include "../../../common/utils/string_utils.h"

#include <map>
#include <iostream>
#include <iomanip>
#include <sstream>
#include <locale>

#pragma warning( push )
#pragma warning( disable : 4250 ) // C4250 - 'class1' : inherits 'class2::member' via dominance 

class CFilter_Parameter : public virtual scgms::IFilter_Parameter, public virtual refcnt::CReferenced {
protected:
	template <typename T>
	using TConvertor = T(*)(const std::wstring&, bool&);
protected:
	const scgms::NParameter_Type mType;
	const std::wstring mConfig_Name;
	filesystem::path mParent_Path;

	//the following SReferenced variables are not part of the union to prevent memory corruption
	refcnt::SReferenced<refcnt::wstr_container> mWChar_Container;
	
	//be aware that segment id and parameters can contain nested variable name
	std::vector<std::wstring> mArray_Vars;	//empty or a var name
	size_t mFirst_Array_Var_idx = std::numeric_limits<size_t>::max();

	refcnt::SReferenced<scgms::time_segment_id_container> mTime_Segment_ID;
	refcnt::SReferenced<scgms::IModel_Parameter_Vector> mModel_Parameters;

	union {	
		double dbl;
		int64_t int64;
		bool boolean;
		GUID guid;		
	} mData = {0.0};

	template <typename TSRC, typename TDST>
	HRESULT Get_Container(TSRC src, TDST dst) {
		if (src) {
			*dst = src.get();
			(*dst)->AddRef();
			return S_OK;
		}
		else {
			*dst = nullptr;
			return E_NOT_SET;
		}
	}

	template <typename D, typename C>
	HRESULT Update_Container_By_Vars(C& container, TConvertor<D> conv) {
		D *current, *end;
		HRESULT rc = container->get(&current, &end);
		if (rc == S_OK) {
			current += mFirst_Array_Var_idx;
			const size_t cnt = std::distance(current , end);
			for (size_t var_idx = mFirst_Array_Var_idx; var_idx < cnt; var_idx++) {
				if (!mArray_Vars[var_idx].empty()) {
					std::wstring str_val;
					std::tie(rc, str_val) = Evaluate_Variable(mArray_Vars[var_idx]);
					if (rc == S_OK) {
						bool valid = false;
						*current = conv(str_val, valid);
						
						if (!valid) {
							rc = E_INVALIDARG;
							break;
						}
					}					
				}

				current++;
			}
		}

		return rc;
	}
	

	template <typename T, typename getter=T(*)()>
	HRESULT Get_Value(T* value, getter get_val, TConvertor<T> conv, const T&sanity_val) {
		HRESULT rc = S_OK;
		if (mVariable_Name.empty()) {
			*value = get_val();
		}
		else {			
			std::wstring var_val;
			std::tie(rc, var_val) = Evaluate_Variable(mVariable_Name);

			if (Succeeded(rc)) {
				bool ok;
				*value = conv(var_val, ok);

				if (!ok) {
					rc = E_INVALIDARG;
					*value = sanity_val;
				}
			}
			else {
				*value = sanity_val; 				
			}
		}

		return rc;
	}	


	template <typename C, typename D>
	HRESULT Get_Container_With_All_Level_Vars_Evaluated(refcnt::SReferenced<C> source_container, C** destination_container, TConvertor<D> conv) {
		HRESULT rc = Evaluate_Container_Variables(source_container, conv);
		if (Succeeded(rc)) {
			rc = Get_Container<refcnt::SReferenced<C>, C**>(source_container, destination_container);
		}

		return rc;
	}

	template <typename C, typename D>
	HRESULT Evaluate_Container_Variables(refcnt::SReferenced<C> &source_container, TConvertor<D> conv) {
		//let's check whether str is or is not a variable
		if (!mVariable_Name.empty()) {
			//let us update re-parse the container

			std::wstring current_value;
			HRESULT local_rc = E_UAC_DISABLED;
			std::tie(local_rc, current_value) = Evaluate_Variable(mVariable_Name);
			if (!Succeeded(local_rc))
				return local_rc;

			if (!Parse_Container<C, D>(source_container, current_value, conv))
				return E_FAIL;

			return S_OK;

		}
		else if (source_container) {
			return Update_Container_By_Vars<D, refcnt::SReferenced<C>>(source_container, conv);
		}
		else
			return E_NOT_SET;
	}

	template <typename C, typename D>
	bool Parse_Container(refcnt::SReferenced<C> &container, const std::wstring &str, TConvertor<D>  conv) {		//returns true of valid
		auto new_container = refcnt::make_shared_reference_ext< refcnt::SReferenced<C>, C>(Parse_Array_String<D, C>(str, conv), false);
		container = std::move(new_container);
		return container.operator bool();
	}

	template <typename D, typename R>
	R* Parse_Array_String(const std::wstring &str, TConvertor<D>  conv) {
		if (str.empty() || (str.size()==0)) return nullptr;

		std::wstring effective_str{ str };	//wcstok modifies the input string		
		std::vector<D> values;
		mArray_Vars.clear();
		mFirst_Array_Var_idx = std::numeric_limits<size_t>::max();

		//std::wstring effective_str{ str };	//wcstok modifies the input string
		const wchar_t* delimiters = L" ";	//string of chars, which designate individual delimiters
		wchar_t* buffer = nullptr;
		wchar_t* str_val = wcstok_s(const_cast<wchar_t*>(effective_str.data()), delimiters, &buffer);

		size_t counter = 0;
		while (str_val != nullptr) {
			auto [is_var, var_name] = scgms::Is_Variable_Name(str_val);
			if (is_var) {
				//store the var's name
				mArray_Vars.push_back(var_name);
				values.push_back(0);	//actually, we don't care about the value here and zero should work always
				
				//and cache index of the first variable
				if (counter < mFirst_Array_Var_idx)
					mFirst_Array_Var_idx = counter;

			} else {
				mArray_Vars.push_back(std::wstring{});	//empty var name

				//and store the real value
				bool ok;				
				const D value = conv(str_val, ok);
				if (!ok) 
					return nullptr;
				values.push_back(value);
			}


			

			str_val = wcstok_s(nullptr, delimiters, &buffer);

			counter++;
		}

		
		//values converted and nested variable names noted
		//=> create and return its container
		R* obj = nullptr;
		if (Manufacture_Object<refcnt::internal::CVector_Container<D>, R>(&obj) == S_OK) {
			if (obj->set(values.data(), values.data() + values.size()) != S_OK) {
				obj->Release();
				obj = nullptr;
			};
		}

		return obj;
	}


	template <typename D, typename R>
	std::tuple<HRESULT, std::wstring>  Array_To_String(R* container, const bool read_interpreted) {		
		if (!container) 			
			return { E_UNEXPECTED, L"" };

		std::tuple<HRESULT, std::wstring> result{ E_UNEXPECTED, L"" };
		std::wstringstream converted;

		//unused keeps static analysis happy about creating an unnamed object
		auto unused = converted.imbue(std::locale(std::wcout.getloc(), new CDecimal_Separator<wchar_t>{ L'.' })); //locale takes ownership of dec_sep
		converted << std::setprecision(std::numeric_limits<long double>::digits10 + 1);

		bool not_empty = false;

		D* current, * end;
		if (container->get(&current, &end) == S_OK) {

			size_t var_idx = 0;
			for (auto iter = current; iter != end; iter++) {
				if (not_empty)
					converted << L" ";
				else
					not_empty = true;

				if ((var_idx < mArray_Vars.size()) && !mArray_Vars[var_idx].empty()) {
					if (read_interpreted) {
						auto [valid, str_val] = Evaluate_Variable(mArray_Vars[var_idx]);
						if (Succeeded(valid)) converted << str_val;
						else {
							std::get<0>(result) = E_NOT_SET;
							std::get<1>(result) = mArray_Vars[var_idx];
							return result;
						}
					}
					else
						converted << L"$(" << mArray_Vars[var_idx] << L")";
				}					
				else {
					converted << *iter;
				}
			}
		}

		std::get<0>(result) = S_OK;
		std::get<1>(result) = converted.str();
		return result;
	}

protected:
	std::tuple<HRESULT, std::wstring> to_string(bool read_interpreted);
protected:
	std::wstring mVariable_Name;
	std::map<std::wstring, std::wstring> mNon_OS_Variables;
	std::tuple<HRESULT, std::wstring> Evaluate_Variable(const std::wstring &var_name);
protected:
	std::wstring mDeferred_Path_Or_Var;	//wstring so that we can pass its c_str when Get_Deffered_File gets called
	const wchar_t* mDeferred_Magic_String_Prefix = L"$([[deferred to]]";
	const wchar_t* mDeferred_Magic_String_Postfix = L")";
	std::wstring Make_Absolute_Path(filesystem::path src_path);
	std::tuple<bool, std::wstring> Is_Deferred_Parameter(const wchar_t* str_value); //returns true/false and filepath if true
	std::wstring Resolve_Deferred_Path();
	std::tuple<HRESULT, std::wstring> Load_From_File(const wchar_t* path);
	HRESULT Save_To_File(const std::wstring& text, const wchar_t* path);
public:
	CFilter_Parameter(const scgms::NParameter_Type type, const wchar_t *config_name);
	virtual ~CFilter_Parameter() {};	

	//conversion
	bool from_string(const scgms::NParameter_Type desired_type, const wchar_t* str);

	virtual HRESULT IfaceCalling Get_Type(scgms::NParameter_Type *type) override final;
	virtual HRESULT IfaceCalling Get_Config_Name(wchar_t **config_name) override final;

	//read-write
	virtual HRESULT IfaceCalling Set_Variable(const wchar_t* name, const wchar_t* value) override final;

	virtual HRESULT IfaceCalling Get_WChar_Container(refcnt::wstr_container** wstr, BOOL read_interpreted) override final;
	virtual HRESULT IfaceCalling Set_WChar_Container(refcnt::wstr_container *wstr) override final;

	virtual HRESULT IfaceCalling Get_File_Path(refcnt::wstr_container** wstr) override final;
	virtual HRESULT IfaceCalling Set_Parent_Path(const wchar_t* parent_path) override final;

	virtual HRESULT IfaceCalling Get_Time_Segment_Id_Container(scgms::time_segment_id_container **ids) override final;
	virtual HRESULT IfaceCalling Set_Time_Segment_Id_Container(scgms::time_segment_id_container *ids) override final;

	virtual HRESULT IfaceCalling Get_Double(double *value) override final;
	virtual HRESULT IfaceCalling Set_Double(const double value) override final;

	virtual HRESULT IfaceCalling Get_Int64(int64_t *value) override final;
	virtual HRESULT IfaceCalling Set_Int64(const int64_t value) override final;

	virtual HRESULT IfaceCalling Get_Bool(BOOL *boolean) override final;
	virtual HRESULT IfaceCalling Set_Bool(const BOOL boolean) override final;

	virtual HRESULT IfaceCalling Get_GUID(GUID *id) override final;
	virtual HRESULT IfaceCalling Set_GUID(const GUID *id) override final;

	virtual HRESULT IfaceCalling Get_Model_Parameters(scgms::IModel_Parameter_Vector **parameters) override final;
	virtual HRESULT IfaceCalling Set_Model_Parameters(scgms::IModel_Parameter_Vector *parameters) override final;

	//management
	virtual HRESULT IfaceCalling Clone(scgms::IFilter_Parameter **deep_copy) override final;
public:
	static const std::wstring mUnused_Variable_Name;
};

#pragma warning( pop )

extern "C" HRESULT IfaceCalling create_filter_parameter(const scgms::NParameter_Type type, const wchar_t *config_name, scgms::IFilter_Parameter **parameter);
