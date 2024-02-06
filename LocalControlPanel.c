/*----------------------------------------------------------------------------
*                            ANCRA PROPRIETARY
* 
* The information contained herein is proprietary to the Ancra International LLC
* 
* and shall not be reproduced or disclosed in whole or in part or used for
* 
* any design or manufacture except when such user possesses direct written
* 
* authorization from the Ancra International LLC.
*  
* (c) Copyright 2023 by the Ancra International LLC. All rights reserved.
*---------------------------------------------------------------------------
*/
/*
 *-----------------------------------------------------------------------------
 *
 *  File Name       : LocalControlPanel.c
 *
 *  CSCI Name       : Control Panel
 *
 *  CSU Name        : Application
 *
 *  Report Number   : TBD
 *
 *-----------------------------------------------------------------------------
 *
 *  Description : MDCLS Local Control Panels(1/2/3 LH/RH) transmit the signals 
 *                as"Panel Command" message to CRDC via CAN bus as per ICD and 
 *                receives the signals as "Panel Status" message from CRDC via 
 *                CAN bus as per ICD.
 *
 *-----------------------------------------------------------------------------
 *
 *  Revision History:
 *
 *  Version  Author        Date             Description
 *           Varunan    (02/03/2023)     	Source code for transmit and Receive 
 *           Sangeetha                      for Local control panels(1/2/3 LH/RH)
 *
 *-----------------------------------------------------------------------------
 */

/****************************** HEADER FILES *********************************/
#include "../../Header/stdtypes.h"
#include "../../Header/system/PIC32MK0512MCM100.h"
#include "../../Header/PanelConfiguration.h"
#include "../../Header/LocalControlPanel.h"
#include "../../Header/DebounceLogic.h"
#include "../../Header/CAN_FD.h"
#include "../../Header/Timers.h"
#include "../../Header/Flash.h"
#include "../../Header/delay.h"
#include "../../Header/WatchDog.h"
#include "../../Header/FaultMonitor.h"

/********************* PREPROCESSOR DIRECTIVES  *****************************/

#define DLD_VALUE       (uint32_t)0x0000000DU
#define JUMP_FLAG       (uint32_t)0x9D03FFFCU

#define VALUE           0x00

/********************************* GLOBAL DATA ELEMENTS ***********************/

LCP_CAN_DATA_TX LCP_CAN_Tx;
LCP_CAN_DATA_RX LCP_CAN_Rx;
PayloadBitsRx_LCP LCP_CAN_Rx_PrevMsgPayload;


/************************ EXPORTED OPERATION DEFINITIONS *********************/
/*----------------------------------------------------------------------------
 * Description : This function Initialize the Input and Output Ports for Local Control Panels(1/2/3 LH/RH).
 *                
 * Arguments   : void
 *
 * Return Value: void
 *
 *-----------------------------------------------------------------------------
 */
void InputAndOutputSignalInitForLCP(void)
{
/*  
--|    TRISBbits.TRISB15 equal to TRUE
--|    TRISBbits.TRISB13 equal to TRUE
--|    TRISEbits.TRISE8 equal to TRUE
--|    TRISGbits.TRISG7 equal to TRUE
--|    ANSELEbits.ANSE8 equal to FALSE
--|    ANSELGbits.ANSG7 equal to FALSE
--|    TRISGbits.TRISG12 equal to TRUE
--|    TRISBbits.TRISB11 equal to TRUE
--|    TRISAbits.TRISA12 equal to TRUE
--|    ANSELEbits.ANSE12 equal to FALSE
--|    TRISGbits.TRISG0  equal to TRUE
--|    TRISDbits.TRISD15  equal to   FALSE
--|    LATDbits.LATD15    equal to   VALUE
--|    TRISDbits.TRISD14  equal to   FALSE
--|    LATDbits.LATD14    equal to   VALUE
--|    TRISCbits.TRISC9   equal to   FALSE
--|    LATCbits.LATC9     equal to   VALUE
*/
    /* Configure Input Signals */
    
    /* TGL AFT FWD */
    TRISBbits.TRISB15 = TRUE;
    TRISBbits.TRISB13 = TRUE;
    /* PORT B 15, 13 PINS ARE DIGITAL */
    
    /* TGL IN OUT */
    TRISEbits.TRISE8 = TRUE;
    TRISGbits.TRISG7 = TRUE;
    ANSELEbits.ANSE8 = FALSE;
    ANSELGbits.ANSG7 = FALSE;
    
	/* DUAL LANE */
	
	/* UNLOCK NEXT */
	
    /* 20FT IN OUT */
    TRISGbits.TRISG12 = TRUE;
    TRISBbits.TRISB11 = TRUE;
    /* PORT G, B 15, 13 PINS ARE DIGITAL */
    
    /* PDU STOP */
    TRISAbits.TRISA12 = TRUE;
    ANSELEbits.ANSE12 = FALSE;
	
	/* LAMP TEST */
    TRISGbits.TRISG0  = TRUE;
    
    /* Configure Output Signals */
    
    /* PANEL ENABLED LED */
    TRISDbits.TRISD15  =   FALSE;
    LATDbits.LATD15    =   VALUE;
	
    /* DUAL LANE LED */
	
    /* UNLOCK NEXT LED */
    
    /* PDU STOP LED */
    TRISDbits.TRISD14  =   FALSE;
    LATDbits.LATD14    =   VALUE;
    
    /* LAMP TEST LED */
    TRISCbits.TRISC9   =   FALSE;
    LATCbits.LATC9     =   VALUE;
}
/*-----------------------------------------------------------------------------
 *  Description : this function Initialize the Input and Output Ports for Local Control Panels(1/2/3 LH/RH) and TMR0, TMR1.
 *                 Initialize Node Identifier with LCP ID(eLCP1LH/eLCP1RH/eLCP2LH/eLCP2RH/eLCP3LH/eLCP3RH).
 *
 *  Arguments   : PanelType
 *
 *  Return Value: void
 *
 *-----------------------------------------------------------------------------
 */
