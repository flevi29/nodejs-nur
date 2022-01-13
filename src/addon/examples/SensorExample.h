#ifndef _SENSOREXAMPLE_H_
#define _SENSOREXAMPLE_H_ 1

#include <NurAPI.h>

/// <summary>
/// Sets the sensor configuration.
/// </summary>
/// <param name="h_api">The hAPI.</param>
/// <param name="sensor_action">The sensor action.</param>
/// <param name="enabled">if set to <c>true</c> [enabled].</param>
/// <returns></returns>
int set_sensor_config(HANDLE h_api, int sensor_action, bool enabled);

#endif
