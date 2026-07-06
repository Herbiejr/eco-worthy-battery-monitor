void handleRoot(AsyncWebServerRequest *request){
	Serial.println("ROOT REQUEST");
	request->send(LittleFS, "/index.html", "text/html");
}

void handleWifiSetup(AsyncWebServerRequest *request) {
	request->send(200, "Starting Config Portal", "text/plain");
	wm.startConfigPortal("BatteryMonitor");
}

void handleResetWifi(AsyncWebServerRequest *request){
	Serial.println("RESET WIFI REQUEST");
	request->send(200, "WiFi reset", "text/plain");
	delay(1000);
	wm.resetSettings();
	ESP.restart();
}

void handleNotFound(AsyncWebServerRequest *request){
	request->send(404, "text/plain", "404 Not Found");
}

void handleSetPanelTilt(AsyncWebServerRequest *request){
	if (!request->hasArg("value")){
		request->send(400, "Missing value", "text/plain");
		return;
	}

	panelTilt = request->arg("value").toFloat();
	prefs.putFloat("tilt", panelTilt);
	Serial.printf("Panel Tilt = %.1f\n", panelTilt);

	request->send(200, "OK", "text/plain");
}

void handleSetPanelAzimuth(AsyncWebServerRequest *request) {
	if (!request->hasArg("value")){
		request->send(400, "Missing value", "text/plain");
		return;
	}

	panelAzimuth = request->arg("value").toFloat();
	prefs.putFloat("azimuth", panelAzimuth);
	Serial.printf("Panel Azimuth = %.1f\n", panelAzimuth);

	request->send(200, "OK", "text/plain");
}

void handleSolarData(AsyncWebServerRequest *request) {
	Serial.println("SOLAR REQUEST");
	request->send(LittleFS, "/solar.html", "text/html");
}

void handleSunData(AsyncWebServerRequest *request) {
	if(!LittleFS.exists("/SunData.json")){
		request->send(404, "text/plain", "File not found");
		return;
	}

	request->send(LittleFS, "/SunData.json", "application/json", false);	// The false means "don't download, display in browser".
}

void handleDownloadSunData(AsyncWebServerRequest *request) {
	if(!LittleFS.exists("/SunData.json")){
		request->send(404, "text/plain", "File not found");
		return;
	}

	request->send(LittleFS, "/SunData.json", "application/json", true);	// The true makes the browser save the file.
}

void handleStartLogging(AsyncWebServerRequest *request){
	loggingEnabled = true;
	lastLog = millis();
	prefs.putBool("logging", true);
	request->send(200, "text/plain", "Logging Started");
	Serial.println("Logging Started");
}

void handleStopLogging(AsyncWebServerRequest *request){
	loggingEnabled = false;
	prefs.putBool("logging", false);
	request->send(200, "text/plain", "Logging Stopped");
	Serial.println("Logging Stopped");
}

void handleClearLog(AsyncWebServerRequest *request){
	bool oldLogging = loggingEnabled;
	loggingEnabled = false;

	LittleFS.remove("/battery.csv");
	File f = LittleFS.open("/battery.csv", FILE_WRITE);

	if (f){
		f.println(
			"time,"
			"b1_voltage,b1_current,b1_watts,b1_soc,b1_temp,b1_capRemain,"
			"b2_voltage,b2_current,b2_watts,b2_soc,b2_temp,b2_capRemain"
		);
		currentLogSize = f.size();
		f.close();
	}

	loggingEnabled = oldLogging;
	
	request->send(200, "Log Cleared", "text/plain");
	Serial.println("Log Cleared");
}

void handleSetInterval(AsyncWebServerRequest *request){
	if (!request->hasArg("sec")){
		request->send(400, "Missing sec", "text/plain");
		return;
	}

	int sec = request->arg("sec").toInt();

	if (sec < 1)
		sec = 1;

	if (sec > 3600)
		sec = 3600;

	logInterval = (unsigned long)sec * 1000UL;
	lastLog = millis();
	prefs.putULong("interval", logInterval);

	Serial.printf("Log interval set to %d seconds\n", sec);
	request->send(200, "OK", "text/plain");
}

