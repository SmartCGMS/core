/**
 * Portal for online glucose level calculation
 * Final calculation and SVG generating part
 * https://diabetes.zcu.cz/
 *
 * Author: Marek Rasocha (mrasocha@students.zcu.cz)
 * Maintainer: Martin Ubl (ublm@students.zcu.cz)
 */

#pragma once

#include "IGenerator.h"

struct Coordinate;
struct Value;
class Stats;

/*
 * Empirical cummulative dist. function (ECDF) generator class
 */
class CECDF_Generator : public IGenerator
{
	private:
		//

	protected:
		// writes plot body
		void Write_Body();
		// writes plot description (axis titles, ..)
		void Write_Description();
		// writes legend
		void Write_Legend();
		// writes ECDF curve for selected signals
		void Write_ECDF_Curve(const ValueVector& reference, const ValueVector& calculated, std::string& name);

		virtual double Normalize_X(double x) const override;
		virtual double Normalize_Y(double y) const override;

		// image start X coordinate
		static int startX;
		// image maximum X coordinate (excl. padding)
		static int maxX;
		// image maximum Y coordinate (excl. padding)
		static int maxY;
		// image limit X coordinate
		static int sizeX;
		// image limit Y coordinate
		static int sizeY;

	public:
		CECDF_Generator(DataMap &inputData, double maxValue, LocalizationMap &localization, int mmolFlag);

		static void Set_Canvas_Size(int width, int height);

		virtual std::string Build_SVG() override final;
};
