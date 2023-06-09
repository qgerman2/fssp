#ifndef UI_h
#define UI_h
#include <string>
#include <vector>
#include "ftxui/component/screen_interactive.hpp"
#include "ftxui/component/loop.hpp"
#include "ftxui/component/component.hpp"
#include "ftxui/dom/elements.hpp"

#define dprintf inofs->ui->Print

using namespace ftxui;

static Element logo = vbox({
	text("   _           ________"),
	text("  (_)__  ___  / __/ __/"),
	text(" / / _ \\/ _ \\/ _/_\\ \\"), 
	text("/_/_//_/\\___/_/ /___/")
});

// I wish these weren't globals
// but everything dies if they are not
static const int print_max = 10;
static std::vector<std::string> logs;
static std::mutex print_mutex;

class inoFS;
class UI {
	private:
		inoFS *inofs;
		ScreenInteractive screen = ScreenInteractive::Fullscreen();
		Loop *loop;
		Component State();
		Element Clients();
		Element Comms();
		Element Log();
		std::vector<std::string> comms;
	public:
		UI(inoFS *inofs);
		void Loop();
		void Print(const char *format, ...);
		void PrintComms(const char *format, ...);
};
#endif