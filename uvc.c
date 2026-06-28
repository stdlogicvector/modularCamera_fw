#include <stddef.h>

#include <cyu3system.h>
#include <cyu3os.h>
#include <cyu3dma.h>
#include <cyu3error.h>
#include <cyu3usb.h>
#include <cyu3gpif.h>
#include <cyu3gpio.h>
#include <cyu3pib.h>
#include <cyu3utils.h>
#include <cyu3socket.h>

#include "uvc.h"
#include "uvc_defs.h"
#include "sensor.h"
#include "camera_ptzcontrol.h"

#if defined(AISC110C)
	#include "cyfxgpif2config_32bit_2043.h" // 9624bytes in 2 packets; This works? Should be 9632 to work?
#elif defined(ORION2K) || defined(PYTHON300)
	#include "cyfxgpif2config_32bit_4091.h"
#elif defined(ADV7182) || defined(LINEARCCD) || defined(MT9P031) || defined(EV76C560) || (defined(HALLARRAY) && defined(GRAY8))
	#include "cyfxgpif2config_8bit_16367.h"
#elif (defined(HALLARRAY) && (defined(YUV444) || defined(RGB888)))
	#include "cyfxgpif2config_24bit_5457.h"
#elif defined(CAMERALINK) || (defined(HALLARRAY) && (defined(YUY2) || defined(YUV565) || defined(RGB565)))
	#include "cyfxgpif2config_16bit_8183.h"
#endif

/*************************************************************************************************
                                         Global Variables
 *************************************************************************************************/
CyU3PThread   uvcAppThread;                      // UVC video streaming thread. 
CyU3PThread   uvcAppEP0Thread;                   // UVC control request handling thread. 
static CyU3PEvent    glFxUVCEvent;               // Event group used to signal threads. 
static CyU3PDmaMultiChannel glChHandleUVCStream; // DMA multi-channel handle. 

// Current UVC control request fields. See USB specification for definition. 
uint8_t  bmReqType, bRequest;                           // bmReqType and bRequest fields. 
uint16_t wValue, wIndex, wLength;                       // wValue, wIndex and wLength fields. 

CyBool_t        isUsbConnected = CyFalse;               // Whether USB connection is active.
CyU3PUSBSpeed_t usbSpeed = CY_U3P_NOT_CONNECTED;        // Current USB connection speed. 
CyBool_t        streamingStarted = CyFalse;             // Whether USB host has started streaming data 
CyBool_t        glIsAppActive  = CyFalse;            	// When CLEAR_FEATURE (stop streaming) request is sent this variable is reset and set when start streaming request is sent by Host. This is used during commit buffer failure event.
static CyBool_t glIsConfigured = CyFalse;               // Whether Application is in configured state or not 

CyU3PDmaChannel glDMAChannelInteruptStatus;

#ifdef UVC_STILL_IMAGE
static volatile uint8_t glStillFlag = 0;             	// Indicates whether a Still Trigger is received from the Host
static uint8_t glStillFrameStarted = 0;   				// Indicates whether the current frame is a Still Frame
uint8_t glStillImageTriggerCtrl= 0;	  					// Indicates whether Still Trigger Control is received or not 
uint8_t glButtonPressEvent = 0;			  				// Indicates whether a Vendor Command 0x88 representing Button press is received or not 
static uint8_t glStatusBuffer[16];
#endif

/* Mac OS does not send EP Clear feature when the app is closed. It just stops issuing IN tokens. So buffer commit failures
 * can be counted and if it reaches beyond a limit, streaming can be stopped. Buffer commit failure code can be cleared
 * on DMA Consumer event so that the limit is not reached under streaming conditions.  */
static uint8_t glCommitBufferFailureCount = 0;

//Variable to track whether the reason for DMA Reset is Frame Timer overflow or Commit Buffer Failure
static uint8_t glDmaResetFlag = CY_FX_UVC_DMA_RESET_EVENT_NOT_ACTIVE;

static uint8_t glUvcVcErrorCode = CY_FX_UVC_VC_ERROR_CODE_NO_ERROR;

void CyFxUvcAppStart(void);
void CyFxUvcAppStop(void);

#ifdef FRAME_TIMER_ENABLE

/* Maximum frame transfer time in milli-seconds. The value is updated for every resolution and frame rate.
 * Note: The value should always be greater than the frame blanking period */
uint16_t glFrameTimerPeriod = CY_FX_UVC_FRAME_TIMER_VAL_200MS;

// Timer used to track frame transfer time. 
static CyU3PTimer UvcTimer;

// Frame timer overflow call back function 
static void CyFxUvcAppProgressTimer(uint32_t arg)
{
    if(glDmaResetFlag == CY_FX_UVC_DMA_RESET_EVENT_NOT_ACTIVE)
    {
        glDmaResetFlag = CY_FX_UVC_DMA_RESET_FRAME_TIMER_OVERFLOW;
        CyU3PEventSet(&glFxUVCEvent, CY_FX_UVC_DMA_RESET_EVENT, CYU3P_EVENT_OR);
    }
}
#endif

#ifdef BACKFLOW_DETECT
uint8_t back_flow_detected = 0;                         // Whether buffer overflow error is detected. 
#endif

#ifdef USB_DEBUG_INTERFACE
CyU3PDmaChannel  glDebugCmdChannel;                     // Channel to receive debug commands on. 
CyU3PDmaChannel  glDebugRspChannel;                     // Channel to send debug responses on. 
uint8_t         *glDebugRspBuffer;                      // Buffer used to send debug responses. 
#endif

// UVC Probe Control Settings for a USB 3.0 connection. 
uint8_t glProbeCtrlFull[CY_FX_UVC_MAX_PROBE_SETTING] = {
    0x00, 0x00,                 				// bmHint : no hit
    0x01,                       				// Use 1st Video format index
    UVC_SS_FULL_FRAME_INDEX,    				// Use 1st Video frame index
    DBVAL(10000000 / SS_FRAMERATE),				// Desired frame interval in the unit of 100ns
    0x00, 0x00,                 				/* Key frame rate in key frame/video frame units: only applicable
                                   	   	   	   	   to video streaming with adjustable compression parameters */
    0x00, 0x00,                 				/* PFrame rate in PFrame / key frame units: only applicable to
                                   	   	   	   	   video streaming with adjustable compression parameters */
    0x00, 0x00,                 				/* Compression quality control: only applicable to video streaming
                                   	   	   	   	   with adjustable compression parameters */
    0x00, 0x00,                 				/* Window size for average bit rate: only applicable to video
                                   	   	   	   	   streaming with adjustable compression parameters */
    0x00, 0x00,                 				// Internal video streaming i/f latency in ms
    DBVAL(SS_HEIGHT*SS_WIDTH*SS_COLORDEPTH/8),	// Max video frame size in bytes
    DBVAL(CY_FX_UVC_STREAM_BUF_SIZE),			// No. of bytes device can rx in single payload = 16 KB

#ifndef FX3_UVC_1_0_SUPPORT
    // UVC 1.1 Probe Control has additional fields from UVC 1.0 
    DBVAL(384000000),		             		// Device Clock
    0x00,                       				// Framing Information - Ignored for uncompressed format
    0x00,                       				// Preferred payload format version
    0x00,                       				// Minimum payload format version
    0x00                        				// Maximum payload format version
#endif
};

uint8_t glProbeCtrlHalf[CY_FX_UVC_MAX_PROBE_SETTING] = {
    0x00, 0x00,                 				// bmHint : no hit
    0x01,                       				// Use 1st Video format index
    UVC_SS_HALF_FRAME_INDEX,    				// Use 1st Video frame index
    DBVAL(10000000 / SS_FRAMERATE),				// Desired frame interval in the unit of 100ns
    0x00, 0x00,                 				/* Key frame rate in key frame/video frame units: only applicable
                                   	   	   	   	   to video streaming with adjustable compression parameters */
    0x00, 0x00,                 				/* PFrame rate in PFrame / key frame units: only applicable to
                                   	   	   	   	   video streaming with adjustable compression parameters */
    0x00, 0x00,                 				/* Compression quality control: only applicable to video streaming
                                   	   	   	   	   with adjustable compression parameters */
    0x00, 0x00,                 				/* Window size for average bit rate: only applicable to video
                                   	   	   	   	   streaming with adjustable compression parameters */
    0x00, 0x00,                 				// Internal video streaming i/f latency in ms
    DBVAL(SS_HEIGHT/2*SS_WIDTH/2*SS_COLORDEPTH/8),	// Max video frame size in bytes
    DBVAL(CY_FX_UVC_STREAM_BUF_SIZE),			// No. of bytes device can rx in single payload = 16 KB

#ifndef FX3_UVC_1_0_SUPPORT
    // UVC 1.1 Probe Control has additional fields from UVC 1.0 
    DBVAL(384000000),		             		// Device Clock
    0x00,                       				// Framing Information - Ignored for uncompressed format
    0x00,                       				// Preferred payload format version
    0x00,                       				// Minimum payload format version
    0x00                        				// Maximum payload format version
#endif
};

// UVC Probe Control Setting for a USB 2.0 connection. 
uint8_t glProbeCtrl20[CY_FX_UVC_MAX_PROBE_SETTING] = {
    0x00, 0x00,                 				// bmHint : no hit
    0x01,                       				// Use 1st Video format index
    0x01,                       				// Use 1st Video frame index
    DBVAL(10000000 / HS_FRAMERATE),     		// Desired frame interval in the unit of 100ns: 15 fps
    0x00, 0x00,                 				/* Key frame rate in key frame/video frame units: only applicable
                                   	   	   	   	   to video streaming with adjustable compression parameters */
    0x00, 0x00,                 				/* PFrame rate in PFrame / key frame units: only applicable to
                                   	   	   	   	   video streaming with adjustable compression parameters */
    0x00, 0x00,                 				/* Compression quality control: only applicable to video streaming
                                   	   	   	   	   with adjustable compression parameters */
    0x00, 0x00,                 				/* Window size for average bit rate: only applicable to video
                                   	   	   	   	   streaming with adjustable compression parameters */
    0x00, 0x00,                 				// Internal video streaming i/f latency in ms
    DBVAL(HS_HEIGHT*HS_WIDTH*HS_COLORDEPTH/8),  // Max video frame size in bytes
    DBVAL(CY_FX_UVC_STREAM_BUF_SIZE),			// No. of bytes device can rx in single payload = 16 KB

#ifndef FX3_UVC_1_0_SUPPORT
    // UVC 1.1 Probe Control has additional fields from UVC 1.0 
    DBVAL(384000000),		             		// Device Clock
    0x00,                               		// Framing Information - Ignored for uncompressed format
    0x00,                               		// Preferred payload format version
    0x00,                               		// Minimum payload format version
    0x00                                		// Maximum payload format version
#endif
};

