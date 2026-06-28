/*
 * This file contains the USB descriptors for the FX3 FrameGrabber
 */

#include "uvc_cfg.h"
#include "uvc_defs.h"
#include "uvc.h"
#include "cdc.h"
#include "dbg.h"
#include "camera_ptzcontrol.h"

//#define DEVICE_CLASS		0xEF
//#define DEVICE_SUBCLASS	0x02
//#define DEVICE_PROTOCOL	0x01

#define DEVICE_CLASS		0x00
#define DEVICE_SUBCLASS		0x00
#define DEVICE_PROTOCOL		0x00

#define VENDOR_CYPRESS	0xB4,0x04

#define VENDOR_ID		0xED,0xFE
#define PRODUCT_ID		0xF9,0xF9

#define FS_MAX_IMGSIZE	(FS_WIDTH*FS_HEIGHT*FS_COLORDEPTH / 8)				// In Bytes
#define FS_MIN_BITRATE	(FS_WIDTH*FS_HEIGHT*FS_COLORDEPTH*FS_FRAMERATE)		// Min bit rate bits/s
#define FS_MAX_BITRATE	(FS_MIN_BITRATE)
#define FS_MIN_INTERVAL	(10000000/FS_FRAMERATE)								// in 100ns units

#define HS_MAX_IMGSIZE	(HS_WIDTH*HS_HEIGHT*HS_COLORDEPTH / 8)				// In Bytes
#define HS_MIN_BITRATE	(HS_WIDTH*HS_HEIGHT*HS_COLORDEPTH*HS_FRAMERATE)		// Min bit rate bits/s
#define HS_MAX_BITRATE	(HS_MIN_BITRATE)
#define HS_MIN_INTERVAL	(10000000/HS_FRAMERATE)								// in 100ns units
#define HS_MAX_INTERVAL (10000000/1)										// in 100ns units
#define HS_STP_INTERVAL (1)

#define SS_MAX_IMGSIZE	(SS_WIDTH*SS_HEIGHT*SS_COLORDEPTH / 8)				// In Bytes
#define SS_MIN_BITRATE	((unsigned int)SS_WIDTH*(unsigned int)SS_HEIGHT*(unsigned int)SS_COLORDEPTH*(unsigned int)SS_FRAMERATE)		// Min bit rate bits/s
#define SS_MAX_BITRATE	(SS_MIN_BITRATE)
#define SS_MIN_INTERVAL	(10000000/SS_FRAMERATE)								// in 100ns units
#define SS_MAX_INTERVAL (10000000/1)										// in 100ns units
#define SS_STP_INTERVAL (1)

// BOS for SS
#define CY_FX_BOS_DSCR_TYPE             15
#define CY_FX_DEVICE_CAPB_DSCR_TYPE     16
#define CY_FX_SS_EP_COMPN_DSCR_TYPE     48

// Device Capability Type Codes
#define CY_FX_WIRELESS_USB_CAPB_TYPE    1
#define CY_FX_USB2_EXTN_CAPB_TYPE       2
#define CY_FX_SS_USB_CAPB_TYPE          3
#define CY_FX_CONTAINER_ID_CAPBD_TYPE   4

// Standard Device Descriptor 
const uint8_t CyFxUSBDeviceDscr[] __attribute__ ((aligned (32))) =
    {
        0x12,                           // Descriptor Size 
        CY_U3P_USB_DEVICE_DESCR,        // Device Descriptor Type 
        0x10,0x02,                    // USB 2.1
//        0x00,0x02,                      // USB 2.0
        DEVICE_CLASS,                   // Device Class 
        DEVICE_SUBCLASS,                // Device Sub-class 
        DEVICE_PROTOCOL,                // Device protocol 
        0x40,                           // Maxpacket size for EP0 : 64 bytes 
        VENDOR_ID,                      // Vendor ID 
        PRODUCT_ID,                     // Product ID
        0x00,0x00,                      // Device release number 
        0x01,                           // Manufacture string index 
        0x02,                           // Product string index 
        0x04,                           // Serial number string index
        0x01                            // Number of configurations 
    };

// Device Descriptor for Super Speed
const uint8_t CyFxUSBDeviceDscrSS[] __attribute__ ((aligned (32))) =
    {
        0x12,                           // Descriptor Size 
        CY_U3P_USB_DEVICE_DESCR,        // Device Descriptor Type
        0x20,0x03,                      // USB 3.20
//        0x10,0x03,                      // USB 3.10
//        0x00,0x03,                      // USB 3.00
        DEVICE_CLASS,                   // Device Class 
        DEVICE_SUBCLASS,                // Device Sub-class 
        DEVICE_PROTOCOL,                // Device protocol 
        0x09,                           // Maxpacket size for EP0 : 2^9 Bytes 
        VENDOR_ID,                      // Vendor ID 
        PRODUCT_ID,                     // Product ID
        0x00,0x00,                      // Device release number 
        0x01,                           // Manufacture string index 
        0x02,                           // Product string index 
        0x04,                           // Serial number string index
        0x01                            // Number of configurations 
    };

// Standard Device Qualifier Descriptor 
const uint8_t CyFxUSBDeviceQualDscr[] __attribute__ ((aligned (32))) =
    {
        0x0A,                           // Descriptor Size 
        CY_U3P_USB_DEVQUAL_DESCR,       // Device Qualifier Descriptor Type 
//        0x00,0x02,                      // USB 2.0
        0x10,0x02,                      // USB 2.1
        DEVICE_CLASS,                   // Device Class 
        DEVICE_SUBCLASS,                // Device Sub-class 
        DEVICE_PROTOCOL,                // Device protocol 
        0x40,                           // Maxpacket size for EP0 : 64 bytes 
        0x01,                           // Number of configurations 
        0x00                            // Reserved 
    };

// Binary Device Object Store descriptor
const uint8_t CyFxUSBBOSDscr[] __attribute__ ((aligned (32))) =
{
        0x05,                           // Descriptor Size
        CY_FX_BOS_DSCR_TYPE,            // Device Descriptor Type
        0x16,0x00,                      // Length of this descriptor and all sub descriptors
        0x02,                           // Number of device capability descriptors

        // USB 2.0 Extension
        0x07,                           // Descriptor Size
        CY_FX_DEVICE_CAPB_DSCR_TYPE,    // Device Capability Type descriptor
        CY_FX_USB2_EXTN_CAPB_TYPE,      // USB 2.0 Extension Capability Type
        0x1E,0x64,0x00,0x00,            // Supported device level features: LPM support, BESL supported, Baseline BESL=400 us, Deep BESL=1000 us.

        // SuperSpeed Device Capability
        0x0A,                           // Descriptor Size
        CY_FX_DEVICE_CAPB_DSCR_TYPE,    // Device Capability Type descriptor
        CY_FX_SS_USB_CAPB_TYPE,         // SuperSpeed Device Capability Type
        0x00,                           // Supported device level features
        0x0C,0x00,                      // Speeds Supported by the device : SS and HS
        0x03,                           // Functionality support
        0x00,                           // U1 Device Exit Latency
        0x00,0x00                       // U2 Device Exit Latency
};

// Standard Full Speed Configuration Descriptor
const uint8_t CyFxUSBFSConfigDscr[] __attribute__ ((aligned (32))) =
	{
		/* Configuration descriptor */
		0x09,                           /* Descriptor size */
		CY_U3P_USB_CONFIG_DESCR,        /* Configuration descriptor type */
		0x09,0x00,                      /* Length of this descriptor and all sub descriptors */
		0x00,                           /* Number of interfaces */
		0x01,                           /* Configuration number */
		0x00,                           /* COnfiguration string index */
		0x80,                           /* Config characteristics - bus powered */
		0x32                            /* Max power consumption of device (in 2mA unit) : 100mA */
	};

