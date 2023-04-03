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

#include "gct3_transfer_functions.h"
#include "gct3_moderation_functions.h"
#include "gct3_integrators.h"

#include <vector>
#include <list>
#include <memory>
#include <algorithm>
#include <cassert>
#include <functional>

#include "../../../../common/utils/DebugHelper.h"

namespace gct3_model
{
	class CDepot;

	// select integrator
	using TDefault_Integrator = CGaussian_Quadrature_Integrator;

	/**
	 * Link between two depots
	 */
	class CDepot_Link {

		private:
			/**
			 * Moderator record
			 */
			struct TModerator {
				// moderator depot
				std::reference_wrapper<CDepot> depot;
				// moderation function
				std::unique_ptr<CModeration_Function> function;
			};

		private:
			// transfer function modelling the transfer between depots
			std::unique_ptr<CTransfer_Function> mTransfer_Function;
			// time of last step (checkpoint)
			double mLast_Time = std::numeric_limits<double>::quiet_NaN();

			// source side of depot link
			std::reference_wrapper<CDepot> mSource;
			// target side of depot link
			std::reference_wrapper<CDepot> mTarget;

			// list of moderators
			std::list<TModerator> mModerators;

			// coefficient for unit conversion during transfer
			double mTransfer_Unit_Coefficient = 1.0;
			// shift for unit conversion during transfer
			double mTransfer_Unit_Shift = 0.0;

			// selected integrator
			std::unique_ptr<IIntegrator> mIntegrator;

		protected:
			template<typename TTransferFnc, typename... Args>
			CDepot_Link(std::unique_ptr<TTransferFnc> transferFunc, CDepot& source, CDepot& target)
				: mTransfer_Function(std::move(transferFunc)),
				mSource(source), mTarget(target), mIntegrator(mTransfer_Function->Create_Integrator()) {
				//
			}

		public:
			// factory method
			template<typename TTransferFnc, typename... Args>
			static CDepot_Link Create(CDepot& source, CDepot& target, Args... args) {
				return CDepot_Link(std::make_unique<TTransferFnc>(args...), source, target);
			}

			// prohibit copy (conserve mass on architectural level)
			CDepot_Link(const CDepot_Link& other) = delete;
			CDepot_Link& operator=(const CDepot_Link& other) = delete;

			// move (to allow list/vector erase)
			CDepot_Link(CDepot_Link&& other) noexcept : mTransfer_Function(std::move(other.mTransfer_Function)), mLast_Time(other.mLast_Time), mSource(other.mSource),
				mTarget(other.mTarget), mModerators(std::move(other.mModerators)), mTransfer_Unit_Coefficient(other.mTransfer_Unit_Coefficient), mTransfer_Unit_Shift(other.mTransfer_Unit_Shift), mIntegrator(std::move(other.mIntegrator)) {
				//
			}
			CDepot_Link& operator=(CDepot_Link&& other) noexcept {
				mTransfer_Function = std::move(other.mTransfer_Function);
				mLast_Time = other.mLast_Time;
				mSource = other.mSource;
				mTarget = other.mTarget;
				mModerators = std::move(other.mModerators);
				mTransfer_Unit_Coefficient = other.mTransfer_Unit_Coefficient;
				mTransfer_Unit_Shift = other.mTransfer_Unit_Shift;
				mIntegrator = std::move(other.mIntegrator);
				return *this;
			}

			// adds moderator to this link
			template<typename TModFunc, typename... Args>
			TModFunc& Add_Moderator(CDepot& moderator, Args... args) {
				mModerators.push_back(TModerator{ moderator, std::make_unique<TModFunc>(std::forward<Args>(args)...) });
				return *dynamic_cast<TModFunc*>(mModerators.rbegin()->function.get());
			}

			// adds unit conversion to this link - the transfered amount is modified half-way in the process; old value is subtracted, then converted and then added to destination depot
			void Set_Unit_Converter(double coefficient, double shift = 0.0) {
				mTransfer_Unit_Coefficient = coefficient;
				mTransfer_Unit_Shift = shift;
			}

			void Init(const double currentTime) {
				mLast_Time = currentTime;
			}

			// steps link to given time
			void Step(const double currentTime);

			// commits last requested step
			void Commit(const double currentTime) {
				mLast_Time = currentTime;
			}

			// is this link expired? i.e.; has the transfer function reached its end?
			bool Is_Expired(double time) const {
				return mTransfer_Function->Is_Expired(time);
			}

