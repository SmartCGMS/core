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
 * a) This file is available under the Apache License, Version 2.0.
 * b) When publishing any derivative work or results obtained using this software, you agree to cite the following paper:
 *    Tomas Koutny and Martin Ubl, "SmartCGMS as a Testbed for a Blood-Glucose Level Prediction and/or 
 *    Control Challenge with (an FDA-Accepted) Diabetic Patient Simulation", Procedia Computer Science,  
 *    Volume 177, pp. 354-362, 2020
 */

/**
 * ZCU/KIV/PPR demonstracni priklad
 * seriovy kod generujici pseudonahodnou sekvenci pomoci xorshift*
 * vcetne experimentalnich uprav
 * https://www.jstatsoft.org/article/view/v008i14
 */

#include <iostream>
#include <csignal>
#include <array>
#include <algorithm>
#include <random>
#include <bit>

#ifdef _WIN32
	#include <Windows.h>
#endif

#if ((defined(__x86_64__) || defined(_M_X64))) && defined(__INTEL_LLVM_COMPILER)
	#include <immintrin.h>

	int rdrand64_step(uint64_t* random_val) {
		return _rdrand64_step(reinterpret_cast<unsigned long long*>(random_val));
	}
#else	
	std::random_device rdrand_rdev;

	// fallback when no RDRAND is available
	int rdrand64_step(uint64_t* random_val) {
		return (static_cast<uint64_t>(rdrand_rdev()) << 32ULL) | static_cast<uint64_t>(rdrand_rdev());
	}	
#endif

#if (__cplusplus >= 201907L)
		//we have std::popcnout
#elif defined(__clang__)
	namespace std {
		template< class T >
		constexpr int popcount(T x) noexcept {
			return __builtin_popcount(x);
		}
	}
#elif defined(__GNUC__)
	namespace std {
		constexpr int popcount(uint64_t x) noexcept {
			return __builtin_popcountl(x);
		}
	}
#elif __has_include(<intrin.h>)
	#include <intrin.h>
	
	namespace std {		
		constexpr int popcount(uint64_t x) noexcept {
			return _mm_popcnt_u64(x);
		}
	}
#elif __has_include(<x86intrin.h>)
	#include <x86intrin.h>

	namespace std {
		template< class T >
		constexpr int popcount(T x) noexcept {
			return __popcount(x);
		}
	}
#else
	//platform with no support for popcount, likely old C++ and Arm
	#error ("Please, upgrade your compiler to C++20 compatible")
#endif


#undef min
#undef max

// N is a number of states we are switching between
template <size_t N>
class CXor_Shift_Star_Device {
	public:
		using result_type = uint64_t;

	protected:
		using TState = std::array<uint64_t, N>;
		static constexpr uint64_t mMultiplier = 0x2545F4914F6CDD1DULL;
		TState mState{ {} };
		TState mHistory{ {} }; // circular buffer serving as a source of pseudoentropy
		size_t mHistory_Head = 0;

		bool mHas_CPU_Entropy = false;
	protected:
		uint64_t rtdsc_index() {
			uint64_t bit_pool = mHistory[__rdtsc() % mHistory.size()];

			// in the current time quantum of the thread is not guaranteed, that rdtsc won't be called after constant time intervals
			// hence, we XOR the state into it
			for (size_t i = 0; i < mState.size(); i++) {
				bit_pool ^= mState[i];
			}

			static_assert(N < 64);
			const size_t idx = std::popcount(bit_pool) % mState.size();

			return idx;
		}

		uint64_t rand_index() {
			uint64_t result;
			rdrand64_step(&result);
			return result % mState.size();
		}

		template <bool return_high_in_low, bool has_cpu_entropy>
		uint64_t advance() {
			// to meaningfully utilize vectorization on such a simple algorithm, we have N small generators, randomly choose one of it and we return its state

			uint64_t im;
			if constexpr (has_cpu_entropy) {
				// however, if the CPU supports DRNS, we don't have to shift all the generators, one suffices

				const uint64_t idx = rand_index();

				im = mState[idx] ^ (mState[idx] >> 12);
				im ^= im << 25;
				im ^= im >> 27;
				mState[idx] = im;
				im *= mMultiplier;
			}
			else {
				// if we use N streams for pseudorandom index generating, we should shift all generators

				// if we put everything into a single loop, the compiler will refuse to autovectorize due to small portion of work
				// this will hopefully make the compiler think that one loop equals one instruction
				for (size_t i = 0; i < mState.size(); i++) {
					mState[i] ^= mState[i] >> 12;
				}

				for (size_t i = 0; i < mState.size(); i++) {
					mState[i] ^= mState[i] << 25;
				}

				for (size_t i = 0; i < mState.size(); i++) {
					mState[i] ^= mState[i] >> 27;
				}

				const uint64_t idx = rtdsc_index();
				im = mState[idx] * mMultiplier;
			}

			if constexpr (return_high_in_low) {
				return im >> 32; // lower 32 bits aren't as random as we need
			}
			else {
				return im << 32;
			}
		}

	public:
		CXor_Shift_Star_Device() {
			uint64_t local_entropy = 0;
			mHas_CPU_Entropy = true;

			//see 5.2.1 Retry Recommendations in the Intel® Digital Random Number Generator (DRNG) Software Implementation Guide 
			for (size_t i = 0; i < std::max(static_cast<size_t>(10), mState.size()); i++) {
				const bool local_success = rdrand64_step(&local_entropy) != 0;
				mHas_CPU_Entropy &= local_success;

				if (i < mState.size()) {
					if (local_success) {
						mState[i] = local_entropy;
					}
					else {
						mState[i] = __rdtsc();
					}
				}
			}

			if (!mHas_CPU_Entropy) {
				for (size_t i = 0; i < mState.size(); i++) {
					uint64_t tmp = mState[i] ^ (mState[i] >> 12);
					tmp ^= tmp << 25;
					tmp ^= tmp >> 27;
					// we don't have to worry about the lower 32 bits
					mHistory[i] = tmp * mMultiplier;
				}
			}
		}

		explicit CXor_Shift_Star_Device(const std::string& token) : CXor_Shift_Star_Device() {};
		CXor_Shift_Star_Device(const CXor_Shift_Star_Device&) = delete;

		CXor_Shift_Star_Device operator=(const CXor_Shift_Star_Device& other) = delete;

		result_type operator()() {
			// lower 32 bits aren't as random as we need, they won't pass the MatrixRank test from the BigCrush package
			// we discard them and generate twice
		
			if (mHas_CPU_Entropy) {
				return advance<true, true>() | advance<false, true>();
			}
			else {
				// CPU does not support DRNS, we have to perform slightly more work ourselves
				const uint64_t result = advance<true, false>() | advance<false, false>();

				// store the result into the history buffer
				mHistory[mHistory_Head] = result;
				if (mHistory_Head > 0) {
					mHistory_Head--;
				}
				else {
					mHistory_Head = N - 1;
				}

				return result;
			}
		}

		static constexpr result_type min() {
			return 0;
		}

		static constexpr result_type max() {
			return std::numeric_limits<result_type>::max();
		}
};
