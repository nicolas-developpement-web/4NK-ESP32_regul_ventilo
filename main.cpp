#include <OneWire.h>
#include <DallasTemperature.h>
#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>
#include <EEPROM_Rotate.h> //https://github.com/xoseperez/eeprom_rotate

// EEPROM CONSTRUCT //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
EEPROM_Rotate EEPROMr;
#define EEPROM_MODE 10
#define EEPROM_THERMOSTAT 14

// Capteur temperature ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#define ONE_WIRE_BUS 12 // DS18b20 is connected to GPIO 12
float Offset = -1.5;    // Offset de la temperature/qualite de sonde
OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature sensors(&oneWire);

// WIFI ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#ifndef APSSID
#define APSSID "4NK XXX"
#define APPSK "XXXXXXXXX"
#endif
const char *ssid = APSSID;
const char *password = APPSK;

// GPIO ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
const int gpioVentilo = 14; // GPIO Mosfet Ventilo
const int gpioLedRouge = 4; // GPIO LED ROUGE
const int gpioLedVerte = 5; // GPIO LED VERTE

// VARIABLES GLOBALES  ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
unsigned int mode;       // MODE 0 = MANUEL, MODE 1 = CHAUDIERE, MODE 2 = CLIMATISEUR
unsigned int thermostat; // Valeur du thermostat
unsigned int pwmVal = 0; // Valeur par defaut du ventilateur
int courbePWM = 4;       // plus le chiffre est haut plus l'acceleration est brutale à régler en fonction de la source de chaleur
int deltaPWM = 3;        // Offset de calcule de la courbe PWM
float tempSonde;         // Temperature de la sonde

// SERVEUR WEB /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
ESP8266WebServer server(80); // Ecoute du serveur

// REQUETE DEPUIS JAVASCRIPT ///////////////////////////////////////////////////////////////////////////////////////////////////////////////
void handleSonde()
{
    Serial.print("Temperature web: ");
    Serial.print(tempSonde);
    Serial.println("°C");
    server.send(200, "text/plain", String(tempSonde)); // On retourne la temperature
}

void handleConfigMode()
{
    String modeValueStr = server.arg("mode");
    mode = modeValueStr.toInt();
    analogWrite(gpioVentilo, 0); // Reset PWM au changement de mode
    led(mode);
    Serial.print("Mode web: ");
    Serial.println(mode);
    // EEPROM ENREGISTREMENT MODE ////////////
    EEPROMr.write(EEPROM_MODE, mode);
    Serial.printf("Commit %s\n", EEPROMr.commit() ? "MODE enregistré" : "Erreur sauvegarde MODE");
}

/**
 * Allumage des leds en fonction du nombre
 * 0 = eteint
 * 1 = rouge
 * 2 = vert
 */
void led(uint8_t e)
{
    if (e == 0)
    {
        digitalWrite(gpioLedRouge, LOW);
        digitalWrite(gpioLedVerte, LOW);
    }
    if (e == 1)
    {
        digitalWrite(gpioLedRouge, HIGH);
        digitalWrite(gpioLedVerte, LOW);
    }
    if (e == 2)
    {
        digitalWrite(gpioLedRouge, LOW);
        digitalWrite(gpioLedVerte, HIGH);
    }
}

void handleConfigPWM()
{
    String pwmValueStr = server.arg("PWM");
    pwmVal = pwmValueStr.toInt();
    if (mode == 0 && pwmVal >= 0 && pwmVal <= 255)
    {
        if (pwmVal < 25)
        {
            analogWrite(gpioVentilo, 0); // Trop bas: on coupe
            Serial.print("PWM trop bas web: ");
            Serial.println(pwmVal);
        }
        else if (pwmVal <= 40)
        {
            analogWrite(gpioVentilo, 60); // Un petit boost
            delay(500);
            analogWrite(gpioVentilo, pwmVal); // On lance le ventilo à la vitesse souhaitée
            Serial.print("PWM starter web: ");
            Serial.println(pwmVal);
        }
        else
        {
            analogWrite(gpioVentilo, pwmVal); // On lance le ventilo à la vitesse souhaitée
            Serial.print("PWM web: ");
            Serial.println(pwmVal);
        }
    }
}

