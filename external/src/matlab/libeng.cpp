#include "libeng.h"

#include <windows.h>

#include "..\PacketStructures.h"

#include <cmath>

#include <stdio.h>

#include "matrix.h"
#pragma comment(lib, "..\\src\\matlab\\libmx_win64_ms.lib")

mxArray* modelIO = nullptr;

double basalInsulin = 0;
double bolusInsulin = 0;
double carbs = 0;

HANDLE file_DataToSmartCGMS;
void* filebuf_DataToSmartCGMS;
HANDLE file_DataFromSmartCGMS;
void* filebuf_DataFromSmartCGMS;
HANDLE event_DataToSmartCGMS;
HANDLE event_DataFromSmartCGMS;

mxArray* getv__smartcgms_settings()
{
	mxArray *requiresSqInsulinSupport, *requiresSqGlucoseSupport, *requiresInsulinDosingSupport, *requiresCPeptideSupport, *requiresSqGlucagonSupport, *numSignals, *signalDescription;
	char* fieldnames[7];

	requiresSqInsulinSupport = mxCreateLogicalScalar(true);
	requiresSqGlucoseSupport = mxCreateLogicalScalar(false);
	requiresInsulinDosingSupport = mxCreateLogicalScalar(false);
	requiresCPeptideSupport = mxCreateLogicalScalar(false);
	requiresSqGlucagonSupport = mxCreateLogicalScalar(false);
	numSignals = mxCreateDoubleScalar(2);
	signalDescription = mxCreateCellMatrix(2, 1);

	mxArray* str;
	str = mxCreateString("IVBG");
	mxSetCell(signalDescription, 0, mxDuplicateArray(str));
	mxDestroyArray(str);
	str = mxCreateString("CGM");
	mxSetCell(signalDescription, 1, mxDuplicateArray(str));
	mxDestroyArray(str);

	for (size_t i = 0; i < 7; i++)
		fieldnames[i] = (char*)mxMalloc(50);

	memcpy(fieldnames[0], "requiresSqInsulinSupport", sizeof("requiresSqInsulinSupport"));
	memcpy(fieldnames[1], "requiresSqGlucoseSupport", sizeof("requiresSqGlucoseSupport"));
	memcpy(fieldnames[2], "requiresInsulinDosingSupport", sizeof("requiresInsulinDosingSupport"));
	memcpy(fieldnames[3], "requiresCPeptideSupport", sizeof("requiresCPeptideSupport"));
	memcpy(fieldnames[4], "requiresSqGlucagonSupport", sizeof("requiresSqGlucagonSupport"));
	memcpy(fieldnames[5], "numSignals", sizeof("numSignals"));
	memcpy(fieldnames[6], "signalDescription", sizeof("signalDescription"));

	mxArray* result = mxCreateStructMatrix(1, 1, 7, const_cast<const char**>(fieldnames));

	for (size_t i = 0; i < 7; i++)
		mxFree(fieldnames[i]);

	mxSetFieldByNumber(result, 0, 0, requiresSqInsulinSupport);
	mxSetFieldByNumber(result, 0, 1, requiresSqGlucoseSupport);
	mxSetFieldByNumber(result, 0, 2, requiresInsulinDosingSupport);
	mxSetFieldByNumber(result, 0, 3, requiresCPeptideSupport);
	mxSetFieldByNumber(result, 0, 4, requiresSqGlucagonSupport);
	mxSetFieldByNumber(result, 0, 5, numSignals);
	mxSetFieldByNumber(result, 0, 6, signalDescription);

	return result;
}

mxArray* getv__debugLogEnabled()
{
	return mxCreateLogicalScalar(false);
}

mxArray* getv__debugLog()
{
	return mxCreateString("");
}

mxArray* getv__modelInputsToModObject()
{
	mxArray* dup = mxDuplicateArray(modelIO);

	mxSetFieldByNumber(dup, 0, 0, mxCreateDoubleScalar(carbs));			// 0 = mealCarbsMgPerMin
	// 1 = fullMealCarbMgExpectedAtStart
	// 2 = glucOrDextIvInjMgPerMin
	// 3 = glucagonSqInjMg
	// 4 = exerciseIntensityAsFrac
	// 5 = glucoseTabletDoseMg
	mxSetFieldByNumber(dup, 0, 6, mxCreateDoubleScalar(basalInsulin));	// 6 = sqInsulinNormalBasal
	// 7 = ivInsulinNormalBasal
	mxSetFieldByNumber(dup, 0, 8, mxCreateDoubleScalar(bolusInsulin));	// 8 = sqInsulinNormalBolus
	// 9 = ivInsulinNormalBolus

	return dup;
}

