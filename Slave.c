#include	"CO_MAIN.H"
#include        "Slave.h"
#include        "IOutils.h"
#include        "CO_PDO.h"
#include        "CO_CANDRV.H"
#include        "Gyro.h"

#define	RTR_DIS	bytes.B1.bits.b2
#define STD_DIS	bytes.B1.bits.b3
#define PDO_DIS	bytes.B1.bits.b4


// These are mapping constants for TPDO1
// starting at 0x1A00 in the dictionary
rom unsigned long uTPDO1Map = 0x60000108;
rom unsigned long uRPDO1Map = 0x62000108;
rom unsigned long uPDO1Dummy = 0x00000008;



unsigned char uIOinFilter;					// 0x6003 filter
unsigned char uIOinPolarity;				// 0x6002 polarity
unsigned char uIOinIntChange;				// 0x6006 interrupt on change
unsigned char uIOinIntRise;					// 0x6007 interrupt on positive edge
unsigned char uIOinIntFall;					// 0x6008 interrupt on negative edge
unsigned char uIOinIntEnable;				// 0x6005 enable interrupts

unsigned char uIOinDigiInOld;				// 


// Static data refered to by the dictionary
rom unsigned char rMaxIndex1 = 1;
rom unsigned char rMaxIndex2 = 8;
rom unsigned char uDemoTPDO1Len = 2;


unsigned char uLocalXmtBuffer[8];			// Local buffer for TPDO1
unsigned char uLocalRcvBuffer[8];			// local buffer fot RPDO1

UNSIGNED8 uDemoState; 					// Bits used to control various states
unsigned char uDemoSyncCount;			// Counter for synchronous types
unsigned char uDemoSyncSet;				// Internal TPDO type control

//Variables for angular
int angularSum = 0x0000;
short angularrate = 0x0000;

//Variables for different modes
unsigned char sumMode = 0x00;
unsigned char angularRateMode = 0x01;

//Variables for different states
unsigned char StartManeuver = 0x00;
unsigned char State = 0x00;

static int timerCounter = 0;

enum ApplicationState APPLICATION_STATE = PREMANEUVER;

void Slave_Init(void)
{
        // Convert to MCHP
	mTOOLS_CO2MCHP(mCOMM_GetNodeID().byte + 0xC0000284L);
	// Store the COB
	mTPDOSetCOB(1, mTOOLS_GetCOBID());

	// Convert to MCHP
	mTOOLS_CO2MCHP(mCOMM_GetNodeID().byte + 0xC0000184L);
	// Store the COB
	mTPDOSetCOB(2, mTOOLS_GetCOBID());

        // Set the pointer to the buffers
	mTPDOSetTxPtr(1, (unsigned char *)(&angularSum));
	mTPDOSetTxPtr(2, (unsigned char *)(&angularrate));
        //mRPDOSetRxPtr(3, (unsigned char *)(&uLocalXmtBuffer[0]));

        // RPDO1
	mTOOLS_CO2MCHP(0xC0000182L);
	mRPDOSetCOB(1, mTOOLS_GetCOBID());
        mRPDOSetRxPtr(1, (unsigned char *)(&StartManeuver));

        // RPDO2
	mTOOLS_CO2MCHP(0xC0000181L);
	mRPDOSetCOB(2, mTOOLS_GetCOBID());
        mRPDOSetRxPtr(2, (unsigned char *)(&State));

	// Set the length
	mTPDOSetLen(1, 4);
        mTPDOSetLen(2, 2);
}

void Slave_ProcessEvents(void)
{
    ReadAngularRate(&angularrate);
    if(APPLICATION_STATE == MANEUVER)
        SumAngular(&angularrate, &angularSum);

    if(mRPDOIsGetRdy(1))
    {
        mAppGoToMANEUVERstate();
        mRPDORead(1);
    }
    if(mRPDOIsGetRdy(2))
    {
        mAppGoToPOSTMANEUVERstate();
        sumMode = 0x01;
        mRPDORead(2);
    }
    
    if(sumMode)
    {
        if(mTPDOIsPutRdy(1))
        {
            sumMode = 0x00;
            mTPDOWritten(1);
        }
    }
}

