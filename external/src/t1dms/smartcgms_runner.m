clearvars

%%% Custom setup

smartcgms_mex_location = '<INSERT YOUR LOCATION HERE>';
subject_name = 'adult#average'; % subject (file) name without extension
simulation_time = 2880;         % in minutes
bg_init = 144;                  % ~8 mmol/l
sensor_noise_factor = 0.2;      % higher number = more noise

% future SmartCGMS would calculate carb rates from meals on its own,
% so this won't be necessary

% two days with standard meals at standard times
meals = [ 0    0.0; 359  0.0;
                    360  5.0; 365  5.0;
                    366  0.0; 539  0.0;
                    540  3.6; 545  3.6;
                    546  0.0; 719  0.0;
                    720  4.0; 735  4.0;
                    736  0.0; 899  0.0;
                    900  2.0; 910  2.0;
                    911  0.0; 1079 0.0;
                    1080 3.6; 1095 3.6;
                    1096 0.0; 1799 0.0;
                    1800 5.0; 1805 5.0;
                    1806 0.0; 1979 0.0;
                    1980 3.6; 1985 3.6; 
                    1986 0.0; 2159 0.0;
                    2160 4.0; 2175 4.0;
                    2176 0.0; 2339 0.0;
                    2340 2.0; 2350 2.0;
                    2351 0.0; 2519 0.0;
                    2520 3.6; 2535 3.6;
                    2536 0.0; 2880 0.0];

%%% Hardware - sensor and pump

hardware = struct;
hardware.SensorType = 1;
hardware.pump_name = 'Generic_1.pmp';
hardware.sensor_name = 'dexcom70.scs';
hardware.pump_noise = 0;
hardware.pump_char = 1;
hardware.pump_bolus_min = 0;        %  0 U
hardware.pump_bolus_max = 10000;    % 10 U
hardware.pump_bolus_inc = 300;
hardware.pump_basal_min = 0;        % 0   U/hr
hardware.pump_basal_max = 30000;    % 30.0 U/hr (safety filter in SmartCGMS may limit this more)
hardware.pump_basal_inc = 5;
hardware.pump_sampling = 1;
hardware.pump_accuracy_bolus_amount = [0 100];
hardware.pump_bolus_mean = [0 100];
hardware.pump_bolus_std2r = [0 0];
hardware.sensor_PACF = sensor_noise_factor;
hardware.sensor_type = 'SU';
hardware.sensor_gamma = -0.5444;
hardware.sensor_lambda = 15.9574;
hardware.sensor_delta = 1.6898;
hardware.sensor_xi = -5.47;
hardware.sensor_sampling = 5;
hardware.sensor_min = 36;   % ~2  mmol/l
hardware.sensor_max = 900;  % ~50 mmol/l

%%% Scenario setup

Lscenario = struct;
Lscenario.SQ_Gluc = struct;
Lscenario.SQ_Gluc.time = 0;
Lscenario.SQ_Gluc.signals = struct;
Lscenario.SQ_Gluc.signals.dimensions = 1;
Lscenario.SQ_Gluc.signals.values = 0;
Lscenario.SQ_Pram = struct;
Lscenario.SQ_Pram.time = 0;
Lscenario.SQ_Pram.signals = struct;
Lscenario.SQ_Pram.signals.dimensions = 1;
Lscenario.SQ_Pram.signals.values = 0;
Lscenario.SQ_Pram.signals.dimension = 1;
Lscenario.SQ_insulin = struct;
Lscenario.SQ_insulin.time = 0;
Lscenario.SQ_insulin.signals = struct;
Lscenario.SQ_insulin.signals.values = [0 0];
Lscenario.SQ_insulin.signals.dimensions = 2;
Lscenario.meals = meals;
Lscenario.meal_announce = struct;
Lscenario.meal_announce.time = 0;
Lscenario.meal_announce.signals = struct;
Lscenario.meal_announce.signals.values = [0 0];
Lscenario.meal_announce.signals.dimensions = 2;
Lscenario.dose = [0 0];
Lscenario.Tdose = [0 360];
Lscenario.Qmeals = 'total';
Lscenario.Tsimul = simulation_time;
Lscenario.Tclosed = 0;
Lscenario.BGinit = bg_init;
Lscenario.Treg = 0;
Lscenario.CR = [];
Lscenario.SQg = 0;
Lscenario.IV_insulin = [0 0];
Lscenario.IV_glucose = [0 0];
Lscenario.QIVD = 'total';
Lscenario.QIVins = 'total';
Lscenario.SCNname = 'custom_smartcgms';
Lscenario.Qbolus = 'total';
Lscenario.simToD = 0;
Lscenario.basal = 0;
Lscenario.Qbasal = 'quest';

%%% Main setup

cwd=[cd filesep];
addpath([cwd 'controller setup'])
addpath([cwd 'subjects'])

% to find C++ MEX library (gpredict3_controller)
addpath(smartcgms_mex_location)

subjects=load_subject([subject_name '.mat']);
subject=subjects(1);

% initialize PRNG with fixed seed (so the experiment could be reproduced)
seed=123456.0;
subject.rg=RandStream.create('mrg32k3a','NumStreams',1,'StreamIndices',1,'Seed',seed);

display(['simulating ' subject.names])
simulation_result=run_simulation(Lscenario, subject, hardware, 1, Lscenario.meals, Lscenario.meal_announce, Lscenario.SQ_insulin.signals, 1);
display(['end of ' subject.names])
        