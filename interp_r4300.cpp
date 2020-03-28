#include <string>
#include <cstdlib>
#include "types.h"

extern regs cpu;

#define FPU_PARTS  int ft = 32+((opcode>>16)&0x1F); int fs = 32+((opcode>>11)&0x1F); int fd = 32+((opcode>>6)&0x1F); int mop = opcode&0x3F
#define SPECIAL_PARTS int rs = (opcode>>21)&0x1F; int rt = (opcode>>16)&0x1F; int rd = (opcode>>11)&0x1F
#define OPCODE_PARTS  int rs = (opcode>>21)&0x1F; int rt = (opcode>>16)&0x1F; u16 offset = (opcode&0xFFFF)
#define REGIMM_PARTS  int rs = (opcode>>21)&0x1F; u16 offset = (opcode&0xFFFF);
#define SA_PART ((opcode>>6)&0x1F)

#define se32to64(a)  ((s64)(s32)(a))
#define se32to64u(a) ((u64)(s64)(s32)(a))
#define se16to32(a)  ((s32)(s16)(a))
#define se16to32u(a) ((u32)(s32)(s16)(a))
#define se8to64(a)   ((s64)(s8) (a))
#define se16to64(a)  ((s64)(s16)(a))
#define ze16to64(a)  ((u64)(u16)(a))
#define  ze8to64(a)  ((u64)(u8) (a))

typedef void(*interpptr)(u32);
void interp_op(u32 opcode);
u32 read32(u32);
u16 read16(u32);
u8  read8(u32);
u64 read64(u32);
void write8(u32, u8);
void write16(u32, u16);
void write32(u32, u32);
void write64(u32, u64);

int branch_delay = 0;
u32 branch_target = 0;

void undef_opcode(u32 opcode)
{
	return;
}

void interp_trap(u32 opcode)
{
	return undef_opcode(opcode);
}

void interp_bne(u32 opcode)
{
	OPCODE_PARTS;
	if( cpu.R[rs] != cpu.R[rt] )
	{
		branch_target = (cpu.PC+4) + (se16to32(offset)<<2);
		branch_delay = 2;
	}
	return;
}

void interp_bnel(u32 opcode)
{
	OPCODE_PARTS;
	if( cpu.R[rs] != cpu.R[rt] )
	{
		branch_target = (cpu.PC+4) + (se16to32(offset)<<2);
		branch_delay = 2;
	} else {
		cpu.PC += 4;
	}
	return;
}

void interp_bltzl(u32 opcode)
{
	REGIMM_PARTS;
	if( (cpu.R[rs] >> 63) )
	{
		branch_target = (cpu.PC+4) + (se16to32(offset)<<2);
		branch_delay = 2;
	} else {
		cpu.PC += 4;
	}
	return;
}

void interp_bltzal(u32 opcode)
{
	REGIMM_PARTS;
	cpu.R[31] = cpu.PC + 8;
	if( (cpu.R[rs] >> 63) )
	{
		branch_target = (cpu.PC+4) + (se16to32(offset)<<2);
		branch_delay = 2;
	}
	return;
}

static bool ignore_once = false;

void interp_bgezal(u32 opcode)
{
	REGIMM_PARTS;
	cpu.R[31] = cpu.PC + 8;
	if( !(cpu.R[rs] >> 63) )
	{
		branch_target = (cpu.PC+4) + (se16to32(offset)<<2);
		branch_delay = 2;
	}
	return;
}

void interp_bltzall(u32 opcode)
{
	REGIMM_PARTS;
	cpu.R[31] = cpu.PC + 8;
	if( (cpu.R[rs] >> 63) )
	{
		branch_target = (cpu.PC+4) + (se16to32(offset)<<2);
		branch_delay = 2;
	} else {
		cpu.PC += 4;
	}
	return;
}

void interp_bgezall(u32 opcode)
{
	REGIMM_PARTS;
	cpu.R[31] = cpu.PC + 8;
	if( !(cpu.R[rs] >> 63) )
	{
		branch_target = (cpu.PC+4) + (se16to32(offset)<<2);
		branch_delay = 2;
	} else {
		cpu.PC += 4;
	}
	return;
}

void interp_blezl(u32 opcode)
{
	OPCODE_PARTS;
	if( (cpu.R[rs] >> 63) || (cpu.R[rs] == 0) )
	{
		branch_target = (cpu.PC+4) + (se16to32(offset)<<2);
		branch_delay = 2;
	} else {
		cpu.PC += 4;
	}
	return;
}

