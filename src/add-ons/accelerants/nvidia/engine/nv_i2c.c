/*
 * i2c interface.
 * Bus should be run at max. 100kHz: see original Philips I2C specification
 *	
 * Rudolf Cornelissen 12/2002-9/2005
 */

#define MODULE_BIT 0x00004000

#include "nv_std.h"

status_t i2c_sec_tv_adapter()
{
	status_t result = B_ERROR;

	/* The secondary DDC channel only exist on dualhead cards */
	if (!si->ps.secondary_head) return result;

	/* make sure the output lines will be active-low when enabled
	 * (they will be pulled 'passive-high' when disabled) */
//	DXIW(GENIODATA,0x00);
	/* send out B_STOP condition on secondary head DDC channel and use it to
	 * check for 'shortcut', indicating the Matrox VGA->TV adapter is connected */

	/* make sure SDA is low */
//	DXIW(GENIOCTRL, (DXIR(GENIOCTRL) | DDC2_DATA));
	snooze(2);
	/* make sure SCL should be high */
//	DXIW(GENIOCTRL, (DXIR(GENIOCTRL) & ~DDC2_CLK));
	snooze(2);
	/* if SCL is low then the bus is blocked by a TV adapter */
//	if (!(DXIR(GENIODATA) & DDC2_CLK)) result = B_OK;
	snooze(5);
	/* set SDA while SCL should be set (generates actual bus-stop condition) */
//	DXIW(GENIOCTRL, (DXIR(GENIOCTRL) & ~DDC2_DATA));
	snooze(5);

	return result;
}

static char FlagIICError (char ErrNo)
//error code list:
//1 - SCL locked low by device (bus is still busy)
//2 - SDA locked low by device (bus is still busy)
//3 - No Acknowledge from device (no handshake)
//4 - SDA not released for master to generate STOP bit
{
	static char IICError = 0;

	if (!IICError) IICError = ErrNo;
	if (ErrNo == -1) IICError = 0;
	return IICError;
}

static void OutSCL(uint8 BusNR, bool Bit)
{
	uint8 data;

	if (BusNR)
	{
		data = (CRTCR(WR_I2CBUS_1) & 0xf0) | 0x01;
		if (Bit)
			CRTCW(WR_I2CBUS_1, (data | 0x20));
		else
			CRTCW(WR_I2CBUS_1, (data & ~0x20));
	}
	else
	{
		data = (CRTCR(WR_I2CBUS_0) & 0xf0) | 0x01;
		if (Bit)
			CRTCW(WR_I2CBUS_0, (data | 0x20));
		else
			CRTCW(WR_I2CBUS_0, (data & ~0x20));
	}
}

static void OutSDA(uint8 BusNR, bool Bit)
{
	uint8 data;
	
	if (BusNR)
	{
		data = (CRTCR(WR_I2CBUS_1) & 0xf0) | 0x01;
		if (Bit)
			CRTCW(WR_I2CBUS_1, (data | 0x10));
		else
			CRTCW(WR_I2CBUS_1, (data & ~0x10));
	}
	else
	{
		data = (CRTCR(WR_I2CBUS_0) & 0xf0) | 0x01;
		if (Bit)
			CRTCW(WR_I2CBUS_0, (data | 0x10));
		else
			CRTCW(WR_I2CBUS_0, (data & ~0x10));
	}
}

static bool InSCL(uint8 BusNR)
{
	if (BusNR)
	{
		if ((CRTCR(RD_I2CBUS_1) & 0x04)) return true;
	}
	else
	{
		if ((CRTCR(RD_I2CBUS_0) & 0x04)) return true;
	}

	return false;
}

static bool InSDA(uint8 BusNR)
{
	if (BusNR)
	{
		if ((CRTCR(RD_I2CBUS_1) & 0x08)) return true;
	}
	else
	{
		if ((CRTCR(RD_I2CBUS_0) & 0x08)) return true;
	}

	return false;
}

static void TXBit (uint8 BusNR, bool Bit)
{
	/* send out databit */
	if (Bit)
	{
		OutSDA(BusNR, true);
		snooze(3);
		if (!InSDA(BusNR)) FlagIICError (2);
	}
	else
	{
		OutSDA(BusNR, false);
	}
	/* generate clock pulse */
	snooze(6);
	OutSCL(BusNR, true);
	snooze(3);
	if (!InSCL(BusNR)) FlagIICError (1);
	snooze(6);
	OutSCL(BusNR, false);
	snooze(6);
}

static uint8 RXBit (uint8 BusNR)
{
	uint8 Bit = 0;

	/* set SDA so input is possible */
	OutSDA(BusNR, true);
	/* generate clock pulse */
	snooze(6);
	OutSCL(BusNR, true);
	snooze(3);
	if (!InSCL(BusNR)) FlagIICError (1);
	snooze(3);
	/* read databit */
	if (InSDA(BusNR)) Bit = 1;
	/* finish clockpulse */
	OutSCL(BusNR, false);
	snooze(6);

	return Bit;
}

