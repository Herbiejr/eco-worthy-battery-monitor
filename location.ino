struct CityInfo {
    uint32_t id;
    String name;
    String province;
    String country;
    float latitude;
    float longitude;
};

std::vector<CityInfo> nearbyCities;

bool getLocationFromIP(){
  Serial.println("Get Location from IP Started...");
  // WiFiClientSecure client;
  WiFiClient client;
  // client.setInsecure();

  HTTPClient http;
  http.begin(client, "http://ip-api.com/json/");

  int code = http.GET();
  if(code != HTTP_CODE_OK){
    Serial.println("This failed in the getLocationFromIP");
    http.end();
    return false;
  }

  DynamicJsonDocument doc(4096);
  deserializeJson(doc, http.getStream());

  // publicIP = doc["query"];
  location.city = doc["city"].as<String>();
  location.province = doc["region"].as<String>();
  location.country = doc["country"].as<String>();

  location.latitude = doc["lat"];
  location.longitude = doc["lon"];

  http.end();
  Serial.println("Get Location from IP Complete!");
  return true;
}

bool lookupNearestCities() {
  if (location.latitude == 0 || location.longitude == 0)
    return false;

// https://geotimedate.org/api/search/latlng/53.8/-113.65
// https://geotimedate.org/api/search/latlng/53.632000/-113.636299/
// fetch("https://geotimedate.org/api/search/latlng/53.632000/-113.636299/", {
//     headers: {
//         "Authorization": "Token 9b09b8c1f47f22498206143bc1ff33797e9d1c50"
//     }
// })
// .then(r => r.text())
// .then(console.log)
// .catch(console.error);

  String url =
    "https://geotimedate.org/api/search/latlng/" +
    String(location.latitude, 6) + "/" +
    String(location.longitude, 6) + "/";

  Serial.printf("Downloading Nearest Cities: %s\n", url.c_str());

  WiFiClientSecure client;
  client.setInsecure();

  HTTPClient http;
  if (!http.begin(client, url))
    return false;

  http.addHeader("Authorization", API_TOKEN);
  http.addHeader("Accept", "application/json");

  int code = http.GET();
  if (code != HTTP_CODE_OK) {
    Serial.printf("Nearest Cities HTTP Error %d\n", code);
    http.end();
    return false;
  }

  DynamicJsonDocument doc(12000);
  DeserializationError err = deserializeJson(doc, http.getStream());
  http.end();

  if (err) {
    Serial.println(err.c_str());
    return false;
  }

  nearbyCities.clear();

  // serializeJsonPretty(doc, Serial);
  // Serial.println();

  JsonArray array = doc["locations_list"].as<JsonArray>();
  for (JsonObject city : array) {
    CityInfo c;

    c.id = city["id"] | 0;
    c.name = city["name"] | "";
    c.province = city["state"] | "";
    c.country = city["country_code"] | "";

    c.latitude = city["latitude"] | 0.0;
    c.longitude = city["longitude"] | 0.0;

    nearbyCities.push_back(c);

    Serial.printf("%u  %s\n",
      c.id,
      c.name.c_str());
  }

  Serial.printf("%u nearby cities loaded\n", nearbyCities.size());
  return nearbyCities.size() > 0;
}