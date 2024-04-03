#include <WiFi.h>
#include <WiFiClient.h>
#include <WebServer.h>
#include <SPI.h>
#include <LoRa.h>

#define MISO 19
#define SCK 18
#define SS 5
#define RST 17
#define DI0 16
#define MOSI 21

/** Ping Logik
 * - Was will ich empfangen? - Rssi, SNR, FreqErr
 * - 
 * - Remote esp32 - Auf Dach - Pinged dauerhaft, in 1min abstand - antwortet auf ping mit eigenem ping
 * 
 * 
 * 
 * 
 * - Wifi esp32 - Bei mir - pingt wenn ich seite aktualisiere - antwortet nicht auf ping
 * 
 */

typedef struct {
    int rssi;
    int prssi;
    float psnr;
    long pfreqerr;
	// unsigned long send_millis;
	bool response_wanted;
} Ping;

typedef struct {
    Ping *ping_data;
	ulong curr_millis;
} Ping_Time_Data;

#define WIFI

// #define DEBUG

#ifdef DEBUG
#define ser_beg(x) Serial.begin(x)
#define ser_print(x) Serial.print(x)
#define ser_println(x) Serial.println(x)
#define ser_printf(x,y) Serial.printf(x,y)
#else
#define ser_beg(x)
#define ser_print(x)
#define ser_println(x)
#define ser_printf(x,y)
#endif

const char* own_ssid = "LoraTestShort";
const char* own_password = "00810081";

/* const char* wifi_ssid = "Universum der Liebe";
 * const char* wifi_password = "t88E6dHxtkbf9";
 */

WebServer server(80);
String sendString = "LoRaTest";
String receivedString = "";
// bool auto_ping = true;
ulong auto_ping_interval = 30e3;
// ulong last_ping = 0;
bool auto_respond = true;

Ping sendPing = {0,0,0,0,0};
Ping_Time_Data sendPingData = {&sendPing,0};
Ping recPing = {0,0,0,0,0};
Ping_Time_Data recPingData = {&recPing,0};

int spread = 12;
long bandWidth = 7.8E3;
int txPower = 20;

ulong wifi_last_use = 0;
bool wifi_on = false;

void sendStr(String str){
	//Lora Send String
	LoRa.beginPacket();
	LoRa.print(str);
	LoRa.endPacket();
}

void ping(bool resp){
	sendPing = {
		LoRa.rssi(),
		LoRa.packetRssi(),
		LoRa.packetSnr(),
		LoRa.packetFrequencyError(),
		// millis(),
		resp
	};
	// LoRa.setTxPower(20);
	// LoRa sender function
	// void sendPingOverLoRa(const Ping& ping) {
	// Serialize the Ping struct into a byte array
	byte buffer[sizeof(Ping)];
	memcpy(buffer, &sendPing, sizeof(Ping));
	ser_print("Ping snd ");
	// ser_println("");
	sendPingData.curr_millis = millis();
	// Send the serialized data over LoRa
	LoRa.beginPacket();
	LoRa.write(buffer, sizeof(Ping));
	LoRa.endPacket();

	// last_ping = millis();
}

/**
 * return :
 * 0 -> recPing new Data,
 * 1 -> Receive Failed
 * 2 -> Ping Read error
 * 3 -> String received
*/
int receive_msg(Ping* recPing){
	byte receivedData[sizeof(Ping)];
	// check if a packet has been received
	int packetSize = LoRa.parsePacket();

	if (packetSize == sizeof(Ping)) {	// Hier kann ich entscheiden, obs ein Ping, oder eine andere Datenübertragung ist.
		size_t readBytes = LoRa.readBytes(receivedData,sizeof(Ping));
		if (readBytes==sizeof(Ping)){
			recPingData.curr_millis = millis();
			ser_println("Rec Ping");
			memcpy(recPing,&receivedData,sizeof(Ping));
			return 0;
			// Received and Read Ping Successfull
		}else{
			// Read Ping Unsuccessfull
			return 1;
		}
	}else if (packetSize){
		ser_println("Rec String");
		// Received String
		receivedString = "";
		while (LoRa.available()) {
			receivedString += (char)LoRa.read();
		}
		return 3;
	} else {
		// Nothing Received
		return 2;
	}
}

