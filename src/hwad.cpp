/*
 ****************************************************************************
 *
 * simulavr - A simulator for the Atmel AVR family of microcontrollers.
 * Copyright (C) 2001, 2002, 2003   Klaus Rudolph		
 * 
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 ****************************************************************************
 */
#include "hwad.h"

#include "irqsystem.h"
#include "trace.h"

#define MUX0 0x01
#define MUX1 0x02
#define MUX2 0x04
#define ADCBG 0x40 //currently not supported


HWAdmux::HWAdmux( AvrDevice *c, PinAtPort _ad0, PinAtPort _ad1, PinAtPort _ad2, 
        PinAtPort _ad3, PinAtPort _ad4, PinAtPort _ad5) : 
Hardware(c), core(c) {

    ad[0]=_ad0;
    ad[1]=_ad1;
    ad[2]=_ad2;
    ad[3]=_ad3;
    ad[4]=_ad4;
    ad[5]=_ad5;


    Reset();
}

void HWAdmux::Reset() {
    admux=0;
}

void HWAdmux::SetAdmux(unsigned char val) {
    admux=val;
}

unsigned char HWAdmux::GetAdmux() {
    return admux;
}

int HWAdmux::GetMuxOutput() {
    return ad[admux&(MUX2|MUX1|MUX0)].GetAnalog();
}

unsigned char RWAdmux::operator=(unsigned char val) { trioaccess("Admux",val); admux->SetAdmux(val);  return val; } 
RWAdmux::operator unsigned char() const { return admux->GetAdmux(); } 

//---------------------------------------------------------------------------------
#define ADEN 0x80
#define ADSC 0x40
#define ADFR 0x20
#define ADIF 0x10
#define ADIE 0x08
#define ADPS2 0x04
#define ADPS1 0x02
#define ADPS0 0x01


HWAd::HWAd( AvrDevice *c, HWAdmux *a, HWIrqSystem *i, PinAtPort _aref, unsigned int iv) :
Hardware(c), core(c), admux(a), irqSystem(i), aref(_aref), irqVec(iv) {
    core->AddToCycleList(this);
//    irqSystem->RegisterIrqPartner(this, iv);

    Reset();
}

void HWAd::Reset() {
    adcsr=0;
    state=IDLE;
    prescaler=0;
    clk=0;
    usedBefore=false;
    adchLocked=false;
}

unsigned char HWAd::GetAdch() { adchLocked=false; return adch; }
unsigned char HWAd::GetAdcl() { adchLocked=true; return adcl; }
unsigned char HWAd::GetAdcsr() { return adcsr; }

void HWAd::SetAdcsr(unsigned char val) {
    unsigned char old=adcsr&(ADIF|ADSC);
    if (val & ADIF ) old&= ~ADIF; //clear IRQ Flag if set in val
    adcsr= old|(val& (~(ADIF)));

    if ((adcsr&(ADIE|ADIF))==(ADIE|ADIF)) {
        irqSystem->SetIrqFlag(this, irqVec);
    } else {
        irqSystem->ClearIrqFlag(irqVec);
    }
}


#if 0
bool HWAd::IsIrqFlagSet(unsigned int vec) {
    return 1;

    /*XXX remove this function later

    if (adcsr&ADIF) {
        //cout << "ad is finished checked in irq: adcsr:" << hex << (unsigned int)adcsr << endl;
    }
    if (vec== irqVec) {
        if ((adcsr&(ADIE|ADIF))==(ADIE|ADIF)) {
            //cout << "IrqFlag is 1" << endl;
            return true;
        }
    }
    return false;
    */
}
#endif

void HWAd::ClearIrqFlag(unsigned int vector){
    if (vector==irqVec) {
        adcsr&=~ADIF;
        irqSystem->ClearIrqFlag( irqVec);
    }
}


