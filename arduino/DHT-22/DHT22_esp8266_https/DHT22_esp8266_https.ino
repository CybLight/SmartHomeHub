#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <WiFiClientSecure.h>
#include <StreamString.h>
#include "ESP8266HTTPClient.h"
#include <ESP8266WebServerSecure.h>
#include "DHT.h"

#define DHTTYPE_OUTDOOR DHT21 // DHT 21 (AM2301)
#define DHTTYPE_INDOOR DHT22	// DHT 22  (AM2302), AM2321

const char *ssid = "StarLink";
const char *password = "R1o2o3t4cvD5a6n7i8e9l";

const char *host = "smarthomehub.ddns.net";
const int httpsPort = 443;

ESP8266WebServerSecure server(443);

const char *rootCACertificate = R"(
-----BEGIN CERTIFICATE-----
MIIHDjCCBPagAwIBAgIQAr3LOFsFTG96SL0R1LfwXzANBgkqhkiG9w0BAQsFADBx
MQswCQYDVQQGEwJVUzErMCkGA1UEChMiVml0YWx3ZXJrcyBJbnRlcm5ldCBTb2x1
dGlvbnMsIExMQzE1MDMGA1UEAxMsVml0YWx3ZXJrcyBJbnRlcm5ldCBTb2x1dGlv
bnMsIE5vLUlQIFRMUyBJQ0EwHhcNMjQwMjExMDAwMDAwWhcNMjUwMjEwMjM1OTU5
WjAgMR4wHAYDVQQDExVzbWFydGhvbWVodWIuZGRucy5uZXQwggEiMA0GCSqGSIb3
DQEBAQUAA4IBDwAwggEKAoIBAQDkgVqKa5Qwdvp0di8M6e8YgLJ5nRxBi+lsLBJq
HRwywAYYmSU/Lz85pWn6rjwrlRvqFw15DkMxtF2h3xwwXempMGm6klxqhxEn3mQk
TF1e4wmY6JGDPSNaEoHPqxs24szFx8Mp1rBjSGJiAdIjbRNnHzLMAtXG2Km1HKq4
18R9+PVC1sFxavRUwtDPahSzkfJz6C4QvEwXwY7tSTBby26dJeNz8Zd/Ka7zuAIm
HPAgPBddyJV1nnbHCSSlPMs2ZFOG29OkooZfIceFRjthoA+kHoxEOpLBFr7tjQHl
Emu5VY9u2wdVCyY5jzOkVaZkzoALdZ6qnb2fmS1He3RtGrjDAgMBAAGjggLxMIIC
7TAfBgNVHSMEGDAWgBQR6j3bKFdj3MJOLnZrGOwv+6osnDAdBgNVHQ4EFgQUe/XK
DkZ1xC80tD6Q4fApfd5R7BYwIAYDVR0RBBkwF4IVc21hcnRob21laHViLmRkbnMu
bmV0MD4GA1UdIAQ3MDUwMwYGZ4EMAQIBMCkwJwYIKwYBBQUHAgEWG2h0dHA6Ly93
d3cuZGlnaWNlcnQuY29tL0NQUzAOBgNVHQ8BAf8EBAMCBaAwHQYDVR0lBBYwFAYI
KwYBBQUHAwEGCCsGAQUFBwMCMIGIBggrBgEFBQcBAQR8MHowJAYIKwYBBQUHMAGG
GGh0dHA6Ly9vY3NwLmRpZ2ljZXJ0LmNvbTBSBggrBgEFBQcwAoZGaHR0cDovL2Nh
Y2VydHMuZGlnaWNlcnQuY29tL1ZpdGFsd2Vya3NJbnRlcm5ldFNvbHV0aW9uc05v
LUlQVExTSUNBLmNydDAMBgNVHRMBAf8EAjAAMIIBfwYKKwYBBAHWeQIEAgSCAW8E
ggFrAWkAdwBOdaMnXJoQwzhbbNTfP1LrHfDgjhuNacCx+mSxYpo53wAAAY2ZyrCL
AAAEAwBIMEYCIQDzuNq9rt3P2BbFfySanMvfzdYI2Ggd1k7BE7gX/s2/DQIhAJUO
5A4V3NBXJWdsn9ZOgfgMfaDS2u7qkceDEiJGHJ2ZAHcAfVkeEuF4KnscYWd8Xv34
0IdcFKBOlZ65Ay/ZDowuebgAAAGNmcqwsQAABAMASDBGAiEAllr2K+oFl3kkf8Me
5pM3hLSBxW3zeMgMU2rc9WmHmH4CIQCjxBJVoL5Pu9hGT7I07vsPNMl6OedUhoAX
pNNvEsxmcQB1AObSMWNAd4zBEEEG13G5zsHSQPaWhIb7uocyHf0eN45QAAABjZnK
sNsAAAQDAEYwRAIgUQ4/YilO3eIdJOJmNSpvlocB/TuWuz7Zhc/UyINRqNUCIEWO
Vm5JQmvSdT57Y5sJQ/goTQF2Nrirx/APNMuN9jkFMA0GCSqGSIb3DQEBCwUAA4IC
AQADIYzTLVAhomhlkBvHJmX5Sv/w4r+Cx1VmX9JsJ2S33+1CYZ1tIhyzobrt1wp8
1lUwgY9VKodBMy5hrML11faMcYAGixOGbgwTNHdoodPap8luSiYRYXAe1ZETuosJ
/ibLHgv0swAFNLNzssQpjesM3sV5vIoH6YPoOMaP84klW2gdvYZk0uKUvyHYban/
sZIbIEL8f0X0KV34iNObB4ZLaSJl00oaX5Kcwe/4/r7kTg+jqB/FY+LQt9XNVoEC
e8YOT/eezByAJ4g3LM4kurUdn53KgwEbSmMI16H7CrXIj77ujiKMW8Z1S55RDtAV
nxjBI9vHMdPXpKtPb4DR7GKtyIFq39mkKbyr69tmnENdquNhsydr3FZhIwpybA1Z
fc7vsj59WYArZU8jFxwWfvudAgGQLJaTmjKLWlNMUgyUKntGw/5wmJbjuZ5q1Dpf
AgNTckJSwy+9rTzY/GBQlz+c3j+DLUAOkMdQrgeLo9A7Jf/+G90kGs/QywTVK0Tq
OiUaofYdH/2f5WxjvknO21W6fQNiUYFTyJurg0VtZIHfNUe4BWQfqkahw/uPXYXQ
WiiFf4E7JXFXsPINO653ygfrmU/TUZyci0EV0RQ4UTWEsH0ST73jkDizvzbgpNIN
xC5d4jz2PvOv6LQGQdQX2LfC6ii+7l+J3AE3nxQxSYkM5Q==
-----END CERTIFICATE-----
)";

