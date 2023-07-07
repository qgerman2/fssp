#ifndef UI_h
#define UI_h
#include <string>
#include "ftxui/component/loop.hpp"

const std::string logo1 = "   ____ ____ ____ ___ ";
const std::string logo2 = "  / __// __// __// _ \\";
const std::string logo3 = " / _/ _\\ \\ _\\ \\ / ___/";
const std::string logo4 = "/_/  /___//___//_/    ";
                            
class inoFS;
class UI {
	private:
		inoFS *inofs;
		ftxui::Loop *loop;
	public:
		UI(inoFS *inofs);
		void Loop();
};
#endif