typedef struct __attribute__ ((packed)) UVCStillProbeCtrl_t
{
    uint8_t     bFormatIndex;
    uint8_t     bFrameIndex;
    uint8_t     bCompressionIndex;
    uint32_t    dwMaxVideoFrameSize;
    uint32_t    dwMaxPayloadTransferSize;
} UVCStillProbeCtrl_t;

typedef struct __attribute__((aligned (4))) UVCCurrentFrameDetails_t
{
    uint8_t  currentFormatIndex;
    uint8_t  currentFrameIndex;
    uint16_t currentFrameWidth;
    uint16_t currentFrameHeight;
    uint16_t currentBitsPerPixel;
}UVCCurrentFrameDetails_t;

UVCCurrentFrameDetails_t glCurrentStillFrameDetails;
UVCCurrentFrameDetails_t glCurrentVideoFrameDetails;

// Video Probe Commit Control. This array is filled out when the host sends down the SET_CUR request. 
static uint8_t glCommitCtrl[CY_FX_UVC_MAX_PROBE_SETTING_ALIGNED];

//Still Commit Control.
//static uint8_t glStillCommitCtrl[CY_FX_MAX_STILL_PROBE_SETTING_ALIGNED];

volatile uint32_t frame_cnt = 0;
volatile uint8_t packet_cnt = 0;

// Scratch buffer used for handling UVC class requests with a data phase. 
static uint8_t glEp0Buffer[32];

// UVC Header to be prefixed at the top of each 16 KB video data buffer. 
uint8_t volatile glUVCHeader[CY_FX_UVC_MAX_HEADER] =
{
    0x0C,                               // Header Length 
    0x8C,                               // Bit field header field 
    0x00, 0x00, 0x00, 0x00,             // Presentation time stamp field 
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00  // Source clock reference field 
};

#ifdef UVC_EXTENSION_UNIT

// Format is: Version 1.0 (Major.Minor) Build date: 9/10/17 (MM/DD/YY) 
static uint8_t glFxUvcFirmwareVersion[5] = {
    1,       // Major version 
    0,       // Minor version 
    9,       // Build month 
    22,      // Build day 
    17       // Build Year 
};
#endif

#ifdef DEBUG_PRINT_FRAME_COUNT
volatile static uint32_t glFrameCount = 0;              // Number of video frames transferred so far. 
volatile static uint32_t glDmaDone = 1;                 // Number of buffers transferred in the current frame. 
#endif

// Add the UVC packet header to the top of the specified DMA buffer. 
void
CyFxUVCAddHeader (
        uint8_t *buffer_p,              // Buffer pointer 
        uint8_t frameInd                // EOF or normal frame indication 
        )
{
    // Copy header to buffer 
    CyU3PMemCopy (buffer_p, (uint8_t*) glUVCHeader, CY_FX_UVC_MAX_HEADER);
    CyU3PMemCopy (buffer_p+2, (uint8_t*) &frame_cnt, sizeof(uint32_t));

    // The EOF flag needs to be set if this is the last packet for this video frame. 
    if (frameInd & CY_FX_UVC_HEADER_EOF)
    {
      	//if (packet_cnt > 0)
    		buffer_p[1] |= CY_FX_UVC_HEADER_EOF;

        glUVCHeader[1] ^= CY_FX_UVC_HEADER_FRAME_ID;

        frame_cnt++;
        packet_cnt = 0;

#ifdef UVC_STILL_IMAGE
        if (glStillFlag == 1)
        {
        	glStillFrameStarted = 1;
        	glStillFlag=0;
        }
        else if (glStillFrameStarted == 1)
        {
        	//Indicate this is the EOF frame of a Still Frame
        	buffer_p[1] |= CY_FX_UVC_STILL_IMAGE_HEADER;
        	glStillFrameStarted=0;
        }
#endif
    }
#ifdef UVC_STILL_IMAGE
    else if (glStillFrameStarted==1)
    {
       	buffer_p[1] |= CY_FX_UVC_STILL_IMAGE_HEADER;
    }
#endif
    else {
    	packet_cnt++;
    }
}

#ifdef UVC_STILL_IMAGE
static void CyFxSendStatusToHost()
{
	CyU3PDmaBuffer_t StatusBuffer;
	CyU3PReturnStatus_t apiRetStatus = CY_U3P_SUCCESS;

	glStatusBuffer[0] = 0x02;  // bStatusType = VideoStreaming interface 
	glStatusBuffer[1] = 0x01;  // bOriginator = ID of VideoStreaming interface 
	glStatusBuffer[2] = 0x00;  // bEvent      = Button press 
	glStatusBuffer[3] = 0x01;  // bValue = Button press state 

	StatusBuffer.buffer = glStatusBuffer;
	StatusBuffer.count  = 4;
	StatusBuffer.size   = sizeof(glStatusBuffer);
	StatusBuffer.status = 0;

	apiRetStatus = CyU3PDmaChannelSetupSendBuffer(&glDMAChannelInteruptStatus, &StatusBuffer);
	if (apiRetStatus != CY_U3P_SUCCESS)
		CyU3PDebugPrint(2, "SetupSendBuffer failed with 0x%x\n", apiRetStatus);

	apiRetStatus = CyU3PDmaChannelWaitForCompletion(&glDMAChannelInteruptStatus, CYU3P_WAIT_FOREVER);
	if (apiRetStatus != CY_U3P_SUCCESS)
		CyU3PDebugPrint(2, "WaitForCompletion failed with 0x%x\n",apiRetStatus);
}
#endif

// Application Error Handler 
void
CyFxUVCAppErrorHandler (
        CyU3PReturnStatus_t apiRetStatus    // API return status 
        )
{
    for (;;)
    {
    	CyU3PDebugPrint (4, "UVC Error handler...\r\n");
        CyU3PThreadSleep (1000);
    }
}

/* This function performs the operations for a Video Streaming Abort.
   This is called every time there is a USB reset, suspend or disconnect event.
 */
static void
CyFxUVCAppAbortHandler (
        void)
{
    // Set Video Stream Abort Event 
    CyU3PEventSet (&glFxUVCEvent, CY_FX_UVC_STREAM_ABORT_EVENT, CYU3P_EVENT_OR);
}

// This is the Callback function to handle the USB Events 
void
CyFxUVCAppUSBEventCB (
        CyU3PUsbEventType_t evtype,
        uint16_t            evdata
        )
{
    switch (evtype)
    {
        case CY_U3P_USB_EVENT_SUSPEND:
            CyU3PDebugPrint (4, "UsbEventCB: SUSPEND encountered...\r\n");
            // Set USB suspend Event 
            CyU3PEventSet (&glFxUVCEvent, CY_FX_USB_SUSPEND_EVENT_HANDLER, CYU3P_EVENT_OR);
            break;

        case CY_U3P_USB_EVENT_EP_UNDERRUN:
        	CyU3PDebugPrint (4, "UsbEventCB: CY_U3P_USB_EVENT_EP_UNDERRUN encountered...\r\n");
            break;

        case CY_U3P_USB_EVENT_SETCONF:
        case CY_U3P_USB_EVENT_RESET:
        case CY_U3P_USB_EVENT_DISCONNECT:
        case CY_U3P_USB_EVENT_CONNECT:
            if (evtype == CY_U3P_USB_EVENT_SETCONF)
              glIsConfigured = CyTrue;
            else
              glIsConfigured = CyFalse;

            // Stop the video streamer application 
            if (glIsAppActive)
            {
              CyU3PDebugPrint(4, "UsbEventCB: Call App Stop\r\n");
              CyFxUVCAppAbortHandler();
            }
            break;

        default:
            break;
    }
}

// Callback to handle the USB Setup Requests and UVC Class events 
CyBool_t
CyFxUVCAppUSBSetupCB (
        uint32_t setupdat0,
        uint32_t setupdat1
        )
{
    CyBool_t uvcHandleReq = CyFalse;
    uint32_t status;

    // Obtain Request Type and Request 
    bmReqType = (uint8_t)(setupdat0 & CY_U3P_USB_REQUEST_TYPE_MASK);
    bRequest  = (uint8_t)((setupdat0 & CY_U3P_USB_REQUEST_MASK) >> CY_U3P_USB_REQUEST_POS);
    wIndex    = (uint16_t)(setupdat1 & CY_U3P_USB_INDEX_MASK);
    wValue    = (uint16_t)((setupdat0 & CY_U3P_USB_VALUE_MASK)  >> CY_U3P_USB_VALUE_POS);
    wLength   = (uint16_t)((setupdat1 & CY_U3P_USB_LENGTH_MASK) >> CY_U3P_USB_LENGTH_POS);

    // Check for UVC Class Requests 
    switch (bmReqType)
    {
        case CY_FX_USB_UVC_GET_REQ_TYPE:
        case CY_FX_USB_UVC_SET_REQ_TYPE:
            // UVC Specific requests are handled in the EP0 thread. 
            switch (wIndex & 0xFF)
            {
                case CY_FX_UVC_CONTROL_INTERFACE:
                    {
                        uvcHandleReq = CyTrue;
                        status = CyU3PEventSet (&glFxUVCEvent, CY_FX_UVC_VIDEO_CONTROL_REQUEST_EVENT, CYU3P_EVENT_OR);

                        if (status != CY_U3P_SUCCESS)
                        {
                            CyU3PDebugPrint (4, "Set CY_FX_UVC_VIDEO_CONTROL_REQUEST_EVENT Failed %x\r\n", status);
                            CyU3PUsbStall (0, CyTrue, CyFalse);
                        }
                    }
                    break;

                case CY_FX_UVC_STREAM_INTERFACE:
                    {
                        uvcHandleReq = CyTrue;
                        status = CyU3PEventSet (&glFxUVCEvent, CY_FX_UVC_VIDEO_STREAM_REQUEST_EVENT, CYU3P_EVENT_OR);

                        if (status != CY_U3P_SUCCESS)
                        {
                            CyU3PDebugPrint (4, "Set CY_FX_UVC_VIDEO_STREAM_REQUEST_EVENT Failed %x\r\n", status);
                            CyU3PUsbStall (0, CyTrue, CyFalse);
                        }
                    }
                    break;

                default:
                    break;
            }
            break;

        case CY_U3P_USB_TARGET_INTF:
            if (bRequest == CY_U3P_USB_SC_SET_INTERFACE)
            {
                /* Some hosts send Set Interface Alternate Setting 0 command while stopping the video
                 * stream. The application uses this event to stop streaming. */
                if ((wIndex == CY_FX_UVC_STREAM_INTERFACE) && (wValue == 0))
                {
                    // Stop GPIF state machine to stop data transfers through FX3 
                    CyU3PDebugPrint (4, "Alternate setting 0..\r\n");

                    // Clear the stall condition and sequence numbers. 
                    CyU3PUsbStall (CY_FX_EP_BULK_VIDEO, CyFalse, CyTrue);
                    CyFxUVCAppAbortHandler ();

                    uvcHandleReq = CyTrue;
                    // Complete Control request handshake 
                    CyU3PUsbAckSetup ();
                }
            }
            else if ((bRequest == CY_U3P_USB_SC_SET_FEATURE) || (bRequest == CY_U3P_USB_SC_CLEAR_FEATURE))
            {
                CyU3PDebugPrint(4, "USBSetupCB:In SET_FTR %d::%d\r\n", glIsAppActive, glIsConfigured);
                if (glIsConfigured)
                {
                    uvcHandleReq = CyTrue;
                    CyU3PUsbAckSetup ();
                }
            }

            break;

        case CY_U3P_USB_TARGET_ENDPT:
            if (bRequest == CY_U3P_USB_SC_CLEAR_FEATURE)
            {
                if (wIndex == CY_FX_EP_BULK_VIDEO)
                {
                    /* Windows OS sends Clear Feature Request after it stops streaming,
                     * however MAC OS sends clear feature request right after it sends a
                     * Commit -> SET_CUR request. Hence, stop the video streaming and clear
                     * the stall condition and sequence numbers */
                    CyU3PDebugPrint (4, "Clear feature request detected...\r\n");

                    // Clear the stall condition and sequence numbers. 
                    CyU3PUsbStall (CY_FX_EP_BULK_VIDEO, CyFalse, CyTrue);
                    CyFxUVCAppAbortHandler();

                    uvcHandleReq = CyTrue;
                    // Complete Control request handshake 
                    CyU3PUsbAckSetup ();
                }
            }
            break;

        default:
            break;
    }

    // Return status of request handling to the USB driver 
    return uvcHandleReq;
}

