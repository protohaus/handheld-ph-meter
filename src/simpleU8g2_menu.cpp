/*
    The code in this file uses open source libraries provided by thecoderscorner

    DO NOT EDIT THIS FILE, IT WILL BE GENERATED EVERY TIME YOU USE THE UI
   DESIGNER INSTEAD EITHER PUT CODE IN YOUR SKETCH OR CREATE ANOTHER SOURCE
   FILE.

    All the variables you may need access to are marked extern in this file for
   easy use elsewhere.
*/

#include "simpleU8g2_menu.h"

#include <tcMenu.h>

// Global variable declarations

const PROGMEM ConnectorLocalInfo applicationInfo = {
    "pH Meter", "fddaa423-cb5c-4024-8f67-a9742f4457f3"};
U8g2GfxMenuConfig gfxConfig;
U8g2MenuRenderer renderer;

// Global Menu Item declarations

// const BooleanMenuInfo PROGMEM minfoSettingsSafetyLock = { "Safety lock", 6,
// 0xFFFF, 1, NO_CALLBACK, NAMING_TRUE_FALSE }; BooleanMenuItem
// menuSettingsSafetyLock(&minfoSettingsSafetyLock, false, NULL); const
// SubMenuInfo PROGMEM minfoSettings = { "Settings", 5, 0xFFFF, 0, NO_CALLBACK
// }; RENDERING_CALLBACK_NAME_INVOKE(fnSettingsRtCall, backSubItemRenderFn,
// "Settings", -1, NO_CALLBACK) BackMenuItem menuBackSettings(fnSettingsRtCall,
// &menuSettingsSafetyLock); SubMenuItem menuSettings(&minfoSettings,
// &menuBackSettings, NULL);
// const BooleanMenuInfo PROGMEM minfoFrozen = { "Frozen", 3, 6, 1, NO_CALLBACK,
// NAMING_YES_NO }; BooleanMenuItem menuFrozen(&minfoFrozen, false,
// &menuStartToasting);
// const char enumStrType_0[] PROGMEM = "Bread";
// const char enumStrType_1[] PROGMEM = "Bagel";
// const char enumStrType_2[] PROGMEM = "Pancake";
// const char* const enumStrType[] PROGMEM = {enumStrType_0, enumStrType_1,
//                                            enumStrType_2};
// const EnumMenuInfo PROGMEM minfoType = {"Type", 2,           4,
//                                         2,      NO_CALLBACK, enumStrType};
// EnumMenuItem menuType(&minfoType, 0, &menuStartToasting);
// const AnalogMenuInfo PROGMEM minfoToasterPower = {
//     "Messen", 1, 2, 10, NO_CALLBACK, 0, 1, "",
// };
// AnalogMenuItem menuToasterPower(&minfoToasterPower, 0, &menuType);

/// PH Sensor

const AnalogMenuInfo PROGMEM minfoEcStableReadingTotal = {
    "Messanzahl", 7, 0xFFFF, 30, onUpdateEcStableReadingTotal, 0, 0, ""};
AnalogMenuItem menuEcStableReadingTotal(&minfoEcStableReadingTotal, 0, NULL);

const AnalogMenuInfo PROGMEM minfoEcCalibrationTolerance = {
    "SD Toleranz", 6, 0xFFFF, 40, onUpdateEcCalibrationTolerance, 0, 2, ""};
AnalogMenuItem menuEcCalibrationTolerance(&minfoEcCalibrationTolerance, 0,
                                          &menuEcStableReadingTotal);

const AnyMenuInfo PROGMEM minfoEcCalibrate1413 = {
    "1413 \xb5S/cm", 5, 0xFFFF, 0, onStartEcCalibrate1413,
};
ActionMenuItem menuEcCalibrate1413(&minfoEcCalibrate1413,
                                   &menuEcCalibrationTolerance);

const AnyMenuInfo PROGMEM minfoEcCalibrate84 = {
    "84 \xb5S/cm", 4, 0xFFFF, 0, onStartEcCalibrate84,
};
ActionMenuItem menuEcCalibrate84(&minfoEcCalibrate84, &menuEcCalibrate1413);

const AnyMenuInfo PROGMEM minfoEcCalibrate84and1413 = {
    "84 und 1413 \xb5S/cm", 3, 0xFFFF, 0, onStartEcCalibrate84and1413,
};
ActionMenuItem menuEcCalibrate84and1413(&minfoEcCalibrate84and1413,
                                        &menuEcCalibrate84);

SubMenuInfo PROGMEM minfoCalibrate = {
    "Kalibrieren", 2, 0xFFFF, 0, NO_CALLBACK,
};
RENDERING_CALLBACK_NAME_INVOKE(fnCalibrateRtCall, backSubItemRenderFn,
                               "Kalibrieren", -1, NO_CALLBACK)
BackMenuItem menuBackEcCalibrate(fnCalibrateRtCall, &menuEcCalibrate84and1413);
SubMenuItem menuEcCalibrate(&minfoCalibrate, &menuBackEcCalibrate, NULL);

const AnyMenuInfo PROGMEM minfoStartEcMeasuring = {
    "Messen", 1, 0xFFFF, 0, onStartEcMeasuring,
};
ActionMenuItem menuStartEcMeasuring(&minfoStartEcMeasuring, &menuEcCalibrate);

// Set up code

void setupMenu() {
  prepareBasicU8x8Config(gfxConfig);
  renderer.setGraphicsDevice(&gfx, &gfxConfig);
  switches.initialise(ioUsingArduino(), true);
  menuMgr.initForUpDownOk(&renderer, &menuStartEcMeasuring, 14, 12, 27);
  menuMgr.setBackButton(13);
}
