/*
 * Copyright (c) 2020 Particle Industries, Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "Particle.h"
#include "tracker_config.h"
#include "tracker.h"
#include "MQTT.h"

#define SENSOR_NAME          "Nelium-Tracker-3"

PRODUCT_ID(TRACKER_PRODUCT_ID);
PRODUCT_VERSION(TRACKER_PRODUCT_VERSION);

SYSTEM_THREAD(ENABLED);
SYSTEM_MODE(SEMI_AUTOMATIC);

STARTUP(
    Tracker::startup();
);

SerialLogHandler logHandler(115200, LOG_LEVEL_TRACE, {
    { "app.gps.nmea", LOG_LEVEL_INFO },
    { "app.gps.ubx",  LOG_LEVEL_INFO },
    { "ncp.at", LOG_LEVEL_INFO },
    { "net.ppp.client", LOG_LEVEL_INFO },
});

void locationGenerationCallback(JSONWriter &writer, LocationPoint &point, const void *context); // Forward declaration
int getLocation(LocationPoint& point);
int getStatus(LocationStatus& status);
void mqttPublishField();

const char* broker = "mqtt.ivanlab.org";
const char *topicData = "ivanlab/tracker/S/3";
const char* topicCtrl = "ivanlab/tracker/C/3";
const char* topicCtrlRx = "ivanlab/tracker/CC/3";

int keepalive = 5;
void callback(char* topic, byte* payload, unsigned int length);
MQTT client("mqtt.ivanlab.org", 1883, keepalive, callback);

unsigned long lastConnectionTime = 0; 
const unsigned long postingInterval = 30L * 1000L; // Post data every 30 seconds.


void setup()
{
    TrackerConfiguration config;
    config.enableIoCanPowerSleep(true)
          .enableIoCanPower(true);

    Tracker::instance().init(config);
    Tracker::instance().location.regLocGenCallback(locationGenerationCallback);

    Particle.connect();

// MQTT initialization
    String connect = String(SENSOR_NAME) + " is connected";

    while (!client.isConnected()){
    client.connect(SENSOR_NAME);
    delay(1000);
    Log.info("!");
    }

    const char* msgBuffer = connect.c_str();
    Log.info(connect);
    client.publish(topicCtrl, msgBuffer );
    client.subscribe(topicCtrlRx);

}

//=================================

void loop()
{
    Tracker::instance().loop();

   client.loop();   // Call the loop continuously to establish connection to the server.


    if (millis() - lastConnectionTime > postingInterval) 
    {
        mqttPublishField(); // Use this function to publish to a single field
        lastConnectionTime = millis();
    }
}

//=================================

void locationGenerationCallback(JSONWriter &writer, LocationPoint &point, const void *context){
    
}

void mqttPublishField() {

    LocationStatus locationStatus;
    LocationPoint locationPoint;

    Tracker::instance().locationService.getStatus(locationStatus);
    Tracker::instance().locationService.getLocation(locationPoint);
    Log.info("GPS lock=%d", locationStatus.locked);
    Log.info("GPS Latitude=%f", locationPoint.latitude);
    Log.info("GPS Longitude=%f",locationPoint.longitude);
    Log.info("GPS speed=%f",locationPoint.speed);
    
    // Check connected to MQTT

        Log.info("Trying to reconnect");
        client.connect(SENSOR_NAME);
        delay(1000);

    // String data = String("GPS Latitude=%f", locationPoint.latitude);
    String data = "{\"Lat\":\"" + String(locationPoint.latitude) + "\"}";
    const char* msgBuffer = data.c_str();
    Log.info(data);
    client.publish(topicData, msgBuffer );
}

void callback(char* topic, byte* payload, unsigned int length) {
    char p[length + 1];
    memcpy(p, payload, length);
    p[length] = NULL;
}