			CDepot_Link& Set_Source_Quantity_Dependent(bool dependent) {
				mTransfer_Function->Set_Source_Quantity_Dependent(dependent);
				return *this;
			}
	};

	/**
	 * Depot of some quantity
	 * - quantity may flow in and out, it may get depleted and based on settings, depletion may lead to depot deletion (e.g.; depot of consumed meal)
	 * - a depot is always contained in a compartment
	 * - every depot has 0..N links to another depots
	 */
	class CDepot : public IQuantizable {

		public:
			// callback for adding a moderator to link
			using TModerator_Add_Fnc = std::function<void(CDepot_Link& link)>;

		private:
			// current depot quantity
			double mQuantity = 0.0;

			// initial depot quantity
			double mInitial_Quantity = 0.0;

			// next step quantity
			double mNext_Quantity = 0.0;

			// do we allow negative amounts to be present? Should be false for physical depots (e.g.; amount of glucose in plasma distribution volume, ...)
			bool mAllow_Negative = false;

			// is this depot persistent? (e.g.; glucose in plasma, ...)
			bool mPersistent = false;

			// list of links to another depot
			std::list<CDepot_Link> mLinks;

			// current depot volume (to be able to calculate concentration); we assume unit volume until changed
			double mSolution_Volume = 1.0;

			// current time
			double mCurrent_Time = 0.0;

			// for debugging purposes - depot name
			std::wstring mName;

		protected:
			void Internal_Set_Quantity(double quantity) {
				mQuantity = quantity;
				mNext_Quantity = quantity;
			}

		public:
			/**
			 * Modify quantity by a given amount
			 * The depot may "refuse" to give/take some of it; 'total' is always modified to reflect the change on the other side of the link
			 * e.g.; total == -20, we have > 20, we decrease our quantity by 20 and set total = 20 (we give away '20' units)
			 * e.g.; total == 20, we have anything, we increase our quantity by 20 and set total = -20 (we take away '20' units)
			 * e.g.; total == -20, we have 10 and !mAllow_Negative, we decrease our quantity by 10 and set total = 10 (we give away '10' units)
			 * this is needed to conserve mass
			 */
			virtual void Mod_Quantity(double &total) {
				if (mQuantity < -total && !mAllow_Negative) {
					total = mQuantity;	// give away what we have
					mNext_Quantity = 0.0;	// set quantity to 0 as we depleted our quantity
				}
				else {
					mNext_Quantity += total;	// modify quantity by given amount
					total = -total;		// indicate successful transfer
				}
			}

			/**
			 * Returns quantity, which has previously been taken away by Decrease_Quantity
			 * this is needed to conserve mass
			 */
			virtual void Return_Quantity(const double amount) {
				mNext_Quantity += amount;
			}

		public:
			CDepot(double initial_quantity = 0.0, bool allow_negative = false) : mQuantity(initial_quantity), mAllow_Negative(allow_negative) {
				if (mQuantity < 0 && !mAllow_Negative) {
					mQuantity = 0;
				}
				mInitial_Quantity = mQuantity;
				mNext_Quantity = mQuantity;
			}

			// prohibit copy; try to respect conservation of mass even on architectural level
			CDepot(const CDepot& other) = delete;
			CDepot& operator=(const CDepot& other) = delete;

			// support move; try to respect conservation of mass even on architectural level
			CDepot(CDepot&& other) noexcept : mQuantity(other.mQuantity), mInitial_Quantity(other.mInitial_Quantity), mNext_Quantity(other.mNext_Quantity), mAllow_Negative(other.mAllow_Negative), mPersistent(other.mPersistent), mSolution_Volume(other.mSolution_Volume), mCurrent_Time(other.mCurrent_Time) {
				other.mQuantity = 0.0;
				other.mNext_Quantity = 0.0;
			}
			CDepot& operator=(CDepot&& other) noexcept {
				mQuantity = other.mQuantity;
				mInitial_Quantity = other.mInitial_Quantity;
				mNext_Quantity = other.mNext_Quantity;
				mAllow_Negative = other.mAllow_Negative;
				mPersistent = other.mPersistent;
				mSolution_Volume = other.mSolution_Volume;
				mCurrent_Time = other.mCurrent_Time;

				other.mQuantity = 0.0;
				return *this;
			}

			// marks depot as (non-)persistent; this means it does (not) get deleted after transfer ends
			CDepot& Set_Persistent(bool state) {
				mPersistent = state;
				return *this;
			}

			// is this depot marked as persistent?
			bool Is_Persistent() const {
				return mPersistent;
			}

