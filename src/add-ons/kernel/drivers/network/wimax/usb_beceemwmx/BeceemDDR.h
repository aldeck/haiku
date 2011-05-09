/*
 *	Beceem WiMax USB Driver.
 *	Copyright (c) 2010 Alexander von Gluck <kallisti5@unixzen.com>
 *	Distributed under the terms of the GNU General Public License.
 *	
 *	Based on GPL code developed by: Beceem Communications Pvt. Ltd
 *	
 *	Description: Wrangle Beceem volatile DDR memory.
 */

#ifndef _USB_BECEEM_DDR_H_
#define _USB_BECEEM_DDR_H_

#include <ByteOrder.h>

#include "DeviceStruct.h"

#define DDR_DUMP_INTERNAL_DEVICE_MEMORY		0xBFC02B00
#define MIPS_CLOCK_REG						0x0f000820

#define MIPS_200_MHZ	0
#define MIPS_160_MHZ	1
#define PLL_800_MHZ		0
#define PLL_266_MHZ		1

#define DDR_80_MHZ      0
#define DDR_100_MHZ     1
#define DDR_120_MHZ     2 //  Additional Frequency for T3LP
#define DDR_133_MHZ     3
#define DDR_140_MHZ     4 //  Not Used (Reserved for future)
#define DDR_160_MHZ     5 //  Additional Frequency for T3LP
#define DDR_180_MHZ     6 //  Not Used (Reserved for future)
#define DDR_200_MHZ     7 //  Not Used (Reserved for future)

class BeceemDDR
{

public:
		WIMAX_DEVICE*	pwmxdevice;

						BeceemDDR();
		status_t		DDRInit(WIMAX_DEVICE* swmxdevice);

// yuck.  These are in a child class class
virtual status_t    ReadRegister(unsigned int reg, size_t size, uint32_t* buffer){ return NULL; };
virtual status_t    WriteRegister(unsigned int reg, size_t size, uint32_t* buffer){ return NULL; };
virtual status_t    BizarroReadRegister(unsigned int reg, size_t size, uint32_t* buffer){ return NULL; };
virtual status_t    BizarroWriteRegister(unsigned int reg, size_t size, uint32_t* buffer){ return NULL; };

};


/*
 * DDR Power modes
 */
typedef enum ePMU_MODES
{
	HYBRID_MODE_7C  = 0,
	INTERNAL_MODE_6 = 1,
	HYBRID_MODE_6   = 2
}PMU_MODE;

/*
 * DDR Init maps, taken from Beceem GPL Linux Driver
 */

typedef struct _DDR_SETTING
{
	unsigned long	ulRegAddress;
    unsigned long	ulRegValue;
}DDR_SETTING, *PDDR_SETTING;
typedef DDR_SETTING DDR_SET_NODE, *PDDR_SET_NODE;

//DDR INIT-133Mhz
#define T3_SKIP_CLOCK_PROGRAM_DUMP_133MHZ 12  //index for 0x0F007000
static DDR_SET_NODE asT3_DDRSetting133MHz[]= {//      # DPLL Clock Setting
                                        {0x0F000800,0x00007212},
                                        {0x0f000820,0x07F13FFF},
                                        {0x0f000810,0x00000F95},
                                        {0x0f000860,0x00000000},
                                        {0x0f000880,0x000003DD},
                                        // Changed source for X-bar and MIPS clock to APLL
                                        {0x0f000840,0x0FFF1B00},
                                        {0x0f000870,0x00000002},
                                        {0x0F00a044,0x1fffffff},
                                        {0x0F00a040,0x1f000000},
                                        {0x0F00a084,0x1Cffffff},
                                        {0x0F00a080,0x1C000000},
                                        {0x0F00a04C,0x0000000C},
                                        //Memcontroller Default values
                                        {0x0F007000,0x00010001},
                                        {0x0F007004,0x01010100},
                                        {0x0F007008,0x01000001},
                                        {0x0F00700c,0x00000000},
                                        {0x0F007010,0x01000000},
                                        {0x0F007014,0x01000100},
                                        {0x0F007018,0x01000000},
                                        {0x0F00701c,0x01020001},// POP - 0x00020001 Normal 0x01020001
                                        {0x0F007020,0x04030107}, //Normal - 0x04030107 POP - 0x05030107
                                        {0x0F007024,0x02000007},
                                        {0x0F007028,0x02020202},
                                        {0x0F00702c,0x0206060a},//ROB- 0x0205050a,//0x0206060a
                                        {0x0F007030,0x05000000},
                                        {0x0F007034,0x00000003},
                                        {0x0F007038,0x110a0200},//ROB - 0x110a0200,//0x180a0200,// 0x1f0a0200
                                        {0x0F00703C,0x02101010},//ROB - 0x02101010,//0x02101018},
                                        {0x0F007040,0x45751200},//ROB - 0x45751200,//0x450f1200},
                                        {0x0F007044,0x110a0d00},//ROB - 0x110a0d00//0x111f0d00
                                        {0x0F007048,0x081b0306},
                                        {0x0F00704c,0x00000000},
                                        {0x0F007050,0x0000001c},
                                        {0x0F007054,0x00000000},
                                        {0x0F007058,0x00000000},
                                        {0x0F00705c,0x00000000},
                                        {0x0F007060,0x0010246c},
                                        {0x0F007064,0x00000010},
                                        {0x0F007068,0x00000000},
                                        {0x0F00706c,0x00000001},
                                        {0x0F007070,0x00007000},
                                        {0x0F007074,0x00000000},
                                        {0x0F007078,0x00000000},
                                        {0x0F00707C,0x00000000},
                                        {0x0F007080,0x00000000},
                                        {0x0F007084,0x00000000},
                                        //# Enable BW improvement within memory controller
                                        {0x0F007094,0x00000104},
                                        //# Enable 2 ports within X-bar
                                        {0x0F00A000,0x00000016},
                                        //# Enable start bit within memory controller
                                        {0x0F007018,0x01010000}
                                        };
