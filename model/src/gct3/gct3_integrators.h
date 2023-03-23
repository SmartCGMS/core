#pragma once

//#include "gct3_transfer_functions.h"

namespace gct3_model {

	class CTransfer_Function;

	class IIntegrator {
		protected:
			const CTransfer_Function& mFunction;

		public:
			IIntegrator(const CTransfer_Function& fnc) : mFunction(fnc) {
				//
			}
			virtual ~IIntegrator() = default;

			virtual double Integrate(double time_start, double time_end) const = 0;
	};

	// Rectangular rule for integration
	class CRectangular_Integrator : public IIntegrator {
		public:
			using IIntegrator::IIntegrator;

			double Integrate(double time_start, double time_end) const override;
	};

	// Single midpoint rule for integration
	class CMidpoint_Integrator : public IIntegrator {
		public:
			using IIntegrator::IIntegrator;

			double Integrate(double time_start, double time_end) const override;
	};

	// Simspon's 1/3 rule for integration
	class CSimpson_1_3_Rule_Integrator : public IIntegrator {
		public:
			using IIntegrator::IIntegrator;

			double Integrate(double time_start, double time_end) const override;
	};

	// Gaussian quadrature rule for integration
	class CGaussian_Quadrature_Integrator : public IIntegrator {
		private:
			// transform to <-1;1> interval
			double Transform(double y_diff_half, double y_sum_half, double t) const;

		public:
			using IIntegrator::IIntegrator;

			double Integrate(double time_start, double time_end) const override;
	};
}
