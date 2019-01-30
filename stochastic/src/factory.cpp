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

#include "factory.h"

#include "..\..\..\common\solver\solution.h"
#include "..\..\..\common\solver\fitness.h"
#include "../../../common/solver/NullMethod.h"

#include "MetaDE.h"
#include "HaltonSequence.h"
#include "DeterministicEvolution.h"


#define DSpecialized_Id_Dispatcher template <typename TSolution, typename TFitness = CFitness<TSolution>>\
class CLocal_Id_Dispatcher : public virtual CSpecialized_Id_Dispatcher<TSolution, TFitness> {\
public:\
	CLocal_Id_Dispatcher() {\
		using TMT_MetaDE = CMetaDE<TSolution, TFitness, CNullMethod<TSolution, TFitness>, 100000, 100, std::mt19937>;\
		CSpecialized_Id_Dispatcher<TSolution, TFitness>::mSolver_Id_Map[mt_metade::id] = std::bind(&Solve_By_Class<TMT_MetaDE, TSolution, TFitness>, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4);\
\
		using THalton_MetaDE = CMetaDE<TSolution, TFitness, CNullMethod<TSolution, TFitness>, 100000, 100, CHalton_Device>;\
		CSpecialized_Id_Dispatcher<TSolution, TFitness>::mSolver_Id_Map[halton_metade::id] = std::bind(&Solve_By_Class<THalton_MetaDE, TSolution, TFitness>, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4);\
\
		using TRandom_MetaDE = CMetaDE<TSolution, TFitness, CNullMethod<TSolution, TFitness>, 100000, 100, std::random_device>;\
		CSpecialized_Id_Dispatcher<TSolution, TFitness>::mSolver_Id_Map[rnd_metade::id] = std::bind(&Solve_By_Class<TRandom_MetaDE, TSolution, TFitness>, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4);\
\
		CSpecialized_Id_Dispatcher<TSolution, TFitness>::mSolver_Id_Map[halton_sequence::id] = std::bind(&Solve_By_Class<CHalton_Sequence<TSolution, TFitness>, TSolution, TFitness>, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4);\
\
		CSpecialized_Id_Dispatcher<TSolution, TFitness>::mSolver_Id_Map[deterministic_evolution::id] = std::bind(&Solve_By_Class<CDeterministic_Evolution<TSolution, TFitness>, TSolution, TFitness>, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4);\
	}\
};\



#include "../../../common/solver/dispatcher.h"


static CId_Dispatcher Id_Dispatcher;


HRESULT IfaceCalling do_solve_model_parameters(const glucose::TSolver_Setup *setup) {	
	return Id_Dispatcher.do_solve_model_parameters(setup);
}
