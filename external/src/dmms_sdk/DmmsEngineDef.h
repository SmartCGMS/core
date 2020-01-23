/*
 * This files defines objects used in the ISimCallbacks interface, which represents one of the DMMS
 * feedback control elements when calling DMMS as a dinamic library. To create a callback object that
 * can be used in the DMMS, first configure a Rx plan in DMMS main GUI and load the "Dynamic Library
 * Callback" in the "Feedback Control Element" list and save the plan. The next step requires to
 * start a C++ project. In that project, user must derive a new class from the "ISimCallback" class and
 * define function for the initialization, iteration, clean-up, number of output signal, and output
 * signal names. Finally, load the DLL in the project, instantiate the DmmsEngine, and run the callback
 * object.
 *
 *
 * Object descriptions:
 *
 * subjObject properties:
 *   name:  subject name, as displayed in the subject selection tab (e.g. adult#001).
 *   type1:  value = 1 indicates that the subject is type 1
 *   CR:  Carbohydrate ratio (g/Unit)
 *   CF:  Correction Factor (mg/dL/Unit)
 *   Gb:  Basal (fasting) glucose concentration (mg/dL)
 *   BW:  bodyweight (kg)
 *   dailyBasalInsulin:  Daily amount of basal "normal" insulin needed to maintain the basal fasting glucose concentration, Gb (Units)
 *   OGTT:  The subject's 2-hour OGTT result -- only applicable to T2 and prediabetic subjects (mg/dL)
 *
 * sensorSigArray structure:
 *   An array of values from sensor signals configured for this Plugin Control. The order of signals in this array matches
 *   the order as entered via the "Edit Input Signals" dialog box within the configuration of this control. Ex: sensorSigArray[0]
 *
 * nextMealObject properties:
 *   amountMg
 *   durationInMinutes
 *   minutesUntilNextMeal
 *   bolusMultiplier
 *
 * nextExerciseObject properties:
 *   intensityFrac:  a value from 0.0 to 1.0, where the following values correspond to the indicated levels:
 *                       0:      No exercise
 *                       0.25:   Light exercise
 *                       0.5:    Moderate exercise
 *                       0.655:  Intense exercise
 *   durationInMinutes
 *   minutesUntilNextSession
 *
 * timeObject properties:
 *   minutesPastSimStart:  number of minutes since the start of the simulation
 *   daysSinceJan1:  number of days since Jan 1 (0 for Jan 1, 1 for Jan 2, etc.) -- can be used for time of year, to handle possible seasonal effects
 *   daysSinceMonday:  number of days since Monday (0 for Monday, 1 for Tuesday, etc.) -- can be used for handling weekly schedules
 *   minutesPastMidnight:  time of day, in minutes since midnight (0=midnight, 360=6AM, 720=noon, etc.)
 *
 * modelInputsToModObject properties:
 *   mealCarbsMgPerMin: rate of consumption of meal carbohydrates (mg/minute)
 *   fullMealCarbMgExpectedAtStart: value indicated only at start of meal (0 at all other times), representing total carbs in upcoming meal (mg)
 *   glucOrDextIvInjMgPerMin: glucose or dextrose provided by IV (mg/minute)
 *   glucagonSqInjMg: rate of glucagon being provided subcutaneously (mg/minute)
 *   exerciseIntensityAsFrac: intensity of exercise as a fraction of full intensity (1.0 = full intensity, 0.0 = no exercise)
 *   glucoseTabletDoseMg: size (in mg) of a glucose tablet being provided at this instant
 *   sqInsulinNormalBasal: subcutaneous delivery rate of normal insulin being used for basal insulin (pmol/minute)
 *   ivInsulinNormalBasal: intraveneous delivery rate of normal insulin being used for basal insulin (pmol/minute)
 *   sqInsulinNormalBolus: subcutaneous delivery rate of normal insulin being used as a bolus (pmol/minute) -- usually the whole bolus is given in 1 minute
 *   ivInsulinNormalBolus: intraveneous delivery rate of normal insulin being used as a bolus (pmol/minute) -- usually the whole bolus is given in 1 minute
 *   slowRelInsulinStandardLongActing: delivery rate of the standard long-acting insulin (pmol/minute) -- usually the whole dose is given in 1 minute
 *   Custom insulins: The order in which custom insulins are associated with the ...CustomInsulin[1], ...CustomInsulin[2], etc.
 *     properties will correspond to the order in the "insulin definitions" configuration page
 *    sqCustomInsulin: An array for subcutaneous delivery rate of the N-th user-defined insulin (pmol/minute), only to be used when this insulin is not of a "slow release" variety
 *     e.g. sqCustomInsulin[0] is subcutaneous delivery rate of the first user-defiend insulin
 *    ivCustomInsulin: An array for intraveneous delivery rate of the N-th user-defined insulin (pmol/minute), only to be used when this insulin is not of a "slow release" variety
 *     e.g. ivCustomInsulin[0] is intraveneous delivery rate of the first user-defiend insulin
 *    slowRelCustomInsulin: An array for delivery rate of the N-th user-defined insulin, to be used when this insulin is of a slow release variety
 *     e.g. slowRelCustomInsulin[0] is the delivery rate of the the first user-defined insulin of a slow release type
 *
 * inNamedSignals:
 *   This is used to receive signals generated from other control elements as output signals (defined in their outSignalName function)
 *   struct InNamedSignals contains three members:
 *      size: number of signals from the output signal of other preceding control elements
 *      names: array of names from the output signal of other preceding control elements
 *      values: array of values from the output signal of other preceding control elements
 *
 * OutSignalArray:
 *   This is used to populate values that will be contributed to the recorded state history files, in the same way that sensor
 *   signals are. The values in outNamedSignals will be made available to control elements running after this control element, via their inNamedSignals parameter
 *   The number and names of output signals must be defined properly in the outSignalName() and numOutputSignals() in the ISimCallbacks class.
*/

