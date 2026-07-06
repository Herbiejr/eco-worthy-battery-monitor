int hhmmssToSeconds(const String& t){
    int h,m,s;
    sscanf(t.c_str(), "%d:%d:%d", &h, &m, &s );
    return h*3600 + m*60 + s;
}

String getCurrentDate(){
    return getCurrentISO8601().substring(0, 10);
}

bool saveLastUpdate(){
    File f = LittleFS.open("/LastUpdate.txt", FILE_WRITE);
    if(!f)
        return false;
    f.print(getCurrentMonth());   // YYYY-MM
    f.close();
    return true;
}

bool cacheIsCurrent(){
    if(!LittleFS.exists("/SunData.json"))
        return false;

    if(!LittleFS.exists("/LastUpdate.txt"))
        return false;

    String currentMonth = getCurrentMonth();
    if(currentMonth.length() != 7){
        Serial.println("NTP unavailable");
        return false;
    }

	
    String storedMonth = getLastUpdateMonth();
    Serial.printf("Current Month: %s\n", currentMonth.c_str());
    Serial.printf("Stored Month : %s\n", storedMonth.c_str());
    return currentMonth == storedMonth;
}

String getCurrentISO8601(){
    struct tm timeinfo;
    if (!getLocalTime(&timeinfo))
        return "";

    char buffer[32];
    strftime(buffer, sizeof(buffer), "%Y-%m-%dT%H:%M:%S", &timeinfo);
    return String(buffer);
}

String getCurrentMonth(){
    return getCurrentISO8601().substring(0, 7);
}

String getLastUpdateMonth(){
    if(!LittleFS.exists("/LastUpdate.txt"))
        return "";

    File f = LittleFS.open("/LastUpdate.txt", FILE_READ);

    if(!f)
        return "";

    String month = f.readString();
    month.trim();
    f.close();
    return month;
}

bool loadJson(){
    if (!LittleFS.exists(JSON_FILE))
        return false;

    File f = LittleFS.open(JSON_FILE, FILE_READ);

    if (!f)
        return false;

    sunDoc.clear();

    DeserializationError err = deserializeJson(sunDoc, f);
    f.close();

    if (err){
        Serial.print("JSON Load Error: ");
        Serial.println(err.c_str());
        return false;
    }

    return true;
}