//80Mhz
#define T3_SKIP_CLOCK_PROGRAM_DUMP_80MHZ 10  //index for 0x0F007000
static DDR_SET_NODE asT3_DDRSetting80MHz[]= {//   # DPLL Clock Setting
                                        {0x0f000810,0x00000F95},
                                        {0x0f000820,0x07f1ffff},
                                        {0x0f000860,0x00000000},
                                        {0x0f000880,0x000003DD},
                                        {0x0F00a044,0x1fffffff},
                                        {0x0F00a040,0x1f000000},
                                        {0x0F00a084,0x1Cffffff},
                                        {0x0F00a080,0x1C000000},
                                        {0x0F00a000,0x00000016},
                                        {0x0F00a04C,0x0000000C},
                                //Memcontroller Default values
                                        {0x0F007000,0x00010001},
                                        {0x0F007004,0x01000000},
                                        {0x0F007008,0x01000001},
                                        {0x0F00700c,0x00000000},
                                        {0x0F007010,0x01000000},
                                        {0x0F007014,0x01000100},
                                        {0x0F007018,0x01000000},
                                        {0x0F00701c,0x01020000},
                                        {0x0F007020,0x04020107},
                                        {0x0F007024,0x00000007},
                                        {0x0F007028,0x02020201},
                                        {0x0F00702c,0x0204040a},
                                        {0x0F007030,0x04000000},
                                        {0x0F007034,0x00000002},
                                        {0x0F007038,0x1F060200},
                                        {0x0F00703C,0x1C22221F},
                                        {0x0F007040,0x8A006600},
                                        {0x0F007044,0x221a0800},
                                        {0x0F007048,0x02690204},
                                        {0x0F00704c,0x00000000},
                                        {0x0F007050,0x0000001c},
                                        {0x0F007054,0x00000000},
                                        {0x0F007058,0x00000000},
                                        {0x0F00705c,0x00000000},
                                        {0x0F007060,0x000A15D6},
                                        {0x0F007064,0x0000000A},
                                        {0x0F007068,0x00000000},
                                        {0x0F00706c,0x00000001},
                                        {0x0F007070,0x00004000},
                                        {0x0F007074,0x00000000},
                                        {0x0F007078,0x00000000},
                                        {0x0F00707C,0x00000000},
                                        {0x0F007080,0x00000000},
                                        {0x0F007084,0x00000000},
                                        {0x0F007094,0x00000104},
                                        //# Enable start bit within memory controller
										{0x0F007018,0x01010000}
                                };
//100Mhz
#define T3_SKIP_CLOCK_PROGRAM_DUMP_100MHZ 13  //index for 0x0F007000
static DDR_SET_NODE asT3_DDRSetting100MHz[]= {//  # DPLL Clock Setting
                                        {0x0F000800,0x00007008},
                                        {0x0f000810,0x00000F95},
                                        {0x0f000820,0x07F13E3F},
                                        {0x0f000860,0x00000000},
                                        {0x0f000880,0x000003DD},
                                // Changed source for X-bar and MIPS clock to APLL
                                //0x0f000840,0x0FFF1800,
                                        {0x0f000840,0x0FFF1B00},
                                        {0x0f000870,0x00000002},
                                        {0x0F00a044,0x1fffffff},
                                        {0x0F00a040,0x1f000000},
                                        {0x0F00a084,0x1Cffffff},
                                        {0x0F00a080,0x1C000000},
                                        {0x0F00a04C,0x0000000C},
                                //# Enable 2 ports within X-bar
                                        {0x0F00A000,0x00000016},
                                //Memcontroller Default values
                                        {0x0F007000,0x00010001},
                                        {0x0F007004,0x01010100},
                                        {0x0F007008,0x01000001},
                                        {0x0F00700c,0x00000000},
                                        {0x0F007010,0x01000000},
                                        {0x0F007014,0x01000100},
                                        {0x0F007018,0x01000000},
                                        {0x0F00701c,0x01020001}, // POP - 0x00020000 Normal 0x01020000
                                        {0x0F007020,0x04020107},//Normal - 0x04030107 POP - 0x05030107
                                        {0x0F007024,0x00000007},
                                        {0x0F007028,0x01020201},
                                        {0x0F00702c,0x0204040A},
                                        {0x0F007030,0x06000000},
                                        {0x0F007034,0x00000004},
                                        {0x0F007038,0x20080200},
                                        {0x0F00703C,0x02030320},
                                        {0x0F007040,0x6E7F1200},
                                        {0x0F007044,0x01190A00},
                                        {0x0F007048,0x06120305},//0x02690204 // 0x06120305
                                        {0x0F00704c,0x00000000},
                                        {0x0F007050,0x0000001C},
                                        {0x0F007054,0x00000000},
                                        {0x0F007058,0x00000000},
                                        {0x0F00705c,0x00000000},
                                        {0x0F007060,0x00082ED6},
                                        {0x0F007064,0x0000000A},
                                        {0x0F007068,0x00000000},
                                        {0x0F00706c,0x00000001},
                                        {0x0F007070,0x00005000},
                                        {0x0F007074,0x00000000},
                                        {0x0F007078,0x00000000},
                                        {0x0F00707C,0x00000000},
                                        {0x0F007080,0x00000000},
                                        {0x0F007084,0x00000000},
                                //# Enable BW improvement within memory controller
                                        {0x0F007094,0x00000104},
                                //# Enable start bit within memory controller
                                        {0x0F007018,0x01010000}
                                };