uint8_t DHTPinIndoor = D4;	 // Пин для датчика в помещении
uint8_t DHTPinOutdoor = D5; // Пин для датчика на улице

// Инициализация датчиков
DHT dhtIndoor(DHTPinIndoor, DHTTYPE_INDOOR);
DHT dhtOutdoor(DHTPinOutdoor, DHTTYPE_OUTDOOR);

WiFiClientSecure client;

void handleRoot()
{
	// Читаем влажность
	float h = dht.readHumidity();
	// Читаем температуру в градусах Цельсия
	float t = dht.readTemperature();

	// Проверяем, удалось ли считать данные с датчика
	if (isnan(h) || isnan(t))
	{
		server.send(500, "text/plain", "Failed to read from DHT sensor!");
		return;
	}

	// Отправляем ответ об успешном выполнении запроса с данными с датчика
	String response = String(t) + ";" + String(h);
	server.send(200, "text/plain", response);

	// Отправляем GET-запрос на сервер и получаем ответ
	BearSSL::WiFiClientSecure client;
	client.setFingerprint(rootCACertificate);

	if (!client.connect(host, httpsPort))
	{
		Serial.println("Failed to connect to server!");
		return;
	}

	client.print(String("GET /") + " HTTP/1.1\r\n" + "Host: " + host + "\r\n" + "Connection: close\r\n\r\n");

	// Получаем и выводим ответ от сервера
	String serverResponse = "";
	while (client.connected() || client.available())
	{
		if (client.available())
		{
			char c = client.read();
			serverResponse += c;
		}
	}

	Serial.println("Response from server:");
	Serial.println(serverResponse);

	// Закрываем соединение
	client.stop();
}

float Temperature;
float Humidity;
float TemperatureIndoor;
float HumidityIndoor;
float TemperatureOutdoor;
float HumidityOutdoor;

