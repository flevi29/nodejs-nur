#include <NurAPI.h>

void NURAPICALLBACK handle_event_napi(HANDLE hApi, DWORD timestamp, int type, LPVOID data, int dataLen)
{
	_tprintf(_T("NOTIFICATION >> "));
	switch (type)
	{
	case NUR_NOTIFICATION_LOG:
	{
		const TCHAR* logMsg = (const TCHAR*)data;
		_tprintf(_T("LOG: %s\r\n"), logMsg);
	}
	break;

	case NUR_NOTIFICATION_PERIODIC_INVENTORY:
	{
		const struct NUR_PERIODIC_INVENTORY_DATA* periodicInventory = (const NUR_PERIODIC_INVENTORY_DATA*)data;
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
		const struct NUR_TRACETAG_DATA* traceTag = (const NUR_TRACETAG_DATA*)data;
		_tprintf(_T("Trace tag\r\n"));
	}
	break;

	case NUR_NOTIFICATION_IOCHANGE:
	{
		const struct NUR_IOCHANGE_DATA* ioChange = (const NUR_IOCHANGE_DATA*)data;
		_tprintf(_T("IO Change Sensor:%d, Source:%d, Dir:%d\r\n"), ioChange->sensor, ioChange->source, ioChange->dir);
	}
	break;

	case NUR_NOTIFICATION_TRIGGERREAD:
	{
		TCHAR epcStr[128];
		const struct NUR_TRIGGERREAD_DATA* triggerRead = (const NUR_TRIGGERREAD_DATA*)data;
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
		const struct NUR_INVENTORYSTREAM_DATA* inventoryStream = (const NUR_INVENTORYSTREAM_DATA*)data;
		_tprintf(_T("Tag data from inventory stream, tagsAdded: %d %s\r\n"),
			inventoryStream->tagsAdded,
			inventoryStream->stopped == TRUE ? _T("STOPPED") : _T(""));
	}
	break;

	case NUR_NOTIFICATION_INVENTORYEX:
	{
		const struct NUR_INVENTORYSTREAM_DATA* inventoryStream = (const NUR_INVENTORYSTREAM_DATA*)data;
		_tprintf(_T("Tag data from extended inventory stream\r\n"));
	}
	break;

	case NUR_NOTIFICATION_DEVSEARCH:
	{
		const struct NUR_NETDEV_INFO* netDev = (const NUR_NETDEV_INFO*)data;
		_tprintf(_T("Device search event: title [%s]; ip %d.%d.%d.%d\r\n"),
			netDev->eth.title,
			netDev->eth.ip[0], netDev->eth.ip[1], netDev->eth.ip[2], netDev->eth.ip[3]);
	}
	break;

	case NUR_NOTIFICATION_CLIENTCONNECTED:
	{
		const struct NUR_CLIENT_INFO* clientInfo = (const NUR_CLIENT_INFO*)data;
		_tprintf(_T("Client device connected to Server\r\n"));
	}
	break;

	case NUR_NOTIFICATION_CLIENTDISCONNECTED:
	{
		const struct NUR_CLIENT_INFO* clientInfo = (const NUR_CLIENT_INFO*)data;
		_tprintf(_T("Client Disconnected from Server\r\n"));
	}
	break;

	case NUR_NOTIFICATION_EASALARM:
	{
		const struct NUR_EASALARMSTREAM_DATA* easAlarmStream = (const NUR_EASALARMSTREAM_DATA*)data;
		_tprintf(_T("NXP EAS Alarm state change\r\n"));
	}
	break;

	case NUR_NOTIFICATION_EPCENUM:
	{
		const struct NUR_EPCENUMSTREAM_DATA* epcEnumStream = (const NUR_EPCENUMSTREAM_DATA*)data;
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
		struct NUR_TUNEEVENT_DATA* tuneData = (struct NUR_TUNEEVENT_DATA*)data;
		_tprintf(_T("Tune event; antenna %d; freq %d; relf power %.1f\r\n"), tuneData->antenna, tuneData->freqKhz, ((float)tuneData->reflPower_dBm / 1000.0));
	}
	break;

	case NUR_NOTIFICATION_TT_CHANGED:
	{
		int tagIdx = 0, bufferCnt = 128;
		TCHAR epcStr[128];
		int events = NUR_TTEV_POSITION | NUR_TTEV_VISIBILITY | NUR_TTEV_SECTOR | NUR_TTEV_RSSI;
		struct NUR_TT_TAG* buffer;

		const struct NUR_TTCHANGED_DATA* ttChangedStream = (const NUR_TTCHANGED_DATA*)data;
		_tprintf(_T("Tag tracking data, changedCount: %d %s\r\n"),
			ttChangedStream->changedCount,
			ttChangedStream->stopped == TRUE ? _T("STOPPED") : _T(""));

		// Get count
		NurApiTagTrackingGetTags(hApi, 0, NULL, &bufferCnt, sizeof(NUR_TT_TAG));

		buffer = new NUR_TT_TAG[bufferCnt];

		// Get all events configured when Tag Tracking was started
		NurApiTagTrackingGetTags(hApi, events, buffer, &bufferCnt, sizeof(NUR_TT_TAG));


		// Loop through tags
		for (tagIdx = 0; tagIdx < bufferCnt; tagIdx++)
		{
			EpcToString(buffer[tagIdx].epc, buffer[tagIdx].epcLen, epcStr);
			_tprintf(_T("EPC '%s' RSSI %d dBm\r\n"), epcStr, buffer[tagIdx].maxRssi);
			_tprintf(_T("Position X %f Y %f Visibility %d Sector %d\r\n"), buffer[tagIdx].X, buffer[tagIdx].Y, buffer[tagIdx].visible, buffer[tagIdx].sector);
		}

		delete[] buffer;
		buffer = NULL;

		if (ttChangedStream->stopped)
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
