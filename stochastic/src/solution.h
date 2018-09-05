/**
 * SmartCGMS - continuous glucose monitoring and controlling framework
 * https://diabetes.zcu.cz/
 *
 * Contact:
 * diabetes@mail.kiv.zcu.cz
 * Medical Informatics, Department of Computer Science and Engineering
 * Faculty of Applied Sciences, University of West Bohemia
 * Technicka 8
 * 314 06, Pilsen
 *
 * Licensing terms:
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
 * WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *
 * a) For non-profit, academic research, this software is available under the
 *    GPLv3 license. When publishing any related work, user of this software
 *    must:
 *    1) let us know about the publication,
 *    2) acknowledge this software and respective literature - see the
 *       https://diabetes.zcu.cz/about#publications,
 *    3) At least, the user of this software must cite the following paper:
 *       Parallel software architecture for the next generation of glucose
 *       monitoring, Proceedings of the 8th International Conference on Current
 *       and Future Trends of Information and Communication Technologies
 *       in Healthcare (ICTH 2018) November 5-8, 2018, Leuven, Belgium
 * b) For any other use, especially commercial use, you must contact us and
 *    obtain specific terms and conditions for the use of the software.
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

EIGEN_DERIVE_ARRAY(CGeneric_Solution, TGeneric_Vector)
};

#pragma warning( pop ) 