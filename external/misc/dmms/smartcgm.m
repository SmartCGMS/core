% subjObject - A 1x1 struct object containing subject parameters
% name:  Subject name, as displayed in the subject selection tab (e.g. adult#001).
% type1:  Boolean indicating true if the subject is type 1
% CR:  Carbohydrate ratio (g/Unit)
% CF:  Correction Factor (mg/dL/Unit)
% Gb:  Basal (fasting) glucose concentration (mg/dL)
% BW:  bodyweight (kg)
% dailyBasalInsulin:  Daily amount of basal "normal" insulin needed to maintain the basal fasting glucose concentration, Gb (Units)
% OGTT:  The subject's 2-hour OGTT result -- only applicable to T2 and prediabetic subjects (mg/dL)
%
% sensorSigArray - A Nx1 array containing provided sensors
% The order of signals in this array matches the order as entered via the "Edit Input Signals" dialog box within the configuration of this control.
%
% nextMealObject - A 1x1 struct object containing the properties of next scheduled meal
% amountMg: The amount of carbohydrates in meal before any multiplier is applied.
% durationInMinutes: The total duration of meal consumption.
% minutesUntilNextMeal: The remaining minutes before the next meal.
% bolusMultiplier: The scaling factor to be applied to the size of the meal.
%
% nextErerciseObject - A 1x1 struct object containing the properties of next scheduled exercise
% intensityFrac: A value from 0.0 to 1.0:
%     0: No exercise
%     0.25:   Light exercise
%     0.5:    Moderate exercise
%     0.655:  Intense exercise
% durationInMinutes: Total duration of the exercise session
% minutesUntilNextSession: The minutes before the next scheduled exercise.
%
% timeObject - A 1x1 struct object containing the properties of simulation time
% minutesPastSimStart:  number of minutes since the start of the simulation
% daysSinceJan1:  number of days since Jan 1 (0 for Jan 1, 1 for Jan 2, etc.) -- can be used for time of year, to handle possible seasonal effects
% daysSinceMonday:  number of days since Monday (0 for Monday, 1 for Tuesday, etc.) -- can be used for handling weekly schedules
% minutesPastMidnight:  time of day, in minutes since midnight (0=midnight, 360=6AM, 720=noon, etc.)
%
% modelInputsToModObject - A 1x1 struct object containing the properties of model input parameters
% mealCarbsMgPerMin:  rate of consumption of meal carbohydrates (mg/minute)
% fullMealCarbMgExpectedAtStart:  value indicated only at start of meal (0 at all other times), representing total carbs in the meal (mg).
% glucOrDextIvInjMgPerMin:  glucose or dextrose provided by IV (mg/minute)
% glucagonSqInjMg:  rate of glucagon being provided subcutaneously (mg/minute)
% exerciseIntensityAsFrac:  intensity of exercise as a fraction of full intensity (1.0 = full intensity, 0.0 = no exercise)
% sqInsulinNormalBasal:  subcutaneous delivery rate of normal insulin being used for basal insulin (pmol/minute)
% ivInsulinNormalBasal:  intraveneous delivery rate of normal insulin being used for basal insulin (pmol/minute)
% sqInsulinNormalBolus:  subcutaneous delivery rate of normal insulin being used as a bolus (pmol/minute) -- usually the whole bolus is given in 1 minute
% ivInsulinNormalBolus:  intraveneous delivery rate of normal insulin being used as a bolus (pmol/minute) -- usually the whole bolus is given in 1 minute
% slowRelInsulinStandardLongActing:  delivery rate of the standard long-acting insulin (pmol/minute) -- usually the whole dose is given in 1 minute
% sqCustomInsulin1:       subcutaneous delivery rate of the 1st user-defined insulin (pmol/minute), only to be used when this insulin is not of a "slow release" variety
% ivCustomInsulin1:       intraveneous delivery rate of the 1st user-defined insulin (pmol/minute), only to be used when this insulin is not of a "slow release" variety
% slowRelCustomInsulin1:  delivery rate of the 1st user-defined insulin, to be used when this insulin is of a slow release variety.
% sqCustomInsulin2:  subcutaneous delivery rate of the 2nd user-defined insulin (pmol/minute), only to be used when this insulin is not of a "slow release" variety
% ivCustomInsulin2:  intraveneous delivery rate of the 2nd user-defined insulin (pmol/minute), only to be used when this insulin is not of a "slow release" variety
% slowRelCustomInsulin2:  delivery rate of the 2nd user-defined insulin, to be used when this insulin is of a slow release variety.
% ...etc.
% The order in which custom insulins are associated with the ...CustomInsulin1, 2, etc. properties will correspond to the order in the "insulin definitions"
% configuration page.
%
%%%%%%%%%%%
% Warning %
%%%%%%%%%%%
% Please do not run any command on an active Matlab engine when it's visible.
% The simulation result may be affected by any command entered in the console.
% To enable debug log functionality, declare "global debugLog" in the main script.

% Simple titration approach to target BG
function modelInputsToModObject = smartcgm(subjObject, sensorSigArray, nextMealObject, nextExerciseObject, timeObject, modelInputsToModObject)    
 	global debugLog; % Enable debug log    
    persistent dTime;
    persistent mt;
    persistent iDose;
    persistent prevBolus;
    if isempty(dTime)
        dTime = [0.0 0.0];
    end
    if isempty(iDose)
        iDose = 0.0;
    end
    if isempty(prevBolus)
        prevBolus = 0;
    end
    glucose_bg = round(sensorSigArray(1),1);
    glucose_ig = round(sensorSigArray(2),1);
    if dTime(1)~=dTime(2)
        iDose = 0;        
        else
        iDose = iDose + prevBolus;
    end
    insulin_injection = iDose/6000;
    time = timeObject.minutesPastSimStart;
    if rem(time,5)
        time = (floor(time/5)+1)*5;
    end
    if nextMealObject.minutesUntilNextMeal == 60
        mt = 60;
    end
    if (mt >= -60)
        ameal_grams = nextMealObject.amountMg/1000;
        ameal_time = mt;
        mt = mt - 1;
    else
        ameal_grams = 0;
        ameal_time = 0;
    end
    debugLog = [debugLog '\n' sprintf('Input - g_ig:%f(mg/dL),g_bg:%f(mg/dL),ins_inj:%f(pmol/L),time:%f(min),meal_g:%f(g),meal_time%:.0f(min)',glucose_ig, glucose_bg, insulin_injection, time, ameal_grams, ameal_time) '\n'];
    
    [basalrate, bolusrate, carbsrate, device_time] = gpredict3_controller(glucose_ig, glucose_bg, insulin_injection, time, ameal_grams, ameal_time);
    dTime(2) = dTime(1);
    dTime(1) = device_time;    
    debugLog = [debugLog sprintf('output - basal_rate:%f (U/hr), bolus_rate:%f(U/hr),carb_rate:%f(g/min),device_time:%s',basalrate, bolusrate, carbsrate, datestr(device_time)) '\n'];
    modelInputsToModObject.sqInsulinNormalBasal = round(basalrate*100);
    modelInputsToModObject.sqInsulinNormalBolus = round(bolusrate*100);
    prevBolus = modelInputsToModObject.sqInsulinNormalBasal + modelInputsToModObject.sqInsulinNormalBolus;
    modelInputsToModObject.mealCarbsMgPerMin = modelInputsToModObject.mealCarbsMgPerMin + carbsrate*1000;
end