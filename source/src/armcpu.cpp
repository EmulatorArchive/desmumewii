/*  Copyright (C) 2006 yopyop
	Copyright (C) 2009-2011 DeSmuME team

    This file is part of DeSmuME

    DeSmuME is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    DeSmuME is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with DeSmuME; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <algorithm>
#include "types.h"
#include "arm_instructions.h"
#include "thumb_instructions.h"
#include "cp15.h"
#include "bios.h"
#include "debug.h"
#include "Disassembler.h"
#include "NDSSystem.h"
#include "MMU_timing.h"
#ifdef HAVE_LUA
#include "lua-engine.h"
#endif

template<u32> static u32 armcpu_prefetch();

FORCEINLINE u32 armcpu_prefetch(armcpu_t *armcpu) { 
	if(armcpu->proc_ID==0) return armcpu_prefetch<0>();
	else return armcpu_prefetch<1>();
}

const unsigned char arm_cond_table[16*16] = {
    /* N=0, Z=0, C=0, V=0 */
    0x00,0xFF,0x00,0xFF,0x00,0xFF,0x00,0xFF,
    0x00,0xFF,0xFF,0x00,0xFF,0x00,0xFF,0x20,
    /* N=0, Z=0, C=0, V=1 */
    0x00,0xFF,0x00,0xFF,0x00,0xFF,0xFF,0x00,
    0x00,0xFF,0x00,0xFF,0x00,0xFF,0xFF,0x20,
    /* N=0, Z=0, C=1, V=0 */
    0x00,0xFF,0xFF,0x00,0x00,0xFF,0x00,0xFF,
    0xFF,0x00,0xFF,0x00,0xFF,0x00,0xFF,0x20,
    /* N=0, Z=0, C=1, V=1 */
    0x00,0xFF,0xFF,0x00,0x00,0xFF,0xFF,0x00,
    0xFF,0x00,0x00,0xFF,0x00,0xFF,0xFF,0x20,
    /* N=0, Z=1, C=0, V=0 */
    0xFF,0x00,0x00,0xFF,0x00,0xFF,0x00,0xFF,
    0x00,0xFF,0xFF,0x00,0x00,0xFF,0xFF,0x20,
    /* N=0, Z=1, C=0, V=1 */
    0xFF,0x00,0x00,0xFF,0x00,0xFF,0xFF,0x00,
    0x00,0xFF,0x00,0xFF,0x00,0xFF,0xFF,0x20,
    /* N=0, Z=1, C=1, V=0 */
    0xFF,0x00,0xFF,0x00,0x00,0xFF,0x00,0xFF,
    0x00,0xFF,0xFF,0x00,0x00,0xFF,0xFF,0x20,
    /* N=0, Z=1, C=1, V=1 */
    0xFF,0x00,0xFF,0x00,0x00,0xFF,0xFF,0x00,
    0x00,0xFF,0x00,0xFF,0x00,0xFF,0xFF,0x20,
    /* N=1, Z=0, C=0, V=0 */
    0x00,0xFF,0x00,0xFF,0xFF,0x00,0x00,0xFF,
    0x00,0xFF,0x00,0xFF,0x00,0xFF,0xFF,0x20,
    /* N=1, Z=0, C=0, V=1 */
    0x00,0xFF,0x00,0xFF,0xFF,0x00,0xFF,0x00,
    0x00,0xFF,0xFF,0x00,0xFF,0x00,0xFF,0x20,
    /* N=1, Z=0, C=1, V=0 */
    0x00,0xFF,0xFF,0x00,0xFF,0x00,0x00,0xFF,
    0xFF,0x00,0x00,0xFF,0x00,0xFF,0xFF,0x20,
    /* N=1, Z=0, C=1, V=1 */
    0x00,0xFF,0xFF,0x00,0xFF,0x00,0xFF,0x00,
    0xFF,0x00,0xFF,0x00,0xFF,0x00,0xFF,0x20,
    /* N=1, Z=1, C=0, V=0 */
    0xFF,0x00,0x00,0xFF,0xFF,0x00,0x00,0xFF,
    0x00,0xFF,0x00,0xFF,0x00,0xFF,0xFF,0x20,
    /* N=1, Z=1, C=0, V=1 */
    0xFF,0x00,0x00,0xFF,0xFF,0x00,0xFF,0x00,
    0x00,0xFF,0xFF,0x00,0x00,0xFF,0xFF,0x20,
    /* N=1, Z=1, C=1, V=0 */
    0xFF,0x00,0xFF,0x00,0xFF,0x00,0x00,0xFF,
    0x00,0xFF,0x00,0xFF,0x00,0xFF,0xFF,0x20,
    /* N=1, Z=1, C=1, V=1 */
    0xFF,0x00,0xFF,0x00,0xFF,0x00,0xFF,0x00,
    0x00,0xFF,0xFF,0x00,0x00,0xFF,0xFF,0x20,
};