#ifndef DMMSENGINEDEF_H
#define DMMSENGINEDEF_H

#ifdef _WIN32
    #define IfaceCalling __stdcall
#else
    #define IfaceCalling
#endif

namespace dmms {
/*!
 * Please read before implementation
 *
 * User must include compiler specific header files for definition of `size_t`,`uint8_t` and `HRESULT`.
 *
 * The use of `int` and `bool` was replaced with size_t and unit8_t, respectively. Because the size of `int` or `bool`
 * is not guaranteed in a compiler or a system (x86 or x64), we use `size_t` and `uint8_t` for better inter-compiler/
 * inter-platform compatibility. A user must include a header file that defines `size_t` and `unit8_t` before include
 * "DmmsEnglieDef.h" in a project. For example, include cstdint for a minGW C++ project.
 *
 * All funtions return HRESULT in order to better follow the COM specification for the returning signature. A user
 * must inlcude a compiler specific header file that defines HRESULT. For example, for none MFC environment, include
 * "winnt.h" or "windows.h"
 *
 */

/* Simulation information */
struct SimInfo {
    char* popName;
    char* subjName;
    double simDuration;
    char* configDir;
    char* baseResultsDir;
    char* simName;
};

/* Subject object - read only */
struct SubjectObject {
    char* name;
    uint8_t type1;
    double CR;
    double CF;
    double Gb;
    double BW;
    double dailyBasalInsulin;
    double OGTT2;
};

/* NextMeal object - read only */
struct NextMealObject {
    double amountMg;
    double durationInMinutes;
    double minutesUntilNextMeal;
    double bolusMultiplier;
};

/* NextExercise object - read only */
struct NextExerciseObject {
    double intensityFrac;
    double durationInMinutes;
    double minutesUntilNextSession;
};

/* Time object - read only */
struct TimeObject {
    double minutesPastSimStart;
    double daysSinceJan1;
    double daysSinceMonday;
    double minutesPastMidnight;
};

/* Sensor Array - read only */
using SensorSigArray = double*;

/* Named input signals from the preceding iteration - read only  */
struct InNamedSignals {
    size_t size;
    const char** names;
    double* values;
};

/* Named output signals to the next iteration - read only */
using OutSignalArray = double* ;
using  OutputSignalNames = char**;

/* Run / Stop status flag */
struct RunStopStatus {
    uint8_t stopRun = 0;
    uint8_t error = 0;
    char* message; // size = 250
};

/* ModelInputs object to be modified */
typedef struct ModelInputToModObject {
    double mealCarbsMgPerMin;
    double fullMealCarbMgExpectedAtStart;
    double glucOrDextIvInjMgPerMin;
    double glucagonSqInjMg;
    double exerciseIntensityAsFrac;
    double glucoseTabletDoseMg;
    double sqInsulinNormalBasal;
    double ivInsulinNormalBasal;
    double sqInsulinNormalBolus;
    double ivInsulinNormalBolus;
    double slowRelInsulinStandardLongActing;
    double* sqCustomInsulin;
    double* ivCustomInsulin;
    double* slowRelCustomInsulin;    
} ModelInputObject;


/* The interface of which that allows users to derive from, so the derived object can be passed to the simulator. */
class ISimCallbacks
{
public:
    /* This required method will be run at each iteration */
    virtual HRESULT IfaceCalling iterationCallback(const SubjectObject *subjObject,
                                   const SensorSigArray *sensorSigArray,
                                   const NextMealObject *nextMealObject,
                                   const NextExerciseObject *nextExerciseObject,
                                   const TimeObject *timeObject,
                                   ModelInputObject *modelInputsToModObject,
                                   const InNamedSignals *inNamedSignals,
                                   OutSignalArray *outSignalArray,
                                   RunStopStatus *runStopStatus) = 0;

    /* This optional method will be executed once before the start of a simulation.
     * A user is also suggested to update the seeding information of pseudo random generatorshere if this function
     * is provided in the derived class. "useUniversalSeed" sets to true indicating the current seeding is set
     * to use a universal seed number across all plugins. "universalSeed" indicates the universal seed number in
     * this case. If "useUniversalSeed" is false, "universalSeed" value is always 0.0. A user may change the
     * universal seed number in the DMMS GUI. A user may also initialize the values of other members in the
     * derived class for later to be used in the iterationCallback(). */
    virtual HRESULT IfaceCalling initializeCallback(const uint8_t useUniversalSeed, const double universalSeed,
                                    const SimInfo *simInfo) = 0;    

    /* Name of output signals*/
    virtual HRESULT IfaceCalling outSignalName(OutputSignalNames *names) = 0;
    /* Number of output signals */
    virtual HRESULT IfaceCalling numOutputSignals(size_t *count) = 0;

    /* This optional method will be executed once after the end of a simulation.
     * A pritical use of this function may be deleting raw pointers or closing a file if a logging system was
     * in use. */
    virtual HRESULT IfaceCalling cleanupCallback() = 0;
};

}

#endif // DMMSENGINEDEF_H