static bool gInitialized = false;
static SOCKET gSocket;
static sockaddr_in gSrcAddr;
static sockaddr_in gAddr;

static SmartCGMS_To_DMMS lastValues{ 0, 0, 0 };
static size_t counter = 0;

static double prevBolus = 0;
static double dTime[2] = {0, 0};
static double iDose = 0;

static int mt = 0;
static int me = 0;

struct
{
	double ig, bg, ins, simtime;
	double mealgrams = 0, mealtime = 0;
	double exctime = 0, excintensity = 0, excduration = 0;
} inputs;

void eval__SmartCGMS()
{
	DMMS_To_SmartCGMS toSend;
	SmartCGMS_To_DMMS received;

	if (dTime[0] != dTime[1])
		iDose = 0;
	else
		iDose = iDose + prevBolus;

	inputs.ins = iDose / 6000;

	double time = inputs.simtime;
	if ((int)time % 5)
		time = (std::floor(time / 5) + 1) * 5;

	double mtime = inputs.mealtime;
	if (mtime > 59.8 && mtime < 60.2)
		mt = 60;

	double ameal_grams = 0;
	double ameal_time = 0;

	if (mt >= -60)
	{
		ameal_grams = inputs.mealgrams / 1000;
		ameal_time = mt;
		mt = mt - 1;
	}

	double etime = inputs.exctime;
	if (etime > 59.8 && etime < 60.2)
		me = 60;

	double aexc_intensity = 0;
	double aexc_time = 0;
	double aexc_duration = 0;

	if (me >= -60)
	{
		aexc_intensity = inputs.excintensity / 1000;
		aexc_time = me;
		aexc_duration = inputs.excduration;
		me = me - 1;
	}

	// extract inputs from Matlab
	toSend.glucose_ig = inputs.ig;
	toSend.glucose_bg = inputs.bg;
	toSend.insulin_injection = inputs.ins;
	toSend.simulation_time = time;
	toSend.announced_meal.grams = ameal_grams;
	toSend.announced_meal.time = ameal_time;
	toSend.announced_excercise.intensity = inputs.excintensity;
	toSend.announced_excercise.time = inputs.exctime;
	toSend.announced_excercise.duration = inputs.excduration;

	// if starting over a new simulation, reset counters and values
	if (toSend.simulation_time < 0.1)
	{
		lastValues.basal_rate = 0.0;
		lastValues.bolus_rate = 0.0;
		lastValues.carbs_rate = 0.0;
		lastValues.device_time = 0.0;
		counter = 0;
	}

	// T1DMS sends values every minute, but most parts of our simulator needs values every 5 minutes - we process only the first one from batch of 5
	if ((counter % 5) == 0)
	{
		// send values
		//sendto(gSocket, reinterpret_cast<char*>(&toSend), sizeof(toSend), 0, (sockaddr*)&gAddr, sizeof(gAddr));

		// copy to shared memory
		memcpy(filebuf_DataToSmartCGMS, &toSend, sizeof(toSend));
		// signal SmartCGMS
		SetEvent(event_DataToSmartCGMS);

		// wait for SmartCGMS to signal us
		WaitForSingleObject(event_DataFromSmartCGMS, INFINITE);

		// receive from shared memory
		memcpy(&received, filebuf_DataFromSmartCGMS, sizeof(received));

		// reset signal state of event from SmartCGMS (the other one is cleared by SmartCGMS)
		ResetEvent(event_DataFromSmartCGMS);

		lastValues = received;
	}

	basalInsulin = lastValues.basal_rate * 100;
	bolusInsulin = lastValues.bolus_rate * 100;
	carbs = *mxGetPr(mxGetFieldByNumber(modelIO, 0, 0)) + lastValues.carbs_rate * 1000;

	double deviceTime = lastValues.device_time;

	dTime[1] = dTime[0];
	dTime[0] = deviceTime;

	prevBolus = basalInsulin + bolusInsulin;

	counter++;
}

void Libeng_Process_Atach() {
	file_DataToSmartCGMS = CreateFileMappingW(INVALID_HANDLE_VALUE, NULL, PAGE_READWRITE, 0, 256, memMapFileName_DataToSmartCGMS);
	file_DataFromSmartCGMS = CreateFileMappingW(INVALID_HANDLE_VALUE, NULL, PAGE_READWRITE, 0, 256, memMapFileName_DataFromSmartCGMS);
	event_DataToSmartCGMS = CreateEventW(NULL, TRUE, FALSE, eventName_DataToSmartCGMS);
	event_DataFromSmartCGMS = CreateEventW(NULL, TRUE, FALSE, eventName_DataFromSmartCGMS);

	filebuf_DataToSmartCGMS = MapViewOfFile(file_DataToSmartCGMS, FILE_MAP_ALL_ACCESS, 0, 0, 256);
	filebuf_DataFromSmartCGMS = MapViewOfFile(file_DataFromSmartCGMS, FILE_MAP_ALL_ACCESS, 0, 0, 256);

	ResetEvent(event_DataToSmartCGMS);
	ResetEvent(event_DataFromSmartCGMS);
}

