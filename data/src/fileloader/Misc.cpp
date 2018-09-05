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

#include "Misc.h"

#include <sstream>
#include <cstring>

void Discard_BOM_If_Present(std::istream& is)
{
	uint8_t bom[4];
	std::memset(bom, 0, 4);

	is.read((char*)bom, 4);

	// EF BB BF / BF BB EF = UTF-8 big/little endian
	if ((bom[0] == 0xEF && bom[1] == 0xBB && bom[2] == 0xBF) ||
		(bom[0] == 0xBF && bom[1] == 0xBB && bom[2] == 0xEF))
	{
		is.seekg(-1, std::ios::cur);
		return;
	}
	// FE FF / FF FE = UTF-16 big/little endian
	else if ((bom[0] == 0xFE && bom[1] == 0xFF) ||
		(bom[0] == 0xFF && bom[1] == 0xFE))
	{
		is.seekg(-2, std::ios::cur);
		return;
	}
	// 00 00 FE FF / FF FE 00 00 = UTF-32 big/little endian
	else if ((bom[0] == 0x00 && bom[1] == 0x00 && bom[2] == 0xFE && bom[3] == 0xFF) ||
		(bom[0] == 0xFF && bom[1] == 0xFE && bom[2] == 0x00 && bom[3] == 0x00))
	{
		return;
	}

	// no BOM in input file, reset istream to original state and return
	is.seekg(-4, std::ios::cur);
}
