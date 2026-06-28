#include "cyu3types.h"
#include <cyu3gpio.h>
#include <cyu3system.h>
#include <cyu3utils.h>
#include <cyu3error.h>

#include "uvc.h"
#include "cdc.h"
#include "dbg.h"

#define LED0_GPIO			60
#define LED1_GPIO			21
#define LED2_GPIO			22

#define USB3_MUX_nOE_GPIO	50
#define USB3_MUX_SEL_GPIO	52

extern const uint8_t CyFxUSBDeviceDscr[];               // USB 2.0 Device descriptor. 
extern const uint8_t CyFxUSBDeviceDscrSS[];             // USB 3.0 device descriptor. 

extern const uint8_t CyFxUSBDeviceQualDscr[];           // USB 2.0 Device Qual descriptor. 
extern const uint8_t CyFxUSBBOSDscr[];                  // USB 3.0 BOS descriptor. 

extern const uint8_t CyFxUSBFSConfigDscr[];             // Full Speed Config descriptor. 
extern const uint8_t CyFxUSBHSConfigDscr[];             // High Speed Config descriptor. 
extern const uint8_t CyFxUSBSSConfigDscr[];             // USB 3.0 config descriptor. 

extern const uint8_t CyFxUSBStringLangIDDscr[];         // String 0 descriptor. 
extern const uint8_t CyFxUSBManufactureDscr[];          // Manufacturer string descriptor. 
extern const uint8_t CyFxUSBProductDscr[];              // Product string descriptor.
extern const uint8_t CyFxUSBDebugDscr[];              	// Debug string descriptor.
extern const uint8_t CyFxUSBSerialDscr[];				// Serial string descriptor.

CyBool_t USB3_event = CyFalse, USB3_connection = CyTrue;

static void
CyFxAppUSBEventCB (
        CyU3PUsbEventType_t evtype,
        uint16_t            evdata
        )
{
	CyFxCDCAppUSBEventCB(evtype, evdata);
#ifdef USE_DEBUG_PORT
	CyFxDBGAppUSBEventCB(evtype, evdata);
#endif
	CyFxUVCAppUSBEventCB(evtype, evdata);

	switch (evtype)
	{
/*	case  CY_U3P_USB_EVENT_USB3_LNKFAIL:
	case  CY_U3P_USB_EVENT_SS_COMP_ENTRY:
		USB3_event = CyTrue;
		USB3_connection = CyFalse;
		break;
*/
	case CY_U3P_USB_EVENT_EP_UNDERRUN:
		CyU3PDebugPrint (4, "UsbEventCB: CY_U3P_USB_EVENT_EP_UNDERRUN encountered...\r\n");
		break;

	case CY_U3P_USB_EVENT_SETCONF:
		if (CyU3PUsbGetSpeed() == CY_U3P_SUPER_SPEED)
		{
			CyU3PDebugPrint(4, "UsbEventCB: Detected SS USB Connection\r\n");
		}
		else if (CyU3PUsbGetSpeed() == CY_U3P_HIGH_SPEED)
		{
			CyU3PDebugPrint(4, "UsbEventCB: Detected HS USB Connection\r\n");
		}
		// no break 
	case CY_U3P_USB_EVENT_RESET:
	case CY_U3P_USB_EVENT_DISCONNECT:
	case CY_U3P_USB_EVENT_CONNECT:
		CyU3PUsbLPMEnable ();
		break;

	default:
		break;
	}
}

static CyBool_t
CyFxAppUSBSetupCB (
        uint32_t setupdat0,
        uint32_t setupdat1
        )
{
	CyBool_t isHandled = CyFalse;

	isHandled |= CyFxCDCAppUSBSetupCB(setupdat0, setupdat1);
#ifdef USE_DEBUG_PORT
	isHandled |= CyFxDBGAppUSBSetupCB(setupdat0, setupdat1);
#endif
	isHandled |= CyFxUVCAppUSBSetupCB(setupdat0, setupdat1);

	return isHandled;
}

/* Callback for LPM requests. Always return true to allow host to transition device
 * into required LPM state U1/U2/U3. When data transmission is active LPM management
 * is explicitly disabled to prevent data transmission errors.
 */
static CyBool_t
CyFxAppLPMRqtCB (
		CyU3PUsbLinkPowerMode link_mode		//USB 3.0 linkmode requested by Host
		)
{
//	CyFxCDCAppLPMRqtCB(link_mode);
//	CyFxUVCAppLPMRqtCB(link_mode);

#ifdef USE_DEBUG_PORT
//	CyFxDBGAppLPMRqtCB(link_mode);
#endif

	return CyTrue;
}

