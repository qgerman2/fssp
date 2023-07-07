#ifndef UI_h
#define UI_h
#include <string>
#include "ftxui/component/screen_interactive.hpp"
#include "ftxui/component/loop.hpp"
#include "ftxui/dom/elements.hpp"

using namespace ftxui;

static Element logo = vbox({
	text("   _           ________"),
	text("  (_)__  ___  / __/ __/"),
	text(" / / _ \\/ _ \\/ _/_\\ \\"), 
	text("/_/_//_/\\___/_/ /___/")
});
                            
class inoFS;
class UI {
	private:
		inoFS *inofs;
		ScreenInteractive screen = ScreenInteractive::TerminalOutput();
		Loop *loop;
		Element State();
	public:
		UI(inoFS *inofs);
		void Loop();
};
#endif