/* --COPYRIGHT--,BSD
 * Copyright (c) 2011, Texas Instruments Incorporated
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * *  Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *
 * *  Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * *  Neither the name of Texas Instruments Incorporated nor the names of
 *    its contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
 * EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 * --/COPYRIGHT--*/
//******************************************************************************
//  Demo Application for MSP430/CC1100-2500 Interface
//  Main code application library v1.2
//
//  K. Quiring
//  Version 1.2
//  Texas Instruments, Inc.
//  July 2006
//  IAR Embedded Workbench v3.41
//******************************************************************************
// Change Log:
//******************************************************************************
// Version:  1.2
// Comments: Add startup delay for startup difference between MSP430 and CCxxxx
//******************************************************************************

#include "include.h"

extern char paTable[];
extern char paTableLen;

char txBuffer[4];
char rxBuffer[4];
unsigned int i;

void main (void)
{
  WDTCTL = WDTPW + WDTHOLD;                 // Stop WDT

  // 5ms delay to compensate for time to startup between MSP430 and CC1100/2500
  __delay_cycles(5000);
  
  TI_CC_SPISetup();                         // Initialize SPI port

  TI_CC_PowerupResetCCxxxx();               // Reset CCxxxx
  writeRFSettings();                        // Write RF settings to config reg
  TI_CC_SPIWriteBurstReg(TI_CCxxx0_PATABLE, paTable, paTableLen);//Write PATABLE

  // Configure ports -- switch inputs, LEDs, GDO0 to RX packet info from CCxxxx
//  TI_CC_SW_PxIES = TI_CC_SW1+TI_CC_SW2+TI_CC_SW3+TI_CC_SW4;//Int on falling edge
//  TI_CC_SW_PxIFG &= ~(TI_CC_SW1+TI_CC_SW2+TI_CC_SW3+TI_CC_SW4);//Clr flags
//  TI_CC_SW_PxIE = TI_CC_SW1+TI_CC_SW2+TI_CC_SW3+TI_CC_SW4;//Activate enables
  TI_CC_SW_PxIES = TI_CC_SW1+TI_CC_SW2;//Int on falling edge
  TI_CC_SW_PxIFG &= ~(TI_CC_SW1+TI_CC_SW2);//Clr flags
  TI_CC_SW_PxIE = TI_CC_SW1+TI_CC_SW2;//Activate enables
  //TI_CC_SW_PxREN = TI_CC_SW1;               // Enable Pull-Up Resistor
  //TI_CC_SW_PxOUT = TI_CC_SW1;               // Enable Pull-Up Resistor
//  TI_CC_LED_PxDIR = TI_CC_LED1 + TI_CC_LED2 + TI_CC_LED3 + TI_CC_LED4; //Outputs
  TI_CC_LED_PxOUT &= ~(TI_CC_LED1 + TI_CC_LED2); //Outputs
  TI_CC_LED_PxDIR = TI_CC_LED1 + TI_CC_LED2; //Outputs

  TI_CC_GDO0_PxIES |= TI_CC_GDO0_PIN;       // Int on falling edge (end of pkt)
  TI_CC_GDO0_PxIFG &= ~TI_CC_GDO0_PIN;      // Clear flag
  TI_CC_GDO0_PxIE |= TI_CC_GDO0_PIN;        // Enable int on end of packet

  TI_CC_SPIStrobe(TI_CCxxx0_SRX);           // Initialize CCxxxx in RX mode.
                                            // When a pkt is received, it will
                                            // signal on GDO0 and wake CPU
  __bis_SR_register(LPM3_bits + GIE);       // Enter LPM3, enable interrupts
}


// The ISR assumes the interrupt came from a press of one of the four buttons
// and therefore does not check the other four inputs.
#pragma vector=PORT3_VECTOR
__interrupt void port3_ISR (void)
{
  if(P3IFG & (TI_CC_SW1 + TI_CC_SW2))
  {
    // Build packet
    txBuffer[0] = 2;                           // Packet length
    txBuffer[1] = 0x01;                        // Packet address
    txBuffer[2] = (~TI_CC_SW_PxIN) & 0x0F;     // Load four switch inputs

    RFSendPacket(txBuffer, 3);                 // Send value over RF
  }
  else if(P3IFG & TI_CC_GDO0_PIN)
  {
    char len=2;                               // Len of pkt to be RXed (only addr
                                            // plus data; size byte not incl b/c
                                            // stripped away within RX function)
    if (RFReceivePacket(rxBuffer,&len))       // Fetch packet from CCxxxx
      TI_CC_LED_PxOUT |= LED0;//rxBuffer[1];         // Toggle LEDs according to pkt data
  }
//  P1IFG &= ~(TI_CC_SW1+TI_CC_SW2+TI_CC_SW3+TI_CC_SW4);//Clr flag that caused int

  TI_CC_SW_PxIFG &= ~(TI_CC_SW1+TI_CC_SW2); // Clr flag that caused int
  TI_CC_GDO0_PxIFG &= ~TI_CC_GDO0_PIN;      // After pkt TX, this flag is set.

  TI_CC_LED_PxOUT |= LED0;//rxBuffer[1];         // Toggle LEDs according to pkt data
}                                           // Clear it.


// The ISR assumes the int came from the pin attached to GDO0 and therefore
// does not check the other seven inputs.  Interprets this as a signal from
// CCxxxx indicating packet received.
#pragma vector=PORT2_VECTOR
__interrupt void port2_ISR (void)
{
  char len=2;                               // Len of pkt to be RXed (only addr
                                            // plus data; size byte not incl b/c
                                            // stripped away within RX function)
  if (RFReceivePacket(rxBuffer,&len))       // Fetch packet from CCxxxx
    TI_CC_LED_PxOUT ^= LED0; //rxBuffer[1];         // Toggle LEDs according to pkt data

  TI_CC_GDO0_PxIFG &= ~TI_CC_GDO0_PIN;                 // Clear flag
}