armcpu_t NDS_ARM7;
armcpu_t NDS_ARM9;

#define SWAP(a, b, c) do      \
	              {       \
                         c=a; \
                         a=b; \
                         b=c; \
		      }       \
                      while(0)

#ifdef GDB_STUB

#define STALLED_CYCLE_COUNT 10

static void
stall_cpu( void *instance) {
  armcpu_t *armcpu = (armcpu_t *)instance;
  printf("UNSTALL\n");
  armcpu->stalled = 1;
}
                      
static void
unstall_cpu( void *instance) {
  armcpu_t *armcpu = (armcpu_t *)instance;
  printf("UNSTALL\n");
  armcpu->stalled = 0;
}

static void
install_post_exec_fn( void *instance,
                      void (*ex_fn)( void *, u32 adr, int thumb),
                      void *fn_data) {
  armcpu_t *armcpu = (armcpu_t *)instance;

  armcpu->post_ex_fn = ex_fn;
  armcpu->post_ex_fn_data = fn_data;
}

static void
remove_post_exec_fn( void *instance) {
  armcpu_t *armcpu = (armcpu_t *)instance;

  armcpu->post_ex_fn = NULL;
}
#endif

#ifdef GDB_STUB
static u32 read_cpu_reg( void *instance, u32 reg_num) {
	armcpu_t *armcpu = (armcpu_t *)instance;
	u32 reg_value = 0;
	
	if ( reg_num <= 14) {
		reg_value = armcpu->R[reg_num];
	}
	else if ( reg_num == 15) {
		reg_value = armcpu->instruct_adr;
	}
	else if ( reg_num == 16) {
		//CPSR
		reg_value = armcpu->CPSR.val;
	}
	
	return reg_value;
}

static void
set_cpu_reg( void *instance, u32 reg_num, u32 value) {
  armcpu_t *armcpu = (armcpu_t *)instance;

  if ( reg_num <= 14) {
    armcpu->R[reg_num] = value;
  }
  else if ( reg_num == 15) {
    armcpu->next_instruction = value;
  }
  else if ( reg_num == 16) {
    /* FIXME: setting the CPSR */
  }
}
#endif

#ifdef GDB_STUB
int armcpu_new( armcpu_t *armcpu, u32 id,
                struct armcpu_memory_iface *mem_if,
                struct armcpu_ctrl_iface **ctrl_iface_ret)
#else
int armcpu_new( armcpu_t *armcpu, u32 id)
#endif
{
	armcpu->proc_ID = id;

#ifdef GDB_STUB
	armcpu->mem_if = mem_if;

	/* populate the control interface */
	armcpu->ctrl_iface.stall = stall_cpu;
	armcpu->ctrl_iface.unstall = unstall_cpu;
	armcpu->ctrl_iface.read_reg = read_cpu_reg;
	armcpu->ctrl_iface.set_reg = set_cpu_reg;
	armcpu->ctrl_iface.install_post_ex_fn = install_post_exec_fn;
	armcpu->ctrl_iface.remove_post_ex_fn = remove_post_exec_fn;
	armcpu->ctrl_iface.data = armcpu;

	*ctrl_iface_ret = &armcpu->ctrl_iface;

	armcpu->post_ex_fn = NULL;
#endif

	armcpu->stalled = 0;

	armcpu_init(armcpu, 0);

	return 0;
} 

//call this whenever CPSR is changed (other than CNVZQ or T flags); interrupts may need to be unleashed
void armcpu_t::changeCPSR()
{
	//but all it does is give them a chance to unleash by forcing an immediate reschedule
	//TODO - we could actually set CPSR through here and look for a change in the I bit
	//that would be a little optimization as well as a safety measure if we prevented setting CPSR directly
	NDS_Reschedule();
}

