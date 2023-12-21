#include <Wire.h>
#include <WiFi.h>
#include <MQTT.h>
#include <BH1750.h>
#include <ADS1115_WE.h>
#include <WiFiClient.h>
#include <ArduinoJson.h>
#include <Adafruit_PWMServoDriver.h>

#include "config.h"
#include "DFRobot_BME280.h"


// Constants
#define I2C_ADDRESS 0x49

#define FREQ 60
#define SERVOMIN 150
#define SERVOMAX 500
#define ANGLE_180 440
#define ANGLE_INCREASE 20

#define SERVO_1 0
#define SERVO_2 1

#define SEA_LEVEL_PRESSURE 1015.0f

// Initializations
typedef DFRobot_BME280_IIC BME;
typedef Adafruit_PWMServoDriver PWM;

BME bme(&Wire, 0x77);
PWM pwm = Adafruit_PWMServoDriver();
ADS1115_WE adc = ADS1115_WE(I2C_ADDRESS);

// Global variables
WiFiClient wifi;
MQTTClient client;
BH1750 light;

float max_value = 0;
int best_servo_1 = 200;
int best_servo_2 = 300;
int angle_min_1 = 150;
int angle_max_1 = ANGLE_180;
int angle_min_2 = 300;
int angle_max_2 = ANGLE_180 + 200;


// =====================================================
// == ARDUINO SETUP METHOD =============================
// =====================================================

void setup() {
    char ssid[] = WIFI_SSID;
    char pass[] = WIFI_PASSWORD;
    char url[] = MQTT_URL;
    int status = WL_IDLE_STATUS;

    Wire.begin();
    Serial.begin(9600);

    // Initialize servo driver
    pwm.begin();
    pwm.setPWMFreq(FREQ);

    // Initialize voltmeter
    if (!adc.init()) {
        Serial.println("ADS1115 not connected!");
    }

    adc.setVoltageRange_mV(ADS1115_RANGE_6144);
    adc.setCompareChannels(ADS1115_COMP_0_GND);
    adc.setMeasureMode(ADS1115_CONTINUOUS);

    // Initialize ambient sensor
    bme.begin();

    // Initialize ambient light sensor
    light.begin(BH1750::CONTINUOUS_HIGH_RES_MODE, 0x23, &Wire);

    // Initialize WiFi connection
    while (status != WL_CONNECTED) {
        Serial.print("Attempting to connect to Network named: "); Serial.println(ssid);
        status = WiFi.begin(ssid, pass);
    }

    Serial.print("SSID: "); Serial.println(WiFi.SSID());
    Serial.print("IP Address: "); Serial.println(WiFi.localIP());

    // Initialize connection to MQTT server
    client.begin(url, wifi);
}


// =====================================================
// == MEASUREMENT HELPER METHODS =======================
// =====================================================

float read_tension() {
    adc.setCompareChannels(ADS1115_COMP_1_GND);
    int16_t actual_val = adc.getRawResult();

    return (actual_val - 13500.00) / 600;
}


String get_direction(int servo_0, int servo_1) {
    String direction = "N";

    float tolerance = 115;
    float inclinations[3] = {300, 550, 640};
    float directions[4] = {150, 265, 380, 495};

    if (servo_0 >= directions[0] - tolerance && servo_0 <= directions[0] + tolerance) {
        direction = "N";
    } else if (servo_0 > directions[1] - tolerance && servo_0 <= directions[1] + tolerance) {
        direction = "NW";
    } else if (servo_0 > directions[2] - tolerance && servo_0 <= directions[2] + tolerance) {
        direction = "W";
    } else if (servo_0 > directions[3] - tolerance && servo_0 <= directions[3] + tolerance) {
        direction = "SW";
    }

    if (direction == "N" && servo_1 > inclinations[1]) {
        direction = "S";
    } else if (direction == "NW" && servo_1 > inclinations[1]) {
        direction = "SE";
    } else if (direction == "W" && servo_1 > inclinations[1]) {
        direction = "E";
    } else if (direction == "SW" && servo_1 > inclinations[1]) {
        direction = "NE";
    }

    return direction;
}