void handleHistory(AsyncWebServerRequest *request) {
	Serial.println("HISTORY REQUEST");
	File f = LittleFS.open("/battery.csv", FILE_READ);

	if (!f) {
		request->send(500, "application/json", "{\"error\":\"file open failed\"}");
		return;
	}

	String json = "[";
	bool first = true;

	// Skip header line
	f.readStringUntil('\n');

	while (f.available()) {
		String line = f.readStringUntil('\n');
		line.trim();

		if (line.length() == 0)
			continue;

		char buf[256];
		line.toCharArray(buf, sizeof(buf));

		char *token = strtok(buf, ",");

		if (!first)
			json += ",";
		first = false;

		json += "{";

		json += "\"time\":\"" + String(token) + "\"";

		token = strtok(NULL, ",");
		json += ",\"b1_voltage\":" + String(token);

		token = strtok(NULL, ",");
		json += ",\"b1_current\":" + String(token);

		token = strtok(NULL, ",");
		json += ",\"b1_watts\":" + String(token);

		token = strtok(NULL, ",");
		json += ",\"b1_soc\":" + String(token);

		token = strtok(NULL, ",");
		json += ",\"b1_temp\":" + String(token);

		token = strtok(NULL, ",");
		json += ",\"b1_capRemain\":" + String(token);

		token = strtok(NULL, ",");
		json += ",\"b2_voltage\":" + String(token);

		token = strtok(NULL, ",");
		json += ",\"b2_current\":" + String(token);

		token = strtok(NULL, ",");
		json += ",\"b2_watts\":" + String(token);

		token = strtok(NULL, ",");
		json += ",\"b2_soc\":" + String(token);

		token = strtok(NULL, ",");
		json += ",\"b2_temp\":" + String(token);

		token = strtok(NULL, ",");
		json += ",\"b2_capRemain\":" + String(token);

		json += "}";
	}

	json += "]";
	f.close();

	request->send(200, "application/json", json);
}

void handleDownload(AsyncWebServerRequest *request){
	// Serial.printf("Free Heap before download: %d bytes\n", ESP.getFreeHeap());

	if (!LittleFS.exists("/battery.csv")) {
		request->send(404, "text/plain", "No Log File Found");
		return;
	}

	request->send(LittleFS, "/battery.csv", "text/csv", true);
}

