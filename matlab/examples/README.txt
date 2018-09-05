=== MATLAB gpredict3 library

== Manifest

The user is advised to use matlab_manifest.xml example attached to this file. The
manifest is meant to be placed in the same directory, as the matlab.dll (.so, ..)
library of gpredict3 simulator.

The manifest file should start with XML version declaration tag, as shown in example.

Inside the <session> tag, you may specify the session name of your MATLAB instance.
MATLAB is able to share running session using given session name, so if you want to
connect gpredict3 with MATLAB environment, open your MATLAB instance and use the
following command: matlab.engine.shareEngine("my_session_name");
Then you need to specify the name inside manifest. We use "gpredict3" session name,
you may want to change it to "my_session_name", or whatever you used in shareEngine
command. If you don't specify session name or the session with such name doesn't exist,
gpredict3 will start a new session.

The <directory> tag contains a working directory for your MATLAB session. It's the
directory you store your scripts in.

Every entity declared inside the manifest must have unique GUID assigned. Note the GUID
is not just some random identifier, and has got rules tied to its generator. You may
want to use specialized tool, such as: https://www.guidgenerator.com/
Every GUID is enclosed in curly brackets and may look like this:
{D1778BF0-208C-4E61-BADE-15141490E5BE}

= Model

The manifest contains <models> group. Every model is declared inside <model> tag.
Parameters:
	- id: Entity GUID, see above
	- description: The description of model shown in GUI
	- dbtable: Database table name for parameter storing

Definition of model also contains <parameters> group. Each <param> has its own
properties:
	- name: Parameter name (to be shown e.g. in GUI)
	- dbname: Database table column name
	- default: Default value of this parameter
	- lowerbound: Lower bound of this parameter
	- upperbound: Upper bound of this parameter

Each model also must define at least one <signal> inside <signals> group. Every signal
then has its properties:
	- id: Entity GUID, see above
	- name: Signal name (to be shown e.g. in GUI)
	- reference: Reference signal GUID
	- scriptname: MATLAB script name (the .m file name) of model signal retrieving script

Several reference signal GUIDs that may be used as reference for your model signal:
	- Interstitial glucose: {3034568D-F498-455B-AC6A-BCF301F69C9E}
	- Blood glucose: 		{F666F6C2-D7C0-43E8-8EE1-C8CAA8F860E5}
	- ISig: 				{3F62C28A-4D25-4086-BD1A-FC442FDDB7CF}
	- Calibration: 			{ED4CD0F5-F728-44FE-9552-97338BD7E8D5}
	- Insulin intake: 		{22D87566-AF1B-4CC7-8D11-C5E04E1E9C8A}
	- Carbohydrates intake: {37AA6AC1-6984-4A06-92CC-A660110D0DC7}
	- Health stress:		{F4438E9A-DD52-45BD-83CE-5E93615E62BD}
	
= Solver

The <solvers> group defines MATLAB implemented solvers. Each solver is declared inside
<solver> tag. Parameters:
	- id: Entity GUID, see above
	- description: Solver description ("name"; to be shown e.g. in GUI)
	- scriptname: MATLAB script name (the .m file name) of solver routines
	
The solver could be specialized to a subset of models. If the <models> group is specified,
it contains <model> tags, each of them defines a model the solver is able to work with.
The <model> tag has only one parameter - id, as the GUID of model.

== The MATLAB scripts

= Model

To define a model script, one needs to create a script, that is able to calculate signal
using given parameters. For a very simple model implementation, see example script
linear_get_continous_levels.m.

This script has several parameters (workspace variables set by gpredict3):
	- model_parameters: vector of parameters (double)
	- model_times: the vector of times we requested the model signal to be calculated
	- model_derivation: order of derivation (0 = f(x), 1 = f'(x), ...)

The script is required to put calculated values inside a "model_output" variable, as shown
in example. The output array MUST be the same length as "model_times" vector. If the model
is, for some reason, unable to calculate the signal in any of requested times, the script
should put NaN to this index in array.

= Solver

To define a solver script, one needs to create a script, that is able to calculate a set
of model parameters given reference signal times and values. For a very simple least
squares solver implementation, see example script least_squares_solve.m.

The solver has several parameters (workspace variables set by gpredict3):
	- solver_times: vector of reference signal times (double)
	- solver_values: vector of reference signal values (double)
	- solver_hints: parameter hints, one parameter set per line
	- solver_lowerbounds: parameter lower boundaries
	- solver_upperbounds: parameter upper boundaries
	
The script is required to put calculated model parameters inside a "solver_output"
variable. If, for some reason, the solver could not calculate a new set of parameters,
the output variable may be set to scalar NaN to indicate, that the solver is unable
to calculate parameters using given inputs.

NOTE: the user has to take care of metric implementation, we decided not to export our
	  implementations to MATLAB scripts.

== Integration with gpredict3

If you define the model and/or solver properly, the manifest, solver and/or model
scripts are correct, the matlab core dynamic library is properly built and the
environment on your machine is properly set, the filter configuration will now offer
you your newly declared solver and/or model (with its signal). Just use the "Calculate"
filter and configure it with you solver and/or model. You should also be able to
use MATLAB model with our solvers, or our models with MATLAB solvers.