void armcpu_init(armcpu_t *armcpu, u32 adr)
{
	armcpu->LDTBit = (armcpu->proc_ID==0); //Si ARM9 utiliser le syte v5 pour le load
	armcpu->intVector = 0xFFFF0000 * (armcpu->proc_ID==0);
	armcpu->waitIRQ = FALSE;
	armcpu->halt_IE_and_IF = FALSE;
	armcpu->intrWaitARM_state = 0;

//#ifdef GDB_STUB
//    armcpu->irq_flag = 0;
//#endif

	for(int i = 0; i < 16; ++i)
	{
		armcpu->R[i] = 0;
		if(armcpu->coproc[i]) free(armcpu->coproc[i]);
		armcpu->coproc[i] = NULL;
	}
	
	armcpu->CPSR.val = armcpu->SPSR.val = SYS;
	
	armcpu->R13_usr = armcpu->R14_usr = 0;
	armcpu->R13_svc = armcpu->R14_svc = 0;
	armcpu->R13_abt = armcpu->R14_abt = 0;
	armcpu->R13_und = armcpu->R14_und = 0;
	armcpu->R13_irq = armcpu->R14_irq = 0;
	armcpu->R8_fiq = armcpu->R9_fiq = armcpu->R10_fiq = armcpu->R11_fiq = armcpu->R12_fiq = armcpu->R13_fiq = armcpu->R14_fiq = 0;
	
	armcpu->SPSR_svc.val = armcpu->SPSR_abt.val = armcpu->SPSR_und.val = armcpu->SPSR_irq.val = armcpu->SPSR_fiq.val = 0;

//#ifdef GDB_STUB
//    armcpu->instruct_adr = adr;
//	armcpu->R[15] = adr + 8;
//#else
	//armcpu->R[15] = adr;
//#endif

	armcpu->next_instruction = adr;
	
	// only ARM9 have co-processor
	if (armcpu->proc_ID==0)
		armcpu->coproc[15] = (armcp_t*)armcp15_new(armcpu);

//#ifndef GDB_STUB
	armcpu_prefetch(armcpu);
//#endif
}

u32 armcpu_switchMode(armcpu_t *armcpu, u8 mode)
{
	u32 oldmode = armcpu->CPSR.bits.mode;
	
	switch(oldmode)
	{
		case USR :
		case SYS :
			armcpu->R13_usr = armcpu->R[13];
			armcpu->R14_usr = armcpu->R[14];
			break;
			
		case FIQ :
			{
                                u32 tmp;
				SWAP(armcpu->R[8], armcpu->R8_fiq, tmp);
				SWAP(armcpu->R[9], armcpu->R9_fiq, tmp);
				SWAP(armcpu->R[10], armcpu->R10_fiq, tmp);
				SWAP(armcpu->R[11], armcpu->R11_fiq, tmp);
				SWAP(armcpu->R[12], armcpu->R12_fiq, tmp);
				armcpu->R13_fiq = armcpu->R[13];
				armcpu->R14_fiq = armcpu->R[14];
				armcpu->SPSR_fiq = armcpu->SPSR;
				break;
			}
		case IRQ :
			armcpu->R13_irq = armcpu->R[13];
			armcpu->R14_irq = armcpu->R[14];
			armcpu->SPSR_irq = armcpu->SPSR;
			break;
			
		case SVC :
			armcpu->R13_svc = armcpu->R[13];
			armcpu->R14_svc = armcpu->R[14];
			armcpu->SPSR_svc = armcpu->SPSR;
			break;
		
		case ABT :
			armcpu->R13_abt = armcpu->R[13];
			armcpu->R14_abt = armcpu->R[14];
			armcpu->SPSR_abt = armcpu->SPSR;
			break;
			
		case UND :
			armcpu->R13_und = armcpu->R[13];
			armcpu->R14_und = armcpu->R[14];
			armcpu->SPSR_und = armcpu->SPSR;
			break;
		default :
			break;
		}
		
		switch(mode)
		{
			case USR :
			case SYS :
				armcpu->R[13] = armcpu->R13_usr;
				armcpu->R[14] = armcpu->R14_usr;
				//SPSR = CPSR;
				break;
				
			case FIQ :
				{
					u32 tmp;
					SWAP(armcpu->R[8], armcpu->R8_fiq, tmp);
					SWAP(armcpu->R[9], armcpu->R9_fiq, tmp);
					SWAP(armcpu->R[10], armcpu->R10_fiq, tmp);
					SWAP(armcpu->R[11], armcpu->R11_fiq, tmp);
					SWAP(armcpu->R[12], armcpu->R12_fiq, tmp);
					armcpu->R[13] = armcpu->R13_fiq;
					armcpu->R[14] = armcpu->R14_fiq;
					armcpu->SPSR = armcpu->SPSR_fiq;
					break;
				}
				
			case IRQ :
				armcpu->R[13] = armcpu->R13_irq;
				armcpu->R[14] = armcpu->R14_irq;
				armcpu->SPSR = armcpu->SPSR_irq;
				break;
				
			case SVC :
				armcpu->R[13] = armcpu->R13_svc;
				armcpu->R[14] = armcpu->R14_svc;
				armcpu->SPSR = armcpu->SPSR_svc;
				break;
				
			case ABT :
				armcpu->R[13] = armcpu->R13_abt;
				armcpu->R[14] = armcpu->R14_abt;
				armcpu->SPSR = armcpu->SPSR_abt;
				break;
				
          case UND :
				armcpu->R[13] = armcpu->R13_und;
				armcpu->R[14] = armcpu->R14_und;
				armcpu->SPSR = armcpu->SPSR_und;
				break;
				
				default :
					break;
	}
	
	armcpu->CPSR.bits.mode = mode & 0x1F;
	armcpu->changeCPSR();
	return oldmode;
}