void interp_bgtzl(u32 opcode)
{
	OPCODE_PARTS;
	if( (cpu.R[rs]!=0) && !(cpu.R[rs] >> 63) )
	{
		branch_target = (cpu.PC+4) + (se16to32(offset)<<2);
		branch_delay = 2;
	} else {
		cpu.PC += 4;
	}
	return;
}

void interp_bgezl(u32 opcode)
{
	REGIMM_PARTS;
	if( !(cpu.R[rs] >> 63) )
	{
		branch_target = (cpu.PC+4) + (se16to32(offset)<<2);
		branch_delay = 2;
	} else {
		cpu.PC += 4;
	}
	return;
}

void interp_beql(u32 opcode)
{
	OPCODE_PARTS;
	if( cpu.R[rs] == cpu.R[rt] )
	{
		branch_target = (cpu.PC+4) + (se16to32(offset)<<2);
		branch_delay = 2;
	} else {
		cpu.PC += 4;
	}
	return;
}

void interp_beq(u32 opcode)
{
	OPCODE_PARTS;
	if( cpu.R[rs] == cpu.R[rt] )
	{
		branch_target = (cpu.PC+4) + (se16to32(offset)<<2);
		branch_delay = 2;
	}
	return;
}

void interp_blez(u32 opcode)
{
	OPCODE_PARTS;
	if( (cpu.R[rs] >> 63) || (cpu.R[rs] == 0) )
	{
		branch_target = (cpu.PC+4) + (se16to32(offset)<<2);
		branch_delay = 2;
	}
	return;
}

void interp_bltz(u32 opcode)
{
	REGIMM_PARTS;
	if( (cpu.R[rs] >> 63) )
	{
		branch_target = (cpu.PC+4) + (se16to32(offset)<<2);
		branch_delay = 2;
	}
	return;
}

void interp_bgez(u32 opcode)
{
	REGIMM_PARTS;
	if( !(cpu.R[rs] >> 63) )
	{
		branch_target = (cpu.PC+4) + (se16to32(offset)<<2);
		branch_delay = 2;
	}
	return;
}

void interp_bgtz(u32 opcode)
{
	OPCODE_PARTS;
	if( (cpu.R[rs]!=0) && !(cpu.R[rs] >> 63) )
	{
		branch_target = (cpu.PC+4) + (se16to32(offset)<<2);
		branch_delay = 2;
	}
	return;
}

interpptr regimm_ops[] = { 
	interp_bltz, interp_bgez, interp_bltzl, interp_bgezl, undef_opcode, undef_opcode, undef_opcode, undef_opcode,
	interp_trap, interp_trap, interp_trap, interp_trap, interp_trap, undef_opcode, interp_trap, undef_opcode,
	interp_bltzal, interp_bgezal, interp_bltzall, interp_bgezall, undef_opcode, undef_opcode, undef_opcode, undef_opcode,
	undef_opcode, undef_opcode, undef_opcode, undef_opcode, undef_opcode, undef_opcode, undef_opcode, undef_opcode
};

void interp_regimm(u32 opcode)
{
	regimm_ops[(opcode>>16)&0x1F](opcode);
	return;
}

void interp_cache(u32 opcode)
{
	// cache not implemented
	return;
}

void interp_sync(u32 opcode)
{
	//unimplemented
	return;
}

void interp_ll(u32 opcode)
{
	//unimplemented
	return;
}

void interp_lld(u32 opcode)
{
	//unimplemented
	return;
}

void interp_sc(u32 opcode)
{
	//unimplemented
	return;
}

void interp_break(u32 opcode)
{
	//todo
	return;
}

void interp_syscall(u32 opcode)
{
	//todo
	return;
}

void interp_j(u32 opcode)
{
	branch_target = ((opcode<<2)&0x0FFFFFFF) | ((cpu.PC+4)&0xF0000000);
	branch_delay = 2;
	return;
}

void interp_jal(u32 opcode)
{
	cpu.R[31] = cpu.PC + 8;
	branch_target = ((opcode<<2)&0x0FFFFFFF) | ((cpu.PC+4)&0xF0000000);
	branch_delay = 2;
	return;
}

void interp_jalr(u32 opcode)
{
	SPECIAL_PARTS;
	if( rd ) cpu.R[rd] = cpu.PC + 8;
	branch_target = cpu.R[rs];
	branch_delay = 2;
	return;
}

void interp_jr(u32 opcode)
{
	SPECIAL_PARTS;
	branch_target = cpu.R[rs];
	branch_delay = 2;
	return;
}

void interp_and(u32 opcode)
{
	SPECIAL_PARTS;
	if( rd ) cpu.R[rd] = (cpu.R[rs] & cpu.R[rt]);
	return;
}

void interp_or(u32 opcode)
{
	SPECIAL_PARTS;
	if( rd ) cpu.R[rd] = (cpu.R[rs] | cpu.R[rt]);
	return;
}

