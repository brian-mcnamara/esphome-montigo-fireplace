#ifndef FIREPLACEHELPER_H
#define FIREPLACEHELPER_H

#include <vector>
#include <cmath>
#include "esphome/core/log.h"
using namespace std;


/*
    Parses the transmission and decodes the state.
    TODO: Figure out the whole packet and properly put it into a structure
*/
int getCode(vector<int> transmission) {

    boolean found = false;
    short dataCounter = 0;
    short blockCounter = 0;
    short packetSize = 22;
    unsigned char data[packetSize];
    unsigned char tempData[packetSize];
    unsigned char block = 0;

    for (const int& duration : transmission) {
        //Skip till second transmission
        if (duration < -4800) {
            if(memcmp(&data, &tempData, sizeof(tempData)) == 0) {
                found = true;
                break;
            } else {
                ESP_LOGD("Fireplace", "Blocks do not match");
                continue;
            }
        }

        int bits = round(duration / 400.0);
        for (int i = 0; i < abs(bits); i++) {
            int pos = 7 - blockCounter++;
            block |= (bits > 0 ? 1 : 0) << pos;
            if (pos == 0) {
                tempData[dataCounter++] = block;
                block = 0;
                blockCounter = 0;
            }
        }
        if (dataCounter >= packetSize) {
            dataCounter = 0;
            memcpy(data, tempData, packetSize);
        }
    }

    if (found) {
        ESP_LOGD("Fireplace", "Block received: %02x%02x%02x%02x%02x%02x%02x%02x", data[14], data[15], data[16], data[17], data[18], data[19], data[20], data[21]);
        if (data[18] == 0xa5) {
            return 0;
        } else if (data[20] == 0x56) {
            return 1;
        } else if (data[20] == 0x6a) {
            return 2;
        } else if (data[20] == 0x66) {
            return 3;
        } else if (data[20] == 0x9a) {
            return 4;
        } else if (data[20] == 0x96) {
            return 5;
        } else if (data[20] == 0xaa) {
            return 6;
        }
    }
    ESP_LOGD("Fireplace", "Unknown code sent");
    return -1;
}

#endif