#include <stddef.h>

#include <cyu3system.h>
#include <cyu3error.h>
#include <cyu3uart.h>
#include <cyu3dma.h>
#include <cyu3gpio.h>
#include <cyu3socket.h>
#include <cyu3utils.h>
#include <uart_regs.h>

#include "cdc.h"

CyU3PThread       cdcAppThread;
static CyU3PDmaChannel   glChHandleUsbtoUart;          	// DMA AUTO (USB TO UART) channel handle.
static CyU3PDmaChannel   glChHandleUarttoUsb;          	// DMA AUTO_SIG(UART TO USB) channel handle.
static volatile CyBool_t glIsAppActive = CyFalse;  		// Whether the application is active or not.
static CyU3PUartConfig_t glUartConfig = {0};           	// Current UART configuration. 
static volatile uint16_t glPktsPending = 0;            	// Number of packets that have been committed since last check. 
static volatile uint32_t regValueEn = 0, regValueDs = 0;

// CDC Class specific requests to be handled by this application. 
#define SET_LINE_CODING        0x20
#define GET_LINE_CODING        0x21
#define SET_CONTROL_LINE_STATE 0x22

void CyFxCDCAppStart(void);
void CyFxCDCAppStop(void);

void
CyFxCDCAppErrorHandler (
        CyU3PReturnStatus_t apiRetStatus    // API return status 
        )
{
    for (;;)
    {
    	CyU3PDebugPrint (4, "CDC Error handler...\r\n");
        CyU3PThreadSleep (5000);
    }
}

void
CyFxCDCDmaCallback(
        CyU3PDmaChannel   *chHandle, // Handle to the DMA channel. 
        CyU3PDmaCbType_t   type,     // Callback type.             
        CyU3PDmaCBInput_t *input)    // Callback status.           
{
    if (type == CY_U3P_DMA_CB_PROD_EVENT)
    {
        CyU3PDmaChannelCommitBuffer(&glChHandleUarttoUsb, input->buffer_p.count, 0);
        glPktsPending++;
    }
}

// This is the callback function to handle the USB events. 
void
CyFxCDCAppUSBEventCB (
        CyU3PUsbEventType_t evtype,
        uint16_t            evdata
        )
{
	CyU3PDebugPrint(4, "CyFxCDCAppUSBEventCB %d\r\n", evtype);

    switch (evtype)
    {
        case CY_U3P_USB_EVENT_SETCONF:
            if (glIsAppActive)
                CyFxCDCAppStop();

            CyFxCDCAppStart();
            break;

        case CY_U3P_USB_EVENT_RESET:
        case CY_U3P_USB_EVENT_CONNECT:
        case CY_U3P_USB_EVENT_DISCONNECT:
            if (glIsAppActive)
                CyFxCDCAppStop();
            break;

        default:
            break;
    }
}

