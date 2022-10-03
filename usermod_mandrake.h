#pragma once

#include "wled.h"
#include "SoftwareSerial.h"
#include "DFRobotDFPlayerMini.h"

constexpr const char* MAGNET_CLOSE_KEY = "Play Music if Magnet is Close";
constexpr const char* HALL_SENSOR_KEY = "Hall Sensor Pin";

constexpr const char* MP3_OUTPUT_PIN_KEY = "MP3 Output Pin";
constexpr const char* MP3_INPUT_PIN_KEY = "MP3 Input Pin";


class MandrakeUsermod : public Usermod {
  private:
    // sample usermod default value for variable (you can also use constructor)
    uint8_t mp3OutputPin = 4;
    uint8_t mp3InputPin = 5;
    uint8_t hallSensorPin = 13;

    bool isPlaying = false;
    bool playMusicIfMagnetIsClose = true;
    //Private class members. You can declare variables and functions only accessible to your usermod here
    unsigned long lastTime = 0;

    SoftwareSerial mySoftwareSerial; // RX, TX
    DFRobotDFPlayerMini myDFPlayer;

  public:
    //Functions called by WLED

    /*
     * setup() is called once at boot. WiFi is not yet connected at this point.
     * You can use it to initialize variables, sensors or similar.
     */
    void setup() {
        Serial.println();
        Serial.println(F("Mandrake Usermod"));
        Serial.println();
        Serial.println(F("Initializing Hall Sensor"));
        pinMode(hallSensorPin, INPUT);

        Serial.println();
        Serial.println(F("Initializing DFPlayer ... (May take 3~5 seconds)"));

        mySoftwareSerial.begin(9600, SWSERIAL_8N1, mp3InputPin, mp3OutputPin);
        if (!myDFPlayer.begin(mySoftwareSerial, false)) {  //Use softwareSerial to communicate with mp3.
            Serial.println(F("Unable to begin:"));
            Serial.println(F("1.Please recheck the connection!"));
            Serial.println(F("2.Please insert the SD card!"));
            while(true) {delay(1000);};
        }
        Serial.println(F("DFPlayer Mini online."));

        myDFPlayer.setTimeOut(500); //Set serial communication time out 500ms
        myDFPlayer.volume(25);  //Set volume value (0~30).
        myDFPlayer.EQ(DFPLAYER_EQ_ROCK);
        myDFPlayer.outputDevice(DFPLAYER_DEVICE_SD);
    }


    /*
     * connected() is called every time the WiFi is (re)connected
     * Use it to initialize network interfaces
     */
    void connected() {
      // Nothing ToDo
    }


    /*
     * loop() is called continuously. Here you can check for events, read sensors, etc.
     * 
     * Tips:
     * 1. You can use "if (WLED_CONNECTED)" to check for a successful network connection.
     *    Additionally, "if (WLED_MQTT_CONNECTED)" is available to check for a connection to an MQTT broker.
     * 
     * 2. Try to avoid using the delay() function. NEVER use delays longer than 10 milliseconds.
     *    Instead, use a timer check as shown here.
     */
    void loop() {
        if (millis() - lastTime > 300) {
            bool magnetIsCloseToSensor = !bool(digitalRead(hallSensorPin));

            if (!(magnetIsCloseToSensor ^ playMusicIfMagnetIsClose) && !isPlaying) {  // XNOR
                myDFPlayer.play();
                isPlaying = true;
            } else if ((magnetIsCloseToSensor ^ playMusicIfMagnetIsClose) && isPlaying) {  // XOR
                myDFPlayer.stop();
                isPlaying = false;
            }
            lastTime = millis();
        }

        if (myDFPlayer.available()) {
            uint8_t messageType = myDFPlayer.readType();
            if (messageType == DFPlayerPlayFinished) {
                isPlaying = false;
            }
            // printDetail(myDFPlayer.readType(), myDFPlayer.read());
        }
    }