void handleConfigThermostat()
{
    String thermoValueStr = server.arg("thermostat");
    thermostat = thermoValueStr.toInt();
    analogWrite(gpioVentilo, 0); // Reset PWM au changement de valeur thermostat
    Serial.print("Thermostat web: ");
    Serial.println(thermostat);
    // EEPROM ENREGISTREMENT ////////////
    EEPROMr.write(EEPROM_THERMOSTAT, thermostat);
    Serial.printf("Commit %s\n", EEPROMr.commit() ? "THERMOSTAT enregistré" : "Erreur sauvegarde THERMOSTAT");
}

// PAGE HTML //////////////////////////////////////////////////////////////////////////////////////////////
void handleRoot()
{
    String page = "<!DOCTYPE HTML><html><head><title>4NK Ventilateur</title><meta name='viewport' content='width=device-width,initial-scale=1,user-scalable=no'><style>html{font-size:2.5vw;height:100%;width:100%;font-family:Arial;margin:auto;text-align:center;background:linear-gradient(to bottom right,#fff,70%,grey)}body{height:100%}h1{font-size:3em}.btn button{font-size:1.5em;min-width:30%;margin:.5%;padding:20px 5px 20px 5px;display:inline-block;background:#3498db;background-image:-webkit-linear-gradient(top,#3498db,#2980b9);background-image:-moz-linear-gradient(top,#3498db,#2980b9);background-image:-ms-linear-gradient(top,#3498db,#2980b9);background-image:-o-linear-gradient(top,#3498db,#2980b9);background-image:linear-gradient(to bottom,#3498db,#2980b9);-webkit-border-radius:8;-moz-border-radius:8;border-radius:8px;-webkit-box-shadow:0 1px 3px #666;-moz-box-shadow:0 1px 3px #666;box-shadow:0 1px 3px #666;font-family:Arial;color:#000;text-decoration:none;cursor:pointer}.btn button:hover{background:#6ec4fa;background-image:-webkit-linear-gradient(top,#6ec4fa,#3498db);background-image:-moz-linear-gradient(top,#6ec4fa,#3498db);background-image:-ms-linear-gradient(top,#6ec4fa,#3498db);background-image:-o-linear-gradient(top,#6ec4fa,#3498db);background-image:linear-gradient(to bottom,#6ec4fa,#3498db);text-decoration:none}.btn .selected{background:grey;pointer-events:none}#footer{text-align:left;font-size:.5em;padding:20px;position:absolute;bottom:0;left:0;right:0}#pwm{width:85%;margin:20px}.thermostat{margin-top:50px;font-weight:700;font-size:7rem}.thermostat button{margin:10px;height:80px;width:80px;background:#72a8cc;background-image:-webkit-linear-gradient(top,#72a8cc,#707d85);background-image:-moz-linear-gradient(top,#72a8cc,#707d85);background-image:-ms-linear-gradient(top,#72a8cc,#707d85);background-image:-o-linear-gradient(top,#72a8cc,#707d85);background-image:linear-gradient(to bottom,#72a8cc,#707d85);-webkit-border-radius:8;-moz-border-radius:8;border-radius:8px;font-family:Arial;color:#000;font-size:70px;text-decoration:none;text-align:center}.thermostat button:hover{background:#e3e3e3;background-image:-webkit-linear-gradient(top,#e3e3e3,#c9c9c9);background-image:-moz-linear-gradient(top,#e3e3e3,#c9c9c9);background-image:-ms-linear-gradient(top,#e3e3e3,#c9c9c9);background-image:-o-linear-gradient(top,#e3e3e3,#c9c9c9);background-image:linear-gradient(to bottom,#e3e3e3,#c9c9c9);text-decoration:none}</style><script>function updateTemperature(){var e=new XMLHttpRequest;e.onreadystatechange=function(){4==this.readyState&&200==this.status&&(document.getElementById('temperature').innerText=this.responseText)},e.open('GET','/temperature',!0),e.send()}function sendPWM(){var e=document.getElementById('pwm').value,t=new XMLHttpRequest;t.open('GET','/PWM?PWM='+e,!0),t.send(),document.getElementById('pwmVal').innerText=Math.round(e/2.55)}function reciveThermostat(){}function sendThermostat(e){var t=document.getElementById('sendThermostat').innerText,n=document.getElementById('sendThermostat').innerText=parseInt(t)+e,d=new XMLHttpRequest;d.open('GET','/thermostat?thermostat='+n,!0),d.send()}function selected(e){var t=new XMLHttpRequest;t.open('GET','/mode?mode='+e,!0),t.send();var n=document.querySelector('html'),d=document.getElementById(0),s=document.getElementById(1),r=document.getElementById('button0'),o=document.getElementById('button1'),i=document.getElementById('button2');0==e&&(n.style.backgroundImage='linear-gradient(to bottom right, white, 70%, green',r.classList.add('selected'),o.classList.remove('selected'),i.classList.remove('selected'),d.removeAttribute('hidden'),s.setAttribute('hidden','hidden')),1==e&&(n.style.backgroundImage='linear-gradient(to bottom right, white, 70%, red',o.classList.add('selected'),r.classList.remove('selected'),i.classList.remove('selected'),s.removeAttribute('hidden'),d.setAttribute('hidden','hidden')),2==e&&(n.style.backgroundImage='linear-gradient(to bottom right, white, 70%, blue',i.classList.add('selected'),r.classList.remove('selected'),o.classList.remove('selected'),s.removeAttribute('hidden'),d.setAttribute('hidden','hidden'))}setInterval(updateTemperature,2e3)</script></head><body onload='updateTemperature()'><h1>4NK Ventilo-1</h1><h1><span id='temperature'>--</span>&deg;C</h1><div class='btn'><button id='button0' onclick='selected(0)'>MANUEL</button><button id='button1' onclick='selected(1)'>CHAUDIERE</button><button id='button2' onclick='selected(2)'>CLIMATISEUR</button></div><div id='0' hidden='hidden'><div class='PWM'><h1><label for='pwm'>Vitesse:&nbsp;<span id='pwmVal'>--</span>&#37;</label></h1><input type='range' id='pwm' name='pwm' min='0' max='255' step='5' onchange='sendPWM()' value='" + String(pwmVal) + "'/></div></div><div id='1' hidden='hidden'><div class='thermostat'><button classe='selected' onclick='sendThermostat(-1)'>-</button><span id='sendThermostat'>" + String(thermostat) + "</span><button onclick='sendThermostat(1)'>+</button></div></div><footer id='footer'>Nicol<a href='mailto:nicolas.developpement.web@gmail.com?subject=Dev ESP32'>&#64;</a>s76 pour 4NK</footer></body></html>";
    server.send(200, "text/html", page);
}

