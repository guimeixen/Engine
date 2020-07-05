// Default to dedicated GPU
#ifdef _WIN32
#include <Windows.h>
extern "C" {
	_declspec(dllexport) DWORD NvOptimusEnablement = 0x00000001;

	__declspec(dllexport) int AmdPowerXpressRequestHighPerformance = 1;
}
#endif