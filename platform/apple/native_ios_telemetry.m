#import <Foundation/Foundation.h>
#import <UIKit/UIKit.h>

#include "platform/native_ios_telemetry.h"
#include "platform/native_log.h"

static id s_thermalObserver;
static id s_powerObserver;
static id s_batteryLevelObserver;
static id s_batteryStateObserver;
static BOOL s_batteryMonitoringWasEnabled;

static const char *CTRPadDevice_ThermalStateName(NSProcessInfoThermalState state)
{
	switch (state)
	{
	case NSProcessInfoThermalStateNominal:
		return "nominal";
	case NSProcessInfoThermalStateFair:
		return "fair";
	case NSProcessInfoThermalStateSerious:
		return "serious";
	case NSProcessInfoThermalStateCritical:
		return "critical";
	}

	return "unknown";
}

static const char *CTRPadDevice_BatteryStateName(UIDeviceBatteryState state)
{
	switch (state)
	{
	case UIDeviceBatteryStateUnplugged:
		return "unplugged";
	case UIDeviceBatteryStateCharging:
		return "charging";
	case UIDeviceBatteryStateFull:
		return "full";
	case UIDeviceBatteryStateUnknown:
		return "unknown";
	}

	return "unknown";
}

static void CTRPadDevice_LogState(const char *reason)
{
	NSProcessInfo *processInfo = NSProcessInfo.processInfo;
	UIDevice *device = UIDevice.currentDevice;
	const float batteryLevel = device.batteryLevel;
	const int batteryPercent = batteryLevel >= 0.0f ? (int)(batteryLevel * 100.0f + 0.5f) : -1;

	Platform_Log("[CTR Device] reason=%s thermal=%s low_power=%s battery_state=%s battery_percent=%d\n", reason,
	             CTRPadDevice_ThermalStateName(processInfo.thermalState), processInfo.lowPowerModeEnabled ? "on" : "off",
	             CTRPadDevice_BatteryStateName(device.batteryState), batteryPercent);
	Platform_LogFlush();
}

int NativeIOSTelemetry_Begin(void)
{
	NSNotificationCenter *notificationCenter;
	UIDevice *device;

	if (!NSThread.isMainThread)
	{
		return 0;
	}
	if (s_thermalObserver != nil)
	{
		return 1;
	}

	notificationCenter = NSNotificationCenter.defaultCenter;
	device = UIDevice.currentDevice;
	s_batteryMonitoringWasEnabled = device.batteryMonitoringEnabled;
	device.batteryMonitoringEnabled = YES;

	s_thermalObserver = [notificationCenter addObserverForName:NSProcessInfoThermalStateDidChangeNotification
	                                                  object:nil
	                                                   queue:NSOperationQueue.mainQueue
	                                              usingBlock:^(__unused NSNotification *notification) {
		                                              CTRPadDevice_LogState("thermal-change");
	                                              }];
	s_powerObserver = [notificationCenter addObserverForName:NSProcessInfoPowerStateDidChangeNotification
	                                                object:nil
	                                                 queue:NSOperationQueue.mainQueue
	                                            usingBlock:^(__unused NSNotification *notification) {
		                                            CTRPadDevice_LogState("power-change");
	                                            }];
	s_batteryLevelObserver = [notificationCenter addObserverForName:UIDeviceBatteryLevelDidChangeNotification
	                                                       object:device
	                                                        queue:NSOperationQueue.mainQueue
	                                                   usingBlock:^(__unused NSNotification *notification) {
		                                                   CTRPadDevice_LogState("battery-level-change");
	                                                   }];
	s_batteryStateObserver = [notificationCenter addObserverForName:UIDeviceBatteryStateDidChangeNotification
	                                                       object:device
	                                                        queue:NSOperationQueue.mainQueue
	                                                   usingBlock:^(__unused NSNotification *notification) {
		                                                   CTRPadDevice_LogState("battery-state-change");
	                                                   }];
	CTRPadDevice_LogState("startup");
	return 1;
}

void NativeIOSTelemetry_End(void)
{
	NSNotificationCenter *notificationCenter;

	if (!NSThread.isMainThread || (s_thermalObserver == nil))
	{
		return;
	}

	CTRPadDevice_LogState("shutdown");
	notificationCenter = NSNotificationCenter.defaultCenter;
	[notificationCenter removeObserver:s_thermalObserver];
	[notificationCenter removeObserver:s_powerObserver];
	[notificationCenter removeObserver:s_batteryLevelObserver];
	[notificationCenter removeObserver:s_batteryStateObserver];
	s_thermalObserver = nil;
	s_powerObserver = nil;
	s_batteryLevelObserver = nil;
	s_batteryStateObserver = nil;
	if (!s_batteryMonitoringWasEnabled)
	{
		UIDevice.currentDevice.batteryMonitoringEnabled = NO;
	}
}
