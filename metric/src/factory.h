#include "../..\..\common\iface\SolverIface.h"
#include "../..\..\common\rtl\hresult.h"

extern "C" HRESULT IfaceCalling DoCreateMetricFactory(size_t factoryid, TMetricParameters params, IMetricFactory **factory);