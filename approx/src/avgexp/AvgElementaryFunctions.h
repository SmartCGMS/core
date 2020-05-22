#pragma once

#include "../../../../common/iface/ApproxIface.h"

#include <vector>

typedef struct {
	double datetime;		//time of measuring
	double level;		//the glucose level/concetration measured
} TGlucoseLevel;


typedef struct {
	TGlucoseLevel pt;
	double k;
} TAvgExpPoint;

using TAvgExpVector = std::vector<TAvgExpPoint>;

/*
	These are the functions, which will will apply to the original AvgExp approxmation.
*/

using TComputeExpK = double (*)(const TGlucoseLevel&, const TGlucoseLevel&);
using TComputeExpKX = double(*)(const TAvgExpPoint&, const double);

struct TAvgElementary {
	double steepDownRatio;
	double steepUpRatio;
	TComputeExpK ComputeExpK;
	TComputeExpKX ComputeExpKX;
} ;

extern const TAvgElementary AvgExpElementary;
extern const TAvgElementary AvgLineElementary;