#include <p18f4680.h>
#include <spi.h>
#include <delays.h>
#include "Gyro.h"
#include "Globals.h"


#define SPI_CS LATAbits.LATA0

void delayms(int count)   //Gives a delay of 1 Ms
{
  int i;
  int j;
  for(i=0;i<count;i++)
  {
    for(j=0;j<25;j++);
  }
}

void DataTx( char data )
{
    SPI_CS = 0;
    WriteSPI( data );
    SPI_CS = 1;
}

char DataRx()
{
    return SSPBUF;
}

void InitializeGyro()
{
    TRISAbits.RA0 = 0;
    SPI_CS = 1;
    TRISCbits.RC3 = 0;
    TRISCbits.RC4 = 1;
    TRISCbits.RC5 = 0;
    OpenSPI( SPI_FOSC_4, MODE_00, SMPEND );

    //Send HighByte command
    SPI_CS = 0;
    WriteSPI( 0x80 );
    delayms( 100000 );
    SPI_CS = 1;
    delayms( 100000 );
}

void ReadAngularRate(short *angularrate)
{
    signed char highbyte = 0x00;
    unsigned char lowbyte = 0x00;
    unsigned char temp = 0x00;
    char neg = 0x00;
    char test = 0x00;

    *angularrate = 0;

    //Send LowByte command
    DataTx( 0x8F );

    //Receive HighByte
    highbyte = DataRx();

    //Send HighByte command
    DataTx( 0x80 );

    //Receive LowByte
    lowbyte = DataRx();

    temp = lowbyte & 0x03;

    test = highbyte & 0x80;

    if(test)
        neg = TRUE;
    else
        neg = FALSE;

    highbyte = highbyte & 0x7F;
    *angularrate = ( (short)highbyte << 2 ) | temp;

    if( neg )
        *angularrate = -512 + *angularrate;
}

void SumAngular(int *angularrate, int *angularSum)
{
        *angularSum = (int)(*angularSum + *angularrate);
}

void ResetAngularSum(int *angularSum)
{
        *angularSum = 0;
}
