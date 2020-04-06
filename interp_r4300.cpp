#include <string>
#include <cstring>
#include <cstdlib>
#include <cmath>
#include "types.h"

extern regs cpu;

#define FPU_PARTS  int ft = ((opcode>>16)&0x1F); int fs = ((opcode>>11)&0x1F); int fd = ((opcode>>6)&0x1F); int mop = opcode&0x3F
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
	__int128 t128 = (__int128)(s64)cpu.R[rs] * (__int128)(s64)cpu.R[rt];
	cpu.HI = (u64)(t128>>64);
	cpu.LO = (u64)t128;
	return;
}

void interp_dmultu(u32 opcode)
{
	SPECIAL_PARTS;
	__uint128_t t128 = (__uint128_t)cpu.R[rs] * (__uint128_t)cpu.R[rt];
	cpu.HI = (u64)(t128>>64);
	cpu.LO = (u64)t128;
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
	if( rt ) cpu.R[rt] = cpu.R[rs] & ze16to64(offset);
	return;
}

void interp_ori(u32 opcode)
{
	OPCODE_PARTS;
	if( rt ) cpu.R[rt] = cpu.R[rs] | ze16to64(offset);
	return;
}

void interp_xori(u32 opcode)
{
	OPCODE_PARTS;
	if( rt ) cpu.R[rt] = cpu.R[rs] ^ ze16to64(offset);
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
		cpu.LO = -1LL;
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
	if( rd == CP0_Compare )
	{
		cpu.C[CP0_Cause] &= ~BIT(15);
	}
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
	cpu.PC -= 4;
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
		*(float*)(cpu.FR+fd) = *(float*)(cpu.FR+fs) + *(float*)(cpu.FR+ft);
		break;
	case 1: //sub.s
		*(float*)(cpu.FR+fd) = *(float*)(cpu.FR+fs) - *(float*)(cpu.FR+ft);
		break;
	case 2: //mul.s
		*(float*)(cpu.FR+fd) = *(float*)(cpu.FR+fs) * *(float*)(cpu.FR+ft);
		break;
	case 3: //div.s
		*(float*)(cpu.FR+fd) = *(float*)(cpu.FR+fs) / *(float*)(cpu.FR+ft);
		break;
	case 4: //sqrt.s
		*(float*)(cpu.FR+fd) = sqrt( *(float*)(cpu.FR+fs) );
		break;
	case 5: //abs.s
		*(u32*)(cpu.FR+fd) = (*(u32*)(cpu.FR+fs)) & ~BIT(31);
		break;
	case 6: //mov.s
		*(float*)(cpu.FR+fd) = ( *(float*)(cpu.FR+fs) );
		break;
	case 7: //neg.s
		*(float*)(cpu.FR+fd) = -( *(float*)(cpu.FR+fs) );
		break;
	case 8: //round.l.s
		cpu.FR[fd] = llround( *(float*)(cpu.FR+fs) );
		break;
	case 9: //trunc.l.s
		cpu.FR[fd] = trunc( *(float*)(cpu.FR+fs) );
		break;
	case 10: //ceil.l.s
		cpu.FR[fd] = ceil( *(float*)(cpu.FR+fs) );
		break;
	case 11: //floor.l.s
		cpu.FR[fd] = floor( *(float*)(cpu.FR+fs) );
		break;
	case 12: //round.w.s
		cpu.FR[fd] =(int) llround( *(float*)(cpu.FR+fs) );
		break;
	case 13: //trunc.w.s
		cpu.FR[fd] =(int) trunc( *(float*)(cpu.FR+fs) );
		break;
	case 14: //ceil.w.s
		cpu.FR[fd] =(int) ceil( *(float*)(cpu.FR+fs) );
		break;
	case 15: //floor.w.s
		cpu.FR[fd] =(int) floor( *(float*)(cpu.FR+fs) );
		break;
	case 33: //cvt.d.s
		*(double*)(cpu.FR+fd) =(double)  ( *(float*)(cpu.FR+fs) );
		break;
	case 36: //cvt.w.s
		cpu.FR[fd] =(int)  ( *(float*)(cpu.FR+fs) );
		break;
	case 37: //cvt.l.s
		cpu.FR[fd] =(s64)  ( *(float*)(cpu.FR+fs) );
		break;
	case 48: //c.f.d
		break;
	case 49: //c.un.d
		break;
	case 50: //c.eq.d
		cpu.FC31 = (*(float*)(cpu.FR+fs) == *(float*)(cpu.FR+ft))<<23;
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
		cpu.FC31 = (*(float*)(cpu.FR+fs) == *(float*)(cpu.FR+ft))<<23;
		break;
	case 60: //c.lt.d
	case 61: //c.nge.d
		cpu.FC31 = (*(float*)(cpu.FR+fs) < *(float*)(cpu.FR+ft))<<23;
		break;
	case 62: //c.le.d
	case 63: //c.ngt.d
		cpu.FC31 = (*(float*)(cpu.FR+fs) <= *(float*)(cpu.FR+ft))<<23;
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
		*(double*)(cpu.FR+fd) = *(double*)(cpu.FR+fs) + *(double*)(cpu.FR+ft);
		break;
	case 1: //sub.d
		*(double*)(cpu.FR+fd) = *(double*)(cpu.FR+fs) - *(double*)(cpu.FR+ft);
		break;
	case 2: //mul.d
		*(double*)(cpu.FR+fd) = *(double*)(cpu.FR+fs) * *(double*)(cpu.FR+ft);
		break;
	case 3: //div.d
		*(double*)(cpu.FR+fd) = *(double*)(cpu.FR+fs) / *(double*)(cpu.FR+ft);
		break;
	case 4: //sqrt.d
		*(double*)(cpu.FR+fd) = sqrt( *(double*)(cpu.FR+fs) );
		break;
	case 5: //abs.d
		*(u64*)(cpu.FR+fd) = ( *(u64*)(cpu.FR+fs) ) & ~BIT(63);
		break;
	case 6: //mov.d
		*(double*)(cpu.FR+fd) = ( *(double*)(cpu.FR+fs) );
		break;
	case 7: //neg.d
		*(double*)(cpu.FR+fd) = -( *(double*)(cpu.FR+fs) );
		break;
	case 8: //round.l.d
		cpu.FR[fd] = llround( *(double*)(cpu.FR+fs) );
		break;
	case 9: //trunc.l.d
		cpu.FR[fd] = trunc( *(double*)(cpu.FR+fs) );
		break;
	case 10: //ceil.l.d
		cpu.FR[fd] = ceil( *(double*)(cpu.FR+fs) );
		break;
	case 11: //floor.l.d
		cpu.FR[fd] = floor( *(double*)(cpu.FR+fs) );
		break;
	case 12: //round.w.d //what happens to upper 32bits of things?
		cpu.FR[fd] =(int) llround( *(double*)(cpu.FR+fs) );
		break;
	case 13: //trunc.w.d
		cpu.FR[fd] =(int) trunc( *(double*)(cpu.FR+fs) );
		break;
	case 14: //ceil.w.d
		cpu.FR[fd] =(int) ceil( *(double*)(cpu.FR+fs) );
		break;
	case 15: //floor.w.d
		cpu.FR[fd] =(int) floor( *(double*)(cpu.FR+fs) );
		break;
	case 32: //cvt.s.d
		*(float*)(&cpu.FR[fd]) =(float) ( *(double*)(cpu.FR+fs) );
		break;
	case 36: //cvt.w.d
		*(s32*)(&cpu.FR[fd]) =(s32) ( *(double*)(cpu.FR+fs) );
		break;
	case 37: //cvt.l.d
		cpu.FR[fd] =(s64) ( *(double*)(cpu.FR+fs) );
		break;
	case 48: //c.f.d
		break;
	case 49: //c.un.d
		break;
	case 50: //c.eq.d
		cpu.FC31 = (*(double*)(cpu.FR+fs) == *(double*)(cpu.FR+ft))<<23;
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
		cpu.FC31 = (*(double*)(cpu.FR+fs) == *(double*)(cpu.FR+ft))<<23;
		break;
	case 60: //c.lt.d
	case 61: //c.nge.d
		cpu.FC31 = (*(double*)(cpu.FR+fs) < *(double*)(cpu.FR+ft))<<23;
		break;
	case 62: //c.le.d
	case 63: //c.ngt.d
		cpu.FC31 = (*(double*)(cpu.FR+fs) <= *(double*)(cpu.FR+ft))<<23;
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
		*(float*)(cpu.FR+fd) = (float) *(int*)(cpu.FR+fs);
		break;
	case 33: //cvt.d.w
		*(double*)(cpu.FR+fd) = (double) *(int*)(cpu.FR+fs);
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
		*(float*)(cpu.FR+fd) = (float)  (s64)cpu.FR[fs];
		break;
	case 33: //cvt.d.l
		*(double*)(cpu.FR+fd) = (double)(s64)cpu.FR[fs];
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
		if( !(cpu.FC31&BIT(23)) )
		{
			branch_target = (cpu.PC+4) + offset;
			branch_delay = 2;
		}
		break;
	case 1: //BC1T
		if( (cpu.FC31&BIT(23)) )
		{
			branch_target = (cpu.PC+4) + offset;
			branch_delay = 2;
		}		
		break;
	case 2: //BC1FL branch on cop1 false likely
		if( !(cpu.FC31&BIT(23)) )
		{
			branch_target = (cpu.PC+4) + offset;
			branch_delay = 2;
		} else {
			cpu.PC += 4;
		}
		break;
	case 3: //BC1TL
		if( !(cpu.FC31&BIT(23)) )
		{
			branch_target = (cpu.PC+4) + offset;
			branch_delay = 2;
		} else {
			cpu.PC += 4;
		}
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
		case 0x18: interp_eret(opcode); return;
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
	u32 addr = se16to32(offset) + (u32)cpu.R[rs];
	//todo: tlb
	cpu.FR[rt] = read64(addr&~7);
	return;
}

void interp_lwc1(u32 opcode)
{
	OPCODE_PARTS;
	u32 addr = se16to32(offset) + (u32)cpu.R[rs];
	//todo: tlb
	cpu.FR[rt] = read32(addr&~3);
	return;
}

void interp_swc1(u32 opcode)
{
	OPCODE_PARTS;
	u32 addr = se16to32(offset) + (u32)cpu.R[rs];
	//todo: tlb
	write32(addr&~3, cpu.FR[rt]);
	return;
}

void interp_sdc1(u32 opcode)
{
	OPCODE_PARTS;
	u32 addr = se16to32(offset) + (u32)cpu.R[rs];
	//todo: tlb
	write64(addr&~7, cpu.FR[rt]);
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
	cpu.R[0] = 0;
	interp_op(opcode);
	cpu.PC += 4;
	if( branch_delay > 0 )
	{
		branch_delay--;
		if( branch_delay == 0 ) cpu.PC = branch_target;
	}
	return 1;
}