void handleStatus(AsyncWebServerRequest *request){
	// Serial.println("STATUS REQUEST");
	unsigned long seconds = millis() / 1000;

	unsigned long days    = seconds / 86400;
	unsigned long hours   = (seconds % 86400) / 3600;
	unsigned long minutes = (seconds % 3600) / 60;
	unsigned long secs    = seconds % 60;

	char uptimeStr[32];

	snprintf(uptimeStr, sizeof(uptimeStr), "%lud-%02lu:%02lu:%02lu", days, hours, minutes, secs);

	float totalVoltage = 0;
	float totalCurrent = 0;
	float totalWatts = 0;
	float totalSOC = 0;
	float totalTemp = 0;

	float totalRemainingAh = 0;
	float totalCapacityAh = 0;

	int connectedCount = 0;

	if(battery1.connected){
    totalVoltage += battery1.voltage;
    totalCurrent += battery1.current;
    totalTemp += battery1.temp;

    totalRemainingAh += battery1.rCap;
    totalCapacityAh += battery1.nCap;

    connectedCount++;
	}

	if(battery2.connected){
    totalVoltage += battery2.voltage;
    totalCurrent += battery2.current;
    totalTemp += battery2.temp;

    totalRemainingAh += battery2.rCap;
    totalCapacityAh += battery2.nCap;

    connectedCount++;
	}

	if(connectedCount > 0){
    totalVoltage /= connectedCount;
    totalTemp /= connectedCount;
	}

	totalWatts = totalVoltage * totalCurrent;

	if(totalCapacityAh > 0){
    totalSOC = (totalRemainingAh / totalCapacityAh) * 100.0;
	}

	float cellDelta1 = getCellDelta(battery1);
	float cellDelta2 = getCellDelta(battery2);

  StaticJsonDocument<1536> doc;

  doc["led"] = LEDStatus;
	
  doc["uptime"] = uptimeStr;
  doc["clients"] = String(WiFi.softAPgetStationNum());

  doc["battery1_voltage"] = String(battery1.voltage,2);
  doc["battery1_current"] = String(battery1.current,2);
  doc["battery1_watts"] = String(battery1.watts,2);
  doc["battery1_soc"] = battery1.soc;
  doc["battery1_temp"] = battery1.temp;
  doc["battery1_unknownConstant0"] = battery1.unknownConstant0;
  doc["battery1_cycle_count"] = battery1.cycCount;
  doc["battery1_reservedFlags"] = battery1.reservedFlags;
  doc["battery1_bal_status"] = battery1.bStatus;
  doc["battery1_unknownConstant1"] = battery1.unknownConstant1;
  doc["battery1_chargeMos"] = battery1.chargeMos ? "true":"false";
  doc["battery1_dischargeMos"] = battery1.dischargeMos ? "true":"false";
  doc["battery1_balanceActive"] = battery1.balanceActive ? "true":"false";
  doc["battery1_chargeFETstatus"] = battery1.chargeFETstatus ? "true":"false";
  doc["battery1_dischargeFETstatus"] = battery1.dischargeFETstatus ? "true":"false";
  doc["battery1_protectionActive"] = battery1.protectionActive ? "true":"false";
  doc["battery1_bluetooth_connected"] = battery1.bluetooth_connected ? "true":"false";
  doc["battery1_systemOn"] = battery1.systemOn ? "true":"false";
  doc["battery1_cellCount"] = battery1.cellCount;
  doc["battery1_remainCap"] = battery1.rCap;
  doc["battery1_maxCap"] = battery1.nCap;
  doc["battery1_connected"] = battery1.connected ? "true":"false";
  doc["battery1_age"] = battery1.lastUpdate ? String((millis() - battery1.lastUpdate)/1000) : "-1";
  doc["battery1_error"] = battery1Error;
  doc["battery1_tempSenCount"] = battery1.tempSenCount;
  doc["battery1_reserved0"] = battery1.reserved0;
  doc["battery1_ahMaxDup"] = battery1.ahMaxDup;
  doc["battery1_ahRemDup"] = battery1.ahRemDup;
  doc["battery1_reserved1"] = battery1.reserved1;
  doc["battery1_checksum"] = battery1.checksum;
  doc["battery1_endByte"] = battery1.endByte;
  doc["battery1_cell1"] = String(battery1.cell[0],3);
  doc["battery1_cell2"] = String(battery1.cell[1],3);
  doc["battery1_cell3"] = String(battery1.cell[2],3);
  doc["battery1_cell4"] = String(battery1.cell[3],3);
  doc["battery1_cellDiff"] = String(battery1.cellDiff,3);

  doc["battery2_voltage"] = String(battery2.voltage,2);
  doc["battery2_current"] = String(battery2.current,2);
  doc["battery2_watts"] = String(battery2.watts,2);
  doc["battery2_soc"] = battery2.soc;
  doc["battery2_temp"] = battery2.temp;
  doc["battery2_unknownConstant0"] = battery2.unknownConstant0;
  doc["battery2_cycle_count"] = battery2.cycCount;
  doc["battery2_reservedFlags"] = battery2.reservedFlags;
  doc["battery2_bal_status"] = battery2.bStatus;
  doc["battery2_unknownConstant1"] = battery2.unknownConstant1;
  doc["battery2_chargeMos"] = battery2.chargeMos ? "true":"false";
  doc["battery2_dischargeMos"] = battery2.dischargeMos ? "true":"false";
  doc["battery2_balanceActive"] = battery2.balanceActive ? "true":"false";
  doc["battery2_chargeFETstatus"] = battery2.chargeFETstatus ? "true":"false";
  doc["battery2_dischargeFETstatus"] = battery2.dischargeFETstatus ? "true":"false";
  doc["battery2_protectionActive"] = battery2.protectionActive ? "true":"false";
  doc["battery2_bluetooth_connected"] = battery2.bluetooth_connected ? "true":"false";
  doc["battery2_systemOn"] = battery2.systemOn ? "true":"false";
  doc["battery2_cellCount"] = battery2.cellCount;
  doc["battery2_remainCap"] = battery2.rCap;
  doc["battery2_maxCap"] = battery2.nCap;
  doc["battery2_connected"] = battery2.connected ? "true":"false";
  doc["battery2_age"] = battery2.lastUpdate ? String((millis() - battery2.lastUpdate)/1000) : "-1";
  doc["battery2_error"] = battery2Error;
  doc["battery2_tempSenCount"] = battery2.tempSenCount;
  doc["battery2_reserved0"] = battery2.reserved0;
  doc["battery2_ahMaxDup"] = battery2.ahMaxDup;
  doc["battery2_ahRemDup"] = battery2.ahRemDup;
  doc["battery2_reserved1"] = battery2.reserved1;
  doc["battery2_checksum"] = battery2.checksum;
  doc["battery2_endByte"] = battery2.endByte;
  doc["battery2_cell1"] = String(battery2.cell[0],3);
  doc["battery2_cell2"] = String(battery2.cell[1],3);
  doc["battery2_cell3"] = String(battery2.cell[2],3);
  doc["battery2_cell4"] = String(battery2.cell[3],3);
  doc["battery2_cellDiff"] = String(battery2.cellDiff,3);

  doc["total_voltage"] = String(totalVoltage,2);
  doc["total_current"] = String(totalCurrent,2);
  doc["total_watts"] = String(totalWatts,2);
  doc["total_soc"] = totalSOC;
  doc["total_temp"] = totalTemp;
  doc["total_remaining_ah"] = totalRemainingAh;
  doc["total_capacity_ah"] = totalCapacityAh;
  doc["battery1_cell_delta"] = String(cellDelta1,3);
  doc["battery2_cell_delta"] = String(cellDelta2,3);

  doc["log_interval"] = logInterval;
  doc["logging"] = loggingEnabled ? "true" : "false";
  doc["log_size"] = currentLogSize;

  doc["wifi_connected"] = (WiFi.status() == WL_CONNECTED) ? "true" : "false";
  doc["ssid"] = WiFi.SSID();
  doc["ip"] = WiFi.localIP().toString();
  doc["ap_ip"] = WiFi.softAPIP().toString();
  doc["sta_ip"] =WiFi.localIP().toString();
  doc["public_ip"] = publicIP;
  doc["gateway"] = WiFi.gatewayIP().toString();
  doc["rssi"] = String(WiFi.RSSI());
  doc["hostname"] = "batterymonitor.local";

  doc["sunrise"] = String(sun.sunrise);
  doc["sunrise_az"] = String(sun.sunrise_az);
  doc["sunset"] = String(sun.sunset);
  doc["sunset_az"] = String(sun.sunset_az);
  doc["daylight"] = String(sun.daylight);
  doc["daylight_diff"] = String(sun.daylight_diff);
  doc["solar_noon_to_12_noon"] = String(sun.solarNoon_to_12_noon);
  doc["solar_noon"] = String(sun.solarNoon);
  doc["solar_noon_altitude"] = String(sun.solarNoon_altitude);
  doc["sun_distance"] = String(sun.distance);
  doc["sun_altitude"] = sun.currentAltitude;
  doc["sun_azimuth"] = sun.currentAzimuth;
  doc["panel_incidence"] = sun.panelIncidence;
  doc["panel_azimuth"] = panelAzimuth;
  doc["panel_tilt"] = panelTilt;

  doc["city"] = location.city;
  doc["province"] = location.province;
  doc["country"] = location.country;
  doc["latitude"] = location.latitude;
  doc["longitude"] = location.longitude;
  doc["geoname_id"] = location.geonameID;

  String output;
  serializeJson(doc, output);
	request->send(200, "application/json", output);
}