u32 armcpu_Wait4IRQ(armcpu_t *cpu)
{
	cpu->waitIRQ = TRUE;
	cpu->halt_IE_and_IF = TRUE;
	return 1;
}

template<u32 PROCNUM>
FORCEINLINE static u32 armcpu_prefetch()
{
	armcpu_t* const armcpu = &ARMPROC;

	u32 curInstruction = armcpu->next_instruction;

	if(armcpu->CPSR.bits.T == 0)
	{

		curInstruction &= 0xFFFFFFFC; //please don't change this to 0x0FFFFFFC -- the NDS will happily run on 0xF******* addresses all day long
		//please note that we must setup R[15] before reading the instruction since there is a protection
		//which prevents PC > 0x3FFF from reading the bios region
		armcpu->instruct_adr = curInstruction;
		armcpu->next_instruction = curInstruction + 4;
		armcpu->R[15] = curInstruction + 8;
		armcpu->instruction = _MMU_read32<PROCNUM, MMU_AT_CODE>(curInstruction);


		return MMU_codeFetchCycles<PROCNUM,32>(curInstruction);
	}


	curInstruction &= 0xFFFFFFFE; //please don't change this to 0x0FFFFFFE -- the NDS will happily run on 0xF******* addresses all day long
	//please note that we must setup R[15] before reading the instruction since there is a protection
	//which prevents PC > 0x3FFF from reading the bios region
	armcpu->instruct_adr = curInstruction;
	armcpu->next_instruction = curInstruction + 2;
	armcpu->R[15] = curInstruction + 4;
	armcpu->instruction = _MMU_read16<PROCNUM, MMU_AT_CODE>(curInstruction);


	if(PROCNUM==0)
	{
		// arm9 fetches 2 instructions at a time in thumb mode
		if(!(curInstruction == armcpu->instruct_adr + 2 && (curInstruction & 2)))
			return MMU_codeFetchCycles<PROCNUM,32>(curInstruction);
		else
			return 0;
	}

	return MMU_codeFetchCycles<PROCNUM,16>(curInstruction);
}

#if 0 /* not used */
static BOOL FASTCALL test_EQ(Status_Reg CPSR) { return ( CPSR.bits.Z); }
static BOOL FASTCALL test_NE(Status_Reg CPSR) { return (!CPSR.bits.Z); }
static BOOL FASTCALL test_CS(Status_Reg CPSR) { return ( CPSR.bits.C); }
static BOOL FASTCALL test_CC(Status_Reg CPSR) { return (!CPSR.bits.C); }
static BOOL FASTCALL test_MI(Status_Reg CPSR) { return ( CPSR.bits.N); }
static BOOL FASTCALL test_PL(Status_Reg CPSR) { return (!CPSR.bits.N); }
static BOOL FASTCALL test_VS(Status_Reg CPSR) { return ( CPSR.bits.V); }
static BOOL FASTCALL test_VC(Status_Reg CPSR) { return (!CPSR.bits.V); }
static BOOL FASTCALL test_HI(Status_Reg CPSR) { return (CPSR.bits.C) && (!CPSR.bits.Z); }
static BOOL FASTCALL test_LS(Status_Reg CPSR) { return (CPSR.bits.Z) || (!CPSR.bits.C); }
static BOOL FASTCALL test_GE(Status_Reg CPSR) { return (CPSR.bits.N==CPSR.bits.V); }
static BOOL FASTCALL test_LT(Status_Reg CPSR) { return (CPSR.bits.N!=CPSR.bits.V); }
static BOOL FASTCALL test_GT(Status_Reg CPSR) { return (!CPSR.bits.Z) && (CPSR.bits.N==CPSR.bits.V); }
static BOOL FASTCALL test_LE(Status_Reg CPSR) { return ( CPSR.bits.Z) || (CPSR.bits.N!=CPSR.bits.V); }
static BOOL FASTCALL test_AL(Status_Reg CPSR) { return 1; }

