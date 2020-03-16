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

#include <memory>
#include <string>
#include <type_traits>

#include "../../../../common/rtl/DeviceLib.h"
#include "../../../../common/utils/string_utils.h"

namespace expression {
    
    union TIntermediate {
        double dval;
        bool bval;
    };

    class CAST_Node {
    public:
        virtual ~CAST_Node() {};
        virtual TIntermediate evaluate(const scgms::UDevice_Event &event) = 0;
    };
    
    template <typename T>
    class CConstant : public virtual CAST_Node {
    protected:
        T mValue;
    public:
        CConstant(T value) : mValue(value) {};
        virtual TIntermediate evaluate(const scgms::UDevice_Event& event) override final {
            TIntermediate result;

            if constexpr (std::is_floating_point<T>::value) result.dval = static_cast<T>(mValue);
                else if constexpr (std::is_same<T, bool>::value) result.bval = mValue;
                    else static_assert(true, "Unsupported type!");

            return result; 
        };
    };

    class CDouble : public virtual CConstant<double> {    
    public:
        CDouble(const char* str) : CConstant<double>(str_2_dbl(str)) { };
    };

    class CNot : public virtual CAST_Node {
    protected:
        std::unique_ptr<CAST_Node> mExpr;
    public:        
        CNot(CAST_Node* expr) : mExpr(expr) {};
        virtual TIntermediate evaluate(const scgms::UDevice_Event& event) override final {
          TIntermediate result;
          result.bval = !(mExpr->evaluate(event).bval);
          return result; 
        } 
    };


    #define DSpecialized_Operator(name, result_arg, operand_arg, oper) \
        class name : public virtual CAST_Node { \
        protected: \
           std::unique_ptr<CAST_Node> mLeft, mRight; \
        public: \
           name(CAST_Node* left, CAST_Node* right)  : mLeft(left), mRight(right) {}; \
           virtual TIntermediate evaluate(const scgms::UDevice_Event &event) override final { \
             TIntermediate result; \
             result.result_arg = mLeft->evaluate(event).operand_arg oper mRight->evaluate(event).operand_arg; \
             return result; \
           } \
        };
    

    DSpecialized_Operator(CPlus, dval, dval, +)
    DSpecialized_Operator(CMinus, dval, dval, -)
    DSpecialized_Operator(CMul, dval, dval, *)
    DSpecialized_Operator(CDiv, dval, dval, /)

    DSpecialized_Operator(CLT, bval, dval, <)
    DSpecialized_Operator(CLTEQ, bval, dval, <= )
    DSpecialized_Operator(CEQ, bval, dval, == )
    DSpecialized_Operator(CNEQ, bval, dval, !=)
    DSpecialized_Operator(CGT, bval, dval, > )
    DSpecialized_Operator(CGTEQ, bval, dval, >= )    

    DSpecialized_Operator(CAND, bval, bval, && )
    DSpecialized_Operator(COR, bval, bval, ||)
    DSpecialized_Operator(CXOR, bval, bval, ^)



    #define DSpecialized_Variable(operand_arg, result_arg) \
        class CVariable_##operand_arg : public virtual CAST_Node { \
        public: \
            virtual TIntermediate evaluate(const scgms::UDevice_Event& event) override final { \
                TIntermediate result; result.result_arg = event.operand_arg(); return result; \
            }; \
        };

    DSpecialized_Variable(level, dval)
    DSpecialized_Variable(is_level_event, bval)

}

using CExpression = std::unique_ptr<expression::CAST_Node>;

CExpression Parse_AST_Tree(const std::wstring& wstr, refcnt::Swstr_list& error_description);