void handleRoot() {
	wifi_last_use = millis();
	// ser_println("handle Root Auto");
	// ping(true);

	String rssi = String(LoRa.rssi());
	String prssi = String(LoRa.packetRssi());
	String psnr = String(LoRa.packetSnr());
	String pfreqerr = String(LoRa.packetFrequencyError());

	String ToA = String((recPingData.curr_millis - sendPingData.curr_millis)/2);
	ulong currentTime = millis();

	String html = "<!DOCTYPE html><html><head><title>ESP32 LoRa WebServer</title></head><body>";
	html += "<h1>ESP32 LoRa WebServer</h1>";

	html += "<p>Own Info - RSSI: " + rssi + " Packet RSSI: " + prssi + " Packet SNR: " + psnr + " Packet Freqency Error: " + pfreqerr + "</p>";
	html += "<p>Received Info - RSSI: " + String(recPing.rssi) + " Packet RSSI: " + String(recPing.prssi) + " Packet SNR: " + String(recPing.psnr) + " Packet Freqency Error: " + String(recPing.pfreqerr) + "</p>";
	html += "<p>Send Info - RSSI: " + String(sendPing.rssi) + " Packet RSSI: " + String(sendPing.prssi) + " Packet SNR: " + String(sendPing.psnr) + " Packet Freqency Error: " + String(sendPing.pfreqerr) + "</p>";

	html += "<p>System time: "+String(currentTime)+"ms</p>";
	html += "<p>Ping Send: "+String(currentTime - sendPingData.curr_millis)+" Ping received: "+String(currentTime - recPingData.curr_millis)+" Total: "+ToA+"ms.</p>";

	html += "<p>Last send Message: " + sendString + "<br>"+"Last received Message: " + receivedString + "</p>";

	// html += "<form method='post' action='/togglePing'><button type='submit'>Auto Ping is "+String(auto_ping)+"</button></form>";
	html += "<form method='post' action='/ping'><button type='submit'>Ping</button></form>";
	html += "<form method='post' action='/post'>Auto Ping Intervall current: "+String(auto_ping_interval)+" (in ms):<input type='number' name='numberInput' min='0' step='1000'><input type='submit' value='Submit'></form>";
	html += "<form method='post' action='/post'>Tx Power current: "+String(txPower)+" (dBm):<input type='number' name='numberInputTX' min='0' step='1' max='20'><input type='submit' value='Submit'></form>";
	html += "<form method='post' action='/post'>Spread current: "+String(spread)+" :<input type='number' name='numberInputSpread' min='0' step='1' max='12'><input type='submit' value='Submit'></form>";

	// html += "<form method='get' action='/'><input type='submit' value='Custom Msg'></form>";
	html += "<form method='post' action='/post'>Send Message (max 32 characters): <input type='text' name='message' maxlength='32'><input type='submit' value='Submit'></form>";
	html += "</body></html>";
	server.send(200, "text/html", html);
}

void handlePing(){
	ping(true);
	server.sendHeader("Location", "/");
	server.send(302, "text/plain", "");
}

// void handleTogglePing() {
// 	auto_ping = !auto_ping;  
// 	server.sendHeader("Location", "/");
// 	server.send(302, "text/plain", "");
// }

void handleToggleRespond(){
	auto_respond = !auto_respond;
	server.sendHeader("Location", "/");
  	server.send(302, "text/plain", "");
}

void loraUpdate(){
	LoRa.setTxPower(txPower);
	LoRa.setSpreadingFactor(spread);
}

