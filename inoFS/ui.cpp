#include <vector>
#include <string>
#include <mutex>
#include "main.h"
#include "server.h"
#include "ui.h"
#include "dbg.h"

#include "ftxui/component/component.hpp"
#include "ftxui/dom/elements.hpp"
#include "ftxui/component/event.hpp"

using namespace ftxui;

UI::UI(inoFS *inofs) {
	this->inofs = inofs;
	static std::vector<std::string> tab_names = {
		"State",
		"Monitor",
		"Control",
		"Log"
	};
	static int tab = 0;
	auto menu = Menu(&tab_names, &tab);

	auto page = Container::Tab({
		Renderer([&] {return State();})
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
	screen.PostEvent(Event::Custom);
	this->loop->RunOnce();
}

Element UI::State() {
	Elements clientList;
	std::vector<Client> clients = inofs->server->GetClients();
	for (auto client = clients.begin(); client != clients.end(); ++client) {
		clientList.push_back(text(inet_ntoa(client->addr.sin_addr)));
	};
	Elements localIPs;
	std::vector<std::string> localips = inofs->server->GetLocalIPs();
	for (auto ip = localips.begin(); ip != localips.end(); ++ip) {
		localIPs.push_back(text(*ip + ":" + std::to_string(SERVER_PORT)));
	};
	return vbox({
		hbox({
			vbox({
				text("Server addresses"),
				vbox(std::move(localIPs))
			}),
			separator(),
			vbox({
				text("Clients"),
				vbox(std::move(clientList))
			})
		}),
		separator(),
		vbox({
			text("Client connected!!!!")
		})
	});
}