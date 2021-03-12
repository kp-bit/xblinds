# xBlinds

The xBlinds project started because I wanted a (cheap) way to motorize my vertical blinds and existing commercial solutions are heavily priced and not getting the best reviews, so a wintery night I started sketching the UI in Notepad.

This project is two-fold; the bin and UI published here and the STL for the casing going up on Thingiverse.

## Quick Guide

The bin is compiled for ESP8266 and will run on most variants but most importantly on the D1 mini which will fit into the 3D-printed casing.

Use your favorite flasher to flash the ESP (plenty of guides available for that out there).

### Initial Setup

When powering up the ESP for the first time, it will show up as a WiFi AP with the SSID xblinds-abcdef (where "abcdef" are the last 6 digits of the MAC address), use the password "persienne" (Venetian blind in Danish) to log on to the SSID.

To access the web based setup, enter the address 4.3.2.1 which will take you to the main screen (described later).

In the bottom of the screen, press WiFi to open this screen:

![WiFisetup](./images/xblinds-wifi.jpeg)

Hostname can't be edited, it will always be xblinds-abcdef (defined by MAC address).

Input your home SSID and password and press save.

Once connected to your home WiFi, the ESP will no longer publish an AP so you need to find its new IP address in your router.

### Adjusting the Blinds

I've put in some default stepper values in the code, they will most likely not fit your usage (or mine, for that matter), they are simply there to test that the stepper moves, when buttons are pressed.

![Setup](images/xblinds-setup.jpeg)

My recommendation is to attach to the blinds when they're closed and then press "Save as Closed" when the beaded line is seated well and the vertical blinds are closed.

> Note!
> Pressing + will turn the pulley counter-clockwise. Be careful you don't overstretch the line and/or break the gears in the stepper or the blinds.

Adjust your blinds, pressing + and - and press "Save as Normal Open" or "Save as Full Open" when they are in the desired position. A "half" position is calculated behind the scenes.

You can test your positions on the main screen:

![Main](images/xblinds.jpeg)

The positions may need some tweaking after the strings have settled in.

### MQTT

My main objective with this project was to have Home Assistant take care of the blinds, instead of me fiddling with my phone to open and close them, which is why I added MQTT to the mix. If you have no need for MQTT, leave this page blank - you may see untested results, if you put in a non-existing server, just saying...


![MQTT](images/xblinds-mqtt.jpeg)

The default group is xblinds/all but you can name it whatever you want. I'm using xblinds/windows for my three xblinds working together.

Input your MQTT server IP address. I'm using Mosquitto on Home Assistant, but I guess you could use CloudMQTT or a similar service, if you want.

Default MQTT port is 1883.

Lastly input a username and password for an account that has write access, as xblinds publishes a status.

### Home Assistant Yaml

For a super simple integration, that will let the blinds act as a light, try this in your configuration.yaml:

```yaml
light:
  - platform: mqtt
    name: "xBlinds"
    state_topic: "xblinds/window/status"
    command_topic: "xblinds/window"
    payload_on: "open"
    payload_off: "close"
    effect_state_topic: "xblinds/window"
    effect_list: [close, open, half, full]
```
You can also use MQTT directly in your automations/scripts:

```yaml
scene_shades_open:
  alias: Shades Open
  sequence:
    - service: mqtt.publish
      data_template:
        topic: "xblinds/window"
        payload_template: "open"
```

The recognized payload keywords are:
- close
- open
- half
- full

The returned status messages are:
- open
- closed


### Thingiverse

If you want to 3D print this project, go to Thingiverse to download the STLs (link coming shortly):

Here's the xblinds unit assembled: (I'd recommed desoldering the LED's on the ULN2003 stepper driver if you print the case in white filament, you'll know why when you see it).

![3D-print](images/xblinds-open.jpg)

And this is what it looks like mounted on the wall:

![Mounted](images/xblinds-mounted.jpg)


### Bill of Materials

#### Steppers and drivers:
ANGEEK 5 pcs. 5V 28BYJ-48 ULN2003 Stepper Motor with Drive Module Board

* Amazon.de: https://www.amazon.de/dp/B07VGV1XFT


#### D1 mini:
AZDelivery D1 Mini NodeMcu with ESP8266-12F WLAN Module

> Note!
> Different manufacturers have different dimensions.

* Amazon.de: https://www.amazon.de/-/en/gp/product/B01N9RXGHY
* AZDelivery: https://www.az-delivery.de/products/d1-mini?_pos=3&_sid=e1e1e37c6&_ss=r


#### Power supply:
Any 5V PSU will do, I've calculated ~2A per unit to be on the safe side, especially if the run at the same time in a group.


### Wiring

A nice diagram might come later... :)

    D1 mini <-> ULN2003
    D1 ------------ IN1
    D2 ------------ IN2
    D3 ------------ IN3
    D4 ------------ IN4

    +5V to:
    D1 mini: 5V
    ULN2003: +

    -5V to:
    D1 mini: G
    ULN2003: -
