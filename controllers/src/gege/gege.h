#pragma once

#include "../../../../common/rtl/FilterLib.h"
#include "../../../../common/rtl/referencedImpl.h"
#include "../../../../common/rtl/UILib.h"

#include "../descriptor.h"
#include "gege_ast.h"

#pragma warning( push )
#pragma warning( disable : 4250 ) // C4250 - 'class1' : inherits 'class2::member' via dominance

class CGEGE_Model : public virtual scgms::CBase_Filter, public virtual scgms::IDiscrete_Model
{
	private:
		gege::TParameters mParameters;

		uint64_t mSegment_id = scgms::Invalid_Segment_Id;

		// we have to hold the current timestamp
		double mCurrent_Time = 0;

		double mLast_Meal = 0;
		double mLast_Meal_Size = 0;

		bool mHas_IG = false;
		std::array<double, 5> mPast_IG;
		size_t mPast_IG_Cursor = 0;

		gege::CContext mContext;

	protected:
		// scgms::CBase_Filter iface implementation
		virtual HRESULT Do_Execute(scgms::UDevice_Event event) override final;
		virtual HRESULT Do_Configure(scgms::SFilter_Configuration configuration, refcnt::Swstr_list& error_description) override final;

		void Advance_Model(double time_advance_delta);
		bool Emit_IBR(double level, double time);

	public:
		CGEGE_Model(scgms::IModel_Parameter_Vector* parameters, scgms::IFilter* output);
		virtual ~CGEGE_Model() = default;

		// scgms::IDiscrete_Model iface
		virtual HRESULT IfaceCalling Initialize(const double new_current_time, const uint64_t segment_id) override final;
		virtual HRESULT IfaceCalling Step(const double time_advance_delta) override final;
};

#pragma warning( pop )