/* DMA callback providing notification when data buffers are received from the sensor and when they have
 * been drained by the USB host.
 *
 * The UVC headers are attached to the data, and forwarded to the USB host in this callback function.
 */
void
CyFxUvcAppDmaCallback (
        CyU3PDmaMultiChannel *chHandle,
        CyU3PDmaCbType_t      type,
        CyU3PDmaCBInput_t    *input
        )
{
    CyU3PDmaBuffer_t    dmaBuffer;
    CyU3PReturnStatus_t status = CY_U3P_SUCCESS;

    if (type == CY_U3P_DMA_CB_PROD_EVENT)
    {
        /* This is a produce event notification to the CPU. This notification is received upon reception of
         * every buffer. The buffer will not be sent out unless it is explicitly committed. The call shall fail
         * if there is a bus reset / usb disconnect or if there is any application error.
         */

#ifdef FRAME_TIMER_ENABLE
        // Received data from the sensor so stop the frame timer 
        CyU3PTimerStop(&UvcTimer);

        // Restart the frame timer so that we receive the next buffer before timer overflows 
        CyU3PTimerModify(&UvcTimer, glFrameTimerPeriod, 0);
        CyU3PTimerStart(&UvcTimer);
#endif

        /* There is a possibility that CyU3PDmaMultiChannelGetBuffer will return CY_U3P_ERROR_INVALID_SEQUENCE here.
         * In such a case, do nothing. We make up for this missed produce event by making repeated commit actions
         * in subsequent produce event callbacks.
         */
        status = CyU3PDmaMultiChannelGetBuffer (chHandle, &dmaBuffer, CYU3P_NO_WAIT);

        while (status == CY_U3P_SUCCESS)
        {
        	// Add Headers
            if (dmaBuffer.count == CY_FX_UVC_BUF_FULL_SIZE)
            {
                // A full buffer indicates there is more data to go in this video frame. 
                CyFxUVCAddHeader (dmaBuffer.buffer - CY_FX_UVC_MAX_HEADER, CY_FX_UVC_HEADER_FRAME);
            }
            else
            {
                // A partially filled buffer indicates the end of the ongoing video frame. 
                CyFxUVCAddHeader (dmaBuffer.buffer - CY_FX_UVC_MAX_HEADER, CY_FX_UVC_HEADER_EOF);

#ifdef DEBUG_PRINT_FRAME_COUNT
                glFrameCount++;
                glDmaDone = 0;
#endif
            }

            // Commit Buffer to USB
            status = CyU3PDmaMultiChannelCommitBuffer (chHandle, (dmaBuffer.count + CY_FX_UVC_MAX_HEADER), 0);
            if (status == CY_U3P_SUCCESS)
            {
#ifdef DEBUG_PRINT_FRAME_COUNT
                glDmaDone++;
#endif
            }
            else
            {
                if(glDmaResetFlag == CY_FX_UVC_DMA_RESET_EVENT_NOT_ACTIVE)
                {
                    glDmaResetFlag = CY_FX_UVC_DMA_RESET_COMMIT_BUFFER_FAILURE;
                    CyU3PEventSet(&glFxUVCEvent, CY_FX_UVC_DMA_RESET_EVENT, CYU3P_EVENT_OR);
                }
                break;
            }

            // Check if any more buffers are ready to go, and commit them here. 
            status = CyU3PDmaMultiChannelGetBuffer (chHandle, &dmaBuffer, CYU3P_NO_WAIT);
        }
    }
    else if (type == CY_U3P_DMA_CB_CONS_EVENT)
    {
        streamingStarted = CyTrue;
        glCommitBufferFailureCount = 0;        // Reset the counter after data is consumed by USB
    }
}

void CyFxUVCGpioIntrCb (
        uint8_t gpioId // Indicates the pin that triggered the interrupt 
        )
{
    if(gpioId == BUTTON_GPIO)
    {
        CyU3PEventSet (&glFxUVCEvent, CY_FX_STILL_CAPTURE_HW_TRIGGER_EVENT, CYU3P_EVENT_OR);
    }

}

// GpifCB callback function is invoked when FV triggers GPIF interrupt 
void
CyFxGpifCB (
        uint8_t currentState            // GPIF state which triggered the interrupt. 
        )
{
    /* The ongoing video frame has ended. If we have a partial buffer sitting on the socket, we need to forcibly
     * wrap it up. We also need to toggle the FW_TRG a couple of times to get the state machine ready for the
     * next frame.
     *
     * Note: DMA channel APIs cannot be used here as this is ISR context. We are making use of the raw socket
     * APIs.
     */

    switch (currentState)
    {
        case PARTIAL_BUF_IN_SCK0:
            CyU3PDmaSocketSetWrapUp (CY_U3P_PIB_SOCKET_0);
            break;

//        case FULL_BUF_IN_SCK0:
//            break;

        case PARTIAL_BUF_IN_SCK1:
            CyU3PDmaSocketSetWrapUp (CY_U3P_PIB_SOCKET_1);
            break;

//        case FULL_BUF_IN_SCK1:
//            break;

        default:
            // This should not happen. Do nothing. 
            return;
    }

    CyU3PGpifControlSWInput (CyTrue);
    CyU3PGpifControlSWInput (CyFalse);
}

/*
 * Handler for control requests addressed to the Processing Unit.
 */
static void
UVCHandleProcessingUnitRqts (
        void)
{
    CyU3PReturnStatus_t apiRetStatus = CY_U3P_SUCCESS;
    uint16_t readCount, brightnessVal;

    switch (wValue)
    {
        case CY_FX_UVC_PU_BRIGHTNESS_CONTROL:
            switch (bRequest)
            {
                case CY_FX_USB_UVC_GET_LEN_REQ: // Length of brightness data = 2 byte. 
                    glEp0Buffer[0] = 2;
                    CyU3PUsbSendEP0Data (2, (uint8_t *)glEp0Buffer);
                    break;

                case CY_FX_USB_UVC_GET_CUR_REQ: // Current brightness value. 
                    glEp0Buffer[0] = SensorGetBrightness ();
                    CyU3PUsbSendEP0Data (2, (uint8_t *)glEp0Buffer);
                    break;

                case CY_FX_USB_UVC_GET_MIN_REQ: // Minimum brightness = 0. 
                    glEp0Buffer[0] = 0;
                    CyU3PUsbSendEP0Data (2, (uint8_t *)glEp0Buffer);
                    break;

                case CY_FX_USB_UVC_GET_MAX_REQ: // Maximum brightness = 255. 
                    glEp0Buffer[0] = 255;
                    CyU3PUsbSendEP0Data (2, (uint8_t *)glEp0Buffer);
                    break;

                case CY_FX_USB_UVC_GET_RES_REQ: // Resolution = 1. 
                    glEp0Buffer[0] = 1;
                    CyU3PUsbSendEP0Data (2, (uint8_t *)glEp0Buffer);
                    break;

                case CY_FX_USB_UVC_GET_INFO_REQ: // Both GET and SET requests are supported, auto modes not supported 
                    glEp0Buffer[0] = 3;
                    CyU3PUsbSendEP0Data (2, (uint8_t *)glEp0Buffer);
                    break;

                case CY_FX_USB_UVC_GET_DEF_REQ: // Default brightness value = 55. 
                    glEp0Buffer[0] = 55;
                    CyU3PUsbSendEP0Data (2, (uint8_t *)glEp0Buffer);
                    break;

                case CY_FX_USB_UVC_SET_CUR_REQ: // Update brightness value. 
                    apiRetStatus = CyU3PUsbGetEP0Data (CY_FX_UVC_MAX_PROBE_SETTING_ALIGNED,
                            glEp0Buffer, &readCount);
                    if (apiRetStatus == CY_U3P_SUCCESS)
                    {
                        brightnessVal = CY_U3P_MAKEWORD(glEp0Buffer[1], glEp0Buffer[0]);
                        // Update the brightness value only if the value is within the range 
                        if(brightnessVal >= 0 && brightnessVal <= 255)
                        {
                            SensorSetBrightness (glEp0Buffer[0]);
                        }
                    }
                    break;

                default:
                    glUvcVcErrorCode = CY_FX_UVC_VC_ERROR_CODE_INVALID_REQUEST;
                    CyU3PUsbStall (0, CyTrue, CyFalse);
                    break;
            }
            break;

        default:
            /*
             * Only the brightness control is supported as of now. Add additional code here to support
             * other controls.
             */
            glUvcVcErrorCode = CY_FX_UVC_VC_ERROR_CODE_INVALID_CONTROL;
            CyU3PUsbStall (0, CyTrue, CyFalse);
            break;
    }
}