			// sets volume of this solution (distribution volume of this depot)
			CDepot& Set_Solution_Volume(double volume) {
				mSolution_Volume = volume;
				return *this;
			}

			// get volume of this solution
			double Get_Solution_Volume() const {
				return mSolution_Volume;
			}

			virtual double Get_Quantity() const override {
				return mQuantity;
			}

			virtual double Get_Initial_Quantity() const {
				return mInitial_Quantity;
			}

			virtual double Get_Concentration() const override {
				return mQuantity / mSolution_Volume;
			}

			CDepot& Set_Name(const std::wstring& name) {
				mName = name;
				return *this;
			}

			const std::wstring& Get_Name() const {
				return mName;
			}

			double Get_Current_Time() const {
				return mCurrent_Time;
			}

			void Init(const double currentTime) {

				mCurrent_Time = currentTime;

				for (auto& link : mLinks) {
					link.Init(currentTime);
				}
			}

			// steps depot to current time
			void Step(const double currentTime) {
				// step every link
				for (auto& link : mLinks) {
					link.Step(currentTime);
				}
			}

			// commits quantity from last requested step (two-phase stepping to avoid loss)
			void Commit(const double currentTime) {
				mQuantity = mNext_Quantity;

				for (auto& lnk : mLinks)
					lnk.Commit(currentTime);

				// erase expired links
				mLinks.erase(std::remove_if(mLinks.begin(), mLinks.end(),
					[currentTime](const CDepot_Link& link) {
						return link.Is_Expired(currentTime);
					}),
					mLinks.end()
				);

				mCurrent_Time = currentTime;
			}

			// add link from this depot to another; first type argument represents type of transfer function, variable arguments are passed to its constructor
			template<typename TTransferFnc, typename... Args>
			CDepot_Link& Link_To(CDepot& target, Args... args) {

				auto link = CDepot_Link::Create<TTransferFnc, Args...>(*this, target, std::forward<Args>(args)...);
				link.Init(mCurrent_Time);

				mLinks.push_back(std::move(link));

				return *mLinks.rbegin();
			}

			// adds moderated link from this depot to another; first type argument represents type of transfer function, variable arguments are passed to its constructor
			template<typename TTransferFnc, typename... Args>
			CDepot_Link& Moderated_Link_To(CDepot& target, TModerator_Add_Fnc addCallback, Args... args) {

				auto link = CDepot_Link::Create<TTransferFnc, Args...>(*this, target, std::forward<Args>(args)...);
				link.Init(mCurrent_Time);
				addCallback(link);

				mLinks.push_back(std::move(link));

				return *mLinks.rbegin();
			}

			// should this depot be deleted?
			bool Is_Finished() const {
				return !mPersistent && mLinks.empty();
			}
	};

	/**
	 * Sink depot - acts as a target for substance disappearance (utilization, etc.)
	 * Always accepts all incoming amount
	 */
	class CSink_Depot : public CDepot {

		protected:
			virtual void Mod_Quantity(double& total) override {
				if (total > 0.0)
					total = -total;
			}

		public:
			using CDepot::CDepot;
	};

	/**
	 * Source depot - acts as a source for substance appearance (unlimited production, etc.)
	 * Always gives all requested amount
	 */
	class CSource_Depot : public CDepot {

		protected:
			virtual void Mod_Quantity(double& total) override {
				if (total < 0.0)
					total = -total;
			}

		public:
			using CDepot::CDepot;
	};

	/**
	 * Externally driven source depot - always gives and takes requested amount, quantity can be set externally
	 */
	class CExternal_State_Depot : public CDepot {

		protected:
			virtual void Mod_Quantity(double& total) override {
				total = -total;
			}

		public:
			void Set_Quantity(double quantity) {
				Internal_Set_Quantity(quantity);
			}

			using CDepot::CDepot;
	};

	/**
	 * Externally driven source depot, that changes the amount based on time of the day (circadian cycle depots)
	 */
	class CExternal_Circadian_State_Depot : public CDepot {

		private:
			std::vector<std::pair<double, double>> mKnots;

		protected:
			virtual void Mod_Quantity(double& total) override {
				total = -total;
			}

		public:
			CExternal_Circadian_State_Depot(double default_quantity, bool allow_negative) : CDepot(default_quantity, allow_negative) {
				//
			}

			void Clear_Knots() {
				mKnots.clear();
			}

