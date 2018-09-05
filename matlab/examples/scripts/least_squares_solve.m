%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% This is an example solver script - simple least squares using
% overdetermined equation system
%
% Inputs:
%   solver_times  - vector of reference signal times (double)
%   solver_values - vector of reference signal values (double)
%   solver_hints  - parameter hints, one parameter set per line
%   solver_lowerbounds - parameter lower boundaries
%   solver_upperbounds - parameter upper boundaries
%
% Outputs:
%   solver_output - vector of model parameters
%
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

% note this solver is declared to be specialized to our linear regression
% model because of the way we use for initializing the following matrix:

T = [solver_times' ones(size(solver_times(1,:)))'];

LHS = T'*T;
RHS = T'*solver_values';

solver_output = (LHS\RHS)'; % transpose, outer code expects a vector
