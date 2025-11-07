#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <WiFiClientSecure.h>
#include <UniversalTelegramBot.h>
#include "DHT.h"

#define DHTTYPE_OUTDOOR DHT21 // DHT 21 (AM2301)
#define DHTTYPE_INDOOR DHT22  // DHT 22  (AM2302), AM2321

const char *ssid = "StarLink";
const char *password = "R1o2o3t4cvD5a6n7i8e9l";

// Telegram Bot Token
#define BOT_TOKEN "5052650376:AAHw2eWqp85AKoQsrCvdfSMJlw1OfQjbiOA"

// Инициализация WiFiClientSecure для HTTPS соединений
WiFiClientSecure client;
UniversalTelegramBot bot(BOT_TOKEN, client);

ESP8266WebServer server(80);

uint8_t DHTPinIndoor = D4;	// Пин для датчика в помещении
uint8_t DHTPinOutdoor = D5; // Пин для датчика на улице

// Инициализация датчиков
DHT dhtIndoor(DHTPinIndoor, DHTTYPE_INDOOR);
DHT dhtOutdoor(DHTPinOutdoor, DHTTYPE_OUTDOOR);

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

	pinMode(DHTPinIndoor, INPUT);
	pinMode(DHTPinOutdoor, INPUT);

	dhtIndoor.begin();
	dhtOutdoor.begin();

	Serial.println("Connecting to ");
	Serial.println(ssid);

	// Connect to your local Wi-Fi network with timeout
	int attempts = 0;
	WiFi.begin(ssid, password);
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
		server.on("/ajax", handle_AJAX); // Новый обработчик AJAX-запросов
		server.onNotFound(handle_NotFound);

		server.begin();
		Serial.println("HTTP server started");

		client.setInsecure(); // Используйте это для тестирования, но для реального использования лучше установить сертификаты

		// Установка обработчика события потери соединения с WiFi
		WiFi.onStationModeDisconnected([](const WiFiEventStationModeDisconnected &)
									   {
			Serial.println("WiFi disconnected! Restarting server...");
			server.close();
			delay(1000);
			server.begin(); });
	}
	else
	{
		Serial.println("");
		Serial.println("Connection failed. Check your credentials or restart the ESP8266.");
	}
}