			void Add_Knot(double timeOfTheDay, double value) {
				mKnots.push_back({ timeOfTheDay, value });

				std::sort(mKnots.begin(), mKnots.end(), [](const auto& a, const auto& b) {
					return a.first < b.first;
				});
			}

			virtual double Get_Quantity() const override {
				if (mKnots.size() < 2)
					return CDepot::Get_Quantity();

				const double curTime = Get_Current_Time();
				double tod = curTime - static_cast<long>(curTime);

				// for now, interpolate linearly between knots; we require 2 knots at minimum

				size_t low = 0;
				size_t high = 0;

				for (size_t i = 0; i < mKnots.size() - 1; i++) {
					if (mKnots[i].first <= tod && mKnots[i + 1].first >= tod) {
						low = i;
						high = i + 1;
						break;
					}
				}

				if (low == high) {
					if (tod <= mKnots[0].first || tod >= mKnots[mKnots.size() - 1].first) {
						low = mKnots.size() - 1;
						high = 0;
					}
					else // should not happen
						return CDepot::Get_Quantity();
				}

				const double tstart = mKnots[low].first;
				double tend = mKnots[high].first;
				const double vstart = mKnots[low].second, vend = mKnots[high].second;

				if (tstart > tend) {
					tend += 1.0;
					if (tod < tstart)
						tod += 1.0;
				}

				return vstart + (vend - vstart) * ( (tod - tstart) / (tend - tstart) );
			}

			using CDepot::CDepot;
	};

	/**
	 * Compartment grouping 1..N depots
	 */
	class CCompartment : public IQuantizable {

		protected:
			// depots within this compartment
			std::vector<std::unique_ptr<CDepot>> mDepots;

			// current time
			double mCurrent_Time = 0.0;

		public:
			CCompartment() = default;

			// prohibit copy; conservation of mass on architectural level
			CCompartment(const CCompartment& other) = delete;
			CCompartment& operator=(const CCompartment& other) = delete;

			// support move; conservation of mass on architectural level
			CCompartment(CCompartment&& other) noexcept : mDepots(std::move(other.mDepots)) {
				//
			}
			CCompartment& operator=(CCompartment& other) noexcept {
				mDepots = std::move(other.mDepots);
				return *this;
			}

			double Get_Quantity() const override {
				double result = 0.0;

				for (const auto& depot : mDepots)
					result += depot->Get_Quantity();

				return result;
			}

			double Get_Solution_Volume() const {
				double result = 0.0;

				for (const auto& depot : mDepots)
					result += depot->Get_Solution_Volume();

				return result;
			}

			double Get_Concentration() const override {
				return Get_Quantity() / Get_Solution_Volume();
			}

			void Init(const double currentTime) {
				mCurrent_Time = currentTime;

				for (auto& depot : mDepots) {
					depot->Init(currentTime);
				}
			}

			// steps all depots in compartment to given time
			void Step(const double currentTime) {
				for (auto& depot : mDepots) {
					depot->Step(currentTime);
				}
			}

			// commits changes in all depots and erases depots at the end of their lifetime
			void Commit(const double currentTime) {
				for (auto& depot : mDepots) {
					depot->Commit(currentTime);
				}

				// erase useless depots
				mDepots.erase(std::remove_if(mDepots.begin(), mDepots.end(),
					[](const std::unique_ptr<CDepot>& depot) {
						return depot->Is_Finished();
					}),
					mDepots.end()
				);
			}

			// has this compartment a persistent depot
			bool Has_Persistent_Depot() const {
				for (auto& depot : mDepots) {
					if (depot->Is_Persistent()) {
						return true;
					}
				}

				return false;
			}

			// retrieves persistent depot in this compartment; throws an exception if no persistent depot found
			CDepot& Get_Persistent_Depot() {
				for (auto& depot : mDepots) {
					if (depot->Is_Persistent()) {
						return *depot;
					}
				}

				throw std::runtime_error{ "No persistent depot found" };
			}

			// creates a depot in this compartment, returns a reference to it; first type argument represents depot type, variable args are passed as depot constructor params
			template<typename TDepot = CDepot, typename... Args>
			TDepot& Create_Depot(Args... args) {
			
				auto ptr = std::make_unique<TDepot>(std::forward<Args>(args)...);
				TDepot& res = *ptr; // pointer/reference will remain valid

				res.Init(mCurrent_Time);

				mDepots.push_back(std::move(ptr));

				return res;
			}

			decltype(mDepots)::const_iterator begin() const {
				return mDepots.begin();
			}

			decltype(mDepots)::const_iterator end() const {
				return mDepots.end();
			}
	};

}
