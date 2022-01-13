#include "NurApiExample.h"
#include "SensorExample.h"
#include "SetupExample.h"

HANDLE h_api = INVALID_HANDLE_VALUE;

WORD fetch_tags_from_module = 0;

/// <summary>
/// Shows the error, free API object and exit if needed.
/// </summary>
/// <param name="hApi">The hAPI.</param>
/// <param name="error">The error.</param>
/// <returns></returns>
void ShowErrorAndExitIfNeeded(HANDLE hApi, int error) // should push this to node and print it to file?
{
	if (error != NUR_NO_ERROR && error != NUR_ERROR_NO_TAG)
	{
		TCHAR errorMsg[128+1]; // 128 + NULL

		NurApiGetErrorMessage(error, errorMsg, 128);

		_tprintf(_T("\r\nNurApi error %d: [%s]\r\n"), error, errorMsg);

		// Free API object
		NurApiFree(hApi);
		exit(EXIT_FAILURE);
	}
}

/// <summary>
/// Convert EPC byte array to string
/// </summary>
/// <param name="epc">The epc.</param>
/// <param name="epcLen">Length of the epc.</param>
/// <param name="epcStr">The epc string.</param>
void EpcToString(const BYTE *epc, DWORD epcLen, TCHAR *epcStr)
{
	int pos = 0;
	for (DWORD n = 0; n<epcLen; n++) {
		pos += _stprintf_s(&epcStr[pos], NUR_MAX_EPC_LENGTH-pos, _T("%02x"), epc[n]);
	}
	epcStr[pos] = 0;
}

/// <summary>
/// Runs tag tracking for 5 seconds and prints out the coordinate for each tag(or stops tracking if it's already running)
/// </summary>
/// <param name="hApi">The hAPI.</param>
/// <returns></returns>
int ToggleTagTracking(HANDLE hApi)
{
	int error = NUR_NO_ERROR;
	// Settings for our tag tracking
	struct NUR_TAGTRACKING_CONFIG ttConfig;
	
	// Perform a full round i.e. scan each antenna before reporting changes
	ttConfig.flags = NUR_TTFL_FULLROUNDREPORT;
	// Report position, visibility and sector changes
	ttConfig.events = NUR_TTEV_POSITION | NUR_TTEV_VISIBILITY | NUR_TTEV_SECTOR | NUR_TTEV_RSSI;
	
	// RSSI needs to change atleast 5dBm
	ttConfig.rssiDeltaFilter = 5;
	// Position needs to change atleast 0.05 normalized coordinate value
	ttConfig.positionDeltaFilter = 0.05f;
	// Scan until no more than 5 new tags is seen
	ttConfig.scanUntilNewTagsCount = 5;
	// Tag visibility change timeout 3000ms
	ttConfig.visibilityTimeout = 3000;

	
	if(!NurApiIsTagTrackingRunning(hApi))
	{
		error = NurApiStartTagTracking(hApi, &ttConfig, sizeof(ttConfig));
		if (error != NUR_NO_ERROR)
		{
			// Failed
			return error;
		}
	}
	else
	{
		NurApiStopTagTracking(hApi);
	}
	return NUR_NO_ERROR;
}