/*********************************************************************
 * Function:        void CO_COMMSyncEvent(void)
 *
 * PreCondition:    none
 *
 * Input:       	none
 *
 * Output:         	none
 *
 * Side Effects:    none
 *
 * Overview:        This is a simple demonstration of a SYNC event
 *					handling function.
 *
 * Note:
 ********************************************************************/
void CO_COMMSyncEvent(void)
{
	// Process only if in a synchronous mode
	if ((uDemoSyncSet == 0) && (uDemoState.bits.b2))
	{
		// Reset the synchronous transmit and transfer to async
		uDemoState.bits.b2 = 0;
		uDemoState.bits.b0 = 1;
	}
	else
	if ((uDemoSyncSet >= 1) && (uDemoSyncSet <= 240))
	{
		// Adjust the sync counter
		uDemoSyncCount--;

		// If time to generate sync
		if (uDemoSyncCount == 0)
		{
			// Reset the sync counter
			uDemoSyncCount = uDemoSyncSet;

			// Start the PDO transmission
			uDemoState.bits.b0 = 1;
		}
	}
}





/*********************************************************************
 * Function:        void CO_COMM_RPDO1_COBIDAccessEvent(void)
 *
 * PreCondition:    none
 *
 * Input:       	none
 *
 * Output:         	none
 *
 * Side Effects:    none
 *
 * Overview:        This is a simple demonstration of a RPDO COB access
 *					handling function.
 *
 * Note:          	This function is called from the dictionary.
 ********************************************************************/
void CO_COMM_RPDO1_COBIDAccessEvent(void)
{
	switch (mCO_DictGetCmd())
	{
		case DICT_OBJ_READ: 	// Read the object
			// Translate MCHP COB to CANopen COB
			mTOOLS_MCHP2CO(mRPDOGetCOB(1));

			// Return the COBID
			*(unsigned long *)(uDict.obj->pReqBuf) = mTOOLS_GetCOBID();
			break;

		case DICT_OBJ_WRITE: 	// Write the object
			// Translate the COB to MCHP format
			mTOOLS_CO2MCHP(*(unsigned long *)(uDict.obj->pReqBuf));

			// If the request is to stop the PDO
			if ((*(UNSIGNED32 *)(&mTOOLS_GetCOBID())).PDO_DIS)
			{
				// And if the COB received matches the stored COB and type then close
				if (!((mTOOLS_GetCOBID() ^ mRPDOGetCOB(1)) & 0xFFFFEFFFL))
				{
					// but only close if the PDO endpoint was open
					if (mRPDOIsOpen(1)) {mRPDOClose(1);}

					// Indicate to the local object that this PDO is disabled
					(*(UNSIGNED32 *)(&mRPDOGetCOB(1))).PDO_DIS = 1;
				}
				else {mCO_DictSetRet(E_PARAM_RANGE);} //error
			}

			// Else if the TPDO is not open then start the TPDO
			else
			{
				// And if the COB received matches the stored COB and type then open
				if (!((mTOOLS_GetCOBID() ^ mRPDOGetCOB(1)) & 0xFFFFEFFFL))
				{
					// but only open if the PDO endpoint was closed
					if (!mRPDOIsOpen(1)) {mRPDOOpen(1);}

					// Indicate to the local object that this PDO is enabled
					(*(UNSIGNED32 *)(&mRPDOGetCOB(1))).PDO_DIS = 0;
				}
				else {mCO_DictSetRet(E_PARAM_RANGE);} //error
			}
			break;
	}
}



/*********************************************************************
 * Function:        void CO_COMM_TPDO1_COBIDAccessEvent(void)
 *
 * PreCondition:    none
 *
 * Input:       	none
 *
 * Output:         	none
 *
 * Side Effects:    none
 *
 * Overview:        This is a simple demonstration of a TPDO COB access
 *					handling function.
 *
 * Note:          	This function is called from the dictionary.
 ********************************************************************/