void setup()
{
	Serial.begin(115200);
	delay(100);

	pinMode(DHTPin, INPUT);

	dht.begin();

	Serial.println("Connecting to ");
	Serial.println(ssid);

	// Connect to your local Wi-Fi network with timeout
	WiFi.begin(ssid, password);
	int attempts = 0;
	while (WiFi.status() != WL_CONNECTED && attempts < 20)
	{
		delay(500); // Подождите 500 мс перед каждой попыткой
		Serial.print(".");
		attempts++;
	}

	if (WiFi.status() == WL_CONNECTED)
	{
		Serial.println("");
		Serial.println("WiFi connected..!");
		Serial.print("Got IP: ");
		Serial.println(WiFi.localIP());

		server.on("/", handle_OnConnect);
		server.onNotFound(handle_NotFound);

		BearSSL::X509List rootCert(rootCACertificate);
		client.setTrustAnchors(&rootCert);

		server.begin();
		Serial.println("HTTP server started");
	}
	else
	{
		Serial.println("");
		Serial.println("Connection failed. Check your credentials or restart the ESP8266.");
	}
}

void loop()
{

	server.handleClient();
}

void handle_OnConnect()
{

	Temperature = dht.readTemperature();
	Humidity = dht.readHumidity();
	client.print(SendHTML(Temperature, Humidity));
}

void handle_NotFound()
{
	server.send(404, "text/plain", "Not found");
}