// Standard High Speed Configuration Descriptor 
const uint8_t CyFxUSBHSConfigDscr[] __attribute__ ((aligned (32))) =
    {
        // Configuration Descriptor Type 
        0x09,                           // Descriptor Size 
        CY_U3P_USB_CONFIG_DESCR,        // Configuration Descriptor Type 
#ifdef FX3_UVC_1_0_SUPPORT
	#ifdef USE_DEBUG_PORT
#ifdef UVC_STILL_IMAGE
        WBVAL(361),						// Length of this descriptor and all sub descriptors
#else
        WBVAL(351),
#endif
        0x06,                           // Number of interfaces
	#else
#ifdef UVC_STILL_IMAGE
        WBVAL(295),
#else
        WBVAL(285),
#endif
        0x04,                           // Number of interfaces
	#endif
#else
	#ifdef USE_DEBUG_PORT
#ifdef UVC_STILL_IMAGE
        WBVAL(362),						// Length of this descriptor and all sub descriptors
#else
        WBVAL(352),
#endif
        0x06,                           // Number of interfaces
	#else
#ifdef UVC_STILL_IMAGE
        WBVAL(296),
#else
        WBVAL(286),
#endif
        0x04,                           // Number of interfaces
	#endif
#endif
        0x01,                           // Configuration number 
        0x00,                           // Configuration string index 
        0x80,                           // Config characteristics - Bus powered 
        0xFA,                           // Max power consumption of device (in 2mA unit) : 500mA 

        // Interface Association Descriptor 
        0x08,                           // Descriptor Size 
        CY_FX_INTF_ASSN_DSCR_TYPE,      // Interface Association Descr Type: 11 
        0x00,                           // Interface number of first VideoControl interface
        0x02,                           // Number of Video interfaces
        0x0E,                           // CC_VIDEO : Video interface class code
        0x03,                           // SC_VIDEO_INTERFACE_COLLECTION : Subclass code 
        0x00,                           // Protocol : Not used 
        0x02,                           // String desc index for interface 

        // Standard Video Control Interface Descriptor 
        0x09,                           // Descriptor size 
        CY_U3P_USB_INTRFC_DESCR,        // Interface Descriptor type 
        0x00,                           // Interface number 
        0x00,                           // Alternate setting number 
        0x01,                           // Number of end points 
        0x0E,                           // CC_VIDEO : Interface class 
        0x01,                           // CC_VIDEOCONTROL : Interface sub class 
        0x00,                           // Interface protocol code 
        0x02,                           // Interface descriptor string index 

        // Class specific VC Interface Header Descriptor 
        0x0D,                           // Descriptor size 
        0x24,                           // Class Specific interface Header Descriptor type
        0x01,                           // Descriptor Sub type : VC_HEADER 
#ifdef FX3_UVC_1_0_SUPPORT
        0x00, 0x01,                     // Revision of UVC class spec: 1.0 - Legacy version 
        WBVAL(80),                      // Total Size of class specific descriptors (till Output terminal)
#else
        0x10, 0x01,                     // Revision of UVC class spec: 1.1 - Minimum version required for USB Compliance. Not supported on Windows XP
        WBVAL(81),	                    // Total Size of class specific descriptors (till Output terminal)
#endif
        DBVAL(48000000), 	            // Clock frequency : 48MHz(Deprecated)
        0x01,                           // Number of streaming interfaces 
        0x01,                           // Video streaming interface 1 belongs to VC interface

        // Input (Camera) Terminal Descriptor 
        0x12,                           // Descriptor size 
        0x24,                           // Class specific interface desc type 
        0x02,                           // Input Terminal Descriptor type 
        0x01,                           // ID of this terminal 
        0x01,0x02,                      // Camera terminal type 
        0x00,                           // No association terminal 
        0x00,                           // String desc index : Not used 
#ifdef UVC_PTZ_SUPPORT
        WBVAL(wObjectiveFocalLengthMin),
        WBVAL(wObjectiveFocalLengthMax),
        WBVAL(wOcularFocalLength),
#else
        0x00,0x00,                      // No optical zoom supported 
        0x00,0x00,                      // No optical zoom supported 
        0x00,0x00,                      // No optical zoom supported 
#endif
        0x03,                           // Size of controls field for this terminal : 3 bytes 
                                        /* A bit set to 1 indicates that the mentioned Control is
                                         * supported for the video stream in the bmControls field
                                         * D0: Scanning Mode
                                         * D1: Auto-Exposure Mode
                                         * D2: Auto-Exposure Priority
                                         * D3: Exposure Time (Absolute)
                                         * D4: Exposure Time (Relative)
                                         * D5: Focus (Absolute)
                                         * D6: Focus (Relative)
                                         * D7: Iris (Absolute)
                                         * D8: Iris (Relative)
                                         * D9: Zoom (Absolute)
                                         * D10: Zoom (Relative)
                                         * D11: PanTilt (Absolute)
                                         * D12: PanTilt (Relative)
                                         * D13: Roll (Absolute)
                                         * D14: Roll (Relative)
                                         * D15: Reserved
                                         * D16: Reserved
                                         * D17: Focus, Auto
                                         * D18: Privacy
                                         * D19: Focus, Simple
                                         * D20: Window
                                         * D21: Region of Interest
                                         * D22 – D23: Reserved, set to zero
                                         */
#ifdef UVC_PTZ_SUPPORT
        0x00,0x0A,0x00,                 // bmControls field of camera terminal: PTZ supported 
#else
        0x00,0x00,0x00,                 // bmControls field of camera terminal: No controls supported 
#endif

        // Processing Unit Descriptor 
#ifdef FX3_UVC_1_0_SUPPORT
        0x0C,                           // Descriptor size 
#else
        0x0D,                           // Descriptor size 
#endif
        0x24,                           // Class specific interface desc type 
        0x05,                           // Processing Unit Descriptor type 
        0x02,                           // ID of this terminal 
        0x01,                           // Source ID : 1 : Connected to input terminal
        0x00,0x40,                      // Digital multiplier 
        0x03,                           // Size of controls field for this terminal : 3 bytes 
                                        /* A bit set to 1 in the bmControls field indicates that
                                         * the mentioned Control is supported for the video stream.
                                         * D0: Brightness
                                         * D1: Contrast
                                         * D2: Hue
                                         * D3: Saturation
                                         * D4: Sharpness
                                         * D5: Gamma
                                         * D6: White Balance Temperature
                                         * D7: White Balance Component
                                         * D8: Backlight Compensation
                                         * D9: Gain
                                         * D10: Power Line Frequency
                                         * D11: Hue, Auto
                                         * D12: White Balance Temperature, Auto
                                         * D13: White Balance Component, Auto
                                         * D14: Digital Multiplier
                                         * D15: Digital Multiplier Limit
                                         * D16: Analog Video Standard
                                         * D17: Analog Video Lock Status
                                         * D18: Contrast, Auto
                                         * D19 – D23: Reserved. Set to zero.
                                         */
        0x01,0x00,0x00,                 // bmControls field of processing unit: Brightness control supported 
        0x00,                           // String desc index : Not used 
#ifndef FX3_UVC_1_0_SUPPORT
        0x00,                           // Analog Video Standards Supported: None 
#endif

#ifdef UVC_EXTENSION_UNIT
        // Extension Unit Descriptor 
        0x1C,                           // Descriptor size 
        0x24,                           // Class specific interface desc type 
        0x06,                           // Extension Unit Descriptor type 
        0x03,                           // ID of this terminal 
        //static const GUID <<name>> =
        //{ 0xacb6890c, 0xa3b3, 0x4060,{ 0x8b, 0x9a, 0xdf, 0x34, 0xee, 0xf3, 0x9a, 0x2e } };
        0x0C, 0x89, 0xB6, 0xAC,         // GUID specific to AN75779 firmware. Obtained from Visual studio 
        0xB3, 0xA3, 0x60, 0x40,
        0x8B, 0x9A, 0xDF, 0x34,
        0xEE, 0xF3, 0x9A, 0x2E,
        0x01,                           // Number of controls in this terminal 
        0x01,                           // Number of input pins in this terminal 
        0x02,                           // Source ID : 2 : Connected to Proc Unit 
        0x03,                           // Size of controls field for this terminal : 3 bytes 
        0x01, 0x00, 0x00,               // Controls supported 
        0x00,                           // String descriptor index : Not used 
#else
        // Extension Unit Descriptor 
        0x1C,                           // Descriptor size 
        0x24,                           // Class specific interface desc type 
        0x06,                           // Extension Unit Descriptor type 
        0x03,                           // ID of this terminal 
        0xFF,0xFF,0xFF,0xFF,            // 16 byte GUID 
        0xFF,0xFF,0xFF,0xFF,
        0xFF,0xFF,0xFF,0xFF,
        0xFF,0xFF,0xFF,0xFF,
        0x00,                           // Number of controls in this terminal 
        0x01,                           // Number of input pins in this terminal 
        0x02,                           // Source ID : 2 : Connected to Proc Unit 
        0x03,                           // Size of controls field for this terminal : 3 bytes 
        0x00,0x00,0x00,                 // No controls supported 
        0x00,                           // String desc index : Not used 
#endif
        // Output Terminal Descriptor 
        0x09,                           // Descriptor size 
        0x24,                           // Class specific interface desc type 
        0x03,                           // Output Terminal Descriptor type 
        0x04,                           // ID of this terminal 
        0x01,0x01,                      // USB Streaming terminal type 
        0x00,                           // No association terminal 
        0x03,                           // Source ID : 3 : Connected to Extn Unit 
        0x00,                           // String desc index : Not used 

        // Video Control Status Interrupt Endpoint Descriptor 
        0x07,                           // Descriptor size 
        CY_U3P_USB_ENDPNT_DESCR,        // Endpoint Descriptor Type 
        CY_FX_EP_CONTROL_STATUS,        // Endpoint address and description 
        CY_U3P_USB_EP_INTR,             // Interrupt End point Type 
        WBVAL(64),                      // Max packet size = 64 bytes
        0x08,                           // Servicing interval : 8ms 

        // Class Specific Interrupt Endpoint Descriptor 
        0x05,                           // Descriptor size 
        0x25,                           // Class Specific Endpoint Descriptor Type 
        CY_U3P_USB_EP_INTR,             // End point Sub Type 
        WBVAL(64),                      // Max packet size = 64 bytes

        // Standard Video Streaming Interface Descriptor (Alternate Setting 0) 
        0x09,                           // Descriptor size 
        CY_U3P_USB_INTRFC_DESCR,        // Interface Descriptor type 
        0x01,                           // Interface number 
        0x00,                           // Alternate setting number 
        0x01,                           // Number of end points : Zero Bandwidth 
        0x0E,                           // Interface class : CC_VIDEO 
        0x02,                           // Interface sub class : CC_VIDEOSTREAMING 
        0x00,                           // Interface protocol code : Undefined 
        0x02,                           // Interface descriptor string index

        // Class-specific Video Streaming Input Header Descriptor
        0x0E,                           // Descriptor size 
        0x24,                           // Class-specific VS interface Type
        0x01,                           // Descriptor Subtype : Input Header
        0x01,                           // 1 format descriptor follows
#ifdef UVC_STILL_IMAGE
        WBVAL(81),                      // Total size of Class specific VS descriptor
#else
        WBVAL(71),
#endif
        CY_FX_EP_BULK_VIDEO,            // EP address for BULK video data 
        0x00,                           // No dynamic format change supported 
        0x04,                           // Output terminal ID : 4
#ifdef UVC_STILL_IMAGE
        0x02,                           // Still image capture method 2 supported 
        0x01,                           // Hardware trigger supported 
#else
        0x01,							// Still image capture method 1 supported
        0x00,
#endif
        0x00,                           // Hardware to initiate still image capture NOT supported 
        0x01,                           // Size of controls field : 1 byte 
        0x00,                           // Video Payload Format Flags

        // Class specific Uncompressed VS format descriptor
		0x1B,                           // Descriptor size
		0x24,                           // Class-specific VS interface Type
		0x04,                           // Subtype : uncompressed format interface
		0x01,                           // Format desciptor index (only one format is supported)
		0x01,                           // number of frame descriptor followed
#if defined(YUY2)
		'Y',  'U',  'Y',  '2',			// GUID for streaming-encoding format: YUY2
		0x00, 0x00, 0x10, 0x00,
		0x80, 0x00, 0x00, 0xAA,
		0x00, 0x38, 0x9B, 0x71,
#elif defined(YUV444)
		'Y',  'V',  '2',  '4',			// GUID for streaming-encoding format: YUV444 24bit
		0x00, 0x00, 0x10, 0x00,
		0x80, 0x00, 0x00, 0xAA,
		0x00, 0x38, 0x9B, 0x71,
#elif defined(YUV565)
		'Y',  'U',  'V',  'P',			// GUID for streaming-encoding format: YUV565 16bit
		0x00, 0x00, 0x10, 0x00,
		0x80, 0x00, 0x00, 0xAA,
		0x00, 0x38, 0x9B, 0x71,
#elif defined(RGB888)
		0x7D, 0xEB, 0x36, 0xE4,			// MEDIASUBTYPE_RGB888 GUID: E436EB7D-524F-11CE-9F53-0020AF0BA770
		0x4F, 0x52, 0xCE, 0x11,
		0x9F, 0x53, 0x00, 0x20,
		0xAF, 0x0B, 0xA7, 0x70,
#elif defined(RGB565)
		0x7B, 0xEB, 0x36, 0xE4,     	// GUID for streaming-encoding format: RGB565
		0x4F, 0x52, 0xCE, 0x11,			// MEDIASUBTYPE_RGB565 GUID: E436EB7B-524F-11CE-9F53-0020AF0BA770
		0x9F, 0x53, 0x00, 0x20,
		0xAF, 0x0B, 0xA7, 0x70,
#elif defined(GRAY16)
		'Y',  '1',  '6',  ' ',			// GUID for streaming-encoding format: GRAY16 (maybe replace ' ' by '0'
		0x00, 0x00, 0x10, 0x00,
		0x80, 0x00, 0x00, 0xAA,
		0x00, 0x38, 0x9B, 0x71,
#elif defined(GRAY8)
		'Y',  '8',  ' ',  ' ',			// GUID for streaming-encoding format: GRAY8 (maybe replace ' ' by '0'
		0x00, 0x00, 0x10, 0x00,
		0x80, 0x00, 0x00, 0xAA,
		0x00, 0x38, 0x9B, 0x71,
#elif defined(RGBP)
		'R',  'G',  'B',  'P',			// GUID for streaming-encoding format: RGBP (RGB565)
		0x00, 0x00, 0x10, 0x00,
		0x80, 0x00, 0x00, 0xAA,
		0x00, 0x38, 0x9B, 0x71,
#else
#error No Encoding selected!
#endif
		HS_COLORDEPTH,					// Number of bits per pixel used to specify color in the decoded video frame. 0 if not applicable
		0x01,                           // Optimum Frame Index for this stream: 1
		IMG_RATIO_X,                    // X dimension of the picture aspect ratio: Non-interlaced in progressive scan
		IMG_RATIO_Y,                    // Y dimension of the picture aspect ratio: Non-interlaced in progressive scan
		INTERLACED,                     // Interlace Flags: Progressive scanning, no interlace
		0x00,                           // duplication of the video stream restriction: 0 - no restriction

		// Class specific Uncompressed VS Frame descriptor
		0x26,                           // Descriptor size
		0x24,                           // Descriptor type
		0x05,                           // Subtype: uncompressed frame interface
		0x01,                           // Frame Descriptor Index
#ifdef UVC_STILL_IMAGE
		0x02,                           // Still image capture method 2 supported, fixed frame rate
#else
		0x01,							// Still image capture method 1 supported
#endif
		WBVAL(HS_WIDTH),
		WBVAL(HS_HEIGHT),
		DBVAL(HS_MIN_BITRATE),
		DBVAL(HS_MAX_BITRATE),
		DBVAL(HS_MAX_IMGSIZE),			// Maximum video or still frame size in bytes(Deprecated)
		DBVAL(HS_MIN_INTERVAL),        	// Default Frame Interval in 100ns units
		0x00,                          	// Frame interval(Frame Rate) types: Only one frame interval supported
		DBVAL(HS_MIN_INTERVAL),        	// Shortest Frame Interval
		DBVAL(HS_MAX_INTERVAL),        	// Longest Frame Interval
		DBVAL(HS_STP_INTERVAL),        	// Frame Interval Step

#ifdef UVC_STILL_IMAGE
		// Still Image descriptor
		0x0A,                           // Descriptor size
		0x24,                           // Descriptor type
		0x03,                           // VS_STILL_IMAGE_FRAME descriptor subtype
		0x00,                           // If method 2 of still image capture is used, this field shall be set to zero
		0x01,                           // Number of Image Size patterns of This format
		WBVAL(STILL_WIDTH),             // Width in pixel
		WBVAL(STILL_HEIGHT),            // Height in pixel
		0x00,                           // Number of Compression pattern of this format
#endif

		// VS Color Matching Descriptor
		0x06,							// Descriptor Size
		0x24,							// Descriptor Type
		0x0D,							// Subtype: Color Matching
		0x01,							// Color Primaries (BT.709, sRGB)
		0x01,							// Transfer Characteristics (BT.709)
		0x04,							// Matrix Coefficients (SMPTE 170M)

        // Endpoint Descriptor for BULK Streaming Video Data 
        0x07,                           // Descriptor size 
        CY_U3P_USB_ENDPNT_DESCR,        // Endpoint Descriptor Type 
        CY_FX_EP_BULK_VIDEO,            // Endpoint address and description 
        CY_U3P_USB_EP_BULK,             // BULK End point
        WBVAL(512),						// High speed max packet size is always 512 bytes.
        0x00,                           // Servicing interval for data transfers

// CDC
        // Interface Association Descriptor 
        0x08,                           // Descriptor Size 
		CY_FX_INTF_ASSN_DSCR_TYPE,      // Interface Association Descr Type: 11 
		0x02,                           // Interface number of first CDC interface
		0x02,                           // Number of CDC interface
		0x02,                           // CDC interface class code
		0x02,                           // Subclass code
		0x01,                           // Protocol : Not used 
		0x02,                           // String desc index for interface

		// Communication Interface descriptor 
		0x09,                           // Descriptor size 
		CY_U3P_USB_INTRFC_DESCR,        // Interface Descriptor type 
		0x02,                           // Interface number 
		0x00,                           // Alternate setting number 
		0x01,                           // Number of endpoints 
		0x02,                           // Interface class : Communication Interface 
		0x02,                           // Interface sub class 
		0x01,                           // Interface protocol code 
		0x02,                           // Interface descriptor string index

		// CDC Class-specific Descriptors 
		// Header functional Descriptor 
		0x05,                           // Descriptors length(5) 
		0x24,                           // Descriptor type : CS_Interface 
		0x00,                           // DescriptorSubType : Header Functional Descriptor 
		0x10,0x01,                      // bcdCDC : CDC Release Number 

		// Abstract Control Management Functional Descriptor 
		0x04,                           // Descriptors Length (4) 
		0x24,                           // bDescriptorType: CS_INTERFACE 
		0x02,                           // bDescriptorSubType: Abstract Control Model Functional Descriptor 
		0x02,                           // bmCapabilities: Supports the request combination of Set_Line_Coding, Set_Control_Line_State, Get_Line_Coding and the notification Serial_State

		// Union Functional Descriptor 
		0x05,                           // Descriptors Length (5) 
		0x24,                           // bDescriptorType: CS_INTERFACE 
		0x06,                           // bDescriptorSubType: Union Functional Descriptor 
		0x02,                           // bMasterInterface 
		0x03,                           // bSlaveInterface

		// Call Management Functional Descriptor 
		0x05,                           //  Descriptors Length (5) 
		0x24,                           //  bDescriptorType: CS_INTERFACE 
		0x01,                           //  bDescriptorSubType: Call Management Functional Descriptor 
		0x00,                           //  bmCapabilities: Device sends/receives call management information only over the Communication Class Interface.
		0x03,                           //  Interface Number of Data Class interface

		// Endpoint Descriptor(Interrupt) 
		0x07,                           // Descriptor size 
		CY_U3P_USB_ENDPNT_DESCR,        // Endpoint descriptor type 
		CY_FX_EP_CDC_INTERRUPT,         // Endpoint address and description 
		CY_U3P_USB_EP_INTR,             // Interrupt endpoint type 
		WBVAL(64),                      // Max packet size = 64 bytes
		0x02,                           // Servicing interval for data transfers 

		// Data Interface Descriptor 
		0x09,                           // Descriptor size 
		CY_U3P_USB_INTRFC_DESCR,        // Interface Descriptor type 
		0x03,                           // Interface number 
		0x00,                           // Alternate setting number 
		0x02,                           // Number of endpoints 
		0x0A,                           // Interface class: Data interface 
		0x00,                           // Interface sub class 
		0x00,                           // Interface protocol code 
		0x02,                           // Interface descriptor string index

		// Endpoint Descriptor(BULK-PRODUCER) 
		0x07,                           // Descriptor size 
		CY_U3P_USB_ENDPNT_DESCR,        // Endpoint descriptor type 
		CY_FX_EP_CDC_PRODUCER,          // Endpoint address and description 
		CY_U3P_USB_EP_BULK,             // BULK endpoint type 
		WBVAL(512),                     // Max packet size = 512 bytes
		0x00,                           // Servicing interval for data transfers 

		// Endpoint Descriptor(BULK-CONSUMER)
		0x07,                           // Descriptor size 
		CY_U3P_USB_ENDPNT_DESCR,        // Endpoint descriptor type 
		CY_FX_EP_CDC_CONSUMER,          // Endpoint address and description 
		CY_U3P_USB_EP_BULK,             // Bulk endpoint type 
		WBVAL(512),                     // Max packet size = 512 bytes
		0x00,                           // Servicing interval for data transfers

#ifdef USE_DEBUG_PORT
// DEBUG
		// Interface Association Descriptor 
		0x08,                           // Descriptor Size 
		CY_FX_INTF_ASSN_DSCR_TYPE,      // Interface Association Descr Type: 11 
		0x04,                           // Interface number of first CDC interface
		0x02,                           // Number of CDC interfaces
		0x02,                           // CDC interface class code
		0x02,                           // Subclass code 
		0x01,                           // Protocol : Not used 
		0x03,                           // String desc index for interface

		// Communication Interface descriptor 
		0x09,                           // Descriptor size 
		CY_U3P_USB_INTRFC_DESCR,        // Interface Descriptor type 
		0x04,                           // Interface number
		0x00,                           // Alternate setting number 
		0x01,                           // Number of endpoints 
		0x02,                           // Interface class: Communication interface
		0x02,                           // Interface sub class 
		0x01,                           // Interface protocol code 
		0x03,                           // Interface descriptor string index

		// CDC Class-specific Descriptors 
		// Header functional Descriptor 
		0x05,                           // Descriptors length(5) 
		0x24,                           // Descriptor type : CS_Interface 
		0x00,                           // DescriptorSubType : Header Functional Descriptor 
		0x10,0x01,                      // bcdCDC : CDC Release Number 

		// Abstract Control Management Functional Descriptor 
		0x04,                           // Descriptors Length (4) 
		0x24,                           // bDescriptorType: CS_INTERFACE 
		0x02,                           // bDescriptorSubType: Abstract Control Model Functional Descriptor 
		0x02,                           // bmCapabilities: Supports the request combination of Set_Line_Coding, Set_Control_Line_State, Get_Line_Coding and the notification Serial_State

		// Union Functional Descriptor 
		0x05,                           // Descriptors Length (5) 
		0x24,                           // bDescriptorType: CS_INTERFACE 
		0x06,                           // bDescriptorSubType: Union Functional Descriptor 
		0x04,                           // bMasterInterface 
		0x05,                           // bSlaveInterface

		// Call Management Functional Descriptor 
		0x05,                           //  Descriptors Length (5) 
		0x24,                           //  bDescriptorType: CS_INTERFACE 
		0x01,                           //  bDescriptorSubType: Call Management Functional Descriptor 
		0x00,                           //  bmCapabilities: Device sends/receives call management information only over the Communication Class Interface.
		0x05,                           //  Interface Number of Data Class interface

		// Endpoint Descriptor(Interrupt) 
		0x07,                           // Descriptor size 
		CY_U3P_USB_ENDPNT_DESCR,        // Endpoint descriptor type 
		CY_FX_EP_DBG_INTERRUPT,         // Endpoint address and description 
		CY_U3P_USB_EP_INTR,             // Interrupt endpoint type 
		WBVAL(64),                      // Max packet size = 16 bytes
		0x02,                           // Servicing interval for data transfers 

		// Data Interface Descriptor 
		0x09,                           // Descriptor size 
		CY_U3P_USB_INTRFC_DESCR,        // Interface Descriptor type 
		0x05,                           // Interface number 
		0x00,                           // Alternate setting number 
		0x02,                           // Number of endpoints 
		0x0A,                           // Interface class: Data interface 
		0x00,                           // Interface sub class 
		0x00,                           // Interface protocol code 
		0x03,                           // Interface descriptor string index

		// Endpoint Descriptor(BULK-PRODUCER) 
		0x07,                           // Descriptor size 
		CY_U3P_USB_ENDPNT_DESCR,        // Endpoint descriptor type 
		CY_FX_EP_DBG_PRODUCER,          // Endpoint address and description 
		CY_U3P_USB_EP_BULK,             // BULK endpoint type 
		WBVAL(512),                     // Max packet size = 64 bytes
		0x00,                           // Servicing interval for data transfers 

		// Endpoint Descriptor(BULK-CONSUMER)
		0x07,                           // Descriptor size 
		CY_U3P_USB_ENDPNT_DESCR,        // Endpoint descriptor type 
		CY_FX_EP_DBG_CONSUMER,          // Endpoint address and description 
		CY_U3P_USB_EP_BULK,             // Bulk endpoint type 
		WBVAL(512),                     // Max packet size = 64 bytes
		0x00,                           // Servicing interval for data transfers
#endif // USE_DEBUG_PORT
    };

