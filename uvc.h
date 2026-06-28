#ifndef _UVC_H_
#define _UVC_H_

#include <cyu3os.h>
#include <cyu3usb.h>
#include <cyu3types.h>
#include <cyu3usbconst.h>
#include <cyu3externcstart.h>

#include "uvc_cfg.h"

extern CyU3PThread   uvcAppThread;                      // UVC video streaming thread. 
extern CyU3PThread   uvcAppEP0Thread;

CyBool_t CyFxUVCAppUSBSetupCB (uint32_t setupdat0, uint32_t setupdat1);
void CyFxUVCAppUSBEventCB (CyU3PUsbEventType_t evtype, uint16_t evdata);
void CyFxUVCGpioIntrCb(uint8_t gpioId);
void UVCAppEP0Thread_Entry (uint32_t input);
void UVCAppThread_Entry (uint32_t input);
void CyFxUVCAppPreInit (void);
void CyFxUVCAppInit (void);

#define BUTTON_GPIO (26)

// Definitions to enable/disable special features in this UVC application. 
// #define UVC_PTZ_SUPPORT				// Enable if Pan, Tilt and Zoom controls are to be implemented.
// #define BACKFLOW_DETECT				// Enable if buffer overflow conditions are to be detected.
// #define DEBUG_PRINT_FRAME_COUNT		// Enable UART debug prints to print the frame count every end of frame
//#define FX3_UVC_1_0_SUPPORT			// Enable to run as UVC 1.0 device. Default is UVC 1.1 device
// #define UVC_EXTENSION_UNIT			// Enable to add a sample UVC extension unit that communicates with the host application associated with this firmware
//#define FRAME_TIMER_ENABLE             // Enable/Disable a timer that aborts an ongoing frame and restarts streaming when the transfer is stalled. Default setting is to enable frame timer

// UVC application thread parameters. 
#define UVC_APP_THREAD_STACK           (0x1000) // Stack size for the video streaming thread is 4 KB. 
#define UVC_APP_THREAD_PRIORITY        (8)      // Priority for the video streaming thread is 8. 

#define UVC_APP_EP0_THREAD_STACK       (0x0800) // Stack size for the UVC control request thread is 2 KB. 
#define UVC_APP_EP0_THREAD_PRIORITY    (8)      // Priority for the UVC control request thread is 8. 

// DMA socket selection for UVC data transfer. 
#define CY_FX_EP_VIDEO_CONS_SOCKET      4    	// USB Consumer socket 4 is used for video data.
#define CY_FX_EP_CONTROL_STATUS_SOCKET  3    	// USB Consumer socket 3 is used for the status pipe.

// Endpoint definition for UVC application 
#define CY_FX_EP_IN_TYPE                0x80
#define CY_FX_EP_BULK_VIDEO             (CY_FX_EP_IN_TYPE | CY_FX_EP_VIDEO_CONS_SOCKET)         // VIDEO EP IN
#define CY_FX_EP_CONTROL_STATUS         (CY_FX_EP_IN_TYPE | CY_FX_EP_CONTROL_STATUS_SOCKET)     // CTRL EP IN

// Invalid state for the GPIF state machine 
#define CY_FX_UVC_INVALID_GPIF_STATE    (257)

// Timeout period for the GPIF state machine switch 
#define CY_FX_UVC_GPIF_SWITCH_TIMEOUT   (2)

// UVC Video Streaming Endpoint Packet Size 
#define CY_FX_EP_BULK_VIDEO_PKT_SIZE    (1024)			// 1024 Bytes

// UVC Video Streaming Endpoint Packet Count 
#if defined(CAMERALINK) || defined(ORION2K) || defined(PYTHON300) || (defined(HALLARRAY) && (defined(YUY2) || defined(YUV565) || defined(RGB565)))
	#define CY_FX_EP_BULK_VIDEO_PKTS_COUNT  (16)	// Packets per DMA buffer. (Adjust GPIF Counter accordingly).
	#define CY_FX_UVC_STREAM_BUF_COUNT      (4)		// Number of DMA buffers per GPIF DMA thread.
#elif defined(ADV7182) || defined(LINEARCCD) || defined(MT9P031) || defined(EV76C560) || (defined(HALLARRAY) && defined(GRAY8))
	#define CY_FX_EP_BULK_VIDEO_PKTS_COUNT  (16)	// Packets per DMA buffer. (Adjust GPIF Counter accordingly).
	#define CY_FX_UVC_STREAM_BUF_COUNT      (4)		// Number of DMA buffers per GPIF DMA thread.
#elif (defined(HALLARRAY) && (defined(YUV444) || defined(RGB888)))
	#define CY_FX_EP_BULK_VIDEO_PKTS_COUNT  (16)	// Packets per DMA buffer. (Adjust GPIF Counter accordingly).
	#define CY_FX_UVC_STREAM_BUF_COUNT      (4)		// Number of DMA buffers per GPIF DMA thread.
#elif defined(AISC110C)
	#define CY_FX_EP_BULK_VIDEO_PKTS_COUNT  (8)		// Packets per DMA buffer. (Adjust GPIF Counter accordingly).
	#define CY_FX_UVC_STREAM_BUF_COUNT      (8)		// Number of DMA buffers per GPIF DMA thread.
#endif

// DMA buffer size used for video streaming. 
#define CY_FX_UVC_STREAM_BUF_SIZE       (CY_FX_EP_BULK_VIDEO_PKTS_COUNT * CY_FX_EP_BULK_VIDEO_PKT_SIZE)  // 16 KB 

// Maximum video data that can be accommodated in one DMA buffer. 
#define CY_FX_UVC_BUF_FULL_SIZE         (CY_FX_UVC_STREAM_BUF_SIZE - 16)

// Maximum commit buffer failures to detect a stop streaming event in a MAC OS 
#define CY_FX_UVC_MAX_COMMIT_BUF_FAILURE_CNT    (30)

#define UVC_SS_FULL_FRAME_INDEX		(1)
#define UVC_SS_HALF_FRAME_INDEX		(2)

#define UVC_PROBE_CTRL_MAX_PAYLOAD_TRANSFER_SIZE (CY_FX_UVC_STREAM_BUF_SIZE)					// DMA Buffer Size
#define UVC_PROBE_CTRL_MAX_VIDEO_FRAME_SIZE		 (SENSOR_WIDTH * SENSOR_HEIGHT * SENSOR_DEPTH)	// Max streaming resolution

#define UVC_PROBE_CONTROL_UPDATE_SIZE (6) // Format Index, Frame Index and Frame interval 
#define CY_FX_STILL_CAPTURE_TIMEOUT (1000)

#include <cyu3externcend.h>

#endif // _UVC_H_

