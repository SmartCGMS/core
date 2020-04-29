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

#include "../../../../common/rtl/Eigen_Buffer.h"

#include <Eigen/Dense>

#include <bitset>
 
 //https://github.com/yixuan/MiniDNN
class CIdentity {
public:
    template <typename TZ, typename TA>
    static void Activate(const TZ& Z, TA& A) { A = Z; };
};

class CReLU {
public:
    template <typename TZ, typename TA>
    static void Activate(const TZ& Z, TA& A) { A.array() = Z.array().max(0.0); };
};

class CTanH {
public:
    template <typename TZ, typename TA>
    static void Activate(const TZ& Z, TA& A) { A.array() = Z.array().tanh(); };
};

class CSigmoid {
public:
    template <typename TZ, typename TA>
    static void Activate(const TZ& Z, TA& A) { A.array() = 1.0 / (1.0 + (-Z.array()).exp()); };
};

class CSoft_Max {
public:
    template <typename TZ, typename TA>
    static void Activate(const TZ& Z, TA& A) {
        using RowArray = Eigen::Array<double, 1, TA::ColsAtCompileTime>;

        A.array() = (Z.rowwise() - Z.colwise().maxCoeff()).array().exp();
        RowArray  colsums = A.colwise().sum();
        A.array().rowwise() /= colsums;
    };
};


template <size_t input_size>
class CNeural_Ouput {
public:
    static const size_t Input_Size = input_size;
    using TInput = Eigen::Matrix<double, 1, input_size>;
public:
    constexpr static size_t Parameter_Count() { return 0; }
    bool Initialize(const double* data, const size_t available_count) { return true; }
};

template <size_t input_size>
class CMulti_Class_Ouput : public CNeural_Ouput<input_size> {
public:
    using TFinal_Output = size_t;
public:    
    TFinal_Output Forward(const CNeural_Ouput<input_size>::TInput& input) const {
        TFinal_Output max_index = 0;
        double max_val = input[0];
        for (auto i = 1; i < CNeural_Ouput<input_size>::TInput::ColsAtCompileTime; i++) {
            if (input[i] > max_val) {
                max_val = input[i];
                max_index = i;
            }
        }        

        return max_index;
    }    
};

template <size_t input_size>
class CMulti_Label_Ouput : public CNeural_Ouput<input_size> {
public:    
    using TFinal_Output = std::bitset<input_size>;
public:
    TFinal_Output Forward(const CNeural_Ouput<input_size>::TInput& input) const {
        TFinal_Output result;
        for (auto i = 0; i < CNeural_Ouput<input_size>::TInput::ColsAtCompileTime; i++)
            if (input[i] >= 0.5) result.set(i);

        return result;
    }    
};

template <size_t input_size, typename TActivation, typename TNext_Layer>
class CNeural_Layer {
public:
    using TInput = Eigen::Matrix<double, 1, input_size>;
    using TFinal_Output = typename TNext_Layer::TFinal_Output;
    static const size_t Input_Size = input_size;
protected:
    using TWeights = Eigen::Matrix<double, TNext_Layer::Input_Size, input_size>;
protected:
    TNext_Layer mNext_Layer;
    TWeights mWeights;
    TInput mBias;
public:
    TFinal_Output Forward(const TInput& input) const {
        typename TNext_Layer::TInput activated;
        TActivation::Activate((input + mBias) * mWeights, activated);
        return mNext_Layer.Forward(activated);
    }

    void Learn(const TInput& input, const TFinal_Output& output) {

    }


    bool Initialize(const double *data = nullptr, const size_t available_count = 0) {
        if (data) {
            const size_t required_count = input_size + input_size * TNext_Layer::Input_Size;
            if (available_count >= required_count) {
                mBias = Map_Double_To_Eigen<TInput>(data);
                mWeights = Map_Double_To_Eigen<TWeights>(data + input_size);

                return mNext_Layer.Initialize(data + required_count, available_count - required_count);
            } else
                return false;
        }
        else {
            mBias.setRandom(TInput::RowsAtCompileTime, TInput::ColsAtCompileTime);
            mWeights.setRandom(TWeights::RowsAtCompileTime, TWeights::ColsAtCompileTime);
            return mNext_Layer.Initialize(nullptr, 0);
        }                
    }

    constexpr static size_t Parameter_Count() {
        return   input_size                             //size of bias
               + input_size*TNext_Layer::Input_Size     //number of weights
               + TNext_Layer::Parameter_Count();
    }
};