// Callback to handle the USB Setup Requests and CDC Class events 
CyBool_t
CyFxCDCAppUSBSetupCB (
        uint32_t setupdat0, // SETUP Data 0 
        uint32_t setupdat1  // SETUP Data 1 
        )
{
    CyBool_t isHandled = CyFalse;
    CyU3PReturnStatus_t apiRetStatus = CY_U3P_SUCCESS;
    CyU3PUartConfig_t uartConfig;
    uint8_t config_data[7];
   	uint16_t readCount = 0;

    /* Fast enumeration is used. Only requests addressed to the interface, class,
     * vendor and unknown control requests are received by this function.
     * This application does not support any class or vendor requests. */

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

    if (wIndex != CY_FX_CDC_CTRL_INTF)
    	return CyFalse;

    if (bType == CY_U3P_USB_STANDARD_RQT)
    {
        /* Handle SET_FEATURE(FUNCTION_SUSPEND) and CLEAR_FEATURE(FUNCTION_SUSPEND)
         * requests here. It should be allowed to pass if the device is in configured
         * state and failed otherwise. */
        if (((bRequest == CY_U3P_USB_SC_SET_FEATURE) || (bRequest == CY_U3P_USB_SC_CLEAR_FEATURE))
        && (wValue == 0))
        {
            if (glIsAppActive)
                CyU3PUsbAckSetup();
            else
                CyU3PUsbStall(0, CyTrue, CyFalse);

            isHandled = CyTrue;
        }
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
                CyFxCDCAppErrorHandler(apiRetStatus);
            }

            if (readCount != 0x07)
            {
                CyFxCDCAppErrorHandler(CY_U3P_ERROR_BAD_SIZE);
            }
            else
            {
                CyU3PMemSet ((uint8_t *)&uartConfig, 0, sizeof (uartConfig));
                uartConfig.baudRate = (CyU3PUartBaudrate_t)(config_data[0] | (config_data[1]<<8)|
                        								   (config_data[2]<<16)|(config_data[3]<<24));
                if (config_data[4] == 0)
                {
                    uartConfig.stopBit = CY_U3P_UART_ONE_STOP_BIT;
                }
                else if (config_data[4] == 2)
                {
                    uartConfig.stopBit = CY_U3P_UART_TWO_STOP_BIT;
                }
                else
                {
                    // Give invalid value. 
                    uartConfig.stopBit = (CyU3PUartStopBit_t)0;
                }
                if (config_data[5] == 1)
                {
                    uartConfig.parity = CY_U3P_UART_ODD_PARITY;
                }
                else if (config_data[5] == 2)
                {
                    uartConfig.parity = CY_U3P_UART_EVEN_PARITY;
                }
                else
                {
                    // 0 = no parity; any other value - invalid parity. 
                    uartConfig.parity = CY_U3P_UART_NO_PARITY;
                }

                uartConfig.txEnable = CyTrue;
                uartConfig.rxEnable = CyTrue;
                uartConfig.flowCtrl = CyFalse;
                uartConfig.isDma = CyTrue;

                // Set the uart configuration 
                apiRetStatus = CyU3PUartSetConfig (&uartConfig, NULL);
                if (apiRetStatus == CY_U3P_SUCCESS)
                {
                    CyU3PMemCopy ((uint8_t *)&glUartConfig, (uint8_t *)&uartConfig, sizeof (CyU3PUartConfig_t));
                } else
                {
                	CyU3PDebugPrint(4, "CyU3PUartSetConfig failed, Error Code = %d\r\n", apiRetStatus);
                }
            }
        }
        else if (bRequest == GET_LINE_CODING )
        {
            // get current uart config 
            config_data[0] =   glUartConfig.baudRate & (0x000000FF);
            config_data[1] = ((glUartConfig.baudRate & (0x0000FF00)) >>  8);
            config_data[2] = ((glUartConfig.baudRate & (0x00FF0000)) >> 16);
            config_data[3] = ((glUartConfig.baudRate & (0xFF000000)) >> 24);

            if (glUartConfig.stopBit == CY_U3P_UART_ONE_STOP_BIT)
            {
                config_data[4] = 0;
            }
            else // CY_U3P_UART_TWO_STOP_BIT 
            {
                config_data[4] = 2;
            }

            if (glUartConfig.parity == CY_U3P_UART_EVEN_PARITY)
            {
                config_data[5] = 2;
            }
            else if (glUartConfig.parity == CY_U3P_UART_ODD_PARITY)
            {
                config_data[5] = 1;
            }
            else
            {
                config_data[5] = 0;
            }
            config_data[6] =  0x08;

            apiRetStatus = CyU3PUsbSendEP0Data(0x07, config_data);
            if (apiRetStatus != CY_U3P_SUCCESS)
            {
            	CyU3PDebugPrint(4, "CyU3PUsbSendEP0Data failed, Error Code = %d\r\n", apiRetStatus);
                CyFxCDCAppErrorHandler(apiRetStatus);
            }
        }
        else if (bRequest == SET_CONTROL_LINE_STATE)
        {
            if (glIsAppActive)
            {
                CyU3PUsbAckSetup ();
            }
            else
                CyU3PUsbStall (0, CyTrue, CyFalse);
        }
        else
        {
        	apiRetStatus = CY_U3P_ERROR_FAILURE;
        }

        if (apiRetStatus != CY_U3P_SUCCESS)
        {
            isHandled = CyFalse;
        }
    }

    return isHandled;
}

