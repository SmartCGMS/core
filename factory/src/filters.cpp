#include "filters.h"

#include <QtCore/QDir>
#include <QtCore/QFileInfo>

namespace imported {
	const char* rsGet_Filter_Descriptors = "do_get_filter_descriptors";
	const char* rsDo_Create_Filter = "do_create_filter";
}

const char* rsSolversDir = "filters";

extern "C" char __ImageBase;


CLoaded_Filters loaded_filters;


HRESULT IfaceCalling get_filter_descriptors(glucose::TFilter_Descriptor **begin, glucose::TFilter_Descriptor **end) {

	if (!loaded_filters.empty()) {
		*begin = &loaded_filters[0];
		*end = *begin + loaded_filters.size();
	}
	else
		*begin = *end = nullptr;

	

	return S_OK;
}

HRESULT IfaceCalling create_filter(const GUID *id, glucose::IFilter_Pipe *input, glucose::IFilter_Pipe *output, glucose::IFilter **filter) {
	return loaded_filters.create_filter(id, input, output, filter);
}

void CLoaded_Filters::load_libraries() {

	const size_t bufsize = 1024;
	char SolverFileName[bufsize];
	GetModuleFileNameA(((HINSTANCE)&__ImageBase), SolverFileName, bufsize);
	QString SolverQFileName(SolverFileName);

	QFileInfo solver_path(SolverQFileName);
	
	QString filepath = solver_path.absoluteDir().absolutePath() + QDir::separator() + QString(rsSolversDir);

	QDir dlldir(filepath);
	QStringList allFiles = dlldir.entryList(QDir::NoDotAndDotDot | QDir::System | QDir::Hidden | QDir::Files, QDir::NoSort);

	foreach(const QString &filename, allFiles) {
		if (QLibrary::isLibrary(filename)) {				//just checks the extension
			imported::TLibraryInfo lib;

			QString fullpath = filepath + QDir::separator() + filename;
			QFileInfo fileInfo(fullpath);
			lib.library = std::make_unique<QLibrary>(); // (new QLibrary()); //QDir::toNativeSeparators(fileInfo.absoluteFilePath());			
			lib.library->setFileName(fileInfo.canonicalFilePath());	//will not work without this

			if (lib.library->load()) {
				lib.create_filter = reinterpret_cast<decltype(lib.create_filter)> (lib.library->resolve(imported::rsDo_Create_Filter));

				bool lib_used = lib.create_filter != nullptr;

				{
					//try to load filter descriptions just once
					const glucose::TGet_Filter_Descriptors desc_func = reinterpret_cast<decltype(desc_func)> (lib.library->resolve(imported::rsGet_Filter_Descriptors));

					glucose::TFilter_Descriptor *desc_begin, *desc_end;

					if ((desc_func) && (desc_func(&desc_begin, &desc_end) == S_OK)) {
						lib_used |= desc_begin != desc_end;
						std::copy(desc_begin, desc_end, std::back_inserter(*this));
					}
				}

			
				if (lib_used) mLibraries.push_back(std::move(lib));
					else lib.library->unload();

			}
		}
	}

}

