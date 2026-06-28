# ModularCamera Firmware

This repository contains the firmware for the Cypress FX3 that provides the USB3 connectivity for the ModularCamera.

When connected to a PC, the FX3 presents as a composite device consisting of one UVC device used for streaming the video data and a CDC device for communicating with the FPGA.

## Different Sensors

As the UVC protocol needs predefined settings for resolution, color format and framerate in the USB descriptors, there currently are separate configurations for the different sensors.
Before compiling for one sensor, set the correct #define in uvc_cfg.h.

## Configuration

Configuration of the sensor is done via the CDC interface. At the moment, all configuration has to be done by the user via this interface.

To make the modularCamera more userfriendly, the plan is to have the FX3 take control of the UART when the CDC device isn't opened from the PC. Commands to start streaming video once the UVC device is opened would then be sent from the FX3 to the FPGA automatically. This hasn't been implemented yet as each sensor has different requirements that has to be abstracted away in the FPGA.

