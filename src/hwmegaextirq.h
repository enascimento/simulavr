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
#ifndef HWMEGAEXTIRQ
#define HWMEGAEXTIRQ
#include <vector>
using namespace std;
#include "hardware.h"
#include "pinatport.h"
#include "rwmem.h"

class HWMegaExtIrq: public Hardware {
	protected:
        unsigned char eimsk;
        unsigned char eifr;
        unsigned char eicra;
        unsigned char eicrb;
        
		HWIrqSystem *irqSystem;

		bool int_old[8];

		//PinAtPort pinI[8];
        vector<PinAtPort> pinI;

		//unsigned int vectorInt[8];
        vector<unsigned int> vectorInt; 

	public:
		HWMegaExtIrq(AvrDevice *core, HWIrqSystem *, 
                PinAtPort p0, PinAtPort p1, PinAtPort p2, PinAtPort p3,
                PinAtPort p4, PinAtPort p5, PinAtPort p6, PinAtPort p7,
                unsigned int, unsigned int, unsigned int, unsigned int, 
                unsigned int, unsigned int, unsigned int, unsigned int);

		unsigned char GetEimsk();
		unsigned char GetEifr();
		void SetEimsk(unsigned char val);
		void SetEifr(unsigned char val);

		void SetEicra(unsigned char val);
		void SetEicrb(unsigned char val);
        unsigned char GetEicra();
        unsigned char GetEicrb();

		//bool IsIrqFlagSet(unsigned int vector);
		void ClearIrqFlag(unsigned int vector);
		unsigned int CpuCycle();
        void CheckForIrq();
};



class RWEicra: public RWMemoryMembers {
    protected:
        HWMegaExtIrq *megaextirq;

    public:
        RWEicra(HWMegaExtIrq *_irq): megaextirq(_irq) {}
        virtual unsigned char operator=(unsigned char);
        virtual operator unsigned char() const;
};



class RWEicrb: public RWMemoryMembers {
    protected:
        HWMegaExtIrq *megaextirq;

    public:
        RWEicrb(HWMegaExtIrq *_irq): megaextirq(_irq) {}
        virtual unsigned char operator=(unsigned char);
        virtual operator unsigned char() const;
};



class RWEimsk: public RWMemoryMembers {
    protected:
        HWMegaExtIrq *megaextirq;

    public:
        RWEimsk(HWMegaExtIrq *_irq): megaextirq(_irq) {}
        virtual unsigned char operator=(unsigned char);
        virtual operator unsigned char() const;
};



class RWEifr: public RWMemoryMembers {
    protected:
        HWMegaExtIrq *megaextirq;

    public:
        RWEifr(HWMegaExtIrq *_irq): megaextirq(_irq) {}
        virtual unsigned char operator=(unsigned char);
        virtual operator unsigned char() const;
};



#endif