static BOOL (FASTCALL* test_conditions[])(Status_Reg CPSR)= {
	test_EQ , test_NE ,
	test_CS , test_CC ,
	test_MI , test_PL ,
	test_VS , test_VC ,
	test_HI , test_LS ,
	test_GE , test_LT ,
	test_GT , test_LE ,
	test_AL
};
#define TEST_COND2(cond, CPSR) \
	(cond<15&&test_conditions[cond](CPSR))
#endif

//TODO - merge with armcpu_irqException?
//http://www.ethernut.de/en/documents/arm-exceptions.html
//http://docs.google.com/viewer?a=v&q=cache:V4ht1YkxprMJ:www.cs.nctu.edu.tw/~wjtsai/EmbeddedSystemDesign/Ch3-1.pdf+arm+exception+handling&hl=en&gl=us&pid=bl&srcid=ADGEEShx9VTHbUhWdDOrTVRzLkcCsVfJiijncNDkkgkrlJkLa7D0LCpO8fQ_hhU3DTcgZh9rcZWWQq4TYhhCovJ625h41M0ZUX3WGasyzWQFxYzDCB-VS6bsUmpoJnRxAc-bdkD0qmsu&sig=AHIEtbR9VHvDOCRmZFQDUVwy53iJDjoSPQ
void armcpu_exception(armcpu_t *cpu, u32 number)
{
	Mode cpumode = USR;
	switch(number)
	{
	case EXCEPTION_RESET: cpumode = SVC; break;
	case EXCEPTION_UNDEFINED_INSTRUCTION: cpumode = UND; break;
	case EXCEPTION_SWI: cpumode = SVC; break;
	case EXCEPTION_PREFETCH_ABORT: cpumode = ABT; break;
	case EXCEPTION_DATA_ABORT: cpumode = ABT; break;
	case EXCEPTION_RESERVED_0x14: emu_halt(); break;
	case EXCEPTION_IRQ: cpumode = IRQ; break;
	case EXCEPTION_FAST_IRQ: cpumode = FIQ; break;
	}

	Status_Reg tmp = cpu->CPSR;
	armcpu_switchMode(cpu, cpumode);				//enter new mode
	cpu->R[14] = cpu->next_instruction;
	cpu->SPSR = tmp;							//save old CPSR as new SPSR
	cpu->CPSR.bits.T = 0;						//handle as ARM32 code
	cpu->CPSR.bits.I = 1;
	cpu->changeCPSR();
	cpu->R[15] = cpu->intVector + number;
	cpu->next_instruction = cpu->R[15];
	printf("armcpu_exception!\n");
	//extern bool dolog;
	//dolog=true;

	//HOW DOES THIS WORTK WITHOUT A PREFETCH, LIKE IRQ BELOW?
	//I REALLY WISH WE DIDNT PREFETCH BEFORE EXECUTING
}

BOOL armcpu_irqException(armcpu_t *armcpu)
{
    Status_Reg tmp;

	if(armcpu->CPSR.bits.I) return FALSE;

      
	tmp = armcpu->CPSR;
	armcpu_switchMode(armcpu, IRQ);


	armcpu->R[14] = armcpu->instruct_adr + 4;

	armcpu->SPSR = tmp;
	armcpu->CPSR.bits.T = 0;
	armcpu->CPSR.bits.I = 1;
	armcpu->next_instruction = armcpu->intVector + 0x18;
	armcpu->waitIRQ = 0;

	//must retain invariant of having next instruction to be executed prefetched
	//(yucky)
	armcpu_prefetch(armcpu);

	return TRUE;
}


/*
u32 TRAPUNDEF(armcpu_t* cpu)
{
	INFO("ARM%c: Undefined instruction: 0x%08X (%s) PC=0x%08X\n", cpu->proc_ID?'7':'9', cpu->instruction, decodeIntruction(false, cpu->instruction), cpu->instruct_adr);

	if (((cpu->intVector != 0) ^ (cpu->proc_ID == ARMCPU_ARM9)))
	{
		armcpu_exception(&NDS_ARM9,0x04);
		return 4;
	}
	else
	{
		emu_halt();
		return 4;
	}
}
//*/