/*
 * Handler for control requests addressed to the UVC Camera Terminal unit.
 */
static void
UVCHandleCameraTerminalRqts (
        void)
{
#ifdef UVC_PTZ_SUPPORT
    CyU3PReturnStatus_t apiRetStatus = CY_U3P_SUCCESS;
    uint16_t readCount;
    uint16_t zoomVal;
    int32_t  panVal, tiltVal;
    CyBool_t sendData = CyFalse;
#endif

    switch (wValue)
    {
#ifdef UVC_PTZ_SUPPORT
        case CY_FX_UVC_CT_ZOOM_ABSOLUTE_CONTROL:
            switch (bRequest)
            {
                case CY_FX_USB_UVC_GET_INFO_REQ:
                    glEp0Buffer[0] = 3;                // Support GET/SET queries. 
                    CyU3PUsbSendEP0Data (1, (uint8_t *)glEp0Buffer);
                    break;

                case CY_FX_USB_UVC_GET_CUR_REQ: // Current zoom control value. 
                    zoomVal  = CyFxUvcAppGetCurrentZoom ();
                    sendData = CyTrue;
                    break;

                case CY_FX_USB_UVC_GET_MIN_REQ: // Minimum zoom control value. 
                    zoomVal  = CyFxUvcAppGetMinimumZoom ();
                    sendData = CyTrue;
                    break;

                case CY_FX_USB_UVC_GET_MAX_REQ: // Maximum zoom control value. 
                    zoomVal  = CyFxUvcAppGetMaximumZoom ();
                    sendData = CyTrue;
                    break;

                case CY_FX_USB_UVC_GET_RES_REQ: // Resolution is one unit. 
                    zoomVal  = CyFxUvcAppGetZoomResolution ();
                    sendData = CyTrue;
                    break;

                case CY_FX_USB_UVC_GET_DEF_REQ: // Default zoom setting. 
                    zoomVal  = CyFxUvcAppGetDefaultZoom ();
                    sendData = CyTrue;
                    break;

                case CY_FX_USB_UVC_SET_CUR_REQ:
                    apiRetStatus = CyU3PUsbGetEP0Data (CY_FX_UVC_MAX_PROBE_SETTING_ALIGNED,
                            glEp0Buffer, &readCount);
                    if (apiRetStatus == CY_U3P_SUCCESS)
                    {
                        zoomVal = (glEp0Buffer[0]) | (glEp0Buffer[1] << 8);
                        CyFxUvcAppModifyZoom (zoomVal);
                    }
                    break;

                default:
                    glUvcVcErrorCode = CY_FX_UVC_VC_ERROR_CODE_INVALID_REQUEST;
                    CyU3PUsbStall (0, CyTrue, CyFalse);
                    break;
            }

            if (sendData)
            {
                // Send the 2-byte data in zoomVal back to the USB host. 
                glEp0Buffer[0] = CY_U3P_GET_LSB (zoomVal);
                glEp0Buffer[1] = CY_U3P_GET_MSB (zoomVal);
                CyU3PUsbSendEP0Data (wLength, (uint8_t *)glEp0Buffer);
            }
            break;

        case CY_FX_UVC_CT_PANTILT_ABSOLUTE_CONTROL:
            switch (bRequest)
            {
                case CY_FX_USB_UVC_GET_INFO_REQ:
                    glEp0Buffer[0] = 3;                // GET/SET requests supported for this control 
                    CyU3PUsbSendEP0Data (1, (uint8_t *)glEp0Buffer);
                    break;

                case CY_FX_USB_UVC_GET_CUR_REQ:
                    panVal   = CyFxUvcAppGetCurrentPan ();
                    tiltVal  = CyFxUvcAppGetCurrentTilt ();
                    sendData = CyTrue;
                    break;

                case CY_FX_USB_UVC_GET_MIN_REQ:
                    panVal   = CyFxUvcAppGetMinimumPan ();
                    tiltVal  = CyFxUvcAppGetMinimumTilt ();
                    sendData = CyTrue;
                    break;

                case CY_FX_USB_UVC_GET_MAX_REQ:
                    panVal   = CyFxUvcAppGetMaximumPan ();
                    tiltVal  = CyFxUvcAppGetMaximumTilt ();
                    sendData = CyTrue;
                    break;

                case CY_FX_USB_UVC_GET_RES_REQ:
                    panVal   = CyFxUvcAppGetPanResolution ();
                    tiltVal  = CyFxUvcAppGetTiltResolution ();
                    sendData = CyTrue;
                    break;

                case CY_FX_USB_UVC_GET_DEF_REQ:
                    panVal   = CyFxUvcAppGetDefaultPan ();
                    tiltVal  = CyFxUvcAppGetDefaultTilt ();
                    sendData = CyTrue;
                    break;

                case CY_FX_USB_UVC_SET_CUR_REQ:
                    apiRetStatus = CyU3PUsbGetEP0Data (CY_FX_UVC_MAX_PROBE_SETTING_ALIGNED,
                            glEp0Buffer, &readCount);
                    if (apiRetStatus == CY_U3P_SUCCESS)
                    {
                        panVal = (glEp0Buffer[0]) | (glEp0Buffer[1]<<8) |
                            (glEp0Buffer[2]<<16) | (glEp0Buffer[2]<<24);
                        tiltVal = (glEp0Buffer[4]) | (glEp0Buffer[5]<<8) |
                            (glEp0Buffer[6]<<16) | (glEp0Buffer[7]<<24);

                        CyFxUvcAppModifyPan (panVal);
                        CyFxUvcAppModifyTilt (tiltVal);
                    }
                    break;

                default:
                    CyU3PUsbStall (0, CyTrue, CyFalse);
                    glUvcVcErrorCode = CY_FX_UVC_VC_ERROR_CODE_INVALID_REQUEST;
                    break;
            }

            if (sendData)
            {
                // Send the 8-byte PAN and TILT values back to the USB host. 
                glEp0Buffer[0] = CY_U3P_DWORD_GET_BYTE0 (panVal);
                glEp0Buffer[1] = CY_U3P_DWORD_GET_BYTE1 (panVal);
                glEp0Buffer[2] = CY_U3P_DWORD_GET_BYTE2 (panVal);
                glEp0Buffer[3] = CY_U3P_DWORD_GET_BYTE3 (panVal);
                glEp0Buffer[4] = CY_U3P_DWORD_GET_BYTE0 (tiltVal);
                glEp0Buffer[5] = CY_U3P_DWORD_GET_BYTE1 (tiltVal);
                glEp0Buffer[6] = CY_U3P_DWORD_GET_BYTE2 (tiltVal);
                glEp0Buffer[7] = CY_U3P_DWORD_GET_BYTE3 (tiltVal);
                CyU3PUsbSendEP0Data (wLength, (uint8_t *)glEp0Buffer);
            }
            break;
#endif

        default:
            glUvcVcErrorCode = CY_FX_UVC_VC_ERROR_CODE_INVALID_CONTROL;
            CyU3PUsbStall (0, CyTrue, CyFalse);
            break;
    }
}

/*
 * Handler for UVC Interface control requests.
 */
static void
UVCHandleInterfaceCtrlRqts (
        void)
{
    switch (wValue)
    {
        /* Control to send video control errors to the Host. When device stalls a video
         * control request, Windows host gets the error through this control */
        case CY_FX_UVC_VC_REQUEST_ERROR_CODE_CONTROL:
            switch (bRequest)
            {
                case CY_FX_USB_UVC_GET_CUR_REQ:
                    CyU3PUsbSendEP0Data(1, &glUvcVcErrorCode);
                    glUvcVcErrorCode = CY_FX_UVC_VC_ERROR_CODE_NO_ERROR;
                    break;
            }
            break;
    }
}

/*
 * Handler for control requests addressed to the Extension Unit.
 */
static void
UVCHandleExtensionUnitRqts (
        void)
{
#ifdef UVC_EXTENSION_UNIT
    CyU3PReturnStatus_t apiRetStatus = CY_U3P_SUCCESS;
    uint16_t readCount;
    CyBool_t sendData = CyFalse;
#endif

    switch (wValue)
    {
#ifdef UVC_EXTENSION_UNIT
        case CY_FX_UVC_XU_GET_FIRMWARE_VERSION_CONTROL:
            switch (bRequest)
            {
                case CY_FX_USB_UVC_GET_INFO_REQ:
                    glEp0Buffer[0] = 3;                // Support GET/SET queries. 
                    CyU3PUsbSendEP0Data (1, (uint8_t *)glEp0Buffer);
                    break;

                case CY_FX_USB_UVC_GET_CUR_REQ: // Current firmware version control value. 
                    CyU3PMemCopy(glEp0Buffer, glFxUvcFirmwareVersion, 5);
                    sendData = CyTrue;
                    break;

                case CY_FX_USB_UVC_GET_MIN_REQ: // Minimum firmware version control value. 
                    // Min value is version 0.1 Build date: 1/1/17 
                    glEp0Buffer[0] = 0;
                    CyU3PMemSet(&glEp0Buffer[1], 1, 3);
                    glEp0Buffer[4] = 17;
                    sendData = CyTrue;
                    break;

                case CY_FX_USB_UVC_GET_MAX_REQ: // Maximum firmware version control value. 
                    // Max value is version 255.255 Build date: 12/31/99 
                    CyU3PMemSet(glEp0Buffer, 0xFF, 2);
                    glEp0Buffer[2] = 12;
                    glEp0Buffer[3] = 31;
                    glEp0Buffer[4] = 99;
                    sendData = CyTrue;
                    break;

                case CY_FX_USB_UVC_GET_RES_REQ: // Resolution is one unit for all the fields 
                    CyU3PMemSet(glEp0Buffer, 1, 5);
                    sendData = CyTrue;
                    break;

                case CY_FX_USB_UVC_GET_LEN_REQ: // Length of the control 
                    // As per UVC spec, we send 2 bytes of data. Firmware version control length is 5 bytes 
                    glEp0Buffer[0] = 5;
                    glEp0Buffer[1] = 0;
                    CyU3PUsbSendEP0Data (2, (uint8_t *)glEp0Buffer);
                    break;

                case CY_FX_USB_UVC_GET_DEF_REQ: // Default firmware version setting, is the current value. But it can be changed 
                    CyU3PMemCopy(glEp0Buffer, glFxUvcFirmwareVersion, 5);
                    sendData = CyTrue;
                    break;

                case CY_FX_USB_UVC_SET_CUR_REQ:
                    apiRetStatus = CyU3PUsbGetEP0Data (CY_FX_UVC_MAX_PROBE_SETTING_ALIGNED,
                            glEp0Buffer, &readCount);
                    if (apiRetStatus == CY_U3P_SUCCESS)
                    {
                        // Copy firmware version sent by Host application 
                        CyU3PMemCopy(glFxUvcFirmwareVersion, glEp0Buffer, 5);
                    }
                    break;

                default:
                    glUvcVcErrorCode = CY_FX_UVC_VC_ERROR_CODE_INVALID_REQUEST;
                    CyU3PUsbStall (0, CyTrue, CyFalse);
                    break;
            }

            if (sendData)
            {
                // Send the data to the USB host. 
                CyU3PUsbSendEP0Data (5, (uint8_t *)glEp0Buffer);
            }
            break;
#endif

        default:
            glUvcVcErrorCode = CY_FX_UVC_VC_ERROR_CODE_INVALID_CONTROL;
            CyU3PUsbStall (0, CyTrue, CyFalse);
            break;
    }
}

