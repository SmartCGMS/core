#include "Misc.h"

#include <sstream>

void Discard_BOM_If_Present(std::istream& is)
{
	uint8_t bom[4];
	memset(bom, 0, 4);

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
