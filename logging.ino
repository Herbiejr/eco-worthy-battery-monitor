void logBatteryData(){
	File f = LittleFS.open("/battery.csv", FILE_APPEND);
	if(!f)
		return;

	time_t now;
	time(&now);
	struct tm timeinfo;
	localtime_r(&now, &timeinfo);
	char timestamp[32];
	strftime(timestamp, sizeof(timestamp), "%Y-%m-%d %H:%M:%S", &timeinfo);

	// "%s,%.2f,%.2f,...\n",
	// timestamp,

	f.printf(
		// "%lu,%.2f,%.2f,%.1f,%.1f,%.2f,%.2f,%.1f,%.1f\n",
		// millis()/1000,
		"%s,%.2f,%.2f,%.2f,%.1f,%.1f,%.1f,%.2f,%.2f,%.2f,%.1f,%.1f,%.1f\n",
		timestamp,
		battery1.voltage,
		battery1.current,
		battery1.watts,
		battery1.soc,
		battery1.temp,
		battery1.rCap,
		
		battery2.voltage,
		battery2.current,
		battery2.watts,
		battery2.soc,
		battery2.temp,
		battery2.rCap
	);

	currentLogSize = f.size();

	f.close();
}