/// <summary>
/// Performs the inventory.
/// </summary>
/// <param name="hApi">The hAPI.</param>
/// <returns></returns>
int PerformInventory(HANDLE hApi)
{
	int rounds = 0;
	int error = 0;
	int tagsFound = 0, tagsMem = 0;
	int tagCount = 0;
	int idx = 0;
	HANDLE hStorage = NULL;
	HANDLE hTag = NULL;
	struct NUR_INVENTORY_RESPONSE invResp;
	struct NUR_TAG_DATA tagData;
	TCHAR epcStr[128];

	// Clear previously inventoried tags from NurApi internal tag storage
	// and from module.
	error = NurApiClearTags(hApi);
	if (error != NUR_NO_ERROR)
	{
		// Failed
		return error;
	}

	_tprintf(_T("Perform inventory..\r\n"));

	while (rounds++ < 5)
	{
		// Perform inventory
		//error = NurApiInventory(hApi, 1, 0, 0, &invResp);
		error = NurApiSimpleInventory(hApi, &invResp);
		_tprintf(_T("Round: %d, Tags: %d / %d\r\n"), rounds, invResp.numTagsFound, invResp.numTagsMem);
		if (error != NUR_NO_ERROR && error != NUR_ERROR_NO_TAG)
		{
			// Failed
			return error;
		}
		if (invResp.numTagsMem > fetch_tags_from_module)
		{
			// Fetch tags from module (including tag meta) to NurApi
			// internal tag storage and clear tags from module.
			_tprintf(_T("Fetch tags from module...\r\n"));
			error = NurApiFetchTags(hApi, TRUE, NULL);
			if (error != NUR_NO_ERROR)
			{
				// Failed
				return error;
			}
		}
	}
	// Fetch tags from module (including tag meta) to NurApi
	// internal tag storage and clear tags from module.
	_tprintf(_T("Fetch tags from module...\r\n"));
	error = NurApiFetchTags(hApi, TRUE, NULL);
	error = NurApiGetTagCount(hApi, &tagCount);
	if (error != NUR_NO_ERROR)
	{
		// Failed
		return error;
	}
	_tprintf(_T("%d unigue tags found\r\n"), tagCount);

	// Loop through tags
	for (idx = 0; idx < tagCount; idx++)
	{
		error = NurApiGetTagData(hApi, idx, &tagData);

		if (error == NUR_NO_ERROR)
		{
			EpcToString(tagData.epc, tagData.epcLen, epcStr);

			// Print tag info
			_tprintf(_T("Tag info:\r\n"));
			_tprintf(_T("  EPC: [%s]\r\n"), epcStr);
			_tprintf(_T("  EPC length: %d\r\n"), tagData.epcLen);
			_tprintf(_T("  RSSI: %d\r\n"), tagData.rssi);
			_tprintf(_T("  Timestamp: %u\r\n"), tagData.timestamp);
			_tprintf(_T("  Frequency: %u\r\n"), tagData.freq);
			_tprintf(_T("  PC bytes: %04x\r\n"), tagData.pc);
		}
		else
		{
			// Print error
			TCHAR errorMsg[128+1]; // 128 + NULL    
			NurApiGetErrorMessage(error, errorMsg, 128);
			_tprintf(_T("NurApiTSGetTagData() returns error %d: [%s]\r\n"), error, errorMsg);
		}
	}

	return NUR_NO_ERROR;
}

/// <summary>
/// Displays the main menu.
/// </summary>
/// <param name="hApi">The hAPI.</param>
void DisplayMainMenu(HANDLE hApi)
{
	_tprintf(_T("\r\n"));
	_tprintf(_T("NurApiExample - Main Menu\r\n"));
	_tprintf(_T("Please make your selection\r\n"));
	_tprintf(_T(" p - Ping module\r\n"));
	_tprintf(_T(" v - Print module version\r\n"));
	_tprintf(_T(" 1 - Perform inventory\r\n"));
	_tprintf(_T(" 2 - Start / Stop inventory stream\r\n"));
	_tprintf(_T(" 3 - Configure sensor notifigation (SAMPO)\r\n"));
	_tprintf(_T(" 4 - Configure TxLevel\r\n"));
	_tprintf(_T(" 5 - List physical antennas\r\n"));
	_tprintf(_T(" 6 - Perform tag tracking\r\n"));
	_tprintf(_T(" 7 - Network device discovery\r\n"));
	_tprintf(_T(" q - QUIT\r\n"));
	int connected = NurApiIsConnected(hApi);
	_tprintf(_T("STATE: %s\r\n"), (connected == 0) ? _T("Connected") : _T("Disconnected"));
}