void InitializationsForLCP(PanelType PanelID)
{
/*
--|    InputAndOutputSignalInitForLCP()
--|    TMR0_Initialize()
--|    TMR1_Initialize()
--|    LCP_CAN_Rx.ArbitrationID.ArbitrationField.NodeID equal to PanelID
*/
	/* Initialize Input and Output Ports for LCP */
    InputAndOutputSignalInitForLCP();
	
    /*Initialize TMR0 */
    TMR0_Initialize();
    
    /*Initialize TMR1 */
    TMR1_Initialize();
    
    /* Initialize Node Identifier with MCP ID */
    LCP_CAN_Rx.ArbitrationID.ArbitrationField.NodeID = PanelID;
}
/*-----------------------------------------------------------------------------
 *  Description : This function is responsible for execute the lamp test for push buttons and PDU stop.
 *              
 *  Arguments   : void
 *
 *  Return Value: void
 *
 *-----------------------------------------------------------------------------
*/
void LampTestProcessForLCP(void)
{
/*
--| if(Debounce_RG00)
--|    LATDbits.LATD15    equal to   TRUE
--|       LATEbits.LATE12 equal to   TRUE
--|	   LATEbits.LATE13    equal to   TRUE
--|	   LATDbits.LATD14    equal to   TRUE
--|	   LATCbits.LATC9     equal to   TRUE
--|	else  
--|    LATDbits.LATD15 equal to LCP_CAN_Rx.Payload.PayloadFormat.Panel_Enabled_LED
--|    LATEbits.LATE12 equal to LCP_CAN_Rx.Payload.PayloadFormat.Dual_Lane_LED
--|	   LATEbits.LATE13 equal to LCP_CAN_Rx.Payload.PayloadFormat.Unlock_Next_LED
*/ 
    if(Debounce_RG00)
    {
		/* PANEL ENABLED LED */
		LATDbits.LATD15    =   TRUE;
		
		/* DUAL LANE LED */
		LATEbits.LATE12    =   TRUE;
		
		/* UNLOCK NEXT LED */
		LATEbits.LATE13    =   TRUE;
		
		/* PDU STOP LED */
		LATDbits.LATD14    =   TRUE;
		
		/* LAMP TEST LED */
		LATCbits.LATC9     =   TRUE;
    }
    else
    {
        LATDbits.LATD15 = LCP_CAN_Rx.Payload.PayloadFormat.Panel_Enabled_LED;
		LATEbits.LATE12 = LCP_CAN_Rx.Payload.PayloadFormat.Dual_Lane_LED;
		LATEbits.LATE13 = LCP_CAN_Rx.Payload.PayloadFormat.Unlock_Next_LED;
		//LATDbits.LATD14 = LCP_CAN_Rx.Payload.PayloadFormat.PDUStopLED;
		//LATCbits.LATC9  = LCP_CAN_Rx.Payload.PayloadFormat.LampTestLED;
    }
}
/*-----------------------------------------------------------------------------
 *  Description : This function validate the Dataload request for Local Control Panels(1/2/3 LH/RH).
 *              
 *  Arguments   : void
 *
 *  Return Value: bool
 *
 *-----------------------------------------------------------------------------
*/
bool ValidateDataloadRequestForLCP(void)
{
/*
--| bool Valid is equal to  FALSE   
--| if(LCP_CAN_Rx.ArbitrationID.ArbitrationTotal is equal to  0x15520480)
--|     if(LCP_CAN_Rx.DLC is equal to  8)     
--|         if((LCP_CAN_Rx.Payload.PayloadTotal[0]is equal to 'A')Logical AND(LCP_CAN_Rx.Payload.PayloadTotal[1]is equal to 'B')\
--|                 Logical AND(LCP_CAN_Rx.Payload.PayloadTotal[2]is equal to 'C')Logical AND(LCP_CAN_Rx.Payload.PayloadTotal[3]is equal to 'D')\
--|                 Logical AND(LCP_CAN_Rx.Payload.PayloadTotal[4]is equal to 'E')Logical AND(LCP_CAN_Rx.Payload.PayloadTotal[5]is equal to 'F')\
--|                 Logical AND(LCP_CAN_Rx.Payload.PayloadTotal[6]is equal to '0')Logical AND(LCP_CAN_Rx.Payload.PayloadTotal[7]is equal to '1'))
--|             Valid is equal to  TRUE
--| return Valid
*/
    bool Valid = FALSE;
    
    if(LCP_CAN_Rx.ArbitrationID.ArbitrationTotal == (uint8_t)0x15520480)
    {
        if(LCP_CAN_Rx.DLC == 8)
        {
            if((LCP_CAN_Rx.Payload.PayloadTotal[0]==(uint8_t)'A')&&(LCP_CAN_Rx.Payload.PayloadTotal[1]==(uint8_t)'B')\
                    &&(LCP_CAN_Rx.Payload.PayloadTotal[2]==(uint8_t)'C')&&(LCP_CAN_Rx.Payload.PayloadTotal[3]==(uint8_t)'D')\
                    &&(LCP_CAN_Rx.Payload.PayloadTotal[4]==(uint8_t)'E')&&(LCP_CAN_Rx.Payload.PayloadTotal[5]==(uint8_t)'F')\
                    &&(LCP_CAN_Rx.Payload.PayloadTotal[6]==(uint8_t)'0')&&(LCP_CAN_Rx.Payload.PayloadTotal[7]==(uint8_t)'1'))
            {
                Valid = TRUE;
            }
        }
    }
    
    return Valid;
}