//Net T3B DDR Settings
//DDR INIT-133Mhz
static DDR_SET_NODE asDPLL_266MHZ[] = {
                                        {0x0F000800,0x00007212},
                                        {0x0f000820,0x07F13FFF},
                                        {0x0f000810,0x00000F95},
                                        {0x0f000860,0x00000000},
                                        {0x0f000880,0x000003DD},
                                        // Changed source for X-bar and MIPS clock to APLL
                                        {0x0f000840,0x0FFF1B00},
                                        {0x0f000870,0x00000002}
									  };
#if 0
static DDR_SET_NODE asDPLL_800MHZ[] = {
										{0x0f000810,0x00000F95},
										{0x0f000810,0x00000F95},
                                        {0x0f000810,0x00000F95},
                                        {0x0f000820,0x03F1365B},
                                        {0x0f000840,0x0FFF0000},
                                        {0x0f000880,0x000003DD},
                                        {0x0f000860,0x00000000}
									  };
#endif

#define T3B_SKIP_CLOCK_PROGRAM_DUMP_133MHZ 11  //index for 0x0F007000
static DDR_SET_NODE asT3B_DDRSetting133MHz[] = {//      # DPLL Clock Setting
                                        {0x0f000810,0x00000F95},
                                        {0x0f000810,0x00000F95},
                                        {0x0f000810,0x00000F95},
                                        {0x0f000820,0x07F13652},
                                        {0x0f000840,0x0FFF0800},
                                        // Changed source for X-bar and MIPS clock to APLL
                                        {0x0f000880,0x000003DD},
                                        {0x0f000860,0x00000000},
                                        // Changed source for X-bar and MIPS clock to APLL
                                        {0x0F00a044,0x1fffffff},
                                        {0x0F00a040,0x1f000000},
                                        {0x0F00a084,0x1Cffffff},
                                        {0x0F00a080,0x1C000000},
                                        //# Enable 2 ports within X-bar
                                        {0x0F00A000,0x00000016},
                                        //Memcontroller Default values
                                        {0x0F007000,0x00010001},
                                        {0x0F007004,0x01010100},
                                        {0x0F007008,0x01000001},
                                        {0x0F00700c,0x00000000},
                                        {0x0F007010,0x01000000},
                                        {0x0F007014,0x01000100},
                                        {0x0F007018,0x01000000},
                                        {0x0F00701c,0x01020001},// POP - 0x00020001 Normal 0x01020001
                                        {0x0F007020,0x04030107}, //Normal - 0x04030107 POP - 0x05030107
                                        {0x0F007024,0x02000007},
                                        {0x0F007028,0x02020202},
                                        {0x0F00702c,0x0206060a},//ROB- 0x0205050a,//0x0206060a
                                        {0x0F007030,0x05000000},
                                        {0x0F007034,0x00000003},
                                        {0x0F007038,0x130a0200},//ROB - 0x110a0200,//0x180a0200,// 0x1f0a0200
                                        {0x0F00703C,0x02101012},//ROB - 0x02101010,//0x02101018},
                                        {0x0F007040,0x457D1200},//ROB - 0x45751200,//0x450f1200},
                                        {0x0F007044,0x11130d00},//ROB - 0x110a0d00//0x111f0d00
                                        {0x0F007048,0x040D0306},
                                        {0x0F00704c,0x00000000},
                                        {0x0F007050,0x0000001c},
                                        {0x0F007054,0x00000000},
                                        {0x0F007058,0x00000000},
                                        {0x0F00705c,0x00000000},
                                        {0x0F007060,0x0010246c},
                                        {0x0F007064,0x00000012},
                                        {0x0F007068,0x00000000},
                                        {0x0F00706c,0x00000001},
                                        {0x0F007070,0x00007000},
                                        {0x0F007074,0x00000000},
                                        {0x0F007078,0x00000000},
                                        {0x0F00707C,0x00000000},
                                        {0x0F007080,0x00000000},
                                        {0x0F007084,0x00000000},
                                        //# Enable BW improvement within memory controller
                                        {0x0F007094,0x00000104},
                                        //# Enable start bit within memory controller
                                        {0x0F007018,0x01010000},
                                        };

