#include <iostream>
#include "main.h"
#include "ui.h"

#include <vector>
#include <memory>  // for shared_ptr, __shared_ptr_access
#include <string>  // for operator+, to_string

#include "ftxui/component/captured_mouse.hpp"  // for ftxui
#include "ftxui/component/component.hpp"  // for Button, Horizontal, Renderer
#include "ftxui/component/component_base.hpp"      // for ComponentBase
#include "ftxui/component/component_options.hpp"   // for ButtonOption
#include "ftxui/component/screen_interactive.hpp"  // for ScreenInteractive
#include "ftxui/dom/elements.hpp"  // for gauge, separator, text, vbox, operator|, Element, border
#include "ftxui/screen/color.hpp"  // for Color, Color::Blue, Color::Green, Color::Red


using namespace ftxui;



UI::UI(FSSP *fssp) {
	this->fssp = fssp;
	static auto screen = ScreenInteractive::TerminalOutput();

	static std::vector<std::string> tab_names = {
		"State",
		"Monitor",
		"Control",
		"Log"
	};
	static int tab = 0;
	auto menu = Menu(&tab_names, &tab);

	auto logo = vbox({
		text(logo1),
		text(logo2),
		text(logo3),
		text(logo4)
	});

	auto state = vbox({
		hbox({
			vbox({
				text("IP Address: "),
				text("127.0.0.1")
			}),
			separator(),
			vbox({
				text("Connected clients: "),
				text(":27077")
			})
		}),
		separator(),
		vbox({
			text("00:18:80 A client connected")
		})
	});
	
	auto page = Container::Tab({
		Renderer([=] {return state;}),
		Renderer([] {return text("miau2");}),
		Renderer([] {return text("miau3");}),
		Renderer([] {return text("miau4");}),
	}, &tab);

	auto dom = Container::Horizontal({
		menu,
		page
	});

	auto render = Renderer(dom, [=] {
		return vbox({
			logo,
			hbox({
				menu->Render() | border,
				page->Render() | border
			})
		});
	});
	
	
	this->loop = new ftxui::Loop(&screen, render);
}

void UI::Loop() {
	this->loop->RunOnce();
}