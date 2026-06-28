#include <cyu3system.h>
#include <cyu3os.h>
#include <cyu3gpio.h>
#include <cyu3dma.h>
#include <cyu3error.h>
#include <cyu3usb.h>

#include "dbg.h"

#ifdef USE_DEBUG_PORT

CyU3PThread dbgAppThread;

static uint8_t config_data[7];
static CyBool_t glIsAppActive = CyFalse;

// CDC Class specific requests to be handled by this application.
#define SET_LINE_CODING        0x20
#define GET_LINE_CODING        0x21
#define SET_CONTROL_LINE_STATE 0x22

void CyFxDBGAppStart(void);
void CyFxDBGAppStop(void);

void
CyFxDBGAppErrorHandler (
        CyU3PReturnStatus_t apiRetStatus
        )
{
    for (;;)
    {
    	CyU3PDebugPrint (4, "DBG Error handler...\r\n");
        CyU3PThreadSleep (5000);
    }
}



// Callback to handle the USB setup requests. 
CyBool_t
CyFxDBGAppUSBSetupCB (
        uint32_t setupdat0,
        uint32_t setupdat1
    )
{
    /* Fast enumeration is used. Only requests addressed to the interface, class,
     * vendor and unknown control requests are received by this function.
     * This application does not support any class or vendor requests. */

	uint16_t readCount = 0;
    CyBool_t isHandled = CyFalse;
    CyU3PReturnStatus_t apiRetStatus = CY_U3P_SUCCESS;

    // Decode the fields from the setup request. 
    uint8_t bmReqType  = (uint8_t) (setupdat0 & CY_U3P_USB_REQUEST_TYPE_MASK);
	uint8_t bType      = (uint8_t) (bmReqType & CY_U3P_USB_TYPE_MASK);
	uint8_t bTarget    = (uint8_t) (bmReqType & CY_U3P_USB_TARGET_MASK);

	uint8_t bRequest   = (uint8_t) ((setupdat0 & CY_U3P_USB_REQUEST_MASK) >> CY_U3P_USB_REQUEST_POS);
	uint16_t wIndex    = (uint16_t) (setupdat1 & CY_U3P_USB_INDEX_MASK);
	uint16_t wValue    = (uint16_t)((setupdat0 & CY_U3P_USB_VALUE_MASK)  >> CY_U3P_USB_VALUE_POS);
//  uint16_t wLength   = (uint16_t)((setupdat1 & CY_U3P_USB_LENGTH_MASK) >> CY_U3P_USB_LENGTH_POS);

    if (bTarget != CY_U3P_USB_TARGET_INTF)
		return CyFalse;

	if (wIndex != CY_FX_DBG_CTRL_INTF)
		return CyFalse;

    if (bType == CY_U3P_USB_STANDARD_RQT)
    {
        /* Handle SET_FEATURE(FUNCTION_SUSPEND) and CLEAR_FEATURE(FUNCTION_SUSPEND)
         * requests here. It should be allowed to pass if the device is in configured
         * state and failed otherwise. */
        if (((bRequest == CY_U3P_USB_SC_SET_FEATURE) || (bRequest == CY_U3P_USB_SC_CLEAR_FEATURE)) && (wValue == 0))
        {
            if (glIsAppActive)
                CyU3PUsbAckSetup();
            else
                CyU3PUsbStall(0, CyTrue, CyFalse);

            isHandled = CyTrue;
        }

        /* CLEAR_FEATURE request for endpoint is always passed to the setup callback
         * regardless of the enumeration model used. When a clear feature is received,
         * the previous transfer has to be flushed and cleaned up. This is done at the
         * protocol level. Since this is just a debug log operation, there is no higher
         * level protocol and there are two DMA channels associated with the function,
         * it is easier to stop and restart the application. If there are more than one
         * EP associated with the channel reset both the EPs. The endpoint stall and toggle
         * / sequence number is also expected to be reset. Return CyFalse to make the
         * library clear the stall and reset the endpoint toggle. Or invoke the
         * CyU3PUsbStall (ep, CyFalse, CyTrue) and return CyTrue. Here we are clearing
         * the stall. */
/*
         if ((bTarget == CY_U3P_USB_TARGET_ENDPT) && (bRequest == CY_U3P_USB_SC_CLEAR_FEATURE)
                && (wValue == CY_U3P_USBX_FS_EP_HALT))
        {
            if ((wIndex == CY_FX_EP_DBG_INTERRUPT) && (glIsAppActive))
            {
                CyFxDBGAppStop();
                CyFxDBGAppStart();
                CyU3PUsbStall(wIndex, CyFalse, CyTrue);

                CyU3PUsbAckSetup ();
                isHandled = CyTrue;
            }
        }
*/
    }

    // Check for CDC Class Requests
	if (bType == CY_U3P_USB_CLASS_RQT)
	{
		isHandled = CyTrue;

		// CDC Specific Requests
		if (bRequest == SET_LINE_CODING)
		{
			apiRetStatus = CyU3PUsbGetEP0Data(0x07, config_data, &readCount);
			if (apiRetStatus != CY_U3P_SUCCESS)
			{
				CyU3PDebugPrint(4, "CyU3PUsbGetEP0Data failed, Error Code = %d\r\n", apiRetStatus);
				CyFxDBGAppErrorHandler(apiRetStatus);
			}
		}
		else if (bRequest == GET_LINE_CODING)
		{
			isHandled = CyTrue;

			apiRetStatus = CyU3PUsbSendEP0Data(0x07, config_data);
			if (apiRetStatus != CY_U3P_SUCCESS)
			{
				CyU3PDebugPrint(4, "CyU3PUsbSendEP0Data failed, Error Code = %d\r\n", apiRetStatus);
				CyFxDBGAppErrorHandler(apiRetStatus);
			}
		}
		else if (bRequest == SET_CONTROL_LINE_STATE)
		{
			isHandled = CyTrue;

			if (glIsAppActive)
			{
				CyU3PUsbAckSetup();
			}
			else
				CyU3PUsbStall(0, CyTrue, CyFalse);
		}
	}

    return isHandled;
}