void CO_COMM_TPDO1_COBIDAccessEvent(void)
{
	switch (mCO_DictGetCmd())
	{
		case DICT_OBJ_READ: 	// Read the object
			// Translate MCHP COB to CANopen COB
			mTOOLS_MCHP2CO(mTPDOGetCOB(1));

			// Return the COBID
			*(unsigned long *)(uDict.obj->pReqBuf) = mTOOLS_GetCOBID();
			break;

		case DICT_OBJ_WRITE: 	// Write the object
			// Translate the COB to MCHP format
			mTOOLS_CO2MCHP(*(unsigned long *)(uDict.obj->pReqBuf));

			// If the request is to stop the PDO
			if ((*(UNSIGNED32 *)(&mTOOLS_GetCOBID())).PDO_DIS)
			{
				// And if the COB received matches the stored COB and type then close
				if (!((mTOOLS_GetCOBID() ^ mTPDOGetCOB(1)) & 0xFFFFEFFFL))
				{
					// but only close if the PDO endpoint was open
					if (mTPDOIsOpen(1)) {mTPDOClose(1);}

					// Indicate to the local object that this PDO is disabled
					(*(UNSIGNED32 *)(&mTPDOGetCOB(1))).PDO_DIS = 1;
				}
				else {mCO_DictSetRet(E_PARAM_RANGE);} //error
			}

			// Else if the TPDO is not open then start the TPDO
			else
			{
				// And if the COB received matches the stored COB and type then open
				if (!((mTOOLS_GetCOBID() ^ mTPDOGetCOB(1)) & 0xFFFFEFFFL))
				{
					// but only open if the PDO endpoint was closed
					if (!mTPDOIsOpen(1)) {mTPDOOpen(1);}

					// Indicate to the local object that this PDO is enabled
					(*(UNSIGNED32 *)(&mTPDOGetCOB(1))).PDO_DIS = 0;
				}
				else {mCO_DictSetRet(E_PARAM_RANGE);} //error
			}
			break;
	}
}



/*********************************************************************
 * Function:        void CO_COMM_TPDO1_TypeAccessEvent(void)
 *
 * PreCondition:    none
 *
 * Input:       	none
 *
 * Output:         	none
 *
 * Side Effects:    none
 *
 * Overview:        This is a simple demonstration of a TPDO type access
 *					handling function.
 *
 * Note:          	This function is called from the dictionary.
 ********************************************************************/
void CO_COMM_TPDO1_TypeAccessEvent(void)
{
	unsigned char tempType;

	switch (mCO_DictGetCmd())
	{
		//case DICT_OBJ_INFO:		// Get information about the object
			// The application should use this to load the
			// structure with legth, access, and mapping.
		//	break;

		case DICT_OBJ_READ: 	// Read the object
			// Write the Type to the buffer
			*(uDict.obj->pReqBuf) = uDemoSyncSet;
			break;

		case DICT_OBJ_WRITE: 	// Write the object
			tempType = *(uDict.obj->pReqBuf);
			if ((tempType >= 0) && (tempType <= 240))
			{
				// Set the new type and resync
				uDemoSyncCount = uDemoSyncSet = tempType;
			}
			else
			if ((tempType == 254) || (tempType == 255))
			{
				uDemoSyncSet = tempType;
			}
			else {mCO_DictSetRet(E_PARAM_RANGE);} //error

			break;
	}
}








/*********************************************************************
 * Function:        void CO_PDO1LSTimerEvent(void)
 *
 * PreCondition:    none
 *
 * Input:       	none
 *
 * Output:         	none
 *
 * Side Effects:    none
 *
 * Overview:        none
 *
 * Note:          	none
 ********************************************************************/
void CO_PDO1LSTimerEvent(void)
{

}

void CO_PDO2LSTimerEvent(void)
{
    if(angularRateMode && (APPLICATION_STATE != MANEUVER))
    {
        timerCounter++;
        if(timerCounter == 10)
        {
            timerCounter = 0;
            COMM_PDO_2_TF = 1;
        }
    }
}

void CO_PDO3LSTimerEvent(void)
{

}

void CO_PDO4LSTimerEvent(void)
{

}


/*********************************************************************
 * Function:        void CO_PDO1TXFinEvent(void)
 *
 * PreCondition:    none
 *
 * Input:       	none
 *
 * Output:         	none
 *
 * Side Effects:    none
 *
 * Overview:        none
 *
 * Note:          	none
 ********************************************************************/
void CO_PDO1TXFinEvent(void)
{

}

void CO_PDO2TXFinEvent(void)
{

}

void CO_PDO3TXFinEvent(void)
{

}

void CO_PDO4TXFinEvent(void)
{

}