#ifdef UVC_STILL_IMAGE
static CyU3PReturnStatus_t CyUVCHandleStillImageGetRequests(void)
{
    CyU3PReturnStatus_t apiRetStatus = CY_U3P_SUCCESS;

    UVCStillProbeCtrl_t stillProbeCtrl =
    {
        .bFormatIndex        		= glCurrentStillFrameDetails.currentFormatIndex,
        .bFrameIndex         		= glCurrentStillFrameDetails.currentFrameIndex,
        .dwMaxVideoFrameSize 		= UVC_PROBE_CTRL_MAX_VIDEO_FRAME_SIZE,
        .dwMaxPayloadTransferSize	= UVC_PROBE_CTRL_MAX_PAYLOAD_TRANSFER_SIZE,
    };
	apiRetStatus = CyU3PUsbSendEP0Data(sizeof(stillProbeCtrl), (uint8_t *)&stillProbeCtrl);

	return apiRetStatus;
}

static CyU3PReturnStatus_t CyUVCHandleStillImageSetRequests(void)
{
	CyU3PReturnStatus_t apiRetStatus = CY_U3P_SUCCESS;
	uint16_t readCount = 0;
	UVCStillProbeCtrl_t probeCtrl;

	// Get the UVC Still probe/commit control data from EP0 
	apiRetStatus = CyU3PUsbGetEP0Data(CY_FX_MAX_STILL_PROBE_SETTING_ALIGNED,(uint8_t *)&probeCtrl,&readCount);

	if(apiRetStatus != CY_U3P_SUCCESS)
	{
		CyU3PDebugPrint(0,"\r\nFunction %s failed at line %d with status = 0x%x\r\n",__func__, __LINE__, apiRetStatus);
	} else if((readCount == 0) || (readCount > CY_FX_MAX_STILL_PROBE_SETTING_ALIGNED))
	{
		CyU3PDebugPrint(0,"\r\nFunction %s failed at line %d with readCount = 0x%x\r\n",__func__, __LINE__, readCount);
	}
	else
	{
		glCurrentStillFrameDetails.currentFrameIndex   = probeCtrl.bFrameIndex;
		glCurrentStillFrameDetails.currentFormatIndex  = probeCtrl.bFormatIndex;
	}
	return apiRetStatus;
}
#endif

/*
 * Handler for the video streaming control requests.
 */
static void
UVCHandleVideoStreamingRqts (
        void)
{
    CyU3PReturnStatus_t apiRetStatus = CY_U3P_SUCCESS;
    uint16_t readCount;
#ifdef UVC_STILL_IMAGE
    uint32_t stillTriggerTimeout = CY_FX_STILL_CAPTURE_TIMEOUT;
#endif

    switch (wValue)
    {
#ifdef UVC_STILL_IMAGE
            case CY_FX_UVC_STILL_PROBE_CONTROL:
        	case CY_FX_UVC_STILL_COMMIT_CONTROL:
        		switch(bRequest)
        		{
        			case CY_FX_USB_UVC_GET_INFO_REQ:
        				glEp0Buffer[0] = 3;           // GET and SET requests supported 
        				CyU3PUsbSendEP0Data (1, (uint8_t *)glEp0Buffer);
        				break;

        			case CY_FX_USB_UVC_GET_LEN_REQ:
        				glEp0Buffer[0] = CY_FX_MAX_STILL_PROBE_SETTING;
        				CyU3PUsbSendEP0Data (1, (uint8_t *)glEp0Buffer);
        				break;

        			case CY_FX_USB_UVC_GET_CUR_REQ:
        			case CY_FX_USB_UVC_GET_MIN_REQ:
        			case CY_FX_USB_UVC_GET_MAX_REQ:
        			case CY_FX_USB_UVC_GET_DEF_REQ:
        				CyUVCHandleStillImageGetRequests();
        				break;

        			case CY_FX_USB_UVC_SET_CUR_REQ:
        				CyUVCHandleStillImageSetRequests();
        				break;

        			default:
        				CyU3PUsbStall (0, CyTrue, CyFalse);
        				break;
        		}
        	break;

        	case CY_FX_UVC_STILL_TRIGGER_CONTROL:
        		switch(bRequest)
        		{
        			case CY_FX_USB_UVC_GET_INFO_REQ:
        				glEp0Buffer[0] = 3;                // GET/SET requests are supported. 
        			    CyU3PUsbSendEP0Data (1, (uint8_t *)glEp0Buffer);
        	            break;

        			case CY_FX_USB_UVC_GET_CUR_REQ:
        				CyU3PUsbSendEP0Data (1, (uint8_t *)&glStillImageTriggerCtrl);
        				break;

        			case CY_FX_USB_UVC_SET_CUR_REQ:
        				apiRetStatus = CyU3PUsbGetEP0Data (CY_FX_MAX_STILL_PROBE_SETTING_ALIGNED,
        						&glStillImageTriggerCtrl, &readCount);

        				if (apiRetStatus == CY_U3P_SUCCESS)
        				{
        					if(glStillImageTriggerCtrl == 1)
        					{
        						/**
        						 * Check if current stream resolution and still resolution are same.
        						 * If not, change streaming resolution and transfer one frame
        						 * This frame will be modified to be a still frame using the glStillFlag and glStillFrameStarted in CyFxUVCAddHeader()
        						 **/
        						if(glCurrentStillFrameDetails.currentFrameIndex != glCurrentVideoFrameDetails.currentFrameIndex)
        						{
        							// Flag still capture so that UVC header is modified accordingly 
        							glStillFlag = 1;

        							// Wait for current video frame to complete 
        							while(glStillFlag)
        							{
        								if(!stillTriggerTimeout)
        								{
        									CyU3PDebugPrint(0,"Last Video Frame Timed out\r\n");
        									break;
        								}
        								else
        								{
            								stillTriggerTimeout--;
            								CyU3PThreadSleep(1);
        								}
        							}
        							// Current video frame has ended. Stop streaming 
        							CyFxUvcAppStop();

        							// Change resolution to chosen still capture resolution 
        							SensorSetResolution(glCurrentStillFrameDetails.currentFrameIndex);

        							// Start streaming 
        							CyFxUvcAppStart();

        							//Wait for one frame (still frame) to be received 
        							stillTriggerTimeout = CY_FX_STILL_CAPTURE_TIMEOUT;

        							while(glStillFrameStarted)
        							{
        								if(!stillTriggerTimeout)
        								{
        									CyU3PDebugPrint(0,"Still Frame Timed out\r\n");
        									break;
        								}
        								else
        								{
            								stillTriggerTimeout--;
            								CyU3PThreadSleep(1);
        								}
        							}

        							// Still frame has ended. Stop streaming 
        							CyFxUvcAppStop();

        							// Change resolution back to video resolution 
        							SensorSetResolution(glCurrentVideoFrameDetails.currentFrameIndex);

        							// Start streaming 
        							CyFxUvcAppStart();
        						}
        						else
        						{
        							// Flag still capture so that UVC header is modified accordingly 
        							glStillFlag = 1;
        						}
        					}
        				}
        				break;

        			default:
        				CyU3PDebugPrint(2,"\r Not Supported \n");
        				break;
        		}
        	break;
#endif

        case CY_FX_UVC_PROBE_CTRL:
            switch (bRequest)
            {
                case CY_FX_USB_UVC_GET_INFO_REQ:
                    glEp0Buffer[0] = 3;                // GET/SET requests are supported. 
                    CyU3PUsbSendEP0Data (1, (uint8_t *)glEp0Buffer);
                    break;

                case CY_FX_USB_UVC_GET_LEN_REQ:
                    glEp0Buffer[0] = CY_FX_UVC_MAX_PROBE_SETTING;
                    CyU3PUsbSendEP0Data (1, (uint8_t *)glEp0Buffer);
                    break;

                case CY_FX_USB_UVC_GET_CUR_REQ:
                case CY_FX_USB_UVC_GET_MIN_REQ:
                case CY_FX_USB_UVC_GET_MAX_REQ:
                case CY_FX_USB_UVC_GET_DEF_REQ: // There is only one setting per USB speed. 
                    if (usbSpeed == CY_U3P_SUPER_SPEED)
                    {
                    	if(glCurrentVideoFrameDetails.currentFrameIndex == UVC_SS_FULL_FRAME_INDEX)
                    	{
                            CyU3PUsbSendEP0Data (CY_FX_UVC_MAX_PROBE_SETTING, (uint8_t *)glProbeCtrlFull);
                    	}
                    	else if (glCurrentVideoFrameDetails.currentFrameIndex == UVC_SS_HALF_FRAME_INDEX)
                    	{
                            CyU3PUsbSendEP0Data (CY_FX_UVC_MAX_PROBE_SETTING, (uint8_t *)glProbeCtrlHalf);
                    	}
                    }
                    else
                    {
                        CyU3PUsbSendEP0Data (CY_FX_UVC_MAX_PROBE_SETTING, (uint8_t *)glProbeCtrl20);
                    }
                    break;

                case CY_FX_USB_UVC_SET_CUR_REQ:
                    apiRetStatus = CyU3PUsbGetEP0Data (CY_FX_UVC_MAX_PROBE_SETTING_ALIGNED,
                            glCommitCtrl, &readCount);

                    if (apiRetStatus == CY_U3P_SUCCESS)
                    {
                    	glCurrentVideoFrameDetails.currentFrameIndex = glCommitCtrl[3];

                    	// Copy the relevant settings from the host provided data into the active data structure.
                    	if(glCurrentVideoFrameDetails.currentFrameIndex == UVC_SS_FULL_FRAME_INDEX)
                    	{
                    		CyU3PMemCopy(&glProbeCtrlFull[2], &glCommitCtrl[2], UVC_PROBE_CONTROL_UPDATE_SIZE);
                    	}
                    	else if (glCurrentVideoFrameDetails.currentFrameIndex == UVC_SS_HALF_FRAME_INDEX)
                    	{
                    		CyU3PMemCopy(&glProbeCtrlHalf[2], &glCommitCtrl[2], UVC_PROBE_CONTROL_UPDATE_SIZE);
                    	}

                    }
                    break;

                default:
                    CyU3PUsbStall (0, CyTrue, CyFalse);
                    break;
            }
            break;

        case CY_FX_UVC_COMMIT_CTRL:
            switch (bRequest)
            {
                case CY_FX_USB_UVC_GET_INFO_REQ:
                    glEp0Buffer[0] = 3;                        // GET/SET requests are supported. 
                    CyU3PUsbSendEP0Data (1, (uint8_t *)glEp0Buffer);
                    break;

                case CY_FX_USB_UVC_GET_LEN_REQ:
                    glEp0Buffer[0] = CY_FX_UVC_MAX_PROBE_SETTING;
                    CyU3PUsbSendEP0Data (1, (uint8_t *)glEp0Buffer);
                    break;

                case CY_FX_USB_UVC_GET_CUR_REQ:
                    if (usbSpeed == CY_U3P_SUPER_SPEED)
                    {
                    	if(glCurrentVideoFrameDetails.currentFrameIndex == UVC_SS_FULL_FRAME_INDEX)
                    	{
                            CyU3PUsbSendEP0Data (CY_FX_UVC_MAX_PROBE_SETTING, (uint8_t *)glProbeCtrlFull);
                    	}
                    	else if (glCurrentVideoFrameDetails.currentFrameIndex == UVC_SS_HALF_FRAME_INDEX)
                    	{
                            CyU3PUsbSendEP0Data (CY_FX_UVC_MAX_PROBE_SETTING, (uint8_t *)glProbeCtrlHalf);
                    	}
                    }
                    else
                    {
                        CyU3PUsbSendEP0Data (CY_FX_UVC_MAX_PROBE_SETTING, (uint8_t *)glProbeCtrl20);
                    }
                    break;

                case CY_FX_USB_UVC_SET_CUR_REQ:
                    /* The host has selected the parameters for the video stream. Check the desired
                       resolution settings, configure the sensor and start the video stream.
                       */
                    apiRetStatus = CyU3PUsbGetEP0Data (CY_FX_UVC_MAX_PROBE_SETTING_ALIGNED,
                            glCommitCtrl, &readCount);
                    if (apiRetStatus == CY_U3P_SUCCESS)
                    {

                        if (usbSpeed == CY_U3P_SUPER_SPEED)
                        {
                        	glCurrentVideoFrameDetails.currentFrameIndex = glCommitCtrl[3];
                        	SensorSetResolution(glCurrentVideoFrameDetails.currentFrameIndex);
#ifdef FRAME_TIMER_ENABLE
                        	if(glCurrentVideoFrameDetails.currentFrameIndex == UVC_SS_FULL_FRAME_INDEX)
                        	{
                        		 /* We are using frame timer value of 200ms as the frame time is 33ms.
                        		  * Having more margin so that DMA reset doen't happen every now and then */
                        		glFrameTimerPeriod = CY_FX_UVC_FRAME_TIMER_VAL_200MS;
                        	}
                        	else if (glCurrentVideoFrameDetails.currentFrameIndex == UVC_SS_HALF_FRAME_INDEX)
                        	{
                        		/* We are using frame timer value of 400ms as the frame time is 66ms.
                        		 * Having more margin so that DMA reset doen't happen every now and then */
                        		glFrameTimerPeriod = CY_FX_UVC_FRAME_TIMER_VAL_400MS;
                        	}
#endif
                        }
                        else // USB 2.0
                        {
                            SensorScaling_HALF ();
#ifdef FRAME_TIMER_ENABLE
                            /* We are using frame timer value of 400ms as the frame time is 66ms.
                             * Having more margin so that DMA reset doen't happen every now and then */
                            glFrameTimerPeriod = CY_FX_UVC_FRAME_TIMER_VAL_400MS;
#endif
                        }

                        // We can start streaming video now. 
                        apiRetStatus = CyU3PEventSet (&glFxUVCEvent, CY_FX_UVC_STREAM_EVENT, CYU3P_EVENT_OR);
                        if (apiRetStatus != CY_U3P_SUCCESS)
                        {
                            CyU3PDebugPrint (4, "Set CY_FX_UVC_STREAM_EVENT failed %x\r\n", apiRetStatus);
                        }
                    }
                    break;

                default:
                    CyU3PUsbStall (0, CyTrue, CyFalse);
                    break;
            }
            break;

        default:
            CyU3PUsbStall (0, CyTrue, CyFalse);
            break;
    }
}

