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

#pragma once

#include "../../../../common/iface/DeviceIface.h"
#include "../../../../common/rtl/FilterLib.h"

#include "../descriptor.h"

#include <Eigen/Dense>

#include <map>

namespace hist_ig_pred {

	using THistogram = Eigen::Array<double, 1, Band_Count, Eigen::RowMajor>;

	class CPattern {
	protected:
		size_t mBand_Idx;
		NPattern_Dir mX2, mX;
		THistogram mHistogram;		
	protected:
		NPattern_Dir dbl_2_pat(const double x) const;
	public:
		CPattern(const double current_level, double x2, double x);
		void Update(const double future_level);
		double Level() const;

		bool operator< (const CPattern &other) const;
	};
}


#pragma warning( push )
#pragma warning( disable : 4250 ) // C4250 - 'class1' : inherits 'class2::member' via dominance

class CHistogram_IG_Prediction : public virtual scgms::CBase_Filter {
protected:
	scgms::SSignal mIst;
	double mDt = 30.0*scgms::One_Minute;
protected:
	std::map<hist_ig_pred::CPattern, hist_ig_pred::CPattern> mPatterns;
	double Update_And_Predict(const double current_time, const double ig_level);//returns prediction at current_time + mDt
protected:	
	// scgms::CBase_Filter iface implementation
	virtual HRESULT Do_Execute(scgms::UDevice_Event event) override final;
	virtual HRESULT Do_Configure(scgms::SFilter_Configuration configuration, refcnt::Swstr_list& error_description) override final;
public:
	CHistogram_IG_Prediction(scgms::IFilter *output);
	virtual ~CHistogram_IG_Prediction();
};

#pragma warning( pop )