void handleLedON(AsyncWebServerRequest *request){
	// Serial.println("LED ON REQUEST");
	LEDStatus = true;
	digitalWrite(blueLedPin, HIGH);

	request->send(200, "OK", "text/plain");
	Serial.println("LED ON");
}

void handleLedOFF(AsyncWebServerRequest *request){
	// Serial.println("LED OFF REQUEST");
	LEDStatus = false;
	digitalWrite(blueLedPin, LOW);

	request->send(200, "OK", "text/plain");
	Serial.println("LED OFF");
}

void handleSettings(AsyncWebServerRequest *request) {
	Serial.println("SETTINGS REQUEST");
	request->send(LittleFS, "/settings.html", "text/html");
}

void handleNearbyCities(AsyncWebServerRequest *request){
  StaticJsonDocument<4096> doc;
  JsonArray array = doc.to<JsonArray>();
  for (const auto &city : nearbyCities){
    JsonObject obj = array.createNestedObject();

    obj["id"] = city.id;
    obj["name"] = city.name;
    obj["province"] = city.province;
    obj["country"] = city.country;
    obj["lat"] = city.latitude;
    obj["lon"] = city.longitude;
  }

  String json;
  serializeJson(doc, json);

  request->send(200, "application/json", json);
}

void handleDetectLocation(AsyncWebServerRequest *request){
  Serial.println("Disabled Bluetooth");
  NimBLEDevice::deinit(true);
  delay(300);

  if(!getLocationFromIP()){
    request->send(500,"text/plain","Unable to determine location");
    Serial.println("Re-enabling Bluetooth");
    NimBLEDevice::init("");
    NimBLEDevice::setPower(ESP_PWR_LVL_P9);  // Optional: reduce power issues
    return;
  }
  if(!lookupNearestCities()){
    request->send(500,"text/plain","No nearby cities");
    Serial.println("Re-enabling Bluetooth");
    NimBLEDevice::init("");
    NimBLEDevice::setPower(ESP_PWR_LVL_P9);  // Optional: reduce power issues
    return;
  }

  Serial.println("Re-enabling Bluetooth");
  NimBLEDevice::init("");
  NimBLEDevice::setPower(ESP_PWR_LVL_P9);  // Optional: reduce power issues

  request->send(200,"text/plain","OK");
}

void handleSetCity(AsyncWebServerRequest *request){
  if(!request->hasParam("id")){
    request->send(400,"text/plain","Missing ID");
    return;
  }

  uint32_t id = request->getParam("id")->value().toInt();
  // Find the selected city in nearbyCities
  for(const auto &city : nearbyCities){
    if(city.id == id){
      location.geonameID = city.id;
      location.city = city.name;
      location.province = city.province;
      location.country = city.country;
      location.latitude = city.latitude;
      location.longitude = city.longitude;
      break;
    }
  }
  // Save everything to Preferences
  prefs.putUInt("geonameID", location.geonameID);
  prefs.putString("city", location.city);
  prefs.putString("province", location.province);
  prefs.putString("country", location.country);
  prefs.putFloat("latitude", location.latitude);
  prefs.putFloat("longitude", location.longitude);

  // Now download the month's sun data
  String payload = downloadMonthlyData();
  if(payload.length()){
    saveSunData(payload);
    loadJson();
    updateTodaySunInfo();
  }

  request->send(200,"text/plain","OK");
}