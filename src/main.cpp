#include <Arduino.h>
#include <config.h>
#include <WiFi.h>
#include <WebServer.h>
#include <Preferences.h>
#include <time.h>
#include "roller_blind.h"

const int BLIND_COUNT = 1;
RollerBlind blinds[BLIND_COUNT] = {
    RollerBlind(16, 17, 4, 1000000, 56000, 2500, 1200),
};

WebServer server(80);
Preferences sharedPrefs;

// Schedule tracking
bool scheduleEnabled = SCHEDULE_ENABLED;
int lastScheduleDay = -1;
bool openedToday = false;
bool closedToday = false;
int lastTimeSyncDay = -1;

void handleOpen()
{
  String response;
  int position = 100; // default: полностью открыть

  if (server.hasArg("position"))
  {
    position = server.arg("position").toInt();
    if (position < 0)
      position = 0;
    if (position > 100)
      position = 100;
  }

  for (int i = 0; i < BLIND_COUNT; i++)
  {
    if (position == 100)
      blinds[i].moveToTop();
    else
      blinds[i].moveToPercent(position);
  }

  response = "{\"status\":\"ok\",\"action\":\"open\",\"position\":" + String(position) + "}";
  server.send(200, "application/json", response);

  Serial.printf("HTTP: /open position=%d%%\n", position);
}

void handleClose()
{
  for (int i = 0; i < BLIND_COUNT; i++)
    blinds[i].moveToZero();

  server.send(200, "application/json", "{\"status\":\"ok\",\"action\":\"close\",\"position\":0}");
  Serial.println("HTTP: /close");
}

void handleStatus()
{
  String state;
  bool anyMoving = false;

  for (int i = 0; i < BLIND_COUNT; i++)
  {
    if (blinds[i].isMoving())
    {
      anyMoving = true;
      break;
    }
  }

  if (anyMoving)
  {
    state = "moving";
  }
  else
  {
    long step = blinds[0].getCurrentStep();
    if (step >= blinds[0].getStepsToTop())
      state = "open";
    else if (step <= 0)
      state = "closed";
    else
      state = "stopped";
  }

  int position = blinds[0].getPositionPercent();

  String response = "{\"state\":\"" + state + "\",\"position\":" + String(position) + "}";
  server.send(200, "application/json", response);
}

void handleTime()
{
  struct tm timeinfo;
  String response;

  if (!getLocalTime(&timeinfo))
  {
    response = "{\"error\":\"time not synced\"}";
    server.send(500, "application/json", response);
    return;
  }

  char timeStr[64];
  strftime(timeStr, sizeof(timeStr), "%Y-%m-%d %H:%M:%S", &timeinfo);

  response = "{\"time\":\"" + String(timeStr) + "\",";
  response += "\"hour\":" + String(timeinfo.tm_hour) + ",";
  response += "\"minute\":" + String(timeinfo.tm_min) + ",";
  response += "\"schedule_enabled\":" + String(scheduleEnabled ? "true" : "false") + ",";
  response += "\"open_at\":\"" + String(SCHEDULE_OPEN_HOUR) + ":" + String(SCHEDULE_OPEN_MINUTE < 10 ? "0" : "") + String(SCHEDULE_OPEN_MINUTE) + "\",";
  response += "\"close_at\":\"" + String(SCHEDULE_CLOSE_HOUR) + ":" + String(SCHEDULE_CLOSE_MINUTE < 10 ? "0" : "") + String(SCHEDULE_CLOSE_MINUTE) + "\",";
  response += "\"opened_today\":" + String(openedToday ? "true" : "false") + ",";
  response += "\"closed_today\":" + String(closedToday ? "true" : "false") + "}";

  server.send(200, "application/json", response);
}

void handleSchedule()
{
  if (server.hasArg("enabled"))
  {
    String val = server.arg("enabled");
    scheduleEnabled = (val == "true" || val == "1");
    Serial.printf("Schedule %s\n", scheduleEnabled ? "enabled" : "disabled");
  }

  String response = "{\"schedule_enabled\":" + String(scheduleEnabled ? "true" : "false") + "}";
  server.send(200, "application/json", response);
}

void handleNotFound()
{
  server.send(404, "application/json", "{\"error\":\"not found\"}");
}

void checkDailyTimeSync()
{
  struct tm timeinfo;
  if (!getLocalTime(&timeinfo))
    return;

  int currentDay = timeinfo.tm_yday;

  if (currentDay != lastTimeSyncDay &&
      timeinfo.tm_hour == 0 &&
      timeinfo.tm_min == 0 &&
      WiFi.status() == WL_CONNECTED)
  {
    lastTimeSyncDay = currentDay;
    Serial.println("Daily NTP resync at 00:00");
    configTime(GMT_OFFSET_SEC, DAYLIGHT_OFFSET_SEC, NTP_SERVER);
  }
}