#define T3B_SKIP_CLOCK_PROGRAM_DUMP_80MHZ 9  //index for 0x0F007000
static DDR_SET_NODE asT3B_DDRSetting80MHz[] = {//       # DPLL Clock Setting
										{0x0f000810,0x00000F95},
										{0x0f000820,0x07F13FFF},
										{0x0f000840,0x0FFF1F00},
										{0x0f000880,0x000003DD},
										{0x0f000860,0x00000000},

										{0x0F00a044,0x1fffffff},
										{0x0F00a040,0x1f000000},
										{0x0F00a084,0x1Cffffff},
										{0x0F00a080,0x1C000000},
										{0x0F00a000,0x00000016},
										//Memcontroller Default values
										{0x0F007000,0x00010001},
										{0x0F007004,0x01000000},
										{0x0F007008,0x01000001},
										{0x0F00700c,0x00000000},
										{0x0F007010,0x01000000},
										{0x0F007014,0x01000100},
										{0x0F007018,0x01000000},
										{0x0F00701c,0x01020000},
										{0x0F007020,0x04020107},
										{0x0F007024,0x00000007},
										{0x0F007028,0x02020201},
										{0x0F00702c,0x0204040a},
										{0x0F007030,0x04000000},
										{0x0F007034,0x02000002},
										{0x0F007038,0x1F060202},
										{0x0F00703C,0x1C22221F},
										{0x0F007040,0x8A006600},
										{0x0F007044,0x221a0800},
										{0x0F007048,0x02690204},
										{0x0F00704c,0x00000000},
										{0x0F007050,0x0100001c},
										{0x0F007054,0x00000000},
										{0x0F007058,0x00000000},
										{0x0F00705c,0x00000000},
										{0x0F007060,0x000A15D6},
										{0x0F007064,0x0000000A},
										{0x0F007068,0x00000000},
										{0x0F00706c,0x00000001},
										{0x0F007070,0x00004000},
										{0x0F007074,0x00000000},
										{0x0F007078,0x00000000},
										{0x0F00707C,0x00000000},
										{0x0F007080,0x00000000},
										{0x0F007084,0x00000000},
										{0x0F007094,0x00000104},
										//# Enable start bit within memory controller
										{0x0F007018,0x01010000}
								};

//100Mhz
#define T3B_SKIP_CLOCK_PROGRAM_DUMP_100MHZ 9  //index for 0x0F007000
static DDR_SET_NODE asT3B_DDRSetting100MHz[] = {//      # DPLL Clock Setting
										{0x0f000810,0x00000F95},
										{0x0f000820,0x07F1369B},
										{0x0f000840,0x0FFF0800},
										{0x0f000880,0x000003DD},
										{0x0f000860,0x00000000},
										{0x0F00a044,0x1fffffff},
										{0x0F00a040,0x1f000000},
										{0x0F00a084,0x1Cffffff},
										{0x0F00a080,0x1C000000},
										//# Enable 2 ports within X-bar
										{0x0F00A000,0x00000016},
								//Memcontroller Default values
										{0x0F007000,0x00010001},
										{0x0F007004,0x01010100},
										{0x0F007008,0x01000001},
										{0x0F00700c,0x00000000},
										{0x0F007010,0x01000000},
										{0x0F007014,0x01000100},
										{0x0F007018,0x01000000},
										{0x0F00701c,0x01020000}, // POP - 0x00020000 Normal 0x01020000
										{0x0F007020,0x04020107},//Normal - 0x04030107 POP - 0x05030107
										{0x0F007024,0x00000007},
										{0x0F007028,0x01020201},
										{0x0F00702c,0x0204040A},
										{0x0F007030,0x06000000},
										{0x0F007034,0x02000004},
										{0x0F007038,0x20080200},
										{0x0F00703C,0x02030320},
										{0x0F007040,0x6E7F1200},
										{0x0F007044,0x01190A00},
										{0x0F007048,0x06120305},//0x02690204 // 0x06120305
										{0x0F00704c,0x00000000},
										{0x0F007050,0x0100001C},
										{0x0F007054,0x00000000},
										{0x0F007058,0x00000000},
										{0x0F00705c,0x00000000},
										{0x0F007060,0x00082ED6},
										{0x0F007064,0x0000000A},
										{0x0F007068,0x00000000},
										{0x0F00706c,0x00000001},
										{0x0F007070,0x00005000},
										{0x0F007074,0x00000000},
										{0x0F007078,0x00000000},
										{0x0F00707C,0x00000000},
										{0x0F007080,0x00000000},
										{0x0F007084,0x00000000},
								//# Enable BW improvement within memory controller
										{0x0F007094,0x00000104},
								//# Enable start bit within memory controller
										{0x0F007018,0x01010000}
							};