#ifdef BACKFLOW_DETECT
static void CyFxUvcAppPibCallback (
        CyU3PPibIntrType cbType,
        uint16_t cbArg)
{
    if ((cbType == CYU3P_PIB_INTR_ERROR) && ((cbArg == 0x1005) || (cbArg == 0x1006)))
    {
        if (!back_flow_detected)
        {
            CyU3PDebugPrint (4, "Backflow detected...\r\n");
            back_flow_detected = 1;
        }
    }
}
#endif

/*
 * Load the GPIF configuration on the GPIF-II engine. This operation is performed at start-up.
 */
static void
CyFxUvcAppGpifInit (
        void)
{
    CyU3PReturnStatus_t apiRetStatus;

    apiRetStatus =  CyU3PGpifLoad ((CyU3PGpifConfig_t *) &CyFxGpifConfig);
    if (apiRetStatus != CY_U3P_SUCCESS)
    {
        CyU3PDebugPrint (4, "Loading GPIF Configuration failed, Error Code = %d\r\n", apiRetStatus);
        CyFxUVCAppErrorHandler (apiRetStatus);
    }
/*
    // Start the state machine from the designated start state.
    apiRetStatus = CyU3PGpifSMStart (START, ALPHA_START);
    if (apiRetStatus != CY_U3P_SUCCESS)
    {
        CyU3PDebugPrint (4, "Starting GPIF state machine failed, Error Code = %d\r\n", apiRetStatus);
        CyFxAppErrorHandler (apiRetStatus);
    }
*/
}

/* This function initializes the USB Module, creates event group,
   sets the enumeration descriptors, configures the Endpoints and
   configures the DMA module for the UVC Application */
void
CyFxUVCAppPreInit (void)
{
    CyU3PReturnStatus_t          apiRetStatus;
    CyU3PGpioSimpleConfig_t      gpioConfig;
    CyU3PPibClock_t              pibclock;

    // Create UVC event group
    apiRetStatus = CyU3PEventCreate(&glFxUVCEvent);
    if (apiRetStatus != 0)
    {
        CyU3PDebugPrint (4, "UVC Create Event failed, Error Code = %d\r\n", apiRetStatus);
        CyFxUVCAppErrorHandler (apiRetStatus);
    }

#ifdef UVC_PTZ_SUPPORT
    CyFxUvcAppPTZInit ();
#endif

    /* CTL pins are restricted and cannot be configured using I/O matrix configuration function,
     * must use GpioOverride to configure it */

    // Sensor Reset Pin
    apiRetStatus = CyU3PDeviceGpioOverride (SENSOR_RESET_GPIO, CyTrue);
    if (apiRetStatus != 0)
    {
        CyU3PDebugPrint (4, "GPIO %d Override failed, Error Code = %d\r\n", SENSOR_RESET_GPIO, apiRetStatus);
        CyFxUVCAppErrorHandler (apiRetStatus);
    }

    gpioConfig.outValue    = CyTrue;
    gpioConfig.driveLowEn  = CyTrue;
    gpioConfig.driveHighEn = CyTrue;
    gpioConfig.inputEn     = CyFalse;
    gpioConfig.intrMode    = CY_U3P_GPIO_NO_INTR;
    apiRetStatus           = CyU3PGpioSetSimpleConfig (SENSOR_RESET_GPIO, &gpioConfig);
    if (apiRetStatus != CY_U3P_SUCCESS)
    {
        CyU3PDebugPrint (4, "GPIO %d Set Config Error, Error Code = %d\r\n", SENSOR_RESET_GPIO, apiRetStatus);
        CyFxUVCAppErrorHandler (apiRetStatus);
    }

    //Configuring BUTTON_GPIO to capture the still image.
    apiRetStatus = CyU3PDeviceGpioOverride (BUTTON_GPIO, CyTrue);
	if (apiRetStatus != 0)
	{
		CyU3PDebugPrint (4, "GPIO %d Override failed, Error Code = %d\r\n", BUTTON_GPIO, apiRetStatus);
		CyFxUVCAppErrorHandler (apiRetStatus);
	}

    gpioConfig.outValue    = CyTrue;
    gpioConfig.driveLowEn  = CyTrue;
    gpioConfig.driveHighEn = CyTrue;
    gpioConfig.inputEn     = CyTrue;
    gpioConfig.intrMode    = CY_U3P_GPIO_INTR_NEG_EDGE;
    apiRetStatus           = CyU3PGpioSetSimpleConfig (BUTTON_GPIO, &gpioConfig);
    if (apiRetStatus != CY_U3P_SUCCESS)
    {
        CyU3PDebugPrint (4, "GPIO %d Set Config Error, Error Code = %d\r\n", BUTTON_GPIO, apiRetStatus);
        CyFxUVCAppErrorHandler (apiRetStatus);
    }

    // Initialize the P-port.
    pibclock.clkDiv      = 2;
    pibclock.clkSrc      = CY_U3P_SYS_CLK;
    pibclock.isDllEnable = CyFalse;
    pibclock.isHalfDiv   = CyFalse;

    apiRetStatus = CyU3PPibInit (CyTrue, &pibclock);
    if (apiRetStatus != CY_U3P_SUCCESS)
    {
        CyU3PDebugPrint (4, "PIB Function Failed to Start, Error Code = %d\r\n", apiRetStatus);
        CyFxUVCAppErrorHandler (apiRetStatus);
    }

    CyFxUvcAppGpifInit ();

    /* Register the GPIF State Machine callback used to get frame end notifications.
     * We use the fast callback version which is triggered from ISR context.
     */
    CyU3PGpifRegisterSMIntrCallback(CyFxGpifCB);

#ifdef BACKFLOW_DETECT
    back_flow_detected = 0;
    CyU3PPibRegisterCallback (CyFxUvcAppPibCallback, CYU3P_PIB_INTR_ERROR);
#endif

    // Image sensor initialization. Reset and then initialize with appropriate configuration.
    //CyU3PThreadSleep(100);
    //SensorReset ();
    //SensorInit ();

    // Default frame is the First Frame Index 1
    glCurrentVideoFrameDetails.currentFrameIndex = UVC_SS_FULL_FRAME_INDEX;
    glCurrentStillFrameDetails.currentFrameIndex = UVC_SS_FULL_FRAME_INDEX;
}

