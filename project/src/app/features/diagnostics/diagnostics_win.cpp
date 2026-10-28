#include "diagnostics.hpp"

//every module a sig is scanned in, plus the ones the interfaces come from
static constexpr const char *const watched_modules[]
{
	"client.dll", "engine.dll", "materialsystem.dll", "vguimatsurface.dll", "shaderapidx9.dll",
	"stdshader_dx9.dll", "studiorender.dll", "vstdlib.dll", "inputsystem.dll", "datacache.dll", "vphysics.dll"
};

std::vector<ModuleStamp> diag_platform::moduleStamps()
{
	std::vector<ModuleStamp> out{};

	for (const char *const name : watched_modules)
	{
		const HMODULE mod{ GetModuleHandleA(name) };

		if (!mod) {
			continue;
		}

		const auto dos{ reinterpret_cast<const IMAGE_DOS_HEADER *>(mod) };

		if (dos->e_magic != IMAGE_DOS_SIGNATURE) {
			continue;
		}

		const auto nt{ reinterpret_cast<const IMAGE_NT_HEADERS *>(reinterpret_cast<uintptr_t>(mod) + dos->e_lfanew) };

		if (nt->Signature != IMAGE_NT_SIGNATURE) {
			continue;
		}

		out.push_back(ModuleStamp{ name, nt->FileHeader.TimeDateStamp, nt->OptionalHeader.SizeOfImage });
	}

	return out;
}

ModuleOffset diag_platform::moduleOf(const uintptr_t addr)
{
	HMODULE mod{};

	if (!addr || !GetModuleHandleExA(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT, reinterpret_cast<LPCSTR>(addr), &mod) || !mod) {
		return {};
	}

	char path[MAX_PATH]{};

	if (!GetModuleFileNameA(mod, path, MAX_PATH)) {
		return {};
	}

	return ModuleOffset{ std::filesystem::path{ path }.filename().string(), addr - reinterpret_cast<uintptr_t>(mod) };
}
