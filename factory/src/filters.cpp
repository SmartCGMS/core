#include "filters.h"

#include "../../../common/rtl/FilesystemLib.h"

namespace imported {
	const char* rsGet_Filter_Descriptors = "do_get_filter_descriptors";
	const char* rsDo_Create_Filter = "do_create_filter";
	const char* rsDo_Create_Metric = "do_create_metric";
}

const wchar_t* rsSolversDir = L"filters";

extern "C" char __ImageBase;


CLoaded_Filters loaded_filters;


HRESULT IfaceCalling get_filter_descriptors(glucose::TFilter_Descriptor **begin, glucose::TFilter_Descriptor **end) {

	if (!loaded_filters.empty()) {
		*begin = &loaded_filters[0];
		*end = *begin + loaded_filters.size();
	}
	else
		*begin = *end = nullptr;

	

	return *begin != nullptr ? S_OK : S_FALSE;
}

HRESULT IfaceCalling create_filter(const GUID *id, glucose::IFilter_Pipe *input, glucose::IFilter_Pipe *output, glucose::IFilter **filter) {
	return loaded_filters.create_filter(id, input, output, filter);
}

HRESULT IfaceCalling create_metric(const glucose::TMetric_Parameters *parameters, glucose::IMetric **metric) {
	return loaded_filters.create_metric(parameters, metric);
}

void CLoaded_Filters::load_libraries() {
	const auto allFiles = List_Directory(Path_Append(Get_Application_Dir(), rsSolversDir));

	for (const auto& filepath : allFiles) {
		if (CDynamic_Library::Is_Library(filepath)) {				//just checks the extension
			imported::TLibraryInfo lib;

			lib.library = std::make_unique<CDynamic_Library>();
			lib.library->Set_Filename(filepath);

			if (lib.library->Load()) {
				lib.create_filter = reinterpret_cast<decltype(lib.create_filter)> (lib.library->Resolve(imported::rsDo_Create_Filter));
				lib.create_metric = reinterpret_cast<decltype(lib.create_metric)> (lib.library->Resolve(imported::rsDo_Create_Metric));

				bool lib_used = (lib.create_filter != nullptr) || (lib.create_metric != nullptr);

				{
					//try to load filter descriptions just once
					const glucose::TGet_Filter_Descriptors desc_func = reinterpret_cast<decltype(desc_func)> (lib.library->Resolve(imported::rsGet_Filter_Descriptors));

					glucose::TFilter_Descriptor *desc_begin, *desc_end;

					if ((desc_func) && (desc_func(&desc_begin, &desc_end) == S_OK)) {
						lib_used |= desc_begin != desc_end;
						std::copy(desc_begin, desc_end, std::back_inserter(*this));						
					}
				}

			
				if (lib_used)
					mLibraries.push_back(std::move(lib));
				else
					lib.library->Unload();

			}
		}
	}

}

