//==========================================================
//  TRAFFIC JAM
//  Commute travel time alarm
//
//  J. van Saders 2016
//==========================================================


// Libraries
#include "neopixel/neopixel.h"


// Definitions
#define MODE_DEBUG 0
#define MODE_RUN 1
#define ONE_DAY_MILLIS (24 * 60 * 60 * 1000)
#define FIFTEEN_MINUTES_MILLIS (15 * 60 * 1000)
#define COMMUTE_AM_START 6
#define COMMUTE_AM_END 10
#define COMMUTE_PM_START 16
#define COMMUTE_PM_END 20
#define COMMUTE_SECONDS_NORMAL (40 * 60)
#define COMMUTE_SECONDS_MODERATE (50 * 60)
#define COMMUTE_SECONDS_HEAVY (60 * 60)
#define TRAFFIC_NORMAL 0
#define TRAFFIC_MODERATE 1
#define TRAFFIC_HEAVY 2
#define LAMPS_OFF 0
#define LAMPS_ON 1

SYSTEM_MODE(AUTOMATIC);
#define PIXEL_PIN D6
#define PIXEL_COUNT 24
#define PIXEL_TYPE WS2812B


// Variables
int appMode = MODE_DEBUG;    //default
unsigned long lastSync = millis();
unsigned long lastCheck = millis();
unsigned int commuteSeconds;
int lampState = LAMPS_OFF;
int lampMode = TRAFFIC_NORMAL;
Adafruit_NeoPixel ring = Adafruit_NeoPixel(PIXEL_COUNT, PIXEL_PIN, PIXEL_TYPE);


//----------------------------------------------------------
//  IsCommuteTime : checks if its commute time and sets
//  commute origin and destination locations
//----------------------------------------------------------
bool isCommuteTime()
{
    // make sure time is local (in case cloud connection drops)
    Time.zone(-4);  // eastern

    bool isTime = false;
    int clkHour = Time.hour();
    bool commuteAM = (clkHour >= COMMUTE_AM_START) && (clkHour <= COMMUTE_AM_END);
    bool commutePM = (clkHour >= COMMUTE_PM_START) && (clkHour <= COMMUTE_PM_END);
    bool weekDay = (Time.weekday() > 1) && (Time.weekday() < 7);


    if (weekDay)
    {
        if ((commuteAM) || (commutePM))
        {
            isTime = true;
        }

    }
    else
    {
        isTime = false;
    }


    //  *** DEBUG MODE ***
    if (appMode == MODE_DEBUG)
    {
        isTime = true;
    }
    //  *** DEBUG MODE ***

    return isTime;
}


//----------------------------------------------------------
//  getTrafficStatus : checks traffic via Google Map API
//  returns 0=normal, 1=moderate, 2=heavy
//----------------------------------------------------------
int getTrafficStatus()
{
    int trafficCode = TRAFFIC_NORMAL;
    int commuteSecondsEstimate = 0;
    TCPClient client;
    String response = "";
    byte server[] = { 255, 255, 255, 255 }; // YOUR SERVER IP ADDRESS


    // make call to Google Maps Direction Matrix API via PHP script on server
    if (client.connect(server, 80))
    {
        Serial.println("connected");

        if (Time.isAM())
        {
            client.println("GET /YOUR_PATH/commuteAM.php HTTP/1.0");
        }
        else
        {
            client.println("GET /YOUR_PATH/commutePM.php HTTP/1.0");
        }

        client.println("Connection: close");
        client.println("Host: YOUR_DOMAIN");
        client.println("Accept: text/html, text/plain");
        client.println("Content-Length: 0");
        client.println();
    }
    else
    {
        //Serial.println("connection failed");
        response = "Content-Type: text/html 0";
    }

    delay(5000);
    while (client.available())
    {
        char c = client.read();
        //Serial.print(c);
        response += c;
    }

    //Serial.println(response);
    //Serial.println();
    //Serial.println("========================");
    int pos = response.indexOf("Content-Type: text/html");
    response = response.substring(pos+24);
    response = response.trim();
    commuteSecondsEstimate = response.toInt();

    if (!client.connected())
    {
        //Serial.println();
        //Serial.println("disconnecting.");
        //Serial.println();
        client.flush();
        client.stop();
    }




    if (commuteSecondsEstimate <= COMMUTE_SECONDS_MODERATE)
    {
        trafficCode = TRAFFIC_NORMAL;
    }

    if (commuteSecondsEstimate > COMMUTE_SECONDS_MODERATE)
    {
        trafficCode = TRAFFIC_MODERATE;
    }

    if (commuteSecondsEstimate > COMMUTE_SECONDS_HEAVY)
    {
        trafficCode = TRAFFIC_HEAVY;
    }


    //  *** DEBUG MODE ***
    if (appMode == MODE_DEBUG)
    {
        //trafficCode = TRAFFIC_NORMAL;
        //trafficCode = TRAFFIC_MODERATE;
        //trafficCode = TRAFFIC_HEAVY;
        Serial.println();
        Serial.println("TrafficCode = " + String(trafficCode));
        Serial.println("Commute seconds estimate = " + String(commuteSecondsEstimate));
    }
    //  *** DEBUG MODE ***

    return trafficCode;
}



