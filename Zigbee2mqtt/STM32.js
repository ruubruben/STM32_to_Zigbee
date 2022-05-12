const fz = require('zigbee-herdsman-converters/converters/fromZigbee');
const tz = require('zigbee-herdsman-converters/converters/toZigbee');
const exposes = require('zigbee-herdsman-converters/lib/exposes');
const reporting = require('zigbee-herdsman-converters/lib/reporting');
const extend = require('zigbee-herdsman-converters/lib/extend');
const e = exposes.presets;
const ea = exposes.access;

const definition = {
    zigbeeModel: ['STM32WB'], // The model ID from: Device with modelID 'lumi.sens' is not supported.
    model: 'p-nucleo WB55', // Vendor model number, look on the device for a model number
    vendor: 'STMicroelectronics', // Vendor of the device (only used for documentation and startup logging)
    description: 'P-nucleo wb55 development board for zigbee', // Description of the device, copy from vendor site. (only used fo>
    extend: extend.light_onoff_brightness(),
    exposes: [e.light_brightness().withEndpoint('button_1'), e.light_brightness().withEndpoint('button_2'), e.light_brightness().>
    endpoint: (device) => {
        return {'button_1': 17, 'button_2': 20, 'button_3': 21, 'button_4': 22, 'button_5': 23};
    },
    meta: {multiEndpoint: true},
};

module.exports = definition;