void handlePost() {
	if (server.hasArg("message")) {
		String newString = server.arg("message");
		if (newString.length() <= 32) {
			sendString = newString;

			//Lora Send String
			LoRa.beginPacket();
			LoRa.print(sendString);
			LoRa.endPacket();

			String reply = "Successfully send String: " + newString + "\nRedirecting...";
			server.sendHeader("Location", "/", true);   // Redirect to the root page
			server.send(302, "text/plain", reply);
		} else {
			String reply = "String: " + newString + " is too long!\nRedirecting...";
			
			server.sendHeader("Location", "/", true);   // Redirect to the root page
			server.send(400, "text/plain", reply);
		}
	} else if (server.hasArg("numberInput")) {
		auto_ping_interval = (unsigned long) server.arg("numberInput").toDouble();

		String reply = "Successfully received Number: " + String(auto_ping_interval) + "\nRedirecting...";
		server.sendHeader("Location", "/", true);   // Redirect to the root page
		server.send(302, "text/plain", reply);
  	}	else if (server.hasArg("numberInputTX")) {
		txPower = server.arg("numberInputTX").toInt();
		loraUpdate();
		String reply = "Successfully received Number: " + String(txPower) + "\nRedirecting...";
		server.sendHeader("Location", "/", true);   // Redirect to the root page
		server.send(302, "text/plain", reply);
	}	else if (server.hasArg("numberInputSpread")) {
		spread = server.arg("numberInputSpread").toInt();
		loraUpdate();
		String reply = "Successfully received Number: " + String(spread) + "\nRedirecting...";
		server.sendHeader("Location", "/", true);   // Redirect to the root page
		server.send(302, "text/plain", reply);
	}	else {
		server.sendHeader("Location", "/", true);   // Redirect to the root page
		server.send(400, "text/plain", "No string received! Redirecting...");
	}
}

void LoraInit(){
	ser_println("Lora Init");
	SPI.begin(SCK, MISO, MOSI, SS);
	LoRa.setPins(SS, RST, DI0);
	// LoRa.rssi();
	// LoRa.packetRssi();
	ser_println("LoRa Transceiver init.");
	while (!LoRa.begin(433E6)) {
		ser_print(".");
		delay(500);
	}
	LoRa.setSpreadingFactor(spread);
	LoRa.setSignalBandwidth(bandWidth); // Set BW to Lowest
	LoRa.setTxPower(txPower);
	ser_println("LoRa Initializing OK!");
}

void wifiInit(){
	ser_println("Wifi init");
	WiFi.softAP(own_ssid, own_password);
	
	IPAddress IP = WiFi.softAPIP();
	ser_print("AP IP address: ");
	ser_println(IP);
	
	//Server
	server.on("/", HTTP_GET,handleRoot);
	server.on("/post", HTTP_POST,handlePost);
	// server.on("/togglePing", HTTP_POST,handleTogglePing);
	// server.on("/toggleRespond", HTTP_POST,handleToggleRespond);
	server.on("/ping", HTTP_POST,handlePing);
	server.begin();

	wifi_on = true;
}

void wifiStop(){
	ser_println("Closing Wifi");
	server.close();
	WiFi.disconnect(true,false);
}

void setup() {
	// Serial
	ser_beg(115200);
	//LORA
	LoraInit();
	#ifdef WIFI
		wifiInit();
	#endif
	// now Own WiFi
	//Wifi login or host own
	// WiFi.begin(wifi_ssid, wifi_password);
	// double waitForWifiStart = millis();
	// while (WiFi.status() != WL_CONNECTED && (millis() < waitForWifiStart + 10e3)) {
    //     delay(10);
	// }
	// bool wifi_connected = WiFi.status() == WL_CONNECTED;
	// if (!wifi_connected)

}

void loop() {
	#ifdef WIFI
		server.handleClient();
	#endif
	
	int pingErr = receive_msg(&recPing);
	// Successfully received Ping with response prompt
	if (pingErr == 0 && recPing.response_wanted){
		// Answere with no wanted response
		ping(false);
	}
	// 	// String Received
	// 	if (pingErr == 3) {
	// 		sendStr(receivedString);
	// 	}
	// }

	// Last send Ping is 30sec ago, and auto_ping is enabled
	if ((millis() - sendPingData.curr_millis) > auto_ping_interval) {
		ping(true);
	}

	// ser_print(".");
	// delay(100);

	// Wifi aus wenn noch nie wifi verwendet, und 10min um.
	// if (wifi_on && wifi_last_use != 0 && (millis() - wifi_last_use) > 60e3){
	// 	txPower = 20;
	// 	loraUpdate();
	// 	wifiStop();
	// }

	// Send Ping in handle Root
	// if (auto_ping && (millis() > auto_ping_last_send + 2500)) {
	// 	// Empfange Ping
	// 	//Lora Send String
	// 	auto_ping_count += 1;
	// 	sendString = String(auto_ping_count);
	// 	LoRa.beginPacket();
	// 	LoRa.print(sendString);
	// 	LoRa.endPacket();
	// 	auto_ping_last_send = millis();
	// }
}