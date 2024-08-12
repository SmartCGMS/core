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

#include "value_convertor.h"

#include <scgms/iface/DeviceIface.h>

CValue_Convertor::CValue_Convertor(const CValue_Convertor& other) {
	operator=(other);
}

bool CValue_Convertor::init(const std::string& expression_string) {

	//the following are known conversions, which we detect to speed things up
	//must be spaceless!
	const std::string conv_F_2_C = "(x-32)/1.8";
	const std::string conv_Mg_dL_2_mmol_L = "x/18.0182";
	const std::string conv_Sleep_Quality = "0.01*x";
	const std::string conv_minutes = "x/(24*60)";
	const std::string conv_seconds = "x/(24*60*60)";

	std::string spaceless_expression = expression_string;
	spaceless_expression.erase(std::remove_if(spaceless_expression.begin(), spaceless_expression.end(), isspace), spaceless_expression.end());

	if (expression_string.empty()) {
		mConversion = NValue_Conversion::identity;
	}
	else if (expression_string == conv_F_2_C) {
		mConversion = NValue_Conversion::f_2_c;
	}
	else if (expression_string == conv_Mg_dL_2_mmol_L) {
		mConversion = NValue_Conversion::mg_dl_2_mmol_l;
	}
	else if (expression_string == conv_Sleep_Quality) {
		mConversion = NValue_Conversion::sleep_quality;
	}
	else if (expression_string == conv_minutes) {
		mConversion = NValue_Conversion::minutes;
	}
	else if (expression_string == conv_seconds) {
		mConversion = NValue_Conversion::seconds;
	}
	else {
		mConversion = NValue_Conversion::general;
	}

	mExpression_String = expression_string;

	return true;
}

double CValue_Convertor::eval(const double val) {
	double result = std::numeric_limits<double>::quiet_NaN();

	auto do_general_conversion = [this, val, &result]() {

		//we do lazy initialization because the engine used so far is incredibly slow when initializing in the debug mode
		if (!mEngine) {
			mConversion = NValue_Conversion::invalid;
			mEngine = std::make_unique<TExpression_Engine>();
			if (mEngine) {
				if (mEngine->mSymbol_Table.add_variable("x", mValue)) {
					mEngine->mExpression_Tree.register_symbol_table(mEngine->mSymbol_Table);
					if (mEngine->mParser.compile(mExpression_String, mEngine->mExpression_Tree)) {
						mConversion = NValue_Conversion::general;
					}
				}

				if (mConversion == NValue_Conversion::invalid) {
					mEngine.reset();
				}
			}
		}

		if (mConversion == NValue_Conversion::general) {
			mValue = val;
			result = mEngine->mExpression_Tree.value();
		}
	};

	switch (mConversion) {
		case NValue_Conversion::identity:
			result = val;
			break;

		case NValue_Conversion::f_2_c:
			result = (val - 32.0) * 5.0 / 9.0;
			break;

		case NValue_Conversion::mg_dl_2_mmol_l:
			result = val * scgms::mgdL_2_mmolL;
			break;

		case NValue_Conversion::sleep_quality:
			result = val * 0.01;
			break;

		case NValue_Conversion::minutes:
			result = val * scgms::One_Minute;
			break;

		case NValue_Conversion::seconds:
			result = val * scgms::One_Second;
			break;

		case NValue_Conversion::general:
			do_general_conversion();
			break;

		default:
			break;	//result is already set to nan
	}

	return result;
}

bool CValue_Convertor::valid() const {
	return mConversion != NValue_Conversion::invalid;
}

CValue_Convertor& CValue_Convertor::operator=(const CValue_Convertor& other) {	
	init(other.mExpression_String);
	
	return *this;
}