void loop()
{
	// Обработка клиентских запросов
	server.handleClient();

	// Проверка подключения к WiFi
	if (WiFi.status() != WL_CONNECTED)
	{
		Serial.println("WiFi отключен! Переподключение...");
		reconnectWiFi();
	}

	// Чтение данных с датчиков
	readSensorData();

	// Получение новых сообщений от Telegram
	int numNewMessages = bot.getUpdates(bot.last_message_received + 1);

	// Обработка новых сообщений
	for (int i = 0; i < numNewMessages; i++)
	{
		String chat_id = String(bot.messages[i].chat_id);
		String text = bot.messages[i].text;

		if (text == "/start")
		{
			bot.sendMessage(chat_id, "Привіт👋🏻 Мене звати CYBLIGHT 🤖 Я ваш погодний асистент ☀️\n"
									 "Запитуйте мене про температуру і вологість, і я з радістю допоможу!\n"
									 "Надішліть /temp, щоб отримати температуру та вологість.\n"
									 "Більше команд можна знайти в меню або відправив команду /help 🤓",
							"");
		}
		else if (text == "/temp")
		{
			// Проверка наличия значений температуры и влажности
			if (!isnan(TemperatureIndoor) && !isnan(HumidityIndoor) && !isnan(TemperatureOutdoor) && !isnan(HumidityOutdoor))
			{
				String message = "🌡 Indoor Temperature: " + String(TemperatureIndoor, 1) + "°C\n" +
								 "\n" +
								 "💧 Indoor Humidity: " + String(HumidityIndoor, 1) + "%\n" +
								 "\n" +
								 "☀️ Outdoor Temperature: " + String(TemperatureOutdoor, 1) + "°C\n" +
								 "\n" +
								 "🌦 Outdoor Humidity: " + String(HumidityOutdoor, 1) + "%";
				bot.sendMessage(chat_id, message, "");
			}
			else
			{
				bot.sendMessage(chat_id, "❗️Помилка читання даних з датчиків❗️", "");
			}
		}
		else if (text == "/indoor")
		{
			if (!isnan(TemperatureIndoor) && !isnan(HumidityIndoor))
			{
				String message = "🌡 Indoor Temperature: " + String(TemperatureIndoor, 1) + "°C\n" +
								 "\n" +
								 "💧 Indoor Humidity: " + String(HumidityIndoor, 1) + "%";
				bot.sendMessage(chat_id, message, "");
			}
			else
			{
				bot.sendMessage(chat_id, "❗️Помилка читання даних із датчика в приміщенні❗️", "");
			}
		}
		else if (text == "/outdoor")
		{
			if (!isnan(TemperatureOutdoor) && !isnan(HumidityOutdoor))
			{
				String message = "☀️ Outdoor Temperature: " + String(TemperatureOutdoor, 1) + "°C\n" +
								 "\n" +
								 "🌦 Outdoor Humidity: " + String(HumidityOutdoor, 1) + "%";
				bot.sendMessage(chat_id, message, "");
			}
			else
			{
				bot.sendMessage(chat_id, "❗️Помилка читання даних з датчика на вулиці❗️", "");
			}
		}
		else if (text == "/restart_indoor")
		{
			dhtIndoor.begin(); // Перезапуск датчика в помещении
			bot.sendMessage(chat_id, "Датчик у приміщенні перезапущено✅", "");
		}
		else if (text == "/restart_outdoor")
		{
			dhtOutdoor.begin(); // Перезапуск датчика на улице
			bot.sendMessage(chat_id, "Датчик на вулиці перезапущено✅", "");
		}
		else if (text == "/restart_all")
		{
			dhtIndoor.begin();	// Перезапуск датчика в помещении
			dhtOutdoor.begin(); // Перезапуск датчика на улице
			bot.sendMessage(chat_id, "Усі датчики перезапущені✅", "");
		}

		else if (text == "/status")
		{
			String message = "WiFi статус: ";
			message += (WiFi.status() == WL_CONNECTED) ? "🛜 Підключений✅\n" : "🛜 Відключений🚫\n";

			message += "Стан датчиків:\n";
			message += "Приміщення: ";
			message += (!isnan(TemperatureIndoor) && !isnan(HumidityIndoor)) ? "Працює✅\n" : "⚠️Помилка⚠️\n";

			message += "Вулиця: ";
			message += (!isnan(TemperatureOutdoor) && !isnan(HumidityOutdoor)) ? "Працює✅\n" : "⚠️Помилка⚠️\n";

			bot.sendMessage(chat_id, message, "");
		}
		else if (text == "/uptime")
		{
			long seconds = millis() / 1000;
			long minutes = (seconds / 60) % 60;
			long hours = (seconds / 3600) % 24;
			long days = seconds / 86400;

			String message = "⏳Час роботи сервера: ";
			message += String(days) + " днів ";
			message += String(hours) + " годин ";
			message += String(minutes) + " хвилин";

			bot.sendMessage(chat_id, message, "");
		}
		else if (text.startsWith("/set_threshold "))
		{
			String params = text.substring(15);
			int separatorIndex = params.indexOf(' ');
			if (separatorIndex != -1)
			{
				float tempThreshold = params.substring(0, separatorIndex).toFloat();
				float humThreshold = params.substring(separatorIndex + 1).toFloat();

				// Сохранение пороговых значений (в глобальных переменных или EEPROM)
				// tempThreshold и humThreshold нужно сохранить для дальнейшего использования

				bot.sendMessage(chat_id, "Порогові значення встановлені: Температура " + String(tempThreshold) + "°C, Вологість " + String(humThreshold) + "%.", "");
			}
			else
			{
				bot.sendMessage(chat_id, "Неправильний формат. Використовуйте /set_threshold [температура] [влажность]", "");
			}
		}
		else if (text == "/help")
		{
			String message = "Доступні команди:\n";
			message += "/start - Початок роботи з ботом\n";
			message += "/temp - Отримати загальну температуру і вологість\n";
			message += "/indoor - Температура і вологість у приміщенні\n";
			message += "/outdoor - Температура і вологість на вулиці\n";
			message += "/restart_all - Перезапуск усіх датчиків\n";
			message += "/restart_indoor - Перезапустити датчик в приміщенні\n";
			message += "/restart_outdoor - Перезапустити датчик на вулиці\n";
			message += "/status - Статус підключення Wi-Fi і датчиків\n";
			message += "/uptime - Час роботи пристрою\n";
			message += "/set_threshold [температура] [вологість] - Встановити порогові значення\n";
			message += "/help - Список команд\n";
			message += "У бота є пасхалка - про яку знає тільки розробник 😁\n";

			bot.sendMessage(chat_id, message, "");
		}
	}
	delay(5000); // Задержка между проверками сообщений
}

