/***************************************************************************
 *   Copyright (C) 2010, 2011, 2012 by Terraneo Federico                   *
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

#include "kernel/logging.h"
#include "kernel/kernel.h"
#include "config/miosix_settings.h"
#include "interfaces/portability.h"
#include "interfaces/arch_registers.h"
#include "interrupts.h"

using namespace miosix;

#ifdef WITH_ERRLOG

/**
 * \internal
 * Used to print an unsigned int in hexadecimal format, and to reboot the system
 * Note that printf/iprintf cannot be used inside an IRQ, so that's why there's
 * this function.
 * \param x number to print
 */
static void printUnsignedInt(unsigned int x)
{
    static const char hexdigits[]="0123456789abcdef";
    char result[]="0x........\r\n";
    for(int i=9;i>=2;i--)
    {
        result[i]=hexdigits[x & 0xf];
        x>>=4;
    }
    IRQerrorLog(result);
}

#endif //WITH_ERRLOG

/**
 * \internal
 * \return the program counter of the thread that was running when the exception
 * occurred.
 */
static unsigned int getProgramCounter()
{
    register unsigned int result;
    // Get program counter when the exception was thrown from stack frame
    asm volatile("mrs   %0,  psp    \n\t"
                 "add   %0, %0, #24 \n\t"
                 "ldr   %0, [%0]    \n\t":"=r"(result));
    return result;
}

/**
 * \internal
 * Wait until all data is sent to the console and reboot
 */
static void waitConsoleAndReboot()
{
    while(!Console::IRQtxComplete()) ; //Wait until all data sent
    miosix_private::IRQsystemReboot();
}

void NMI_Handler()
{
    IRQerrorLog("\r\n***Unexpected NMI\r\n");
    waitConsoleAndReboot();
}

void HardFault_Handler()
{
    #ifdef WITH_ERRLOG
    IRQerrorLog("\r\n***Unexpected HardFault @ ");
    printUnsignedInt(getProgramCounter());
    unsigned int hfsr=SCB->HFSR;
    if(hfsr & SCB_HFSR_FORCED_Msk)
        IRQerrorLog("Fault escalation occurred\r\n");
    if(hfsr & SCB_HFSR_VECTTBL_Msk)
        IRQerrorLog("A BusFault occurred during a vector table read\r\n");
    #endif //WITH_ERRLOG
    waitConsoleAndReboot();
}

void __attribute__((naked)) MemManage_Handler()
{
    saveContext();
    //Call MemManage_impl(). Name is a C++ mangled name.
    asm volatile("bl _Z14MemManage_implv");
    restoreContext();
}

void __attribute__((noinline)) MemManage_impl()
{
    unsigned int cfsr=SCB->CFSR;
    #ifdef WITH_PROCESSES
    int id, arg=0;
    if(cfsr & 0x00000001) id=MP_XN;
    else if(cfsr & 0x00000080) { id=MP; arg=SCB->MMFAR; }
    else id=MP_NOADDR;
    if(miosix::Thread::IRQreportFault(miosix_private::FaultData(
        id,getProgramCounter(),arg))) return;
    #endif //WITH_PROCESSES
    #ifdef WITH_ERRLOG
    IRQerrorLog("\r\n***Unexpected MemManage @ ");
    printUnsignedInt(getProgramCounter());
    if(cfsr & 0x00000080)
    {
        IRQerrorLog("Fault caused by attempted access to ");
        printUnsignedInt(SCB->MMFAR);
    } else IRQerrorLog("The address that caused the fault is missing\r\n");
    if(cfsr & 0x00000010)
        IRQerrorLog("Fault occurred during exception stacking\r\n");
    if(cfsr & 0x00000008)
        IRQerrorLog("Fault occurred during exception unstacking\r\n");
    if(cfsr & 0x00000002)
        IRQerrorLog("Fault was caused by invalid PC\r\n");
    if(cfsr & 0x00000001)
        IRQerrorLog("Fault was caused by attempted execution from XN area\r\n");
    #endif //WITH_ERRLOG
    waitConsoleAndReboot();
}