template<int PROCNUM>
u32 armcpu_exec()
{
	// Usually, fetching and executing are processed parallelly.
	// So this function stores the cycles of each process to
	// the variables below, and returns appropriate cycle count.
	u32 cFetch = 0;
	u32 cExecute = 0;

	//this assert is annoying. but sometimes it is handy.
	//assert(ARMPROC.instruct_adr!=0x00000000);

#if 0 //#ifdef GDB_STUB
	if (ARMPROC.stalled) {
		return STALLED_CYCLE_COUNT;
	}

	/* check for interrupts */
	if (ARMPROC.irq_flag) {
		armcpu_irqException(&ARMPROC);
	}

	cFetch = armcpu_prefetch(&ARMPROC);

	if (ARMPROC.stalled) {
		return MMU_fetchExecuteCycles<PROCNUM>(cExecute, cFetch);
	}
#endif

	if(ARMPROC.CPSR.bits.T == 0)
	{
		if(
			CONDITION(ARMPROC.instruction) == 0x0E  //fast path for unconditional instructions
			|| (TEST_COND(CONDITION(ARMPROC.instruction), CODE(ARMPROC.instruction), ARMPROC.CPSR)) //handles any condition
			)
		{
#ifdef HAVE_LUA
			CallRegisteredLuaMemHook(ARMPROC.instruct_adr, 4, ARMPROC.instruction, LUAMEMHOOK_EXEC); // should report even if condition=false?
#endif
			if(PROCNUM==0) {
				#ifdef DEVELOPER
				DEBUG_statistics.instructionHits[0].arm[INSTRUCTION_INDEX(ARMPROC.instruction)]++;
				#endif
				cExecute = arm_instructions_set_0[INSTRUCTION_INDEX(ARMPROC.instruction)](ARMPROC.instruction);
			}
			else {
				#ifdef DEVELOPER
				DEBUG_statistics.instructionHits[1].arm[INSTRUCTION_INDEX(ARMPROC.instruction)]++;
				#endif
				cExecute = arm_instructions_set_1[INSTRUCTION_INDEX(ARMPROC.instruction)](ARMPROC.instruction);
			}
		}
		else
			cExecute = 1; // If condition=false: 1S cycle
#ifdef GDB_STUB
		if ( ARMPROC.post_ex_fn != NULL) {
			/* call the external post execute function */
			ARMPROC.post_ex_fn(ARMPROC.post_ex_fn_data, ARMPROC.instruct_adr, 0);
		}
		ARMPROC.mem_if->prefetch32( ARMPROC.mem_if->data, ARMPROC.next_instruction);
#endif
		cFetch = armcpu_prefetch<PROCNUM>();
		return MMU_fetchExecuteCycles<PROCNUM>(cExecute, cFetch);
	}

#ifdef HAVE_LUA
	CallRegisteredLuaMemHook(ARMPROC.instruct_adr, 2, ARMPROC.instruction, LUAMEMHOOK_EXEC);
#endif
	if(PROCNUM==0)
	{
		#ifdef DEVELOPER
		DEBUG_statistics.instructionHits[0].thumb[ARMPROC.instruction>>6]++;
		#endif
		cExecute = thumb_instructions_set_0[ARMPROC.instruction>>6](ARMPROC.instruction);
	}
	else {
		#ifdef DEVELOPER
		DEBUG_statistics.instructionHits[1].thumb[ARMPROC.instruction>>6]++;
		#endif
		cExecute = thumb_instructions_set_1[ARMPROC.instruction>>6](ARMPROC.instruction);
	}

#ifdef GDB_STUB
	if ( ARMPROC.post_ex_fn != NULL) {
		/* call the external post execute function */
		ARMPROC.post_ex_fn( ARMPROC.post_ex_fn_data, ARMPROC.instruct_adr, 1);
	}
	ARMPROC.mem_if->prefetch32( ARMPROC.mem_if->data, ARMPROC.next_instruction);
#endif
	cFetch = armcpu_prefetch<PROCNUM>();
	return MMU_fetchExecuteCycles<PROCNUM>(cExecute, cFetch);
}

//these templates needed to be instantiated manually
template u32 armcpu_exec<0>();
template u32 armcpu_exec<1>();
