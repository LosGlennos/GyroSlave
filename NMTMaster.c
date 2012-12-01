#include	"CO_MAIN.H"
#include        "NMTMaster.h"

void NMTMaster_Transmit(unsigned char nodeid, unsigned char state)
{
    mCANIsPutReady(0);
    *(unsigned long *)(mCANGetPtrTxCOB()) = 0; // Sets both RXB0SIDH and RXB0SIDL to 0.
    mCANPutDataLen(2); // NMT-messages are 2 bytes long(state+nodeid).
    mCANPutDataByte0(state); 
    mCANPutDataByte1(nodeid);
    mCANSendMessage();
}
