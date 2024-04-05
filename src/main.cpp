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

const char ping[] = "ping";
const char ack[] = "ack";


#define WIFI
#define WIFI_LOGIN

#define DEBUG

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

const char* wifi_ssid = "Universum der Liebe";
const char* wifi_password = "t88E6dHxtkbf";

// const char* wifi_ssid = "KabelBox-E838";
// const char* wifi_password = "89581988927807050157";

WebServer server(80);
ulong auto_ping_interval = 10e3;

int spread = 12;
long bandwidth = 7.8E3;
int txPower = 20;

ulong lastSendPing = 0;
ulong lastSendAck = 0;

ulong rtt = 0;

ulong lastRecPing = 0;
ulong lastRecAck = 0;


void send_ping(){
	lastSendPing = millis();
	LoRa.beginPacket();
	LoRa.write((uint8_t*)ping, sizeof(ping));
	LoRa.endPacket();
}

void send_ack(){
	lastSendAck = millis();
	LoRa.beginPacket();
	LoRa.write((uint8_t*)ack, sizeof(ack));
	LoRa.endPacket(); 
}

void succ_rec_ping(){
	lastRecPing = millis();
	send_ack();
}

void succ_rec_ack(){
	lastRecAck = millis();
	rtt = lastRecAck - lastSendPing;
}

void receive(){
    int psize = LoRa.parsePacket();
    if (psize == sizeof(ping)){
        char buffer[sizeof(ping)];
        LoRa.readBytes(buffer, sizeof(ping));
		if (!strncmp(buffer,ping,sizeof(ping))){
			succ_rec_ping();
		}
    } else if (psize == sizeof(ack)){
       char buffer[sizeof(ack)];
        LoRa.readBytes(buffer, sizeof(ack));
		if (!strncmp(buffer,ack,sizeof(ack))){
			succ_rec_ack();
		}
    }else if (psize){
        ser_println("Faulty Package");
        while (LoRa.available()){
			char a = LoRa.read();
		}
    }
}

void loraUpdate(){
	LoRa.setTxPower(txPower);
	LoRa.setSpreadingFactor(spread);
}

void handleRoot() {
	String rssi = String(LoRa.rssi());
	String prssi = String(LoRa.packetRssi());
	String psnr = String(LoRa.packetSnr());
	String pfreqerr = String(LoRa.packetFrequencyError());
	ulong currentTime = millis();

	String html = "<!DOCTYPE html><html><head><title>ESP32 LoRa WebServer</title></head><body>";
	html += "<h1>ESP32 LoRa WebServer</h1>";

	html += "<p>Own Info - RSSI: " + rssi + " Packet RSSI: " + prssi + " Packet SNR: " + psnr + " Packet Freqency Error: " + pfreqerr + "</p>";

	html += "<p>System time: "+String(currentTime)+"ms</p>";
	html += "<p>Ping Send: "+String(currentTime - lastSendPing)+" Ping received: "+String(currentTime - lastRecAck)+" Total: "+String(rtt)+"ms.</p>";

	// html += "<form method='post' action='/togglePing'><button type='submit'>Auto Ping is "+String(auto_ping)+"</button></form>";
	html += "<form method='post' action='/ping'><button type='submit'>Ping</button></form>";
	html += "<form method='post' action='/post'>Auto Ping Intervall current: "+String(auto_ping_interval)+" (in ms):<input type='number' name='numberInput' min='0' step='1000'><input type='submit' value='Submit'></form>";
	html += "<form method='post' action='/post'>Tx Power current: "+String(txPower)+" (dBm):<input type='number' name='numberInputTX' min='0' step='1' max='20'><input type='submit' value='Submit'></form>";
	html += "<form method='post' action='/post'>Spread current: "+String(spread)+" :<input type='number' name='numberInputSpread' min='0' step='1' max='12'><input type='submit' value='Submit'></form>";

	// html += "<form method='get' action='/'><input type='submit' value='Custom Msg'></form>";
	html += "</body></html>";
	server.send(200, "text/html", html);
}

void handlePing(){
	send_ping();
	server.sendHeader("Location", "/");
	server.send(302, "text/plain", "");
}

void handlePost() {
	if (server.hasArg("numberInput")) {
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
	// LoRa.setSpreadingFactor(spread);
	// LoRa.setSignalBandwidth(bandwidth); // Set BW to Lowest
	// LoRa.setTxPower(txPower);
	ser_println("LoRa Initializing OK!");
}

void wifiInit(){
	ser_println("Wifi init");
	#ifdef WIFI_LOGIN
		WiFi.begin(wifi_ssid, wifi_password);
		while (WiFi.status() != WL_CONNECTED) {
			delay(100);
			ser_print(".");
		}
		IPAddress IP = WiFi.localIP();
	#else
		WiFi.softAP(own_ssid, own_password);
		IPAddress IP = WiFi.softAPIP();
	#endif
	ser_print("IP address: ");
	ser_println(IP);
	//Server
	server.on("/", HTTP_GET,handleRoot);
	server.on("/post", HTTP_POST,handlePost);
	server.on("/ping", HTTP_POST,handlePing);
	server.begin();
	ser_println("Wifi and Server initialized.");
}


void setup() {
	// Serial
	ser_beg(115200);
	//LORA
	LoraInit();
	#ifdef WIFI
		wifiInit();
	#endif
}

void loop() {
	#ifdef WIFI
		server.handleClient();
	#endif
	
	receive();

	if (millis() - lastSendPing > auto_ping_interval){
		send_ping();
	}

}