#include <Arduino.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <SPI.h>

//Define 7-Segment Constants & functions
const byte numsec[10] = {0b11101011, 0b10000001, 0b01110011, 0b11010011, 0b10011001, 
                         0b11011010, 0b11111010, 0b10000011, 0b11111011, 0b11011011};
static const int spiClk = 1000000; // 1 MHz
SPIClass *vspi = NULL;
void sent56(int in5, int in6);
void sent11sec(byte data[]);
void vspiCommand(byte data);
//End defining 7-Segment Constants & functions

//Define FIBO Super Secret Wifi Password
const char *ssid = "FIBOWIFI";
const char *password = "Fibo@2538";
//End defining FIBO Super Secret Wifi Password

//Parameters
double daily = 0; //Daily money usage
double monthly = 0; //Monthly money usage
const int yellowLED = 16; //Yellow LED
const int redLED = 4;  //Red LED
const int greenLED = 0;  //Green LED

void setup()
{
    //Initialize SPI class for 7-Segment Display
    vspi = new SPIClass(VSPI);
    vspi->begin();
    pinMode(1, OUTPUT);  //RCLK
    pinMode(22, OUTPUT); //OE
    pinMode(17, OUTPUT); //CRL
    sent56(88888, 888888); //Check every digit by showing 88888 and 888888 respectively
    
    //Set LED Pin Mode
    pinMode(yellowLED, OUTPUT);
    pinMode(redLED, OUTPUT);
    pinMode(greenLED, OUTPUT);
    
    //Initial Serial Port for debugging
    Serial.begin(9600);

    //Connect to WIFI
    WiFi.begin(ssid, password);
    while (WiFi.status() != WL_CONNECTED)
    {
        delay(1000);
        Serial.println("Connecting to WiFi..");
    }

    //Response to serial port
    delay(1000);
    Serial.println("Connected to the WiFi network");
}

void loop()
{
    if ((WiFi.status() == WL_CONNECTED))
    {
        //Task 1: Retrieve latest MATLAB values
        HTTPClient http;
        http.begin("https://api.thingspeak.com/channels/581739/feeds.json?results=1"); //Specify the URL
        int httpCode = http.GET(); //Make the request
        if (httpCode > 0)
        { 
            String payload = http.getString();
            StaticJsonBuffer<1024> jsonBuffer;
            JsonObject &root = jsonBuffer.parseObject(payload);
            daily = root["feeds"][0]["field1"];
            monthly = root["feeds"][0]["field2"];
        } else {
            Serial.println("Error on HTTP request");
        }
        http.end(); //Free up the resources

        //Task 2: Show Money on 7-Segments
        digitalWrite(22, LOW);
        digitalWrite(17, HIGH);
        sent56(daily, monthly);

        //Task 3: Display light according to power consumption
        if (daily < 700 && daily >= 0) {
            digitalWrite(greenLED, 1); digitalWrite(redLED, 0); digitalWrite(yellowLED, 0);
        } else if (daily >= 700 && daily < 1400) {
            digitalWrite(yellowLED, 1); digitalWrite(redLED, 0); digitalWrite(greenLED, 0);
        } else {
            digitalWrite(redLED, 1); digitalWrite(greenLED, 0); digitalWrite(yellowLED, 0);
        }
    }
    delay(20000);
}

void sent56(int in5, int in6)
{
    byte data[11];
    for (int i = 0; i < 5; i++)
    {
        data[i] = in5 % 10;
        in5 = (in5 - data[i]) / 10;
    }
    for (int i = 0; i < 6; i++)
    {
        data[5 + i] = in6 % 10;
        in6 = (in6 - data[5 + i]) / 10;
    }
    sent11sec(data);
}
void sent11sec(byte data[])
{
    for (int i = 10; i >= 0; i--)
    {
        digitalWrite(1, LOW);
        vspiCommand(numsec[data[i]]);
        digitalWrite(1, HIGH);
    }
}
void vspiCommand(byte data)
{

    //use it as you would the regular arduino SPI API
    vspi->beginTransaction(SPISettings(spiClk, MSBFIRST, SPI_MODE0));
    //pull SS slow to prep other end for transfer
    vspi->transfer(data);
    //pull ss high to signify end of data transfer
    vspi->endTransaction();
}