#ifndef _DBG_H_
#define _DBG_H_

#include "cyu3types.h"
#include "cyu3usbconst.h"
#include "cyu3externcstart.h"

extern CyU3PThread     dbgAppThread;

//#define USE_DEBUG_PORT

void DBGAppThread_Entry (uint32_t input);
void CyFxDBGAppInit(void);
CyBool_t CyFxDBGAppUSBSetupCB (uint32_t setupdat0, uint32_t setupdat1);
void CyFxDBGAppUSBEventCB (CyU3PUsbEventType_t evtype, uint16_t evdata);
void UVCAppEP0Thread_Entry (uint32_t input);
CyBool_t CyFxDBGAppLPMRqtCB(CyU3PUsbLinkPowerMode link_mode);

#define CY_FX_DBG_THREAD_STACK       (0x100)
#define CY_FX_DBG_THREAD_PRIORITY    (7)

#define CY_FX_DBG_CTRL_INTF			4

#define CY_FX_DBG_INT_EP			5
#define CY_FX_DBG_EP				6

#define CY_FX_EP_IN_TYPE            0x80
#define CY_FX_EP_DBG_PRODUCER		(CY_FX_DBG_EP)							// EP OUT
#define CY_FX_EP_DBG_CONSUMER		(CY_FX_DBG_EP | CY_FX_EP_IN_TYPE) 		// EP IN
#define CY_FX_EP_DBG_INTERRUPT		(CY_FX_DBG_INT_EP | CY_FX_EP_IN_TYPE)	// EP INT

#define CY_FX_EP_DBG_SOCKET         (CY_U3P_UIB_SOCKET_CONS_0 | CY_FX_DBG_EP)

#include "cyu3externcend.h"

#endif // _DBG_H_