void reconnectWiFi()
{
	int attempts = 0;
	WiFi.begin(ssid, password);
	while (WiFi.status() != WL_CONNECTED && attempts < 20)
	{
		delay(500); // Подождите 500 мс перед каждой попыткой
		Serial.print(".");
		attempts++;
	}

	if (WiFi.status() == WL_CONNECTED)
	{
		Serial.println("");
		Serial.println("WiFi подключен..!");
		Serial.print("IP-адрес: ");
		Serial.println(WiFi.localIP());
	}
	else
	{
		Serial.println("");
		Serial.println("Ошибка подключения. Проверьте ваши учетные данные или перезагрузите ESP8266.");
	}
}

void readSensorData()
{
	float tempIndoor = dhtIndoor.readTemperature(); // Чтение температуры в помещении
	float humIndoor = dhtIndoor.readHumidity();		// Чтение влажности в помещении

	float tempOutdoor = dhtOutdoor.readTemperature(); // Чтение температуры на улице
	float humOutdoor = dhtOutdoor.readHumidity();	  // Чтение влажности на улице

	TemperatureIndoor = dhtIndoor.readTemperature();
	HumidityIndoor = dhtIndoor.readHumidity();
	TemperatureOutdoor = dhtOutdoor.readTemperature();
	HumidityOutdoor = dhtOutdoor.readHumidity();

	// Проверка на ошибки чтения данных с датчиков
	if (isnan(tempIndoor) || isnan(humIndoor) || isnan(tempOutdoor) || isnan(humOutdoor))

	{
		Serial.println("Ошибка при чтении данных с датчика DHT!");
	}
	else
	{
		TemperatureIndoor = tempIndoor;
		HumidityIndoor = humIndoor;
		TemperatureOutdoor = tempOutdoor;
		HumidityOutdoor = humOutdoor;
	}
	return;
}

void handle_OnConnect()
{
	server.send(200, "text/html", SendHTML(TemperatureIndoor, HumidityIndoor, TemperatureOutdoor, HumidityOutdoor));
}

void handle_NotFound()
{
	server.send(404, "text/plain", "Not found");
}

void handle_AJAX()
{
	// Читаем данные с датчиков
	readSensorData();

	// Создаем JSON-объект
	String response = "{\"temperatureIndoor\": " + String(TemperatureIndoor, 1) + ", ";
	response += "\"humidityIndoor\": " + String(HumidityIndoor, 1) + ", ";
	response += "\"temperatureOutdoor\": " + String(TemperatureOutdoor, 1) + ", ";
	response += "\"humidityOutdoor\": " + String(HumidityOutdoor, 1) + "}";

	// Отправляем JSON-ответ
	server.send(200, "application/json", response);
}