void
CyFxUVCAppInit (void)
{
	CyU3PDmaChannelConfig_t 	 dmaConfig;
	CyU3PDmaMultiChannelConfig_t dmaMultiConfig;
	CyU3PReturnStatus_t          apiRetStatus = CY_U3P_SUCCESS;
	CyU3PEpConfig_t              endPointConfig;
	CyU3PUSBSpeed_t 			 usbspeed = CY_U3P_SUPER_SPEED;
	uint16_t					 size = CY_FX_EP_BULK_VIDEO_PKT_SIZE;
	uint16_t					 count = CY_FX_EP_BULK_VIDEO_PKTS_COUNT;

	CyU3PDebugPrint (4, "CyFxUVCAppInit\r\n");

    usbspeed = CyU3PUsbGetSpeed();
/*
	switch (usbspeed)
	 {
	 case CY_U3P_FULL_SPEED:
		 size = 64;
		 break;

	 case CY_U3P_HIGH_SPEED:
		 size = 512;
		 break;

	 case CY_U3P_SUPER_SPEED:
		 size = 1024;
		 break;

	 case CY_U3P_NOT_CONNECTED:
		 CyU3PDebugPrint (4, "USB not connected\r\n");
		 CyFxUVCAppErrorHandler (apiRetStatus);
		 break;
	 }
*/
	CyU3PDebugPrint (4, "USB Speed = %d\r\n", usbspeed);

    // Configure the video streaming endpoint.
    endPointConfig.enable   = CyTrue;
    endPointConfig.epType   = CY_U3P_USB_EP_BULK;
    endPointConfig.pcktSize = size; //CY_FX_EP_BULK_VIDEO_PKT_SIZE;
    endPointConfig.isoPkts  = 0;
    endPointConfig.burstLen = count; //(usbspeed == CY_U3P_SUPER_SPEED) ? count : 1;
    endPointConfig.streams  = 0;

    apiRetStatus = CyU3PSetEpConfig(CY_FX_EP_BULK_VIDEO, &endPointConfig);

    if (apiRetStatus != CY_U3P_SUCCESS)
    {
        CyU3PDebugPrint (4, "USB Set Endpoint %d config failed, Error Code = %d\r\n", CY_FX_EP_BULK_VIDEO, apiRetStatus); //TODO: 03.09.2025 Fails here
        CyFxUVCAppErrorHandler (apiRetStatus);
    }

    /* Configure the status interrupt endpoint.
       Note: This endpoint is not being used by the application as of now. This can be used in case
       UVC device needs to notify the host about any error conditions. A MANUAL_OUT DMA channel
       can be associated with this endpoint and used to send these data packets.
     */
    endPointConfig.enable   = CyTrue;
    endPointConfig.epType   = CY_U3P_USB_EP_INTR;
    endPointConfig.pcktSize = 64;
    endPointConfig.isoPkts  = 0;
    endPointConfig.streams  = 0;
    endPointConfig.burstLen = 1;

    apiRetStatus = CyU3PSetEpConfig (CY_FX_EP_CONTROL_STATUS, &endPointConfig);

    if (apiRetStatus != CY_U3P_SUCCESS)
    {
        CyU3PDebugPrint (4, "USB Set Endpoint %d config failed, Error Code = %d\r\n", CY_FX_EP_CONTROL_STATUS, apiRetStatus);
        CyFxUVCAppErrorHandler (apiRetStatus);
    }

    // Create a DMA Channel from CPU to Status Interrupt Endpoint
	dmaConfig.size = 16;
	dmaConfig.count = 0;
	dmaConfig.prodSckId = (CyU3PDmaSocketId_t) CY_U3P_CPU_SOCKET_PROD;
	dmaConfig.consSckId = (CyU3PDmaSocketId_t) (CY_U3P_UIB_SOCKET_CONS_0 | CY_FX_EP_CONTROL_STATUS_SOCKET);
	dmaConfig.prodAvailCount = 0;
	dmaConfig.prodHeader = 0;
	dmaConfig.prodFooter = 0;
	dmaConfig.consHeader = 0;
	dmaConfig.dmaMode = CY_U3P_DMA_MODE_BYTE;
	dmaConfig.notification = 0;
	dmaConfig.cb = 0;

	apiRetStatus = CyU3PDmaChannelCreate(&glDMAChannelInteruptStatus, CY_U3P_DMA_TYPE_MANUAL_OUT, &dmaConfig);

	if(apiRetStatus != CY_U3P_SUCCESS)
	{
		CyU3PDebugPrint(2, "Status Channel creation failed with 0x%x\n", apiRetStatus);
		CyFxUVCAppErrorHandler(apiRetStatus);
	}

    // Create a DMA Manual channel for sending the video data to the USB host.
    dmaMultiConfig.size           = CY_FX_UVC_STREAM_BUF_SIZE;
    dmaMultiConfig.count          = CY_FX_UVC_STREAM_BUF_COUNT;
    dmaMultiConfig.validSckCount  = 2;
    dmaMultiConfig.prodSckId [0]  = (CyU3PDmaSocketId_t)CY_U3P_PIB_SOCKET_0;
    dmaMultiConfig.prodSckId [1]  = (CyU3PDmaSocketId_t)CY_U3P_PIB_SOCKET_1;
    dmaMultiConfig.consSckId [0]  = (CyU3PDmaSocketId_t)(CY_U3P_UIB_SOCKET_CONS_0 | CY_FX_EP_VIDEO_CONS_SOCKET);
    dmaMultiConfig.prodAvailCount = 0;
    dmaMultiConfig.prodHeader     = 12;                 // 12 byte UVC header to be added.
    dmaMultiConfig.prodFooter     = 4;                  // 4 byte footer to compensate for the 12 byte header.
    dmaMultiConfig.consHeader     = 0;
    dmaMultiConfig.dmaMode        = CY_U3P_DMA_MODE_BYTE;
    dmaMultiConfig.notification   = CY_U3P_DMA_CB_PROD_EVENT | CY_U3P_DMA_CB_CONS_EVENT;
    dmaMultiConfig.cb             = CyFxUvcAppDmaCallback;

    apiRetStatus = CyU3PDmaMultiChannelCreate (&glChHandleUVCStream, CY_U3P_DMA_TYPE_MANUAL_MANY_TO_ONE, &dmaMultiConfig);

    if (apiRetStatus != CY_U3P_SUCCESS)
    {
        CyU3PDebugPrint (4, "DMA Channel Creation Failed, Error Code = %d\r\n", apiRetStatus);
        CyFxUVCAppErrorHandler (apiRetStatus);
    }

#ifdef FRAME_TIMER_ENABLE
    CyU3PTimerCreate(&UvcTimer, CyFxUvcAppProgressTimer, 0x00, glFrameTimerPeriod, 0, CYU3P_NO_ACTIVATE);
#endif
}

void
CyFxUvcAppStart()
{
    CyU3PReturnStatus_t   apiRetStatus;

#ifdef DEBUG_PRINT_FRAME_COUNT
    // Clear state variables.
    glDmaDone    = 1;
    glFrameCount = 0;
#endif // DEBUG_PRINT_FRAME_COUNT

    // Start with frame ID 0.
    glUVCHeader[1] &= ~CY_FX_UVC_HEADER_FRAME_ID;

    // Make sure we return to an active USB link state and stay there.
    CyU3PUsbLPMDisable ();
    if (CyU3PUsbGetSpeed () == CY_U3P_SUPER_SPEED)
    {
        CyU3PUsbSetLinkPowerState (CyU3PUsbLPM_U0);
        CyU3PBusyWait (200);
    }
    else
    {
        CyU3PUsb2Resume ();
    }

    // Place the EP in NAK mode before cleaning up the pipe.
    CyU3PUsbSetEpNak (CY_FX_EP_BULK_VIDEO, CyTrue);
    CyU3PBusyWait (125);

    // Reset and flush the endpoint pipe.
    CyU3PUsbFlushEp (CY_FX_EP_BULK_VIDEO);
    CyU3PDmaMultiChannelReset (&glChHandleUVCStream);

    // Set DMA Channel transfer size, first producer socket
    apiRetStatus = CyU3PDmaMultiChannelSetXfer (&glChHandleUVCStream, 0, 0);
    if (apiRetStatus != CY_U3P_SUCCESS)
    {
        // Error handling
        CyU3PDebugPrint (4, "DMA Channel Set Transfer Failed, Error Code = %d\r\n", apiRetStatus);
        CyFxUVCAppErrorHandler (apiRetStatus);
    }

    CyU3PUsbSetEpNak (CY_FX_EP_BULK_VIDEO, CyFalse);
    CyU3PBusyWait (125);

#ifdef FRAME_TIMER_ENABLE
    // Start the frame timer so that we receive first buffer on time
    CyU3PTimerModify(&UvcTimer, glFrameTimerPeriod, 0);
    CyU3PTimerStart(&UvcTimer);
#endif
    glDmaResetFlag = CY_FX_UVC_DMA_RESET_EVENT_NOT_ACTIVE;

    // Start the state machine from the designated start state.
    apiRetStatus = CyU3PGpifSMSwitch(CY_FX_UVC_INVALID_GPIF_STATE, START_SCK0,
            CY_FX_UVC_INVALID_GPIF_STATE, ALPHA_START_SCK0, CY_FX_UVC_GPIF_SWITCH_TIMEOUT);

    if (apiRetStatus != CY_U3P_SUCCESS)
    {
        CyU3PDebugPrint (4, "Switching GPIF state machine failed, Error Code = %d\r\n", apiRetStatus);
        CyFxUVCAppErrorHandler (apiRetStatus);
    }

    CyU3PDebugPrint (4, "UVC Application started\r\n");
}