void BusFault_Handler()
{
    #ifdef WITH_ERRLOG
    IRQerrorLog("\r\n***Unexpected BusFault @ ");
    printUnsignedInt(getProgramCounter());
    unsigned int cfsr=SCB->CFSR;
    if(cfsr & 0x00008000)
    {
        IRQerrorLog("Fault caused by attempted access to ");
        printUnsignedInt(SCB->BFAR);
    } else IRQerrorLog("The address that caused the fault is missing\r\n");
    if(cfsr & 0x00001000)
        IRQerrorLog("Fault occurred during exception stacking\r\n");
    if(cfsr & 0x00000800)
        IRQerrorLog("Fault occurred during exception unstacking\r\n");
    if(cfsr & 0x00000400)
        IRQerrorLog("Fault is imprecise\r\n");
    if(cfsr & 0x00000200)
        IRQerrorLog("Fault is precise\r\n");
    if(cfsr & 0x00000100)
        IRQerrorLog("Fault happened during instruction fetch\r\n");
    #endif //WITH_ERRLOG
    waitConsoleAndReboot();
}

void __attribute__((naked)) UsageFault_Handler()
{
    saveContext();
    //Call UsageFault_impl(). Name is a C++ mangled name.
    asm volatile("bl _Z15UsageFault_implv");
    restoreContext();
}

void __attribute__((noinline)) UsageFault_impl()
{
    unsigned int cfsr=SCB->CFSR;
    #ifdef WITH_PROCESSES
    int id;
    if(cfsr & 0x02000000) id=UF_DIVZERO;
    else if(cfsr & 0x01000000) id=UF_UNALIGNED;
    else if(cfsr & 0x00080000) id=UF_COPROC;
    else if(cfsr & 0x00040000) id=UF_EXCRET;
    else if(cfsr & 0x00020000) id=UF_EPSR;
    else if(cfsr & 0x00010000) id=UF_UNDEF;
    else id=UF_UNEXP;
    if(miosix::Thread::IRQreportFault(miosix_private::FaultData(
        id,getProgramCounter()))) return;
    #endif //WITH_PROCESSES
    #ifdef WITH_ERRLOG
    IRQerrorLog("\r\n***Unexpected UsageFault @ ");
    printUnsignedInt(getProgramCounter());
    if(cfsr & 0x02000000) IRQerrorLog("Divide by zero\r\n");
    if(cfsr & 0x01000000) IRQerrorLog("Unaligned memory access\r\n");
    if(cfsr & 0x00080000) IRQerrorLog("Attempted coprocessor access\r\n");
    if(cfsr & 0x00040000) IRQerrorLog("EXC_RETURN not expected now\r\n");
    if(cfsr & 0x00020000) IRQerrorLog("Invalid EPSR usage\r\n");
    if(cfsr & 0x00010000) IRQerrorLog("Undefined instruction\r\n");
    #endif //WITH_ERRLOG
    waitConsoleAndReboot();
}

void __attribute__((naked)) DebugMon_Handler()
{
    saveContext();
    //Call DebugMon_impl(). Name is a C++ mangled name.
    asm volatile("bl _Z13DebugMon_implv");
    restoreContext();
}

void __attribute__((noinline)) DebugMon_impl()
{
    #ifdef WITH_PROCESSES
    if(miosix::Thread::IRQreportFault(miosix_private::FaultData(
        DEBUGMON,getProgramCounter()))) return;
    #endif //WITH_PROCESSES
    #ifdef WITH_ERRLOG
    IRQerrorLog("\r\n***Unexpected DebugMon @ ");
    printUnsignedInt(getProgramCounter());
    #endif //WITH_ERRLOG
    waitConsoleAndReboot();
}

void PendSV_Handler()
{
    #ifdef WITH_ERRLOG
    IRQerrorLog("\r\n***Unexpected PendSV @ ");
    printUnsignedInt(getProgramCounter());
    #endif //WITH_ERRLOG
    waitConsoleAndReboot();
}

void unexpectedInterrupt()
{
    IRQerrorLog("\r\n***Unexpected Peripheral interrupt\r\n");
    waitConsoleAndReboot();
}