void interp_xor(u32 opcode)
{
	SPECIAL_PARTS;
	if( rd ) cpu.R[rd] = (cpu.R[rs] ^ cpu.R[rt]);
	return;
}

void interp_nor(u32 opcode)
{
	SPECIAL_PARTS;
	if( rd ) cpu.R[rd] = ~(cpu.R[rs] | cpu.R[rt]);
	return;
}

void interp_addu(u32 opcode)
{
	SPECIAL_PARTS;
	if( rd ) cpu.R[rd] = se32to64( (u32)cpu.R[rs] + (u32)cpu.R[rt] );
	return;
}

void interp_subu(u32 opcode)
{
	SPECIAL_PARTS;
	if( rd ) cpu.R[rd] = se32to64( (u32)cpu.R[rs] - (u32)cpu.R[rt] );
	return;
}

void interp_dmult(u32 opcode)
{
	SPECIAL_PARTS;
	return;
}

void interp_dmultu(u32 opcode)
{
	SPECIAL_PARTS;
	return;
}

void interp_daddu(u32 opcode)
{
	SPECIAL_PARTS;
	if( rd ) cpu.R[rd] = cpu.R[rs] + cpu.R[rt];
	return;
}

void interp_dsubu(u32 opcode)
{
	SPECIAL_PARTS;
	if( rd ) cpu.R[rd] = cpu.R[rs] - cpu.R[rt];
	return;
}

void interp_lui(u32 opcode)
{
	OPCODE_PARTS;
	if( rt ) cpu.R[rt] = se32to64( (offset<<16) );
	return;
}

void interp_andi(u32 opcode)
{
	OPCODE_PARTS;
	if( rt ) cpu.R[rt] = cpu.R[rs] & offset;
	return;
}

void interp_ori(u32 opcode)
{
	OPCODE_PARTS;
	if( rt ) cpu.R[rt] = cpu.R[rs] | offset;
	return;
}

void interp_xori(u32 opcode)
{
	OPCODE_PARTS;
	if( rt ) cpu.R[rt] = cpu.R[rs] ^ offset;
	return;
}

void interp_srlv(u32 opcode)
{
	SPECIAL_PARTS;
	if( rd ) cpu.R[rd] = se32to64(  (u32)cpu.R[rt] >> (cpu.R[rs]&0x1F)  );
	return;
}

void interp_slti(u32 opcode)
{
	OPCODE_PARTS;
	if( rt ) cpu.R[rt] = (s32)cpu.R[rs] < se16to32(offset);
	return;
}

void interp_sltiu(u32 opcode)
{
	OPCODE_PARTS;
	if( rt ) cpu.R[rt] = (u32)cpu.R[rs] < se16to32u(offset);
	return;
}

void interp_slt(u32 opcode)
{
	SPECIAL_PARTS;
	if( rd ) cpu.R[rd] = (s64)cpu.R[rs] < (s64)cpu.R[rt];
	return;
}

void interp_sltu(u32 opcode)
{
	SPECIAL_PARTS;
	if( rd ) cpu.R[rd] = cpu.R[rs] < cpu.R[rt];
	return;
}

void interp_sllv(u32 opcode)
{
	SPECIAL_PARTS;
	if( rd ) cpu.R[rd] = se32to64(  (u32)cpu.R[rt] << (cpu.R[rs]&0x1F)  );
	return;
}

void interp_srav(u32 opcode)
{
	SPECIAL_PARTS;
	if( rd ) cpu.R[rd] = se32to64(  (s32)cpu.R[rt] >> (cpu.R[rs]&0x1F)  );
	return;
}

void interp_srl(u32 opcode)
{
	SPECIAL_PARTS;
	if( rd ) cpu.R[rd] = se32to64(  (u32)cpu.R[rt] >> SA_PART   );
	return;
}

void interp_sll(u32 opcode)
{
	SPECIAL_PARTS;
	if( rd ) cpu.R[rd] = se32to64(  (u32)cpu.R[rt] << SA_PART   );
	return;
}

void interp_dsllv(u32 opcode)
{
	SPECIAL_PARTS;
	if( rd ) cpu.R[rd] = cpu.R[rt] << (cpu.R[rs]&0x3F);
	return;
}

void interp_dsrlv(u32 opcode)
{
	SPECIAL_PARTS;
	if( rd ) cpu.R[rd] = cpu.R[rt] >> (cpu.R[rs]&0x3F);
	return;
}

void interp_dsrav(u32 opcode)
{
	SPECIAL_PARTS;
	if( rd ) cpu.R[rd] = (s64)cpu.R[rt] >> (cpu.R[rs]&0x3F);
	return;
}