String generateHeader()
{
	String header;
	header += "<div class=\"header\">\n";
	header += "<a href=\"/\" class=\"logo\">\n";
	header += "<img src=\"https://storage.googleapis.com/smarthomemedia/Logo.svg\" alt=\"Smart Home Logo\" class=\"logo\">\n";
	header += "</a>\n";
	header += "<h1>Інформація про температуру</h1>\n";
	// header += "<div class=\"buttons\">\n";
	// header += "<a href=\"/\" class=\"button\">Головна</a>\n";
	// header += "<a href=\"/project\" class=\"button\">PROJECT</a>\n";
	// header += "<a href=\"/arduino\" class=\"button\">ARDUINO</a>\n";
	// header += "<a href=\"/contacts\" class=\"button\">Контакти</a>\n";
	// header += "</div>\n";
	header += "</div>\n";
	return header;
}

String SendHTML(float temperatureIndoor, float humidityIndoor, float temperatureOutdoor, float humidityOutdoor)
{
	String ptr = "<!DOCTYPE html> <html>\n";

	ptr += "<meta http-equiv=\"Content-type\" content=\"text/html; charset=utf-8\">\n";

	ptr += "<head>\n";

	ptr += "<meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0, user-scalable=no\">\n";
	ptr += "<meta http-equiv=\"Cache-Control\" content=\"no-cache, no-store, must-revalidate\">\n";
	ptr += "<meta http-equiv=\"Pragma\" content=\"no-cache\">\n";
	ptr += "<meta http-equiv=\"Expires\" content=\"0\">\n";

	ptr += "<link href=\"https://fonts.googleapis.com/css?family=Open+Sans:300,400,600\" rel=\"stylesheet\">\n";
	ptr += "<title>Smart Home Hub</title>\n";
	ptr += "<link rel=\"icon\" href=\"https://storage.googleapis.com/smarthomemedia/favicon_1.ico\" type =\"image/x-icon\">";

	ptr += "<style>\n";
	ptr += "html { font-family: 'Open Sans', sans-serif; display: block; margin: 0px; padding: 0; text-align: center;color: #333333;}\n";
	ptr += "body{margin: 0 auto; padding: 0; width: 100%; max-width: 100%;}\n";
	ptr += "h1 {margin: 50px auto 30px;}\n";
	ptr += ".side-by-side{display: inline-block;vertical-align: middle;position: relative;}\n";
	ptr += ".humidity-icon{background-color: #3498db;width: 30px;height: 30px;border-radius: 50%;line-height: 36px;}\n";
	ptr += ".humidity-text{font-weight: 600;padding-left: 15px;font-size: 19px;width: 160px;text-align: left;}\n";
	ptr += ".humidity{font-weight: 300;font-size: 60px;color: #3498db;}\n";
	ptr += ".temperature-icon{background-color: #f39c12;width: 30px;height: 30px;border-radius: 50%;line-height: 40px;}\n";
	ptr += ".temperature-text{font-weight: 600;padding-left: 15px;font-size: 19px;width: 160px;text-align: left;}\n";
	ptr += ".temperature{font-weight: 300;font-size: 60px;color: #f39c12;}\n";
	ptr += ".superscript{font-size: 17px;font-weight: 600;position: absolute;right: -20px;top: 15px;}\n";
	ptr += ".header{background-color: #49c9d7; color: #333; padding: 10px 0; display: flex; justify-content: space-between; align-items: center;}\n";
	ptr += ".logo{float: left; margin-bottom: 15px; margin-left: 15px; margin-top: 15px;}\n";
	ptr += ".buttons {margin-right: 20px;}\n";
	ptr += ".button {padding: 5px 10px; background-color: #333; color: #fff; text-decoration: none; margin-left: 10px; border-radius: 5px;}\n";
	ptr += "footer{position: fixed; bottom: 0; left: 0; width: 100%; margin: 0; padding: 10px 0; background-color: #1e905d; color: #ffffff; text-align: center;}\n";
	ptr += ".data{padding: 10px; background-color: #c3e6cb;}\n";
	ptr += ".main-content{padding: 10px; background-color: #c3e6cb;}\n";
	ptr += ".language-button {position: absolute; top: 10px; right: 10px;}\n";
	ptr += "@media only screen and (max-width: 600px) {body {margin: 0; padding-bottom: 60px;} h1 {margin-top: 70px;}}\n";
	ptr += "@media only screen and (max-width: 480px) {body {margin: 0; padding-bottom: 60px;} h1 {margin-top: 70px;}}\n";
	ptr += "</style>\n";

	ptr += "<script>\n";
	ptr += "var language = 'uk';\n"; // Украинский язык по умолчанию
	ptr += "var texts = {\n";
	ptr += "'uk': {\n";
	ptr += "'footer-text-1': '© CybLight, 2024',\n";
	ptr += "'footer-text-2': 'Сайт на 100% зроблений із вторинно перероблених пікселів',\n";
	ptr += "'button-text': 'English'\n";
	ptr += "},\n";
	ptr += "'en': {\n";
	ptr += "'footer-text-1': '© CybLight, 2024',\n";
	ptr += "'footer-text-2': 'Сайт на 100% сделан из вторично переработанных пикселей',\n";
	ptr += "'button-text': 'Русский'\n";
	ptr += "},\n";
	ptr += "'ru': {\n";
	ptr += "'footer-text-1': '© CybLight, 2024',\n";
	ptr += "'footer-text-2': 'Site made from 100% recycled pixels',\n";
	ptr += "'button-text': 'Українська'\n";
	ptr += "}\n";
	ptr += "};\n";

	ptr += "function switchLanguage() {\n";
	ptr += "if (language === 'uk') language = 'en';\n";		 // Переключаем на английский, если текущий украинский
	ptr += "else if (language === 'en') language = 'ru';\n"; // Переключаем на русский, если текущий английский
	ptr += "else language = 'uk';\n";						 // Иначе переключаем на украинский
	ptr += "document.getElementById('language-button').textContent = texts[language]['button-text'];\n";
	ptr += "updateFooter();\n";
	ptr += "}\n";

	ptr += "function updateFooter() {\n";
	ptr += "document.getElementById('footer-text-1').textContent = texts[language]['footer-text-1'];\n";
	ptr += "document.getElementById('footer-text-2').textContent = texts[language]['footer-text-2'];\n";
	ptr += "}\n";

	ptr += "</script>\n";

	ptr += "<script>\n";
	ptr += "setInterval(loadTempData,10000);\n";
	ptr += "function loadTempData() {\n";
	ptr += "var xhttp = new XMLHttpRequest();\n";
	ptr += "xhttp.onreadystatechange = function() {\n";
	ptr += "if (this.readyState == 4 && this.status == 200) {\n";
	ptr += "var tempData = JSON.parse(this.responseText);\n";
	ptr += "document.getElementById(\"indoorTemp\").innerHTML = tempData.temperatureIndoor + '°C';\n";
	ptr += "document.getElementById(\"outdoorTemp\").innerHTML = tempData.temperatureOutdoor + '°C';\n";
	ptr += "document.getElementById(\"indoorHumidity\").innerHTML = tempData.humidityIndoor + '%';\n";
	ptr += "document.getElementById(\"outdoorHumidity\").innerHTML = tempData.humidityOutdoor + '%';\n";
	ptr += "}\n";
	ptr += "};\n";
	ptr += "xhttp.open(\"GET\", \"/ajax\", true);\n"; // Обработчик AJAX-запросов
	ptr += "xhttp.send();\n";
	ptr += "}\n";
	ptr += "</script>\n";

	ptr += "</head>\n";

	ptr += "<body>\n";

	ptr += "<button class=\"language-button\" onclick=\"switchLanguage()\" id=\"language-button\">Русский</button>\n";

	ptr += generateHeader();

	ptr += "<div class=\"main-content\" id=\"webpage\">\n";

	ptr += "<div class=\"data\">\n";
	ptr += "<h2>Всередині приміщення:</h2>\n";
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
	ptr += "<div id=\"indoorTemp\" class=\"side-by-side temperature\">";
	ptr += String(temperatureIndoor, 1); // Форматирование температуры с одним знаком после запятой
	// ptr +=(int)indoorTemp;
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
	ptr += "<div id=\"indoorHumidity\" class=\"side-by-side humidity\">";
	ptr += String(humidityIndoor, 1); // Форматирование влажности с одним знаком после запятой
	ptr += "<span class=\"superscript\">%</span></div>\n";
	ptr += "</div>\n";

	ptr += "<div class=\"data\">\n";
	ptr += "<h2>На вулиці:</h2>\n";
	ptr += "<div class=\"side-by-side temperature-icon\">\n";

	ptr += "<svg version=\"1.1\" id=\"Layer_1_outdoor\" xmlns=\"http://www.w3.org/2000/svg\" xmlns:xlink=\"http://www.w3.org/1999/xlink\" x=\"0px\" y=\"0px\"\n";
	ptr += "width=\"9.915px\" height=\"22px\" viewBox=\"0 0 9.915 22\" enable-background=\"new 0 0 9.915 22\" xml:space=\"preserve\">\n";
	ptr += "<path fill=\"#FFFFFF\" d=\"M3.498,0.53c0.377-0.331,0.877-0.501,1.374-0.527C5.697-0.04,6.522,0.421,6.924,1.142\n";
	ptr += "c0.237,0.399,0.315,0.871,0.311,1.33C7.229,5.856,7.245,9.24,7.227,12.625c1.019,0.539,1.855,1.424,2.301,2.491\n";
	ptr += "c0.491,1.163,0.518,2.514,0.062,3.693c-0.414,1.102-1.24,2.038-2.276,2.594c-1.056,0.583-2.331,0.743-3.501,0.463\n";
	ptr += "c-1.417-0.323-2.659-1.314-3.3-2.617C0.014,18.26-0.115,17.104,0.1,16.022c0.296-1.443,1.274-2.717,2.58-3.394\n";
	ptr += "c0.013-3.44,0-6.881,0.007-10.322C2.674,1.634,2.974,0.955,3.498,0.53z\"/>\n";
	ptr += "</svg>\n";

	ptr += "</div>\n";
	ptr += "<div class=\"side-by-side temperature-text\">Температура</div>\n";
	ptr += "<div id=\"outdoorTemp\" class=\"side-by-side temperature\">";
	ptr += String(temperatureOutdoor, 1); // Форматирование температуры с одним знаком после запятой
	ptr += "<span class=\"superscript\">°C</span></div>\n";
	ptr += "</div>\n";

	ptr += "<div class=\"data\">\n";
	ptr += "<div class=\"side-by-side humidity-icon\">\n";

	ptr += "<svg version=\"1.1\" id=\"Layer_2_outdoor\" xmlns=\"http://www.w3.org/2000/svg\" xmlns:xlink=\"http://www.w3.org/1999/xlink\" x=\"0px\" y=\"0px\"\n\"; width=\"12px\" height=\"17.955px\" viewBox=\"0 0 13 17.955\" enable-background=\"new 0 0 13 17.955\" xml:space=\"preserve\">\n";
	ptr += "<path fill=\"#FFFFFF\" d=\"M1.819,6.217C3.139,4.064,6.5,0,6.5,0s3.363,4.064,4.681,6.217c1.793,2.926,2.133,5.05,1.571,7.057\n";
	ptr += "c-0.438,1.574-2.264,4.681-6.252,4.681c-3.988,0-5.813-3.107-6.252-4.681C-0.313,11.267,0.026,9.143,1.819,6.217\"></path>\n";
	ptr += "</svg>\n";

	ptr += "</div>\n";
	ptr += "<div class=\"side-by-side humidity-text\">Вологість</div>\n";
	ptr += "<div id=\"outdoorHumidity\" class=\"side-by-side humidity\">";
	ptr += String(humidityOutdoor, 1); // Форматирование влажности с одним знаком после запятой
	ptr += "<span class=\"superscript\">%</span></div>\n";

	ptr += "</div>\n";
	ptr += "</body>\n";

	ptr += "<footer>";
	ptr += "<div>© CybLight, 2024</div>";
	ptr += "<div>Сайт на 100% зроблений із вторинно перероблених пікселів</div>";
	ptr += "</footer>";
	ptr += "</html>\n";
	return ptr;
}