#define T3LP_SKIP_CLOCK_PROGRAM_DUMP_133MHZ 9  //index for 0x0F007000
static DDR_SET_NODE asT3LP_DDRSetting133MHz[]= {//	# DPLL Clock Setting
								{0x0f000820,0x03F1365B},
								{0x0f000810,0x00002F95},
								{0x0f000880,0x000003DD},
								// Changed source for X-bar and MIPS clock to APLL
								{0x0f000840,0x0FFF0000},
								{0x0f000860,0x00000000},
								{0x0F00a044,0x1fffffff},
								{0x0F00a040,0x1f000000},
								{0x0F00a084,0x1Cffffff},
								{0x0F00a080,0x1C000000},
								{0x0F00A000,0x00000016},
								//Memcontroller Default values
								{0x0F007000,0x00010001},
								{0x0F007004,0x01010100},
								{0x0F007008,0x01000001},
								{0x0F00700c,0x00000000},
								{0x0F007010,0x01000000},
								{0x0F007014,0x01000100},
								{0x0F007018,0x01000000},
								{0x0F00701c,0x01020001},// POP - 0x00020001 Normal 0x01020001
								{0x0F007020,0x04030107}, //Normal - 0x04030107 POP - 0x05030107
								{0x0F007024,0x02000007},
								{0x0F007028,0x02020200},
								{0x0F00702c,0x0206060a},//ROB- 0x0205050a,//0x0206060a
								{0x0F007030,0x05000000},
								{0x0F007034,0x00000003},
								{0x0F007038,0x200a0200},//ROB - 0x110a0200,//0x180a0200,// 0x1f0a0200
								{0x0F00703C,0x02101020},//ROB - 0x02101010,//0x02101018,
								{0x0F007040,0x45711200},//ROB - 0x45751200,//0x450f1200,
								{0x0F007044,0x110D0D00},//ROB - 0x110a0d00//0x111f0d00
								{0x0F007048,0x04080306},
								{0x0F00704c,0x00000000},
								{0x0F007050,0x0100001c},
								{0x0F007054,0x00000000},
								{0x0F007058,0x00000000},
								{0x0F00705c,0x00000000},
								{0x0F007060,0x0010245F},
								{0x0F007064,0x00000010},
								{0x0F007068,0x00000000},
								{0x0F00706c,0x00000001},
								{0x0F007070,0x00007000},
								{0x0F007074,0x00000000},
								{0x0F007078,0x00000000},
								{0x0F00707C,0x00000000},
								{0x0F007080,0x00000000},
								{0x0F007084,0x00000000},
								{0x0F007088,0x01000001},
								{0x0F00708c,0x00000101},
								{0x0F007090,0x00000000},
								//# Enable BW improvement within memory controller
								{0x0F007094,0x00040000},
								{0x0F007098,0x00000000},
								{0x0F0070c8,0x00000104},
								//# Enable 2 ports within X-bar
								//# Enable start bit within memory controller
								{0x0F007018,0x01010000}
};

#define T3LP_SKIP_CLOCK_PROGRAM_DUMP_100MHZ 11  //index for 0x0F007000
static DDR_SET_NODE asT3LP_DDRSetting100MHz[]= {//	# DPLL Clock Setting
								{0x0f000810,0x00002F95},
								{0x0f000820,0x03F1369B},
								{0x0f000840,0x0fff0000},
								{0x0f000860,0x00000000},
								{0x0f000880,0x000003DD},
								// Changed source for X-bar and MIPS clock to APLL
								{0x0f000840,0x0FFF0000},
								{0x0F00a044,0x1fffffff},
								{0x0F00a040,0x1f000000},
								{0x0F00a084,0x1Cffffff},
								{0x0F00a080,0x1C000000},
								//Memcontroller Default values
								{0x0F007000,0x00010001},
								{0x0F007004,0x01010100},
								{0x0F007008,0x01000001},
								{0x0F00700c,0x00000000},
								{0x0F007010,0x01000000},
								{0x0F007014,0x01000100},
								{0x0F007018,0x01000000},
								{0x0F00701c,0x01020000},// POP - 0x00020001 Normal 0x01020001
								{0x0F007020,0x04020107}, //Normal - 0x04030107 POP - 0x05030107
								{0x0F007024,0x00000007},
								{0x0F007028,0x01020200},
								{0x0F00702c,0x0204040a},//ROB- 0x0205050a,//0x0206060a
								{0x0F007030,0x06000000},
								{0x0F007034,0x00000004},
								{0x0F007038,0x1F080200},//ROB - 0x110a0200,//0x180a0200,// 0x1f0a0200
								{0x0F00703C,0x0203031F},//ROB - 0x02101010,//0x02101018,
								{0x0F007040,0x6e001200},//ROB - 0x45751200,//0x450f1200,
								{0x0F007044,0x011a0a00},//ROB - 0x110a0d00//0x111f0d00
								{0x0F007048,0x03000305},
								{0x0F00704c,0x00000000},
								{0x0F007050,0x0100001c},
								{0x0F007054,0x00000000},
								{0x0F007058,0x00000000},
								{0x0F00705c,0x00000000},
								{0x0F007060,0x00082ED6},
								{0x0F007064,0x0000000A},
								{0x0F007068,0x00000000},
								{0x0F00706c,0x00000001},
								{0x0F007070,0x00005000},
								{0x0F007074,0x00000000},
								{0x0F007078,0x00000000},
								{0x0F00707C,0x00000000},
								{0x0F007080,0x00000000},
								{0x0F007084,0x00000000},
								{0x0F007088,0x01000001},
								{0x0F00708c,0x00000101},
								{0x0F007090,0x00000000},
								{0x0F007094,0x00010000},
								{0x0F007098,0x00000000},
								{0x0F0070C8,0x00000104},
								//# Enable 2 ports within X-bar
								{0x0F00A000,0x00000016},
								//# Enable start bit within memory controller
								{0x0F007018,0x01010000}
};