// Super Speed Configuration Descriptor 
const uint8_t CyFxUSBSSConfigDscr[] __attribute__ ((aligned (32))) =
    {
        // Configuration Descriptor Type 
        0x09,                           // Descriptor Size 
        CY_U3P_USB_CONFIG_DESCR,        // Configuration Descriptor Type 
#ifdef FX3_UVC_1_0_SUPPORT
	#ifdef USE_DEBUG_PORT
#ifdef UVC_STILL_IMAGE
        WBVAL(447),						// Length of this descriptor and all sub descriptors
#else
        WBVAL(437),
#endif
        0x06,                           // Number of interfaces
	#else
#ifdef UVC_STILL_IMAGE
        WBVAL(363),
#else
        WBVAL(353),
#endif
        0x04,                           // Number of interfaces
	#endif
#else
	#ifdef USE_DEBUG_PORT
#ifdef UVC_STILL_IMAGE
        WBVAL(448),						// Length of this descriptor and all sub descriptors
#else
        WBVAL(438),
#endif
        0x06,                           // Number of interfaces
	#else
#ifdef UVC_STILL_IMAGE
        WBVAL(364),
#else
        WBVAL(354),
#endif
        0x04,                           // Number of interfaces
	#endif
#endif
        0x01,                           // Configuration number 
        0x00,                           // Configuration string index 
        0x80,                           // Config characteristics - Bus powered 
        0x32,                           // Max power consumption of device (in 8mA unit) : 400mA 

        // Interface Association Descriptor 
        0x08,                           // Descriptor Size 
        CY_FX_INTF_ASSN_DSCR_TYPE,      // Interface Association Descr Type: 11 
        0x00,                           // interface number of first VideoControl interface
        0x02,                           // Number of Video interface
        0x0E,                           // CC_VIDEO : Video interface class code
        0x03,                           // SC_VIDEO_INTERFACE_COLLECTION : Subclass code 
        0x00,                           // Protocol : Not used 
        0x02,                           // String desc index for interface 

        // Standard Video Control Interface Descriptor 
        0x09,                           // Descriptor size 
        CY_U3P_USB_INTRFC_DESCR,        // Interface Descriptor type 
        0x00,                           // Interface number 
        0x00,                           // Alternate setting number 
        0x01,                           // Number of end points 
        0x0E,                           // CC_VIDEO : Interface class 
        0x01,                           // CC_VIDEOCONTROL : Interface sub class 
        0x00,                           // Interface protocol code 
        0x02,                           // Interface descriptor string index 

        // Class specific VC Interface Header Descriptor 
        0x0D,                           // Descriptor size 
        0x24,                           // Class Specific interface Header Descriptor type
        0x01,                           // Descriptor Sub type : VC_HEADER 
#ifdef FX3_UVC_1_0_SUPPORT
        0x00, 0x01,                     // Revision of UVC class spec: 1.0 - Legacy version 
        WBVAL(80),                      // Total Size of class specific descriptors (till Output terminal)
#else
        0x10, 0x01,                     // Revision of UVC class spec: 1.1 - Minimum version required for USB Compliance. Not supported on Windows XP
        WBVAL(81),                      // Total Size of class specific descriptors (till Output terminal)
#endif
        DBVAL(48000000),				// Clock frequency : 48MHz(Deprecated)
        0x01,                           // Number of streaming interfaces 
        0x01,                           // Video streaming interface 1 belongs to VC interface

        // Input (Camera) Terminal Descriptor 
        0x12,                           // Descriptor size 
        0x24,                           // Class specific interface desc type 
        0x02,                           // Input Terminal Descriptor type 
        0x01,                           // ID of this terminal 
        0x01,0x02,                      // Camera terminal type 
        0x00,                           // No association terminal 
        0x00,                           // String desc index : Not used 
#ifdef UVC_PTZ_SUPPORT
        WBVAL(wObjectiveFocalLengthMin),
        WBVAL(wObjectiveFocalLengthMax),
        WBVAL(wOcularFocalLength),
#else
        0x00,0x00,                      // No optical zoom supported 
        0x00,0x00,                      // No optical zoom supported 
        0x00,0x00,                      // No optical zoom supported 
#endif
        0x03,                           // Size of controls field for this terminal : 3 bytes 
                                        /* A bit set to 1 in the bmControls field indicates that
                                         * the mentioned Control is supported for the video stream.
                                         * D0: Scanning Mode
                                         * D1: Auto-Exposure Mode
                                         * D2: Auto-Exposure Priority
                                         * D3: Exposure Time (Absolute)
                                         * D4: Exposure Time (Relative)
                                         * D5: Focus (Absolute)
                                         * D6: Focus (Relative)
                                         * D7: Iris (Absolute)
                                         * D8: Iris (Relative)
                                         * D9: Zoom (Absolute)
                                         * D10: Zoom (Relative)
                                         * D11: PanTilt (Absolute)
                                         * D12: PanTilt (Relative)
                                         * D13: Roll (Absolute)
                                         * D14: Roll (Relative)
                                         * D15: Reserved
                                         * D16: Reserved
                                         * D17: Focus, Auto
                                         * D18: Privacy
                                         * D19: Focus, Simple
                                         * D20: Window
                                         * D21: Region of Interest
                                         * D22 – D23: Reserved, set to zero
                                         */
#ifdef UVC_PTZ_SUPPORT
        0x00,0x0A,0x00,                 // bmControls field of camera terminal: PTZ supported 
#else
        0x00,0x00,0x00,                 // bmControls field of camera terminal: No controls supported 
#endif

        // Processing Unit Descriptor 
#ifdef FX3_UVC_1_0_SUPPORT
        0x0C,                           // Descriptor size 
#else
        0x0D,                           // Descriptor size 
#endif
        0x24,                           // Class specific interface desc type 
        0x05,                           // Processing Unit Descriptor type 
        0x02,                           // ID of this terminal 
        0x01,                           // Source ID : 1 : Connected to input terminal 
        0x00,0x40,                      // Digital multiplier 
        0x03,                           // Size of controls field for this terminal : 3 bytes 
        0x01,0x00,0x00,                 // bmControls field of processing unit: Brightness control supported 
        0x00,                           // String desc index : Not used 
#ifndef FX3_UVC_1_0_SUPPORT
        0x00,                           // Analog Video Standards Supported: None 
#endif

#ifdef UVC_EXTENSION_UNIT
        // Extension Unit Descriptor 
        0x1C,                           // Descriptor size 
        0x24,                           // Class specific interface desc type 
        0x06,                           // Extension Unit Descriptor type 
        0x03,                           // ID of this terminal 
        //static const GUID <<name>> =
        //{ 0xacb6890c, 0xa3b3, 0x4060,{ 0x8b, 0x9a, 0xdf, 0x34, 0xee, 0xf3, 0x9a, 0x2e } };
        0x0C, 0x89, 0xB6, 0xAC,         // GUID specific to AN75779 firmware. Obtained from Visual studio 
        0xB3, 0xA3, 0x60, 0x40,
        0x8B, 0x9A, 0xDF, 0x34,
        0xEE, 0xF3, 0x9A, 0x2E,
        0x01,                           // Number of controls in this terminal 
        0x01,                           // Number of input pins in this terminal 
        0x02,                           // Source ID : 2 : Connected to Proc Unit 
        0x03,                           // Size of controls field for this terminal : 3 bytes 
        0x01, 0x00, 0x00,               // Controls supported 
        0x00,                           // String descriptor index : Not used 
#else
        // Extension Unit Descriptor 
        0x1C,                           // Descriptor size 
        0x24,                           // Class specific interface desc type 
        0x06,                           // Extension Unit Descriptor type 
        0x03,                           // ID of this terminal 
        0xFF,0xFF,0xFF,0xFF,            // 16 byte GUID 
        0xFF,0xFF,0xFF,0xFF,
        0xFF,0xFF,0xFF,0xFF,
        0xFF,0xFF,0xFF,0xFF,
        0x00,                           // Number of controls in this terminal 
        0x01,                           // Number of input pins in this terminal 
        0x02,                           // Source ID : 2 : Connected to Proc Unit 
        0x03,                           // Size of controls field for this terminal : 3 bytes 
        0x00,0x00,0x00,                 // No controls supported 
        0x00,                           // String desc index : Not used 
#endif

        // Output Terminal Descriptor 
        0x09,                           // Descriptor size 
        0x24,                           // Class specific interface desc type 
        0x03,                           // Output Terminal Descriptor type 
        0x04,                           // ID of this terminal 
        0x01,0x01,                      // USB Streaming terminal type 
        0x00,                           // No association terminal 
        0x03,                           // Source ID : 3 : Connected to Extn Unit 
        0x00,                           // String desc index : Not used 

        // Video Control Status Interrupt Endpoint Descriptor 
        0x07,                           // Descriptor size 
        CY_U3P_USB_ENDPNT_DESCR,        // Endpoint Descriptor Type 
        CY_FX_EP_CONTROL_STATUS,        // Endpoint address and description 
        CY_U3P_USB_EP_INTR,             // Interrupt End point Type 
        WBVAL(64),                      // Max packet size = 64 bytes
        0x01,                           // Servicing interval 

        // Super Speed Endpoint Companion Descriptor 
        0x06,                           // Descriptor size 
        CY_U3P_SS_EP_COMPN_DESCR,       // SS Endpoint Companion Descriptor Type 
        0x00,                           // Max no. of packets in a Burst : 1 
        0x00,                           // Attribute: N.A. 
        WBVAL(1024),                    // Bytes per interval:1024

        // Class Specific Interrupt Endpoint Descriptor 
        0x05,                           // Descriptor size 
        0x25,                           // Class Specific Endpoint Descriptor Type 
        CY_U3P_USB_EP_INTR,             // End point Sub Type 
        WBVAL(64),                      // Max packet size = 64 bytes

        // Standard Video Streaming Interface Descriptor (Alternate Setting 0) 
        0x09,                           // Descriptor size 
        CY_U3P_USB_INTRFC_DESCR,        // Interface Descriptor type 
        0x01,                           // Interface number 
        0x00,                           // Alternate setting number 
        0x01,                           // Number of end points 
        0x0E,                           // Interface class : CC_VIDEO 
        0x02,                           // Interface sub class : CC_VIDEOSTREAMING 
        0x00,                           // Interface protocol code : Undefined 
        0x00,                           // Interface descriptor string index 

        // Class-specific Video Streaming Input Header Descriptor
        0x0E,                           // Descriptor size 
        0x24,                           // Class-specific VS interface Type
        0x01,                           // Descriptotor Subtype : Input Header 
        0x01,                           // 1 format desciptor follows 
#ifdef UVC_STILL_IMAGE
        WBVAL(118),                      // Total size of Class specific VS descr
#else
        WBVAL(105),
#endif
//        0x47,0x00,                      // Total size of Class specific VS descr
        CY_FX_EP_BULK_VIDEO,            // EP address for BULK video data 
        0x00,                           // No dynamic format change supported 
        0x04,                           // Output terminal ID : 4 
#ifdef UVC_STILL_IMAGE
        0x02,                           // Still image capture method 2 supported 
        0x01,                           // Hardware trigger supported
#else
        0x01,							// Still image capture method 1 supported
        0x00,
#endif
        0x00,                           // Hardware to initiate still image capture NOT supported 
        0x01,                           // Size of controls field : 1 byte 
        0x00,                           // D2 : Compression quality supported 

        // Class specific Uncompressed VS format descriptor 
        0x1B,                           // Descriptor size 
        0x24,                           // Class-specific VS interface Type
        0x04,                           // Subtype : uncompressed format interface
        0x01,                           // Format desciptor index 
        0x02,                           // Number of frame descriptors followed
//      0x01,	                        // Number of frame descriptors followed
#if defined(YUY2)
		'Y',  'U',  'Y',  '2',			// GUID for streaming-encoding format: YUY2
		0x00, 0x00, 0x10, 0x00,
		0x80, 0x00, 0x00, 0xAA,
		0x00, 0x38, 0x9B, 0x71,
#elif defined(YUV444)
		'Y',  'V',  '2',  '4',			// GUID for streaming-encoding format: YUV444 24bit
		0x00, 0x00, 0x10, 0x00,
		0x80, 0x00, 0x00, 0xAA,
		0x00, 0x38, 0x9B, 0x71,
#elif defined(YUV565)
		'Y',  'U',  'V',  'P',			// GUID for streaming-encoding format: YUV565 16bit
		0x00, 0x00, 0x10, 0x00,
		0x80, 0x00, 0x00, 0xAA,
		0x00, 0x38, 0x9B, 0x71,
#elif defined(RGB888)
		0x7D, 0xEB, 0x36, 0xE4,			// MEDIASUBTYPE_RGB888 GUID: E436EB7D-524F-11CE-9F53-0020AF0BA770
		0x4F, 0x52, 0xCE, 0x11,
		0x9F, 0x53, 0x00, 0x20,
		0xAF, 0x0B, 0xA7, 0x70,
#elif defined(RGB565)
		0x7B, 0xEB, 0x36, 0xE4,     	// GUID for streaming-encoding format: RGB565
		0x4F, 0x52, 0xCE, 0x11,			// MEDIASUBTYPE_RGB565 GUID: E436EB7B-524F-11CE-9F53-0020AF0BA770
		0x9F, 0x53, 0x00, 0x20,
		0xAF, 0x0B, 0xA7, 0x70,
#elif defined(GRAY16)
		'Y',  '1',  '6',  ' ',			// GUID for streaming-encoding format: GRAY16 (maybe replace ' ' by '0'
		0x00, 0x00, 0x10, 0x00,
		0x80, 0x00, 0x00, 0xAA,
		0x00, 0x38, 0x9B, 0x71,
#elif defined(GRAY8)
		'Y',  '8',  ' ',  ' ',			// GUID for streaming-encoding format: GRAY8 (maybe replace ' ' by '0'
		0x00, 0x00, 0x10, 0x00,
		0x80, 0x00, 0x00, 0xAA,
		0x00, 0x38, 0x9B, 0x71,
#elif defined(RGBP)
		'R',  'G',  'B',  'P',			// GUID for streaming-encoding format: RGBP (RGB565)
		0x00, 0x00, 0x10, 0x00,
		0x80, 0x00, 0x00, 0xAA,
		0x00, 0x38, 0x9B, 0x71,
#else
#error No Encoding selected!
#endif
        SS_COLORDEPTH,					// Number of bits per pixel used to specify color in the decoded video frame. 0 if not applicable
		0x01,                           // Optimum Frame Index for this stream: 1
		IMG_RATIO_X,                    // X dimension of the picture aspect ratio: Non-interlaced in progressive scan
		IMG_RATIO_Y,                    // Y dimension of the picture aspect ratio: Non-interlaced in progressive scan        0x00,                           // Interlace Flags: Progressive scanning, no interlace
		INTERLACED,                     // Interlace Flags: Progressive scanning, no interlace
        0x00,                           // duplication of the video stream restriction: 0 - no restriction 

        // Class specific Uncompressed VS frame descriptor - 1 Full Frame
        0x26,                           // Descriptor size
        0x24,                           // Descriptor type
        0x05,                           // Subtype: uncompressed frame interface
        UVC_SS_FULL_FRAME_INDEX,        // Frame Descriptor Index
#ifdef UVC_STILL_IMAGE
        0x02,                           // Still image capture method 2 supported
#else
        0x01,							// Still image capture method 1 supported
#endif
        WBVAL(SS_WIDTH),                // Width in pixel
        WBVAL(SS_HEIGHT),               // Height in pixel
        DBVAL(SS_MIN_BITRATE),			// Min bit rate bits/s
        DBVAL(SS_MAX_BITRATE),         	// Max bit rate bits/s
        DBVAL(SS_MAX_IMGSIZE),         	// Maximum video or still frame size in bytes(Deprecated)
        DBVAL(SS_MIN_INTERVAL),        	// Default Frame Interval in 100ns units
        0x00,							// Frame interval(Frame Rate) types: Continuous interval
        DBVAL(SS_MIN_INTERVAL),			// Shortest Frame Interval
        DBVAL(SS_MAX_INTERVAL),			// Longest Frame Interval
        DBVAL(SS_STP_INTERVAL),			// Frame Interval Step

        // Class specific Uncompressed VS frame descriptor - 2 Half Frame
        0x26,                           // Descriptor size
        0x24,                           // Descriptor type
        0x05,                           // Subtype: uncompressed frame interface
        UVC_SS_HALF_FRAME_INDEX,        // Frame Descriptor Index
#ifdef UVC_STILL_IMAGE
        0x02,                           // Still image capture method 2 supported
#else
        0x01,							// Still image capture method 1 supported
#endif
        WBVAL(SS_WIDTH/2),              // Width in pixel
        WBVAL(SS_HEIGHT/2),             // Height in pixel
        DBVAL(SS_MIN_BITRATE/4),		// Min bit rate bits/s
        DBVAL(SS_MAX_BITRATE/4),        // Max bit rate bits/s
        DBVAL(SS_MAX_IMGSIZE/4),        // Maximum video or still frame size in bytes(Deprecated)
        DBVAL(SS_MIN_INTERVAL),         // Default Frame Interval in 100ns units
        0x00,							// Frame interval(Frame Rate) types: Continuous interval
        DBVAL(SS_MIN_INTERVAL),			// Shortest Frame Interval
        DBVAL(SS_MAX_INTERVAL),			// Longest Frame Interval
        DBVAL(SS_STP_INTERVAL),			// Frame Interval Step

#ifdef UVC_STILL_IMAGE
        //Still Image Descriptor
        0x0A,                           // Descriptor size
        0x24,                           // Descriptor type 
        0x03,                           // VS_STILL_IMAGE_FRAME descriptor subtype 
        0x00,                           // If method 2 of still image capture is used, this field shall be set to zero 
        0x01,                           // Number of Image Size patterns of this format
        WBVAL(STILL_WIDTH),             // Width in pixel
        WBVAL(STILL_HEIGHT),            // Height in pixel
        0x00,                           // Number of Compression pattern of this format 
#endif

		// VS Color Matching Descriptor
		0x06,							// Descriptor Size
		0x24,							// Descriptor Type
		0x0D,							// Subtype: Color Matching
		0x01,							// Color Primaries (BT.709, sRGB)
		0x01,							// Transfer Characteristics (BT.709)
		0x04,							// Matrix Coefficients (SMPTE 170M)

        // Endpoint Descriptor for BULK Streaming Video Data 
        0x07,                           // Descriptor size 
        CY_U3P_USB_ENDPNT_DESCR,        // Endpoint Descriptor Type 
        CY_FX_EP_BULK_VIDEO,            // Endpoint address and description 
        CY_U3P_USB_EP_BULK,             // BULK End point
        WBVAL(CY_FX_EP_BULK_VIDEO_PKT_SIZE), // EP MaxPcktSize: 1024B
        0x00,                           // Servicing interval for data transfers

        // Super Speed Endpoint Companion Descriptor 
        0x06,                           // Descriptor size
        CY_U3P_SS_EP_COMPN_DESCR,       // SS Endpoint Companion Descriptor Type
        (CY_FX_EP_BULK_VIDEO_PKTS_COUNT-1), // Max number of packets per burst: 16
        0x00,                           // Attribute: Streams not defined
        0x00, 0x00,                     // No meaning for bulk

// CDC
        // Interface Association Descriptor 
        0x08,                           // Descriptor Size
		CY_FX_INTF_ASSN_DSCR_TYPE,      // Interface Association Descr Type: 11 
		0x02,                           // Interface number of first CDC interface
		0x02,                           // Number of CDC interfaces
		0x02,                           // CDC interface class code
		0x02,                           // Subclass code
		0x01,                           // Protocol : Not used 
		0x02,                           // String desc index for interface

		// Communication Interface descriptor 
		0x09,                           // Descriptor size 
		CY_U3P_USB_INTRFC_DESCR,        // Interface Descriptor type 
		0x02,                           // Interface number 
		0x00,                           // Alternate setting number 
		0x01,                           // Number of endpoints 
		0x02,                           // Interface class : Communication Interface
		0x02,                           // Interface sub class 
		0x01,                           // Interface protocol code 
		0x02,                           // Interface descriptor string index

		// CDC Class-specific Descriptors 
		// Header functional Descriptor 
		0x05,                           // Descriptors length(5) 
		0x24,                           // Descriptor type : CS_Interface 
		0x00,                           // DescriptorSubType : Header Functional Descriptor 
		0x10,0x01,                      // bcdCDC : CDC Release Number 

		// Abstract Control Management Functional Descriptor 
		0x04,                           // Descriptors Length (4) 
		0x24,                           // bDescriptorType: CS_INTERFACE 
		0x02,                           // bDescriptorSubType: Abstract Control Model Functional Descriptor 
		0x02,                           // bmCapabilities: Supports the request combination of Set_Line_Coding, Set_Control_Line_State, Get_Line_Coding and the notification Serial_State

		// Union Functional Descriptor 
		0x05,                           // Descriptors Length (5) 
		0x24,                           // bDescriptorType: CS_INTERFACE 
		0x06,                           // bDescriptorSubType: Union Functional Descriptor 
		0x02,                           // bMasterInterface 
		0x03,                           // bSlaveInterface

		// Call Management Functional Descriptor 
		0x05,                           //  Descriptors Length (5) 
		0x24,                           //  bDescriptorType: CS_INTERFACE 
		0x01,                           //  bDescriptorSubType: Call Management Functional Descriptor 
		0x00,                           //  bmCapabilities: Device sends/receives call management information only over the Communication Class Interface.
		0x03,                           //  Interface Number of Data Class interface

		// Endpoint Descriptor(Interrupt) 
		0x07,                           // Descriptor size 
		CY_U3P_USB_ENDPNT_DESCR,        // Endpoint descriptor type 
		CY_FX_EP_CDC_INTERRUPT,			// Endpoint address and description 
		CY_U3P_USB_EP_INTR,             // Interrupt endpoint type 
		WBVAL(64),                  	// Max packet size = 64 bytes
		0x02,                           // Servicing interval for data transfers 

		// Super speed endpoint companion descriptor for interrupt endpoint 
		0x06,                           // Descriptor size 
		CY_U3P_SS_EP_COMPN_DESCR,       // SS endpoint companion descriptor type 
		0x00,                           // Max no. of packets in a Burst : 1 
		0x00,                           // Mult.: Max number of packets : 1 
		WBVAL(64),                    	// Bytes per interval : 64

		// Data Interface Descriptor 
		0x09,                           // Descriptor size 
		CY_U3P_USB_INTRFC_DESCR,        // Interface Descriptor type 
		0x03,                           // Interface number 
		0x00,                           // Alternate setting number 
		0x02,                           // Number of endpoints 
		0x0A,                           // Interface class: Data interface 
		0x00,                           // Interface sub class 
		0x00,                           // Interface protocol code 
		0x02,							// Interface descriptor string index

		// Endpoint Descriptor(BULK-PRODUCER) 
		0x07,                           // Descriptor size 
		CY_U3P_USB_ENDPNT_DESCR,        // Endpoint descriptor type 
		CY_FX_EP_CDC_PRODUCER,          // Endpoint address and description 
		CY_U3P_USB_EP_BULK,             // BULK endpoint type 
		WBVAL(1024),                    // Max packet size = 1024 bytes
		0x00,                           // Servicing interval for data transfers 

		// Super speed endpoint companion descriptor for producer ep 
		0x06,                           // Descriptor size 
		CY_U3P_SS_EP_COMPN_DESCR,       // SS endpoint companion descriptor type 
		0x00,                           // Max no. of packets in a burst : 1 
		0x00,                           // Mult.: Max number of packets : 1 
		0x00, 0x00,                     // No meaning for bulk

		// Endpoint Descriptor(BULK-CONSUMER)
		0x07,                           // Descriptor size 
		CY_U3P_USB_ENDPNT_DESCR,        // Endpoint descriptor type 
		CY_FX_EP_CDC_CONSUMER,          // Endpoint address and description 
		CY_U3P_USB_EP_BULK,             // BULK endpoint type 
		WBVAL(1024),                    // Max packet size = 1024 bytes
		0x00,                           // Servicing interval for data transfers 

		// Super speed endpoint companion descriptor for consumer ep 
		0x06,                           // Descriptor size 
		CY_U3P_SS_EP_COMPN_DESCR,       // SS endpoint companion descriptor type 
		0x00,                           // Max no. of packets in a burst : 1 
		0x00,                           // Mult.: Max number of packets : 1
		0x00, 0x00,                     // No meaning for bulk

#ifdef USE_DEBUG_PORT
// DEBUG (84 bytes)
		// Interface Association Descriptor
		0x08,                           // Descriptor Size
		CY_FX_INTF_ASSN_DSCR_TYPE,      // Interface Association Descr Type: 11
		0x04,                           // Interface number of first CDC interface
		0x02,                           // Number of CDC interfaces
		0x02,                           // CDC interface class code
		0x02,                           // Subclass code
		0x01,                           // Protocol : Not used
		0x03,                           // String desc index for interface

		// Communication Interface descriptor
		0x09,                           // Descriptor size
		CY_U3P_USB_INTRFC_DESCR,        // Interface Descriptor type
		0x04,                           // Interface number
		0x00,                           // Alternate setting number
		0x01,                           // Number of endpoints
		0x02,                           // Interface class: Communication interface
		0x02,                           // Interface sub class
		0x01,                           // Interface protocol code
		0x03,                           // Interface descriptor string index

		// CDC Class-specific Descriptors
		// Header functional Descriptor
		0x05,                           // Descriptors length(5)
		0x24,                           // Descriptor type : CS_Interface
		0x00,                           // DescriptorSubType : Header Functional Descriptor
		0x10,0x01,                      // bcdCDC : CDC Release Number

		// Abstract Control Management Functional Descriptor
		0x04,                           // Descriptors Length (4)
		0x24,                           // bDescriptorType: CS_INTERFACE
		0x02,                           // bDescriptorSubType: Abstract Control Model Functional Descriptor
		0x02,                           // bmCapabilities: Supports the request combination of Set_Line_Coding, Set_Control_Line_State, Get_Line_Coding and the notification Serial_State

		// Union Functional Descriptor
		0x05,                           // Descriptors Length (5)
		0x24,                           // bDescriptorType: CS_INTERFACE
		0x06,                           // bDescriptorSubType: Union Functional Descriptor
		0x04,                           // bMasterInterface
		0x05,                           // bSlaveInterface

		// Call Management Functional Descriptor
		0x05,                           //  Descriptors Length (5)
		0x24,                           //  bDescriptorType: CS_INTERFACE
		0x01,                           //  bDescriptorSubType: Call Management Functional Descriptor
		0x00,                           //  bmCapabilities: Device sends/receives call management information only over the Communication Class Interface.
		0x05,                           //  Interface Number of Data Class interface

		// Endpoint Descriptor(Interrupt)
		0x07,                           // Descriptor size
		CY_U3P_USB_ENDPNT_DESCR,        // Endpoint descriptor type
		CY_FX_EP_DBG_INTERRUPT,         // Endpoint address and description
		CY_U3P_USB_EP_INTR,             // Interrupt endpoint type
		WBVAL(64),	                    // Max packet size = 64 bytes
		0x02,                           // Servicing interval for data transfers

		// Super speed endpoint companion descriptor for interrupt endpoint
		0x06,                           // Descriptor size
		CY_U3P_SS_EP_COMPN_DESCR,       // SS endpoint companion descriptor type
		0x00,                           // Max no. of packets in a Burst : 1
		0x00,                           // Mult.: Max number of packets : 1
		WBVAL(64),                   	// Bytes per interval : 1024

		// Data Interface Descriptor
		0x09,                           // Descriptor size
		CY_U3P_USB_INTRFC_DESCR,        // Interface Descriptor type
		0x05,                           // Interface number
		0x00,                           // Alternate setting number
		0x02,                           // Number of endpoints
		0x0A,                           // Interface class: Data interface
		0x00,                           // Interface sub class
		0x00,                           // Interface protocol code
		0x03,                           // Interface descriptor string index

		// Endpoint Descriptor(BULK-PRODUCER)
		0x07,                           // Descriptor size
		CY_U3P_USB_ENDPNT_DESCR,        // Endpoint descriptor type
		CY_FX_EP_DBG_PRODUCER,          // Endpoint address and description
		CY_U3P_USB_EP_BULK,             // BULK endpoint type
		WBVAL(1024),                    // Max packet size = 1024 bytes
		0x00,                           // Servicing interval for data transfers

		// Super speed endpoint companion descriptor for producer ep
		0x06,                           // Descriptor size
		CY_U3P_SS_EP_COMPN_DESCR,       // SS endpoint companion descriptor type
		0x00,                           // Max no. of packets in a burst : 1
		0x00,                           // Mult.: Max number of packets : 1
		0x00, 0x00,                     // No meaning for bulk

		// Endpoint Descriptor(BULK- CONSUMER)
		0x07,                           // Descriptor size
		CY_U3P_USB_ENDPNT_DESCR,        // Endpoint descriptor type
		CY_FX_EP_DBG_CONSUMER,          // Endpoint address and description
		CY_U3P_USB_EP_BULK,             // Bulk endpoint type
		WBVAL(1024),                    // Max packet size = 1024 bytes
		0x00,

		// Super speed endpoint companion descriptor for consumer ep
		0x06,                           // Descriptor size
		CY_U3P_SS_EP_COMPN_DESCR,       // SS endpoint companion descriptor type
		0x00,                           // Max no. of packets in a burst : 1
		0x00,                           // Mult.: Max number of packets : 1
		0x00, 0x00,                     // No meaning for bulk
#endif // USE_DEBUG_PORT
    };