/*-----------------------------------------------------------------------------
 *  Description : This function updates ArbitrationID with Pre-defined values (UNUSED1, LCC, SourceFID, FSB, 
                  LC, PVT FunctionID, NodeID, AIDRCI) and DLC length for Local Control Panels(1/2/3 LH/RH).
 *
 *  Arguments   : PanelType
 *
 *  Return Value: void
 *
 *-----------------------------------------------------------------------------
*/
 
void UpdateControlPanelArbitrationIDForLCP(PanelType PanelID)
{
/*
--| LCP_CAN_Tx.ArbitrationID.ArbitrationField.UNUSED1 			is equal to  AIDUNUSED
--| LCP_CAN_Tx.ArbitrationID.ArbitrationField.LCC 				is equal to  AIDLCC
--| LCP_CAN_Tx.ArbitrationID.ArbitrationField.SourceFID 		is equal to  AIDSOURCEFID
--| LCP_CAN_Tx.ArbitrationID.ArbitrationField.FSB  				is equal to  AIDFSB
--| LCP_CAN_Tx.ArbitrationID.ArbitrationField.LCL 				is equal to  AIDLCL
--| LCP_CAN_Tx.ArbitrationID.ArbitrationField.PVT 				is equal to  AIDPVT
--| LCP_CAN_Tx.ArbitrationID.ArbitrationField.FunctionID        is equal to  AIDPANELCMD
--| LCP_CAN_Tx.ArbitrationID.ArbitrationField.NodeID    		is equal to  PanelID
--| LCP_CAN_Tx.ArbitrationID.ArbitrationField.RCI 				is equal to  AIDRCI
--| LCP_CAN_Tx.DLC is equal to  8
*/
    
    //Updating Arbitration field values as per ICD
    LCP_CAN_Tx.ArbitrationID.ArbitrationField.UNUSED1 			= AIDUNUSED;
    LCP_CAN_Tx.ArbitrationID.ArbitrationField.LCC 				= AIDLCC;
    LCP_CAN_Tx.ArbitrationID.ArbitrationField.SourceFID 		= AIDSOURCEFID;
    LCP_CAN_Tx.ArbitrationID.ArbitrationField.FSB  				= AIDFSB;
    LCP_CAN_Tx.ArbitrationID.ArbitrationField.LCL 				= AIDLCL;
    LCP_CAN_Tx.ArbitrationID.ArbitrationField.PVT 				= AIDPVT;
    LCP_CAN_Tx.ArbitrationID.ArbitrationField.FunctionID        = AIDPANELCMD;
    LCP_CAN_Tx.ArbitrationID.ArbitrationField.NodeID    		= PanelID;
    LCP_CAN_Tx.ArbitrationID.ArbitrationField.RCI 				= AIDRCI;

    LCP_CAN_Tx.DLC = 8; 
}