#define T3LP_SKIP_CLOCK_PROGRAM_DUMP_80MHZ 9  //index for 0x0F007000
static DDR_SET_NODE asT3LP_DDRSetting80MHz[]= {//	# DPLL Clock Setting
								{0x0f000820,0x07F13FFF},
								{0x0f000810,0x00002F95},
								{0x0f000860,0x00000000},
								{0x0f000880,0x000003DD},
								{0x0f000840,0x0FFF1F00},
								{0x0F00a044,0x1fffffff},
								{0x0F00a040,0x1f000000},
								{0x0F00a084,0x1Cffffff},
								{0x0F00a080,0x1C000000},
								{0x0F00A000,0x00000016},
								{0x0f007000,0x00010001},
								{0x0f007004,0x01000000},
								{0x0f007008,0x01000001},
								{0x0f00700c,0x00000000},
								{0x0f007010,0x01000000},
								{0x0f007014,0x01000100},
								{0x0f007018,0x01000000},
								{0x0f00701c,0x01020000},
								{0x0f007020,0x04020107},
								{0x0f007024,0x00000007},
								{0x0f007028,0x02020200},
								{0x0f00702c,0x0204040a},
								{0x0f007030,0x04000000},
								{0x0f007034,0x00000002},
								{0x0f007038,0x1d060200},
								{0x0f00703c,0x1c22221d},
								{0x0f007040,0x8A116600},
								{0x0f007044,0x222d0800},
								{0x0f007048,0x02690204},
								{0x0f00704c,0x00000000},
								{0x0f007050,0x0100001c},
								{0x0f007054,0x00000000},
								{0x0f007058,0x00000000},
								{0x0f00705c,0x00000000},
								{0x0f007060,0x000A15D6},
								{0x0f007064,0x0000000A},
								{0x0f007068,0x00000000},
								{0x0f00706c,0x00000001},
								{0x0f007070,0x00004000},
								{0x0f007074,0x00000000},
								{0x0f007078,0x00000000},
								{0x0f00707c,0x00000000},
								{0x0f007080,0x00000000},
								{0x0f007084,0x00000000},
								{0x0f007088,0x01000001},
								{0x0f00708c,0x00000101},
								{0x0f007090,0x00000000},
								{0x0f007094,0x00010000},
								{0x0f007098,0x00000000},
								{0x0F0070C8,0x00000104},
								{0x0F007018,0x01010000}
};




///T3 LP-B (UMA-B)

#define T3LPB_SKIP_CLOCK_PROGRAM_DUMP_160MHZ 7  //index for 0x0F007000
static DDR_SET_NODE asT3LPB_DDRSetting160MHz[]= {//	# DPLL Clock Setting

								{0x0f000820,0x03F137DB},
								{0x0f000810,0x01842795},
								{0x0f000860,0x00000000},
								{0x0f000880,0x000003DD},
								{0x0f000840,0x0FFF0400},
								{0x0F00a044,0x1fffffff},
								{0x0F00a040,0x1f000000},
								{0x0f003050,0x00000021},//this is flash/eeprom clock divisor which set the flash clock to 20 MHz
								{0x0F00a084,0x1Cffffff},//Now dump from her in internal memory
								{0x0F00a080,0x1C000000},
								{0x0F00A000,0x00000016},
								{0x0f007000,0x00010001},
								{0x0f007004,0x01000001},
								{0x0f007008,0x01000101},
								{0x0f00700c,0x00000000},
								{0x0f007010,0x01000100},
								{0x0f007014,0x01000100},
								{0x0f007018,0x01000000},
								{0x0f00701c,0x01020000},
								{0x0f007020,0x04030107},
								{0x0f007024,0x02000007},
								{0x0f007028,0x02020200},
								{0x0f00702c,0x0206060a},
								{0x0f007030,0x050d0d00},
								{0x0f007034,0x00000003},
								{0x0f007038,0x170a0200},
								{0x0f00703c,0x02101012},
								{0x0f007040,0x45161200},
								{0x0f007044,0x11250c00},
								{0x0f007048,0x04da0307},
								{0x0f00704c,0x00000000},
								{0x0f007050,0x0000001c},
								{0x0f007054,0x00000000},
								{0x0f007058,0x00000000},
								{0x0f00705c,0x00000000},
								{0x0f007060,0x00142bb6},
								{0x0f007064,0x20430014},
								{0x0f007068,0x00000000},
								{0x0f00706c,0x00000001},
								{0x0f007070,0x00009000},
								{0x0f007074,0x00000000},
								{0x0f007078,0x00000000},
								{0x0f00707c,0x00000000},
								{0x0f007080,0x00000000},
								{0x0f007084,0x00000000},
								{0x0f007088,0x01000001},
								{0x0f00708c,0x00000101},
								{0x0f007090,0x00000000},
								{0x0f007094,0x00040000},
								{0x0f007098,0x00000000},
								{0x0F0070C8,0x00000104},
								{0x0F007018,0x01010000}
};