void interp_dsll(u32 opcode)
{
	SPECIAL_PARTS;
	if( rd ) cpu.R[rd] = cpu.R[rt] << ((opcode>>6)&0x1F);
	return;
}

void interp_dsll32(u32 opcode)
{
	SPECIAL_PARTS;
	if( rd ) cpu.R[rd] = cpu.R[rt] << (32 + SA_PART);
	return;
}

void interp_sra(u32 opcode)
{
	SPECIAL_PARTS;
	if( rd ) cpu.R[rd] = se32to64(  (s32)cpu.R[rt] >> SA_PART   );
	return;
}

void interp_dsra(u32 opcode)
{
	SPECIAL_PARTS;
	if( rd ) cpu.R[rd] = (s64)cpu.R[rt] >> SA_PART;
	return;
}

void interp_dsrl(u32 opcode)
{
	SPECIAL_PARTS;
	if( rd ) cpu.R[rd] = cpu.R[rt] >> SA_PART;
	return;
}

void interp_dsrl32(u32 opcode)
{
	SPECIAL_PARTS;
	if( rd ) cpu.R[rd] = cpu.R[rt] >> (32 + SA_PART);
	return;
}

void interp_dsra32(u32 opcode)
{
	SPECIAL_PARTS;
	if( rd ) cpu.R[rd] = (s64)cpu.R[rt] >> (32 + SA_PART);
	return;
}

void interp_mfhi(u32 opcode)
{
	SPECIAL_PARTS;
	if( rd ) cpu.R[rd] = cpu.HI;
	return;
}

void interp_mflo(u32 opcode)
{
	SPECIAL_PARTS;
	if( rd ) cpu.R[rd] = cpu.LO;
	return;
}

void interp_mthi(u32 opcode)
{
	SPECIAL_PARTS;
	cpu.LO = cpu.R[rs];
	return;
}

void interp_mtlo(u32 opcode)
{
	SPECIAL_PARTS;
	cpu.LO = cpu.R[rs];
	return;
}

void interp_lb(u32 opcode)
{
	OPCODE_PARTS;
	u32 addr = se16to32(offset) + (u32)cpu.R[rs];
	//todo: tlb
	cpu.R[rt] = se8to64( read8(addr) );
	return;
}

void interp_lbu(u32 opcode)
{
	OPCODE_PARTS;
	u32 addr = se16to32(offset) + (u32)cpu.R[rs];
	//todo: tlb
	cpu.R[rt] = ze8to64( read8(addr) );
	return;
}

void interp_lh(u32 opcode)
{
	OPCODE_PARTS;
	u32 addr = se16to32(offset) + (u32)cpu.R[rs];
	//todo: tlb
	cpu.R[rt] = se16to64( read16(addr&~1) );
	return;
}

void interp_lhu(u32 opcode)
{
	OPCODE_PARTS;
	u32 addr = se16to32(offset) + (u32)cpu.R[rs];
	//todo: tlb
	cpu.R[rt] = ze16to64( read16(addr&~1) );
	return;
}

void interp_lw(u32 opcode)
{
	OPCODE_PARTS;
	u32 addr = se16to32(offset) + (u32)cpu.R[rs];
	//todo: tlb
	cpu.R[rt] = se32to64( read32(addr&~3) );
	return;
}

void interp_ld(u32 opcode)
{
	OPCODE_PARTS;
	u32 addr = se16to32(offset) + (u32)cpu.R[rs];
	//todo: tlb
	cpu.R[rt] = read64(addr&~7);
	return;
}

void interp_sb(u32 opcode)
{
	OPCODE_PARTS;
	u32 addr = se16to32(offset) + (u32)cpu.R[rs];
	//todo: tlb
	write8(addr, cpu.R[rt]);
	return;
}

void interp_sh(u32 opcode)
{
	OPCODE_PARTS;
	u32 addr = se16to32(offset) + (u32)cpu.R[rs];
	//todo: tlb
	write16(addr&~1, cpu.R[rt]);
	return;
}

void interp_sw(u32 opcode)
{
	OPCODE_PARTS;
	u32 addr = se16to32(offset) + (u32)cpu.R[rs];
	//todo: tlb
	write32(addr&~3, cpu.R[rt]);
	return;
}

void interp_sd(u32 opcode)
{
	OPCODE_PARTS;
	u32 addr = se16to32(offset) + (u32)cpu.R[rs];
	//todo: tlb
	write64(addr&~7, cpu.R[rt]);
	return;
}

void interp_lwu(u32 opcode)
{
	OPCODE_PARTS;
	u32 addr = se16to32(offset) + (u32)cpu.R[rs];
	//todo: tlb
	if( rt ) cpu.R[rt] = (u64)(u32) read32(addr&~3);
	return;
}