/*-----------------------------------------------------------------------------
 *  Description : This function check the received messages Arbitration field(Function identifier,Node_ID, 
 *                Source_ID) and Panel Error from CRDC if all values are matching with defined values it will set 
 *                to true.
 *
 *  Arguments   : PanelType
 *
 *  Return Value: bool
 *
 *-----------------------------------------------------------------------------
*/
bool ValidateLCPArbitrationID(PanelType PanelID)
{
/* 
--| bool Valid is equal to  FALSE 
--| if((LCP_CAN_Rx.ArbitrationID.ArbitrationField.FunctionID is equal to  AIDPANELSTS)Logical OR \
--|         (LCP_CAN_Rx.ArbitrationID.ArbitrationField.FunctionID is equal to  AIDPANELERR))
--|     if(LCP_CAN_Rx.ArbitrationID.ArbitrationField.NodeID is equal to  PanelID)
--|         if((LCP_CAN_Rx.ArbitrationID.ArbitrationField.LCC is equal to  AIDLCC)Logical AND \
--|                     (LCP_CAN_Rx.ArbitrationID.ArbitrationField.SourceFID is equal to  AIDSOURCEFID))
--|             if((LCP_CAN_Rx.ArbitrationID.ArbitrationField.FSB is equal to  AIDFSB)Logical AND \
--|                     (LCP_CAN_Rx.ArbitrationID.ArbitrationField.LCL is equal to  AIDLCL)Logical AND \
--|                     (LCP_CAN_Rx.ArbitrationID.ArbitrationField.PVT is equal to  AIDPVT))
--|                 if(LCP_CAN_Rx.ArbitrationID.ArbitrationField.RCI is equal to  AIDRCI)
--|                     Valid is equal to  TRUE 
--| return Valid
*/
    bool Valid = FALSE;
    
    if((LCP_CAN_Rx.ArbitrationID.ArbitrationField.FunctionID == AIDPANELSTS)||\
            (LCP_CAN_Rx.ArbitrationID.ArbitrationField.FunctionID == AIDPANELERR))
    {
        if(LCP_CAN_Rx.ArbitrationID.ArbitrationField.NodeID == PanelID)
        {
            if((LCP_CAN_Rx.ArbitrationID.ArbitrationField.LCC == AIDLCC)&&\
                        (LCP_CAN_Rx.ArbitrationID.ArbitrationField.SourceFID == AIDSOURCEFID))
            {
                if((LCP_CAN_Rx.ArbitrationID.ArbitrationField.FSB == AIDFSB)&&\
                        (LCP_CAN_Rx.ArbitrationID.ArbitrationField.LCL == AIDLCL)&&\
                        (LCP_CAN_Rx.ArbitrationID.ArbitrationField.PVT == AIDPVT))
                {
                    if(LCP_CAN_Rx.ArbitrationID.ArbitrationField.RCI == AIDRCI)
                    {
                        Valid = TRUE;
                    }  
                }
            }
        }      
    }
    
    return Valid;
}
/*----------------------------------------------------------------------------
 * Description : This function updates the CAN_payload bits as per the status of input pins in PORT registers
 *               with de bounce and event captured for buttons and switches.
 *                           
 *  Arguments   : void
 *
 *  Return Value: void
 *
 *-----------------------------------------------------------------------------
 */
void NonLatchInputProcessForLCP(void)
{
/*
--|LCP_CAN_Tx.Payload.PayloadFormat.TGLS_Drive_FWD  equal to Debounce_RB15
--|    LCP_CAN_Tx.Payload.PayloadFormat.TGLS_Drive_AFT   equal to Debounce_RB13
--|    LCP_CAN_Tx.Payload.PayloadFormat.Dual_Lane equal to Debounce_RD04
--|    LCP_CAN_Tx.Payload.PayloadFormat.Unlock_Next equal to Debounce_RD03
--|    LCP_CAN_Tx.Payload.PayloadFormat.PDU_Stop equal to Debounce_RA12
*/
    LCP_CAN_Tx.Payload.PayloadFormat.TGLS_Drive_FWD         = (uint8_t)Debounce_RB15;
    LCP_CAN_Tx.Payload.PayloadFormat.TGLS_Drive_AFT         = (uint8_t)Debounce_RB13;
    //LCP_CAN_Tx.Payload.PayloadFormat.TGLS_Drive_Neutral    = Debounce_RG14;
    LCP_CAN_Tx.Payload.PayloadFormat.Dual_Lane              = (uint8_t)Debounce_RD04;
    LCP_CAN_Tx.Payload.PayloadFormat.Unlock_Next            = (uint8_t)Debounce_RD03;
    LCP_CAN_Tx.Payload.PayloadFormat.PDU_Stop				= (uint8_t)Debounce_RA12;
}
/*----------------------------------------------------------------------------
 * Description : This function updates the CAN payload bits as per the status of input pins in PORT registers 
                 with de bounce and event captured for buttons and switches.
 *		
 *  Arguments   : void
 *
 *  Return Value: void
 *
 *-----------------------------------------------------------------------------
 */

