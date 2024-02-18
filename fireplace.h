#ifndef FIREPLACEHELPER_H
#define FIREPLACEHELPER_H

#include <vector>
#include <cmath>
#include "esphome/core/log.h"
using namespace std;


/*
    Parses the transmission and decodes the state.
    TODO: Figure out the whole packet and properly put it into a structure
    TODO: Since it sends the same packet multiple times, this should capture the multiple packets and find if any are bad
*/
int getCode(vector<int> transmission) {

    boolean found = false;
    short offset = 116;
    short dataCounter = 0;
    short counter = 0;
    short blockCounter = 0;
    unsigned char data[3];
    unsigned char block = 0;

    for (const int& duration : transmission) {
        //Skip till second transmission
        if (duration > -4800 && !found) {
            found = true;
            continue;
        } else if (duration < -4800) {
            break;
        }

        int bits = round(duration / 413.0);
        for (int i = 0; i < abs(bits); i++) {
            if (counter++ < offset) {
                continue;
            }
            int pos = 7 - blockCounter++;
            block |= (bits > 0 ? 1 : 0) << pos;
            if (pos == 0) {
                data[dataCounter++] = block;
                block = 0;
                blockCounter = 0;
                if (dataCounter >= 3) {
                    ESP_LOGD("Fireplace", "Block received: %02x%02x%02x", data[0], data[1], data[2]);
                    if ((data[2] & 0x0F) == 0x0a) {
                        return 0;
                    } else if (data[0] == 0xb3) {
                        return 1;
                    } else if (data[0] == 0xcb) {
                        return 2;
                    } else if (data[0] == 0xd2) {
                        return 3;
                    } else if (data[0] == 0x95) {
                        return 4;
                    } else if (data[0] == 0x32) {
                        return 5;
                    } else if (data[0] == 0xa5 || (data[2] & 0x0F) == 0x04) {
                        return 6;
                    }
                    return -1;
                }
            }
        }
    }
    ESP_LOGD("Fireplace", "Unknown code sent");
    return -1;
}

#endif