// This is the callback function to handle the USB events. 
void
CyFxDBGAppUSBEventCB (
    CyU3PUsbEventType_t evtype,
    uint16_t            evdata
    )
{
    switch (evtype)
    {
        case CY_U3P_USB_EVENT_SETCONF:
            if (glIsAppActive)
                CyFxDBGAppStop();

            CyFxDBGAppStart();
            break;

        case CY_U3P_USB_EVENT_RESET:
        case CY_U3P_USB_EVENT_CONNECT:
        case CY_U3P_USB_EVENT_DISCONNECT:
            if (glIsAppActive)
                CyFxDBGAppStop();
            break;

        default:
            break;
    }
}

/* Callback function to handle LPM requests from the USB 3.0 host. This function is invoked by the API
   whenever a state change from U0 -> U1 or U0 -> U2 happens. If we return CyTrue from this function, the
   FX3 device is retained in the low power state. If we return CyFalse, the FX3 device immediately tries
   to trigger an exit back to U0.

   This application does not have any state in which we should not allow U1/U2 transitions; and therefore
   the function always return CyTrue.
 */
CyBool_t
CyFxDBGAppLPMRqtCB (
        CyU3PUsbLinkPowerMode link_mode)
{
    return CyTrue;
}

void
CyFxDBGAppInit (void)
{

}

/* This function starts the logger application. This is called
 * when a SET_CONF event is received from the USB host. The endpoints
 * are configured and the DMA pipe is setup in this function. */
