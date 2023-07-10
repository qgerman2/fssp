#include <vector>
#include <string>
#include <mutex>
#include <cstdarg>
#include "main.h"
#include "server.h"
#include "ui.h"

#include "ftxui/component/component.hpp"
#include "ftxui/component/component_options.hpp"
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
		"Comms",
		"Log"
	};
	static int tab = 0;
	auto menu = Menu(&tab_names, &tab);

	auto page = Container::Tab({
		State(),
		Renderer([&] {return Clients();}),
		Renderer([&] {return Comms();}),
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
	print_mutex.lock();
	if (logs.size() > print_max) {
		logs.erase(logs.begin());
	}
	logs.push_back(std::string(msgBuf, bytes));
	print_mutex.unlock();
}

void UI::PrintComms(const char *format, ...) {
	va_list args;
	char msgBuf[2048];
	va_start(args, format);
	int bytes = vsprintf(msgBuf, format, args);
	va_end(args);
	print_mutex.lock();
	if (comms.size() > print_max) {
		comms.erase(comms.begin());
	}
	comms.push_back(std::string(msgBuf, bytes));
	print_mutex.unlock();
}

Component UI::State() {
	Elements localIPs;
	auto localips = inofs->server->GetLocalIPs();
	for (auto ip = localips.begin(); ip != localips.end(); ++ip) {
		localIPs.push_back(text(*ip + ":" + std::to_string(SERVER_PORT)));
	};
	return Container::Vertical({
		Container::Horizontal({
			Renderer([=] {
				return vbox({
					text("Server IPs:"),
					vbox(std::move(localIPs)),
					separator(),
					text("Clients: " + std::to_string(inofs->server->GetClients().size()))
				});
			}),
			Renderer([=] {return separator();}),
			Container::Vertical({
				Renderer([=] {return text("Serial: ");}),
				Checkbox("Autoscan", &inofs->server->serialEnabled),
				Button("Scan", [&] {
					inofs->server->PollSerialDevices(false);
				}, ButtonOption::Simple())
			})
		}),
		Renderer([&] {
			std::string simStatus = "Not connected to flight-sim";
			if (inofs->sim->isConnected()) {
				simStatus = "Connected to flight-sim!";
			};
			return vbox({
				separator(),
				text(simStatus)
			});
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
		std::string address;
		if (client->id.isSerial) {
			address = client->id.device;
		} else {
			address = std::string(inet_ntoa(client->id.addr.sin_addr)) + ":" + std::to_string(client->id.addr.sin_port);
		}
		std::string id = std::to_string(client->id.num);
		std::string precision = "4 bytes (float)";
		if (client->double_precision) {
			precision = "8 bytes (double)";
		}
		Element el = vbox({
			text("ID: " + id),
			text("Address: " + address),
			text("Control vars: " + std::to_string(client->control.size())),
			text("Monitor vars: " + std::to_string(client->monitor.size())),
			text("Floating point precision: " + precision)
		});
		clientList.push_back(el);
	}
	return vbox(std::move(clientList));
}

Element UI::Comms() {
	print_mutex.lock();
	Elements msgs;
	for (auto msg = comms.begin(); msg != comms.end(); ++msg) {
		msgs.push_back(text(*msg));
	}
	print_mutex.unlock();
	return vbox(std::move(msgs));
}

Element UI::Log() {
	print_mutex.lock();
	Elements msgs;
	for (auto msg = logs.begin(); msg != logs.end(); ++msg) {
		msgs.push_back(text(*msg));
	}
	print_mutex.unlock();
	return vbox(std::move(msgs));
}