void setup()
{
    Serial.begin(115200);

    // LED /////////////////////////////////////////////////////////////////////////////////////////////////
    pinMode(gpioVentilo, OUTPUT);
    digitalWrite(gpioVentilo, LOW);
    pinMode(gpioLedRouge, OUTPUT);
    digitalWrite(gpioLedRouge, LOW);
    pinMode(gpioLedVerte, OUTPUT);
    digitalWrite(gpioLedVerte, LOW);

    // WIFI /////////////////////////////////////////////////////////////////////////////////////////////////
    delay(3000);
    Serial.println();
    Serial.println("Configuration AP Wifi [ " + String(APSSID) + " ]");
    Serial.println("Mot de passe: " + String(APPSK));
    WiFi.softAP(ssid, password); // You can remove the password parameter if you want the AP to be open.
    IPAddress myIP = WiFi.softAPIP();
    Serial.print("AP IP address: http://");
    Serial.println(myIP);

    // PAGES HTML ////////////////////////////////////////////////////////////////////////////////////////////
    server.on("/", handleRoot);                       // page de base
    server.on("/temperature", handleSonde);           // Reception requete javascript
    server.on("/thermostat", handleConfigThermostat); // Reception requete javascript
    server.on("/mode", handleConfigMode);             // Reception requete javascript
    server.on("/PWM", handleConfigPWM);               // Reception requete javascript
    server.begin();
    Serial.println("HTTP server started");

    // INFOS EEPROM //////////////////////////////////////////////////////////////////////////////////////////
    uint8_t size = 0;
    if (EEPROMr.last() > 1000)
    { // 4MiB boards
        size = 4;
    }
    else if (EEPROMr.last() > 250)
    { // 1MiB boards
        size = 2;
    }
    else
    {
        size = 1;
    }
    EEPROMr.size(size);
    EEPROMr.begin(4096);
    mode = EEPROMr.read(EEPROM_MODE);
    led(mode);
    thermostat = EEPROMr.read(EEPROM_THERMOSTAT);
    if (thermostat == 255)
    {
        thermostat = 19; // Si nouveau ESP32 19°c par defaut
    }

    // ERASE EEPROM !!! //
    // Desactiver la lecture des données: mode = EEPROMr.read(EEPROM_MODE);
    // EEPROMr.dump(Serial, EEPROM_THERMOSTAT); // changer en fonction de ce que l'on souhaite effacer

    // SENSOR ACTIVATION /////////////////////////////////////////////////////////////////////////////////////
    sensors.begin(); // Initialize DS18B20 sensor
}