void
CyFxAppErrorHandler (
        CyU3PReturnStatus_t apiRetStatus
        )
{
    for (;;)
    {
        CyU3PDebugPrint (4, "Main Error handler...\r\n");
        CyU3PThreadSleep (5000);
    }
}

void CyFxGpioIntrCb (
        uint8_t gpioId // Indicates the pin that triggered the interrupt
        )
{
	CyFxUVCGpioIntrCb(gpioId);
}

void
CyFxAppInit (
        void )

{
	CyU3PReturnStatus_t          apiRetStatus;
	CyU3PGpioSimpleConfig_t      gpioConfig;
	CyU3PGpioClock_t             gpioClock;

    // Init the GPIO module
    gpioClock.fastClkDiv = 2;
    gpioClock.slowClkDiv = 2;
    gpioClock.simpleDiv  = CY_U3P_GPIO_SIMPLE_DIV_BY_2;
    gpioClock.clkSrc     = CY_U3P_SYS_CLK;
    gpioClock.halfDiv    = 0;

    // Initialize Gpio interface
    apiRetStatus = CyU3PGpioInit(&gpioClock, CyFxGpioIntrCb);
    if (apiRetStatus != 0)
    {
        CyU3PDebugPrint (4, "GPIO Init failed, Error Code = %d\r\n", apiRetStatus);
        CyFxAppErrorHandler(apiRetStatus);
    }

    // LED
    apiRetStatus = CyU3PDeviceGpioOverride (LED0_GPIO, CyTrue);
	if (apiRetStatus != 0)
	{
		CyU3PDebugPrint (4, "GPIO Override failed, Error Code = %d\r\n", apiRetStatus);
		CyFxAppErrorHandler(apiRetStatus);
	}

	gpioConfig.outValue    = CyFalse;
	gpioConfig.driveLowEn  = CyTrue;
	gpioConfig.driveHighEn = CyTrue;
	gpioConfig.inputEn     = CyFalse;
	gpioConfig.intrMode    = CY_U3P_GPIO_NO_INTR;
	apiRetStatus           = CyU3PGpioSetSimpleConfig (LED0_GPIO, &gpioConfig);
	if (apiRetStatus != CY_U3P_SUCCESS)
	{
		CyU3PDebugPrint (4, "GPIO %d Set Config Error, Error Code = %d\r\n", LED0_GPIO, apiRetStatus);
		CyFxAppErrorHandler(apiRetStatus);
	}

    // Mux !OE Pin
    apiRetStatus = CyU3PDeviceGpioOverride (USB3_MUX_nOE_GPIO, CyTrue);
	if (apiRetStatus != 0)
	{
		CyU3PDebugPrint (4, "GPIO Override failed, Error Code = %d\r\n", apiRetStatus);
		CyFxAppErrorHandler(apiRetStatus);
	}

	gpioConfig.outValue    = CyFalse;
	gpioConfig.driveLowEn  = CyTrue;
	gpioConfig.driveHighEn = CyTrue;
	gpioConfig.inputEn     = CyFalse;
	gpioConfig.intrMode    = CY_U3P_GPIO_NO_INTR;
	apiRetStatus           = CyU3PGpioSetSimpleConfig (USB3_MUX_nOE_GPIO, &gpioConfig);
	if (apiRetStatus != CY_U3P_SUCCESS)
	{
		CyU3PDebugPrint (4, "GPIO %d Set Config Error, Error Code = %d\r\n", USB3_MUX_nOE_GPIO, apiRetStatus);
		CyFxAppErrorHandler(apiRetStatus);
	}

	// Mux SEL Pin
	apiRetStatus = CyU3PDeviceGpioOverride (USB3_MUX_SEL_GPIO, CyTrue);
	if (apiRetStatus != 0)
	{
		CyU3PDebugPrint (4, "GPIO Override failed, Error Code = %d\r\n", apiRetStatus);
		CyFxAppErrorHandler(apiRetStatus);
	}

	gpioConfig.outValue    = CyFalse;
	gpioConfig.driveLowEn  = CyTrue;
	gpioConfig.driveHighEn = CyTrue;
	gpioConfig.inputEn     = CyFalse;
	gpioConfig.intrMode    = CY_U3P_GPIO_NO_INTR;
	apiRetStatus           = CyU3PGpioSetSimpleConfig (USB3_MUX_SEL_GPIO, &gpioConfig);
	if (apiRetStatus != CY_U3P_SUCCESS)
	{
		CyU3PDebugPrint (4, "GPIO %d Set Config Error, Error Code = %d\r\n", USB3_MUX_SEL_GPIO, apiRetStatus);
		CyFxAppErrorHandler(apiRetStatus);
	}

	CyFxCDCAppPreInit();
	CyFxUVCAppPreInit();

	CyU3PDebugPrint(4, "CyFxAppInit\r\n");

	// USB initialization. 
	apiRetStatus = CyU3PUsbStart ();
	if (apiRetStatus != CY_U3P_SUCCESS)
	{
		CyU3PDebugPrint (4, "USB Function Failed to Start, Error Code = %d\r\n", apiRetStatus);
		CyFxAppErrorHandler(apiRetStatus);
	}

	// Setup the Callback to Handle the USB Setup Requests 
	CyU3PUsbRegisterSetupCallback (CyFxAppUSBSetupCB, CyTrue);

	// Setup the Callback to Handle the USB Events 
	CyU3PUsbRegisterEventCallback (CyFxAppUSBEventCB);

	// Register a callback to handle LPM requests from the USB 3.0 host. 
	CyU3PUsbRegisterLPMRequestCallback (CyFxAppLPMRqtCB);

	// Register the USB device descriptors with the driver. 
	apiRetStatus = CyU3PUsbSetDesc (CY_U3P_USB_SET_HS_DEVICE_DESCR, 0, (uint8_t *)CyFxUSBDeviceDscr);
	if (apiRetStatus != CY_U3P_SUCCESS)
	{
		CyU3PDebugPrint (4, "USB Descriptor %d failed, Error Code = %d\r\n", CY_U3P_USB_SET_HS_DEVICE_DESCR, apiRetStatus);
		CyFxAppErrorHandler (apiRetStatus);
	}

	apiRetStatus = CyU3PUsbSetDesc (CY_U3P_USB_SET_SS_DEVICE_DESCR, 0, (uint8_t *)CyFxUSBDeviceDscrSS);
	if (apiRetStatus != CY_U3P_SUCCESS)
	{
		CyU3PDebugPrint (4, "USB Descriptor %d failed, Error Code = %d\r\n", CY_U3P_USB_SET_SS_DEVICE_DESCR, apiRetStatus);
		CyFxAppErrorHandler (apiRetStatus);
	}

	// BOS and Device qualifier descriptors. 
	apiRetStatus = CyU3PUsbSetDesc (CY_U3P_USB_SET_DEVQUAL_DESCR, 0, (uint8_t *)CyFxUSBDeviceQualDscr);
	if (apiRetStatus != CY_U3P_SUCCESS)
	{
		CyU3PDebugPrint (4, "USB Descriptor %d failed, Error Code = %d\r\n", CY_U3P_USB_SET_DEVQUAL_DESCR, apiRetStatus);
		CyFxAppErrorHandler (apiRetStatus);
	}

	apiRetStatus = CyU3PUsbSetDesc (CY_U3P_USB_SET_SS_BOS_DESCR, 0, (uint8_t *)CyFxUSBBOSDscr);
	if (apiRetStatus != CY_U3P_SUCCESS)
	{
		CyU3PDebugPrint (4, "USB Descriptor %d failed, Error Code = %d\r\n", CY_U3P_USB_SET_SS_BOS_DESCR, apiRetStatus);
		CyFxAppErrorHandler (apiRetStatus);
	}

	// Configuration descriptors. 
	apiRetStatus = CyU3PUsbSetDesc (CY_U3P_USB_SET_HS_CONFIG_DESCR, 0, (uint8_t *)CyFxUSBHSConfigDscr);
	if (apiRetStatus != CY_U3P_SUCCESS)
	{
		CyU3PDebugPrint (4, "USB Descriptor %d failed, Error Code = %d\r\n", CY_U3P_USB_SET_HS_CONFIG_DESCR, apiRetStatus);
		CyFxAppErrorHandler (apiRetStatus);
	}

	apiRetStatus = CyU3PUsbSetDesc (CY_U3P_USB_SET_FS_CONFIG_DESCR, 0, (uint8_t *)CyFxUSBFSConfigDscr);
	if (apiRetStatus != CY_U3P_SUCCESS)
	{
		CyU3PDebugPrint (4, "USB Descriptor %d failed, Error Code = %d\r\n", CY_U3P_USB_SET_FS_CONFIG_DESCR, apiRetStatus);
		CyFxAppErrorHandler (apiRetStatus);
	}

	apiRetStatus = CyU3PUsbSetDesc (CY_U3P_USB_SET_SS_CONFIG_DESCR, 0, (uint8_t *)CyFxUSBSSConfigDscr);
	if (apiRetStatus != CY_U3P_SUCCESS)
	{
		CyU3PDebugPrint (4, "USB Descriptor %d failed, Error Code = %d\r\n", CY_U3P_USB_SET_SS_CONFIG_DESCR, apiRetStatus);
		CyFxAppErrorHandler (apiRetStatus);
	}

	// String Descriptors 
	apiRetStatus = CyU3PUsbSetDesc (CY_U3P_USB_SET_STRING_DESCR, 0, (uint8_t *)CyFxUSBStringLangIDDscr);
	if (apiRetStatus != CY_U3P_SUCCESS)
	{
		CyU3PDebugPrint (4, "USB Language Descriptor failed, Error Code = %d\r\n", apiRetStatus);
		CyFxAppErrorHandler (apiRetStatus);
	}

	apiRetStatus = CyU3PUsbSetDesc (CY_U3P_USB_SET_STRING_DESCR, 1, (uint8_t *)CyFxUSBManufactureDscr);
	if (apiRetStatus != CY_U3P_SUCCESS)
	{
		CyU3PDebugPrint (4, "USB Manufacturer Descriptor failed, Error Code = %d\r\n", apiRetStatus);
		CyFxAppErrorHandler (apiRetStatus);
	}

	apiRetStatus = CyU3PUsbSetDesc (CY_U3P_USB_SET_STRING_DESCR, 2, (uint8_t *)CyFxUSBProductDscr);
	if (apiRetStatus != CY_U3P_SUCCESS)
	{
		CyU3PDebugPrint (4, "USB Product Descriptor failed, Error Code = %d\r\n", apiRetStatus);
		CyFxAppErrorHandler (apiRetStatus);
	}

#ifdef USE_DEBUG_PORT
	apiRetStatus = CyU3PUsbSetDesc (CY_U3P_USB_SET_STRING_DESCR, 3, (uint8_t *)CyFxUSBDebugDscr);
	if (apiRetStatus != CY_U3P_SUCCESS)
	{
		CyU3PDebugPrint (4, "USB Debug Descriptor failed, Error Code = %d\r\n", apiRetStatus);
		CyFxAppErrorHandler (apiRetStatus);
	}

#endif

	apiRetStatus = CyU3PUsbSetDesc (CY_U3P_USB_SET_STRING_DESCR, 4, (uint8_t *)CyFxUSBSerialDscr);
	if (apiRetStatus != CY_U3P_SUCCESS)
	{
		CyU3PDebugPrint (4, "USB Serial Descriptor failed, Error Code = %d\r\n", apiRetStatus);
		CyFxAppErrorHandler (apiRetStatus);
	}

	// Disable USB2
	apiRetStatus = CyU3PUsbControlUsb2Support(CyFalse);

	if (apiRetStatus != CY_U3P_SUCCESS)
	{
		CyU3PDebugPrint (4, "Disabling USB2 failed, Error Code = %d\r\n", apiRetStatus);
		CyFxAppErrorHandler (apiRetStatus);
	}

	CyU3PBusyWait(50);

	// Try to Connect via USB3
	apiRetStatus = CyU3PConnectState(CyTrue, CyTrue);

	CyU3PBusyWait(300);

	if (CyU3PUsbGetSpeed() != CY_U3P_SUPER_SPEED)
	{
		CyU3PConnectState(CyFalse, CyFalse);

		CyU3PGpioSetValue(USB3_MUX_SEL_GPIO, CyTrue); // Switch Mux

		CyU3PUsbControlUsb2Support(CyTrue);

		CyU3PBusyWait(200);

		// Enable USB connection from the FX3 device, preferably at USB 3.0 speed.
		apiRetStatus = CyU3PConnectState (CyTrue, CyTrue);
		if (apiRetStatus != CY_U3P_SUCCESS)
		{
			CyU3PDebugPrint (4, "USB3 Connect failed, Error Code = %d\r\n", apiRetStatus);
			CyFxAppErrorHandler (apiRetStatus);
		}

		CyU3PBusyWait(100);
	}

	CyFxCDCAppInit();
#ifdef USE_DEBUG_PORT
	CyFxDBGAppInit();
#endif
	CyFxUVCAppInit();

	CyU3PDebugPrint (4, "USB Connected\r\n");
	CyU3PGpioSetValue(60, CyTrue);
}

