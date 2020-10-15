#include <Windows.h>

#include <Python.h>

//We cannot do lazy Python initialization when needed, because the Python runtime seems to be thread sensitive

BOOL APIENTRY DllMain(HMODULE hModule,
                      DWORD  ul_reason_for_call,
                      LPVOID lpReserved)
{
	switch (ul_reason_for_call) {
		case DLL_PROCESS_ATTACH:	Py_Initialize();
									break;

		case DLL_THREAD_ATTACH:
		case DLL_THREAD_DETACH:
									break;


		case DLL_PROCESS_DETACH:	Py_Finalize();
									break;
	}
	return TRUE;
}