#define T3LPB_SKIP_CLOCK_PROGRAM_DUMP_133MHZ 7  //index for 0x0F007000
static DDR_SET_NODE asT3LPB_DDRSetting133MHz[]= {//	# DPLL Clock Setting
								{0x0f000820,0x03F1365B},
								{0x0f000810,0x00002F95},
								{0x0f000880,0x000003DD},
								// Changed source for X-bar and MIPS clock to APLL
								{0x0f000840,0x0FFF0000},
								{0x0f000860,0x00000000},
								{0x0F00a044,0x1fffffff},
								{0x0F00a040,0x1f000000},
								{0x0f003050,0x00000021},//flash/eeprom clock divisor which set the flash clock to 20 MHz
								{0x0F00a084,0x1Cffffff},//dump from here in internal memory
								{0x0F00a080,0x1C000000},
								{0x0F00A000,0x00000016},
								//Memcontroller Default values
								{0x0F007000,0x00010001},
								{0x0F007004,0x01010100},
								{0x0F007008,0x01000001},
								{0x0F00700c,0x00000000},
								{0x0F007010,0x01000000},
								{0x0F007014,0x01000100},
								{0x0F007018,0x01000000},
								{0x0F00701c,0x01020001},// POP - 0x00020001 Normal 0x01020001
								{0x0F007020,0x04030107}, //Normal - 0x04030107 POP - 0x05030107
								{0x0F007024,0x02000007},
								{0x0F007028,0x02020200},
								{0x0F00702c,0x0206060a},//ROB- 0x0205050a,//0x0206060a
								{0x0F007030,0x05000000},
								{0x0F007034,0x00000003},
								{0x0F007038,0x190a0200},//ROB - 0x110a0200,//0x180a0200,// 0x1f0a0200
								{0x0F00703C,0x02101017},//ROB - 0x02101010,//0x02101018,
								{0x0F007040,0x45171200},//ROB - 0x45751200,//0x450f1200,
								{0x0F007044,0x11290D00},//ROB - 0x110a0d00//0x111f0d00
								{0x0F007048,0x04080306},
								{0x0F00704c,0x00000000},
								{0x0F007050,0x0100001c},
								{0x0F007054,0x00000000},
								{0x0F007058,0x00000000},
								{0x0F00705c,0x00000000},
								{0x0F007060,0x0010245F},
								{0x0F007064,0x00000010},
								{0x0F007068,0x00000000},
								{0x0F00706c,0x00000001},
								{0x0F007070,0x00007000},
								{0x0F007074,0x00000000},
								{0x0F007078,0x00000000},
								{0x0F00707C,0x00000000},
								{0x0F007080,0x00000000},
								{0x0F007084,0x00000000},
								{0x0F007088,0x01000001},
								{0x0F00708c,0x00000101},
								{0x0F007090,0x00000000},
								//# Enable BW improvement within memory controller
								{0x0F007094,0x00040000},
								{0x0F007098,0x00000000},
								{0x0F0070c8,0x00000104},
								//# Enable 2 ports within X-bar
								//# Enable start bit within memory controller
								{0x0F007018,0x01010000}
};

