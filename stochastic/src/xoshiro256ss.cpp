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


#include "xoshiro256ss.h"

 /*  Written in 2018 by David Blackman and Sebastiano Vigna (vigna@acm.org)

 To the extent possible under law, the author has dedicated all copyright
 and related and neighboring rights to this software to the public domain
 worldwide. This software is distributed without any warranty.

 See <http://creativecommons.org/publicdomain/zero/1.0/>. */

 /* This is xoshiro256** 1.0, our all-purpose, rock-solid generator. It has
	excellent (sub-ns) speed, a state (256 bits) that is large enough for
	any parallel application, and it passes all tests we are aware of.

	For generating just floating-point numbers, xoshiro256+ is even faster.

	The state must be seeded so that it is not everywhere zero. If you have
	a 64-bit seed, we suggest to seed a splitmix64 generator and use its
	output to fill s. */


uint64_t split_mix(uint64_t x) {
	uint64_t z = (x += 0x9e3779b97f4a7c15);
	z = (z ^ (z >> 30)) * 0xbf58476d1ce4e5b9;
	z = (z ^ (z >> 27)) * 0x94d049bb133111eb;
	return z ^ (z >> 31);
}

static inline uint64_t rotl(const uint64_t x, int k) {
	return (x << k) | (x >> (64 - k));
}

Cxoshiro256ss::Cxoshiro256ss() {
	mState[0] = split_mix(409);
	mState[1] = split_mix(419);
	mState[2] = split_mix(421);
	mState[3] = split_mix(431);
}

Cxoshiro256ss::Cxoshiro256ss(const Cxoshiro256ss& other) {
	mState = other.mState;	

	long_jump();
}

Cxoshiro256ss Cxoshiro256ss::operator= (const Cxoshiro256ss &other) {
	mState = other.mState;

	long_jump();

	return *this;
}


Cxoshiro256ss::result_type Cxoshiro256ss::operator()() {
	const uint64_t result_starstar = rotl(mState[1] * 5, 7) * 9;

	const uint64_t t = mState[1] << 17;

	mState[2] ^= mState[0];
	mState[3] ^= mState[1];
	mState[1] ^= mState[2];
	mState[0] ^= mState[3];

	mState[2] ^= t;

	mState[3] = rotl(mState[3], 45);

	return result_starstar;
}


void Cxoshiro256ss::jump() {
	static const uint64_t JUMP[] = { 0x180ec6d33cfd0aba, 0xd5a61266f0c9392c, 0xa9582618e03fc9aa, 0x39abdc4529b1661c };

	uint64_t s0 = 0;
	uint64_t s1 = 0;
	uint64_t s2 = 0;
	uint64_t s3 = 0;
	for (int i = 0; i < sizeof JUMP / sizeof *JUMP; i++)
		for (int b = 0; b < 64; b++) {
			if (JUMP[i] & static_cast<uint64_t>(1) << b) {
				s0 ^= mState[0];
				s1 ^= mState[1];
				s2 ^= mState[2];
				s3 ^= mState[3];
			}
			operator()();
		}

	mState[0] = s0;
	mState[1] = s1;
	mState[2] = s2;
	mState[3] = s3;
}

void Cxoshiro256ss::long_jump() {
	static const uint64_t LONG_JUMP[] = { 0x76e15d3efefdcbbf, 0xc5004e441c522fb3, 0x77710069854ee241, 0x39109bb02acbe635 };

	uint64_t s0 = 0;
	uint64_t s1 = 0;
	uint64_t s2 = 0;
	uint64_t s3 = 0;
	for (int i = 0; i < sizeof LONG_JUMP / sizeof *LONG_JUMP; i++)
		for (int b = 0; b < 64; b++) {
			if (LONG_JUMP[i] & static_cast<uint64_t>(1) << b) {
				s0 ^= mState[0];
				s1 ^= mState[1];
				s2 ^= mState[2];
				s3 ^= mState[3];
			}
			operator()();
		}

	mState[0] = s0;
	mState[1] = s1;
	mState[2] = s2;
	mState[3] = s3;
}