/// <summary>
/// Prints the module version.
/// </summary>
/// <param name="hApi">The hAPI.</param>
/// <returns></returns>
int PrintModuleVersion(HANDLE hApi)
{
	struct NUR_READERINFO info;
	struct NUR_DEVICECAPS devcaps;
	int error;

	error = NurApiGetReaderInfo(hApi, &info, sizeof(info));
	if (error == NUR_NO_ERROR)
	{
		_tprintf(_T("Module Version: [%d.%d-%c]\r\n"), info.swVerMajor, info.swVerMinor, info.devBuild);
	}
	error = NurApiGetDeviceCaps(hApi, &devcaps);
	if (error == NUR_NO_ERROR)
	{
		_tprintf(_T("DeviceCaps.szTagBuffer: %d\r\n"), devcaps.szTagBuffer);
		//FetchTagsFromModule = devcaps.szTagBuffer / 3;
	}
	_tprintf(_T("\r\n"));
	return error;
}

/// <summary>
/// Set notification callback.
/// </summary>
/// <param name="hApi">The hAPI.</param>
/// <param name="timestamp">The timestamp.</param>
/// <param name="type">The type.</param>
/// <param name="data">The data.</param>
/// <param name="dataLen">Length of the data.</param>
void NURAPICALLBACK MyNotificationFunc(HANDLE hApi, DWORD timestamp, int type, LPVOID data, int dataLen)
{
	_tprintf(_T("NOTIFICATION >> "));
	switch (type)
	{
	case NUR_NOTIFICATION_LOG:
		{
			const TCHAR *logMsg = (const TCHAR *)data;
			_tprintf(_T("LOG: %s\r\n"), logMsg);
		}
		break;

	case NUR_NOTIFICATION_PERIODIC_INVENTORY:
		{
			const struct NUR_PERIODIC_INVENTORY_DATA *periodicInventory = (const NUR_PERIODIC_INVENTORY_DATA *)data;
			_tprintf(_T("Periodic inventory data\r\n"));
		}
		break;

	case NUR_NOTIFICATION_TRDISCONNECTED:
		_tprintf(_T("Transport disconnected\r\n"));
		break;

	case NUR_NOTIFICATION_MODULEBOOT:
		_tprintf(_T("Module booted\r\n"));
		break;

	case NUR_NOTIFICATION_TRCONNECTED:
		_tprintf(_T("Transport connected\r\n"));
		break;

	case NUR_NOTIFICATION_TRACETAG:
		{
			const struct NUR_TRACETAG_DATA *traceTag = (const NUR_TRACETAG_DATA *)data;
			_tprintf(_T("Trace tag\r\n"));
		}
		break;

	case NUR_NOTIFICATION_IOCHANGE:
		{
			const struct NUR_IOCHANGE_DATA *ioChange = (const NUR_IOCHANGE_DATA *)data; 
			_tprintf(_T("IO Change Sensor:%d, Source:%d, Dir:%d\r\n"), ioChange->sensor, ioChange->source, ioChange->dir);
		}
		break;

	case NUR_NOTIFICATION_TRIGGERREAD:
		{
			TCHAR epcStr[128];
			const struct NUR_TRIGGERREAD_DATA *triggerRead = (const NUR_TRIGGERREAD_DATA *)data;
			EpcToString(triggerRead->epc, triggerRead->epcLen, epcStr);

			_tprintf(_T("IO triggered tag read EPC: %s, RSSI: %d\r\n"), epcStr, triggerRead->rssi);
		}
		break;

	case NUR_NOTIFICATION_HOPEVENT:
		{
			_tprintf(_T("Channel hopping event\r\n"));
		}
		break;

	case NUR_NOTIFICATION_INVENTORYSTREAM:
		{
			const struct NUR_INVENTORYSTREAM_DATA *inventoryStream = (const NUR_INVENTORYSTREAM_DATA *)data;
			_tprintf(_T("Tag data from inventory stream, tagsAdded: %d %s\r\n"),
				inventoryStream->tagsAdded,
				inventoryStream->stopped == TRUE ? _T("STOPPED") : _T("") );
		}
		break;

	case NUR_NOTIFICATION_INVENTORYEX:
		{
			const struct NUR_INVENTORYSTREAM_DATA *inventoryStream = (const NUR_INVENTORYSTREAM_DATA *)data;
			_tprintf(_T("Tag data from extended inventory stream\r\n"));
		}
		break;

	case NUR_NOTIFICATION_DEVSEARCH:
		{
			const struct NUR_NETDEV_INFO *netDev = (const NUR_NETDEV_INFO *)data;
			_tprintf(_T("Device search event: title [%s]; ip %d.%d.%d.%d\r\n"), 
				netDev->eth.title,
				netDev->eth.ip[0], netDev->eth.ip[1], netDev->eth.ip[2], netDev->eth.ip[3]);
		}
		break;

	case NUR_NOTIFICATION_CLIENTCONNECTED:
		{
			const struct NUR_CLIENT_INFO *clientInfo = (const NUR_CLIENT_INFO *)data;
			_tprintf(_T("Client device connected to Server\r\n"));
		}
		break;

	case NUR_NOTIFICATION_CLIENTDISCONNECTED:
		{
			const struct NUR_CLIENT_INFO *clientInfo = (const NUR_CLIENT_INFO *)data;
			_tprintf(_T("Client Disconnected from Server\r\n"));
		}
		break;

	case NUR_NOTIFICATION_EASALARM:
		{
			const struct NUR_EASALARMSTREAM_DATA *easAlarmStream = (const NUR_EASALARMSTREAM_DATA *)data;
			_tprintf(_T("NXP EAS Alarm state change\r\n"));
		}
		break;

	case NUR_NOTIFICATION_EPCENUM:
		{
			const struct NUR_EPCENUMSTREAM_DATA *epcEnumStream = (const NUR_EPCENUMSTREAM_DATA *)data;
			_tprintf(_T("New EPC was programmed by the EPC enumeration stream\r\n"));
		}
		break;

	case NUR_NOTIFICATION_EXTIN:
		{
			_tprintf(_T("State change in Ext input port 0-3\r\n"));
		}
		break;

	case NUR_NOTIFICATION_GENERAL:
		{
			_tprintf(_T("DEPRECATED\r\n"));
		}
		break;

	case NUR_NOTIFICATION_TUNEEVENT:
		{
			struct NUR_TUNEEVENT_DATA *tuneData = (struct NUR_TUNEEVENT_DATA *)data;
			_tprintf(_T("Tune event; antenna %d; freq %d; relf power %.1f\r\n"), tuneData->antenna, tuneData->freqKhz, ((float)tuneData->reflPower_dBm/1000.0));
		}
		break;

	case NUR_NOTIFICATION_TT_CHANGED:
		{
			int tagIdx = 0,bufferCnt = 128;
			TCHAR epcStr[128];
			int events = NUR_TTEV_POSITION | NUR_TTEV_VISIBILITY | NUR_TTEV_SECTOR | NUR_TTEV_RSSI;
			struct NUR_TT_TAG *buffer;

			const struct NUR_TTCHANGED_DATA *ttChangedStream = (const NUR_TTCHANGED_DATA *)data;
			_tprintf(_T("Tag tracking data, changedCount: %d %s\r\n"),
				ttChangedStream->changedCount,
				ttChangedStream->stopped == TRUE ? _T("STOPPED") : _T("") );

			// Get count
			NurApiTagTrackingGetTags(hApi, 0, NULL, &bufferCnt, sizeof(NUR_TT_TAG));
			
			buffer = new NUR_TT_TAG[bufferCnt];

			// Get all events configured when Tag Tracking was started
			NurApiTagTrackingGetTags(hApi, events, buffer, &bufferCnt, sizeof(NUR_TT_TAG));
			

			// Loop through tags
			for (tagIdx=0; tagIdx<bufferCnt; tagIdx++)
			{
				EpcToString(buffer[tagIdx].epc, buffer[tagIdx].epcLen, epcStr);
				_tprintf(_T("EPC '%s' RSSI %d dBm\r\n"), epcStr, buffer[tagIdx].maxRssi);
				_tprintf(_T("Position X %f Y %f Visibility %d Sector %d\r\n"), buffer[tagIdx].X, buffer[tagIdx].Y, buffer[tagIdx].visible, buffer[tagIdx].sector);
			}

			delete[] buffer;
			buffer = NULL;

			if(ttChangedStream->stopped)
			{
				_tprintf(_T("Tag tracking restarted\r\n"));
				// Restart tag tracking
				NurApiStartTagTracking(hApi, NULL, 0);
			}
			else
			{
				_tprintf(_T("Tag tracking stopped\r\n"));
			}
		}
		break;

	default:
		_tprintf(_T("Unhandled notification: %d\r\n"), type);
		break;
	}
}

