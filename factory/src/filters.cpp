#include "filters.h"


namespace imported {
	const char* rsCreate_Filter_Factory_Symbol = "DoCreateFilterFactory";
}

const char* rsSolversDir = "solvers";

extern "C" char __ImageBase;


CFilter_Factories filter_factories;


HRESULT IfaceCalling get_filter_factories(glucose::IFilter_Factory **begin, glucose::IFilter_Factory **end) {

	if (!filter_factories.empty()) {
		*begin = filter_factories[0];
		*end = *begin + filter_factories.size();
	}
	else
		*begin = *end = nullptr;

	

	return S_OK;
}

void CFilter_Factories::load_libraries() {	

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
			TLibraryInfo lib;

			QString fullpath = filepath + QDir::separator() + filename;
			QFileInfo fileInfo(fullpath);
			lib.library = std::make_unique<QLibrary>(); // (new QLibrary()); //QDir::toNativeSeparators(fileInfo.absoluteFilePath());			
			lib.library->setFileName(fileInfo.canonicalFilePath());	//will not work without this

			if (lib.library->load()) {
				lib.solver = reinterpret_cast<decltype(lib.solver)> (lib.library->resolve(rsCreateSolverFuncName));
				lib.calculation = reinterpret_cast<decltype(lib.calculation)> (lib.library->resolve(rsCreateCalculationFuncName));
				lib.metric = reinterpret_cast<decltype(lib.metric)> (lib.library->resolve(rsCreateMetricFuncName));


				{
					//try to load model descriptions just once
					const TGetModelDescription desc_func = reinterpret_cast<decltype(desc_func)> (lib.library->resolve(rsGetModelDescriptionFuncName));
					size_t desc_count;
					TUIModel_Description *desc;
					if ((desc_func) && (desc_func(&desc_count, &desc) == S_OK)) {
						std::copy(desc, desc + desc_count, std::back_inserter(mDescribed_Models));
					}
				}

				{
					//try to load solver descriptions just once
					const TGetSolverDescription desc_func = reinterpret_cast<decltype(desc_func)> (lib.library->resolve(rsGetSolverDescription));
					size_t desc_count;
					TUISolver_Method_Description *desc;
					if ((desc_func) && (desc_func(&desc_count, &desc) == S_OK)) {
						std::copy(desc, desc + desc_count, std::back_inserter(mDescribed_Solvers));
					}
				}

				if ((lib.solver != nullptr) || (lib.metric != nullptr) || (lib.calculation != nullptr)) mLibraries.push_back(std::move(lib));
				else lib.library->unload();

			}
		}
	}

}