//----------------------------------------------------------
//  COS : cosine approximation. Input is 0 to 360 degrees
//----------------------------------------------------------
// Cosine approximation where x is in degrees
double COS(double x)
{
    double Cos = 1;

    double a = 1;
    double b = -0.00012345;  // 1/90^2

    // bound x to (0,360)
    if (x<0) {x+= 360;}
    if (x>=360) {x-= 360;}

    if (x<=90) {Cos = a + b*x*x;}
    if ((x>90)&&(x<=270)) {Cos = -a -b*(x-180)*(x-180);}
    if (x>270) {Cos = a + b*(x-360)*(x-360);}

    return Cos;
}



//----------------------------------------------------------
//  updateLamps : breathe mode animation
//----------------------------------------------------------
void updateLamps()
{
    static int brightness = 0;
    static int cycleIndex = 0;
    static bool breatheIn = true;
    int ledR;
    int ledG;
    int ledB;
    int normalR=0;
    int normalG=255;
    int normalB=25;
    int moderateR=255;
    int moderateG=204;
    int moderateB=0;
    int heavyR=255;
    int heavyG=25;
    int heavyB=0;


    // set brightness level and breathing direction
    // sine wave
    delay(8);
    cycleIndex++;
    if (cycleIndex > 360)
    {
        // reset range
        cycleIndex = 0;
    }

    // don't turn fully off
    brightness = (int)(511.0 + 496.0 * COS(cycleIndex));


    // set color mode
    switch (lampMode)
    {
        case TRAFFIC_NORMAL:
            ledR = normalR;
            ledG = normalG;
            ledB = normalB;
            break;
        case TRAFFIC_MODERATE:
            ledR = moderateR;
            ledG = moderateG;
            ledB = moderateB;
            break;
        case TRAFFIC_HEAVY:
            ledR = heavyR;
            ledG = heavyG;
            ledB = heavyB;
            break;
        default:
            ledR = normalR;
            ledG = normalG;
            ledB = normalB;
    }


    // update NeoPixel colors
    if (lampState == LAMPS_ON)
    {
       // multiply by brightness factor
       ledR = ledR * brightness;
       ledG = ledG * brightness;
       ledB = ledB * brightness;

       // scale by max value of 1024
       ledR = ledR >> 10;
       ledG = ledG >> 10;
       ledB = ledB >> 10;
    }

    if (lampState == LAMPS_OFF)
    {
        ledR = 0;
        ledG = 0;
        ledB = 0;
    }

    // update NeoPixel values
    for (int i=0; i<ring.numPixels(); i++)
    {
        ring.setPixelColor(i, ledR, ledG, ledB);
    }
    ring.show();

    //  *** DEBUG MODE ***
    if (appMode == MODE_DEBUG)
    {
        Serial.println(String(brightness) + "," + String(ledR) + "," + String(ledG) + "," + String(ledB));
    }
    //  *** DEBUG MODE ***
}



//----------------------------------------------------------
//  SETUP
//----------------------------------------------------------
void setup()
{
    // Sync clock
    lastSync = millis();
    lastCheck = millis();
    Particle.syncTime();

    // Set app mode
    appMode = MODE_RUN;
    //appMode = MODE_DEBUG;

    // Set time zone
    Time.zone(-4);  // Eastern

    // Initializations
    if (appMode==MODE_DEBUG)
    {
        // Set up for serial debug
        pinMode(D7,OUTPUT);         // Turn on the D7 led so we know it's time
        digitalWrite(D7,HIGH);      // to open the Serial Terminal.
        Serial.begin(9600);         // Open serial over USB.
        while(!Serial.available())  // Wait here until the user presses ENTER
            Particle.process();     // in the Serial Terminal. Call the BG Tasks
                                    // while we are hanging around doing nothing.

        digitalWrite(D7,LOW);       // Turn off the D7 led ... your serial is serializing!
        Serial.println("Serial connected");
    }
    else
    {
        // Set up for normal operation
    }

}


//----------------------------------------------------------
//  LOOP
//----------------------------------------------------------
void loop()
{
    // Check intervals
    if (millis() - lastSync > ONE_DAY_MILLIS)
    {
        // time to resync clock from cloud
        Particle.syncTime();
        lastSync = millis();
        lastCheck=millis();
    }
    if (millis()-lastCheck > FIFTEEN_MINUTES_MILLIS)
    {
        // reset timestamp
        lastCheck=millis();

        // time to check on commute
        if (isCommuteTime())
        {
            // get traffic status and set lamp mode
            lampMode = getTrafficStatus();
            lampState = LAMPS_ON;

        }
        else
        {
            // turn lamps off outside rush hours
            lampState = LAMPS_OFF;
        }

    }

    // update lamp animation if they are 'ON'
    updateLamps();


}
