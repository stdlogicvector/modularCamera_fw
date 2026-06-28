/*
   The following constants are taken from the USB and USB Video Class (UVC) specifications.
   They are defined here for convenient usage in the rest of the application source code.
 */
#define CY_FX_INTF_ASSN_DSCR_TYPE       (0x0B)          		// Type code for Interface Association Descriptor (IAD)

#define CY_FX_UVC_MAX_HEADER           (12)             		// Maximum UVC header size, in bytes.
#define CY_FX_UVC_HEADER_DEFAULT_BFH   (0x8C)           		// Default BFH (Bit Field Header) for the UVC Header

#define CY_FX_UVC_STILL_PROBE_CONTROL    (uint16_t)(0x0300)		// wValue setting used to access STILL PROBE control
#define CY_FX_UVC_STILL_COMMIT_CONTROL   (uint16_t)(0x0400)		// wValue setting used to access STILL COMMIT control
#define CY_FX_UVC_STILL_TRIGGER_CONTROL  (uint16_t)(0x0500)		// wValue setting used to access STILL Trigger control

#ifdef FX3_UVC_1_0_SUPPORT
#define CY_FX_UVC_MAX_PROBE_SETTING    (26)             		// Maximum number of bytes in Probe Control
#define CY_FX_UVC_MAX_PROBE_SETTING_ALIGNED (32)        		// Probe control data size aligned to 16 bytes.
#else
#define CY_FX_UVC_MAX_PROBE_SETTING    (34)             		// Maximum number of bytes in Probe Control
#define CY_FX_UVC_MAX_PROBE_SETTING_ALIGNED (48)        		// Probe control data size aligned to 16 bytes.
#endif

#define CY_FX_MAX_STILL_PROBE_SETTING             	(11)    	//Maximum number of bytes in Still probe Control
#define CY_FX_MAX_STILL_PROBE_SETTING_ALIGNED    	(16)    	// Still probe Control aligned to 16 bytes

#define CY_FX_UVC_HEADER_FRAME          (0)                     // UVC header value for normal frame indication
#define CY_FX_UVC_HEADER_EOF            (uint8_t)(1 << 1)       // UVC header value for end of frame indication
#define CY_FX_UVC_HEADER_FRAME_ID       (uint8_t)(1 << 0)       // Frame ID toggle bit in UVC header.

#define CY_FX_UVC_STILL_IMAGE_HEADER    (uint8_t)(1 << 5)       // Set bit D5 of bmHeaderInfo to indicate it is a Still Frame

#define CY_FX_USB_UVC_SET_REQ_TYPE      (uint8_t)(0x21)         // UVC Interface SET Request Type
#define CY_FX_USB_UVC_GET_REQ_TYPE      (uint8_t)(0xA1)         // UVC Interface GET Request Type
#define CY_FX_USB_UVC_GET_CUR_REQ       (uint8_t)(0x81)         // UVC GET_CUR Request
#define CY_FX_USB_UVC_SET_CUR_REQ       (uint8_t)(0x01)         // UVC SET_CUR Request
#define CY_FX_USB_UVC_GET_MIN_REQ       (uint8_t)(0x82)         // UVC GET_MIN Request
#define CY_FX_USB_UVC_GET_MAX_REQ       (uint8_t)(0x83)         // UVC GET_MAX Request
#define CY_FX_USB_UVC_GET_RES_REQ       (uint8_t)(0x84)         // UVC GET_RES Request
#define CY_FX_USB_UVC_GET_LEN_REQ       (uint8_t)(0x85)         // UVC GET_LEN Request
#define CY_FX_USB_UVC_GET_INFO_REQ      (uint8_t)(0x86)         // UVC GET_INFO Request
#define CY_FX_USB_UVC_GET_DEF_REQ       (uint8_t)(0x87)         // UVC GET_DEF Request

#define CY_FX_UVC_STREAM_INTERFACE      (uint8_t)(1)            // Streaming Interface : Alternate Setting 1
#define CY_FX_UVC_CONTROL_INTERFACE     (uint8_t)(0)            // Control Interface
#define CY_FX_UVC_PROBE_CTRL            (uint16_t)(0x0100)      // wValue setting used to access PROBE control.
#define CY_FX_UVC_COMMIT_CTRL           (uint16_t)(0x0200)      // wValue setting used to access COMMIT control.

#define CY_FX_UVC_INTERFACE_CTRL        (uint8_t)(0)            // wIndex value used to select UVC interface control.
#define CY_FX_UVC_CAMERA_TERMINAL_ID    (uint8_t)(1)            // wIndex value used to select Camera terminal.
#define CY_FX_UVC_PROCESSING_UNIT_ID    (uint8_t)(2)            // wIndex value used to select Processing Unit.
#define CY_FX_UVC_EXTENSION_UNIT_ID     (uint8_t)(3)            // wIndex value used to select Extension Unit.

