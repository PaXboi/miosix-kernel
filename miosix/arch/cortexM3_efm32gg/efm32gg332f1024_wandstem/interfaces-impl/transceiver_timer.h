/***************************************************************************
 *   Copyright (C) 2016 by Fabiano Riccardi                                *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   As a special exception, if other files instantiate templates or use   *
 *   macros or inline functions from this file, or you compile this file   *
 *   and link it with other works to produce a work based on this file,    *
 *   this file does not by itself cause the resulting work to be covered   *
 *   by the GNU General Public License. However the source code for this   *
 *   file must still be made available in accordance with the GNU General  *
 *   Public License. This exception does not invalidate any other reasons  *
 *   why a work based on this file might be covered by the GNU General     *
 *   Public License.                                                       *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, see <http://www.gnu.org/licenses/>   *
 ***************************************************************************/

#include "high_resolution_timer_base.h"
#include "timer_interface.h"

#ifndef TRANSCEIVER_TIMER_H
#define TRANSCEIVER_TIMER_H

namespace miosix{
    class TransceiverTimer : public HardwareTimer {
        public:
            //transceiver::excChB //usato per la ricezione INPUT_CAPTURE TIM2_CC0 PA8
            //transceiver::stxon //usato per attivare la trasmissione OUTPUTCOMPARE TIM2_CC1 PA9
            static Thread *tWaiting;
            static TransceiverTimer& instance();
            virtual ~TransceiverTimer();
            
            long long getValue() const;   
            
            void wait(long long tick);         
            bool absoluteWait(long long tick); 
            
            bool absoluteWaitTrigger(long long tick);
            
            bool waitTimeoutOrEvent(long long tick);
            bool absoluteWaitTimeoutOrEvent(long long tick);

            long long tick2ns(long long tick);
            long long ns2tick(long long ns);
            
            unsigned int getTickFrequency() const;
            
            long long getExtEventTimestamp() const;
            

        private:
            TransceiverTimer();
            HighResolutionTimerBase& b;
            TimeConversion tc;
            static const int stabilizingTime;
    };

} //miosix namespace



#endif /* TRANSCEIVER_TIMER_H */

