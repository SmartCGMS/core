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

#include "PlotBackgroundMap.h"

/** General **/

#include <array>

const std::array<TCoordinate, 9> Coords_a = { { TCoordinate{70, 0}, TCoordinate{70, 56}, TCoordinate{400, 320}, TCoordinate{400, 400},
										   TCoordinate{400 / 1.2, 400}, TCoordinate{70, 84}, TCoordinate{175 / 3.0, 70},
										   TCoordinate{0, 70}, TCoordinate{0, 0} } };

const std::array<TCoordinate, 4> Coords_b_up = { { TCoordinate{70, 84}, TCoordinate{400 / 1.2, 400}, TCoordinate{290, 400}, TCoordinate{70, 180} } };

const std::array<TCoordinate, 8> Coords_b_down = { { TCoordinate{130, 0}, TCoordinate{180 , 70}, TCoordinate{240, 70}, TCoordinate{240, 180},
												TCoordinate{400, 180}, TCoordinate{400, 320}, TCoordinate{70,56}, TCoordinate{70,0} } };

const std::array<TCoordinate, 3> Coords_c_up = { { TCoordinate{70, 180}, TCoordinate{290, 400}, TCoordinate{70, 400} } };

const std::array<TCoordinate, 3> Coords_c_down = { { TCoordinate{180,0}, TCoordinate{180,70}, TCoordinate{130,0} } };

const std::array<TCoordinate, 5> Coords_d_up = { { TCoordinate{0, 70}, TCoordinate{175 / 3.0, 70}, TCoordinate{70,84}, TCoordinate{70,180}, TCoordinate{0,180} } };

const std::array<TCoordinate, 4> Coords_d_down = { { TCoordinate{240, 70}, TCoordinate{400, 70}, TCoordinate{400,180}, TCoordinate{240,180} } };

const std::array<TCoordinate, 4> Coords_e_up = { { TCoordinate{70, 180}, TCoordinate{70,400}, TCoordinate{0,400}, TCoordinate{0,180} } };

const std::array<TCoordinate, 4> Coords_e_down = { { TCoordinate{180,0}, TCoordinate{400, 0}, TCoordinate{400,70}, TCoordinate{180,70} } };

/** Parkes **/

const std::array<TCoordinate, 12> Coords_a_t1 = { { TCoordinate{50, 0}, TCoordinate{50, 30}, TCoordinate{170, 145}, TCoordinate{385, 300}, TCoordinate{550,450}, TCoordinate{550, 550},
		TCoordinate{430, 550}, TCoordinate{280,380}, TCoordinate{140, 170}, TCoordinate{30, 50}, TCoordinate{0,50}, TCoordinate{0,0} } };

const std::array<TCoordinate, 10> Coords_b_up_t1 = { { TCoordinate{0, 50}, TCoordinate{30, 50}, TCoordinate{140, 170}, TCoordinate{280,380}, TCoordinate{430, 550}, TCoordinate{260,550}, TCoordinate{70,110},
		TCoordinate{50,80}, TCoordinate{30,60}, TCoordinate{0,60} } };

const std::array<TCoordinate, 9> Coords_b_down_t1 = { { TCoordinate{120, 0}, TCoordinate{120,30}, TCoordinate{260, 130}, TCoordinate{550, 250}, TCoordinate{550, 450},
		TCoordinate{385, 300}, TCoordinate{170, 145}, TCoordinate{50, 30}, TCoordinate{50, 0} } };

const std::array<TCoordinate, 10> Coords_c_up_t1 = { { TCoordinate{0, 60}, TCoordinate{30,60}, TCoordinate{50,80}, TCoordinate{70,110}, TCoordinate{260,550}, TCoordinate{125, 550}, TCoordinate{80, 215},
		TCoordinate{50, 125}, TCoordinate{25, 100}, TCoordinate{0,100} } };

const std::array<TCoordinate, 7> Coords_c_down_t1 = { { TCoordinate{250, 0}, TCoordinate{250, 40}, TCoordinate{550, 150}, TCoordinate{550, 250}, TCoordinate{260, 130}, TCoordinate{120, 30}, TCoordinate{120, 0} } };

const std::array<TCoordinate, 8> Coords_d_up_t1 = { { TCoordinate{0, 100}, TCoordinate{25, 100}, TCoordinate{50, 125}, TCoordinate{80, 215}, TCoordinate{125, 550}, TCoordinate{50,550}, TCoordinate{35,155}, TCoordinate{0,150} } };

const std::array<TCoordinate, 4> Coords_d_down_t1 = { { TCoordinate{550, 0}, TCoordinate{550, 150}, TCoordinate{250, 40}, TCoordinate{250, 0} } };

const std::array<TCoordinate, 4> Coords_e_up_t1 = { { TCoordinate{0, 150}, TCoordinate{35,155}, TCoordinate{50,550}, TCoordinate{0,550} } };

/** diabetes 2 **/

const std::array<TCoordinate, 11> Coords_a_t2 = { { TCoordinate{50, 0}, TCoordinate{50, 30}, TCoordinate{90, 80}, TCoordinate{330, 230}, TCoordinate{550,450}, TCoordinate{550, 550},
		TCoordinate{440, 550}, TCoordinate{230,330}, TCoordinate{30, 50}, TCoordinate{0,50}, TCoordinate{0,0} } };

const std::array<TCoordinate, 7> Coords_b_up_t2 = { { TCoordinate{0, 50}, TCoordinate{30, 50}, TCoordinate{230,330}, TCoordinate{440, 550}, TCoordinate{280,550},
		TCoordinate{30,60}, TCoordinate{0,60} } };

const std::array<TCoordinate, 8> Coords_b_down_t2 = { { TCoordinate{90, 0}, TCoordinate{260, 130}, TCoordinate{550, 250}, TCoordinate{550,450}, TCoordinate{330, 230},
		TCoordinate{90, 80}, TCoordinate{50, 30}, TCoordinate{50, 0} } };

const std::array<TCoordinate, 7> Coords_c_up_t2 = { { TCoordinate{0, 60}, TCoordinate{30,60}, TCoordinate{280,550}, TCoordinate{125, 550}, TCoordinate{35, 90}, TCoordinate{25, 80}, TCoordinate{0, 80} } };

const std::array<TCoordinate, 7> Coords_c_down_t2 = { { TCoordinate{250, 0}, TCoordinate{250, 40}, TCoordinate{410,110}, TCoordinate{550, 160}, TCoordinate{550, 250}, TCoordinate{260, 130}, TCoordinate{90, 0} } };

const std::array<TCoordinate, 7> Coords_d_up_t2 = { { TCoordinate{0, 80}, TCoordinate{25, 80}, TCoordinate{35, 90}, TCoordinate{125, 550}, TCoordinate{50,550}, TCoordinate{35,200}, TCoordinate{0,200} } };

const std::array<TCoordinate, 6> Coords_d_down_t2 = { { TCoordinate{550, 160}, TCoordinate{410,110}, TCoordinate{250, 40}, TCoordinate{250,0}, TCoordinate{550,0}, TCoordinate{550,160} } };

const std::array<TCoordinate, 4> Coords_e_up_t2 = { { TCoordinate{0, 200}, TCoordinate{35,200}, TCoordinate{50,550}, TCoordinate{0,550} } };