void interp_daddiu(u32 opcode)
{
	OPCODE_PARTS;
	if( rt ) cpu.R[rt] = cpu.R[rs] + se16to64(offset);
	return;
}

void interp_addiu(u32 opcode)
{
	OPCODE_PARTS;
	if( rt ) cpu.R[rt] = se32to64( (u32)cpu.R[rs] + se16to32(offset) );
	return;
}

void interp_mult(u32 opcode)
{
	SPECIAL_PARTS;
	s64 a = se32to64(cpu.R[rs]) * se32to64(cpu.R[rt]);
	cpu.HI = se32to64(a>>32);
	cpu.LO = se32to64(a&0xffffffff);
	return;
}

void interp_div(u32 opcode)
{
	SPECIAL_PARTS;
	s64 a = (s64)(s32)cpu.R[rs];
	s64 b = (s64)(s32)cpu.R[rt];

	if( b == 0 )
	{
		if( cpu.R[rs]&BIT(31) )
		{
			cpu.LO = 1;
		} else {
			cpu.LO = -1LL;
		}
		cpu.HI = cpu.R[rs];
	} else {
		cpu.LO = ( a/b );
		cpu.HI = ( a%b );
	}
	return;
}

void interp_divu(u32 opcode)
{
	SPECIAL_PARTS;
	if( cpu.R[rt] == 0 )
	{
		cpu.LO = -1LL;
		cpu.HI = cpu.R[rs];
	} else {
		cpu.LO = se32to64((u32)cpu.R[rs] / (u32)cpu.R[rt]);
		cpu.HI = se32to64((u32)cpu.R[rs] % (u32)cpu.R[rt]);
	}
	return;
}

void interp_ddiv(u32 opcode)
{
	SPECIAL_PARTS;
	if( cpu.R[rt] == 0 )
	{
		if( cpu.R[rs]&BIT(31) )
		{
			cpu.LO = 1;
		} else {
			cpu.LO = -1LL;
		}
		cpu.HI = cpu.R[rs];
	} else {
		cpu.LO = (s64)cpu.R[rs] / (s64)cpu.R[rt];
		cpu.HI = (s64)cpu.R[rs] % (s64)cpu.R[rt];
	}
	return;
}

void interp_ddivu(u32 opcode)
{
	SPECIAL_PARTS;
	if( cpu.R[rt] == 0 )
	{
		if( cpu.R[rs]&BIT(31) )
		{
			cpu.LO = 1;
		} else {
			cpu.LO = -1LL;
		}
		cpu.HI = cpu.R[rs];
	} else {
		cpu.LO = cpu.R[rs] / cpu.R[rt];
		cpu.HI = cpu.R[rs] % cpu.R[rt];
	}
	return;
}

void interp_multu(u32 opcode)
{
	SPECIAL_PARTS;
	u64 a = se32to64u(cpu.R[rs]) * se32to64u(cpu.R[rt]);
	cpu.HI = se32to64(a>>32);
	cpu.LO = se32to64(a&0xffffffff);
	return;
}

void interp_mfc0(u32 opcode)
{
	SPECIAL_PARTS;  // not actually SPECIAL, but uses rt and rd that match that opcode format
	if( rt ) cpu.R[rt] = cpu.C[rd];
	return;
}

void interp_mtc0(u32 opcode)
{
	SPECIAL_PARTS;  // not actually SPECIAL, but uses rt and rd that match that opcode format
	cpu.C[rd] = (u64)(u32) cpu.R[rt];
	return;
}

void interp_dmfc0(u32 opcode)
{
	return;
}

void interp_dmtc0(u32 opcode)
{
	return;
}

void interp_cfc0(u32 opcode)
{
	return;
}

void interp_ctc0(u32 opcode)
{
	return;
}

void interp_eret(u32 opcode)
{
	if( cpu.C[CP0_Status] & BIT(2) )
	{
		cpu.PC = cpu.C[CP0_ErrorEPC];
		cpu.C[CP0_Status] &= ~BIT(2);
	} else {
		cpu.PC = cpu.C[CP0_EPC];
		cpu.C[CP0_Status] &= ~BIT(1);
	}
	return;
}

void interp_mfc1(u32 opcode)
{
	SPECIAL_PARTS;
	return;
}

void interp_mtc1(u32 opcode)
{
	SPECIAL_PARTS;
	return;
}

void interp_dmfc1(u32 opcode)
{
	SPECIAL_PARTS;
	return;
}

void interp_dmtc1(u32 opcode)
{
	SPECIAL_PARTS;
	return;
}