/*
 * This function is called by the FX3 framework once the ThreadX RTOS has started up.
 * The application specific threads and other OS resources are created and initialized here.
 */
void
CyFxApplicationDefine (
        void)
{
    uint32_t apiRetStatus = CY_U3P_SUCCESS;

    CyU3PDebugPrint(4, "CyFxApplicationDefine\r\n");

    CyFxAppInit();

#ifdef USE_DEBUG_PORT
    void *ptr0;

    ptr0 = CyU3PMemAlloc(CY_FX_DBG_THREAD_STACK);

    if (ptr0 == 0)
	{
		CyU3PDebugPrint (4, "CyU3PMemAlloc for DBG failed, Error Code = %d\r\n", CY_U3P_ERROR_MEMORY_ERROR);
		CyFxAppErrorHandler (CY_U3P_ERROR_MEMORY_ERROR);
	}

    // Create thread for Debug handling. 
    apiRetStatus = CyU3PThreadCreate (&dbgAppThread,	// App Thread structure
    			 "20:Debug App Thread",        			// Thread ID and Thread name
				 DBGAppThread_Entry,        			// App Thread Entry function
				 0,                         			// No input parameter to thread
				 ptr0,                      			// Pointer to the allocated thread stack
				 CY_FX_DBG_THREAD_STACK,    			// App Thread stack size
				 CY_FX_DBG_THREAD_PRIORITY, 			// App Thread priority
				 CY_FX_DBG_THREAD_PRIORITY, 			// App Thread pre-emption threshold
				 CYU3P_NO_TIME_SLICE,       			// No time slice for the application thread
				 CYU3P_AUTO_START           			// Start the Thread immediately
				 );

    if (apiRetStatus != CY_U3P_SUCCESS)
	{
		CyU3PDebugPrint (4, "DBG Thread Create failed, Error Code = %d\r\n", apiRetStatus);
		CyFxAppErrorHandler (apiRetStatus);
	}
#endif // USE_DEBUG_PORT

    void *ptr1, *ptr2, *ptr3;

    ptr1 = CyU3PMemAlloc(CY_FX_CDC_THREAD_STACK);
    ptr2 = CyU3PMemAlloc(UVC_APP_THREAD_STACK);
    ptr3 = CyU3PMemAlloc(UVC_APP_THREAD_STACK);

    if ((ptr1 == 0) || (ptr2 == 0) || (ptr3 == 0))
    {
		CyU3PDebugPrint (4, "CyU3PMemAlloc failed, Error Code = %d\r\n", CY_U3P_ERROR_MEMORY_ERROR);
		CyFxAppErrorHandler (CY_U3P_ERROR_MEMORY_ERROR);
	}

    if (apiRetStatus != CY_U3P_SUCCESS)
	{
		CyU3PDebugPrint (4, "EP0 Thread Create failed, Error Code = %d\r\n", apiRetStatus);
		CyFxAppErrorHandler (apiRetStatus);
	}

    // Create thread for CDC handling.
    apiRetStatus = CyU3PThreadCreate (&cdcAppThread,    // CDC App Thread structure
                "30:CDC App Thread",                    // Thread ID and Thread name
                CDCAppThread_Entry,                  	// CDC App Thread Entry function
                0,                                      // No input parameter to thread
                ptr1,                                   // Pointer to the allocated thread stack
                CY_FX_CDC_THREAD_STACK,              	// CDC App Thread stack size
                CY_FX_CDC_THREAD_PRIORITY,            	// CDC App Thread priority
                CY_FX_CDC_THREAD_PRIORITY,            	// CDC App Thread priority
                CYU3P_NO_TIME_SLICE,                    // No time slice for the application thread
                CYU3P_AUTO_START                        // Start the Thread immediately
                );

    if (apiRetStatus != CY_U3P_SUCCESS)
	{
		CyU3PDebugPrint (4, "CDC Thread Create failed, Error Code = %d\r\n", apiRetStatus);
		CyFxAppErrorHandler (apiRetStatus);
	}

    // Create the UVC application thread. 
    apiRetStatus = CyU3PThreadCreate (&uvcAppThread,   	// UVC Thread structure 
            "40:UVC App Thread",                       	// Thread Id and name
            UVCAppThread_Entry,                         // UVC Application Thread Entry function 
            0,                                          // No input parameter to thread 
            ptr2,                                       // Pointer to the allocated thread stack 
            UVC_APP_THREAD_STACK,                       // UVC Application Thread stack size 
            UVC_APP_THREAD_PRIORITY,                    // UVC Application Thread priority 
            UVC_APP_THREAD_PRIORITY,                    // Threshold value for thread pre-emption. 
            CYU3P_NO_TIME_SLICE,                        // No time slice for the application thread 
            CYU3P_AUTO_START                            // Start the Thread immediately 
            );

    if (apiRetStatus != CY_U3P_SUCCESS)
    {
    	CyU3PDebugPrint(4, "UVC Thread Create failed, Error Code = %d\r\n", apiRetStatus);
    	CyFxAppErrorHandler(apiRetStatus);
    }

    // Create the control request handling thread. 
    apiRetStatus = CyU3PThreadCreate (&uvcAppEP0Thread,  // UVC Thread structure 
            "41:UVC App EP0 Thread",                     // Thread Id and name
            UVCAppEP0Thread_Entry,                       // UVC Application EP0 Thread Entry function 
            0,                                           // No input parameter to thread 
            ptr3,                                        // Pointer to the allocated thread stack 
            UVC_APP_EP0_THREAD_STACK,                    // UVC Application Thread stack size 
            UVC_APP_EP0_THREAD_PRIORITY,                 // UVC Application Thread priority 
            UVC_APP_EP0_THREAD_PRIORITY,                 // Threshold value for thread pre-emption. 
            CYU3P_NO_TIME_SLICE,                         // No time slice for the application thread 
            CYU3P_AUTO_START                             // Start the Thread immediately 
            );

    return;
}

