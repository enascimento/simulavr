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

#include <iostream>
using namespace std;

#include "hwport.h"
#include "avrdevice.h"
#include "trace.h"


HWPort::HWPort(AvrDevice *core, const string &name):Hardware(core),myName(name) {
    Reset();
    for (int tt=0; tt<8; tt++) { 
        string dummy=name+(char)('0'+tt);
        core->RegisterPin(dummy, &p[tt]);
        p[tt].mask=1<<tt;
        p[tt].pinOfPort=&pin;
    }
}


void HWPort::CalcPin() {
    //    cout << "Calc Pin for Port " << myName << endl;
    //calculating the value for register "pin" from the Pin p[] array
    pin=0;
    for (int tt=0; tt<8; tt++) {
        if (p[tt].connectedTo->CalcNet()) pin|=(1<<tt);
    }
}

void HWPort::CalcOutputs() { //Calculate the new output value to be transmitted to the environment
    unsigned char actualBit=0x01;
    unsigned char actualBitNo=0;


    do { //calc all bits
        unsigned char workingPort=0;
        unsigned char workingDdr=0;

        if (useAlternatePortIfDdrSet&actualBit) {
            if (ddr&actualBit) {
                workingPort|= alternatePort&actualBit;
                workingDdr|= actualBit;
            }
        } else {
            if (useAlternateDdr&actualBit) {
                workingDdr|=alternateDdr&actualBit;
            } else {
                workingDdr|=ddr&actualBit;
            }


            if (useAlternatePort&actualBit) {
                workingPort|=alternatePort&actualBit;
            } else {
                workingPort|=port&actualBit;
            }
        }

        //cout << " \n \nworking ddr 0x" << hex << (unsigned int) workingDdr << " working port 0x" << hex << (unsigned int) workingPort << dec << endl;

        if (workingDdr&actualBit) {	//Ddr is output
            if (workingPort&actualBit) { //Port is High
                p[actualBitNo].SetOutState(Pin::HIGH);
            } else {
                p[actualBitNo].SetOutState(Pin::LOW);
            }
        } else { //ddr is low
            if (workingPort&actualBit) {
                p[actualBitNo].SetOutState(Pin::PULLUP);
            } else { 
                p[actualBitNo].SetOutState(Pin::TRISTATE);
            }
        }

        actualBit<<=1;
        actualBitNo++;

    } while (actualBit); // as long as all bits are calculated
    CalcPin(); //now transfer the result also to all HWPort::pin instances
} //end of Calc()

string HWPort::GetPortString() {
    unsigned char dummy[9];
    unsigned int tt;
    for (tt=0; tt<8; tt++) {
        dummy[7-tt]=(unsigned char)p[tt];
    }

    dummy[tt]=0;
    return string((char*)dummy);
}


unsigned char RWPort::operator=(unsigned char val) { trioaccess("Port",val);hwport->SetPort(val); return val; } 

unsigned char RWPin::operator=(unsigned char val) { trioaccess("Pin",val);cerr << "Not allowed to write to Pin! Read-Only!" << endl; return 0;} 

unsigned char RWDdr::operator=(unsigned char val) { trioaccess("Ddr",val);hwport->SetDdr(val); return val; } 


RWPort::operator unsigned char() const { return  hwport->GetPort(); } 
RWDdr::operator unsigned char() const { return  hwport->GetDdr(); } 
RWPin::operator unsigned char() const { return  hwport->GetPin(); } 