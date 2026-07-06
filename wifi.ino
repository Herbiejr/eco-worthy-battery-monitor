void onWiFiEvent(WiFiEvent_t event){
	switch(event){
		case ARDUINO_EVENT_WIFI_STA_CONNECTED:
			Serial.println("WiFi Connected");
			break;

		case ARDUINO_EVENT_WIFI_STA_GOT_IP:
			currentIP = WiFi.localIP();
			currentGateway = WiFi.gatewayIP();

			Serial.print("IP: ");
			Serial.println(currentIP);

			Serial.print("Gateway: ");
			Serial.println(currentGateway);

			wifiConnected = true;
      wifiConnectedAt = millis();

			MDNS.end();
			MDNS.begin("batterymonitor");

			configTzTime(
					"MST7MDT,M3.2.0/2,M11.1.0/2",
					"pool.ntp.org",
					"time.nist.gov");

			// Optional:
			// updateLocationFromIP();
			// downloadMonthlyData();
			sunDataNeedsUpdate = true;
			break;

		case ARDUINO_EVENT_WIFI_STA_DISCONNECTED:
			Serial.println("Disconnected");
			wifiConnected = false;
			ntpSynced = false;
			break;
	}
}

String getPublicIP(){
  WiFiClientSecure client;
  client.setInsecure();

  HTTPClient http;
  http.begin(client, "https://ipapi.co/json/");

  int code = http.GET();
  if(code != HTTP_CODE_OK){
    http.end();
    return "";
  }

  DynamicJsonDocument doc(4096);
  deserializeJson(doc, http.getStream());
  http.end();
  return doc["ip"].as<String>();
}