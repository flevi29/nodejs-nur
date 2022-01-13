#include <NurAPI.h>

// Example: Set module TX level
int set_tx_level(HANDLE h_api, const int level)
{
	NUR_MODULESETUP setup{};
	setup.txLevel = level;
	return NurApiSetModuleSetup(h_api, NUR_SETUP_TXLEVEL, &setup, sizeof(setup));
}

// Example: Get module TX level
int get_tx_level(HANDLE h_api, int *level)
{
	NUR_MODULESETUP setup{};
	const int ret = NurApiGetModuleSetup(h_api, NUR_SETUP_TXLEVEL, &setup, sizeof(setup));
	if (ret == NUR_NO_ERROR) {
		*level = setup.txLevel;
	}
	return ret;
}