void interp_ctc1(u32 opcode)
{
	SPECIAL_PARTS;
	return;
}

void interp_cfc1(u32 opcode)
{
	SPECIAL_PARTS;
	return;
}

void interp_cop1S(u32 opcode)
{
	FPU_PARTS;
	switch( mop )
	{
	case 0: //add.s
		break;
	case 1: //sub.s
		break;
	case 2: //mul.s
		break;
	case 3: //div.s
		break;
	case 4: //sqrt.s
		break;
	case 5: //abs.s
		break;
	case 6: //mov.s
		break;
	case 7: //neg.s
		break;
	case 8: //round.l.s
		break;
	case 9: //trunc.l.s
		break;
	case 10: //ceil.l.s
		break;
	case 11: //floor.l.s
		break;
	case 12: //round.w.s
		break;
	case 13: //trunc.w.s
		break;
	case 14: //ceil.w.s
		break;
	case 15: //floor.w.s
		break;
	case 33: //cvt.d.s
		break;
	case 36: //cvt.w.s
		break;
	case 37: //cvt.l.s
		break;
	case 48: //c.f.d
		break;
	case 49: //c.un.d
		break;
	case 50: //c.eq.d
		break;
	case 51: //c.ueq.d
		break;
	case 52: //c.olt.d
		break;
	case 53: //c.ult.d
		break;
	case 54: //c.ole.d
		break;
	case 55: //c.ule.d
		break;
	case 56: //c.sf.d
		break;
	case 57: //c.ngle.d
		break;
	case 58: //c.seq.d
		break;
	case 59: //c.ngl.d
		break;
	case 60: //c.lt.d
	case 61: //c.nge.d
		break;
	case 62: //c.le.d
	case 63: //c.ngt.d
		break;

	}
	return;
}

void interp_cop1D(u32 opcode)
{
	FPU_PARTS;
	switch( mop )
	{
	case 0: //add.d
		break;
	case 1: //sub.d
		break;
	case 2: //mul.d
		break;
	case 3: //div.d
		break;
	case 4: //sqrt.d
		break;
	case 5: //abs.d
		break;
	case 6: //mov.d
		break;
	case 7: //neg.d
		break;
	case 8: //round.l.d
		break;
	case 9: //trunc.l.d
		break;
	case 10: //ceil.l.d
		break;
	case 11: //floor.l.d
		break;
	case 12: //round.w.d //what happens to upper 32bits of things?
		break;
	case 13: //trunc.w.d
		break;
	case 14: //ceil.w.d
		break;
	case 15: //floor.w.d
		break;
	case 32: //cvt.s.d
		break;
	case 36: //cvt.w.d
		break;
	case 37: //cvt.l.d
		break;
	case 48: //c.f.d
		break;
	case 49: //c.un.d
		break;
	case 50: //c.eq.d
		break;
	case 51: //c.ueq.d
		break;
	case 52: //c.olt.d
		break;
	case 53: //c.ult.d
		break;
	case 54: //c.ole.d
		break;
	case 55: //c.ule.d
		break;
	case 56: //c.sf.d
		break;
	case 57: //c.ngle.d
		break;
	case 58: //c.seq.d
		break;
	case 59: //c.ngl.d
		break;
	case 60: //c.lt.d
	case 61: //c.nge.d
		break;
	case 62: //c.le.d
	case 63: //c.ngt.d
		break;

	}
	return;
}

void interp_cop1W(u32 opcode)
{
	FPU_PARTS;
	switch( mop )
	{
	case 32: //cvt.s.w
		break;
	case 33: //cvt.d.w
		break;
	}
	return;
}

void interp_cop1L(u32 opcode)
{
	FPU_PARTS;
	switch( mop )
	{
	case 32: //cvt.s.l
		break;
	case 33: //cvt.d.l
		break;
	}
	return;
}

void interp_bc1(u32 opcode)
{
	u32 subop = (opcode>>16)&0x1F;
	s32 offset =(s32) (s16)(opcode&0xffff);
	offset <<= 2;

	switch( subop )
	{
	case 0: //BC1F branch on cop1 false
		break;
	case 1: //BC1T
		break;
	case 2: //BC1FL branch on cop1 false likely
		break;
	case 3: //BC1TL
		break;
	}

	return;
}

