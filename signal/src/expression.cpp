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
 * Univerzitni 8
 * 301 00, Pilsen
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

#include "expression.h"
#include <cmath>


class CConstant : public virtual expression::IOperator {
protected:
    double mValue;
public:
    CConstant(const double val) : mValue(val) {};

    virtual double evaluate(const double level) override final {
        return mValue;
    }
};

class COperator : public virtual expression::IOperator {
protected:
    std::unique_ptr<expression::IOperator> mLeft, mRight;    
public:
    COperator(std::unique_ptr<expression::IOperator> left, std::unique_ptr<expression::IOperator> right) : mLeft{ std::move(left) }, mRight{ std::move(right) } {};
};


class COR : public virtual COperator {
public:    
    virtual double evaluate(const double level) override final {
        return std::isnormal(mLeft->evaluate(level)) || std::isnormal(mRight->evaluate(level)) ? 0.0 : 1.0;
    }
};


class CEqual : public virtual COperator {
public:
    virtual double evaluate(const double level) override final {
        return mLeft->evaluate(level) == mRight->evaluate(level);
    }
};



CExpression::CExpression(const std::wstring& src) {
    mOperator = Parse(src);
}

std::unique_ptr<expression::IOperator> CExpression::Parse(const std::wstring& src) {
    enum class NState {nothing, accumulating_left};

    NState state = NState::nothing;

    for (auto i = 0; i < src.size(); i++) {
        std::wstring left, right, tmp;

        switch (src[i]) {
            case '(': state = NState::accumulating_left; tmp.clear(); break;



            default: tmp += src[i];
        }

    }

    return nullptr;
}

double CExpression::evaluate(const double level) {
    return mOperator ? mOperator->evaluate(level) : std::numeric_limits<double>::quiet_NaN();
}