    /*
     * addToJsonInfo() can be used to add custom entries to the /json/info part of the JSON API.
     * Creating an "u" object allows you to add custom key/value pairs to the Info section of the WLED web UI.
     * Below it is shown how this could be used for e.g. a light sensor
     */
    
    void addToJsonInfo(JsonObject& root)
    {
        JsonObject user = root["u"];
        if (user.isNull()) user = root.createNestedObject("u");

        JsonArray playArr = user.createNestedArray(MAGNET_CLOSE_KEY);
        playArr.add(playMusicIfMagnetIsClose);
    }
    


    /*
     * addToJsonState() can be used to add custom entries to the /json/state part of the JSON API (state object).
     * Values in the state object may be modified by connected clients
     */
    void addToJsonState(JsonObject& root)
    {
      //root["user0"] = userVar0;
    }


    /*
     * readFromJsonState() can be used to receive data clients send to the /json/state part of the JSON API (state object).
     * Values in the state object may be modified by connected clients
     */
    void readFromJsonState(JsonObject& root)
    {
      //if (root["bri"] == 255) Serial.println(F("Don't burn down your garage!"));
    }


    /*
     * addToConfig() can be used to add custom persistent settings to the cfg.json file in the "um" (usermod) object.
     * It will be called by WLED when settings are actually saved (for example, LED settings are saved)
     * If you want to force saving the current state, use serializeConfig() in your loop().
     * 
     * CAUTION: serializeConfig() will initiate a filesystem write operation.
     * It might cause the LEDs to stutter and will cause flash wear if called too often.
     * Use it sparingly and always in the loop, never in network callbacks!
     * 
     * addToConfig() will also not yet add your setting to one of the settings pages automatically.
     * To make that work you still have to add the setting to the HTML, xml.cpp and set.cpp manually.
     * 
     * I highly recommend checking out the basics of ArduinoJson serialization and deserialization in order to use custom settings!
     */
    void addToConfig(JsonObject& root)
    {
      JsonObject top = root.createNestedObject("mandrakeusermod");
      top[MP3_OUTPUT_PIN_KEY] = mp3OutputPin;
      top[MP3_INPUT_PIN_KEY] = mp3InputPin;
      top[HALL_SENSOR_KEY] = hallSensorPin;
      top[MAGNET_CLOSE_KEY] = playMusicIfMagnetIsClose;
    }


    /*
     * readFromConfig() can be used to read back the custom settings you added with addToConfig().
     * This is called by WLED when settings are loaded (currently this only happens once immediately after boot)
     * 
     * readFromConfig() is called BEFORE setup(). This means you can use your persistent values in setup() (e.g. pin assignments, buffer sizes),
     * but also that if you want to write persistent values to a dynamic buffer, you'd need to allocate it here instead of in setup.
     * If you don't know what that is, don't fret. It most likely doesn't affect your use case :)
     * 
     * Return true in case your config was complete, or false if you'd like WLED to save your defaults to disk
     * 
     * This function is guaranteed to be called on boot, but could also be called every time settings are updated
     */
    bool readFromConfig(JsonObject& root)
    {
        //set defaults for variables when declaring the variable (class definition or constructor)
        JsonObject top = root["mandrakeusermod"];
        if (top.isNull()) return false;

        mp3OutputPin = top[MP3_OUTPUT_PIN_KEY] | mp3OutputPin;
        mp3InputPin = top[MP3_INPUT_PIN_KEY] | mp3InputPin;
        hallSensorPin = top[HALL_SENSOR_KEY] | hallSensorPin;
        playMusicIfMagnetIsClose = top[MAGNET_CLOSE_KEY];

        return !top[MAGNET_CLOSE_KEY].isNull();;
    }

   
    /*
     * getId() allows you to optionally give your V2 usermod an unique ID (please define it in const.h!).
     * This could be used in the future for the system to determine whether your usermod is installed.
     */
    uint16_t getId()
    {
      return USERMOD_ID_MANDRAKE;
    }

   //More methods can be added in the future, this example will then be extended.
   //Your usermod will remain compatible as it does not need to implement all methods from the Usermod base class!
};