void acquire_and_send(int servo1, int servo2) {
    float temperature = bme.getTemperature();
    uint32_t press = bme.getPressure();
    float alti = bme.calAltitude(SEA_LEVEL_PRESSURE, press);
    float humi = bme.getHumidity();
    float tens = read_tension();
    float lux = light.readLightLevel();
    lux = std::log(lux) / std::log(54612);
    String direction = get_direction(servo1, servo2);

    Serial.println();
    Serial.println("======== start print ========");
    Serial.print("temperature (unit Celsius): "); Serial.println(temperature);
    Serial.print("altitude (unit meter): "); Serial.println(alti);
    Serial.print("humidity (unit percent): "); Serial.println(humi);
    Serial.print("light (unit percent): "); Serial.println(lux);
    Serial.print("energy production (unit percent): "); Serial.println(tens);
    Serial.print("direction: "); Serial.println(direction);
    Serial.println("========  end print  ========");
    Serial.println();

    send_data(temperature, alti, humi, lux, tens, direction);
}


// =====================================================
// == WIRELESS CONNECTIONS METHODS =====================
// =====================================================

void send_data(float temperature, float altitude, float humidity, float light, float energy, String direction) {
    StaticJsonDocument<200> doc;
    doc["t"] = round(temperature);
    doc["a"] = round(altitude);
    doc["h"] = round(humidity);
    doc["l"] = round(light * 100);
    doc["e"] = round(energy * 100);
    doc["d"] = direction;

    String body;
    serializeJson(doc, body);
    char char_array[body.length() + 1];
    body.toCharArray(char_array, body.length() + 1);

    Serial.println("Sending to server:");
    Serial.println(body);
    Serial.println(char_array);

    // Send data to the MQTT server
    client.publish("/sensors", body);
}


void connect() {
    char name[] = MQTT_NAME;
    char broker[] = MQTT_USERNAME;
    char password[] = MQTT_PASSWORD;

    Serial.print("connecting...");

    while (!client.connect(name, broker, password)) {
        Serial.print(".");
        delay(1000);
    }

    Serial.println("\nconnected!");
}


// =====================================================
// == ARDUINO MAIN LOOP ================================
// =====================================================

void loop() {
    // Initiate values and start servos movement loop
    float curr_tension = read_tension();

    // Perform scan only if current tension is < 95% of the last best tension, and it is daytime (tension > 0.15)
    if ((curr_tension < 0.95 * max_value || max_value == 0) && curr_tension > 0.15) {
        max_value = 0.0;

        for (int angle = angle_min_1; angle <= angle_max_1; angle += ANGLE_INCREASE) {
            int pulse = map(angle, 0, 360, SERVOMIN, SERVOMAX);

            // Move bottom servo
            pwm.setPWM(SERVO_1, 0, pulse);

            for (int angle_2 = angle_min_2; angle_2 <= angle_max_2; angle_2 += ANGLE_INCREASE) {
                int pulse = map(angle_2, 0, 500, SERVOMIN, SERVOMAX);

                // Move top servo
                pwm.setPWM(SERVO_2, 0, pulse);
                delay(100);

                float current = read_tension();

                if (current > max_value) {
                    best_servo_1 = angle;
                    best_servo_2 = angle_2;
                    max_value = current;
                }

                Serial.println("======== start print ========");
                Serial.print("Current: "); Serial.println(current);
                Serial.print("Best: "); Serial.println(max_value);
                Serial.print("Pos (bottom): "); Serial.println(angle);
                Serial.print("Pos (top): "); Serial.println(angle_2);
                Serial.print("Best pos (bottom): "); Serial.println(best_servo_1);
                Serial.print("Best pos (top): "); Serial.println(best_servo_2);
                Serial.print("Current Direction: "); Serial.println(get_direction(angle, angle_2));
                Serial.println("========  end print  ========");

                delay(500);
            }
        }

        angle_min_1 = best_servo_1 - ANGLE_INCREASE * 2;
        angle_max_1 = best_servo_1 + ANGLE_INCREASE * 2;

        int pulse1 = map(best_servo_1, 0, 360, SERVOMIN, SERVOMAX);
        int pulse2 = map(best_servo_2, 0, 360, SERVOMIN, SERVOMAX);

        // Move servos to best found position
        pwm.setPWM(SERVO_1, 0, pulse1);
        pwm.setPWM(SERVO_2, 0, pulse2);
    }

    // MQTT client connection loop
    client.loop();

    if (!client.connected()) {
        connect();
    }

    // Capture sensors data and send to ElasticSearch
    acquire_and_send(best_servo_1, best_servo_2);

    delay(1000 * 60 * 1);
}
