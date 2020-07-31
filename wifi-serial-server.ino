#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>
#include <ArduinoJson.h>
#include <DNSServer.h>
#include <ESP8266WebServerSecure.h>
#define ESP8266
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

const String postForms = "<html>\
  <head>\
    <title>ESP8266 Web Server</title>\
    <style>\
      body { background-color: #cccccc; font-family: Arial, Helvetica, Sans-Serif; Color: #000088; }\
    </style>\
  </head>\
  <body>\
    <h1>Hello from ESP8266WebServerSecure!</h1><br>\
  </body>\
</html>";

#define SEND_SERIAL_TIME (75)
#define ONBOARD_LED (2)
#define ONBOARD_D6 (12)

enum SerialResult
{
    SR_SUCCESS,
    SR_TIMEOUT,
    SR_BUSY,
    SR_CRC,
    SR_ERROR,
    SR_UNSUPPORTED_MODEL
};

class SerialTerminal
{
public:
    void setup()
    {
        _lastRX = 0;
        resetBuffer();
        Serial.begin(115200, SERIAL_8N1);
    }

    void writeHex(const char *line)
    {
        unsigned int byteint;
        size_t index;
        size_t str_len = strlen(line);
        uint8_t bytearray[str_len / 2];

        // read
        for (index = 0; index < (str_len / 2); index++)
        {
            sscanf(line + 2 * index, "%02x", &byteint);
            bytearray[index] = (uint8_t)byteint;
        }
        Serial.write(&bytearray[0], sizeof(bytearray));
    }

    void readHex(char *output, size_t size)
    {
        size_t len = min(length(), size / 2);
        uint8_t *bytearray = buffer();
        for (int index = 0; index < len; index++)
        {
            sprintf(output + 2 * index, "%02X", (unsigned int)bytearray[index]);
        }
    }

    void handle()
    {
        unsigned long t = millis();
        bool forceSend = false;
        bool timeout = false;

        resetBuffer();

        while (!(timeout || forceSend))
        {

            size_t len = (_bufferWritePtr - &_buffer[0]);
            int free = (sizeof(_buffer) - len);

            int available = Serial.available();
            if (available > 0 && free > 0)
            {
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

    size_t length()
    {
        return (_bufferWritePtr - &_buffer[0]);
    }

    SerialResult getResult(const char *model)
    {
        if (strcmp(model, "eltrade") == 0)
        {
            return getResultEltrade();
        }
        return SR_UNSUPPORTED_MODEL;
    }

    SerialResult getResultEltrade()
    {
        if (length() == 0)
            // timeout.
            return SR_TIMEOUT;

        String s = "";
        s.concat((char *)buffer(), length());

        if ((s.length() == 1) && (s[0] == 0x15))
            // checksum.
            return SR_CRC;

        if (s.lastIndexOf(0x16) >= 0)
        {
            s = s.substring(s.lastIndexOf(0x16) + 1);
            if (s.length() == 0)
            {
                // busy
                return SR_BUSY;
            }
        }

        String errors = s.substring(s.indexOf(4) + 1, s.indexOf(5));

        if ((errors[0] & 0b00100000) > 1)
            return SR_ERROR;

        if ((errors[4] & 0b00100000) > 1)
            return SR_ERROR;

        return SR_SUCCESS;
    }

protected:
    uint8_t _buffer[2048];
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

void sendError(const char *message, size_t index, const char *input, bool sendOutput)
{
    size_t out_length = term.length() * 2;
    char output[out_length + 1];
    char buffer[100 + strlen(message) + strlen(input) + out_length];
    term.readHex(&output[0], out_length);
    snprintf_P(
        buffer,
        sizeof(buffer),
        PSTR("{\n\"result\": \"error\", \n\"error\": \"%s\", \n\"index\": \"%d\", \n\"input\": \"%s\", \n\"output\": \"%s\"\n}"),
        message,
        index,
        input,
        term.length() > 0 ? output : "");
    server.send(400, "application/json", buffer);
}

void handleSerial()
{
    digitalWrite(ONBOARD_LED, LOW);

    const size_t capacity = JSON_OBJECT_SIZE(2) + JSON_ARRAY_SIZE(400) + 60;
    DynamicJsonDocument doc(capacity);
    DeserializationError error = deserializeJson(doc, server.arg("plain"));

    if (error)
    {
        sendError(error.c_str(), 0, "", false);
        return;
    }

    const char *model = doc["model"];
    JsonArray array = doc["commands"];
    SerialResult result = SR_SUCCESS;

    for (int index = 0; index < array.size(); index++)
    {
        uint8_t loop = 0;
        const char *line = array[index];

        term.writeHex(line);
        term.handle();
        result = term.getResult(model);

        if (result == SR_TIMEOUT)
        {
            sendError("timeout", index, line, true);
            break;
        }

        if (result == SR_CRC)
        {
            sendError("crc", index, line, true);
            break;
        }

        // loop for busy
        while (result == SR_BUSY && (loop++ < 3))
        {
            term.handle();
            result = term.getResult(model);
        }

        if (result == SR_BUSY)
        {
            sendError("busy", index, line, true);
            break;
        }

        if (result == SR_ERROR)
        {
            sendError("general", index, line, true);
            break;
        }

        if (result == SR_UNSUPPORTED_MODEL)
        {
            sendError("unsupported", index, line, true);
            break;
        }
    }
    if (result == SR_SUCCESS)
    {
        server.send(200, "application/json", "{\"result\": \"OK\"}");
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

    //set custom ip for portal
    wifiManager.setSTAStaticIPConfig(IPAddress(192, 168, 1, 113), IPAddress(192, 168, 1, 1), IPAddress(255, 255, 255, 0));

    wifiManager.autoConnect("ESP-FPLink");
    configTime(3 * 3600, 0, "pool.ntp.org", "time.nist.gov");

    server.getServer().setRSACert(new BearSSL::X509List(serverCert), new BearSSL::PrivateKey(serverKey));

    server.on("/", []() {
        server.send(200, "text/html", postForms);
    });

    server.on("/wifireset", [&wifiManager]() {
        wifiManager.resetSettings();
        server.send(200, "text/plain", "ok, restart");
        ESP.restart();
    });

    server.on("/serial", HTTP_POST, handleSerial);
    server.on("/serial", HTTP_OPTIONS, []() {
        server.sendHeader("Access-Control-Allow-Origin", "*");
        server.sendHeader("Access-Control-Allow-Methods", "*");
        server.sendHeader("Access-Control-Allow-Headers", "*");
        server.sendHeader("Access-Control-Max-Age", "6000");
        server.send(204);
    });

    server.onNotFound([]() {
        server.send(404, "text/plain", "not found.");
    });
    server.begin();
}

void loop()
{
    server.handleClient();
}