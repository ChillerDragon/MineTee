/*
    BASIC ANTI-CHEATS SYSTEM FOR TEEWORLDS (SERVER SIDE)
    By unsigned char*
*/
#ifndef H_ANTICHEATS
#define H_ANTICHEATS
#include <string>

namespace twac
{

	class Funcs
	{
	public:
		static const char* TWACVersion();
		static std::string CensoreString(std::string in);
	};

}

#endif // H_ANTICHEATS