void checkSchedule()
{
  if (!scheduleEnabled)
    return;

  struct tm timeinfo;
  if (!getLocalTime(&timeinfo))
    return;

  int currentDay = timeinfo.tm_yday;
  int currentHour = timeinfo.tm_hour;
  int currentMinute = timeinfo.tm_min;

  // Reset flags at midnight (new day)
  if (currentDay != lastScheduleDay)
  {
    lastScheduleDay = currentDay;
    openedToday = false;
    closedToday = false;
    Serial.printf("New day detected, schedule reset. Day: %d\n", currentDay);
  }

  // Check open time
  if (!openedToday &&
      currentHour == SCHEDULE_OPEN_HOUR &&
      currentMinute == SCHEDULE_OPEN_MINUTE)
  {
    Serial.printf("Schedule: Opening blinds at %02d:%02d\n", currentHour, currentMinute);
    for (int i = 0; i < BLIND_COUNT; i++)
      blinds[i].moveToTop();
    openedToday = true;
  }

  // Check close time
  if (!closedToday &&
      currentHour == SCHEDULE_CLOSE_HOUR &&
      currentMinute == SCHEDULE_CLOSE_MINUTE)
  {
    Serial.printf("Schedule: Closing blinds at %02d:%02d\n", currentHour, currentMinute);
    for (int i = 0; i < BLIND_COUNT; i++)
      blinds[i].moveToZero();
    closedToday = true;
  }
}

void reconnectWiFi()
{
  Serial.println("WiFi disconnected, reconnecting...");
  WiFi.disconnect();
  WiFi.begin(WIFI_SSID, WIFI_PASS);

  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 20)
  {
    delay(500);
    Serial.print(".");
    attempts++;
  }

  if (WiFi.status() == WL_CONNECTED)
  {
    Serial.println("\nWiFi reconnected!");
    Serial.print("IP address: ");
    Serial.println(WiFi.localIP());
    Serial.print("MAC address: ");
    Serial.println(WiFi.macAddress());
  }
  else
  {
    Serial.println("\nWiFi reconnect failed, will retry...");
  }
}

void setup()
{
  Serial.begin(9600);
  delay(1000);

  Serial.println("\n\n=== ESP32 Roller Blind Controller (HTTP) ===");
  Serial.print("Device: ");
  Serial.println(HOSTNAME);

  sharedPrefs.begin("blinds-shared", false);
  int sharedPercent = sharedPrefs.getInt("percent", 0);
  Serial.printf("Shared position restored: %d%%\n", sharedPercent);

  for (int i = 0; i < BLIND_COUNT; i++)
    blinds[i].begin(i, sharedPercent);

  Serial.print("Connecting to WiFi: ");
  Serial.println(WIFI_SSID);

  WiFi.mode(WIFI_STA);
  WiFi.setHostname(HOSTNAME);
  WiFi.begin(WIFI_SSID, WIFI_PASS);
  while (WiFi.status() != WL_CONNECTED)
  {
    Serial.printf("status: %d\n", WiFi.status());
    Serial.print(".");
    delay(1000);
  }

  Serial.println("\nWiFi connected!");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
  Serial.print("MAC address: ");
  Serial.println(WiFi.macAddress());

  // Configure NTP
  Serial.println("Configuring NTP...");
  configTime(GMT_OFFSET_SEC, DAYLIGHT_OFFSET_SEC, NTP_SERVER);

  struct tm timeinfo;
  if (getLocalTime(&timeinfo, 10000))
  {
    char timeStr[64];
    strftime(timeStr, sizeof(timeStr), "%Y-%m-%d %H:%M:%S", &timeinfo);
    Serial.printf("NTP synced: %s\n", timeStr);
    Serial.printf("Schedule: Open at %02d:%02d, Close at %02d:%02d\n",
                  SCHEDULE_OPEN_HOUR, SCHEDULE_OPEN_MINUTE,
                  SCHEDULE_CLOSE_HOUR, SCHEDULE_CLOSE_MINUTE);
  }
  else
  {
    Serial.println("NTP sync failed, will retry in background");
  }

  // HTTP endpoints
  server.on("/open", HTTP_GET, handleOpen);
  server.on("/close", HTTP_GET, handleClose);
  server.on("/status", HTTP_GET, handleStatus);
  server.on("/time", HTTP_GET, handleTime);
  server.on("/schedule", HTTP_GET, handleSchedule);
  server.onNotFound(handleNotFound);

  server.begin();
  Serial.println("HTTP server started on port 80");
  Serial.println("Endpoints: /open, /close, /status, /time, /schedule?enabled=true|false");

  Serial.println("Setup complete!");
}

void loop()
{
  // Проверка WiFi
  static unsigned long lastWiFiCheck = 0;
  if (WiFi.status() != WL_CONNECTED)
  {
    unsigned long now = millis();
    if (now - lastWiFiCheck > 5000)
    {
      lastWiFiCheck = now;
      reconnectWiFi();
    }
  }

  server.handleClient();

  for (int i = 0; i < BLIND_COUNT; i++)
    blinds[i].update();

  // Check schedule every second
  static unsigned long lastScheduleCheck = 0;
  unsigned long nowMillis = millis();
  if (nowMillis - lastScheduleCheck >= 1000)
  {
    lastScheduleCheck = nowMillis;
    checkDailyTimeSync();
    checkSchedule();
  }

  // Сохраняем общую позицию когда любой мотор остановился
  for (int i = 0; i < BLIND_COUNT; i++)
  {
    if (blinds[i].hasJustStopped())
    {
      int percent = blinds[0].getPositionPercent();
      sharedPrefs.putInt("percent", percent);
      Serial.printf("Shared position saved: %d%%\n", percent);
      break;
    }
  }

  // Проверка таймаута
  for (int i = 0; i < BLIND_COUNT; i++)
  {
    if (blinds[i].hasTimeoutOccurred())
    {
      Serial.printf("CRITICAL: ROLLER BLIND %d TIMEOUT!\n", i);
      blinds[i].clearTimeoutFlag();
    }
  }
}