int
main (
        void)
{
    CyU3PReturnStatus_t apiRetStatus;
    CyU3PIoMatrixConfig_t io_cfg;
    CyU3PSysClockConfig_t clockConfig;

    CyU3PDebugPrint(4, "Main\r\n");

    clockConfig.setSysClk400  = CyTrue;
	clockConfig.cpuClkDiv     = 2;
	clockConfig.dmaClkDiv     = 2;
	clockConfig.mmioClkDiv    = 2;
	clockConfig.useStandbyClk = CyFalse;
	clockConfig.clkSrc        = CY_U3P_SYS_CLK;

	apiRetStatus = CyU3PDeviceInit (&clockConfig);

	if (apiRetStatus != CY_U3P_SUCCESS)
	{
		CyU3PDebugPrint (4, "CyU3PDeviceInit failed, Error Code = %d\r\n", apiRetStatus);
		CyFxAppErrorHandler (apiRetStatus);
	}

    // Turn on instruction cache to improve firmware performance. Use Release build to improve it further 
    apiRetStatus = CyU3PDeviceCacheControl (CyTrue, CyFalse, CyFalse);

    // Configure the IO matrix for the device. 
    io_cfg.isDQ32Bit        = CyTrue;
    io_cfg.s0Mode       	= CyFalse;
    io_cfg.s1Mode	        = CyFalse;
    io_cfg.lppMode          = CY_U3P_IO_MATRIX_LPP_DEFAULT;
    io_cfg.gpioSimpleEn[0]  = 0;
    io_cfg.gpioSimpleEn[1]  = 0;
    io_cfg.gpioComplexEn[0] = 0;
    io_cfg.gpioComplexEn[1] = 0;
    io_cfg.useUart          = CyTrue;	// With DQ32 only one of UART,SPI,I2S can be used.
    io_cfg.useI2C           = CyTrue;   // I2C is used for the sensor interface. 
    io_cfg.useI2S           = CyFalse;
    io_cfg.useSpi           = CyFalse;

    apiRetStatus = CyU3PDeviceConfigureIOMatrix (&io_cfg);

    if (apiRetStatus != CY_U3P_SUCCESS)
	{
		CyU3PDebugPrint (4, "CyU3PDeviceConfigureIOMatrix failed, Error Code = %d\r\n", apiRetStatus);
		CyFxAppErrorHandler (apiRetStatus);
	}

    // This is a non returnable call for initializing the RTOS kernel 
    CyU3PKernelEntry ();

    // Dummy return to make the compiler happy 
    return 0;
}
