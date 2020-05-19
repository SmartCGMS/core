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

#include "neural_net_descriptor.h"

#include <Eigen/Dense>

#include <vector>

class CNeural_Prediction_Data {
protected:
    //experimental
    //https://stackoverflow.com/questions/10657503/find-running-median-from-a-stream-of-integers
    //https://stackoverflow.com/questions/1309263/rolling-median-algorithm-in-c
    //https://stackoverflow.com/questions/10930732/c-efficiently-calculating-a-running-median
    //https://www.johndcook.com/blog/skewness_kurtosis/
    
    double mCount = 0.0;
    double mRunning_Avg = std::numeric_limits<double>::quiet_NaN();
    double mRunning_Median = std::numeric_limits<double>::quiet_NaN();
protected:
    //diagnostics
    bool mDump_Params = true;
    double mRunning_Variance = std::numeric_limits<double>::quiet_NaN();    
public:
    CNeural_Prediction_Data();

    void Update(const double level);
    double Level() const;
    bool Valid() const;

    void Dump_Params() const;
};