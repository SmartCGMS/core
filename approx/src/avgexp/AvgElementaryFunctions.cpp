#include "AvgElementaryFunctions.h"


double ComputeExpK(const TGlucoseLevel& lpt, const TGlucoseLevel& rpt) {

#ifdef _DEBUG
	_ASSERT((rpt.level > 0.0) && (lpt.level > 0.0));
#endif

	double delta = rpt.datetime - lpt.datetime;
	double k;
	if (delta != 0.0) k = log(rpt.level / lpt.level) / delta;
		else k = 0.0;	//seems like there are two levels at the same time

	//double k = (rpt.level - lpt.level) / (rpt.datetime - lpt.datetime);	

#ifdef _DEBUG
	_ASSERT(!isnan(k));
#endif

	return k;
}

double ComputeExpKX(const TAvgExpPoint& lpt, const double rx) {
	double ry = lpt.pt.level*exp(lpt.k*(rx - lpt.pt.datetime));

	//double ry = (rx - lpt->pt.datetime)*lpt->k + lpt->pt.level;

#ifdef _DEBUG
	_ASSERT(!isnan(ry));
#endif

	return ry;
}

double ComputeLineK(const TGlucoseLevel& lpt, const TGlucoseLevel& rpt) {

#ifdef _DEBUG
	_ASSERT((rpt.level > 0.0) && (lpt.level > 0.0));
#endif
	
	double delta = rpt.datetime - lpt.datetime;
	double k;
	if (delta != 0) k = (rpt.level - lpt.level) / delta;
		else k = 0;

	return k;
}

double ComputeLineKX(const TAvgExpPoint& lpt, const double rx) {
	double ry = lpt.k*(rx - lpt.pt.datetime) + lpt.pt.level;

	return ry;
}


const TAvgElementary AvgExpElementary = { -489.6, -489.0 , ComputeExpK, ComputeExpKX};
const TAvgElementary AvgLineElementary = { -489.6, -489.0, ComputeLineK, ComputeLineKX };