static void bstart (uint8 BusNR)
{
	/* make sure SDA is high */
	OutSDA(BusNR, true);
	snooze(3);
	OutSCL(BusNR, true);
	snooze(3);
	if (!InSCL(BusNR)) FlagIICError (1);
	snooze(6);
	/* clear SDA while SCL set (bus-start condition) */
	OutSDA(BusNR, false);
	snooze(6);
	OutSCL(BusNR, false);
	snooze(6);

	LOG(4,("I2C: START condition generated on bus %d; status is %d\n",
		BusNR, FlagIICError (0)));
}

static void bstop (uint8 BusNR)
{
	/* make sure SDA is low */
	OutSDA(BusNR, false);
	snooze(3);
	OutSCL(BusNR, true);
	snooze(3);
	if (!InSCL(BusNR)) FlagIICError (1);
	snooze(6);
	/* set SDA while SCL set (bus-stop condition) */
	OutSDA(BusNR, true);
	snooze(3);
	if (!InSDA(BusNR)) FlagIICError (4);
	snooze(3);

	LOG(4,("I2C: STOP condition generated on bus %d; status is %d\n",
		BusNR, FlagIICError (0)));
}

static uint8 i2c_readbyte(uint8 BusNR, bool Ack)
{
	uint8 cnt, bit, byte = 0;

	/* enable access to primary head */
	set_crtc_owner(0);

	/* read data */
	for (cnt = 8; cnt > 0; cnt--)
	{
		byte <<= 1;
		bit = RXBit (BusNR);
		byte += bit;
	}
	/* send acknowledge */
	TXBit (BusNR, Ack);

	LOG(4,("I2C: read byte ($%02x) from bus #%d; status is %d\n",
		byte, BusNR, FlagIICError(0)));

	return byte;
}

static bool i2c_writebyte (uint8 BusNR, uint8 byte)
{
	uint8 cnt;
	bool bit;

	/* enable access to primary head */
	set_crtc_owner(0);

	/* write data */
	for (cnt = 8; cnt > 0; cnt--)
	{
		bit = (byte & 0x80);
		TXBit (BusNR, bit);
		byte <<= 1;
	}
	/* read acknowledge */
	bit = RXBit (BusNR);
	if (bit) FlagIICError (3);

	LOG(4,("I2C: written byte ($%02x) to bus #%d; status is %d\n",
		byte, BusNR, FlagIICError(0)));

	return bit;
}
//end rud.

/*-------------------------------------------
 *PUBLIC functions
 */ 
int i2c_maven_read(unsigned char address)
{
	int error=0;
	int data=0;
/*
	i2c_start();
	{
		error+=i2c_sendbyte(MAVEN_READ);
		error+=i2c_sendbyte(address);
//		data = i2c_readbyte(0);
	}	
	i2c_stop();
*/
	if (error>0) LOG(8,("I2C: MAVR ERROR - %x\n",error));
	return data;
}

void i2c_maven_write(unsigned char address, unsigned char data)
{
	int error=0;
/*
	i2c_start();
	{
		error+=i2c_sendbyte(MAVEN_WRITE);
		error+=i2c_sendbyte(address);
		error+=i2c_sendbyte(data);
	}	
	i2c_stop();
*/
	if (error>0) LOG(8,("I2C: MAVW ERROR - %x\n",error));
}

status_t i2c_init(void)
{
	uint8 bus;
	bool *i2c_bus = &(si->ps.i2c_bus0);

	LOG(4,("I2C: searching for wired I2C buses...\n"));

	/* preset no board wired buses */
	si->ps.i2c_bus0 = false;
	si->ps.i2c_bus1 = false;

	/* find existing buses */	
	for (bus = 0; bus < 2; bus++)
	{
		/* reset status */
		FlagIICError (-1);
		snooze(6);
		/* init and/or stop I2C bus */
		bstop(bus);
		/* check for hardware coupling of SCL and SDA -out and -in lines */
		snooze(6);
		OutSCL(bus, false);
		OutSDA(bus, true);
		snooze(3);
		if (InSCL(bus) || !InSDA(bus)) continue;
		snooze(3);
		OutSCL(bus, true);
		OutSDA(bus, false);
		snooze(3);
		if (!InSCL(bus) || InSDA(bus)) continue;
		i2c_bus[bus] = true;
		snooze(3);
		/* re-init bus */
		bstop(bus);
	}

	for (bus = 0; bus < 2; bus++)
	{
		if (i2c_bus[bus])
			LOG(4,("I2C: bus #%d wiring check: passed\n", bus));
		else
			LOG(4,("I2C: bus #%d wiring check: failed\n", bus));
	}

	return B_OK;
}