#define T3LPB_SKIP_CLOCK_PROGRAM_DUMP_100MHZ 8  //index for 0x0F007000
static DDR_SET_NODE asT3LPB_DDRSetting100MHz[]= {//	# DPLL Clock Setting
								{0x0f000810,0x00002F95},
								{0x0f000820,0x03F1369B},
								{0x0f000840,0x0fff0000},
								{0x0f000860,0x00000000},
								{0x0f000880,0x000003DD},
								// Changed source for X-bar and MIPS clock to APLL
								{0x0f000840,0x0FFF0000},
								{0x0F00a044,0x1fffffff},
								{0x0F00a040,0x1f000000},
								{0x0f003050,0x00000021},//flash/eeprom clock divisor which set the flash clock to 20 MHz
								{0x0F00a084,0x1Cffffff}, //dump from here in internal memory
								{0x0F00a080,0x1C000000},
								//Memcontroller Default values
								{0x0F007000,0x00010001},
								{0x0F007004,0x01010100},
								{0x0F007008,0x01000001},
								{0x0F00700c,0x00000000},
								{0x0F007010,0x01000000},
								{0x0F007014,0x01000100},
								{0x0F007018,0x01000000},
								{0x0F00701c,0x01020000},// POP - 0x00020001 Normal 0x01020001
								{0x0F007020,0x04020107}, //Normal - 0x04030107 POP - 0x05030107
								{0x0F007024,0x00000007},
								{0x0F007028,0x01020200},
								{0x0F00702c,0x0204040a},//ROB- 0x0205050a,//0x0206060a
								{0x0F007030,0x06000000},
								{0x0F007034,0x00000004},
								{0x0F007038,0x1F080200},//ROB - 0x110a0200,//0x180a0200,// 0x1f0a0200
								{0x0F00703C,0x0203031F},//ROB - 0x02101010,//0x02101018,
								{0x0F007040,0x6e001200},//ROB - 0x45751200,//0x450f1200,
								{0x0F007044,0x011a0a00},//ROB - 0x110a0d00//0x111f0d00
								{0x0F007048,0x03000305},
								{0x0F00704c,0x00000000},
								{0x0F007050,0x0100001c},
								{0x0F007054,0x00000000},
								{0x0F007058,0x00000000},
								{0x0F00705c,0x00000000},
								{0x0F007060,0x00082ED6},
								{0x0F007064,0x0000000A},
								{0x0F007068,0x00000000},
								{0x0F00706c,0x00000001},
								{0x0F007070,0x00005000},
								{0x0F007074,0x00000000},
								{0x0F007078,0x00000000},
								{0x0F00707C,0x00000000},
								{0x0F007080,0x00000000},
								{0x0F007084,0x00000000},
								{0x0F007088,0x01000001},
								{0x0F00708c,0x00000101},
								{0x0F007090,0x00000000},
								{0x0F007094,0x00010000},
								{0x0F007098,0x00000000},
								{0x0F0070C8,0x00000104},
								//# Enable 2 ports within X-bar
								{0x0F00A000,0x00000016},
								//# Enable start bit within memory controller
								{0x0F007018,0x01010000}
};

#define T3LPB_SKIP_CLOCK_PROGRAM_DUMP_80MHZ 7  //index for 0x0F007000
static DDR_SET_NODE asT3LPB_DDRSetting80MHz[]= {//	# DPLL Clock Setting
								{0x0f000820,0x07F13FFF},
								{0x0f000810,0x00002F95},
								{0x0f000860,0x00000000},
								{0x0f000880,0x000003DD},
								{0x0f000840,0x0FFF1F00},
								{0x0F00a044,0x1fffffff},
								{0x0F00a040,0x1f000000},
								{0x0f003050,0x00000021},//flash/eeprom clock divisor which set the flash clock to 20 MHz
								{0x0F00a084,0x1Cffffff},// dump from here in internal memory
								{0x0F00a080,0x1C000000},
								{0x0F00A000,0x00000016},
								{0x0f007000,0x00010001},
								{0x0f007004,0x01000000},
								{0x0f007008,0x01000001},
								{0x0f00700c,0x00000000},
								{0x0f007010,0x01000000},
								{0x0f007014,0x01000100},
								{0x0f007018,0x01000000},
								{0x0f00701c,0x01020000},
								{0x0f007020,0x04020107},
								{0x0f007024,0x00000007},
								{0x0f007028,0x02020200},
								{0x0f00702c,0x0204040a},
								{0x0f007030,0x04000000},
								{0x0f007034,0x00000002},
								{0x0f007038,0x1d060200},
								{0x0f00703c,0x1c22221d},
								{0x0f007040,0x8A116600},
								{0x0f007044,0x222d0800},
								{0x0f007048,0x02690204},
								{0x0f00704c,0x00000000},
								{0x0f007050,0x0100001c},
								{0x0f007054,0x00000000},
								{0x0f007058,0x00000000},
								{0x0f00705c,0x00000000},
								{0x0f007060,0x000A15D6},
								{0x0f007064,0x0000000A},
								{0x0f007068,0x00000000},
								{0x0f00706c,0x00000001},
								{0x0f007070,0x00004000},
								{0x0f007074,0x00000000},
								{0x0f007078,0x00000000},
								{0x0f00707c,0x00000000},
								{0x0f007080,0x00000000},
								{0x0f007084,0x00000000},
								{0x0f007088,0x01000001},
								{0x0f00708c,0x00000101},
								{0x0f007090,0x00000000},
								{0x0f007094,0x00010000},
								{0x0f007098,0x00000000},
								{0x0F0070C8,0x00000104},
								{0x0F007018,0x01010000}
};

#endif // _USB_BECEEM_DDR_H_

