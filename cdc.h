#ifndef _CDC_H_
#define _CDC_H_

#include <cyu3os.h>
#include <cyu3usb.h>
#include <cyu3types.h>
#include <cyu3usbconst.h>
#include <cyu3externcstart.h>

extern CyU3PThread       cdcAppThread;

CyBool_t CyFxCDCAppUSBSetupCB (uint32_t setupdat0, uint32_t setupdat1);
void CyFxCDCAppUSBEventCB (CyU3PUsbEventType_t evtype, uint16_t evdata);
void CDCAppThread_Entry (uint32_t input);
void CyFxCDCAppPreInit (void);
void CyFxCDCAppInit (void);

#define CDC_DEBUG_PIN					23

#define CY_FX_CDC_DMA_BUF_COUNT      	(6)
#define CY_FX_CDC_THREAD_STACK       	(0x400)
#define CY_FX_CDC_THREAD_PRIORITY    	(8)

#define CY_FX_CDC_CTRL_INTF				2

#define CY_FX_CDC_INT_EP				1
#define CY_FX_CDC_EP				  	2

#define CY_FX_EP_IN_TYPE                0x80
#define CY_FX_EP_CDC_PRODUCER           (CY_FX_CDC_EP)							// EP OUT
#define CY_FX_EP_CDC_CONSUMER           (CY_FX_CDC_EP 	  | CY_FX_EP_IN_TYPE) 	// EP IN
#define CY_FX_EP_CDC_INTERRUPT          (CY_FX_CDC_INT_EP | CY_FX_EP_IN_TYPE)	// EP INT

#define CY_FX_EP_TX_PROD_SOCKET			(CY_U3P_UIB_SOCKET_PROD_0 | CY_FX_CDC_EP)
#define CY_FX_EP_TX_CONS_SOCKET			CY_U3P_LPP_SOCKET_UART_CONS
#define CY_FX_EP_RX_PROD_SOCKET			CY_U3P_LPP_SOCKET_UART_PROD
#define CY_FX_EP_RX_CONS_SOCKET			(CY_U3P_UIB_SOCKET_CONS_0 | CY_FX_CDC_EP)

#include "cyu3externcend.h"

#endif // _CDC_H_

