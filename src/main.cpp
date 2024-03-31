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

const char* own_ssid = "LoraTest";
const char* own_password = "00810081";

const char* wifi_ssid = "Universum der Liebe";
const char* wifi_password = "t88E6dHxtkbf9";


WebServer server(80);
String sendString = "LoRaTest";
String receivedString = "";
bool auto_ping = true;

// #define DEBUG

#ifdef DEBUG
#define ser_beg(x) Serial.begin(x)
#define ser_print(x) Serial.print(x)
#define ser_println(x) Serial.println(x)
#define ser_printf(x) Serial.printf(x)
#else
#define ser_beg(x) Serial.begin(x)
#define ser_print(x) Serial.print(x)
#define ser_println(x) Serial.println(x)
#define ser_printf(x) Serial.printf(x)
#endif

void handleRoot() {
	String rssi = String(LoRa.rssi());
	String prssi = String(LoRa.packetRssi());
	String psnr = String(LoRa.packetSnr());
	String pfreqerr = String(LoRa.packetFrequencyError());
	auto_ping = false;

	String html = "<!DOCTYPE html><html><head><title>ESP32 LoRa WebServer</title>";
	html += "</head><body>";
	html += "<h1>ESP32 LoRa WebServer</h1>";

	html += "<p>Info - RSSI: " + rssi + " Packet RSSI: " + prssi + " Packet SNR: " + psnr + " Packet Freqency Error: " + pfreqerr + "</p>";
	html += "<p>Last send Message: " + sendString + "<br>"+"Last received Message: " + receivedString + "</p>";

	html += "<form method='get' action='/autoPing'><input type='submit' value='Auto Ping'></form>";

	html += "<form method='post' action='/post'>Send Message (max 32 characters): <input type='text' name='message' maxlength='32'><input type='submit' value='Submit'></form>";
	html += "</body></html>";
	server.send(200, "text/html", html);
}


void handleAutoPing_update() {
	String data = "{\"rssi\": " + String(LoRa.rssi()) +
		", \"prssi\": " + String(LoRa.packetRssi()) +
		", \"psnr\": " + String(LoRa.packetSnr()) +
		", \"pfreqerr\": " + String(LoRa.packetFrequencyError()) +
		", \"recvStr\": " + receivedString +
		"}";
	server.send(200, "application/json", data);
}

void handleAutoPing() {
	String rssi = String(LoRa.rssi());
	String prssi = String(LoRa.packetRssi());
	String psnr = String(LoRa.packetSnr());
	String pfreqerr = String(LoRa.packetFrequencyError());
	auto_ping = true;
	
	String html = "<!DOCTYPE html><html><head><title>ESP32 LoRa WebServer</title>";
	html += "<meta http-equiv='refresh' content='1'></head><body>";
	html += "<h1>ESP32 LoRa WebServer</h1>";
	
	html += "<p>Info - RSSI: <span id='rssi'>" + rssi + "</span> Packet RSSI: <span id='prssi'>" + prssi + "</span> Packet SNR: <span id='psnr'>" + psnr + "</span> Packet Freqency Error: <span id='pfreqerr'>" + pfreqerr + "</span></p>";
	html += "<p>Last send Message: " + sendString + "<br>"+"Last received Message: <span id='recvStr'>" + receivedString + "</span></p>";

	html += "Auto Ping Mode";
	html += "<script>setInterval(() => {";
	html += "fetch('/autoPing_update').then(response => response.json()).then(data => {";
	html += "document.getElementById('rssi').innerText = data.rssi;";
	html += "document.getElementById('prssi').innerText = data.prssi;";
	html += "document.getElementById('psnr').innerText = data.psnr;";
	html += "document.getElementById('pfreqerr').innerText = data.pfreqerr;";
	html += "document.getElementById('recvStr').innerText = data.recvStr;";
	html += "});}, 1000);</script>";

	html += "<form method='get' action='/'><input type='submit' value='Custom Msg'></form>";

	html += "</body></html>";
	server.send(200, "text/html", html);
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
	} else {
		server.sendHeader("Location", "/", true);   // Redirect to the root page
		server.send(400, "text/plain", "No string received! Redirecting...");
	}
}

void setup() {
	// Serial
	ser_beg(115200);
	//LORA
	SPI.begin(SCK, MISO, MOSI, SS);
	LoRa.setPins(SS, RST, DI0);
	LoRa.setSpreadingFactor(12);
	LoRa.setSignalBandwidth(0); // Set BW to Lowest
	LoRa.rssi();
	LoRa.packetRssi();
	ser_println("LoRa Transceiver init.");
	while (!LoRa.begin(433E6)) {
		ser_print(".");
		delay(500);
	}
	ser_println("LoRa Initializing OK!");
	//Wifi login or host own
	WiFi.begin(wifi_ssid, wifi_password);
	double waitForWifiStart = millis();
	while (WiFi.status() != WL_CONNECTED && (millis() < waitForWifiStart + 10e3)) {
        delay(10);
	}
	bool wifi_connected = WiFi.status() == WL_CONNECTED;
	if (!wifi_connected)
		WiFi.softAP(own_ssid, own_password);
	IPAddress IP = WiFi.softAPIP();
	ser_print("AP IP address: ");
	ser_println(IP);
	server.on("/", HTTP_GET,handleRoot);
	server.on("/post", HTTP_POST,handlePost);
	server.on("/autoPing", HTTP_GET, handleAutoPing);
	server.on("/autoPing_update", HTTP_GET, handleAutoPing_update);
	server.begin();
}

double auto_ping_last_send = 0;
int auto_ping_count = 0;
void loop() {
	server.handleClient();
	// check if a packet has been received
	int packetSize = LoRa.parsePacket();
	if (packetSize) {
		receivedString = "";
		while (LoRa.available()) {
			receivedString += (char)LoRa.read();
		}
	}
	if (auto_ping && (millis() > auto_ping_last_send + 2500)) {
		//Lora Send String
		auto_ping_count += 1;
		sendString = String(auto_ping_count);
		LoRa.beginPacket();
		LoRa.print(sendString);
		LoRa.endPacket();
		auto_ping_last_send = millis();
	}
}