void LatchedInputProcessForLCP(void)
{    
/* 
--|if(LCP_CAN_Rx_PrevMsgPayload.Dual_Lane_LED  not equal to LCP_CAN_Rx.Payload.PayloadFormat.Dual_Lane_LED)
--|		LCP_CAN_Tx.Payload.PayloadFormat.Dual_Lane equal to LCP_CAN_Rx.Payload.PayloadFormat.Dual_Lane_LED
--|	if(LCP_CAN_Rx_PrevMsgPayload.Unlock_Next_LED  not equal to LCP_CAN_Rx.Payload.PayloadFormat.Unlock_Next_LED)
--|		LCP_CAN_Tx.Payload.PayloadFormat.Unlock_Next equal to LCP_CAN_Rx.Payload.PayloadFormat.Unlock_Next_LED
--|
--|    LCP_CAN_Rx_PrevMsgPayload equal to LCP_CAN_Rx.Payload.PayloadFormat
--|	
--|    LCP_CAN_Tx.Payload.PayloadFormat.TGLS_Drive_FWD equal to Debounce_RB15
--|    LCP_CAN_Tx.Payload.PayloadFormat.TGLS_Drive_AFT equal to Debounce_RB13   
--|	if((previousDebounce_RB14 is equal to  FALSE)AND (Debounce_RB14 is equal to  TRUE))
--|        if(LCP_CAN_Tx.Payload.PayloadFormat.Dual_Lane is equal to  FALSE)
--|            LCP_CAN_Tx.Payload.PayloadFormat.Dual_Lane is equal to  TRUE
--|        else if(LCP_CAN_Tx.Payload.PayloadFormat.Dual_Lane is equal to  TRUE)
--|            LCP_CAN_Tx.Payload.PayloadFormat.Dual_Lane is equal to  FALSE
--|	if((previousDebounce_RA03 is equal to  FALSE)AND (Debounce_RA03 is equal to  TRUE))
--|        if(LCP_CAN_Tx.Payload.PayloadFormat.Unlock_Next is equal to  FALSE)
--|            LCP_CAN_Tx.Payload.PayloadFormat.Unlock_Next is equal to  TRUE
--|        else if(LCP_CAN_Tx.Payload.PayloadFormat.Unlock_Next is equal to  TRUE)
--|                LCP_CAN_Tx.Payload.PayloadFormat.Unlock_Next is equal to  FALSE
--|                LCP_CAN_Tx.Payload.PayloadFormat.PDU_Stop is equal to  Debounce_RA12	
--| previousDebounce_RB15 is equal to  Debounce_RB15

--|	previousDebounce_RB13 is equal to  Debounce_RB13


--|	previousDebounce_RD04 is equal to  Debounce_RD04
--|	previousDebounce_RD03 is equal to  Debounce_RD03
--|	previousDebounce_RA12is equal to  Debounce_RA12
*/
if(LCP_CAN_Rx_PrevMsgPayload.Dual_Lane_LED != LCP_CAN_Rx.Payload.PayloadFormat.Dual_Lane_LED)
{
    LCP_CAN_Tx.Payload.PayloadFormat.Dual_Lane = LCP_CAN_Rx.Payload.PayloadFormat.Dual_Lane_LED;
}
if(LCP_CAN_Rx_PrevMsgPayload.Unlock_Next_LED != LCP_CAN_Rx.Payload.PayloadFormat.Unlock_Next_LED)
{
	LCP_CAN_Tx.Payload.PayloadFormat.Unlock_Next = LCP_CAN_Rx.Payload.PayloadFormat.Unlock_Next_LED;
}

    LCP_CAN_Rx_PrevMsgPayload = LCP_CAN_Rx.Payload.PayloadFormat;
	
    LCP_CAN_Tx.Payload.PayloadFormat.TGLS_Drive_FWD         = (uint8_t)Debounce_RB15;
    LCP_CAN_Tx.Payload.PayloadFormat.TGLS_Drive_AFT         = (uint8_t)Debounce_RB13;
	if((previousDebounce_RB14 == FALSE)&&(Debounce_RB14 == TRUE))
    {
        if(LCP_CAN_Tx.Payload.PayloadFormat.Dual_Lane == FALSE)
        {
            LCP_CAN_Tx.Payload.PayloadFormat.Dual_Lane = TRUE;
        }
        else if(LCP_CAN_Tx.Payload.PayloadFormat.Dual_Lane == TRUE)
        {
            LCP_CAN_Tx.Payload.PayloadFormat.Dual_Lane = FALSE;
        }
        else
        {
            /* Do Nothing */
        }
    }
	if((previousDebounce_RD03 == FALSE)&&(Debounce_RD03 == TRUE))
    {
        if(LCP_CAN_Tx.Payload.PayloadFormat.Unlock_Next == FALSE)
        {
            LCP_CAN_Tx.Payload.PayloadFormat.Unlock_Next = TRUE;
        }
        else if(LCP_CAN_Tx.Payload.PayloadFormat.Unlock_Next == TRUE)
        {
            LCP_CAN_Tx.Payload.PayloadFormat.Unlock_Next = FALSE;
        }
        else
        {
            /* Do Nothing */
        }
    }
	
    LCP_CAN_Tx.Payload.PayloadFormat.PDU_Stop				= (uint8_t)Debounce_RA12;
	
	previousDebounce_RB15 = Debounce_RB15;
	previousDebounce_RB13 = Debounce_RB13;
	previousDebounce_RD04 = Debounce_RD04;
	previousDebounce_RD03 = Debounce_RD03;
    previousDebounce_RA12 = Debounce_RA12;
}

/*----------------------------------------------------------------------------
 * Description : This function updates the Local Control Panels(1/2/3 LH/RH) arbitration ID and execute the Latched and non-latched input 
 *                process for Local Control Panels(1/2/3 LH/RH).
 *		
 *  Arguments   : PanelType 
 *
 *  Return Value: void
 *
 *-----------------------------------------------------------------------------
 */

void InputProcessForLCP(PanelType PanelID)
{
/*
--|bool packetsnt equal to false
--|	LatchedInputProcessForLCP()
--|    UpdateControlPanelArbitrationIDForLCP(PanelID)
--|    packetsnt equal to CANFD1_MessageTransmit(LCP_CAN_Tx.ArbitrationID.ArbitrationTotal
--|                     LCP_CAN_Tx.DLC LCP_CAN_Tx.Payload.PayloadTotal 0 0 0)
*/
    bool packetsnt = false;
    
	LatchedInputProcessForLCP();
    
    UpdateControlPanelArbitrationIDForLCP(PanelID);
    
    packetsnt = CANFD1_MessageTransmit(LCP_CAN_Tx.ArbitrationID.ArbitrationTotal,
                     LCP_CAN_Tx.DLC, LCP_CAN_Tx.Payload.PayloadTotal, 0, 0, 0); 
}

