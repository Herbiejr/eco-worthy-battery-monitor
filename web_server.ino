// Station Mode (STA) - Client
// Soft Access Point Mode (SAP) - Access Point (max 5 connections)
// https://lastminuteengineers.com/creating-esp32-web-server-arduino-ide/
// https://www.youtube.com/watch?v=rw_1E-2Dwrs
// https://github.com/mike805/eco-worthy-battery-logger/blob/main/ewbatlog.py

#include <WiFi.h>
#include <NimBLEDevice.h>
#include <LittleFS.h>
#include <Preferences.h>
#include <WiFiManager.h>	// Include WiFiManager FIRST
#include <ESPmDNS.h>
#include <time.h>
#include <ArduinoOTA.h>
#include <WiFiMulti.h>
// === CRITICAL: Prevent HTTP_GET conflict ===
#define WEBSERVER_H
// Include AsyncWebServer AFTER the define
#include <ESPAsyncWebServer.h>
#include <ArduinoJson.h>
#include <WiFiClientSecure.h>
#include <HTTPClient.h>

#define blueLedPin 2

// Discovered Advertised Device: Name: DP04S007L4S100A, Address: a5:c2:37:6e:14:e7, serviceUUID: 0xff00 
// Advertised Device Result: Name: DP04S007L4S100A, Address: a5:c2:37:6e:14:e7, manufacturer data: e7146e37c2a5, serviceUUID: 0xff00 
// Discovered Advertised Device: Name: DP04S007L4S100A, Address: a5:c2:37:6e:13:96, serviceUUID: 0xff00 
// Advertised Device Result: Name: DP04S007L4S100A, Address: a5:c2:37:6e:13:96, manufacturer data: 96136e37c2a5, serviceUUID: 0xff00 

#define BATTERY1_MAC "A5:C2:37:6E:13:96"
#define BATTERY2_MAC "A5:C2:37:6E:14:E7"

static NimBLEUUID serviceUUID("ff00");	// SERVICE_UUID "0000ff00-0000-1000-8000-00805f9b34fb"
static NimBLEUUID charUUID("ff02");	// CHARACTERISTIC_WRITE_UUID "0000ff02-0000-1000-8000-00805f9b34fb"
static NimBLEUUID charUUIDread("ff01");	// CHARACTERISTIC_READ_UUID "0000ff01-0000-1000-8000-00805f9b34fb"

struct BatteryData{
	float voltage = 0;
	float current = 0;
	float watts = 0;
	float soc = 0;
	float temp = 0;
	float rCap = 0;
	float nCap = 0;
	float unknownConstant0 = 0;
	float cycCount = 0;
	float reservedFlags = 0;
	float bStatus = 0;
	float unknownConstant1 = 0;
	bool chargeMos = false;
	bool dischargeMos = false;
	bool balanceActive = false;
	bool chargeFETstatus = false;
	bool dischargeFETstatus = false;
	bool protectionActive = false;
	bool bluetooth_connected = false;
	bool systemOn = false;
	float cellCount = 0;
	float tempSenCount = 0;
	float reserved0 = 0;
	float ahMaxDup = 0;
	float ahRemDup = 0;
	float reserved1 = 0;
	float checksum = 0;
	float endByte = 0;

	bool connected = false;
	unsigned long lastUpdate = 0;

	float cell[4] = {0, 0, 0, 0};
	float cellDiff = 0;
};

BatteryData battery1;
std::vector<uint8_t> battery1Buffer;
NimBLEClient* battery1Client = nullptr;
NimBLERemoteCharacteristic* battery1WriteChar = nullptr;
NimBLERemoteCharacteristic* battery1NotifyChar = nullptr;

BatteryData battery2;
std::vector<uint8_t> battery2Buffer;
NimBLEClient* battery2Client = nullptr;
NimBLERemoteCharacteristic* battery2WriteChar = nullptr;
NimBLERemoteCharacteristic* battery2NotifyChar = nullptr;
bool battery1Busy = false;
bool battery2Busy = false;

static WiFiManager wm;
// WebServer server(80);
AsyncWebServer server(80);
WiFiMulti wifiMulti;

bool wifiConnected = false;
bool ntpSynced = false;
IPAddress currentIP;
IPAddress currentGateway;
String publicIP;
IPAddress lastIP;
unsigned long lastInternetCheck = 0;
unsigned long lastLocationCheck = 0;
unsigned long wifiConnectedAt = 0;

