%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% This is an example model script - simple linear model y=kx+q
%
% Inputs:
%   model_parameters - vector of parameters (double)
%   model_times - vector of times we requested the model signal to be
%				  calculated (double)
%   model_derivation - order of derivation (0 = f(x), 1 = f'(x), ...)
%
% Outputs:
%   model_output - vector of calculated signal values (double)
%            it MUST be the same length as TIMES and INPUT vectors
%            if the model couldn't calculate the value in requested time,
%            NaN is put at its place
%
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

% model_parameters = [k q]

% f(x) = kx+q
if model_derivation == 0
    model_output = model_times*model_parameters(1) + model_parameters(2);
else
    % higher order derivations yields zeros
    model_output = zeros(size(model_times), 1);
    % f'(x) = k
    if model_derivation == 1
        for i = 1 : size(model_times)
            model_output(i) = model_parameters(1);
        end
    end
end