// Processing Unit specific UVC control selector codes defined in the USB Video Class specification.
#define CY_FX_UVC_PU_BACKLIGHT_COMPENSATION_CONTROL         (uint16_t)(0x0100)
#define CY_FX_UVC_PU_BRIGHTNESS_CONTROL                     (uint16_t)(0x0200)
#define CY_FX_UVC_PU_CONTRAST_CONTROL                       (uint16_t)(0x0300)
#define CY_FX_UVC_PU_GAIN_CONTROL                           (uint16_t)(0x0400)
#define CY_FX_UVC_PU_POWER_LINE_FREQUENCY_CONTROL           (uint16_t)(0x0500)
#define CY_FX_UVC_PU_HUE_CONTROL                            (uint16_t)(0x0600)
#define CY_FX_UVC_PU_SATURATION_CONTROL                     (uint16_t)(0x0700)
#define CY_FX_UVC_PU_SHARPNESS_CONTROL                      (uint16_t)(0x0800)
#define CY_FX_UVC_PU_GAMMA_CONTROL                          (uint16_t)(0x0900)
#define CY_FX_UVC_PU_WHITE_BALANCE_TEMPERATURE_CONTROL      (uint16_t)(0x0A00)
#define CY_FX_UVC_PU_WHITE_BALANCE_TEMPERATURE_AUTO_CONTROL (uint16_t)(0x0B00)
#define CY_FX_UVC_PU_WHITE_BALANCE_COMPONENT_CONTROL        (uint16_t)(0x0C00)
#define CY_FX_UVC_PU_WHITE_BALANCE_COMPONENT_AUTO_CONTROL   (uint16_t)(0x0D00)
#define CY_FX_UVC_PU_DIGITAL_MULTIPLIER_CONTROL             (uint16_t)(0x0E00)
#define CY_FX_UVC_PU_DIGITAL_MULTIPLIER_LIMIT_CONTROL       (uint16_t)(0x0F00)
#define CY_FX_UVC_PU_HUE_AUTO_CONTROL                       (uint16_t)(0x1000)
#define CY_FX_UVC_PU_ANALOG_VIDEO_STANDARD_CONTROL          (uint16_t)(0x1100)
#define CY_FX_UVC_PU_ANALOG_LOCK_STATUS_CONTROL             (uint16_t)(0x1200)

// Camera Terminal specific UVC control selector codes defined in the USB Video Class specification.
#define CY_FX_UVC_CT_SCANNING_MODE_CONTROL                  (uint16_t)(0x0100)
#define CY_FX_UVC_CT_AE_MODE_CONTROL                        (uint16_t)(0x0200)
#define CY_FX_UVC_CT_AE_PRIORITY_CONTROL                    (uint16_t)(0x0300)
#define CY_FX_UVC_CT_EXPOSURE_TIME_ABSOLUTE_CONTROL         (uint16_t)(0x0400)
#define CY_FX_UVC_CT_EXPOSURE_TIME_RELATIVE_CONTROL         (uint16_t)(0x0500)
#define CY_FX_UVC_CT_FOCUS_ABSOLUTE_CONTROL                 (uint16_t)(0x0600)
#define CY_FX_UVC_CT_FOCUS_RELATIVE_CONTROL                 (uint16_t)(0x0700)
#define CY_FX_UVC_CT_FOCUS_AUTO_CONTROL                     (uint16_t)(0x0800)
#define CY_FX_UVC_CT_IRIS_ABSOLUTE_CONTROL                  (uint16_t)(0x0900)
#define CY_FX_UVC_CT_IRIS_RELATIVE_CONTROL                  (uint16_t)(0x0A00)
#define CY_FX_UVC_CT_ZOOM_ABSOLUTE_CONTROL                  (uint16_t)(0x0B00)
#define CY_FX_UVC_CT_ZOOM_RELATIVE_CONTROL                  (uint16_t)(0x0C00)
#define CY_FX_UVC_CT_PANTILT_ABSOLUTE_CONTROL               (uint16_t)(0x0D00)
#define CY_FX_UVC_CT_PANTILT_RELATIVE_CONTROL               (uint16_t)(0x0E00)
#define CY_FX_UVC_CT_ROLL_ABSOLUTE_CONTROL                  (uint16_t)(0x0F00)
#define CY_FX_UVC_CT_ROLL_RELATIVE_CONTROL                  (uint16_t)(0x1000)
#define CY_FX_UVC_CT_PRIVACY_CONTROL                        (uint16_t)(0x1100)

#define LOOP_TIMEOUT                                        (1000)      // Period of frame count updates.

#ifdef UVC_EXTENSION_UNIT
// Extension Unit Terminal Controls specific UVC control selector codes
#define CY_FX_UVC_XU_GET_FIRMWARE_VERSION_CONTROL           (uint16_t)(0x0100)
// Customer specific controls can be added here
#endif

