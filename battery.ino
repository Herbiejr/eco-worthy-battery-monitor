//0xDD, 0xA5, 0x03, 0x00, 0xFF, 0xFD, 0x77
//221, 165, 3, 255, 253, 119
const uint8_t statusQuery[] ={
	0xDD,	//FRAME_START 0xDD
	0xA5,	// FRAME_READ 0xA5
	0x03,	// CMD_READ_BASIC_INFO 0x03
	0x00,
	0xFF,
	0xFD,
	0x77	// FRAME_END 0x77
};

const uint8_t batCellsQuery[] ={
	0xDD,	//FRAME_START 0xDD
	0xA5,	// FRAME_READ 0xA5
	0x04,	// CMD_READ_CELL_VOLTAGES 0x04
	0x00,
	0xFF,
	0xFC,
	0x77	// FRAME_END 0x77
};

// 
// const uint8_t batHardwareQuery[] =
// {
// 	0xDD,	//FRAME_START 0xDD
// 	0xA5,	// FRAME_READ 0xA5
// 	0x05,	// 05=BMS version
// 	0x00,
// 	0xFF,
// 	0xFC,
// 	0x77	// FRAME_END 0x77
// };

// MOS Control Command 
// const uint8_t MOSControlQuery[] =
// {
// 	0xDD,	//FRAME_START 0xDD
// 	0xA5,	// FRAME_READ 0xA5
// 	0xE1,	// 05=BMS version
// 	0x02,
// 	0x00,	//XX,	// XX means the state of MOS control
// 	0x??
// 	0x77	// FRAME_END 0x77
// };

// Protocol Frame Structure
// #define FRAME_START 0xDD
// #define FRAME_READ 0xA5
// #define FRAME_WRITE 0x5A
// #define FRAME_END 0x77

void battery1NotifyCallback(NimBLERemoteCharacteristic* pRC, uint8_t* pData, size_t length, bool isNotify){
	battery1Error = "";
	battery1Buffer.insert(battery1Buffer.end(), pData, pData + length);

	if (length > 0 && pData[length - 1] == 0x77){
			// Serial.printf("Bat 1 Complete packet length=%u\n", battery1Buffer.size());
			// Serial.printf("Battery 1 Buffer = %u\n", battery1Buffer);
			parsePacket(battery1Buffer.data(), battery1Buffer.size(), battery1);
			battery1Buffer.clear();
	}else{
		return;
	}
}

void battery2NotifyCallback(NimBLERemoteCharacteristic* pRC, uint8_t* pData, size_t length, bool isNotify){
	battery2Error = "";
	battery2Buffer.insert(battery2Buffer.end(), pData, pData + length);

	if (length > 0 && pData[length - 1] == 0x77){
			// Serial.printf("Bat 2 Complete packet length=%u\n", battery2Buffer.size());
			// Serial.printf("Battery 2 Buffer = %u\n", battery2Buffer);
			parsePacket(battery2Buffer.data(), battery2Buffer.size(), battery2);
			battery2Buffer.clear();
	}else{
		return;
	}
}

bool connectBattery1(){
	if (battery1Client) {
		battery1Client->disconnect();
		NimBLEDevice::deleteClient(battery1Client);
		battery1Client = nullptr;
	}

	battery1Client = NimBLEDevice::createClient();
	if (!battery1Client) {
			Serial.println("Failed to create BLE client!");
			return false;
	}

	if (!battery1Client->connect(NimBLEAddress(BATTERY1_MAC, BLE_ADDR_PUBLIC))){
		battery1Error = "Connect Failed";
		Serial.println("Battery1 connect failed");
		// battery1Client->disconnect();
		NimBLEDevice::deleteClient(battery1Client);
		battery1Client = nullptr;

		return false;
	}

	NimBLERemoteService* service = battery1Client->getService(serviceUUID);
	if (!service){
		battery1Error = "Service Not Found";
		Serial.println("Battery1 service not found");
		battery1Client->disconnect();
		NimBLEDevice::deleteClient(battery1Client);
		battery1Client = nullptr;

		return false;
	}

	battery1WriteChar = service->getCharacteristic(charUUID);	// FF02
	battery1NotifyChar = service->getCharacteristic("ff01");	// FF01
	// battery1ReadChar = service->getCharacteristic(charUUIDread);	// FF01
	if (!battery1WriteChar){
		battery1Error = "Characteristic Not Found";
		Serial.println("Battery1 characteristic not found");
		battery1Client->disconnect();
		NimBLEDevice::deleteClient(battery1Client);
		battery1Client = nullptr;

		return false;
	}

	if (!battery1NotifyChar->subscribe(true, battery1NotifyCallback)){
		battery1Error = "Subscribe Failed";
		Serial.println("Battery1 subscribe failed");
		battery1Client->disconnect();
		NimBLEDevice::deleteClient(battery1Client);
		battery1Client = nullptr;

		return false;
	}

	Serial.println("Battery 1 Connected");
	battery1Error = "";
	
	return true;
}

