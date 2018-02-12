#pragma once

#include "../../../common/iface/SolverIface.h"
#include "../../../common/iface/UIIface.h"
#include "../../../common/rtl/CLibrary.h"

#include <vector>

#include <vector>
#include <memory>

namespace imported {
	extern const char* rsCreate_Filter_Factory_Symbol;
	
	typedef struct {
		std::unique_ptr<CLibrary> library;	//we hate this pointer, but we have to - otherwise Qt won't let us to use it
		glucose::TCreate_Filter create_filter;
		glucose::TCreate_Metric create_metric;
	} TLibraryInfo;
}

class CLoaded_Filters : public std::vector<glucose::TFilter_Descriptor> {
protected:
protected:
	std::vector<imported::TLibraryInfo> mLibraries;


	template <typename functype, typename... Args>
	HRESULT CallFunc(functype funcegetter, Args... args) const {
		for (auto &iter : mLibraries) {
			auto funcptr = funcegetter(iter);
			if (funcptr != nullptr) {
				if ((funcptr)(args...) == S_OK)
					return S_OK;
			}
		}
		return E_NOTIMPL;
	}
public:	
	void load_libraries();

	HRESULT create_filter(const GUID *id, glucose::IFilter_Pipe *input, glucose::IFilter_Pipe *output, glucose::IFilter **filter) {
		auto call_create_filter = [](const imported::TLibraryInfo &info) { return info.create_filter; }; //actually, we do offsetof & co., but in a much cleaner and in-lineable way
		return CallFunc(call_create_filter, id, input, output, filter);
	}

	HRESULT create_metric(const glucose::TMetric_Parameters *parameters, glucose::IMetric **metric) {
		auto call_create_filter = [](const imported::TLibraryInfo &info) { return info.create_metric  ; }; //actually, we do offsetof & co., but in a much cleaner and in-lineable way
		return CallFunc(call_create_filter, parameters, metric);
	}
};

extern CLoaded_Filters loaded_filters;

#ifdef _WIN32
	extern "C" __declspec(dllexport)  HRESULT IfaceCalling get_filter_descriptors(glucose::TFilter_Descriptor **begin, glucose::TFilter_Descriptor **end);
	extern "C" __declspec(dllexport)  HRESULT IfaceCalling create_filter(const GUID *id, glucose::IFilter_Pipe *input, glucose::IFilter_Pipe *output, glucose::IFilter **filter);	
	extern "C" __declspec(dllexport)  HRESULT IfaceCalling create_metric(const glucose::TMetric_Parameters *parameters, glucose::IMetric **metric);
#endif