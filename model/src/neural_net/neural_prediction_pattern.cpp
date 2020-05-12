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

#include "neural_prediction_pattern.h"

#include "neural_net_descriptor.h"

#undef min
#undef max

CNeural_Prediction_Data::CNeural_Prediction_Data() {
    mHistogram.setZero();
}

void CNeural_Prediction_Data::Update(const double level, const double delta) {
    const size_t band_idx =  const_neural_net::Level_2_Band_Index(level);
    mHistogram(band_idx) += 1.0;

    if (!std::isnan(delta)) {
        if (std::isnan(mMax_Increase_Delta)) mMax_Increase_Delta = delta;
            else mMax_Increase_Delta = std::max(delta, mMax_Increase_Delta);

        if (std::isnan(mMax_Decrease_Delta)) mMax_Decrease_Delta = delta;
            else mMax_Decrease_Delta = std::min(delta, mMax_Decrease_Delta);
    }
}

double CNeural_Prediction_Data::Level() const {
    const double area = mHistogram.sum();
    double moments = 0.0;
    for (auto i = 0; i < mHistogram.cols(); i++) {
        moments += mHistogram(i) * static_cast<double>(i);
    }

    const size_t result_idx = static_cast<size_t>(std::round(moments / area));

    return const_neural_net::Band_Index_2_Level(result_idx);
}


double CNeural_Prediction_Data::Apply_Max_Delta(const double calculated_level, const double reference_level) {
    double result = calculated_level;
    if (!std::isnan(mMax_Increase_Delta)) result = std::max(calculated_level, reference_level + 2.0*mMax_Increase_Delta);
    if (!std::isnan(mMax_Decrease_Delta)) result = std::min(calculated_level, reference_level + 2.0*mMax_Decrease_Delta);

    return result;
}