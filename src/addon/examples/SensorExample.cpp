#include <NurAPI.h>

/// <summary>
/// Sets the sensor configuration.
/// </summary>
/// <param name="h_api">The hAPI.</param>
/// <param name="sensor_action">The sensor action.</param>
/// <param name="enabled">if set to <c>true</c> [enabled].</param>
/// <returns></returns>
int set_sensor_config(HANDLE h_api, const int sensor_action, const bool enabled)
{
	NUR_SENSOR_CONFIG sensors {
		sensor_action,
		enabled,
		sensor_action,
		enabled
	};
	
	return NurApiSetSensorConfig(h_api, &sensors, sizeof(sensors));
}