bool sunDataNeedsUpdate = false;

bool LEDStatus = LOW;
// bool LEDStatus = false;
bool loggingEnabled = true;

unsigned long lastBLERead = 0;
unsigned long lastReconnectAttempt = 0;
unsigned long lastLog = 0;
unsigned long logInterval = 60000; // 60 seconds default

unsigned long lastCellQuery = 0;
bool cellsVisible = false;
bool cells1visible = false;
bool cells2visible = false;

String battery1Error = "";
String battery2Error = "";

Preferences prefs;
size_t currentLogSize = 0;

const char* API_TOKEN = "Token 9b09b8c1f47f22498206143bc1ff33797e9d1c50";
const int GEONAME_ID = 6078636;   // Morinville
const char* JSON_FILE = "/SunData.json";
DynamicJsonDocument sunDoc(24576);	// may be too small or larger than necessary.
static unsigned long lastSunCheck = 0;
static unsigned long lastSunCalc = 0;

struct LocationInfo {
	bool autoLocation = true;

	uint32_t geonameID = 0;

	String city;
	String province;
	String country;

	float latitude = 0;
	float longitude = 0;
};

LocationInfo location;

bool sunDataLoaded = false;
int currentSunDay = -1;
int currentSunMonth = -1;

// South facing = 180°
// East facing  = 90°
// West facing  = 270°
// North facing = 0°
float panelAzimuth  = 180.0;   // south
float panelTilt     = 45.0;