void
CyFxDBGAppStart (
        void)
{
    uint16_t size = 0;
    CyU3PEpConfig_t epCfg;
    CyU3PReturnStatus_t apiRetStatus = CY_U3P_SUCCESS;
    CyU3PUSBSpeed_t usbSpeed = CyU3PUsbGetSpeed();

    CyU3PGpioSetValue(21, CyTrue);

    /* First identify the usb speed. Once that is identified,
     * create a DMA channel and start the transfer on this. */

    // Based on the Bus Speed configure the endpoint packet size
    switch (usbSpeed)
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

        default:
            CyFxDBGAppErrorHandler(CY_U3P_ERROR_FAILURE);
            break;
    }

    CyU3PMemSet ((uint8_t *)&epCfg, 0, sizeof (epCfg));
    epCfg.enable = CyTrue;
    epCfg.epType = CY_U3P_USB_EP_BULK;
    epCfg.burstLen = 1;
    epCfg.streams = 0;
    epCfg.isoPkts  = 0;
    epCfg.pcktSize = size;

    // Producer endpoint configuration
    apiRetStatus = CyU3PSetEpConfig(CY_FX_EP_DBG_PRODUCER, &epCfg);
    if (apiRetStatus != CY_U3P_SUCCESS)
    {
        CyFxDBGAppErrorHandler (apiRetStatus);
    }

    // Consumer endpoint configuration
	apiRetStatus = CyU3PSetEpConfig(CY_FX_EP_DBG_CONSUMER, &epCfg);
	if (apiRetStatus != CY_U3P_SUCCESS)
	{
		CyFxDBGAppErrorHandler (apiRetStatus);
	}

    epCfg.enable = CyTrue;
    epCfg.epType = CY_U3P_USB_EP_INTR;
    epCfg.pcktSize = 64;
    epCfg.isoPkts  = 1;

    // Interrupt endpoint configuration
    apiRetStatus = CyU3PSetEpConfig(CY_FX_EP_DBG_INTERRUPT, &epCfg);
    if (apiRetStatus != CY_U3P_SUCCESS)
    {
        CyFxDBGAppErrorHandler (apiRetStatus);
    }

    // Flush the Endpoint memory
    //CyU3PUsbFlushEp(CY_FX_EP_DBG_INTERRUPT);

    apiRetStatus = CyU3PDebugInit (CY_FX_EP_DBG_SOCKET, 4);
    if (apiRetStatus != CY_U3P_SUCCESS)
    {
        CyFxDBGAppErrorHandler (apiRetStatus);
    }

    // Disable log message headers.
    CyU3PDebugPreamble (CyFalse);

    CyU3PDebugPrint(4, "CyFxDBGAppInit finished\r\n"); // No Print is possible prior to this

    // Update the status flag.
    glIsAppActive = CyTrue;

    CyU3PGpioSetValue(21, CyFalse);
}

/* This function stops the logger application. This shall be called whenever
 * a RESET or DISCONNECT event is received from the USB host. The endpoints are
 * disabled and the DMA pipe is destroyed by this function. */
void
CyFxDBGAppStop (
        void)
{
    CyU3PEpConfig_t epCfg;
    CyU3PReturnStatus_t apiRetStatus = CY_U3P_SUCCESS;

    // Update the flag.
    glIsAppActive = CyFalse;

    // Disable the debug log mechanism.
    CyU3PDebugDeInit ();

    // Flush the endpoint memory
    CyU3PUsbFlushEp(CY_FX_EP_DBG_INTERRUPT);
    CyU3PUsbFlushEp(CY_FX_EP_DBG_PRODUCER);
    CyU3PUsbFlushEp(CY_FX_EP_DBG_CONSUMER);

    // Disable endpoints.
    CyU3PMemSet ((uint8_t *)&epCfg, 0, sizeof (epCfg));
    epCfg.enable = CyFalse;

    apiRetStatus = CyU3PSetEpConfig(CY_FX_EP_DBG_PRODUCER, &epCfg);
    if (apiRetStatus != CY_U3P_SUCCESS)
    {
        CyFxDBGAppErrorHandler (apiRetStatus);
    }

    apiRetStatus = CyU3PSetEpConfig(CY_FX_EP_DBG_CONSUMER, &epCfg);
	if (apiRetStatus != CY_U3P_SUCCESS)
	{
		CyFxDBGAppErrorHandler (apiRetStatus);
	}

	apiRetStatus = CyU3PSetEpConfig(CY_FX_EP_DBG_INTERRUPT, &epCfg);
	if (apiRetStatus != CY_U3P_SUCCESS)
	{
		CyFxDBGAppErrorHandler (apiRetStatus);
	}
}

void
DBGAppThread_Entry (
        uint32_t input)
{
    for (;;)
    {
        if (glIsAppActive)
        {
        	CyU3PDebugPrint (2, ".");
        	CyU3PThreadSleep(1000);
        }
        CyU3PThreadSleep(100);

        CyU3PThreadRelinquish();
    }
}

#endif // USE_DEBUG_PORT
