# STM32WB55 to Zigbee2MQTT

## An easy way to get the Nucleo-WB55RG board working with Zigbee2MQTT.

Online there is very few resources to get Zigbee correctly working with the STM32WB55. The only real resources are the examples provided by STM.
Those examples are great when just starting out, but when you want to connect your own Zigbee device to something other then another STM board it gets tricky.

So for the last couple of weeks I've been trying to connect my Zigbee device to Zigbee2MQTT so the device could actually be used. This was more tricky then I excpected so I thought mabye people could learn something from it.

### Usage

for this example the Nucleo board acts as a controller for a WS2812b LED-strip. When genOnOff or genLevelControl commands are sent to device it will turn on LED's and turn on certain effects.

the LED's need to be connected to 5v, ground and pin 25(*This pin is used for PWM creation.*)

### Code

#### Nucleo WB55RG

The Nucleo Board's code is generated with the STMcubeMX tool, it sets the board as a centralized Zigbee router with 6 endpoints. It also sets channel 1 of timer 1 as a PWM generation output with DMA. this is used by the WS2812b LED's to set up the correct timing.
The 6 endpoints are configured for both the genOnOff and genLevelControl clusters, the on off switches can be used to toggle different effects and the sliders are used to control different variables (*red, green, blue, brightness & time*)
With the generated files from cubeMX there is only adding the code to the endpoints that turns on and off the LED's (*all code for LED's and zigbee is found under Nucleo WB55 code/Source code.*) if you want to see the full source code create a project based of the added IOC file with cubeMX or cubeIDE and add in the source files in the right place.

#### Zigbee2MQTT converter

the added Zigbee2MQTT converter added is just a standerd converter that adds in the multiple endpoints to configure the LED's over Zigbee. To use this add the STM32.js file as a external converter in the configuration.yaml file.
