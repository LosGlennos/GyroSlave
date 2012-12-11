// CONFIG1H
#pragma config OSC = HS      // Oscillator Selection bits (HS oscillator, PLL enabled (Clock Frequency = 4 x FOSC1))
#pragma config FCMEN = OFF      // Fail-Safe Clock Monitor Enable bit (Fail-Safe Clock Monitor disabled)
#pragma config IESO = OFF       // Internal/External Oscillator Switchover bit (Oscillator Switchover mode disabled)

// CONFIG2L
#pragma config PWRT = OFF       // Power-up Timer Enable bit (PWRT disabled)
#pragma config BOREN = BOHW     // Brown-out Reset Enable bits (Brown-out Reset enabled in hardware only (SBOREN is disabled))
#pragma config BORV = 3         // Brown-out Reset Voltage bits (VBOR set to 2.1V)

// CONFIG2H
#pragma config WDT = OFF         // Watchdog Timer Enable bit (WDT enabled)
#pragma config WDTPS = 32768    // Watchdog Timer Postscale Select bits (1:32768)

// CONFIG3H
#pragma config PBADEN = ON      // PORTB A/D Enable bit (PORTB<4:0> pins are configured as analog input channels on Reset)
#pragma config LPT1OSC = OFF    // Low-Power Timer 1 Oscillator Enable bit (Timer1 configured for higher power operation)
#pragma config MCLRE = ON       // MCLR Pin Enable bit (MCLR pin enabled; RE3 input pin disabled)

// CONFIG4L
#pragma config STVREN = ON      // Stack Full/Underflow Reset Enable bit (Stack full/underflow will cause Reset)
#pragma config LVP = OFF         // Single-Supply ICSP Enable bit (Single-Supply ICSP enabled)
#pragma config BBSIZ = 1024     // Boot Block Size Select bits (1K words (2K bytes) Boot Block)
#pragma config XINST = OFF      // Extended Instruction Set Enable bit (Instruction set extension and Indexed Addressing mode disabled (Legacy mode))

// CONFIG5L
#pragma config CP0 = OFF        // Code Protection bit (Block 0 (000800-003FFFh) not code-protected)
#pragma config CP1 = OFF        // Code Protection bit (Block 1 (004000-007FFFh) not code-protected)
#pragma config CP2 = OFF        // Code Protection bit (Block 2 (008000-00BFFFh) not code-protected)
#pragma config CP3 = OFF        // Code Protection bit (Block 3 (00C000-00FFFFh) not code-protected)

// CONFIG5H
#pragma config CPB = OFF        // Boot Block Code Protection bit (Boot block (000000-0007FFh) not code-protected)
#pragma config CPD = OFF        // Data EEPROM Code Protection bit (Data EEPROM not code-protected)

// CONFIG6L
#pragma config WRT0 = OFF       // Write Protection bit (Block 0 (000800-003FFFh) not write-protected)
#pragma config WRT1 = OFF       // Write Protection bit (Block 1 (004000-007FFFh) not write-protected)
#pragma config WRT2 = OFF       // Write Protection bit (Block 2 (008000-00BFFFh) not write-protected)
#pragma config WRT3 = OFF       // Write Protection bit (Block 3 (00C000-00FFFFh) not write-protected)

// CONFIG6H
#pragma config WRTC = OFF       // Configuration Register Write Protection bit (Configuration registers (300000-3000FFh) not write-protected)
#pragma config WRTB = OFF       // Boot Block Write Protection bit (Boot block (000000-0007FFh) not write-protected)
#pragma config WRTD = OFF       // Data EEPROM Write Protection bit (Data EEPROM not write-protected)

// CONFIG7L
#pragma config EBTR0 = OFF      // Table Read Protection bit (Block 0 (000800-003FFFh) not protected from table reads executed in other blocks)
#pragma config EBTR1 = OFF      // Table Read Protection bit (Block 1 (004000-007FFFh) not protected from table reads executed in other blocks)
#pragma config EBTR2 = OFF      // Table Read Protection bit (Block 2 (008000-00BFFFh) not protected from table reads executed in other blocks)
#pragma config EBTR3 = OFF      // Table Read Protection bit (Block 3 (00C000-00FFFFh) not protected from table reads executed in other blocks)

// CONFIG7H
#pragma config EBTRB = OFF      // Boot Block Table Read Protection bit (Boot block (000000-0007FFh) not protected from table reads executed in other blocks)





#include	"CO_MAIN.H"
#include	"Timer.H"
#include        "CO_COMM.h"
#include        "Compilation.h"
#include        "NMTMaster.h"
#include        "NMTextension.h"
#include        "IOutils.h"

#ifdef MASTER
#include	"Master.h"
#endif
#if defined SLAVE1 || defined SLAVE2 || defined SLAVE3
#include	"Slave.h"
#include "Gyro.h"
#endif

unsigned char test3[0x20];
unsigned long msgID;
unsigned char hMsg;




void main(void)
{	

	TimerInit();			// Init my timer

	mSYNC_SetCOBID(0x1000);		// Set the SYNC COB ID (MCHP format)

	mCO_SetNodeID(0x04);		// Set the Node ID

        // 0x04 => 250kbps
        // 0x00 => 1Mbps
	mCO_SetBaud(0x05);		// Set the baudrate

	mNMTE_SetHeartBeat(0x0004);	// Set the initial heartbeat
	mNMTE_SetGuardTime(0x0000);	// Set the initial guard time
	mNMTE_SetLifeFactor(0x00);	// Set the initial life time

        IO_Init();
        InitializeGyro();
        Slave_Init();
        
	mCO_InitAll();	// Initialize CANopen to run, bootup will be sent

        if( !COMM_STATE_PREOP )
            while( 1 );

        STOPLED  = COMM_STATE_STOP;
        PREOPLED = COMM_STATE_PREOP;
        OPLED    = COMM_STATE_OPER;

        // Enable TPDO:s and RPDO:s
        mTPDOOpen(1);
        mTPDOOpen(2);

	while(1)
	{
                
		// Process CANopen events
		mCO_ProcessAllEvents();

                // Process application specific functions

                Slave_ProcessEvents();
		
		// 1ms timer events
		if (TimerIsOverflowEvent())
		{
			// Process timer related events
			mCO_ProcessAllTimeEvents();	
		}

	}
}