interpptr special_ops[] = { interp_sll, undef_opcode, interp_srl, interp_sra, interp_sllv, undef_opcode, interp_srlv, interp_srav,
	interp_jr, interp_jalr, undef_opcode, undef_opcode, interp_syscall, interp_break, undef_opcode, interp_sync,
	interp_mfhi, interp_mthi, interp_mflo, interp_mtlo, interp_dsllv, undef_opcode, interp_dsrlv, interp_dsrav,
	interp_mult, interp_multu, interp_div, interp_divu, interp_dmult, interp_dmultu, interp_ddiv, interp_ddivu,
	interp_addu, interp_addu, interp_subu, interp_subu, interp_and, interp_or, interp_xor, interp_nor, 
	undef_opcode, undef_opcode, interp_slt, interp_sltu, interp_daddu, interp_daddu, interp_dsubu, interp_dsubu,
	interp_trap, interp_trap, interp_trap, interp_trap, interp_trap, undef_opcode, interp_trap, undef_opcode,
	interp_dsll, undef_opcode, interp_dsrl, interp_dsra, interp_dsll32, undef_opcode, interp_dsrl32, interp_dsra32
};

interpptr cop0_ops[] = { interp_mfc0, interp_dmfc0, interp_cfc0, undef_opcode, interp_mtc0, interp_dmtc0, interp_ctc0, undef_opcode,
	undef_opcode /* bcc0? */, undef_opcode,  undef_opcode,  undef_opcode,  undef_opcode,  undef_opcode,  undef_opcode,  undef_opcode, 
};

interpptr cop1_ops[] = { interp_mfc1, interp_dmfc1, interp_cfc1, undef_opcode, interp_mtc1, interp_dmtc1, interp_ctc1, undef_opcode,
	interp_bc1, undef_opcode, undef_opcode, undef_opcode, undef_opcode, undef_opcode, undef_opcode, undef_opcode,
	interp_cop1S, interp_cop1D, undef_opcode, undef_opcode, interp_cop1W, interp_cop1L, undef_opcode, undef_opcode, 
	undef_opcode, undef_opcode, undef_opcode, undef_opcode, undef_opcode, undef_opcode, undef_opcode, undef_opcode
};

void interp_cop0(u32 opcode)
{
	if( (opcode>>25) & 1 )
	{
		switch( opcode&0x3F )
		{
		//todo: tlb
		case 0x18: return interp_eret(opcode);
		default: break;
		}
		return;
	}

	cop0_ops[(opcode>>21)&0x1F](opcode);
	return;
}

void interp_cop1(u32 opcode)
{
	cop1_ops[(opcode>>21)&0x1F](opcode);
	return;
}

void interp_cop2(u32 opcode)
{
	return;
}

void interp_ldc1(u32 opcode)
{
	OPCODE_PARTS;
	return;
}

void interp_lwc1(u32 opcode)
{
	OPCODE_PARTS;
	return;
}

void interp_swc1(u32 opcode)
{
	OPCODE_PARTS;
	return;
}

void interp_sdc1(u32 opcode)
{
	OPCODE_PARTS;
	return;
}

void interp_swc2(u32 opcode)
{
	return;
}

void interp_lwc2(u32 opcode)
{
	return;
}

void interp_ldc2(u32 opcode)
{
	return;
}


void interp_ldl(u32 opcode)
{
	OPCODE_PARTS;
	u32 addr = cpu.R[rs] + se16to32(offset);
	//todo: tlb
	if( ! rt ) return;
	if( (addr&7) == 0 ) 
	{
		cpu.R[rt] = read64(addr); 
	} else {
		unsigned int shft = (addr&7)<<3;
		unsigned long long mask = (1ULL << shft) - 1;
		cpu.R[rt] = (cpu.R[rt]&mask) | (read64(addr&~7)<<shft);
	}
	return;
}

void interp_ldr(u32 opcode)
{
	OPCODE_PARTS;
	u32 addr = cpu.R[rs] + se16to32(offset);
	//todo: tlb
	if( ! rt ) return;
	if( (addr&7) == 7 ) 
	{
		cpu.R[rt] = read64(addr); 
	} else {
		unsigned int shft = (7 - (addr & 7)) << 3;;
		unsigned long long mask = (1ULL << (((addr&7)+1)<<3)) - 1;;
		cpu.R[rt] = (cpu.R[rt]&~mask) | (read64(addr&~7)>>shft);
	}
	return;
}

void interp_lwl(u32 opcode)
{
	// unaligned loads and stores based on mupen64
	// I had a switch with the masks and shifts, and probably wrong
	// trying to figure out which byte is going to end up where
	// and if the byteswap from the read function affects things
	// no matter how many times I look at various diagrams, the 
	// switch on addr and masks/shifts explicitly does not match
	// what happens with this parameterized (equationized? :P ) stuff
	// also the mips manuals are horrible compared to anything written by Intel
	OPCODE_PARTS;
	u32 addr = cpu.R[rs] + se16to32(offset);
	if( (addr&3) == 0 ) 
	{ 
		cpu.R[rt] = se32to64(read32(addr)); 
	} else {
		u32 shft = (addr&3)<<3;
		u64 mask = (1ULL << shft) - 1;
		cpu.R[rt] = se32to64( (cpu.R[rt]&mask) | (read32(addr&~3)<<shft) );
	}
	return;
}