unsigned int HWAd::CpuCycle() {
    if (adcsr&ADEN) { //ad is enabled
        prescaler++;
        if (prescaler>=128) prescaler=0;

        //ATTENTION: clk runs 2 times faster then clock cycle in spec
        //we need the half clk for getting the analog values 
        //by cycle 1.5 as noted in avr spec, this means cycle 3 in this modell!

        unsigned char oldClk=clk;

        switch ( adcsr & ( ADPS2|ADPS1|ADPS0)) { //clocking with prescaler
            case 0: 
            case ADPS0:
                if ((prescaler%1)==0) clk++;
                break;

            case ADPS1:
                if ((prescaler%2)==0) clk++;
                break;

            case (ADPS1|ADPS0):
                if ((prescaler%4)==0) clk++;
                break;

            case (ADPS2):
                if ((prescaler%8)==0) clk++;
                break;

            case (ADPS2|ADPS0):
                if ((prescaler%16)==0) clk++;
                break;

            case (ADPS2|ADPS1):
                if ((prescaler%32)==0) clk++;
                break;

            case (ADPS2|ADPS1|ADPS0):
                if ((prescaler%64)==0) clk++;
                break;

        } //end of switch prescaler selection

        if (clk!=oldClk) { //the clk has changed

            switch (state) {
                case IDLE:
                    clk=0;
                    if (adcsr&ADSC) { //start a conversion
                        if (usedBefore) {
                            //cout << "running adc" << endl;
                            state=RUNNING;
                        } else {
                            //cout << "Init adc" << endl;
                            state=INIT;
                        }
                    }
                    break;

                case INIT:
                    //we only wait 
                    if (clk==13*2) {
                        state = RUNNING;
                        clk=1*2; //we goes 1 clk ahead while waiting only 12 cycles in real
                        //only corrected while avr spec say : after 13 cycles... 
                        //normaly that can also be done after 12 cycles and start a
                        //14 cycle long normal run... but this is here spec conform :-)
                        usedBefore=true;
                        //cout << "switch adc from init to running" << endl;
                    }
                    break;

                case RUNNING:
                    if (clk==1.5*2) { //sampling
                        adSample= admux->GetMuxOutput();
                        int adref= aref.GetAnalog();
                        //cout << "Sampling adc,  Aref:" << hex << adref << " Ain " << hex << adSample << endl;
                        if (adSample>adref) adSample=aref;
                        if (adref==0) {
                            adSample=INT_MAX;
                        } else {
                            adSample= (int)((float)adSample/(float)adref*INT_MAX);
                        }

                        //cout << "Calculated adSample is: " << adSample << " in % " << dec << (float)adSample/(float)INT_MAX*100 << endl;

                    } else if ( clk== 13*2) {
                        //calc sample to go to 10 bit value
                        adSample= adSample>>(sizeof(int)*8 -(10+1)); 
                        if (adchLocked) {
                            if (trace_on) {
                                traceOut<< "AD-Result lost adch is locked!" << endl;
                            } else {
                                cerr << "AD-Result lost adch is locked!" << endl;
                            }
                        } else { //adch is unlocked
                            adch=adSample>>8;
                        }
                        adcl=adSample&0xff;
                        //cout << "AD Value is " << hex << adSample << endl;
                        
                        adcsr|=ADIF;        //set irq flag (conversion complete)
                        if ((adcsr&(ADIE|ADIF))==(ADIE|ADIF)) {
                            irqSystem->SetIrqFlag(this, irqVec);
                        }
                        //cout << "ad finished " << endl;
                        if (adcsr& ADFR) { //start again and state is running again
                            clk=0;
                        } else {
                            adcsr&=~ADSC;    //not free running->clear ADSC Bit
                        }
                    } else if (clk==14*2) {
                        clk=0; 
                        state = IDLE;
                    }
                    break;

            } // end of switch state




        }

    } else { //ad not enabled
        prescaler=0;
        clk=0;
    }

    return 0;
}



unsigned char RWAdch::operator=(unsigned char val) { trioaccess("Write to adch not supported!",val);  return val; } 
unsigned char RWAdcl::operator=(unsigned char val) { trioaccess("Write to adcl not supported!",val);  return val; } 
unsigned char RWAdcsr::operator=(unsigned char val) { trioaccess("Adcsr",val); ad->SetAdcsr(val);  return val; } 
RWAdcsr::operator unsigned char() const { return ad->GetAdcsr(); } 
RWAdcl::operator unsigned char() const { return ad->GetAdcl(); } 
RWAdch::operator unsigned char() const { return ad->GetAdch(); } 