// This function initializes the USB module, UART module and sets the enumeration descriptors 
void
CyFxCDCAppPreInit (
        void )
{
    CyU3PReturnStatus_t apiRetStatus = CY_U3P_SUCCESS;
    CyU3PGpioSimpleConfig_t      gpioConfig;

    /* CTL pins are restricted and cannot be configured using I/O matrix configuration function,
	* must use GpioOverride to configure it */
	apiRetStatus = CyU3PDeviceGpioOverride(CDC_DEBUG_PIN, CyTrue);
	if (apiRetStatus != CY_U3P_SUCCESS)
	{
		CyU3PDebugPrint (4, "GPIO Override failed, Error Code = %d\r\n", apiRetStatus);
		CyFxCDCAppErrorHandler (apiRetStatus);
	}

	// Debug Pin
	gpioConfig.outValue    = CyFalse;
	gpioConfig.driveLowEn  = CyTrue;
	gpioConfig.driveHighEn = CyTrue;
	gpioConfig.inputEn     = CyFalse;
	gpioConfig.intrMode    = CY_U3P_GPIO_NO_INTR;
	apiRetStatus           = CyU3PGpioSetSimpleConfig(CDC_DEBUG_PIN, &gpioConfig);
	if (apiRetStatus != CY_U3P_SUCCESS)
	{
		CyU3PDebugPrint (4, "GPIO Set Config Error, Error Code = %d\r\n", apiRetStatus);
		CyFxCDCAppErrorHandler (apiRetStatus);
	}

    // Initialize the UART module 
    apiRetStatus = CyU3PUartInit();
    if (apiRetStatus != CY_U3P_SUCCESS)
    {
    	CyU3PDebugPrint(4, "CyU3PUartInit failed, Error Code = %d\r\n", apiRetStatus);
        CyFxCDCAppErrorHandler(apiRetStatus);
    }

    // Configure the UART 
    CyU3PMemSet ((uint8_t *)&glUartConfig, 0, sizeof (glUartConfig));
    glUartConfig.baudRate = CY_U3P_UART_BAUDRATE_921600;
    glUartConfig.stopBit  = CY_U3P_UART_ONE_STOP_BIT;
    glUartConfig.parity   = CY_U3P_UART_NO_PARITY;
    glUartConfig.txEnable = CyTrue;
    glUartConfig.rxEnable = CyTrue;
    glUartConfig.flowCtrl = CyFalse;
    glUartConfig.isDma	  = CyTrue;

    // Set the UART configuration 
    apiRetStatus = CyU3PUartSetConfig(&glUartConfig, NULL);
    if (apiRetStatus != CY_U3P_SUCCESS )
    {
    	CyU3PDebugPrint(4, "CyU3PUartSetConfig failed, Error Code = %d\r\n", apiRetStatus);
        CyFxCDCAppErrorHandler(apiRetStatus);
    }

    // UART Config Value for Enabling Rx Block
    regValueEn = UART->lpp_uart_config;

    // UART Config Value for Disabling the Rx Block
    regValueDs = UART->lpp_uart_config & (~(CY_U3P_LPP_UART_RTS | CY_U3P_LPP_UART_RX_ENABLE));

    // Set the UART transfer
    apiRetStatus = CyU3PUartTxSetBlockXfer(0xFFFFFFFF);
    if (apiRetStatus != CY_U3P_SUCCESS)
    {
    	CyFxCDCAppErrorHandler(apiRetStatus);
    }

    apiRetStatus = CyU3PUartRxSetBlockXfer(0xFFFFFFFF);
    if (apiRetStatus != CY_U3P_SUCCESS)
    {
    	CyFxCDCAppErrorHandler(apiRetStatus);
    }

/*
    // Initialize the Debug logger module.
	apiRetStatus = CyU3PDebugInit (CY_U3P_LPP_SOCKET_UART_CONS, 4);
	if (apiRetStatus != CY_U3P_SUCCESS)
	{
		CyU3PDebugPrint(4, "CyU3PDebugInit failed, Error Code = %d\r\n", apiRetStatus);
		CyFxCDCAppErrorHandler (apiRetStatus);
	}

	// Disable log message headers.
	CyU3PDebugPreamble (CyFalse);
*/
	CyU3PDebugPrint(4, "CyFxCDCAppInit finished\r\n"); // No Print is possible prior to this
}

void
CyFxCDCAppInit (
        void )
{

}

