// WIFI_SSID и WIFI_PASS подставляются из .env через load_env.py
// (см. extra_scripts в platformio.ini)
#define HOSTNAME "http-roller-blind"

// NTP Settings
#define NTP_SERVER "pool.ntp.org"
#define GMT_OFFSET_SEC 18000  // UTC+5 - 5 * 3600
#define DAYLIGHT_OFFSET_SEC 0 // No daylight saving adjustment

// Schedule Settings (24-hour format)
#define SCHEDULE_OPEN_HOUR 8
#define SCHEDULE_OPEN_MINUTE 0
#define SCHEDULE_CLOSE_HOUR 18
#define SCHEDULE_CLOSE_MINUTE 0
#define SCHEDULE_ENABLED true
