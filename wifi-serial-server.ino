#include <Arduino.h>
#include <ESP8266WiFi.h> //https://github.com/esp8266/Arduino

//needed for library
#include <DNSServer.h>
#include <ESP8266WebServer.h>
#include <ESP8266WebServerSecure.h>
#include <WiFiManager.h> //https://github.com/tzapu/WiFiManager

BearSSL::ESP8266WebServerSecure server(443);

static const char serverCert[] PROGMEM = R"EOF(
-----BEGIN CERTIFICATE-----
MIICcDCCAdmgAwIBAgIUbhp9FWJ87ooLd9H+O3ni9fGkgQMwDQYJKoZIhvcNAQEL
BQAwYzELMAkGA1UEBhMCWFgxDDAKBgNVBAgMA04vQTEMMAoGA1UEBwwDTi9BMSAw
HgYDVQQKDBdTZWxmLXNpZ25lZCBjZXJ0aWZpY2F0ZTEWMBQGA1UEAwwNMTkyLjE2
OC4xLjExMzAeFw0yMDA2MjgwMDEwMDNaFw00NzExMTMwMDEwMDNaMGMxCzAJBgNV
BAYTAlhYMQwwCgYDVQQIDANOL0ExDDAKBgNVBAcMA04vQTEgMB4GA1UECgwXU2Vs
Zi1zaWduZWQgY2VydGlmaWNhdGUxFjAUBgNVBAMMDTE5Mi4xNjguMS4xMTMwgZ8w
DQYJKoZIhvcNAQEBBQADgY0AMIGJAoGBANTjYpQW+g0n61xiMsbQ4y5tIOtOW6t2
DV+dsZ43OodzUXtCpJQFtasX/p9fOP2DV9s5hziAQ4qN2qM1Ofm815yVLKp3Jhkn
/ogcmRcvw0ctvYrxpeR7Ayeq3dU1NoyZ5WETDSDN7iPsIjNBMKDkSfwcB6zLP+hy
oieJ7uWIKaEHAgMBAAGjITAfMAwGA1UdEwQFMAMBAf8wDwYDVR0RBAgwBocEwKgB
cTANBgkqhkiG9w0BAQsFAAOBgQCkbxJiMk3y1nEAv87jdjUqaR+GgJMUMhyHGlCE
LfLpdZmztUw27ijoH672FD1+c5zyJZURAUgXcVWAjpWhGxVN+9cZLVnl9q2I9n7s
cq+5Rwwa7HHwteTCCjofDnRNInEqVNl7jpJ3UgfszMaxh8FQEfX8rueONqgA7D5K
Abhe1Q==
-----END CERTIFICATE-----
)EOF";

static const char serverKey[] PROGMEM = R"EOF(
-----BEGIN RSA PRIVATE KEY-----
MIICXwIBAAKBgQDU42KUFvoNJ+tcYjLG0OMubSDrTlurdg1fnbGeNzqHc1F7QqSU
BbWrF/6fXzj9g1fbOYc4gEOKjdqjNTn5vNeclSyqdyYZJ/6IHJkXL8NHLb2K8aXk
ewMnqt3VNTaMmeVhEw0gze4j7CIzQTCg5En8HAesyz/ocqInie7liCmhBwIDAQAB
AoGBAMoAwQdYDgjxugi9PQUeLJNfBX+BqMY3jFUQIvvH2Aeyyrw07kluaYVhjT/Y
cRFM0c649bANNQmBtNZsqQhpwgOdCvKljpa9uGhL2bBJIUMc3jCrrgTjd9w4nKN1
oY6T/iqpzUsUv9JUyLn7BWa71DXLE/xy/14jvywWgd2o1PUhAkEA7QEKNRNg0cyP
1BEWDSNVc14EoJwXELxC975EPki/E0r2wqp7Hq78GLsDDcprkjKfogdMwLmSzmpd
Wzy25Q3NXwJBAOXzhm+qFtihtGxBgoLPDffJ5tIC18ZbFTVv/U5plzIXxhdpH4vh
dJdYNWndkYEsifw83AG/rNx0Y1UhKtBhpVkCQQC8v2RCqsEjtHcjG8xlACvQaiLj
Sgwwfs4SgYvV0pehpMTqeVz+LbuFcoJXHEsZLonlP00H+4KIMztQKwU5XAmfAkEA
oFBJN2xDhUAnQxng3UVxHYFbNKrat+UkQh8TYClpSXkdl5Cod6L039aVVnssR/w/
LuVXFLkG0KMr5Vu8N4778QJBAKWAndJHUt4+FDoH7g7DS2VVhFNF/ohPjE4OcYNg
iqOqOppWNgg4HJVSrjdFWlVJku+00cmaWdUv2LT03M34W2M=
-----END RSA PRIVATE KEY-----
)EOF";

