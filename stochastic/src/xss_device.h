/**
 * ZCU/KIV/PPR demonstracni priklad
 * seriovy kod generujici pseudonahodnou sekvenci pomoci xorshift*
 * vcetne experimentalnich uprav
 * https://www.jstatsoft.org/article/view/v008i14
 * 
 * This file is available under the Apache License, Version 2.0.
 * 
 */



#include <iostream>
#include <csignal>
#include <array>
#include <algorithm>
#include <random>

#ifdef _WIN32
	#include <Windows.h>
#endif

#if defined(__x86_64__) && !defined(_DEBUG)
	#include <immintrin.h>
#else
	int _rdrand64_step(unsigned __int64* random_val) {
		return 0;
	}
#endif


#undef min
#undef max


template <size_t N>				//N je pocet stavu, mezi kterymi budeme nahodne prepinat
class CXor_Shift_Star_Device {
protected:
	using TState = std::array<uint64_t, N>;
	static constexpr uint64_t mMultiplier = 0x2545F4914F6CDD1DULL;
	TState mState{ {} };
	TState mHistory{ {} };			//kruhovy buffer, ktery slouzi jako zdroj pseudoentropie
	size_t mHistory_Head = 0;		
protected:
	uint64_t rtdsc_index() {
		uint64_t bit_pool = mHistory[__rdtsc() % mHistory.size()];
			//v aktualnim casovem kvantu vlakna neni zaruceno, ze se rdtsc nebude volat po konstantnich intervalech
			//proto k nemu jeste prixorujeme stavy

		for (size_t i = 0; i < mState.size(); i++)
			bit_pool ^= mState[i];

		static_assert(N < 64);
		const size_t idx = __popcnt64(bit_pool) % mState.size();

		return idx;
	}

	uint64_t rand_index() {
		uint64_t result;		
		_rdrand64_step(&result);
		return result % mState.size();
	}

	template <bool return_high_in_low, bool has_cpu_entropy>
	uint64_t advance() {
		//abychom smysluplne vyuzili vektorizaci na tak jednoduchem algoritmu
		//tak mame N malych generatoru, z nich nahodne vybirame jeden, jehoz vystup pak vratime


		uint64_t im;
		if constexpr (has_cpu_entropy) {
			//nicmene, pokud cpu podporuje DRNG, tak nemusime posouvat vsechny generatory, ale staci nam jenom jeden

			const uint64_t idx = rand_index();

			im = mState[idx] ^ (mState[idx] >> 12);
			im ^= im << 25;
			im ^= im >> 27;
			mState[idx] = im;
			im *= mMultiplier;
		}
		else {
			//pokud ale pouzivame n streamu pro generovani pseudonahodneho indexu, tak bychom proudy meli posouvat vsechny			

			//kdybychom vse dali do jednoho cyklu, tak prekladac odmitne autovektorizovat s tim, ze je prilis malo prace
			//takto pochopi kazdou radku jako ekvivalent jedne instrukce
			for (size_t i = 0; i < mState.size(); i++)
				mState[i] ^= mState[i] >> 12;

			for (size_t i = 0; i < mState.size(); i++)
				mState[i] ^= mState[i] << 25;

			for (size_t i = 0; i < mState.size(); i++)
				mState[i] ^= mState[i] >> 27;

			const uint64_t idx = rtdsc_index();
			im = mState[idx] * mMultiplier;
		}
		

		if constexpr (return_high_in_low)
			return im >> 32;		//nisich 32 bitu neni uplne nahodnych
		else
			return im << 32;
	}
protected:
	bool mHas_CPU_Entropy = false;
public:
	using result_type = uint64_t;	
public:
	CXor_Shift_Star_Device() {
		uint64_t local_entropy = 0;
		mHas_CPU_Entropy = true;
		for (size_t i = 0; i < std::max(10ull, mState.size()); i++) {	//see 5.2.1 Retry Recommendations in the Intel� Digital Random Number Generator (DRNG) Software Implementation Guide 
			const bool local_success = _rdrand64_step(&local_entropy) != 0;
			mHas_CPU_Entropy &= local_success;

			if (i < mState.size()) {
				if (local_success)
					mState[i] = local_entropy;
				else
					mState[i] = __rdtsc();				
			}
		}

		if (!mHas_CPU_Entropy) {
			for (size_t i = 0; i < mState.size(); i++) {			
				uint64_t tmp = mState[i] ^ (mState[i] >> 12);
				tmp ^= tmp << 25;
				tmp ^= tmp >> 27;
				mHistory[i] = tmp * mMultiplier;	//nepotrebujeme prave ted se starat o nizsich 32bitu
			}
		}
	}

	explicit CXor_Shift_Star_Device(const std::string& token) : CXor_Shift_Star_Device() {};
	CXor_Shift_Star_Device(const CXor_Shift_Star_Device&) = delete;

	CXor_Shift_Star_Device operator= (const CXor_Shift_Star_Device& other) = delete;

	result_type operator()() {
		//nizsich 32 bitu neni uplne nahodnych, neprojdou MatrixRank test z baliku BigCrush
		//=> zahodime je a generujeme 2x
		
		if (mHas_CPU_Entropy) {
			return advance<true, true>() | advance<false, true>();
		} else {
			//cpu nema podporu DRNG, takze musime udelat vic prace sami
			const uint64_t result = advance<true, false>() | advance<false, false>();

			//vysledek si jeste ulozime do historie
			mHistory[mHistory_Head] = result;
			if (mHistory_Head > 0)
				mHistory_Head--;
			else
				mHistory_Head = N - 1;

			return result;
		}		
	}

	static constexpr result_type min() { return 0; }
	static constexpr result_type max() { return std::numeric_limits<result_type>::max(); }
};