#include <cyu3system.h>
#include <cyu3os.h>
#include <cyu3dma.h>
#include <cyu3error.h>
#include <cyu3uart.h>
#include <cyu3i2c.h>
#include <cyu3types.h>
#include <cyu3gpio.h>
#include <cyu3utils.h>
#include "sensor.h"
#include "uvc.h"

// I2C initialization.
static void
SensorI2CInit (void)
{
    CyU3PI2cConfig_t i2cConfig;;
    CyU3PReturnStatus_t status;

    status = CyU3PI2cInit ();
    if (status != CY_U3P_SUCCESS)
    {
        CyU3PDebugPrint (4, "I2C initialization failed!\n");
    }

    //  Set I2C Configuration
    i2cConfig.bitRate    = 100000;      //  100 KHz
    i2cConfig.isDma      = CyFalse;
    i2cConfig.busTimeout = 0xffffffffU;
    i2cConfig.dmaTimeout = 0xffff;

    status = CyU3PI2cSetConfig (&i2cConfig, 0);
    if (CY_U3P_SUCCESS != status)
    {
        CyU3PDebugPrint (4, "I2C configuration failed!\n");
    }
}

/* This function inserts a delay between successful I2C transfers to prevent
   false errors due to the slave being busy.
 */
static void
SensorI2CAccessDelay (
        CyU3PReturnStatus_t status)
{
    // Add a 10us delay if the I2C operation that preceded this call was successful. 
    if (status == CY_U3P_SUCCESS)
        CyU3PBusyWait (10);
}

// Write to an I2C slave with two bytes of data. 
CyU3PReturnStatus_t
SensorWrite2B (
        uint8_t slaveAddr,
        uint8_t highAddr,
        uint8_t lowAddr,
        uint8_t highData,
        uint8_t lowData)
{
    CyU3PReturnStatus_t apiRetStatus = CY_U3P_SUCCESS;
    CyU3PI2cPreamble_t  preamble;
    uint8_t buf[2];

    // Validate the I2C slave address. 
    if (slaveAddr != SENSOR_ADDR_WR)
    {
        CyU3PDebugPrint (4, "I2C Slave address is not valid!\n");
        return 1;
    }

    // Set the parameters for the I2C API access and then call the write API. 
    preamble.buffer[0] = slaveAddr;
    preamble.buffer[1] = highAddr;
    preamble.buffer[2] = lowAddr;
    preamble.length    = 3;             //  Three byte preamble. 
    preamble.ctrlMask  = 0x0000;        //  No additional start and stop bits. 

    buf[0] = highData;
    buf[1] = lowData;

    apiRetStatus = CyU3PI2cTransmitBytes (&preamble, buf, 2, 0);
    SensorI2CAccessDelay (apiRetStatus);

    return apiRetStatus;
}

CyU3PReturnStatus_t
SensorWrite (
        uint8_t slaveAddr,
        uint8_t highAddr,
        uint8_t lowAddr,
        uint8_t count,
        uint8_t *buf)
{
    CyU3PReturnStatus_t apiRetStatus = CY_U3P_SUCCESS;
    CyU3PI2cPreamble_t preamble;

    // Validate the I2C slave address. 
    if (slaveAddr != SENSOR_ADDR_WR)
    {
        CyU3PDebugPrint (4, "I2C Slave address is not valid!\n");
        return 1;
    }

    if (count > 64)
    {
        CyU3PDebugPrint (4, "ERROR: SensorWrite count > 64\n");
        return 1;
    }

    // Set up the I2C control parameters and invoke the write API. 
    preamble.buffer[0] = slaveAddr;
    preamble.buffer[1] = highAddr;
    preamble.buffer[2] = lowAddr;
    preamble.length    = 3;
    preamble.ctrlMask  = 0x0000;

    apiRetStatus = CyU3PI2cTransmitBytes (&preamble, buf, count, 0);
    SensorI2CAccessDelay (apiRetStatus);

    return apiRetStatus;
}

