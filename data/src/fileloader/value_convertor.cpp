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
 * a) For non-profit, academic research, this software is available under the
 *      GPLv3 license.
 * b) For any other use, especially commercial use, you must contact us and
 *       obtain specific terms and conditions for the use of the software.
 * c) When publishing work with results obtained using this software, you agree to cite the following paper:
 *       Tomas Koutny and Martin Ubl, "Parallel software architecture for the next generation of glucose
 *       monitoring", Procedia Computer Science, Volume 141C, pp. 279-286, 2018
 */

#include "value_convertor.h"


bool CValue_Convertor::init(const std::string& expression_string) {
	mExpression_String = expression_string;

	mValid = mSymbol_Table.add_variable("x", mValue);
	if (mValid) {
		mExpression_Tree.register_symbol_table(mSymbol_Table);
		mValid = mParser.compile(expression_string, mExpression_Tree);
	}


}
double CValue_Convertor::eval(const double val) {
	double result = std::numeric_limits<double>::quiet_NaN();

	if (mValid) {
		mValue = val;
		result = mExpression_Tree.value();
	}

	return result;
}

bool CValue_Convertor::valid() const {
	return mValid;
}

CValue_Convertor& CValue_Convertor::operator=(const CValue_Convertor& other) {
	mSymbol_Table = other.mSymbol_Table;
	mExpression_Tree = other.mExpression_Tree;
	mExpression_String = other.mExpression_String;
	mValid = mParser.compile(mExpression_String, mExpression_Tree);

	return *this;
}