void interp_lwr(u32 opcode)
{
	OPCODE_PARTS;
	u32 addr = cpu.R[rs] + se16to32(offset);
	if( (addr&3) == 3 ) 
	{ 
		cpu.R[rt] = se32to64(read32(addr&~3)); 
	} else {
		u32 shft = (3 - (addr & 3)) << 3;
		u64 mask = (1ULL << (((addr&3)+1)<<3)) - 1;
		cpu.R[rt] = se32to64( (cpu.R[rt]&~mask) | (read32(addr&~3)>>shft) );
	}
	return;
}

void interp_swl(u32 opcode)
{
	OPCODE_PARTS;
	u32 addr = cpu.R[rs] + se16to32(offset);
	//todo: tlb
	if( (addr&3) == 0 ) 
	{
		write32(addr, cpu.R[rt]);
	} else {
		u32 mask = (1ULL << ((4 - (addr & 3)) * 8)) - 1;
		int shift = (addr&3) * 8;
		write32(addr, (read32(addr&~3)&~mask) | (((u32)cpu.R[rt])>>shift) );
	}
	return;
}

void interp_swr(u32 opcode)
{
	OPCODE_PARTS;
	u32 addr = cpu.R[rs] + se16to32(offset);
	//todo: tlb
	if( (addr&3) == 3 )
	{
		write32(addr, cpu.R[rt]);
	} else {
		int shft = (3-(addr&3))<<3;
		u32 mask = (1 << shft) - 1;
		write32(addr, ((read32(addr&~3)&mask) | (cpu.R[rt]<<shft)));
	}
	return;
}

void interp_sdl(u32 opcode)
{
	OPCODE_PARTS;
	u32 addr = cpu.R[rs] + se16to32(offset);
	//todo: tlb
	if( (addr&7) == 0 )
	{
		write64(addr, cpu.R[rt]);
	} else {
		u64 mask = (1ULL << ((8 - (addr&7))*8)) - 1;
		int shift = (addr&7)*8;
		write64(addr, (read64(addr&~7)&~mask) | (cpu.R[rt]>>shift));
	}
	return;
}

void interp_sdr(u32 opcode)
{
	OPCODE_PARTS;
	u32 addr = cpu.R[rs] + se16to32(offset);
	//todo: tlb
	if( (addr&7) == 7 )
	{
		write64(addr, cpu.R[rt]);
	} else {
		int shift = (7 - (addr & 7)) * 8;
		u64 mask = (1ULL << shift) - 1;
		write64(addr, (read64(addr&~7) & mask) | (cpu.R[rt] << shift));
	}
	return;
}

void interp_sdc2(u32 opcode)
{
	return;
}

void interp_scd(u32 opcode)
{
	return;
}

void interp_special(u32 opcode)
{
	special_ops[opcode & 0x3F](opcode);
	return;
}

interpptr opcodes[] = { interp_special, interp_regimm, interp_j, interp_jal, interp_beq, interp_bne, interp_blez, interp_bgtz,
	interp_addiu, interp_addiu, interp_slti, interp_sltiu, interp_andi, interp_ori, interp_xori, interp_lui,			
	interp_cop0, interp_cop1, interp_cop2, undef_opcode, interp_beql, interp_bnel, interp_blezl, interp_bgtzl,
	interp_daddiu, interp_daddiu, interp_ldl, interp_ldr, undef_opcode, undef_opcode, undef_opcode, undef_opcode,
	interp_lb, interp_lh, interp_lwl, interp_lw, interp_lbu, interp_lhu, interp_lwr, interp_lwu,
	interp_sb, interp_sh, interp_swl, interp_sw, interp_sdl, interp_sdr, interp_swr, interp_cache,
	interp_ll, interp_lwc1, interp_lwc2, undef_opcode, interp_lld, interp_ldc1, interp_ldc2, interp_ld,
	interp_sc, interp_swc1, interp_swc2, undef_opcode, interp_scd, interp_sdc1, interp_sdc2, interp_sd
};

void interp_op(u32 opcode)
{
	opcodes[(opcode>>26)](opcode);
	return;
}

int interp_cpu_run()
{
	u32 opcode = read32(cpu.PC);
	interp_op(opcode);
	if( branch_delay > 0 )
	{
		branch_delay--;
		if( branch_delay == 0 ) cpu.PC = branch_target;
		else cpu.PC += 4;
	} else {
		cpu.PC += 4;
	}
	return 1;
}