// Undefined Terminal Controls specific UVC control selector codes defined in the USB Video Class specification
#define CY_FX_UVC_VC_REQUEST_ERROR_CODE_CONTROL             (uint16_t)(0x0200)

// Video control Error Codes
#define CY_FX_UVC_VC_ERROR_CODE_NO_ERROR                    (0x00)
#define CY_FX_UVC_VC_ERROR_CODE_NOT_READY                   (0x01)
#define CY_FX_UVC_VC_ERROR_CODE_WRONG_STATE                 (0x02)
#define CY_FX_UVC_VC_ERROR_CODE_POWER                       (0x03)
#define CY_FX_UVC_VC_ERROR_CODE_OUT_OF_RANGE                (0x04)
#define CY_FX_UVC_VC_ERROR_CODE_INVALID_UNIT                (0x05)
#define CY_FX_UVC_VC_ERROR_CODE_INVALID_CONTROL             (0x06)
#define CY_FX_UVC_VC_ERROR_CODE_INVALID_REQUEST             (0x07)
#define CY_FX_UVC_VC_ERROR_CODE_INVALID_VAL_IN_RANGE        (0x08)
#define CY_FX_UVC_VC_ERROR_CODE_UNKNOWN                     (0xFF)

// Event bits used for signaling the UVC application threads.

/* Stream request event. Event flag that indicates that the streaming of video data has
   been enabled by the host. This flag is retained ON as long as video streaming is allowed,
   and is only turned off when the host indicates that data transfer should be stopped.
 */
#define CY_FX_UVC_STREAM_EVENT                  (1 << 0)

/* Abort streaming event. This event flag is set when the UVC host sends down a request
   (SET_INTERFACE or CLEAR_FEATURE) that indicates that video streaming should be stopped.
   The CY_FX_UVC_STREAM_EVENT event is cleared before setting this flag, and these two
   events can be considered as mutually exclusive.
 */
#define CY_FX_UVC_STREAM_ABORT_EVENT            (1 << 1)

/* UVC VIDEO_CONTROL_REQUEST event. This event flag indicates that a UVC class specific
   request addressed to the video control interface has been received. It should be cleared
   as soon as serviced by the firmware.
 */
#define CY_FX_UVC_VIDEO_CONTROL_REQUEST_EVENT   (1 << 2)

/* UVC VIDEO_STREAM_REQUEST event. This event flag indicates that a UVC class specific
   request addressed to the video streaming interface has been received. It should be cleared
   as soon as serviced by the firmware.
 */
#define CY_FX_UVC_VIDEO_STREAM_REQUEST_EVENT    (1 << 3)

/* FX3 DMA Reset event. This event is set when FX3 is not able to commit a buffer due to a slower USB Host or due to
 * a frame timer overflow. When the device is streaming a higher resolution with higher fps, the USB bandwidth will
 * be saturated and Host will not be able to keep up. The video stream may work for few seconds and then device will
 * receive a commit buffer failure. It is also possible that Sensor/ISP fails to send video data due to some reasons.
 * In such cases, it is better to reset DMA and restart the video stream so that there is a continuous video preview.
 */
#define CY_FX_UVC_DMA_RESET_EVENT               (1 << 4)

/* USB suspend event handler. This event is set when the USB host sends a USB suspend event to put the FX3
 * device into low power mode. This event is sent when the Host application is closed and after the device enumerates.
 */
#define CY_FX_USB_SUSPEND_EVENT_HANDLER         (1 << 5)

#define CY_FX_STILL_CAPTURE_HW_TRIGGER_EVENT    (1 << 7)

// Enum for a DMA reset event
typedef enum CyFxUvcDmaResetVal
{
    CY_FX_UVC_DMA_RESET_EVENT_NOT_ACTIVE = 0,           // FX3 DMA reset event haven't occurred
    CY_FX_UVC_DMA_RESET_COMMIT_BUFFER_FAILURE,          // FX3 DMA reset event caused due to a commit buffer failure
    CY_FX_UVC_DMA_RESET_FRAME_TIMER_OVERFLOW            // FX3 DMA reset event caused due to frame timer overflow
} CyFxUvcDmaResetVal_t;

// Enum for different frame timer values
typedef enum CyFxUvcFrameTimerVal
{
    CY_FX_UVC_FRAME_TIMER_VAL_100MS = 100,
    CY_FX_UVC_FRAME_TIMER_VAL_200MS = 200,
    CY_FX_UVC_FRAME_TIMER_VAL_300MS = 300,
    CY_FX_UVC_FRAME_TIMER_VAL_400MS = 400,
} CyFxUvcFrameTimerVal_t;

#define WBVAL(x) 	(((unsigned short) x) & 0xFF),((((unsigned short) x) >> 8) & 0xFF)
#define DBVAL(x)	(((unsigned int) x) & 0xFF),((((unsigned int) x) >> 8) & 0xFF),((((unsigned int) x) >> 16) & 0xFF),((((unsigned int) x) >> 24) & 0xFF)