void
CyFxUvcAppStop()
{
#ifdef DEBUG_PRINT_FRAME_COUNT
    // Clear state variables.
    glDmaDone    = 1;
    glFrameCount = 0;
#endif // DEBUG_PRINT_FRAME_COUNT

#ifdef FRAME_TIMER_ENABLE
    // Stop the frame timer during an application stop
    CyU3PTimerStop(&UvcTimer);
#endif

    // Disable the GPIF state machine.
    CyU3PGpifDisable (CyFalse);
    streamingStarted = CyFalse;
    glDmaResetFlag = CY_FX_UVC_DMA_RESET_EVENT_NOT_ACTIVE;

    // Place the EP in NAK mode before cleaning up the pipe.
    CyU3PUsbSetEpNak (CY_FX_EP_BULK_VIDEO, CyTrue);
    CyU3PBusyWait (125);

    // Reset and flush the endpoint pipe.
    CyU3PDmaMultiChannelReset (&glChHandleUVCStream);
    CyU3PUsbFlushEp (CY_FX_EP_BULK_VIDEO);
    CyU3PUsbSetEpNak (CY_FX_EP_BULK_VIDEO, CyFalse);
    CyU3PBusyWait (125);

    // Allow USB low power link transitions at this stage.
    CyU3PUsbLPMEnable ();
    CyU3PDebugPrint (4, "Application Stopped\r\n");
}

/*
 * Entry function for the UVC Application Thread
 */
void
UVCAppThread_Entry (
        uint32_t input)
{
    CyU3PUsbLinkPowerMode usb3mode;
    uint16_t              wakeReason;
    CyU3PReturnStatus_t   apiRetStatus;
    uint32_t              flag;
    uint32_t 		      eventMask;

    CyU3PDebugPrint(4, "UVCAppThread_Entry\r\n");

    // Initialize the UVC Application
    //CyFxUVCAppInit ();

    /*
       The actual data forwarding from sensor to USB host is done from the DMA and GPIF callback
       functions. The thread is only responsible for checking for streaming start/stop conditions.

       The CY_FX_UVC_STREAM_EVENT event flag will indicate that the UVC video stream should be started.

       The CY_FX_UVC_STREAM_ABORT_EVENT event indicates that we need to abort the video streaming. This
       only happens when we receive a CLEAR_FEATURE request indicating that streaming is to be stopped,
       or when we have a critical error in the data path.

       The CY_FX_UVC_DMA_RESET_EVENT indicates that we need to reset the DMA and endpoint buffers and
       disable GPIF. We restarting the GPIF state machine from first state and device will start streaming
       video after it receives the next frame. FOr camera applications, if we discard few frames and resatrt
       video stream, it shouldn't be a problem. It may be a bad user experience if we abruptly stop video stream.
       Note that we will not restart video stream after few commit buffer failures as this is required for MAC OS.

       The CY_FX_USB_SUSPEND_EVENT_HANDLER indicates that device must enter low power USB suspend mode.
       There is a provision for users to reset and/or turn OFF power to the sensor/Image signal processor (ISP).
     */

    eventMask = CY_FX_UVC_STREAM_ABORT_EVENT |
    				   CY_FX_UVC_STREAM_EVENT |
    				   CY_FX_UVC_DMA_RESET_EVENT |
    				   CY_FX_USB_SUSPEND_EVENT_HANDLER
#ifdef UVC_STILL_IMAGE
    				   | CY_FX_STILL_CAPTURE_HW_TRIGGER_EVENT
#endif
    				   ;

    for (;;)
    {
        apiRetStatus = CyU3PEventGet(&glFxUVCEvent, eventMask, CYU3P_EVENT_OR_CLEAR, &flag, LOOP_TIMEOUT);

        if (apiRetStatus == CY_U3P_SUCCESS)
        {
#ifdef UVC_STILL_IMAGE
        	if (flag & CY_FX_STILL_CAPTURE_HW_TRIGGER_EVENT)
        	{
        	    CyFxSendStatusToHost();
        	}
#endif
            // Request to start video stream.
            if ((flag & CY_FX_UVC_STREAM_EVENT) != 0)
            {
                glIsAppActive = CyTrue;
                CyFxUvcAppStart();
            }

            // Video stream abort requested.
            if ((flag & CY_FX_UVC_STREAM_ABORT_EVENT) != 0)
            {
                glIsAppActive = CyFalse;
                CyFxUvcAppStop();
            }

            if (((flag & CY_FX_UVC_DMA_RESET_EVENT) != 0) && glIsAppActive)
            {
                if(glDmaResetFlag == CY_FX_UVC_DMA_RESET_COMMIT_BUFFER_FAILURE)
                    CyU3PDebugPrint (4, "DMA Reset Event: Commit buffer failure\r\n");

#ifdef FRAME_TIMER_ENABLE
                else if(glDmaResetFlag == CY_FX_UVC_DMA_RESET_FRAME_TIMER_OVERFLOW)
                    CyU3PDebugPrint (4, "DMA Reset Event: Frame timer overflow, time period = %d\r\n", glFrameTimerPeriod);
#endif

                CyFxUvcAppStop();

                if ((glIsAppActive) && (++glCommitBufferFailureCount < CY_FX_UVC_MAX_COMMIT_BUF_FAILURE_CNT))
                {
                    CyFxUvcAppStart();
                }

                // Reset the video streaming flags for a MAC OS
                if (glCommitBufferFailureCount == CY_FX_UVC_MAX_COMMIT_BUF_FAILURE_CNT)
                {
                    glIsAppActive = CyFalse;
                    CyU3PDebugPrint (4, "Application Stopped after %d Commit buffer failures\r\n", glCommitBufferFailureCount);
                    glCommitBufferFailureCount = 0;
                }
            }

            // Handle USB suspend event by putting device into low power mode
            if ((flag & CY_FX_USB_SUSPEND_EVENT_HANDLER) != 0)
            {
                /* Include your code here... to reset and/or shutdown power to the sensor/ISP
                 * This will help to reduce the overall power consumption by the kit */

                // Place FX3 in Low Power Suspend mode
                CyU3PDebugPrint(4, "UVCAppThread_Entry Entering USB Suspend Mode\r\n");
                CyU3PThreadSleep(5);

                /* As per the USB3 specs, link layer takes 10ms (Max.) to leave U3 state. If the device sees some spurious (unwanted)
                 * USB activity it will leave suspend mode (even though USB is in U3 state). The device can wait for 10ms (U3 exit LFPS duration)
                 * and check the Link Layer State. If the link layer is not in U3 state, CX3 device will wake up else it will enter suspend mode */
                do
                {
                    apiRetStatus = CyU3PSysEnterSuspendMode(CY_U3P_SYS_USB_BUS_ACTVTY_WAKEUP_SRC, 0, &wakeReason);
                    if ((apiRetStatus != CY_U3P_SUCCESS) || (CyU3PUsbGetSpeed() != CY_U3P_SUPER_SPEED))
                        break;

                    // Wait for the maximum U3 exit LFPS duration.
                    CyU3PThreadSleep(10);

                    // If the link is still in U3, we can continue to attempt Suspend mode entry.
                    apiRetStatus = CyU3PUsbGetLinkPowerState(&usb3mode);
                    if ((apiRetStatus != CY_U3P_SUCCESS) || (usb3mode != CyU3PUsbLPM_U3))
                        break;
                }while(1);

                // Leaving Low Power Suspend mode
                CyU3PDebugPrint(4, "UVCAppThread_Entry Leaving Suspend Mode\r\n");

                // Include your code here... to bring sensor/ISP out of reset and/or switch ON the power to the sensor/ISP
                //TODO: Sensor Reset?
            }
        }

#ifdef DEBUG_PRINT_FRAME_COUNT
        CyU3PDebugPrint (4, "UVC: Completed %d frames and %d buffers\r\n", glFrameCount,
                (glDmaDone != 0) ? (glDmaDone - 1) : 0);
#endif

        //CyU3PThreadRelinquish();
    }
}

/*
 * Entry function for the UVC control request processing thread.
 */
void
UVCAppEP0Thread_Entry (
        uint32_t input)
{
    uint32_t eventMask = (CY_FX_UVC_VIDEO_CONTROL_REQUEST_EVENT | CY_FX_UVC_VIDEO_STREAM_REQUEST_EVENT);
    uint32_t eventFlag;

    CyU3PDebugPrint(4, "UVCAppEP0Thread_Entry\r\n");

    for (;;)
    {
        // Wait for a Video control or streaming related request on the control endpoint. 
        if (CyU3PEventGet(&glFxUVCEvent, eventMask, CYU3P_EVENT_OR_CLEAR, &eventFlag, CYU3P_WAIT_FOREVER) == CY_U3P_SUCCESS)
        {
        	if (!isUsbConnected)
			{
				usbSpeed = CyU3PUsbGetSpeed();
				if (usbSpeed != CY_U3P_NOT_CONNECTED)
				{
					isUsbConnected = CyTrue;
				}
			}

            if (eventFlag & CY_FX_UVC_VIDEO_CONTROL_REQUEST_EVENT)
            {
                switch ((wIndex >> 8))
                {
                    case CY_FX_UVC_PROCESSING_UNIT_ID:
                        UVCHandleProcessingUnitRqts ();
                        break;

                    case CY_FX_UVC_CAMERA_TERMINAL_ID:
                        UVCHandleCameraTerminalRqts ();
                        break;

                    case CY_FX_UVC_INTERFACE_CTRL:
                        UVCHandleInterfaceCtrlRqts ();
                        break;

                    case CY_FX_UVC_EXTENSION_UNIT_ID:
                        UVCHandleExtensionUnitRqts ();
                        break;

                    default:
                        // Unsupported request. Fail by stalling the control endpoint. 
                        CyU3PUsbStall (0, CyTrue, CyFalse);
                        break;
                }
            }

            if (eventFlag & CY_FX_UVC_VIDEO_STREAM_REQUEST_EVENT)
            {
                if (wIndex != CY_FX_UVC_STREAM_INTERFACE)
                {
                    CyU3PUsbStall (0, CyTrue, CyFalse);
                }
                else
                {
                    UVCHandleVideoStreamingRqts ();
                }
            }

        }

        // Allow other ready threads to run. 
        CyU3PThreadRelinquish();
    }
}