// Standard Language ID String Descriptor 
const uint8_t CyFxUSBStringLangIDDscr[] __attribute__ ((aligned (32))) =
    {
        0x04,                           // Descriptor Size 
        CY_U3P_USB_STRING_DESCR,        // Device Descriptor Type 
        0x09,0x04                       // Language ID supported 
    };

// Standard Manufacturer String Descriptor 
const uint8_t CyFxUSBManufactureDscr[] __attribute__ ((aligned (32))) =
    {
        0x1E,                           // Descriptor Size
        CY_U3P_USB_STRING_DESCR,        // Device Descriptor Type 
        's', 0x00,
        't', 0x00,
        'd', 0x00,
        'L', 0x00,
        'o', 0x00,
        'g', 0x00,
        'i', 0x00,
        'c', 0x00,
        'V', 0x00,
        'e', 0x00,
        'c', 0x00,
        't', 0x00,
        'o', 0x00,
        'r', 0x00,
    };

// Standard Product String Descriptor
const uint8_t CyFxUSBProductDscr[] __attribute__ ((aligned (32))) =
    {
        (13*2 + 2),                     // Descriptor Size
        CY_U3P_USB_STRING_DESCR,        // Device Descriptor Type 
        'm', 0x00,
        'o', 0x00,
        'd', 0x00,
        'u', 0x00,
        'l', 0x00,
        'a', 0x00,
        'r', 0x00,
        'C', 0x00,
        'a', 0x00,
        'm', 0x00,
        'e', 0x00,
        'r', 0x00,
        'a', 0x00,
    };

