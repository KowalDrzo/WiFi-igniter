#include <Arduino.h>
#include <WiFi.h>
#include "pinout.h"

const char* ssid     = "ESP32-testy";
const char* password = "12345678";

String mainPage = R"rawliteral(
    <!DOCTYPE html>
    <html lang="pl-PL">
    <head>
        <title>Wifi Igniter</title>
        <meta charset="UTF-8">
        <style>
            body {
                font-family: Arial, sans-serif;
                margin: 0;
                padding: 0;
                display: flex;
                flex-direction: column;
                align-items: center;
                justify-content: center;
                height: 100vh;
                background-color: #f5f5f5;
            }

            h1 {
                font-size: 24px;
                margin-bottom: 20px;
            }

            #submitButton {
                background-color: #007bff;
                color: white;
                border: none;
                padding: 10px 20px;
                border-radius: 4px;
                cursor: pointer;
                transition: background-color 0.3s ease-in-out;
            }

            #submitButton:hover {
                background-color: red;
            }

            #continuityText {
                font-size: 18px;
                font-weight: bold;
                margin-top: 20px;
            }

            .en {
                color: green;
            }
            .dis {
                color: gray;
            }
        </style>
    </head>
    <body>
        <h1>Wifi Igniter</h1>
        <h3 id="continuityText">Ciągłość</h3>
        <button id="submitButton">Odpalenie zapalnika</button>

        <script>
            const submitButton = document.getElementById('submitButton');

            submitButton.addEventListener('click', function() {
                const isConfirmed = confirm('Czy na pewno chcesz odpalić zapalnik?');
                if (isConfirmed) {
                    const xhr = new XMLHttpRequest();
                    xhr.open('POST', '/fire', true);
                    xhr.send();
                }
            });

            function refreshCont() {
                fetch('/continuity')
                .then(response => response.text())
                .then(data => {
                    continuityText.className = '';
                    if (data.includes('1')) {
                        continuityText.classList.add('en');
                    } else if (data.includes('0')) {
                        continuityText.classList.add('dis');
                    }
                });
            }

            setInterval(refreshCont, 1000);
        </script>
    </body>
    </html>
)rawliteral";

WiFiServer server(80);
String header;

void setup() {

    Serial.begin(115200);
    Serial.setTimeout(40);
    pinMode(MOSFET_PIN, OUTPUT);
    pinMode(CONTIN_PIN, INPUT_PULLUP);

    WiFi.softAP(ssid, password);

    server.begin();

    vTaskDelay(1000 / portTICK_PERIOD_MS);
}

void loop() {

    WiFiClient client = server.available();

    if (client) {

        String currentLine = "";
        while (client.connected()) {
            if (client.available()) {
                char c = client.read();
                header += c;
                if (c == '\n') {

                if (currentLine.length() == 0) {

                    // HTTP response header:
                    client.println("HTTP/1.1 200 OK");
                    client.println("Content-type:text/html");
                    client.println("Connection: close");
                    client.println();

                    // POST command to fire the igniter:
                    if (header.indexOf("POST /fire") >= 0) {
                        digitalWrite(BUZZER_PIN, 1);
                        vTaskDelay(3000 / portTICK_PERIOD_MS);
                        digitalWrite(BUZZER_PIN, 0);
                        digitalWrite(MOSFET_PIN, 1);
                        vTaskDelay(2000 / portTICK_PERIOD_MS);
                        digitalWrite(MOSFET_PIN, 0);
                        client.println("Odpalono");
                    }

                    // GET command to check the igniter wire continuity:
                    else if (header.indexOf("GET /continuity") >= 0) {
                        client.println(!digitalRead(CONTIN_PIN));
                    }

                    // GET command to download main web page:
                    else if (header.indexOf("GET /") >= 0) {
                        client.println(mainPage);
                    }

                    client.println();
                    break;
                }
                else {
                    currentLine = "";
                }
                } else if (c != '\r') {
                    currentLine += c;
                }
            }
        }
        header = "";
        client.stop();
    }
}
