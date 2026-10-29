#include "game/game.hpp"

#include "app/features/notifs/notifs.hpp"
#include "app/features/diagnostics/diagnostics.hpp"
#include "sdk/tracked/load_report.hpp"

#include <chrono>

constexpr int errc_noerr{ 0 };
constexpr int errc_sigs{ 1 };
constexpr int errc_ifaces{ 2 };
constexpr int errc_cvars{ 3 };
constexpr int errc_hasload{ 4 };
constexpr int errc_hasunload{ 5 };
constexpr int errc_hooks{ 6 };

void dump_error_string(const std::string& errors, const bool copyToClipboard)
{
	MessageBoxA(nullptr, errors.c_str(), "RunkSEO64", MB_OK | MB_ICONERROR);

	if ( copyToClipboard )
	{
		const char* output = errors.c_str();
		const size_t len = errors.size() + 1; // + null terminator

		// Allocate movable memory
		HGLOBAL hMem = GlobalAlloc(GMEM_MOVEABLE, len);
		if ( !hMem )
			return;

		// Lock the memory and copy the error string into it
		LPVOID pMem = GlobalLock(hMem);
		if ( !pMem )
		{
			GlobalFree(hMem);
			return;
		}
		memcpy(pMem, output, len);
		GlobalUnlock(hMem);

		// Open the clipboard and set the data
		if ( OpenClipboard(nullptr) )
		{
			if ( EmptyClipboard() )
			{
				if ( SetClipboardData(CF_TEXT, hMem) == nullptr )
					GlobalFree(hMem);
			}
			else
				GlobalFree(hMem);

			CloseClipboard();
		}
		else
			GlobalFree(hMem);
	}
}

//hooks the menu itself can't live without, missing one is still worth a message box
static bool isCriticalHook(std::string_view name)
{
	return name == "CDirect3DDevice9_EndScene" || name == "MatSurfaceWindowProc";
}

//globals read directly every frame, there's nothing sensible to fall back to without them
static bool isCoreSig(std::string_view name)
{
	constexpr std::string_view core[]
	{
		"net_time", "host_frameticks", "host_frametime_unbounded", "host_frametime_stddeviation", "g_pGameRules",
		"s_pVertexColor", "s_pVertexColorIgnoreZ", "s_pWireframe", "s_pWireframeIgnoreZ", "C_BaseEntity_m_nPredictionRandomSeed"
	};

	return std::find(std::begin(core), std::end(core), name) != std::end(core);
}

static double msSince(const std::chrono::steady_clock::time_point start)
{
	return std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - start).count();
}

int load()
{
	//sigs that don't resolve after a game update only take their own features down,
	//everything is recorded for the diagnostics tab
	std::string errorSigs{};

	for (Sig *const s : getInsts<Sig>())
	{
		if (!s) {
			continue;
		}

		const auto start{ std::chrono::steady_clock::now() };
		const bool ok{ s->init() };

		load_report->add(LoadItem{ LoadKind::SIG, s->name(), ok, s->addr().get(), msSince(start), ok ? "" : "pattern not found" });

		if (!ok && isCoreSig(s->name())) {
			errorSigs += std::format("{}\n", s->name());
		}
	}

	if (!errorSigs.empty())
	{
		dump_error_string(errorSigs, true);
		return errc_sigs;
	}

	//interfaces are used everywhere, so a missing one still stops the load
	std::string errorIfaces{};

	for (Iface *const i : getInsts<Iface>())
	{
		if (!i) {
			continue;
		}

		const auto start{ std::chrono::steady_clock::now() };
		const bool ok{ i->init() };

		load_report->add(LoadItem{ LoadKind::IFACE, i->name(), ok, reinterpret_cast<uintptr_t>(i->ptr()), msSince(start), ok ? "" : "not found" });

		if (!ok) {
			errorIfaces += std::format("{}\n", i->name());
		}
	}

	if (!errorIfaces.empty())
	{
		dump_error_string(errorIfaces, true);
		return errc_ifaces;
	}

	for (CvarInit *const c : getInsts<CvarInit>())
	{
		if (!c) {
			continue;
		}

		const bool ok{ c->init() };

		load_report->add(LoadItem{ LoadKind::CVAR, c->name(), ok, 0, 0.0, ok ? "" : "cvar not found" });

		if (!ok) {
			return errc_cvars;
		}
	}

	for (Font *const f : getInsts<Font>())
	{
		if (!f) {
			continue;
		}

		f->init();
	}

	cfg::load(utils::getFilePath() + "cfg\\default.json");

	if (std::filesystem::exists(utils::getFilePath() + "cfg\\default.json")) {
		notifs->message("loaded 'default'");
	}

	for (HasLoad *const l : getInsts<HasLoad>())
	{
		if (!l || !l->onLoad()) {
			return errc_hasload;
		}
	}

	if (MH_Initialize() != MH_OK) {
		return errc_hooks;
	}

	std::string errorHooks{};

	for (Hook *const h : getInsts<Hook>())
	{
		if (!h) {
			continue;
		}

		const bool ok{ h->init() };

		std::string detail{};

		if (!ok) {
			detail = h->target() ? MH_StatusToString(h->status()) : "target not found, its sig is missing";
		}

		load_report->add(LoadItem{ LoadKind::HOOK, h->name(), ok, reinterpret_cast<uintptr_t>(h->target()), 0.0, detail });

		if (!ok && isCriticalHook(h->name())) {
			errorHooks += std::format("{} ({})\n", h->name(), detail);
		}
	}

	if (MH_EnableHook(MH_ALL_HOOKS) != MH_OK) {
		return errc_hooks;
	}

	//works out what's disabled, checks for a game update and reports anything that failed
	diagnostics->finishLoad();

	if (!errorHooks.empty()) {
		dump_error_string("the menu may not work, these hooks failed:\n" + errorHooks, true);
	}

	return errc_noerr;
}

int unload()
{
	if (MH_Uninitialize() != MH_OK) {
		return errc_hooks;
	}

	for (HasUnload *const u : getInsts<HasUnload>())
	{
		if (!u || !u->onUnload()) {
			return errc_hasunload;
		}
	}

	return errc_noerr;
}

DWORD WINAPI mainThread(LPVOID param)
{
	if (const int errc{ load() }) {
		MessageBoxA(0, std::format("failed to load ({})", errc).c_str(), "RunkSEO64", 0);
		FreeLibraryAndExitThread(reinterpret_cast<HMODULE>(param), EXIT_FAILURE);
	}

	while (!GetAsyncKeyState(VK_F11)) {
		Sleep(500);
	}

	if (const int errc{ unload() }) {
		MessageBoxA(0, std::format("failed to unload ({})", errc).c_str(), "RunkSEO64", 0);
		FreeLibraryAndExitThread(reinterpret_cast<HMODULE>(param), EXIT_FAILURE);
	}

	Sleep(500);

	FreeLibraryAndExitThread(reinterpret_cast<HMODULE>(param), EXIT_SUCCESS);
}

BOOL WINAPI DllMain(HINSTANCE inst, DWORD reason, LPVOID reserved)
{
	if (reason != DLL_PROCESS_ATTACH) {
		return FALSE;
	}

	const HANDLE main_thread{ CreateThread(0, 0, mainThread, inst, 0, 0) };

	if (!main_thread) {
		return FALSE;
	}

	CloseHandle(main_thread);

	return TRUE;
}