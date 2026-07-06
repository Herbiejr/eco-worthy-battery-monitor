struct SunInfo {
	String sunrise;
	String sunrise_az;
	String sunset;
	String sunset_az;
	String daylight;
	String daylight_diff;
	String solarNoon_to_12_noon;
	String solarNoon;
	float solarNoon_altitude = 0;
	float distance = 0;

	// calculated
	float currentAltitude = 0;
	float currentAzimuth = 0;
	float panelIncidence = 0;
};

SunInfo sun;

String downloadMonthlyData(){
	String timestamp = getCurrentISO8601();

	// e.g. https://geotimedate.org/api/sun_data/?geoname_id=6078636&dt=2026-06-23T07:00:00
	// getCountry - https://geotimedate.org/api/get-country/
	// getState - https://geotimedate.org/api/get-state/?code=CA
	// getCities - https://geotimedate.org/api/get-substate/?code=CA.01
	// getCounty - https://geotimedate.org/api/get-county/?code=CA.01
	// getCitiesUS - https://geotimedate.org/api/get-subcounty/?code=CA.01.6078636

	// This worked but returned russia results - https://geotimedate.org/api/search/latlng/53.8/113.65
	// This worked but returned an area of results -
	// (Morinville, St. Albert, Edmonton, Fort Saskatchewan, Spruce Grove, Wild Rose, Silver Berry, Sherwood Park, Stony Plain, Beaumont) - 
	// https://geotimedate.org/api/search/latlng/53.8/-113.65		//53.632/-113.6363	//53.8028/-113.6439	//53.801118/-113.642480	//53.80014/-113.65203
	// https://geotimedate.org/api/get-state/?code=CA

	String url =
			"https://geotimedate.org/api/sun_data/"
			"?geoname_id=" +
			String(location.geonameID) +
			"&dt=" +
			timestamp;

	Serial.print("Downloading: ");
	Serial.println(url);

	WiFiClientSecure client;
	client.setInsecure();

	HTTPClient http;
  if (!http.begin(client, url)) {
    Serial.println("http.begin() FAILED");
    return "";
  }

	http.addHeader("Authorization", API_TOKEN);
	http.addHeader("Accept", "application/json");

	int code = http.GET();
	if (code != HTTP_CODE_OK){
    Serial.printf("Download Monthly Data HTTP Error %d\n", code);
		http.end();
		return "";
	}
  
	String payload = http.getString();
	Serial.printf("Payload size: %u\n", payload.length());
  Serial.println(payload);
  http.end();

  // DynamicJsonDocument doc(20000);
  // DeserializationError err = deserializeJson(doc, payload);

  // if (err) {
  //   Serial.println(err.c_str());
  //   return "0";
  // }

  // serializeJsonPretty(doc, Serial);
  // Serial.println();

	return payload;
}

JsonObject findTodayRecord(){
    String today = getCurrentDate();
    JsonArray data = sunDoc["sun_data"];

    for (JsonObject day : data){
			String date = day["date"].as<String>();

			if (date.startsWith(today)){
				return day;
			}
    }
    return JsonObject();
}

bool saveSunData(String payload){
    File f = LittleFS.open("/SunData.json", FILE_WRITE);

    if(!f)
        return false;

    f.print(payload);
    f.close();
    return true;
}

void updateTodaySunInfo(){
	JsonObject day = findTodayRecord();
	if(day.isNull())
		return;
	sun.sunrise = day["sunrise"]["time"].as<String>();
	sun.sunrise_az = day["sunrise"]["azimuth"].as<String>();
	sun.sunset = day["sunset"]["time"].as<String>();
	sun.sunset_az = day["sunset"]["azimuth"].as<String>();
	sun.daylight = day["daylight_duration"]["length"].as<String>();
	sun.daylight_diff = day["daylight_duration"]["diff"].as<String>();
  String iso = day["solar_noon"].as<String>();
  int t = iso.indexOf('T');
  int dot = iso.indexOf('.');
  String timeOnly = iso.substring(t + 1, dot);
	sun.solarNoon_to_12_noon = timeOnly;
	sun.solarNoon = day["solar_noon"]["time"].as<String>();
	sun.solarNoon_altitude = day["solar_noon"]["altitude"].as<float>();
	sun.distance = day["distance_to_the_sun"].as<float>();

	sunDataLoaded = true;
}

void updateSunPosition(){
	struct tm timeinfo;
	if(!getLocalTime(&timeinfo))
		return;

	int now = timeinfo.tm_hour * 3600 + timeinfo.tm_min  * 60 + timeinfo.tm_sec;
	String sunriseTime = sun.sunrise.substring(11,19);
	String sunsetTime = sun.sunset.substring(11,19);
	String solarNoonTime = sun.solarNoon.substring(11,19);
	int sunriseSec = hhmmssToSeconds(sunriseTime);
	int sunsetSec = hhmmssToSeconds(sunsetTime);
	int noonSec = hhmmssToSeconds(solarNoonTime);

	if(now < sunriseSec || now > sunsetSec){
		sun.currentAltitude = 0;
    sun.currentAzimuth = 0;
		return;
	}

	// float daylightFraction = (float)(now - sunriseSec) / (float)(sunsetSec - sunriseSec);
	// // altitude curve
	// sun.currentAltitude = (sin(daylightFraction * PI) * sun.solarNoon_altitude);	// - 10;

  // === Altitude (already decent) ===
	float halfDay = (sunsetSec - sunriseSec) / 2.0f;
	float x = (now - noonSec) / halfDay;
	sun.currentAltitude = sun.solarNoon_altitude * cos(x * PI / 2.0f);

	// === Improved Azimuth Calculation ===
	float riseAz = sun.sunrise_az.toFloat();
	float setAz = sun.sunset_az.toFloat();
	// sun.currentAzimuth = (riseAz + daylightFraction * (setAz - riseAz));	// - 16.5;

  // Linear interpolation with cosine smoothing (your current method is okay)
	float daylightProgress = (float)(now - sunriseSec) / (float)(sunsetSec - sunriseSec);

  // Better smoothing using cosine
	sun.currentAzimuth = riseAz + (setAz - riseAz) * (0.5f - 0.5f * cos(daylightProgress * PI));
}

void calculatePanelIncidence(){
	// How directly the sun is striking the face of the solar panel
	// Perfect Incidence = 0°  Production = Maximum
	// Incidence = 45°  Production ≈ 70%
	// Incidence = 90°  Production ≈ 0%

	float azDiff = fabs(sun.currentAzimuth - panelAzimuth);

	if(azDiff > 180)
		azDiff = 360 - azDiff;

	float tiltDiff = fabs(panelTilt - sun.currentAltitude);

	// sun.panelIncidence = sqrt(azDiff * azDiff + tiltDiff * tiltDiff);

	float azRad = radians(azDiff);
	float tiltRad = radians(tiltDiff);
	sun.panelIncidence = degrees(acos(cos(tiltRad) * cos(azRad)));
}