// This function starts the CDC application
void
CyFxCDCAppStart(
        void )
{
    uint16_t size = 0;
    CyU3PEpConfig_t epCfg;
    CyU3PDmaChannelConfig_t dmaCfg;
    CyU3PReturnStatus_t apiRetStatus = CY_U3P_SUCCESS;
    CyU3PUSBSpeed_t usbSpeed = CyU3PUsbGetSpeed();

    CyU3PDebugPrint(4, "CyFxCDCAppStart\r\n");

    // Based on the Bus speed configure the endpoint packet size
    switch (usbSpeed)
    {
        case CY_U3P_FULL_SPEED:
            size = 64;
            break;

        case CY_U3P_HIGH_SPEED:
            size = 512;
            break;

        case  CY_U3P_SUPER_SPEED:
            // Turning low power mode off to avoid USB transfer delays.
            CyU3PUsbLPMDisable();
            size = 1024;
            break;

        case CY_U3P_NOT_CONNECTED:
            CyFxCDCAppErrorHandler (CY_U3P_ERROR_FAILURE);
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
    apiRetStatus = CyU3PSetEpConfig(CY_FX_EP_CDC_PRODUCER, &epCfg);
    if (apiRetStatus != CY_U3P_SUCCESS)
    {
    	CyU3PDebugPrint (4, "USB Set Endpoint %d config failed, Error Code = %d\r\n", CY_FX_EP_CDC_PRODUCER, apiRetStatus);
        CyFxCDCAppErrorHandler (apiRetStatus);
    }

    //CyU3PUsbSetEpPktMode(CY_FX_EP_CDC_PRODUCER, CyTrue);

    // Consumer endpoint configuration
    apiRetStatus = CyU3PSetEpConfig(CY_FX_EP_CDC_CONSUMER, &epCfg);
    if (apiRetStatus != CY_U3P_SUCCESS)
    {
    	CyU3PDebugPrint (4, "USB Set Endpoint %d config failed, Error Code = %d\r\n", CY_FX_EP_CDC_CONSUMER, apiRetStatus);
        CyFxCDCAppErrorHandler (apiRetStatus);
    }

    // Interrupt endpoint configuration
    epCfg.epType = CY_U3P_USB_EP_INTR;
    epCfg.pcktSize = 64;
    epCfg.isoPkts = 1;

    apiRetStatus = CyU3PSetEpConfig(CY_FX_EP_CDC_INTERRUPT, &epCfg);
    if (apiRetStatus != CY_U3P_SUCCESS)
    {
    	CyU3PDebugPrint (4, "USB Set Endpoint %d config failed, Error Code = %d\r\n", CY_FX_EP_CDC_INTERRUPT, apiRetStatus);
        CyFxCDCAppErrorHandler(apiRetStatus);
    }

    // Create a DMA_AUTO channel between usb producer socket and uart consumer socket
    dmaCfg.size = size;
    dmaCfg.count = CY_FX_CDC_DMA_BUF_COUNT;
    dmaCfg.prodSckId = CY_FX_EP_TX_PROD_SOCKET;
    dmaCfg.consSckId = CY_FX_EP_TX_CONS_SOCKET;
    dmaCfg.dmaMode = CY_U3P_DMA_MODE_BYTE;
    dmaCfg.notification = 0;
    dmaCfg.cb = NULL;
    dmaCfg.prodHeader = 0;
    dmaCfg.prodFooter = 0;
    dmaCfg.consHeader = 0;
    dmaCfg.prodAvailCount = 0;

    apiRetStatus = CyU3PDmaSocketIsValidConsumer(CY_FX_EP_TX_CONS_SOCKET);
    if (apiRetStatus != CyTrue)
	{
		CyU3PDebugPrint(4, "CyU3PDmaSocketIsValidConsumer %d failed, Error Code = %d\r\n", CY_FX_EP_TX_CONS_SOCKET, apiRetStatus);
		CyFxCDCAppErrorHandler(apiRetStatus);
	}

    apiRetStatus = CyU3PDmaSocketIsValidProducer(CY_FX_EP_TX_PROD_SOCKET);
    if (apiRetStatus != CyTrue)
   	{
   		CyU3PDebugPrint(4, "CyU3PDmaSocketIsValidProducer %d failed, Error Code = %d\r\n", CY_FX_EP_TX_PROD_SOCKET, apiRetStatus);
   		CyFxCDCAppErrorHandler(apiRetStatus);
   	}

    apiRetStatus = CyU3PDmaChannelCreate(&glChHandleUsbtoUart, CY_U3P_DMA_TYPE_AUTO, &dmaCfg);
    if (apiRetStatus != CY_U3P_SUCCESS)
    {
    	CyU3PDebugPrint(4, "CyU3PDmaChannelCreate 0 failed, Error Code = %d\r\n", apiRetStatus);
        CyFxCDCAppErrorHandler(apiRetStatus);
    }

    // Create a DMA_MANUAL channel between uart producer socket and usb consumer socket
    // Use a smaller buffer size (16 bytes) to ensure that packets get filled in a short time.
    dmaCfg.size         = 16;
    dmaCfg.prodSckId    = CY_FX_EP_RX_PROD_SOCKET;
    dmaCfg.consSckId    = CY_FX_EP_RX_CONS_SOCKET;
    dmaCfg.notification = CY_U3P_DMA_CB_PROD_EVENT;
    dmaCfg.cb           = CyFxCDCDmaCallback;

    apiRetStatus = CyU3PDmaChannelCreate(&glChHandleUarttoUsb, CY_U3P_DMA_TYPE_MANUAL, &dmaCfg);
    if (apiRetStatus != CY_U3P_SUCCESS)
    {
    	CyU3PDebugPrint(4, "CyU3PDmaChannelCreate 1 failed, Error Code = %d\r\n", apiRetStatus);
        CyFxCDCAppErrorHandler(apiRetStatus);
    }

    // Set DMA Channel transfer size
    apiRetStatus = CyU3PDmaChannelSetXfer(&glChHandleUsbtoUart, 0);
    if (apiRetStatus != CY_U3P_SUCCESS)
    {
    	CyU3PDebugPrint(4, "CyU3PDmaChannelSetXfer failed, Error Code = %d\r\n", apiRetStatus);
        CyFxCDCAppErrorHandler(apiRetStatus);
    }

    apiRetStatus = CyU3PDmaChannelSetXfer(&glChHandleUarttoUsb, 0);
    if (apiRetStatus != CY_U3P_SUCCESS)
    {
    	CyU3PDebugPrint(4, "CyU3PDmaChannelSetXfer failed, Error Code = %d\r\n", apiRetStatus);
        CyFxCDCAppErrorHandler(apiRetStatus);
    }

    // Update the status flag.
    glIsAppActive = CyTrue;
}

void
CyFxCDCAppStop (
        void)
{
    CyU3PEpConfig_t epCfg;
    CyU3PReturnStatus_t apiRetStatus = CY_U3P_SUCCESS;

    CyU3PDebugPrint(4, "CyFxCDCAppStop\r\n");

    // Update the flag.
    glIsAppActive = CyFalse;

    // Flush the endpoint memory
    CyU3PUsbFlushEp(CY_FX_EP_CDC_PRODUCER);
    CyU3PUsbFlushEp(CY_FX_EP_CDC_CONSUMER);
    CyU3PUsbFlushEp(CY_FX_EP_CDC_INTERRUPT);

    // Destroy the channel
    CyU3PDmaChannelDestroy (&glChHandleUsbtoUart);
    CyU3PDmaChannelDestroy (&glChHandleUarttoUsb);

    // Disable endpoints.
    CyU3PMemSet ((uint8_t *)&epCfg, 0, sizeof (epCfg));
    epCfg.enable = CyFalse;

    apiRetStatus = CyU3PSetEpConfig(CY_FX_EP_CDC_PRODUCER, &epCfg);
    if (apiRetStatus != CY_U3P_SUCCESS)
    {
    	CyU3PDebugPrint(4, "CyU3PSetEpConfig %d failed\r\n", CY_FX_EP_CDC_PRODUCER);
        CyFxCDCAppErrorHandler (apiRetStatus);
    }

    apiRetStatus = CyU3PSetEpConfig(CY_FX_EP_CDC_CONSUMER, &epCfg);
    if (apiRetStatus != CY_U3P_SUCCESS)
    {
    	CyU3PDebugPrint(4, "CyU3PSetEpConfig %d failed\r\n", CY_FX_EP_CDC_CONSUMER);
        CyFxCDCAppErrorHandler (apiRetStatus);
    }

    apiRetStatus = CyU3PSetEpConfig(CY_FX_EP_CDC_INTERRUPT, &epCfg);
    if (apiRetStatus != CY_U3P_SUCCESS)
    {
    	CyU3PDebugPrint(4, "CyU3PSetEpConfig %d failed\r\n", CY_FX_EP_CDC_INTERRUPT);
        CyFxCDCAppErrorHandler (apiRetStatus);
    }
}

// Entry function for the CDCAppThread 
void
CDCAppThread_Entry (
        uint32_t input)
{
    CyU3PDebugPrint(4, "CDCAppThread_Entry\r\n");

    for (;;)
    {
        if (glIsAppActive)
        {
            /* While the application is active, check for data sent during the last 50 ms. If no data
               has been sent to the host, use the channel wrap-up feature to send any partial buffer to
               the USB host.
            */
            if (glPktsPending == 0)
            {
                // Disable UART Receiver Block 
                UART->lpp_uart_config = regValueDs;

                CyU3PDmaChannelSetWrapUp(&glChHandleUarttoUsb);

                // Enable UART Receiver Block 
                UART->lpp_uart_config = regValueEn;
            }

            glPktsPending = 0;
        }

        CyU3PThreadSleep (50);
    }
}
