/**
 * SmartCGMS - continuous glucose monitoring and controlling framework
 * https://diabetes.zcu.cz/
 *
 * Copyright (c) since 2018 University of West Bohemia.
 *
 * Contact:
 * diabetes@mail.kiv.zcu.cz
 * Medical Informatics, Department of Computer Science and Engineering
 * Faculty of Applied Sciences, University of West Bohemia
 * Univerzitni 8, 301 00 Pilsen
 * Czech Republic
 * 
 * 
 * Purpose of this software:
 * This software is intended to demonstrate work of the diabetes.zcu.cz research
 * group to other scientists, to complement our published papers. It is strictly
 * prohibited to use this software for diagnosis or treatment of any medical condition,
 * without obtaining all required approvals from respective regulatory bodies.
 *
 * Especially, a diabetic patient is warned that unauthorized use of this software
 * may result into severe injure, including death.
 *
 *
 * Licensing terms:
 * Unless required by applicable law or agreed to in writing, software
 * distributed under these license terms is distributed on an "AS IS" BASIS, WITHOUT
 * WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *
 * a) This file is available under the Apache License, Version 2.0.
 * b) When publishing any derivative work or results obtained using this software, you agree to cite the following paper:
 *    Tomas Koutny and Martin Ubl, "SmartCGMS as a Testbed for a Blood-Glucose Level Prediction and/or 
 *    Control Challenge with (an FDA-Accepted) Diabetic Patient Simulation", Procedia Computer Science,  
 *    Volume 177, pp. 354-362, 2020
 */

#pragma once

#include <ctime>
#include <map>
#include <vector>
#include <string>

#include <scgms/rtl/guid.h>
#include <scgms/iface/UIIface.h>

typedef std::map<std::string, std::string> LocalizationMap;

struct Value {
	Value(double _value = 0.0, time_t _date = 0, uint64_t _segment_id = 0);
	~Value();

	double Get_Value();
	void Set_Value(double _value);

	double value;
	time_t date;
	time_t normalize;
	uint64_t segment_id;
};

typedef std::vector<Value> ValueVector;

struct TCoordinate {
	double x, y;
};

class Stats {
	private:
		std::vector<double> mAbsuluteError;
		std::vector<double> mRelativeError;
		double mSumAbsolute;
		double mSumRelative;
		bool mIsSorted;
		bool mPrintStats;

	public:
		Stats();
		~Stats();

		void Sort();
		void Quartile_Rel(double& q0, double& q1, double& q2, double& q3, double& q4);
		double Avarage_Rel() const;
		void Quartile_Abs(double& q0, double& q1, double& q2, double& q3, double& q4);
		double Avarage_Abs() const;
		void Next_Error(double measured, double counted);

		void Set_Print_Stats(bool state);
		bool Get_Print_Stats() const;
};

struct Data {
	Data();
	~Data();
	Data(ValueVector _values, bool _visible, bool _empty, bool _calculated = false);

	bool Is_Visible();
	bool Is_Visible_Legend();

	ValueVector values;
	bool visible = false;
	bool empty = true;
	bool calculated = false;
	scgms::NSignal_Visualization visualization_style = scgms::NSignal_Visualization::smooth;
	GUID signal_id = Invalid_GUID;
	std::string identifier;
	std::string refSignalIdentifier;
	bool forceDraw = false;
	std::wstring nameAlias = L"";
	double valuesScale = 1.0;
};

typedef std::map<std::string, Data> DataMap;
