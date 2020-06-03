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

namespace neural_net {

    //https://blog.demofox.org/2017/03/09/how-to-train-neural-networks-with-backpropagation/
    //https://www.guivi.one/2019/03/19/neural-network-with-eigen-lib-19/
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
    class CNeural_Terminal {
    public:
        static const size_t Input_Size = input_size;
        using TInput = Eigen::Matrix<double, input_size, 1>;
    public:
        constexpr static size_t Parameter_Count() { return 0; }
        bool Initialize(const double* data, const size_t available_count) { return true; }       
    };

    template <size_t input_size>
    class CMulti_Class_Terminal : public CNeural_Terminal<input_size> {
    public:
        using TFinal_Output = size_t;
    public:
        TFinal_Output Forward(const typename CNeural_Terminal<input_size>::TInput& input) const {
            TFinal_Output max_index = 0;
            double max_val = input[0];
            for (auto i = 1; i < CNeural_Terminal<input_size>::TInput::RowsAtCompileTime; i++) {
                if (input[i] > max_val) {
                    max_val = input[i];
                    max_index = i;
                }
            }

            return max_index;
        }

        template <typename TOptimizer>
        typename CNeural_Terminal<input_size>::TInput Backpropagate(const typename CNeural_Terminal<input_size>::TInput& input, const TFinal_Output& reference_output, TOptimizer& optimizer) {
            //error is ideal_activation - activation 
            //ideal activation has all elements set to zero, except the one at the target position
            typename CNeural_Terminal<input_size>::TInput error = 0.0 - input;
            error[reference_output] = 1.0 - input[reference_output];
            return input;
        }
    };

    template <size_t input_size>
    class CMulti_Label_Terminal : public CNeural_Terminal<input_size> {
    public:
        using TFinal_Output = std::bitset<input_size>;
    public:
        TFinal_Output Forward(const typename CNeural_Terminal<input_size>::TInput& input) const {
            TFinal_Output result{ 0 };
            for (auto i = 0; i < CNeural_Terminal<input_size>::TInput::RowsAtCompileTime; i++)
                if (input[i] >= 0.5) result.set(i);

            return result;
        }

        template <typename TOptimizer>
        typename CNeural_Terminal<input_size>::TInput Backpropagate(const typename CNeural_Terminal<input_size>::TInput& input, const TFinal_Output& reference_output, TOptimizer& optimizer) {
            typename CNeural_Terminal<input_size>::TInput error;

            //error is ideal_activation - activation 
            //ideal activation has all elements set to the target's bits
            for (auto i = 0; i < CNeural_Terminal<input_size>::TInput::RowsAtCompileTime; i++)
                error[i] = (reference_output[i] ? 1.0 : 0.0) - input[i];

            return error;
        }
    };

    template <size_t input_size, typename TActivation, typename TNext_Layer>
    class CNeural_Layer {
    public:
        using TInput = Eigen::Matrix<double, input_size, 1>;
        using TOutput = typename TNext_Layer::TInput;
        using TFinal_Output = typename TNext_Layer::TFinal_Output;
        static const size_t Input_Size = input_size;
    protected:
        using TWeights = Eigen::Matrix<double, TNext_Layer::Input_Size, input_size>;
    protected:
        TNext_Layer mNext_Layer;
        TWeights mWeights;
        TOutput mBias;
    public:
        TFinal_Output Forward(const TInput& input) const {
            return mNext_Layer.Forward(Intermediate_Output(input));
        }

        TOutput Intermediate_Output(const TInput& input) const {
            TOutput activated;
            TActivation::Activate(mWeights*input + mBias, activated);
            return activated;
        }


        template <typename TOptimizer>
        TInput Backpropagate(const TInput& input, const TFinal_Output& reference_output, TOptimizer& optimizer) {
            const TOutput activated = Intermediate_Output(input);
            // dCost/dIntermediateOutput is the return value of our backprop method,
            // thus, it is calculated by the next layer
            const TOutput next_dCost_by_dIntermediateOutput = mNext_Layer.Backpropagate(activated, reference_output, optimizer);
            
            const TOutput dA_by_dZ = activated.cwiseProduct((1.0 - activated.array()).matrix());
            const TOutput /*dCost_by_dZ is dBias*/ dBias = next_dCost_by_dIntermediateOutput.cwiseProduct(dA_by_dZ);

            TWeights dWeight;   //dWeight.array() = dBias.rowwise() * input.array(); but how?
            for (auto i = 0; i < TOutput::RowsAtCompileTime; i++) {
                dWeight.row(i) = dBias(i) * input;
            }

            
            
            const TInput dCost_by_dIntermediateOutput = mWeights.transpose() * dBias;

            optimizer.Update(mWeights, dWeight);
            optimizer.Update(mBias, dBias);


            return dCost_by_dIntermediateOutput;
        }


        bool Initialize(const double* data = nullptr, const size_t available_count = 0) {
            if (data) {
                const size_t required_count = input_size + input_size * TNext_Layer::Input_Size;
                if (available_count >= required_count) {
                    mBias = Map_Double_To_Eigen<TOutput>(data);
                    mWeights = Map_Double_To_Eigen<TWeights>(data + input_size);

                    return mNext_Layer.Initialize(data + required_count, available_count - required_count);
                }
                else
                    return false;
            }
            else {
                mBias.setRandom(TInput::RowsAtCompileTime, TInput::ColsAtCompileTime);
                mWeights.setRandom(TWeights::RowsAtCompileTime, TWeights::ColsAtCompileTime);
                return mNext_Layer.Initialize(nullptr, 0);
            }
        }

        constexpr static size_t Parameter_Count() {
            return   TNext_Layer::Input_Size                  //size of bias
                   + input_size * TNext_Layer::Input_Size     //number of weights
                   + TNext_Layer::Parameter_Count();
        }
    };

    template <size_t output_size, typename TActivation = CSigmoid>
    using CMulti_Label_Output = CNeural_Layer<output_size, TActivation, CMulti_Label_Terminal<output_size>>;

    template <size_t output_size, typename TActivation = CSoft_Max>
    using CMulti_Class_Output = CNeural_Layer<output_size, TActivation, CMulti_Class_Terminal<output_size>>;

    template <typename TFirst_Layer, typename TOptimizer>
    class CNeural_Network {
    protected:
        TOptimizer mOptimizer;
        TFirst_Layer mFirst_Layer;
    public:
        using TInput = typename TFirst_Layer::TInput;
        using TFinal_Output = typename TFirst_Layer::TFinal_Output;
    public:
        //no data means init with random values
        bool Initialize(const double* data = nullptr, const size_t available_count = 0) {
            return mFirst_Layer.Initialize(data, available_count);
        }

        constexpr static size_t Parameter_Count() {
            return TFirst_Layer::Parameter_Count();
        }


        typename TFirst_Layer::TFinal_Output Forward(const TInput& input) {
            return mFirst_Layer.Forward(input);
        }

        void Learn(const typename TInput& input, const typename TFirst_Layer::TFinal_Output& reference_output) {
            mFirst_Layer.Backpropagate(input, reference_output, mOptimizer);
        }
    };


    class CSGD {
    protected:
        double mLearning_Rate = 0.001;
        double mDecay = 0.0;
    public:
        template <typename TTarget, typename TGradient>
        void Update(TTarget& target, const TGradient& gradient) {
            target.noalias() -= mLearning_Rate * (gradient + mDecay * target);
        }
    };
}