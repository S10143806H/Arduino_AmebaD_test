/*
  This sketch shows how to use power save deepsleep mode
*/
#include "ameba_soc.h"
#include <PowerSave.h>

#define DEEPSLEEP_MODE                  11
#define SET_DS_AON_TIMER_WAKEUP         0
#define SET_DS_AON_WAKEPIN_WAKEUP       1
#define SET_DS_RTC_WAKEUP               2

//SET_DS_AON_TIMER_WAKEUP:0; SET_DS_AON_WAKEPIN_WAKEUP:1; SET_DS_RTC_WAKEUP:2;
#define DS_WAKEUP_SOURCE                SET_DS_AON_TIMER_WAKEUP

#define AON_TIMER_SLEEP_DURATION        5000
#define DS_RTC_ALARM_DAY                0
#define DS_RTC_ALARM_HOUR               0
#define DS_RTC_ALARM_MIN                0
#define DS_RTC_ALARM_SEC                10
#define SET_DS_AON_WAKEPIN_WAKEUPPIN    16    // D16:16    D17:17    D26:26    D27:27

#if (DS_WAKEUP_SOURCE == SET_DS_AON_TIMER_WAKEUP)
    char DS_AON_TIMER_WAKEUP[] = "Set Deepsleep wakeup AON timer";
#elif (DS_WAKEUP_SOURCE == SET_DS_RTC_WAKEUP)
    char DS_RTC_WAKEUP[] = "Set Deepsleep wakeup RTC";
#elif (DS_WAKEUP_SOURCE == SET_DS_AON_WAKEPIN_WAKEUP)
    #if (SET_DS_AON_WAKEPIN_WAKEUPPIN == 16)
        char DS_AON_WAKEPIN_WAKEUP_D16[] = "Set Deepsleep wakeup AON pin D16";
    #elif (SET_DS_AON_WAKEPIN_WAKEUPPIN == 17)
        char DS_AON_WAKEPIN_WAKEUP_D17[] = "Set Deepsleep wakeup AON pin D17";
    #elif (SET_DS_AON_WAKEPIN_WAKEUPPIN == 26)
        char DS_AON_WAKEPIN_WAKEUP_D26[] = "Set Deepsleep wakeup AON pin D26";
    #elif (SET_DS_AON_WAKEPIN_WAKEUPPIN == 27)
        char DS_AON_WAKEPIN_WAKEUP_D27[] = "Set Deepsleep wakeup AON pin D27";
    #endif
#endif


void DeepSleep_wakeup(void) {
    printf("\r\nDeep sleep wakeuped! \r\n");
    uint32_t wakereason_number = PowerSave.AONWakeReason();

    if (wakereason_number == 11) {
        printf("AonWakepin wakeup, wakepin is D%d. Wait 5s sleep again.    \r\n", PowerSave.WakePinCheck());
        delay(5000);
    }

    if (wakereason_number == 22) {
        PowerSave.AONTimerCmd();
        printf("Aontimer wakeup. Wait 5s sleep again.    \r\n");
        delay(5000);
    }

    if (wakereason_number == 33) {
        printf("RTC wakeup. Wait 5s sleep again.    \r\n");
        delay(5000);
    }

    PowerSave.AONWakeClear();
}

void setup() {
    // Open serial communications and wait for port to open:
    Serial.begin(115200);
    while (!Serial) {
        ; // wait for serial port to connect. Needed for native USB port only
    }

    PowerSave.begin(DEEPSLEEP_MODE);

    if (TRUE == (PowerSave.DsleepWakeStatusGet())) {
        DeepSleep_wakeup();
    }

    PowerSave.AONTimerDuration(0);

#if (DS_WAKEUP_SOURCE == SET_DS_AON_TIMER_WAKEUP)
    printf("%s.    \r\n", DS_AON_TIMER_WAKEUP);
#elif (DS_WAKEUP_SOURCE == SET_DS_RTC_WAKEUP)
    printf("%s.    \r\n", DS_RTC_WAKEUP);
#elif (DS_WAKEUP_SOURCE == SET_DS_AON_WAKEPIN_WAKEUP)
    #if (SET_DS_AON_WAKEPIN_WAKEUPPIN == 16)
        printf("%s.    \r\n", DS_AON_WAKEPIN_WAKEUP_D16);
    #elif (SET_DS_AON_WAKEPIN_WAKEUPPIN == 17)
        printf("%s.    \r\n", DS_AON_WAKEPIN_WAKEUP_D17);
    #elif (SET_DS_AON_WAKEPIN_WAKEUPPIN == 26)
        printf("%s.    \r\n", DS_AON_WAKEPIN_WAKEUP_D26);
    #elif (SET_DS_AON_WAKEPIN_WAKEUPPIN == 27)
        printf("%s.    \r\n", DS_AON_WAKEPIN_WAKEUP_D27);
    #endif
#endif

    switch (DS_WAKEUP_SOURCE) {
        case SET_DS_AON_TIMER_WAKEUP:
            PowerSave.AONTimerDuration(AON_TIMER_SLEEP_DURATION);
            break;

        case SET_DS_AON_WAKEPIN_WAKEUP:
            break;

        case SET_DS_RTC_WAKEUP:
            PowerSave.RTCWakeSetup(DS_RTC_ALARM_DAY, DS_RTC_ALARM_HOUR, DS_RTC_ALARM_MIN, DS_RTC_ALARM_SEC);
            break;

        default:
            printf("Unknown wakeup source.    \r\n");
        break;
    }

    PowerSave.enable();
}

void loop() {
    delay(1000);
}
