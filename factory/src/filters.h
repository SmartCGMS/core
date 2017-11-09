#pragma once

#include "../../../common/iface/FilterIface.h"

#include <vector>
#include <QtCore/QLibrary>
#include <QtCore/QString>

#include <vector>
#include <memory>

namespace imported {
	extern const char* rsCreate_Filter_Factory_Symbol;
	using TCreate_Filter_Factory = HRESULT(glucose::IFilter_Factory**);

	typedef struct {
		std::unique_ptr<QLibrary> library;	//we hate this pointer, but we have to - otherwise Qt won't let us to use it
		TCreate_Filter_Factory filter_factory;
	} TLibraryInfo;
}

class CFilter_Factories : public std::vector<glucose::IFilter_Factory*> {
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
	~CFilter_Factories();	//we need to free all known factories
	void load_libraries();
};

extern CFilter_Factories filter_factories;

#ifdef _WIN32
	extern "C" __declspec(dllexport)  HRESULT IfaceCalling get_filter_factories(glucose::IFilter_Factory **begin, glucose::IFilter_Factory **end);
#endif