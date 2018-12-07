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

#pragma once

#include "../../../common/rtl/hresult.h"
#include "../../../common/rtl/DeviceLib.h"
#include "../../../common/rtl/referencedImpl.h"
#include "../../../common/rtl/ModelsLib.h"

#include "descriptor.h"

#include <Eigen/Dense>
#include <vector>
#include <random>

template <typename TSolution>
using TAligned_Solution_Vector = std::vector<TSolution, AlignmentAllocator<TSolution>>;


//Let us define the vectors to cope with commans in macro arguments

//typedef Eigen::Matrix<floattype, 1, 5> TDiffusion4_Vector; - would be nice, but fails with Eigen

//specialized vectors that will benefit from compile-time known size
#define TDiffusion_v2_Vector Eigen::Array<double, 1, diffusion_v2_model::param_count>
#define TSteil_Rebrin_Vector Eigen::Array<double, 1, steil_rebrin::param_count>
#define TSteil_Rebrin_Diffusion_Prediction_Vector Eigen::Array<double, 1, steil_rebrin_diffusion_prediction::param_count>
#define TDiffusion_Prediction_Vector Eigen::Array<double, 1, diffusion_prediction::param_count>

//generic vector for any unknown model
#define TGeneric_Vector Eigen::Array<double, 1, Eigen::Dynamic, Eigen::RowMajor>
		
//For the EIGEN_DERIVE macro, see https://eigen.tuxfamily.org/dox/TopicCustomizingEigen.html

//Although we could have go on with creating Eigen-array wrappers and have much cleaner code, we would
//be creating the wrappers very often. Or, we would be calling some kind of reset function often.
//This way, we eliminated such an overhead.

#define EIGEN_DERIVE_ARRAY(derived_type, inherited_type) \
class derived_type : public inherited_type, public virtual glucose::IModel_Parameter_Vector, public virtual refcnt::CNotReferenced { \
public: \
	derived_type(const derived_type &other) : inherited_type(other) {} \
	derived_type(void) : inherited_type() {} \
	typedef inherited_type Base; \
	template<typename OtherDerived> \
	derived_type(const Eigen::ArrayBase<OtherDerived>& other) : inherited_type(other) { } \
	derived_type & operator= (const derived_type& other) { \
		this->Base::operator=(other); \
		return *this; \
	} \
	template<typename OtherDerived> \
	derived_type & operator= (const Eigen::ArrayBase <OtherDerived>& other) { \
		this->Base::operator=(other); \
		return *this; \
	} \
	\
	derived_type Compose(const derived_type &) const { \
		return derived_type(*this); \
	} \
	\
	derived_type Decompose(const derived_type& src)  { \
		*this = src; \
		return derived_type(src); \
	} \
	\
	virtual HRESULT set(double *begin, double *end) final { \
		size_t i=0; \
		for (auto iter = begin; iter != end; iter++ ) {\
			operator[](i) = *iter; \
			i++; \
	    }\
		return S_OK; \
	} \
	\
	virtual HRESULT add(double *begin, double *end) final { \
		return E_NOTIMPL; \
	} \
	\
	virtual HRESULT get(double **begin, double **end) const final { \
		*begin = const_cast<double*>(data()); \
		*end = *begin + cols(); \
		return S_OK; \
	} \
    \
	virtual HRESULT empty() const override final { \
		return cols()>0 ? S_OK : S_FALSE; \
	}


#pragma warning( push )
#pragma warning( disable : 4250 ) // C4250 - 'class1' : inherits 'class2::member' via dominance

EIGEN_DERIVE_ARRAY(CDiffusion_v2_Solution, TDiffusion_v2_Vector)
};

EIGEN_DERIVE_ARRAY(CSteil_Rebrin_Solution, TSteil_Rebrin_Vector)
};

EIGEN_DERIVE_ARRAY(CSteil_Rebrin_Diffusion_Prediction_Solution, TSteil_Rebrin_Diffusion_Prediction_Vector)
};

EIGEN_DERIVE_ARRAY(CDiffusion_Prediction_Solution, TDiffusion_Prediction_Vector)
};

EIGEN_DERIVE_ARRAY(CGeneric_Solution, TGeneric_Vector)
};

#pragma warning( pop )
