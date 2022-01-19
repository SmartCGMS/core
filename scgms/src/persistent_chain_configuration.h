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

#include "../../../common/iface/FilterIface.h"
#include "../../../common/rtl/referencedImpl.h"
#include "../../../common/rtl/FilesystemLib.h"
#include "../../../common/rtl/UILib.h"


#pragma warning( push )
#pragma warning( disable : 4250 ) // C4250 - 'class1' : inherits 'class2::member' via dominance 

class CPersistent_Chain_Configuration : public virtual scgms::IPersistent_Filter_Chain_Configuration, public virtual refcnt::internal::CVector_Container<scgms::IFilter_Configuration_Link*> {
protected:
	filesystem::path mFile_Path;
	std::wstring Get_Parent_Path() noexcept;
	void Advertise_Parent_Path() noexcept;
protected:
	const wchar_t* mDeferred_Magic_String_Prefix = L"$(deferred to ";
	const wchar_t* mDeferred_Magic_String_Postfix = L")";
	std::tuple<bool, std::wstring> Is_Deferred_Parameter(const wchar_t* str_value);
	std::tuple<HRESULT, std::wstring> Load_From_File(const wchar_t *path);
	HRESULT Save_To_File(const std::wstring &text, const wchar_t *path);
protected:
	wchar_t* Describe_GUID(const GUID& val, const scgms::NParameter_Type param_type, const scgms::CSignal_Description& signal_descriptors) const noexcept;
public:
	virtual ~CPersistent_Chain_Configuration() noexcept {};

	virtual HRESULT IfaceCalling add(scgms::IFilter_Configuration_Link** begin, scgms::IFilter_Configuration_Link** end) noexcept override;

	virtual HRESULT IfaceCalling Get_Parent_Path(refcnt::wstr_container** path) noexcept override final;
	virtual HRESULT IfaceCalling Set_Parent_Path(const wchar_t* parent_path) noexcept override final;
	virtual HRESULT IfaceCalling Set_Variable(const wchar_t* name, const wchar_t* value) noexcept override final;

	virtual HRESULT IfaceCalling Load_From_File(const wchar_t *file_path, refcnt::wstr_list *error_description) noexcept override final;
	virtual HRESULT IfaceCalling Load_From_Memory(const char *memory, const size_t len, refcnt::wstr_list *error_description) noexcept override final;
	virtual HRESULT IfaceCalling Save_To_File(const wchar_t *file_path, refcnt::wstr_list* error_description) noexcept override final;	
};

#pragma warning( pop )

extern "C" HRESULT IfaceCalling create_persistent_filter_chain_configuration(scgms::IPersistent_Filter_Chain_Configuration **configuration) noexcept;