CyU3PReturnStatus_t
SensorRead2B (
        uint8_t slaveAddr,
        uint8_t highAddr,
        uint8_t lowAddr,
        uint8_t *buf)
{
    CyU3PReturnStatus_t apiRetStatus = CY_U3P_SUCCESS;
    CyU3PI2cPreamble_t preamble;

    if (slaveAddr != SENSOR_ADDR_RD)
    {
        CyU3PDebugPrint (4, "I2C Slave address is not valid!\n");
        return 1;
    }

    preamble.buffer[0] = slaveAddr & I2C_SLAVEADDR_MASK;        //  Mask out the transfer type bit. 
    preamble.buffer[1] = highAddr;
    preamble.buffer[2] = lowAddr;
    preamble.buffer[3] = slaveAddr;
    preamble.length    = 4;
    preamble.ctrlMask  = 0x0004;                                //  Send start bit after third byte of preamble. 

    apiRetStatus = CyU3PI2cReceiveBytes (&preamble, buf, 2, 0);
    SensorI2CAccessDelay (apiRetStatus);

    return apiRetStatus;
}

CyU3PReturnStatus_t
SensorRead (
        uint8_t slaveAddr,
        uint8_t highAddr,
        uint8_t lowAddr,
        uint8_t count,
        uint8_t *buf)
{
    CyU3PReturnStatus_t apiRetStatus = CY_U3P_SUCCESS;
    CyU3PI2cPreamble_t preamble;

    // Validate the parameters. 
    if (slaveAddr != SENSOR_ADDR_RD)
    {
        CyU3PDebugPrint (4, "I2C Slave address is not valid!\n");
        return 1;
    }
    if ( count > 64 )
    {
        CyU3PDebugPrint (4, "ERROR: SensorWrite count > 64\n");
        return 1;
    }

    preamble.buffer[0] = slaveAddr & I2C_SLAVEADDR_MASK;        //  Mask out the transfer type bit. 
    preamble.buffer[1] = highAddr;
    preamble.buffer[2] = lowAddr;
    preamble.buffer[3] = slaveAddr;
    preamble.length    = 4;
    preamble.ctrlMask  = 0x0004;                                //  Send start bit after third byte of preamble. 

    apiRetStatus = CyU3PI2cReceiveBytes (&preamble, buf, count, 0);
    SensorI2CAccessDelay (apiRetStatus);

    return apiRetStatus;
}

void
SensorReset (
        void)
{
    CyU3PReturnStatus_t apiRetStatus;

    // Drive the GPIO low to reset the sensor. 
    apiRetStatus = CyU3PGpioSetValue (SENSOR_RESET_GPIO, CyFalse);
    if (apiRetStatus != CY_U3P_SUCCESS)
    {
        CyU3PDebugPrint (4, "GPIO Set Value Error, Error Code = %d\n", apiRetStatus);
        return;
    }

    // Wait for some time to allow proper reset. 
    CyU3PThreadSleep (10);

    // Drive the GPIO high to bring the sensor out of reset. 
    apiRetStatus = CyU3PGpioSetValue (SENSOR_RESET_GPIO, CyTrue);
    if (apiRetStatus != CY_U3P_SUCCESS)
    {
        CyU3PDebugPrint (4, "GPIO Set Value Error, Error Code = %d\n", apiRetStatus);
        return;
    }

    // Delay the allow the sensor to power up. 
    CyU3PThreadSleep (10);
    return;
}

void
SensorInit (
        void)
{
	SensorI2CInit();

    if (SensorI2cBusTest () != CY_U3P_SUCCESS)        // Verify that the sensor is connected. 
    {
        CyU3PDebugPrint (4, "Error: Reading Sensor ID failed!\r\n");
        return;
    }

    // Update sensor configuration based on desired video stream parameters. Using 720p 30fps as default setting.
    SensorScaling_FULL();
}

/*
 * Verify that the sensor can be accessed over the I2C bus from FX3.
 */
uint8_t
SensorI2cBusTest (
        void)
{
    // The sensor ID register can be read here to verify sensor connectivity. 
    uint8_t buf[2];

    // Reading sensor ID 
    if (SensorRead2B (SENSOR_ADDR_RD, 0x00, 0x00, buf) == CY_U3P_SUCCESS)
    {
        if ((buf[0] == 0x24) && (buf[1] == 0x81))
        {
            return CY_U3P_SUCCESS;
        }
    }
    return 1;
}

void SensorSetResolution(uint8_t frameIndex)
{
	switch (frameIndex)
	{
		case UVC_SS_HALF_FRAME_INDEX:
			SensorScaling_HALF();
			break;

		case UVC_SS_FULL_FRAME_INDEX:
			SensorScaling_FULL();
			break;

		default:
			break;
	}
}

void
SensorScaling_HALF (
        void)
{

}

void
SensorScaling_FULL (
        void)
{

}

uint8_t
SensorGetBrightness (
        void)
{
    return 0;
}

void
SensorSetBrightness (
        uint8_t input)
{

}


