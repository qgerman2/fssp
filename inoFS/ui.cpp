#include <vector>
#include <string>
#include <mutex>
#include <cstdarg>
#include "main.h"
#include "server.h"
#include "ui.h"
#include "dbg.h"

#include "ftxui/component/component.hpp"
#include "ftxui/dom/elements.hpp"
#include "ftxui/component/event.hpp"
#include "ftxui/screen/screen.hpp"

using namespace ftxui;

UI::UI(inoFS *inofs) {
	this->inofs = inofs;

	Screen::Cursor cursor = screen.cursor();
	cursor.shape = cursor.Hidden;
	screen.SetCursor(cursor);

	static std::vector<std::string> tab_names = {
		"State",
		"Clients",
		"Log"
	};
	static int tab = 0;
	auto menu = Menu(&tab_names, &tab);

	auto page = Container::Tab({
		Renderer([&] {return State();}),
		Renderer([&] {return Clients();}),
		Renderer([&] {return Log();})
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
	this->loop = new ftxui::Loop(&screen, std::move(render));
}

void UI::Loop() {
	screen.PostEvent(Event::Custom);
	this->loop->RunOnce();
}

void UI::Print(const char *format, ...) {
	va_list args;
	char msgBuf[2048];
	va_start(args, format);
	int bytes = vsprintf(msgBuf, format, args);
	va_end(args);
	OutputDebugStringA(msgBuf);
	if (console.size() > console_max) {
		console.erase(console.begin());
	}
	console.push_back(std::string(msgBuf, bytes));
}

Element UI::State() {
	std::vector<Client> clients = inofs->server->GetClients();
	Elements localIPs;
	std::vector<std::string> localips = inofs->server->GetLocalIPs();
	for (auto ip = localips.begin(); ip != localips.end(); ++ip) {
		localIPs.push_back(text(*ip + ":" + std::to_string(SERVER_PORT)));
	};
	std::string simStatus = "Not connected to flight-sim";
	if (inofs->sim->isConnected()) {
		simStatus = "Connected to flight-sim!";
	};
	return vbox({
		hbox({
			vbox({
				text("Server IPs:"),
				vbox(std::move(localIPs))
			}),
			separator(),
			vbox({
				text("Clients: "),
				text(std::to_string(clients.size()))
			})
		}),
		separator(),
		vbox({
			text(simStatus)
		})
	});
}

Element UI::Clients() {
	Elements clientList;
	std::vector<Client> clients = inofs->server->GetClients();
	for (auto client = clients.begin(); client != clients.end(); ++client) {
		if (client != clients.begin()) {
			clientList.push_back(separator());
		}
		std::string address = inet_ntoa(client->addr.sin_addr);
		std::string port = std::to_string(client->addr.sin_port);
		std::string id = std::to_string(client->id);
		Element el = vbox({
			text("Address: " + address + ":" + port),
			text("ID: " + id),
			text("Control vars: " + std::to_string(client->control.size())),
			text("Monitor vars: " + std::to_string(client->monitor.size()))
		});
		clientList.push_back(el);
	}
	return vbox(std::move(clientList));
}

Element UI::Log() {
	Elements msgs;
	for (auto msg = console.begin(); msg != console.end(); ++msg) {
		msgs.push_back(text(*msg));
	}
	return vbox(std::move(msgs));
}