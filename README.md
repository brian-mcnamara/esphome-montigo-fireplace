# ESPHome Montigo Fireplace Transciver

This project aims to integrate Montigo fireplaces controlled with a remote into
Home Assistant using esphome. 

**Note:** I have only tested this on my fireplace. Since I have not decoded the 
payload, it may be there is a serial as part of the payload and I would need to 
modify this project to allow for that. Please raise an issue if this is the case.
Please uncomment the remote_receiver.dump.raw configuration and add the dumps to the issue.

## Components:

I used a esp8266 board along with a CC1101 transviver I bought, specifically:
- HiLetgo ESP8266 NodeMCU ESP-12E
- E07-M1101D CC1101 development board

### Pinouts:

The following is the pin connections from the CC1101 to the ESP board
- 1 &rarr; GND
- 2 &rarr; 3v3
- 3 &rarr; D1
- 4 &rarr; D3
- 5 &rarr; D5
- 6 &rarr; D7
- 7 &rarr; D6
- 8 &rarr; D2

## Quick start:

Add a secret.yaml file to contain:
```yaml
ssid: [wifi ssid]
password: [wifi password]
```

Followed by using [esphome programmer](https://esphome.io/guides/getting_started_command_line#first-uploading)
to compile and install onto the esp board.

Thanks to dbuezas for creating the cc1101 helper used in this project: https://github.com/dbuezas/esphome-cc1101