void queryBattery1(){
	if (!battery1Client || !battery1Client->isConnected()){
		battery1.connected = false;
		return;
	}

	if (!battery1NotifyChar)
		return;

	bool success = battery1WriteChar->writeValue(statusQuery, sizeof(statusQuery), false);
	if (!success){
		battery1Error = "Query Failed";
		Serial.println("Battery1 query failed");
	}
}

void queryBattery1Cells(){
	if (!battery1Client || !battery1Client->isConnected()){
		battery1.connected = false;
		return;
	}

	if (!battery1NotifyChar)
		return;

	bool success = battery1WriteChar->writeValue(batCellsQuery, sizeof(batCellsQuery), false);
	if (!success){
		battery1Error = "Query Failed";
		Serial.println("Battery1 query failed");
	}
}

bool connectBattery2(){
	if (battery2Client) {
		battery2Client->disconnect();
		NimBLEDevice::deleteClient(battery2Client);
		battery2Client = nullptr;
	}

	battery2Client = NimBLEDevice::createClient();

	if (!battery2Client) {
			Serial.println("Failed to create BLE client!");
			return false;
	}

	if (!battery2Client->connect(NimBLEAddress(BATTERY2_MAC, BLE_ADDR_PUBLIC))){
		battery2Error = "Connect Failed";
		Serial.println("Battery2 connect failed");
		NimBLEDevice::deleteClient(battery2Client);
		battery2Client = nullptr;

		return false;
	}

	NimBLERemoteService* service = battery2Client->getService(serviceUUID);
	if (!service){
		battery2Error = "Service Not Found";
		Serial.println("Battery2 service not found");
		battery2Client->disconnect();
		NimBLEDevice::deleteClient(battery2Client);
		battery2Client = nullptr;

		return false;
	}

	battery2WriteChar = service->getCharacteristic(charUUID);
	battery2NotifyChar = service->getCharacteristic("ff01");
	if (!battery2WriteChar){
		battery2Error = "Characteristic Not Found";
		Serial.println("Battery2 characteristic not found");
		battery2Client->disconnect();
		NimBLEDevice::deleteClient(battery2Client);
		battery2Client = nullptr;

		return false;
	}	

	if (!battery2NotifyChar->subscribe(true, battery2NotifyCallback)){
		battery2Error = "Subscribe Failed";
		Serial.println("Battery2 subscribe failed");
		battery2Client->disconnect();
		NimBLEDevice::deleteClient(battery2Client);
		battery2Client = nullptr;

		return false;
	}
	Serial.println("Battery 2 Connected");
	battery2Error = "";
	
	return true;
}

void queryBattery2(){
	if (!battery2Client || !battery2Client->isConnected()){
		battery2.connected = false;
		return;
	}

	if (!battery2NotifyChar)
		return;

	bool success = battery2WriteChar->writeValue(statusQuery, sizeof(statusQuery), false);
	if (!success){
		battery2Error = "Query Failed";
		Serial.println("Battery2 query failed");
	}
}

void queryBattery2Cells(){
	if (!battery2Client || !battery2Client->isConnected()){
		battery2.connected = false;
		return;
	}

	if (!battery2NotifyChar)
		return;

	bool success = battery2WriteChar->writeValue(batCellsQuery, sizeof(batCellsQuery), false);
	if (!success){
		battery2Error = "Query Failed";
		Serial.println("Battery2 query failed");
	}
}

float getCellDelta(const BatteryData& bat){
	float minCell = bat.cell[0];
	float maxCell = bat.cell[0];

	for(int i = 1; i < 4; i++){
		if(bat.cell[i] < minCell) minCell = bat.cell[i];
		if(bat.cell[i] > maxCell) maxCell = bat.cell[i];
	}

	return maxCell - minCell;
}

void handleCells(AsyncWebServerRequest *request) {
	// Serial.println("CELLS VISIBLE REQUEST");
	cellsVisible = (request->arg("visible") == "1");
	request->send(200, "text/plain", "OK");
}

void handleCells1(AsyncWebServerRequest *request) {
	// Serial.println("BAT 1 CELLS VISIBLE REQUEST");
	cells1visible =
		request->arg("visible") == "1";
	request->send(200, "text/plain", "OK");
}