#define SEND_SERIAL_TIME (250)
#define ONBOARD_LED (2)
#define ONBOARD_D6 (12)

class SerialTerminal
{
public:
    void setup()
    {
        _lastRX = 0;
        resetBuffer();
        Serial.begin(115200, SERIAL_8N1);
    }

    void write(const String &data)
    {
        Serial.write(data.c_str(), data.length());
        resetBuffer();
    }

    void handle()
    {
        unsigned long t = millis();
        bool forceSend = false;
        bool timeout = false;
        while (!(timeout || forceSend))
        {

            size_t len = (_bufferWritePtr - &_buffer[0]);
            int free = (sizeof(_buffer) - len);

            int available = Serial.available();
            if (available > 0 && free > 0)
            {
                // Serial.write("\nRead bytes...");
                int readBytes = available;
                if (readBytes > free)
                {
                    readBytes = free;
                }
                readBytes = Serial.readBytes(_bufferWritePtr, readBytes);
                _bufferWritePtr += readBytes;
                t = millis();
            }

            // check for data in buffer
            len = (_bufferWritePtr - &_buffer[0]);
            if (len >= sizeof(_buffer))
            {
                forceSend = true;
            }
            if ((millis() - t) > SEND_SERIAL_TIME)
            {
                timeout = true;
            }
        }
    }

    uint8_t *buffer()
    {
        return &_buffer[0];
    }

    size_t lenght()
    {
        return (_bufferWritePtr - &_buffer[0]);
    }

protected:
    uint8_t _buffer[2058];
    uint8_t *_bufferWritePtr;
    unsigned long _lastRX;

    void resetBuffer()
    {
        _bufferWritePtr = &_buffer[0];
    }

    inline void addChar(char c)
    {
        *_bufferWritePtr = (uint8_t)c; // message type for Webinterface
        _bufferWritePtr++;
    }
};

SerialTerminal term;

void handleRoot()
{
    digitalWrite(ONBOARD_LED, LOW);
    server.send(200, "text/plain", "Hello from esp8266 over HTTPS!");
    digitalWrite(ONBOARD_LED, HIGH);
}

void handleNotFound()
{
    digitalWrite(ONBOARD_LED, LOW);
    String message = "File Not Found\n\n";
    message += "URI: ";
    message += server.uri();
    message += "\nMethod: ";
    message += (server.method() == HTTP_GET) ? "GET" : "POST";
    message += "\nArguments: ";
    message += server.args();
    message += "\n";
    for (uint8_t i = 0; i < server.args(); i++)
    {
        message += server.argName(i) + ": " + server.arg(i) + "\n";
    }

    server.send(404, "text/plain", message);
    digitalWrite(ONBOARD_LED, HIGH);
}

void handleSerial()
{
    digitalWrite(ONBOARD_LED, LOW);
    String body = server.arg("plain");
    term.write(body);
    term.handle();
    if (term.lenght() > 0)
    {
        // Serial.write("\nHas data");
        server.send(200, "application/octet-stream", term.buffer(), term.lenght());
    }
    else
    {
        // Serial.write("\nTimeout.");
        server.send(204, "application/octet-stream", "Timeout.");
    }
    digitalWrite(ONBOARD_LED, HIGH);
}

void setup()
{
    pinMode(ONBOARD_LED, OUTPUT);
    pinMode(ONBOARD_D6, INPUT_PULLUP);
    digitalWrite(ONBOARD_LED, HIGH);
    term.setup();

    WiFiManager wifiManager;
    wifiManager.setDebugOutput(false);

    //reset saved settings
    // wifiManager.resetSettings();

    //set custom ip for portal
    //wifiManager.setAPStaticIPConfig(IPAddress(10,0,1,1), IPAddress(10,0,1,1), IPAddress(255,255,255,0));
    wifiManager.setSTAStaticIPConfig(IPAddress(192, 168, 1, 113), IPAddress(192, 168, 1, 1), IPAddress(255, 255, 255, 0));

    wifiManager.autoConnect("ESP-FPLink");
    configTime(3 * 3600, 0, "pool.ntp.org", "time.nist.gov");

    server.getServer().setRSACert(new BearSSL::X509List(serverCert), new BearSSL::PrivateKey(serverKey));
    server.on("/", handleRoot);
    server.on("/serial", HTTP_POST, handleSerial);
    server.on("/inline", []() {
        server.send(200, "text/plain", "this works as well");
    });
    server.on("/resetwifi", [&wifiManager]() {
        wifiManager.resetSettings();
        server.send(200, "text/plain", "ok, restart");
        ESP.restart();
    });
    server.onNotFound(handleNotFound);
    server.begin();
}

void loop()
{
    server.handleClient();
}