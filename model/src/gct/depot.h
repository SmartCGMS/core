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

#include "transfer_functions.h"

#include <vector>
#include <memory>
#include <algorithm>
#include <cassert>
#include <functional>

#include "../../../../common/utils/DebugHelper.h"

class CDepot;

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
			// "how much" moderator quantity influences transfer; TODO: this will be reworked to a moderation function
			double moderation_factor;
			// "how much" of moderation eliminates the moderator amount; TODO: this will be reworked to a moderation function
			double moderator_elimination_factor;

			// TODO: moderation function (linear, exponential, thresholding, ...)
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

	protected:
		template<typename TTransferFnc, typename... Args>
		CDepot_Link(std::unique_ptr<TTransferFnc> transferFunc, CDepot& source, CDepot& target)
			: mTransfer_Function(std::move(transferFunc)),
			mSource(source), mTarget(target) {
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
			mTarget(other.mTarget), mModerators(std::move(other.mModerators)) {
			//
		}
		CDepot_Link& operator=(CDepot_Link&& other) noexcept {
			mTransfer_Function = std::move(other.mTransfer_Function);
			mLast_Time = other.mLast_Time;
			mSource = other.mSource;
			mTarget = other.mTarget;
			mModerators = std::move(other.mModerators);
			return *this;
		}

		void Add_Moderator(CDepot& moderator, double modFactor, double modEliminationFactor) {
			mModerators.push_back(TModerator{ moderator, modFactor, modEliminationFactor });
		}

		void Step(const double currentTime);

		bool Is_Expired(double time) const {
			return mTransfer_Function->Is_Expired(time);
		}
};

/**
 * Depot of some quantity
 * - quantity may flow in and out, it may get depleted and based on settings, depletion may lead to depot deletion (e.g.; depot of consumed meal)
 * - a depot is always contained in a compartment
 * - every depot has 0..N links to another depots
 */
class CDepot : public IQuantizable {

	private:
		// current depot quantity
		double mQuantity = 0.0;

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
			assert(amount >= 0.0 && "CDepot::Return_Quantity: attempt to add a negative amount");

			mNext_Quantity += amount;
		}

	public:
		CDepot(double initial_quantity = 0.0, bool allow_negative = false) : mQuantity(initial_quantity), mAllow_Negative(allow_negative) {
			if (mQuantity < 0 && !mAllow_Negative) {
				mQuantity = 0;
			}
			mNext_Quantity = mQuantity;
		}

		// prohibit copy; try to respect conservation of mass even on architectural level
		CDepot(const CDepot& other) = delete;
		CDepot& operator=(const CDepot& other) = delete;

		// support move; try to respect conservation of mass even on architectural level
		CDepot(CDepot&& other) noexcept : mQuantity(other.mQuantity), mNext_Quantity(other.mNext_Quantity), mAllow_Negative(other.mAllow_Negative) {
			other.mQuantity = 0.0;
			other.mNext_Quantity = 0.0;
		}
		CDepot& operator=(CDepot&& other) noexcept {
			mQuantity = other.mQuantity;
			mNext_Quantity = other.mNext_Quantity;
			mAllow_Negative = other.mAllow_Negative;

			other.mQuantity = 0.0;
			return *this;
		}

		void Set_Persistent(bool state) {
			mPersistent = state;
		}

		bool Is_Persistent() const {
			return mPersistent;
		}

		void Set_Solution_Volume(double volume) {
			mSolution_Volume = volume;
		}

		double Get_Solution_Volume() const {
			return mSolution_Volume;
		}

		virtual double Get_Quantity() const override {
			return mQuantity;
		}

		virtual double Get_Concentration() const override {
			return mQuantity / mSolution_Volume;
		}

		void Step(const double currentTime) {
			// step every link
			for (auto& link : mLinks) {
				link.Step(currentTime);
			}
		}

		void Commit(const double currentTime) {
			mQuantity = mNext_Quantity;

			// erase expired links
			mLinks.erase(std::remove_if(mLinks.begin(), mLinks.end(),
				[currentTime](const CDepot_Link& link) {
					return link.Is_Expired(currentTime);
				}),
				mLinks.end()
			);
		}

		template<typename TTransferFnc, typename... Args>
		bool Link_To(CDepot& target, Args... args) {

			mLinks.push_back(CDepot_Link::Create<TTransferFnc, Args...>(*this, target, std::forward<Args>(args)...));

			return true;
		}

		using TModerator_Add_Fnc = std::function<void(CDepot_Link& link)>;

		template<typename TTransferFnc, typename... Args>
		bool Moderated_Link_To(CDepot& target, TModerator_Add_Fnc addCallback, Args... args) {

			auto link = CDepot_Link::Create<TTransferFnc, Args...>(*this, target, std::forward<Args>(args)...);
			addCallback(link);

			mLinks.push_back(std::move(link));

			return true;
		}

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
 * Compartment grouping 1..N depots
 */
class CCompartment : public IQuantizable {

	protected:
		// depots within this compartment
		std::vector<std::unique_ptr<CDepot>> mDepots;

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

		void Step(const double currentTime) {
			for (auto& depot : mDepots) {
				depot->Step(currentTime);
			}
		}

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

		bool Has_Persistent_Depot() const {
			for (auto& depot : mDepots) {
				if (depot->Is_Persistent()) {
					return true;
				}
			}
		}

		CDepot& Get_Persistent_Depot() {
			for (auto& depot : mDepots) {
				if (depot->Is_Persistent()) {
					return *depot;
				}
			}

			throw std::runtime_error{ "No persistent depot found" };
		}

		template<typename TDepot = CDepot, typename... Args>
		CDepot& Create_Depot(Args... args) {
			
			auto ptr = std::make_unique<TDepot>(std::forward<Args>(args)...);
			TDepot& res = *ptr; // pointer/reference will remain valid

			mDepots.push_back(std::move(ptr));

			return res;
		}
};