void handleCells2(AsyncWebServerRequest *request) {
	// Serial.println("BAT 2 CELLS VISIBLE REQUEST");
	cells2visible =
		request->arg("visible") == "1";
	request->send(200, "text/plain", "OK");
}

void parsePacket(uint8_t* pData, size_t length, BatteryData& bat){
  if (length == 0)
    return;

  if (pData == nullptr)
    return;

	if (pData[0] != 0xDD){
		Serial.printf("First Packet NOT: 0xDD from %s\n", bat);
		// Serial.println();
		return;
	}
  // if (pData[length - 1] != 0x77){
	// 	Serial.printf("Last Packet NOT: 0x77 from %s\n", bat);
	// 	// Serial.println();
	// 	return;
  // }

	//======================================================
	// Serial.printf("%s - FULL PACKET (%u bytes)\n", bat, length);
  const char* batteryName = (&bat == &battery1) ? "Battery 1" : "Battery 2";

  Serial.printf("%s Packet: ", batteryName);

	for(size_t i=0; i<length; i++)
			Serial.printf("%02X ", pData[i]);

	Serial.println();
	//======================================================

	if (pData[1] == 0x03){	// General Packet data
		// Serial.printf("Second Packet 0x03");
		// DD 03 00 22 05 9F 00 00 27 08 27 10 00 00 33 12 00 00 00 00 00 00 66 64 03 04 01 0B 39 00 00 00 27 10 27 08 00 00 FD 13 77 

		// for(size_t i=0; i<length; i++)
		// 		Serial.printf("%02X ", pData[i]);
		// Serial.println();

		//uint16_t startByte = pData[0];	// << 8) | pData[0];	// Start byte
		//uint16_t command = pData[1];	// << 8) | pData[1];	// basic info response
		//uint16_t payloadLength = (pData[2] << 8) | pData[3];	// Payload length

		uint16_t rawVoltage = (pData[4] << 8) | pData[5];	// Battery voltage
		int16_t rawCurrent = (pData[6] << 8) | pData[7];	// Battery current
		uint16_t ahRem = (pData[8] << 8) | pData[9];	// Remaining capacity
		uint16_t ahMax = (pData[10] << 8) | pData[11];	// Full capacity
		uint16_t cycleCount = (pData[12] << 8) | pData[13];	// Cycle Count

		uint16_t unknownConstant0 = (pData[14] << 8) | pData[15];	// Product Date - Two bytes are used for transmission such as 0x2068, where the date is the lowest 5: 0x2028 & 0x1f = 8 for date; the month (0x2068 > > > 5) & 0x0f = 0x03 for March; the year 2000 + 0x2068 > 9 = 2000 + 0x10 = 2016; 
		uint16_t reservedFlags = (pData[16] << 8) | pData[21];	// Reserved Flags or Balance Status - Each bit represents each cell block’s balance, 0 is off, 1 is on ; 1~16pcs in series. 
		uint16_t unknownConstant1 = (pData[22] << 8) | pData[23];	// Unknown constant / Balance Status_High- Each bit represents each cell block’s balance, 0 is off, 1 is on ; 17~32pcs in series, 32pcs at the most. Increased base on V0 

		uint16_t switches = pData[24];	// << 8) | pData[24];	// Protection Status - 8 Switches - ：Each bit represents a protective state, 0 is unprotected, and 1 is protected
		uint16_t cellCount = pData[25];	// << 8) | pData[25];	// Cell Count
		uint16_t tempSenCount = pData[26];	//	<< 8) | pData[26];	// Temp Sensor count
		uint16_t rawTemp = (pData[27] << 8) | pData[28];	// Temperature

		uint16_t reserved0 = (pData[29] << 8) | pData[31];	// Reserved

		uint16_t ahMaxDup = (pData[32] << 8) | pData[33];	// Nominal capacity (duplicate)
		uint16_t ahRemDup = (pData[34] << 8) | pData[35];	// Remaining capacity (duplicate)

		uint16_t reserved1 = (pData[36] << 8) | pData[37];	// Reserved

		//uint16_t checksum = (pData[38] << 8) | pData[39];	// Checksum
		//uint16_t endByte = pData[40];	// << 8) | pData[40];	// End byte

		bat.voltage = rawVoltage / 100.0f;
		bat.current = rawCurrent / 100.0f;
		bat.watts = (bat.voltage * bat.current);
		
		if(ahMax > 0)
			bat.soc = ((float)ahRem / ahMax) * 100.0f;

		bat.rCap = ahRem / 100.0f;
		bat.nCap = ahMax / 100.0f;
		bat.unknownConstant0 = unknownConstant0;
		bat.cycCount = cycleCount;
		bat.reservedFlags = reservedFlags;
		bat.unknownConstant1 = unknownConstant1;

		bat.chargeMos = (switches & 0x01) != 0; // Charge MOS - Charge MOSFET enabled (ON = can charge)
		bat.dischargeMos = (switches & 0x02) != 0;  // Discharge MOS - Discharge MOSFET enabled (ON = can discharge)
		bat.balanceActive = (switches & 0x03) != 0; // BalancingCell balancing is currently active
		bat.chargeFETstatus = (switches & 0x04) != 0;  // Charge FET Status - Actual hardware feedback of Charge FET (sometimes redundant with bit 0)
		bat.dischargeFETstatus = (switches & 0x05) != 0;  // Discharge FET Status - Actual hardware feedback of Discharge FET
		bat.protectionActive = (switches & 0x06) != 0;  // Reserved / Alarm - Often used for "Short Circuit Protection" or "Temperature Protection Active"
		bat.bluetooth_connected = (switches & 0x07) != 0;  // Reserved - Usually unused or "Bluetooth Connected" flag on some firmware versions
		bat.systemOn = (switches & 0x08) == 0;  // System Power / Sleep - BMS overall power state. Often inverted (0 = normal, 1 = sleep/pre-sleep)

		bat.cellCount = cellCount;
		bat.tempSenCount = tempSenCount;
		bat.temp = (rawTemp - 2731) * 0.1f;	// (2848 - 2731) * 0.1 = 11.7
		bat.reserved0 = reserved0;
		bat.ahMaxDup = ahMaxDup / 100.00f;
		bat.ahRemDup = ahRemDup / 100.00f;

		bat.reserved1 = reserved1;

		bat.checksum = checksum;
		bat.endByte = endByte;

		bat.lastUpdate = millis();
		bat.connected = true;
		return;

	}else if (pData[1] == 0x04){	// Cells Packet data
		// DD 04 00 08 0D CB 0D CC 0D CE 0D CE FC 91 77 
		// DD 04 00 08 0D CB 0D CC 0D CE 0D CE FC 91 77 
		// DD 04 00 08 0D CC 0D CE 0D CF 0D D0 FC 8B 77 
		// DD 04 00 08 0D CD 0D CF 0D D1 0D D1 FC 86 77 

		// Serial.printf("Second Packet 0x04");
		// Serial.printf("Cells packet %u bytes\n", length);
		// for(size_t i=0; i<length; i++)
		// 		Serial.printf("%02X ", pData[i]);
		// Serial.println();

		//uint16_t c_startByte = pData[0];	// << 8) | pData[0];	// Start byte
		//uint16_t c_command = pData[1];	// << 8) | pData[1];	// basic info response
		//uint16_t c_payloadLength = (pData[2] << 8) | pData[3];	// Payload length

		uint16_t c_rawVoltage1 = (pData[4] << 8) | pData[5];	// Cell 1 voltage
		uint16_t c_rawVoltage2 = (pData[6] << 8) | pData[7];	// Cell 2 voltage
		uint16_t c_rawVoltage3 = (pData[8] << 8) | pData[9];	// Cell 3 voltage
		uint16_t c_rawVoltage4 = (pData[10] << 8) | pData[11];	// Cell 4 voltage

		//uint16_t c_checksum = (pData[12] << 8) | pData[13];	// Checksum
		//uint16_t c_endByte = pData[14];	// << 8) | pData[40];	// End byte

		bat.cell[0] = c_rawVoltage1 / 1000.0f;
		bat.cell[1] = c_rawVoltage2 / 1000.0f;
		bat.cell[2] = c_rawVoltage3 / 1000.0f;
		bat.cell[3] = c_rawVoltage4 / 1000.0f;
		
		// Voltage Differential = Highest Cell Voltage - Lowest Cell Voltage
		float maxV = max(max(c_rawVoltage1, c_rawVoltage2), max(c_rawVoltage3, c_rawVoltage4));
		float minV = min(min(c_rawVoltage1, c_rawVoltage2), min(c_rawVoltage3, c_rawVoltage4));
		float differential = maxV - minV;
		bat.cellDiff = differential / 1000.000f;

		return;
	}else{	// if (pData[1] == 0x05){	// hardware version no.

		Serial.printf("Second Packet %s", pData[1]);
		Serial.printf("Cells packet %u bytes\n", length);
		for(size_t i=0; i<length; i++)
				Serial.printf("%02X ", pData[i]);
		Serial.println();
	}
}