/// <summary>
/// Reads the keystrokes until its not line feed.
/// </summary>
/// <returns></returns>
int ReadKeystroke()
{
	int keystroke = 0;
	do
	{
		keystroke = tolower(getchar());
	} while (keystroke == '\n');
	return keystroke;
}

void PerPolarityInventory(HANDLE hApi)
{
	struct NUR_ANTENNA_MAPPING map[NUR_MAX_ANTENNAS_EX];
	int nrMappings = 0;
	int error, i;
	TCHAR *lastAntenna = NULL;
	
	// Get list of available antennas on reader
	error = NurApiGetAntennaMap(hApi, map, &nrMappings, _countof(map), sizeof(map[0]));
	if (error != NUR_NO_ERROR)
		return; // Handle error..

	// Use always autoswitch antenna, so inventory goes through all polarities.
	struct NUR_MODULESETUP setup;
	setup.selectedAntenna = NUR_ANTENNAID_AUTOSELECT;
	error = NurApiSetModuleSetup(hApi, NUR_SETUP_SELECTEDANT, &setup, sizeof(setup));
	if (error != NUR_NO_ERROR)
		return; // Handle error..

	// Loop through all antennas (w/o polarities) and perform inventory
	for (i=0; i<nrMappings; i++)
	{
		struct NUR_INVENTORY_RESPONSE invResp;
		TCHAR *polarityPtr;
		
		// Check for polarity mark '.' in antenna name
		polarityPtr = _tcschr(map[i].name, '.');
		if (polarityPtr) *polarityPtr = '\0';

		// If last antenna is same as current without polarity, skip
		// We want to do inventory only once per main antenna (all polarities)
		if (lastAntenna && _tcscmp(lastAntenna, map[i].name) == 0)
			continue;

		// Store last antenna string pointer
		lastAntenna = map[i].name;

	    // Enable only current antenna
		NurApiEnablePhysicalAntenna(hApi, map[i].name, TRUE);
 
		// Clear module and api tag storage
		NurApiClearTags(hApi);
                
		// Perform inventory (with module stored settings)
		NurApiSimpleInventory(hApi, &invResp);
		_tprintf(_T("%s: Found %d tags\r\n"), map[i].name, invResp.numTagsFound);
 
		if (invResp.numTagsFound > 0)
		{
			int tagCount, tagIdx;
			TCHAR epcStr[128];

			// Fetch tags from module to api tag storage
			NurApiFetchTags(hApi, TRUE, NULL);
 
			// Get tag count from api tag storage
			NurApiGetTagCount(hApi, &tagCount);
			
			// Loop through tags
			for (tagIdx=0; tagIdx<tagCount; tagIdx++)
			{
				struct NUR_TAG_DATA_EX tagData;
				error = NurApiGetTagDataEx(hApi, tagIdx, &tagData, sizeof(tagData));
				if (error != NUR_NO_ERROR)
					return; // Handle error..
				EpcToString(tagData.epc, tagData.epcLen, epcStr);
				_tprintf(_T("EPC '%s' RSSI %d dBm\r\n"), epcStr, tagData.rssi);
			}
		}
	}
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

device_version get_device_version(){
	device_version dv{};

	dv.error = NurApiGetReaderInfo(h_api, &dv.ri, sizeof(dv.ri));
	if (dv.error != NUR_NO_ERROR){
		return dv;
	}
	dv.error = NurApiGetDeviceCaps(h_api, &dv.dc);
	// TODO: FetchTagsFromModule = devcaps.szTagBuffer / 3;
	return dv;
}

tags_array get_tags(const int Q, const int session, const int max_rounds){
	NUR_INVENTORY_RESPONSE inv_resp{};
	NUR_TAG_DATA_EX tag_data{};

	tags_array t{};
	
	// Clear previously inventoried tags from NurApi internal tag storage
	// and from module.
	t.error = NurApiClearTags(h_api);
	if (t.error != NUR_NO_ERROR){
		// Failed
		// TODO: maybe print error details to file
		return t;
	}

	int rounds = 0;
	while (rounds++ < max_rounds){
		// Perform inventory
		t.error = NurApiSimpleInventory(h_api, &inv_resp);
		//t.error = NurApiInventory(h_api, rounds, Q, session, &inv_resp);
		if (t.error != NUR_NO_ERROR && t.error != NUR_ERROR_NO_TAG){
			// Failed
			// TODO: maybe print error details to file
			return t;
		}
		if (inv_resp.numTagsMem > fetch_tags_from_module){
			// Fetch tags from module (including tag meta) to NurApi
			// internal tag storage and clear tags from module.
			t.error = NurApiFetchTags(h_api, TRUE, nullptr);
			if (t.error != NUR_NO_ERROR){
				// Failed
				// TODO: maybe print error details to file
				return t;
			}
		}
	}
	// Fetch tags from module (including tag meta) to NurApi
	// internal tag storage and clear tags from module.
	t.error = NurApiFetchTags(h_api, TRUE, nullptr);
	//if (t.error != NUR_NO_ERROR){
	//	// Failed
	//	// TODO: maybe print error details to file
	//	return t;
	//}
	int tag_count = 0;
	t.error = NurApiGetTagCount(h_api, &tag_count);
	if (t.error != NUR_NO_ERROR){
		// Failed
		// TODO: maybe print error details to file
		return t;
	}

	// Loop through tags
	t.tags = new NUR_TAG_DATA_EX[tag_count];
	t.length = tag_count;
	for (int idx = 0; idx < tag_count; idx++){
		t.error = NurApiGetTagDataEx(h_api, idx, &tag_data, sizeof(tag_data));

		if (t.error == NUR_NO_ERROR){
			/*TCHAR epc_str[NUR_MAX_EPC_LENGTH_EX*2];
			epc_to_string(tagData.epc, tagData.epcLen, epc_str);*/
			t.tags[idx] = tag_data;
		}
		else{
			// TODO: maybe print error details to file
		}
	}

	return t;
}

tags_array get_tags_with_internal_setup(const int max_rounds) {
	return get_tags(0, 0, max_rounds);
}

int init_nur_handle(){
	h_api = NurApiCreate();
	if (h_api == INVALID_HANDLE_VALUE){
		return NUR_ERROR_INVALID_HANDLE;
	}
	return NUR_SUCCESS;
}

int connect_device_usb(){
	int error;
#ifdef USE_USB_AUTO_CONNECT
	error = NurApiSetUsbAutoConnect(h_api, TRUE);
	//ShowErrorAndExitIfNeeded(h_api, error);
#else
	// Connect API to module through serial port TODO
	error = NurApiConnectSerialPortEx(h_api, _T("/dev/ttyACM0"), NUR_DEFAULT_BAUDRATE);
	//ShowErrorAndExitIfNeeded(hApi, error);
#endif
	return error;
}

int connect_device_ethernet(std::wstring &ip){
	// int error = NUR_ERROR_GENERAL;
	//error = NurApiConnectSocket(h_api, ip.c_str(), 4333);
	// ShowErrorAndExitIfNeeded(h_api, error);
	// TODO: maybe print error details to file
	// return error;
	return 1;
}

void disconnect(){ // TODO
	NurApiFree(h_api);
}

int ping_connected_device(){
	return NurApiPing(h_api, nullptr);
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

#ifdef WIN32
int _tmain(int argc, TCHAR* argv[])
#else
int main(int argc, char* argv[])
#endif
{

	auto inh = init_nur_handle();

	// Set notification callback TODO
	_tprintf(_T("Set notification callback...\r\n"));
	NurApiSetNotificationCallback(h_api, MyNotificationFunc);
	//ShowErrorAndExitIfNeeded(h_api, error);

	// Set full log level, w/o data TODO
	_tprintf(_T("Set NurApi logging level mask...\r\n"));
	// NurApiSetLogLevel(hApi, NUR_LOG_ALL & ~NUR_LOG_DATA);
	NurApiSetLogLevel(h_api, NUR_LOG_ERROR);
	
	auto cdu = connect_device_usb();
	//auto ta = get_tags_with_internal_setup(5);
	PerformInventory(h_api);
	//std::cout << ta.length;
	disconnect();
	
	return 0;
	
	int error = 0;
	int choice = 0;

	// Create new API object
	_tprintf(_T("Create new NurApi object...\r\n"));
	h_api = NurApiCreate();
	if (h_api == INVALID_HANDLE_VALUE)
	{
		_tprintf(_T("Cound not create NurApi object.\r\n"));
		return 1;
	}

	// Set notification callback TODO
	_tprintf(_T("Set notification callback...\r\n"));
	error = NurApiSetNotificationCallback(h_api, MyNotificationFunc);
	ShowErrorAndExitIfNeeded(h_api, error);

	// Set full log level, w/o data TODO
	_tprintf(_T("Set NurApi logging level mask...\r\n"));
	// NurApiSetLogLevel(hApi, NUR_LOG_ALL & ~NUR_LOG_DATA);
	NurApiSetLogLevel(h_api, NUR_LOG_ERROR);

#ifdef USE_USB_AUTO_CONNECT
	_tprintf(_T("Enable USB auto connect functionality...\r\n"));
	error = NurApiSetUsbAutoConnect(h_api, TRUE);
	ShowErrorAndExitIfNeeded(h_api, error);
#else
	// Connect API to module through serial port
	_tprintf(_T("Connect to NUR module using serial port transport... (/dev/ttyACM0)\r\n"));
	error = NurApiConnectSerialPortEx(hApi, "/dev/ttyACM0", NUR_DEFAULT_BAUDRATE);
	ShowErrorAndExitIfNeeded(hApi, error);
#endif

	// EXAMPLE: Connect over network
	// error = NurApiConnectSocket(hApi, _T("192.168.100.12"), 4333);

	// API ready
	DisplayMainMenu(h_api);
	do{
		choice = ReadKeystroke();
		switch(choice)
		{
		case (int)'1':
			// Perform inventory
			_tprintf(_T("Perform inventory...\r\n"));
			error = PerformInventory(h_api);
			ShowErrorAndExitIfNeeded(h_api, error);
			DisplayMainMenu(h_api);
			break;

		case (int)'2':
			// Start / Stop InventoryStream
			if (NurApiIsInventoryStreamRunning(h_api))
			{
				_tprintf(_T("Stop InventoryStream...\r\n"));
				error = NurApiStopInventoryStream(h_api);
				ShowErrorAndExitIfNeeded(h_api, error);
			}
			else
			{
				_tprintf(_T("Start InventoryStream...\r\n"));
				// TODO: how could I possibly make use of this? works through events
				error = NurApiStartInventoryStream(h_api, 0, 0, 0);
				ShowErrorAndExitIfNeeded(h_api, error);
			}
			break;

		case (int)'3':
			{ // TODO: how should I use this
				// Enable NUR module (SAMPO) sensor notifigation
				_tprintf(_T("Configure sensor notifigation (SAMPO)...\r\n"));
				_tprintf(_T("[0 = NONE, 1 = NOTIFY, 2 = SCANTAG, 3 = INVETORY]: "));
				int sensorAction = NUR_GPIO_ACT_NONE;
				switch(ReadKeystroke())
				{
				case (int)'1':
					sensorAction = NUR_GPIO_ACT_NOTIFY;
					break;
				case (int)'2':
					sensorAction = NUR_GPIO_ACT_SCANTAG;
					break;
				case (int)'3':
					sensorAction = NUR_GPIO_ACT_INVENTORY;
					break;
				}
				error = set_sensor_config(h_api, sensorAction, TRUE);
				ShowErrorAndExitIfNeeded(h_api, error);
				DisplayMainMenu(h_api);
			}
			break;

		case (int)'4':
			{
				// Configure TxLevel
				_tprintf(_T("Configure TxLevel...\r\n"));
				_tprintf(_T("[0 - 19 = MAX - MIN]: "));
				int txLevel = 0;
				//scanf_s("%d", &txLevel);
				if (scanf("%d", &txLevel) == 1) {
					error = set_tx_level(h_api, txLevel);
					ShowErrorAndExitIfNeeded(h_api, error);
					error = get_tx_level(h_api, &txLevel);
					ShowErrorAndExitIfNeeded(h_api, error);
					_tprintf(_T("TxLevel is now %d\r\n"), txLevel);
				} else {
					_tprintf(_T("Invalid input\r\n"));
				}
			}
			break;

		case (int)'v':
			// Print module version
			_tprintf(_T("Print module version...\r\n"));
			error = PrintModuleVersion(h_api);
			ShowErrorAndExitIfNeeded(h_api, error);
			break;

		case (int)'p':
			// Ping module
			_tprintf(_T("Ping module...\r\n"));
			error = NurApiPing(h_api, NULL);
			ShowErrorAndExitIfNeeded(h_api, error);
			_tprintf(_T("OK\r\n"));
			break;

		case (int)'5':
			{
				// List physical antennas
				struct NUR_ANTENNA_MAPPING map[NUR_MAX_ANTENNAS_EX];
				int nrMappings = 0;
				int i;
				
				_tprintf(_T("List physical antenna map\r\n"));
				error = NurApiGetAntennaMap(h_api, map, &nrMappings, _countof(map), sizeof(map[0]));
				ShowErrorAndExitIfNeeded(h_api, error);
				
				for (i=0; i<nrMappings; i++) {
					_tprintf(_T("Logical id [%d] = '%s'\r\n"), map[i].antennaId, map[i].name);
				}
				
				_tprintf(_T("OK\r\n"));
			}
			break;

		case (int)'6':
			{
				_tprintf(_T("Start/Stop tag tracking...\r\n"));
				error = ToggleTagTracking(h_api);
				ShowErrorAndExitIfNeeded(h_api, error);
				DisplayMainMenu(h_api);
			}
			break;

		case (int)'7':
			{
				// Perform device discovery
				_tprintf(_T("Discovering devices (%d ms)...\r\n"), MAX_DEVQUERY_TIMEOUT);
				error = NurApiDiscoverDevices(h_api, MAX_DEVQUERY_TIMEOUT);
				ShowErrorAndExitIfNeeded(h_api, error);
				DisplayMainMenu(h_api);
			}
			break;

		default:
			// Unknown choice
			DisplayMainMenu(h_api);
			break;
		}
	} while (choice != (int)'q' && choice != -1);

	// Free API object
	_tprintf(_T("Free NurApi object...\r\n"));
	NurApiFree(h_api);

	return 0;
}