/*----------------------------------------------------------------------------
 * Description : This function Validate the Arbitration ID received via CAN with Local Control Panels(1/2/3 LH/RH) Arbitration ID.Updates 
 *                  the output status of LEDs and Dataload request as per the Local Control Panels(1/2/3 LH/RH) Payload bits received via CAN.
 *               
 *  Arguments   : PanelType 
 *
 *  Return Value: void
 *
 *-----------------------------------------------------------------------------
 */

void LedControlForLCP(PanelType PanelID)
{
/*  
--| bool packetrvd is equal to  FALSE, packetsnt is equal to  FALSE
--| bool ArbitrationCheckresult is equal to  FALSE
--| bool DataloadRequestStatus is equal to  FALSE
--| CANFD_MSG_RX_ATTRIBUTE msgAttr
--| LCP_CAN_DATA_RX PreviousData
--| PreviousData.ArbitrationID.ArbitrationTotal is equal to  LCP_CAN_Rx.ArbitrationID.ArbitrationTotal
--| PreviousData.DLC is equal to  LCP_CAN_Rx.DLC
--| PreviousData.Payload.PayloadTotal[0] is equal to  LCP_CAN_Rx.Payload.PayloadTotal[0]
--| PreviousData.Payload.PayloadTotal[1] is equal to  LCP_CAN_Rx.Payload.PayloadTotal[1]
--| PreviousData.Payload.PayloadTotal[2] is equal to  LCP_CAN_Rx.Payload.PayloadTotal[2]
--| PreviousData.Payload.PayloadTotal[3] is equal to  LCP_CAN_Rx.Payload.PayloadTotal[3]
--| PreviousData.Payload.PayloadTotal[4] is equal to  LCP_CAN_Rx.Payload.PayloadTotal[4]
--| PreviousData.Payload.PayloadTotal[5] is equal to  LCP_CAN_Rx.Payload.PayloadTotal[5]
--| PreviousData.Payload.PayloadTotal[6] is equal to  LCP_CAN_Rx.Payload.PayloadTotal[6]
--| PreviousData.Payload.PayloadTotal[7] is equal to  LCP_CAN_Rx.Payload.PayloadTotal[7]
--| packetrvd is equal to  CANFD1_Receive(AND (LCP_CAN_Rx.ArbitrationID.ArbitrationTotal),
--|             AND (LCP_CAN_Rx.DLC),AND (LCP_CAN_Rx.Payload.PayloadTotal), 0, 2, AND msgAttr)
--| ArbitrationCheckresult is equal to  ValidateLCPArbitrationID(PanelID)
--| DataloadRequestStatus is equal to  ValidateDataloadRequestForLCP()    
--| if((packetrvd is equal to  TRUE) Logical AND  (ArbitrationCheckresult is equal to  TRUE))
--|     if((LCP_CAN_Rx.ArbitrationID.ArbitrationField.FunctionID is equal to  AIDPANELERR)Logical AND \
--|ifdef GROWTH_PROVISION
--|endif
--|         if(LCP_CAN_Rx.ArbitrationID.ArbitrationField.FunctionID is equal to  AIDPANELSTS)       
--|             LATDbits.LATD15 is equal to LCP_CAN_Rx.Payload.PayloadFormat.Panel_Enabled_LED;
--|             LATEbits.LATE12 is equal to LCP_CAN_Rx.Payload.PayloadFormat.Dual_Lane_LED;
 --|            LATEbits.LATE13 is equal to LCP_CAN_Rx.Payload.PayloadFormat.Unlock_Next_LED;
--|                 LCP_CAN_Rx.Payload.PayloadFormat.LED_Panel_Enabled
--|     else
--|         LCP_CAN_Rx.ArbitrationID.ArbitrationTotal is equal to  PreviousData.ArbitrationID.ArbitrationTotal
--|         LCP_CAN_Rx.DLC is equal to  PreviousData.DLC
--|         LCP_CAN_Rx.Payload.PayloadTotal[0] is equal to  PreviousData.Payload.PayloadTotal[0]
--|         LCP_CAN_Rx.Payload.PayloadTotal[1] is equal to  PreviousData.Payload.PayloadTotal[1]
--|         LCP_CAN_Rx.Payload.PayloadTotal[2] is equal to  PreviousData.Payload.PayloadTotal[2]
--|         LCP_CAN_Rx.Payload.PayloadTotal[3] is equal to  PreviousData.Payload.PayloadTotal[3]
--|         LCP_CAN_Rx.Payload.PayloadTotal[4] is equal to  PreviousData.Payload.PayloadTotal[4]
--|         LCP_CAN_Rx.Payload.PayloadTotal[5] is equal to  PreviousData.Payload.PayloadTotal[5]
--|         LCP_CAN_Rx.Payload.PayloadTotal[6] is equal to  PreviousData.Payload.PayloadTotal[6]
--|         LCP_CAN_Rx.Payload.PayloadTotal[7] is equal to  PreviousData.Payload.PayloadTotal[7]
--| else if((packetrvd is equal to  TRUE) Logical AND  (DataloadRequestStatus is equal to  TRUE))  
--|     LATAbits.LATA8 is equal to  FALSE 
--|     LATBbits.LATB4 is equal to  FALSE
--|     LATAbits.LATA4 is equal to  FALSE
--|     DRV_FLASH0_WriteWord(JUMP_FLAG, DLD_VALUE)
--|     delay_us(100)
--|     WATCHDOG_TimerStart()
--|     while(TRUE)
--| else
--|         LCP_CAN_Rx.ArbitrationID.ArbitrationTotal is equal to  PreviousData.ArbitrationID.ArbitrationTotal
--|         LCP_CAN_Rx.DLC is equal to  PreviousData.DLC
--|         LCP_CAN_Rx.Payload.PayloadTotal[0] is equal to  PreviousData.Payload.PayloadTotal[0]
--|         LCP_CAN_Rx.Payload.PayloadTotal[1] is equal to  PreviousData.Payload.PayloadTotal[1]
--|         LCP_CAN_Rx.Payload.PayloadTotal[2] is equal to  PreviousData.Payload.PayloadTotal[2]
--|         LCP_CAN_Rx.Payload.PayloadTotal[3] is equal to  PreviousData.Payload.PayloadTotal[3]
--|         LCP_CAN_Rx.Payload.PayloadTotal[4] is equal to  PreviousData.Payload.PayloadTotal[4]
--|         LCP_CAN_Rx.Payload.PayloadTotal[5] is equal to  PreviousData.Payload.PayloadTotal[5]
--|         LCP_CAN_Rx.Payload.PayloadTotal[6] is equal to  PreviousData.Payload.PayloadTotal[6]
--|         LCP_CAN_Rx.Payload.PayloadTotal[7] is equal to  PreviousData.Payload.PayloadTotal[7]
*/     
    bool packetrvd = FALSE, packetsnt = FALSE;
    bool ArbitrationCheckresult = FALSE;
    bool DataloadRequestStatus = FALSE;
    CANFD_MSG_RX_ATTRIBUTE msgAttr;
    LCP_CAN_DATA_RX PreviousData;
    
	/* Store the Previous Cycle Data */
    PreviousData.ArbitrationID.ArbitrationTotal = LCP_CAN_Rx.ArbitrationID.ArbitrationTotal;
    PreviousData.DLC = LCP_CAN_Rx.DLC;
    PreviousData.Payload.PayloadTotal[0] = LCP_CAN_Rx.Payload.PayloadTotal[0];
    PreviousData.Payload.PayloadTotal[1] = LCP_CAN_Rx.Payload.PayloadTotal[1];
    PreviousData.Payload.PayloadTotal[2] = LCP_CAN_Rx.Payload.PayloadTotal[2];
    PreviousData.Payload.PayloadTotal[3] = LCP_CAN_Rx.Payload.PayloadTotal[3];
    PreviousData.Payload.PayloadTotal[4] = LCP_CAN_Rx.Payload.PayloadTotal[4];
    PreviousData.Payload.PayloadTotal[5] = LCP_CAN_Rx.Payload.PayloadTotal[5];
    PreviousData.Payload.PayloadTotal[6] = LCP_CAN_Rx.Payload.PayloadTotal[6];
    PreviousData.Payload.PayloadTotal[7] = LCP_CAN_Rx.Payload.PayloadTotal[7];
    
    /* Receive the Current Cycle CAN Frame */
    packetrvd = CANFD1_Receive(&(LCP_CAN_Rx.ArbitrationID.ArbitrationTotal),
                &(LCP_CAN_Rx.DLC),&(LCP_CAN_Rx.Payload.PayloadTotal), 0, 2, &msgAttr);
    
    /* Validate the Control Panel Arbitration ID received in CAN */
    ArbitrationCheckresult = ValidateLCPArbitrationID(PanelID);
    /* Check for valid Upload Request with SWFIN received in CAN for DL */
    DataloadRequestStatus = ValidateDataloadRequestForLCP();
    
    if((packetrvd == TRUE) && (ArbitrationCheckresult == TRUE))
    {
#ifdef GROWTH_PROVISION
        #endif
        if(LCP_CAN_Rx.ArbitrationID.ArbitrationField.FunctionID == AIDPANELSTS)
        {
			//Writing the received output signals
            LATDbits.LATD15 = LCP_CAN_Rx.Payload.PayloadFormat.Panel_Enabled_LED;
            LATEbits.LATE12 = LCP_CAN_Rx.Payload.PayloadFormat.Dual_Lane_LED;
            LATEbits.LATE13 = LCP_CAN_Rx.Payload.PayloadFormat.Unlock_Next_LED;
           
        }
        else
        {
            /* Restore the Previous Cycle Data */
            LCP_CAN_Rx.ArbitrationID.ArbitrationTotal = PreviousData.ArbitrationID.ArbitrationTotal;
            LCP_CAN_Rx.DLC = PreviousData.DLC;
            LCP_CAN_Rx.Payload.PayloadTotal[0] = PreviousData.Payload.PayloadTotal[0];
            LCP_CAN_Rx.Payload.PayloadTotal[1] = PreviousData.Payload.PayloadTotal[1];
            LCP_CAN_Rx.Payload.PayloadTotal[2] = PreviousData.Payload.PayloadTotal[2];
            LCP_CAN_Rx.Payload.PayloadTotal[3] = PreviousData.Payload.PayloadTotal[3];
            LCP_CAN_Rx.Payload.PayloadTotal[4] = PreviousData.Payload.PayloadTotal[4];
            LCP_CAN_Rx.Payload.PayloadTotal[5] = PreviousData.Payload.PayloadTotal[5];
            LCP_CAN_Rx.Payload.PayloadTotal[6] = PreviousData.Payload.PayloadTotal[6];
            LCP_CAN_Rx.Payload.PayloadTotal[7] = PreviousData.Payload.PayloadTotal[7];
        }
    }
    else if((packetrvd == TRUE) && (DataloadRequestStatus == TRUE))
    {      
        LATAbits.LATA8 = FALSE; /* ON LED1 */
        LATBbits.LATB4 = FALSE; /* ON LED2 */
        LATAbits.LATA4 = FALSE; /* ON LED3 */

        /* Update Jump Flag Flash Address with Data load Value */
        DRV_FLASH0_WriteWord(JUMP_FLAG, DLD_VALUE);
        delay_us(100);
        /* Perform Internal Watch Dog Reset */
        WATCHDOG_TimerStart();
        /* Stop Heart Beat to perform External Watch Dog Reset */
        while(TRUE)
        {
            /* Do Nothing */
        }
    }
    else
    {
        /* Restore the Previous Cycle Data */
		LCP_CAN_Rx.ArbitrationID.ArbitrationTotal = PreviousData.ArbitrationID.ArbitrationTotal;
		LCP_CAN_Rx.DLC = PreviousData.DLC;
		LCP_CAN_Rx.Payload.PayloadTotal[0] = PreviousData.Payload.PayloadTotal[0];
		LCP_CAN_Rx.Payload.PayloadTotal[1] = PreviousData.Payload.PayloadTotal[1];
		LCP_CAN_Rx.Payload.PayloadTotal[2] = PreviousData.Payload.PayloadTotal[2];
		LCP_CAN_Rx.Payload.PayloadTotal[3] = PreviousData.Payload.PayloadTotal[3];
		LCP_CAN_Rx.Payload.PayloadTotal[4] = PreviousData.Payload.PayloadTotal[4];
		LCP_CAN_Rx.Payload.PayloadTotal[5] = PreviousData.Payload.PayloadTotal[5];
		LCP_CAN_Rx.Payload.PayloadTotal[6] = PreviousData.Payload.PayloadTotal[6];
		LCP_CAN_Rx.Payload.PayloadTotal[7] = PreviousData.Payload.PayloadTotal[7];
    }
}
/*-----------------------------------------------------------------------------
 *  Description : This function control the default status of LEDs for Local Control Panels(1/2/3 LH/RH).
 *              
 *  Arguments   : PanelType
 *
 *  Return Value: void
 *
 *-----------------------------------------------------------------------------
*/
void LEDControlDefaultsForLCP(PanelType PanelID)
{ 
/*   
--|    if(Debounce_RG00 equal to FALSE)
--|        if(LCP_CAN_Rx.ArbitrationID.ArbitrationField.NodeID equal to PanelID)
--|           if(LCP_CAN_Tx.Payload.PayloadFormat.PDU_Stop equal to TRUE)
--|              LATDbits.LATD14 equal to LATDbits.LATD14
*/

    if(Debounce_RG00 == FALSE)
    {
        if(LCP_CAN_Rx.ArbitrationID.ArbitrationField.NodeID == PanelID)
        {
			if(LCP_CAN_Tx.Payload.PayloadFormat.PDU_Stop == TRUE)
            {
                /* Blink PDU Stop LED */
                LATDbits.LATD14 = ~LATDbits.LATD14;
            }
        }
    }
}
/*-----------------------------------------------------------------------------
 * Description : This function is responsible for updates the fault status for push buttons and PDU stop, 
 *               Toggle switches for Local Control Panels(1/2/3 LH/RH).
 *              
 *  Arguments   : void
 *
 *  Return Value: void
 *
 *-----------------------------------------------------------------------------
*/

void FaultMonitoringForLCP(void)
{
/*
--| LCP_CAN_Tx.Payload.PayloadFormat.Unlock_Next_Fault   is equal to  0
--| LCP_CAN_Tx.Payload.PayloadFormat.Dual_Lane_Fault     is equal to  0
--| LCP_CAN_Tx.Payload.PayloadFormat.Lamp_Test_Fault     is equal to  LampTestFault()
*/
    /* Place Holder for Fault Monitor Requirements */
    LCP_CAN_Tx.Payload.PayloadFormat.Unlock_Next_Fault   = 0;
    LCP_CAN_Tx.Payload.PayloadFormat.Dual_Lane_Fault     = 0;
    LCP_CAN_Tx.Payload.PayloadFormat.Lamp_Test_Fault     = (uint8_t)LampTestFault();
}

/*************************** End of file **************************/