void setup(){
	Serial.begin(115200);

	prefs.begin("logger", false);
	loggingEnabled = prefs.getBool("logging", true);
	logInterval = prefs.getULong("interval", 60000);
	Serial.printf("Log interval loaded: %lu ms\n", logInterval);
	
	panelTilt = prefs.getFloat("tilt", panelTilt);
	panelAzimuth = prefs.getFloat("azimuth", panelAzimuth);

	location.geonameID = prefs.getUInt("geonameID", GEONAME_ID);

	location.city = prefs.getString("city", "");
	location.province = prefs.getString("province", "");
	location.country = prefs.getString("country", "");

	location.latitude = prefs.getFloat("latitude", 0.0);
	location.longitude = prefs.getFloat("longitude", 0.0);

	if (!LittleFS.begin(true)) {
		Serial.println("LittleFS Mount Failed");
		loggingEnabled = false;
	}

	// Create CSV file if it doesn't exist
	if (!LittleFS.exists("/battery.csv")) {
		File f = LittleFS.open("/battery.csv", FILE_WRITE);
		if (f) {
			f.println(
				"time,"
				"b1_voltage,b1_current,b1_watts,b1_soc,b1_temp,b1_capRemain,"
				"b2_voltage,b2_current,b2_watts,b2_soc,b2_temp,b2_capRemain,"
				"tot_volts,tot_amp,tot_watt,avg_soc,avg_temp,avg_capRemain"
			);
			currentLogSize = f.size();
			f.close();
		}
	}
	if (LittleFS.exists("/battery.csv")) {
		File f = LittleFS.open("/battery.csv", FILE_READ);
		if (f) {
			currentLogSize = f.size();
			f.close();
		}
	}

	// List files (helpful for debugging)
	// File root = LittleFS.open("/");
	// File file = root.openNextFile();
	// while(file){
	// 	Serial.printf("LittleFS FILE: %s (%u bytes)\n", file.name(), file.size());
	// 	file = root.openNextFile();
	// }

	// pinMode(blueLedPin, OUTPUT);
	// digitalWrite(blueLedPin, LOW);
	
	// === WiFi Connection ===
	WiFi.onEvent(onWiFiEvent);
	prefs.putBool("autoLoc", location.autoLocation);
	location.autoLocation = prefs.getBool("autoLoc", true);
	if(!wm.autoConnect("BatteryMonitor", "12345678")) {
		Serial.println("WiFi Connection Failed");
		Serial.println("Starting Captive Portal");
	}else{
		Serial.println("WiFi Connected");
		wifiConnected = true;
		Serial.print("IP: ");
		Serial.println(WiFi.localIP());

		Serial.print("AP IP: ");
		Serial.println(WiFi.softAPIP());

		Serial.print("STA IP: ");
		Serial.println(WiFi.localIP());

		Serial.print("Gateway: ");
    Serial.println(WiFi.gatewayIP());
	}

	while (WiFi.status() != WL_CONNECTED) {
		delay(500);
		Serial.print(".");
	}
	
	// === OTA, Time, mDNS ===
	ArduinoOTA.setHostname("BatteryMonitor");
	ArduinoOTA.begin();

	if(WiFi.status() == WL_CONNECTED){
		// configTime(0, 0, "pool.ntp.org", "time.nist.gov");
		configTzTime("MST7MDT,M3.2.0/2,M11.1.0/2", "pool.ntp.org", "time.nist.gov");
		Serial.println("Waiting for NTP");

		struct tm timeinfo;
		uint32_t ntpStart = millis();
		while (!getLocalTime(&timeinfo)){
			delay(500);
			Serial.print(".");

			if(millis() - ntpStart > 30000){
				Serial.println();
				Serial.println("NTP timeout");
				break;
			}
		}

		if(getLocalTime(&timeinfo)){
			ntpSynced = true;
			Serial.printf(
        "%04d-%02d-%02d %02d:%02d:%02d %s\n",
        timeinfo.tm_year + 1900,
        timeinfo.tm_mon + 1,
        timeinfo.tm_mday,
        timeinfo.tm_hour,
        timeinfo.tm_min,
        timeinfo.tm_sec,
				" - Time Synced"
	    );
	
			currentSunDay = timeinfo.tm_mday;
	    currentSunMonth = timeinfo.tm_mon;

			if(cacheIsCurrent()){
				Serial.println("Using cached monthly data");
        loadJson();
	    }else{
				if (prefs.getUInt("geonameID", 0) == 0){
					  prefs.putUInt("geonameID", GEONAME_ID);
				}
				location.geonameID = prefs.getUInt("geonameID", GEONAME_ID);
				Serial.printf("geonameID = %u\n", location.geonameID);
				if(location.geonameID != 0){
					if(!cacheIsCurrent()){
						Serial.println("Cache is not Current");
						String payload = downloadMonthlyData();
						if(payload.length() > 0){
							saveSunData(payload);
						}else{
							Serial.println("Download failed.");
						}
					}

					if(loadJson()){
						updateTodaySunInfo();
						saveLastUpdate();
					}else{
						Serial.println("loadJSON Failed!");
					}
    		}
			}
			publicIP = getPublicIP();
		}
	}

	if(MDNS.begin("batterymonitor")){
		MDNS.addService("http", "tcp", 80);
		Serial.println("mDNS Started");
	}

	// === Web Server Routes ===
	// Serve all static files from LittleFS (css, js, html, etc.)
	server.serveStatic("/", LittleFS, "/").setDefaultFile("index.html");

	server.on("/", HTTP_GET, handleRoot);
	server.on("/settings", HTTP_GET, handleSettings);
	server.on("/ledon", HTTP_GET, handleLedON);
	server.on("/ledoff", HTTP_GET, handleLedOFF);
	server.on("/status", HTTP_GET, handleStatus);
	server.on("/download", HTTP_GET, handleDownload);
	server.on("/history", HTTP_GET, handleHistory);
	server.on("/startlogging", HTTP_GET, handleStartLogging);
	server.on("/stoplogging", HTTP_GET, handleStopLogging);
	server.on("/clearlog", HTTP_GET, handleClearLog);
	server.on("/setinterval", HTTP_GET, handleSetInterval);
	server.on("/resetwifi", HTTP_GET, handleResetWifi);
	server.on("/cells", HTTP_GET, handleCells);
	server.on("/cells1", HTTP_GET, handleCells1);
	server.on("/cells2", HTTP_GET, handleCells2);
	server.on("/solar", HTTP_GET, handleSolarData);
	server.on("/sundata", HTTP_GET, handleSunData);
	server.on("/downloadsundata", HTTP_GET, handleDownloadSunData);
	server.on("/setazimuth", HTTP_GET, handleSetPanelAzimuth);
	server.on("/settilt", HTTP_GET, handleSetPanelTilt);
	server.on("/nearbycities", HTTP_GET, handleNearbyCities);
	server.on("/setcity", HTTP_GET, handleSetCity);
	server.on("/detectlocation", HTTP_GET, handleDetectLocation);

	server.onNotFound(handleNotFound);

	server.begin();
	Serial.println("HTTP Server Started");
	delay(1000);  // Give WiFi and BLE stack time to settle
	
	esp_coex_preference_set(ESP_COEX_PREFER_BALANCE);
	
	//NimBLEDevice::init("ESP32_BMS_Manager");
	//NimBLEDevice::setPower(ESP_PWR_LVL_P9);  // Optional: reduce power issues
	//NimBLEScan* pBLEScan = NimBLEDevice::getScan();
	// CRITICAL: 25% Duty Cycle scan window leaves 75% of radio time open for Wi-Fi
    //NimBLEDevice->setInterval(200); 
    //NimBLEDevice->setWindow(50);    
    //NimBLEDevice->setActiveScan(false);


	// Serial.println("Connecting to Battery 1...");
	// connectBattery1();
	// delay(1000);

	// Serial.println("Connecting to Battery 2...");
	// connectBattery2();
	// delay(1000);

	Serial.println("Setup completed!");
}