String SendHTML(float Temperaturestat, float Humiditystat)
{
	String ptr = "<!DOCTYPE html> <html>\n";
	ptr += "<meta http-equiv=\"Content-type\" content=\"text/html; charset=utf-8\"><head><meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0, user-scalable=no\">\n";
	ptr += "<meta http-equiv=\"Cache-Control\" content=\"no-cache, no-store, must-revalidate\">\n";
	ptr += "<meta http-equiv=\"Pragma\" content=\"no-cache\">\n";
	ptr += "<meta http-equiv=\"Expires\" content=\"0\">\n";
	ptr += "<link href=\"https://fonts.googleapis.com/css?family=Open+Sans:300,400,600\" rel=\"stylesheet\">\n";
	ptr += "<title>INFO TEMP</title>\n";
	ptr += "<style>html { font-family: 'Open Sans', sans-serif; display: block; margin: 0px auto; padding: 0; text-align: center;color: #333333;}\n";
	ptr += "body{margin-top: 50px; margin: 0px; padding: 0;}\n";
	ptr += "h1 {margin: 50px auto 30px;}\n";
	ptr += ".side-by-side{display: inline-block;vertical-align: middle;position: relative;}\n";
	ptr += ".humidity-icon{background-color: #3498db;width: 30px;height: 30px;border-radius: 50%;line-height: 36px;}\n";
	ptr += ".humidity-text{font-weight: 600;padding-left: 15px;font-size: 19px;width: 160px;text-align: left;}\n";
	ptr += ".humidity{font-weight: 300;font-size: 60px;color: #3498db;}\n";
	ptr += ".temperature-icon{background-color: #f39c12;width: 30px;height: 30px;border-radius: 50%;line-height: 40px;}\n";
	ptr += ".temperature-text{font-weight: 600;padding-left: 15px;font-size: 19px;width: 160px;text-align: left;}\n";
	ptr += ".temperature{font-weight: 300;font-size: 60px;color: #f39c12;}\n";
	ptr += ".superscript{font-size: 17px;font-weight: 600;position: absolute;right: -20px;top: 15px;}\n";
	ptr += ".header{background-color: #49c9d7; color: #ffffff; padding: 10px 0; display: flex; justify-content: space-between; align-items: center;}\n";
	ptr += ".logo{float: left; margin-bottom: 20px;}\n";
	ptr += ".buttons {margin-right: 20px;}\n";
	ptr += ".button {padding: 5px 10px; background-color: #333; color: #fff; text-decoration: none; margin-left: 10px; brder-radius: 5px;}\n";
	ptr += "footer{position: fixed; bottom: 0; width: 100%; margin: 0px; padding: 0; background-color: #1e905d; color: #ffffff; text-align: center; padding: 10px 0;}\n";
	ptr += ".data{padding: 10px;}\n";
	ptr += "</style>\n";
	// ptr +="<meta http-equiv=\"refresh\" content=\"2\" >\n";
	ptr += "<script>\n";
	ptr += "setInterval(loadDoc,200);\n";
	ptr += "function loadDoc() {\n";
	ptr += "var xhttp = new XMLHttpRequest();\n";
	ptr += "xhttp.onreadystatechange = function() {\n";
	ptr += "if (this.readyState == 4 && this.status == 200) {\n";
	ptr += "document.getElementById(\"webpage\").innerHTML =this.responseText}\n";
	ptr += "};\n";
	ptr += "xhttp.open(\"GET\", \"/\", true);\n";
	ptr += "xhttp.send();\n";
	ptr += "}\n";
	ptr += "</script>\n";

	ptr += "</head>\n";
	ptr += "<body>\n";

	ptr += "<div class=\"header\">\n";
	ptr += "<img src=\"https://storage.googleapis.com/smarthomemedia/Logo.svg\" alt=\"Smart Home Logo\" class=\"logo\">\n";
	ptr += "<div class=\"buttons\">\n";
	ptr += "<a href=\"/\" class=\"button\">Головна</a>\n";
	ptr += "<a href=\"/project\" class=\"button\">PROJECT</a>\n";
	ptr += "<a href=\"/arduino\" class=\"button\">ARDUINO</a>\n";
	ptr += "<a href=\"/contacts\" class=\"button\">Контакти</a>\n";
	ptr += "</div>\n";
	ptr += "</div>\n";

	ptr += "<div id=\"page-content\">\n";

	ptr += "<h1>Інформація про температуру в кімнаті:</h1>\n";
	ptr += "<div class=\"data\">\n";
	ptr += "<div class=\"side-by-side temperature-icon\">\n";
	ptr += "<svg version=\"1.1\" id=\"Layer_1\" xmlns=\"http://www.w3.org/2000/svg\" xmlns:xlink=\"http://www.w3.org/1999/xlink\" x=\"0px\" y=\"0px\"\n";
	ptr += "width=\"9.915px\" height=\"22px\" viewBox=\"0 0 9.915 22\" enable-background=\"new 0 0 9.915 22\" xml:space=\"preserve\">\n";
	ptr += "<path fill=\"#FFFFFF\" d=\"M3.498,0.53c0.377-0.331,0.877-0.501,1.374-0.527C5.697-0.04,6.522,0.421,6.924,1.142\n";
	ptr += "c0.237,0.399,0.315,0.871,0.311,1.33C7.229,5.856,7.245,9.24,7.227,12.625c1.019,0.539,1.855,1.424,2.301,2.491\n";
	ptr += "c0.491,1.163,0.518,2.514,0.062,3.693c-0.414,1.102-1.24,2.038-2.276,2.594c-1.056,0.583-2.331,0.743-3.501,0.463\n";
	ptr += "c-1.417-0.323-2.659-1.314-3.3-2.617C0.014,18.26-0.115,17.104,0.1,16.022c0.296-1.443,1.274-2.717,2.58-3.394\n";
	ptr += "c0.013-3.44,0-6.881,0.007-10.322C2.674,1.634,2.974,0.955,3.498,0.53z\"/>\n";
	ptr += "</svg>\n";
	ptr += "</div>\n";
	ptr += "<div class=\"side-by-side temperature-text\">Температура</div>\n";
	ptr += "<div class=\"side-by-side temperature\">";
	ptr += String(Temperaturestat, 1); // Форматирование температуры с одним знаком после запятой
	// ptr +=(int)Temperaturestat;
	ptr += "<span class=\"superscript\">°C</span></div>\n";
	ptr += "</div>\n";
	ptr += "<div class=\"data\">\n";
	ptr += "<div class=\"side-by-side humidity-icon\">\n";
	ptr += "<svg version=\"1.1\" id=\"Layer_2\" xmlns=\"http://www.w3.org/2000/svg\" xmlns:xlink=\"http://www.w3.org/1999/xlink\" x=\"0px\" y=\"0px\"\n\"; width=\"12px\" height=\"17.955px\" viewBox=\"0 0 13 17.955\" enable-background=\"new 0 0 13 17.955\" xml:space=\"preserve\">\n";
	ptr += "<path fill=\"#FFFFFF\" d=\"M1.819,6.217C3.139,4.064,6.5,0,6.5,0s3.363,4.064,4.681,6.217c1.793,2.926,2.133,5.05,1.571,7.057\n";
	ptr += "c-0.438,1.574-2.264,4.681-6.252,4.681c-3.988,0-5.813-3.107-6.252-4.681C-0.313,11.267,0.026,9.143,1.819,6.217\"></path>\n";
	ptr += "</svg>\n";
	ptr += "</div>\n";
	ptr += "<div class=\"side-by-side humidity-text\">Вологість</div>\n";
	ptr += "<div class=\"side-by-side humidity\">";
	ptr += String(Humiditystat, 1); // Форматирование влажности с одним знаком после запятой
	ptr += "<span class=\"superscript\">%</span></div>\n";
	ptr += "</div>\n";

	ptr += "</div>\n";
	ptr += "<footer>";
	ptr += "<div>© CybLight, 2024</div>";
	ptr += "<div>Сайт на 100% зроблений із вторинно перероблених пикселів</div>";
	ptr += "</footer>";
	ptr += "</body>\n";
	ptr += "</html>\n";
	return ptr;
}
