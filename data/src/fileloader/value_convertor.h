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

#include <string>
#include <memory>

#include "../third party/exprtk.hpp"

class CValue_Convertor {
	protected:
		enum class NValue_Conversion {
			identity,
			f_2_c,
			mg_dl_2_mmol_l,
			sleep_quality,
			general,
			minutes,
			seconds,
			invalid,
		};

		NValue_Conversion mConversion = NValue_Conversion::invalid;

		//most of the time, there won't by any conversion => it is worth to allocate it only when needed to save the overall init time
		struct TExpression_Engine {
			exprtk::symbol_table<double> mSymbol_Table;
			exprtk::expression<double>   mExpression_Tree;
			exprtk::parser<double>       mParser;
		};

		std::unique_ptr<TExpression_Engine> mEngine;

		std::string mExpression_String;
		double mValue = std::numeric_limits<double>::quiet_NaN();		//eval value placeholder due to the expretk lib design

	public:
		CValue_Convertor() {};
		CValue_Convertor(const CValue_Convertor& other);

		bool init(const std::string& expression);
		double eval(const double val); //may return nan if cannot eval
		bool valid() const; //true if expression is correct

		CValue_Convertor& operator=(const CValue_Convertor& other);
};