void Libeng_Process_Detach() {
	if (filebuf_DataToSmartCGMS)
		UnmapViewOfFile(filebuf_DataToSmartCGMS);
	if (filebuf_DataFromSmartCGMS)
		UnmapViewOfFile(filebuf_DataFromSmartCGMS);

	if (file_DataToSmartCGMS)
		CloseHandle(file_DataToSmartCGMS);
	if (file_DataFromSmartCGMS)
		CloseHandle(file_DataFromSmartCGMS);
	if (event_DataToSmartCGMS)
		CloseHandle(event_DataToSmartCGMS);
	if (event_DataFromSmartCGMS)
		CloseHandle(event_DataFromSmartCGMS);
}

extern "C"
{
	int PROXY_engClose(int* eng)
	{
		if (eng)
			delete eng;

		return 0;
	}

	int PROXY_engEvalString(void *a, const char *b)
	{
		const char* identstr = "modelInputsToModObject=smartcgm";
		if (strncmp(b, identstr, strlen(identstr)) == 0)
		{
			eval__SmartCGMS();

			return 0;
		}

		return 0;
	}

	mxArray* PROXY_engGetVariable(void* a, const char* b)
	{
		if (b)
		{
			if (strcmp(b, "settings") == 0)
				return getv__smartcgms_settings();
			else if (strcmp(b, "debugLogEnabled") == 0)
				return getv__debugLogEnabled();
			else if (strcmp(b, "debugLog") == 0)
				return getv__debugLog();
			else if (strcmp(b, "modelInputsToModObject") == 0)
				return getv__modelInputsToModObject();
		}

		return nullptr;
	}

	int PROXY_engGetVisible(void *ep, bool *value)
	{
		*value = true;
		return 0;
	}

	void* PROXY_engOpen(const char *startcmd)
	{
		return new int;
	}

	void* PROXY_engOpenSingleUse(const char* a, void* b, int* c)
	{
		return new int;
	}

	int PROXY_engOutputBuffer(void *ep, char *p, int n)
	{
		return 0;
	}

	int PROXY_engPutVariable(void *a, const char *b, const mxArray *c)
	{
		if (b)
		{
			if (strcmp(b, "subjectObject") == 0)
			{
				// discard
				//return 0;
			}
			else if (strcmp(b, "sensorSigArray") == 0)
			{
				mxDouble* sigs = mxGetDoubles(c);
				double bg = sigs[0];
				double ig = sigs[1];

				inputs.ig = ig;
				inputs.bg = bg;
			}
			else if (strcmp(b, "nextMealObject") == 0)
			{
				double amount = *mxGetPr(mxGetFieldByNumber(c, 0, 0)); // amount/1000 [g]
				double duration = *mxGetPr(mxGetFieldByNumber(c, 0, 1)); // eating time (minutes from start to end of consumption)
				double minsUntilMeal = *mxGetPr(mxGetFieldByNumber(c, 0, 2)); // minutes until meal

				inputs.mealgrams = amount;
				inputs.mealtime = minsUntilMeal;
			}
			else if (strcmp(b, "nextExerciseObject") == 0)
			{
				// UNK; TODO: decode

				double val1 = *mxGetPr(mxGetFieldByNumber(c, 0, 0));
				double val2 = *mxGetPr(mxGetFieldByNumber(c, 0, 1));
				double val3 = *mxGetPr(mxGetFieldByNumber(c, 0, 2));

				/*f = fopen("calls_libeng.txt", "a");
				fprintf(f, "excercise: %lf %lf %lf\n", val1, val2, val3);
				fclose(f);*/
			}
			else if (strcmp(b, "timeObject") == 0)
			{
				double minutesPastSimStart = *mxGetPr(mxGetFieldByNumber(c, 0, 0));

				inputs.simtime = minutesPastSimStart;
			}
			else if (strcmp(b, "modelInputsToModObject") == 0)
			{
				if (modelIO)
					mxDestroyArray(modelIO);
				modelIO = mxDuplicateArray(c);
			}
		}

		return 0;
	}

	int PROXY_engSetVisible(void *ep, bool value)
	{
		return 0;
	}
}