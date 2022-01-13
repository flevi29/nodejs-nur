#pragma once
#include <NurAPI.h>

// Conflicts w/ g++ stdlib
#undef min
#undef max

#include <iostream>

#ifdef WIN32
#define USE_USB_AUTO_CONNECT 1
#endif

struct tags_array{
	NUR_TAG_DATA_EX* tags;
	int length;
	int error;
};

struct device_version{
	NUR_READERINFO ri;
	NUR_DEVICECAPS dc;
	int error;
};

int connect_device_ethernet(std::wstring& ip);
int connect_device_usb();
int init_nur_handle();
tags_array get_tags_with_internal_setup(int max_rounds);
tags_array get_tags(int Q, int session, int max_rounds);
device_version get_device_version();
void disconnect();
int ping_connected_device();