void loop()
{
    server.handleClient();         // Envoi vers le client http
    sensors.requestTemperatures(); // Request temperature from DS18B20 sensor
    tempSonde = sensors.getTempCByIndex(0) + Offset;
    if (tempSonde != DEVICE_DISCONNECTED_C)
    {
        if (mode == 1) // MODE CHAUDIERE
        {
            if (tempSonde < thermostat)
            {
                float delta = thermostat - tempSonde;
                if (delta > 0)
                {
                    int pwm = pow(delta + deltaPWM, courbePWM); // OFFSET à regler avec la courbePWM et deltaPWM
                    if (pwm > 255)                              // Bornage impultions du ventilo
                    {
                        pwm = 255;
                    }
                    if (pwm < 30)
                    {
                        pwm = 0;
                    }
                    analogWrite(gpioVentilo, pwm);
                    Serial.print("PWM auto M 1: ");
                    Serial.println(pwm);
                    Serial.print("Sonde: ");
                    Serial.print(tempSonde);
                    Serial.println("°C");
                    Serial.print(pwm);
                    Serial.print(",");
                    Serial.print(tempSonde);
                    Serial.print(",");
                    Serial.println(delta);
                }
            }
        }
        else if (mode == 2) // MODE CLIMATISEUR
        {
            if (tempSonde > thermostat)
            {
                float delta = tempSonde - thermostat;
                if (delta > 0)
                {
                    int pwm = pow(delta + deltaPWM, courbePWM); // OFFSET à regler avec la courbePWM et deltaPWM
                    if (pwm > 255)                              // Bornage impultions du ventilo
                    {
                        pwm = 255;
                    }
                    if (pwm < 30)
                    {
                        pwm = 0;
                    }
                    analogWrite(gpioVentilo, pwm);
                    Serial.print("PWM auto M 2: ");
                    Serial.println(pwm);
                    Serial.print("Sonde: ");
                    Serial.print(tempSonde);
                    Serial.println("°C");
                    Serial.print(pwm);
                    Serial.print(",");
                    Serial.print(tempSonde);
                    Serial.print(",");
                    Serial.println(delta);
                }
            }
        }
        else if (mode == 0) // MODE MANUEL
        {
            Serial.println("M0 Sonde: ");
            Serial.println(tempSonde);
        }
    }
    else
    {
        Serial.println("Error: Could not read temperature data");
    }
}