void loop(){
	ArduinoOTA.handle();
	
	// static unsigned long lastHeap = 0;
	// if (millis() - lastHeap > 5000) {
	// 		lastHeap = millis();
	// 		Serial.printf("Free Heap: %d bytes\n", ESP.getFreeHeap());
	// }

	if (wifiConnected && sunDataNeedsUpdate && millis() - wifiConnectedAt > 5000) {
		Serial.println("Sundata Needs Update");
    	sunDataNeedsUpdate = false;
		Serial.println("Disabled Bluetooth");
		NimBLEDevice::deinit(true);
		delay(300);
		getCurrentISO8601();

	    if(location.autoLocation){
			bool success = true;
			if (!getLocationFromIP()) {
				Serial.println("Location lookup failed");
				success = false;
			}
			if (success && !lookupNearestCities()) {
				Serial.println("Nearest Cities lookup failed");
				success = false;
			}
			if (success) {
				// if geoname changed
				String payload = downloadMonthlyData();
				if (payload.length() == 0) {
					Serial.println("Sun data download failed");
					success = false;
				}
			}
			if (success) {
				saveSunData(payload);
				if (!loadJson()) {
					Serial.println("loadJson() failed");
	            	success = false;
				}
			}
			if(success){
				updateTodaySunInfo();
			}
			
			Serial.println("Re-enabling Bluetooth");
			NimBLEDevice::init("");
			NimBLEDevice::setPower(ESP_PWR_LVL_P9);  // Optional: reduce power issues
	
			if(!success){
				return;
			}
		}
	}

	if(millis() - lastBLERead > 2000){
		// Serial.println("Querying Bat 1");
		lastBLERead = millis();
		if (battery1Busy) return;
		battery1Busy = true;
		// Serial.printf("Battery 1 busy = %d\n", battery1Busy);
		queryBattery1();
		// Serial.println("Querying Bat 1 complete");
		if (cells1visible) {
			delay(500);
			queryBattery1Cells();
			delay(500);

    }
		battery1Busy = false;
		// Serial.printf("Battery 1 busy = %d\n", battery1Busy);

		// Serial.println("Querying Bat 2");
		if (battery2Busy) return;
		battery2Busy = true;
		// Serial.printf("Battery 2 busy = %d\n", battery2Busy);
		queryBattery2();
		// Serial.println("Querying Bat 2 Complete");
    if (cells2visible) {
			delay(500);
			queryBattery2Cells();
			delay(500);
    }
		battery2Busy = false;
		// Serial.printf("Battery 2 busy = %d\n", battery2Busy);
	}
	
	if (battery1.connected && millis() - battery1.lastUpdate > 15000){
		Serial.println("Checking for Bat 1 last Update");
		battery1.connected = false;
		battery1Error = "Data Timeout";

		if (battery1Client) {
			battery1Client->disconnect();
			battery1WriteChar = nullptr;
			battery1NotifyChar = nullptr;
		}
	}

	if (battery2.connected && millis() - battery2.lastUpdate > 15000){
		Serial.println("Checking for Bat 2 last Update");
		battery2.connected = false;
		battery2Error = "Data Timeout";

		if (battery2Client) {
			battery2Client->disconnect();
			battery2WriteChar = nullptr;
			battery2NotifyChar = nullptr;
		}
	}

	if (millis() - lastReconnectAttempt > 23300){
		// Serial.println("Checking for last reconnect attempt");
		lastReconnectAttempt = millis();
		// Serial.printf("Last Reconnect Attempt = %s\n", lastReconnectAttempt);

		// if (battery1Busy) return;
		battery1Busy = true;
		// Serial.printf("Battery 1 busy = %d\n", battery1Busy);
		// Serial.printf("battery1Client = %p\n", battery1Client);
		// Serial.printf("battery2Client = %p\n", battery2Client);
		if (!battery1Client || !battery1Client->isConnected()){
			Serial.println("Reconnecting Battery 1...");
			if (battery1Client){
				battery1Client->disconnect();
				delay(50); // let BLE stack settle
				NimBLEDevice::deleteClient(battery1Client);
				battery1Client = nullptr;
			}
			connectBattery1();
		}
		battery1Busy = false;
		// Serial.printf("Battery 1 busy = %d\n", battery1Busy);

		// if (battery2Busy) return;
		battery2Busy = true;
		// Serial.printf("Battery 2 busy = %d\n", battery2Busy);
		if (!battery2Client || !battery2Client->isConnected()){
			Serial.println("Reconnecting Battery 2...");
			if (battery2Client){
				battery2Client->disconnect();
				delay(50); // let BLE stack settle
				NimBLEDevice::deleteClient(battery2Client);
				battery2Client = nullptr;
			}
			connectBattery2();
		}
		battery2Busy = false;
		// Serial.printf("Battery 2 busy = %d\n", battery2Busy);
	}
	
	if (loggingEnabled && millis() - lastLog >= logInterval) {
		Serial.println("Checking the logging interval");
		lastLog = millis();
		if ((battery1.connected && battery1.lastUpdate) || (battery2.connected && battery2.lastUpdate)) {
			logBatteryData();
		}
	}

	if(millis() - lastSunCheck > 360000){   // once per hour
		// Serial.println("Checking last sun Check");
		lastSunCheck = millis();

		struct tm timeinfo;
		if(getLocalTime(&timeinfo)){
			// Month changed
			if(timeinfo.tm_mon != currentSunMonth){
				currentSunMonth = timeinfo.tm_mon;
				Serial.println("New month detected");

				if(downloadMonthlyData().length() > 0){
					loadJson();
					updateTodaySunInfo();
				}
			}
			// Day changed
			else if(timeinfo.tm_mday != currentSunDay){
				currentSunDay = timeinfo.tm_mday;
				Serial.println("New day detected");
				updateTodaySunInfo();
			}
		}
	}

	if(millis() - lastSunCalc > 60000){
		// Serial.println("Checking last suncalc");
		lastSunCalc = millis();
		updateSunPosition();
		calculatePanelIncidence();
	}

	if(millis() - lastInternetCheck > 1800000){
		Serial.println("Checking last internet check");
		lastInternetCheck = millis();
		String oldIP = publicIP;
		Serial.println("Disabled Bluetooth");
		NimBLEDevice::deinit(true);
		publicIP = getPublicIP();

		if(oldIP != publicIP){
			Serial.println("Public IP Changed");

			if(location.autoLocation){
				if (!getLocationFromIP()) {
					Serial.println("Location lookup failed");
					Serial.println("Re-enabling Bluetooth");
					NimBLEDevice::init("");
					NimBLEDevice::setPower(ESP_PWR_LVL_P9);  // Optional: reduce power issues
					return;
				}
				// lookupNearestCities();
				// if geoname changed
				String payload = downloadMonthlyData();
				if (payload.length() == 0) {
					Serial.println("Sun data download failed");
					Serial.println("Re-enabling Bluetooth");
					NimBLEDevice::init("");
					NimBLEDevice::setPower(ESP_PWR_LVL_P9);  // Optional: reduce power issues
					return;
				}
				saveSunData(payload);
				if (loadJson()) {
					updateTodaySunInfo();
					Serial.println("Re-enabling Bluetooth");
					NimBLEDevice::init("");
					NimBLEDevice::setPower(ESP_PWR_LVL_P9);  // Optional: reduce power issues
				}
			}
		}
	}
}