// Debug Product String Descriptor
const uint8_t CyFxUSBDebugDscr[] __attribute__ ((aligned (32))) =
    {
        (19*2 + 2),                     // Descriptor Size
        CY_U3P_USB_STRING_DESCR,        // Device Descriptor Type
        'm', 0x00,
        'o', 0x00,
        'd', 0x00,
        'u', 0x00,
        'l', 0x00,
        'a', 0x00,
        'r', 0x00,
        'C', 0x00,
        'a', 0x00,
        'm', 0x00,
        'e', 0x00,
        'r', 0x00,
        'a', 0x00,
        ' ', 0x00,
        'D', 0x00,
        'e', 0x00,
        'b', 0x00,
        'u', 0x00,
        'g', 0x00,
    };

// Serial Number Descriptor (for consistent enumeration)
const uint8_t CyFxUSBSerialDscr[] __attribute__ ((aligned (32))) =
    {
#if defined(AISC110C)
    	((8+6)*2 + 2),                 // Descriptor Size
    	CY_U3P_USB_STRING_DESCR,       // Device Descriptor Type
        'A', 0x00,
        'I', 0x00,
        'S', 0x00,
        'C', 0x00,
        '1', 0x00,
        '1', 0x00,
        '0', 0x00,
        'C', 0x00,
#elif defined(ADV7182)
        ((7+6)*2 + 2),                 // Descriptor Size
        CY_U3P_USB_STRING_DESCR,       // Device Descriptor Type
        'A', 0x00,
        'D', 0x00,
        'V', 0x00,
        '7', 0x00,
        '1', 0x00,
        '8', 0x00,
        '2', 0x00,
#elif defined(LINEARCCD)
        ((8+6)*2 + 2),                 // Descriptor Size
        CY_U3P_USB_STRING_DESCR,       // Device Descriptor Type
        'C', 0x00,
        'C', 0x00,
        'D', 0x00,
        '-', 0x00,
        'L', 0x00,
        'i', 0x00,
        'n', 0x00,
        'e', 0x00,
#elif defined(HALLARRAY)
        ((9+6)*2 + 2),                 // Descriptor Size
        CY_U3P_USB_STRING_DESCR,       // Device Descriptor Type
        'H', 0x00,
        'a', 0x00,
        'l', 0x00,
        'l', 0x00,
        'A', 0x00,
        'r', 0x00,
        'r', 0x00,
        'a', 0x00,
        'y', 0x00,
#elif defined(ORION2K)
        ((7+6)*2 + 2),                 // Descriptor Size
        CY_U3P_USB_STRING_DESCR,       // Device Descriptor Type
        'O', 0x00,
        'r', 0x00,
        'i', 0x00,
        'o', 0x00,
        'n', 0x00,
        '2', 0x00,
        'k', 0x00,
#elif defined(CAMERALINK)
        ((10+6)*2 + 2),                // Descriptor Size
        CY_U3P_USB_STRING_DESCR,       // Device Descriptor Type
        'C', 0x00,
        'a', 0x00,
        'm', 0x00,
        'e', 0x00,
        'r', 0x00,
        'a', 0x00,
        'L', 0x00,
        'i', 0x00,
        'n', 0x00,
        'k', 0x00,
#elif defined(MT9P031)
        ((7+6)*2 + 2),                // Descriptor Size
        CY_U3P_USB_STRING_DESCR,       // Device Descriptor Type
        'M', 0x00,
        'T', 0x00,
        '9', 0x00,
        'P', 0x00,
        '0', 0x00,
        '3', 0x00,
        '1', 0x00,
#elif defined(EV76C560)
        ((8+6)*2 + 2),                // Descriptor Size
        CY_U3P_USB_STRING_DESCR,       // Device Descriptor Type
        'E', 0x00,
        'V', 0x00,
        '7', 0x00,
        '6', 0x00,
        'C', 0x00,
        '5', 0x00,
        '6', 0x00,
        '0', 0x00,
#elif defined(PYTHON300)
        ((9+6)*2 + 2),                // Descriptor Size
        CY_U3P_USB_STRING_DESCR,       // Device Descriptor Type
        'P', 0x00,
        'Y', 0x00,
        'T', 0x00,
        'H', 0x00,
        'O', 0x00,
        'N', 0x00,
        '3', 0x00,
        '0', 0x00,
        '0', 0x00,
#endif
        '-', 0x00,
        '0', 0x00,
        '0', 0x00,
        '0', 0x00,
        '0', 0x00,
        '1', 0x00,
    };

/* Place this buffer as the last buffer so that no other variable / code shares
 * the same cache line. Do not add any other variables / arrays in this file.
 * This will lead to variables sharing the same cache line. */
const uint8_t CyFxUsbDscrAlignBuffer[32] __attribute__ ((aligned (32)));
