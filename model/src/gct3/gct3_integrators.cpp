#include "gct3_integrators.h"
#include "gct3_transfer_functions.h"

namespace gct3_model {

	double CRectangular_Integrator::Integrate(double time_start, double time_end) const {
		const double y_diff = time_end - time_start;
		const double x0 = mFunction.Calculate_Transfer_Input(time_start);

		return y_diff * x0;
	}

	double CMidpoint_Integrator::Integrate(double time_start, double time_end) const {
		const double y_diff = time_end - time_start;
		const double x0 = mFunction.Calculate_Transfer_Input(time_start);
		const double x1 = mFunction.Calculate_Transfer_Input(time_end);

		return y_diff * (x1 + x0) * 0.5;
	}

	double CSimpson_1_3_Rule_Integrator::Integrate(double time_start, double time_end) const {
		const double y_diff = time_end - time_start;
		const double x0 = mFunction.Calculate_Transfer_Input(time_start);
		const double x1 = mFunction.Calculate_Transfer_Input((time_start + time_end) / 2.0);
		const double x2 = mFunction.Calculate_Transfer_Input(time_end);

		return (y_diff / 6.0) * (x0 + 4 * x1 + x2);
	}

	double CGaussian_Quadrature_Integrator::Transform(double y_diff_half, double y_sum_half, double t) const {
		return mFunction.Calculate_Transfer_Input(y_sum_half + y_diff_half * t) * y_diff_half;
	}

	double CGaussian_Quadrature_Integrator::Integrate(double time_start, double time_end) const {

		const double y_diff = time_end - time_start;
		const double y_sum = time_end + time_start;

		const double y_diff_half = y_diff * 0.5;
		const double y_sum_half = y_sum * 0.5;

		const double x0 = 0.555555556 * Transform(y_diff_half, y_sum_half, 0.7745966692);
		const double x1 = 0.888888889 * Transform(y_diff_half, y_sum_half, 0.0);
		const double x2 = 0.555555556 * Transform(y_diff_half, y_sum_half, -0.7745966692);

		return (x0 + x1 + x2);
	}
}
