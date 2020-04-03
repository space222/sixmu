#include <string>
#include <cstdlib>
#include <cstdio>
#include "types.h"

rspregs rsp;

#define COP2_PARTS int vt = (opcode>>16)&0x1F; int vs = (opcode>>11)&0x1F; int vd = (opcode>>6)&0x1F; int e = (opcode>>21)&0xf
#define SPECIAL_PARTS int rs = (opcode>>21)&0x1F; int rt = (opcode>>16)&0x1F; int rd = (opcode>>11)&0x1F
#define OPCODE_PARTS  int rs = (opcode>>21)&0x1F; int rt = (opcode>>16)&0x1F; u16 offset = (opcode&0xFFFF)
#define REGIMM_PARTS  int rs = (opcode>>21)&0x1F; u16 offset = (opcode&0xFFFF);
#define STORE_PARTS int base =(opcode>>21)&0x1F; int vt =(opcode>>16)&0x1F; int sop =(opcode>>11)&0x1F; int e =(opcode>>7)&15; int offset = (opcode&0x7f)
#define SA_PART ((opcode>>6)&0x1F)

#define se16to32(a)  ((s32)(s16)(a))
#define se16to32u(a) ((u32)(s32)(s16)(a))
#define se8to32(a)   ((s32)(s8) (a))
#define ze16to32(a)  ((u32)(u16)(a))
#define  ze8to32(a)  ((u32)(u8) (a))

typedef void(*interpptr)(u32);
void rsp_rsp_interp_op(u32 opcode);
u32 sp_reg_read32(u32);
void sp_reg_write32(u32, u32);
static int branch_delay = 0;
static u32 branch_target = 0;
extern u8 DMEM[0x1000];
extern u8 IMEM[0x1000];

u8 read8_rsp(u32 addr) { return DMEM[addr&0xFFF]; }
u16 read16_rsp(u32 addr) { return __builtin_bswap16(*(u16*)(DMEM+(addr&0xffe))); }
u32 read32_rsp(u32 addr) { return __builtin_bswap32(*(u32*)(DMEM+(addr&0xffc))); }
void write8_rsp(u32 addr, u8 v) { DMEM[addr&0xfff] = v; return; }
void write16_rsp(u32 addr, u16 v) { *(u16*)(DMEM+(addr&0xffe)) = __builtin_bswap16(v); return; }
void write32_rsp(u32 addr, u32 v) { *(u32*)(DMEM+(addr&0xffc)) = __builtin_bswap32(v); return; }

void rsp_undef_opcode(u32 opcode)
{
	printf("RSP: Undefined opcode %x\n", opcode);
	return;
}

void rsp_interp_bne(u32 opcode)
{
	OPCODE_PARTS;
	if( rsp.R[rs] != rsp.R[rt] )
	{
		branch_target = (rsp.PC+4) + (se16to32(offset)<<2);
		branch_delay = 2;
	}
	return;
}

void rsp_interp_bltzal(u32 opcode)
{
	REGIMM_PARTS;
	rsp.R[31] = rsp.PC + 8;
	if( (rsp.R[rs] >> 31) )
	{
		branch_target = (rsp.PC+4) + (se16to32(offset)<<2);
		branch_delay = 2;
	}
	return;
}

static bool ignore_once = false;

void rsp_interp_bgezal(u32 opcode)
{
	REGIMM_PARTS;
	rsp.R[31] = rsp.PC + 8;
	if( !(rsp.R[rs] >> 31) )
	{
		branch_target = (rsp.PC+4) + (se16to32(offset)<<2);
		branch_delay = 2;
	}
	return;
}

void rsp_interp_beq(u32 opcode)
{
	OPCODE_PARTS;
	if( rsp.R[rs] == rsp.R[rt] )
	{
		branch_target = (rsp.PC+4) + (se16to32(offset)<<2);
		branch_delay = 2;
	}
	return;
}

void rsp_interp_blez(u32 opcode)
{
	OPCODE_PARTS;
	if( (rsp.R[rs] >> 31) || (rsp.R[rs] == 0) )
	{
		branch_target = (rsp.PC+4) + (se16to32(offset)<<2);
		branch_delay = 2;
	}
	return;
}

void rsp_interp_bltz(u32 opcode)
{
	REGIMM_PARTS;
	if( (rsp.R[rs] >> 31) )
	{
		branch_target = (rsp.PC+4) + (se16to32(offset)<<2);
		branch_delay = 2;
	}
	return;
}

void rsp_interp_bgez(u32 opcode)
{
	REGIMM_PARTS;
	if( !(rsp.R[rs] >> 31) )
	{
		branch_target = (rsp.PC+4) + (se16to32(offset)<<2);
		branch_delay = 2;
	}
	return;
}

void rsp_interp_bgtz(u32 opcode)
{
	OPCODE_PARTS;
	if( (rsp.R[rs]!=0) && !(rsp.R[rs] >> 31) )
	{
		branch_target = (rsp.PC+4) + (se16to32(offset)<<2);
		branch_delay = 2;
	}
	return;
}

interpptr rsp_regimm_ops[] = { 
	rsp_interp_bltz, rsp_interp_bgez, rsp_undef_opcode, rsp_undef_opcode, rsp_undef_opcode, rsp_undef_opcode, rsp_undef_opcode, rsp_undef_opcode,
	rsp_undef_opcode, rsp_undef_opcode, rsp_undef_opcode, rsp_undef_opcode, rsp_undef_opcode, rsp_undef_opcode, rsp_undef_opcode, rsp_undef_opcode,
	rsp_interp_bltzal, rsp_interp_bgezal, rsp_undef_opcode, rsp_undef_opcode, rsp_undef_opcode, rsp_undef_opcode, rsp_undef_opcode, rsp_undef_opcode,
	rsp_undef_opcode, rsp_undef_opcode, rsp_undef_opcode, rsp_undef_opcode, rsp_undef_opcode, rsp_undef_opcode, rsp_undef_opcode, rsp_undef_opcode
};

void rsp_interp_regimm(u32 opcode)
{
	rsp_regimm_ops[(opcode>>16)&0x1F](opcode);
	return;
}

extern u32 mi_regs[4];
extern u32 sp_regs[13];
void rsp_interp_break(u32 opcode)
{
	printf("RSP: broke\n");
	if( sp_regs[4]&BIT(6) )
	{
		mi_regs[2] |= 1;
	}
	sp_regs[4] |= 3;
	return;
}

void rsp_interp_j(u32 opcode)
{
	branch_target = ((opcode<<2)&0x0FFFFFFF) | ((rsp.PC+4)&0xF0000000);
	branch_delay = 2;
	return;
}

void rsp_interp_jal(u32 opcode)
{
	rsp.R[31] = rsp.PC + 8;
	branch_target = ((opcode<<2)&0x0FFFFFFF) | ((rsp.PC+4)&0xF0000000);
	branch_delay = 2;
	return;
}

void rsp_interp_jalr(u32 opcode)
{
	SPECIAL_PARTS;
	if( rd ) rsp.R[rd] = rsp.PC + 8;
	branch_target = rsp.R[rs];
	branch_delay = 2;
	return;
}

void rsp_interp_jr(u32 opcode)
{
	SPECIAL_PARTS;
	branch_target = rsp.R[rs];
	branch_delay = 2;
	return;
}

void rsp_interp_and(u32 opcode)
{
	SPECIAL_PARTS;
	if( rd ) rsp.R[rd] = (rsp.R[rs] & rsp.R[rt]);
	return;
}

void rsp_interp_or(u32 opcode)
{
	SPECIAL_PARTS;
	if( rd ) rsp.R[rd] = (rsp.R[rs] | rsp.R[rt]);
	return;
}

void rsp_interp_xor(u32 opcode)
{
	SPECIAL_PARTS;
	if( rd ) rsp.R[rd] = (rsp.R[rs] ^ rsp.R[rt]);
	return;
}

void rsp_interp_nor(u32 opcode)
{
	SPECIAL_PARTS;
	if( rd ) rsp.R[rd] = ~(rsp.R[rs] | rsp.R[rt]);
	return;
}

void rsp_interp_addu(u32 opcode)
{
	SPECIAL_PARTS;
	if( rd ) rsp.R[rd] = rsp.R[rs] + rsp.R[rt];
	return;
}

void rsp_interp_subu(u32 opcode)
{
	SPECIAL_PARTS;
	if( rd ) rsp.R[rd] = rsp.R[rs] - rsp.R[rt];
	return;
}

void rsp_interp_lui(u32 opcode)
{
	OPCODE_PARTS;
	if( rt ) rsp.R[rt] = (offset<<16);
	return;
}

void rsp_interp_andi(u32 opcode)
{
	OPCODE_PARTS;
	if( rt ) rsp.R[rt] = rsp.R[rs] & ze16to32(offset);
	return;
}

void rsp_interp_ori(u32 opcode)
{
	OPCODE_PARTS;
	if( rt ) rsp.R[rt] = rsp.R[rs] | ze16to32(offset);
	return;
}

void rsp_interp_xori(u32 opcode)
{
	OPCODE_PARTS;
	if( rt ) rsp.R[rt] = rsp.R[rs] ^ ze16to32(offset);
	return;
}

void rsp_interp_srlv(u32 opcode)
{
	SPECIAL_PARTS;
	if( rd ) rsp.R[rd] = rsp.R[rt] >> (rsp.R[rs]&0x1F);
	return;
}

void rsp_interp_slti(u32 opcode)
{
	OPCODE_PARTS;
	if( rt ) rsp.R[rt] = (s32)rsp.R[rs] < se16to32(offset);
	return;
}

void rsp_interp_sltiu(u32 opcode)
{
	OPCODE_PARTS;
	if( rt ) rsp.R[rt] = (u32)rsp.R[rs] < se16to32u(offset);
	return;
}

void rsp_interp_slt(u32 opcode)
{
	SPECIAL_PARTS;
	if( rd ) rsp.R[rd] = (s32)rsp.R[rs] < (s32)rsp.R[rt];
	return;
}

void rsp_interp_sltu(u32 opcode)
{
	SPECIAL_PARTS;
	if( rd ) rsp.R[rd] = rsp.R[rs] < rsp.R[rt];
	return;
}

void rsp_interp_sllv(u32 opcode)
{
	SPECIAL_PARTS;
	if( rd ) rsp.R[rd] = rsp.R[rt] << (rsp.R[rs]&0x1F);
	return;
}

void rsp_interp_srav(u32 opcode)
{
	SPECIAL_PARTS;
	if( rd ) rsp.R[rd] = (s32)rsp.R[rt] >> (rsp.R[rs]&0x1F);
	return;
}

void rsp_interp_srl(u32 opcode)
{
	SPECIAL_PARTS;
	if( rd ) rsp.R[rd] = rsp.R[rt] >> SA_PART;
	return;
}

void rsp_interp_sll(u32 opcode)
{
	SPECIAL_PARTS;
	if( rd ) rsp.R[rd] = rsp.R[rt] << SA_PART;
	return;
}

void rsp_interp_sra(u32 opcode)
{
	SPECIAL_PARTS;
	if( rd ) rsp.R[rd] = (s32)rsp.R[rt] >> SA_PART;
	return;
}

void rsp_interp_lb(u32 opcode)
{
	OPCODE_PARTS;
	u32 addr = se16to32(offset) + (u32)rsp.R[rs];
	rsp.R[rt] = se8to32( read8_rsp(addr) );
	return;
}

void rsp_interp_lbu(u32 opcode)
{
	OPCODE_PARTS;
	u32 addr = se16to32(offset) + (u32)rsp.R[rs];
	rsp.R[rt] = ze8to32( read8_rsp(addr) );
	return;
}

void rsp_interp_lh(u32 opcode)
{
	OPCODE_PARTS;
	u32 addr = se16to32(offset) + (u32)rsp.R[rs];
	rsp.R[rt] = se16to32( read16_rsp(addr&~1) );
	return;
}

void rsp_interp_lhu(u32 opcode)
{
	OPCODE_PARTS;
	u32 addr = se16to32(offset) + (u32)rsp.R[rs];
	rsp.R[rt] = ze16to32( read16_rsp(addr&~1) );
	return;
}

void rsp_interp_lw(u32 opcode)
{
	OPCODE_PARTS;
	u32 addr = se16to32(offset) + (u32)rsp.R[rs];
	rsp.R[rt] = read32_rsp(addr&~3);
	return;
}

void rsp_interp_sb(u32 opcode)
{
	OPCODE_PARTS;
	u32 addr = se16to32(offset) + (u32)rsp.R[rs];
	write8_rsp(addr, rsp.R[rt]);
	return;
}

void rsp_interp_sh(u32 opcode)
{
	OPCODE_PARTS;
	u32 addr = se16to32(offset) + (u32)rsp.R[rs];
	write16_rsp(addr&~1, rsp.R[rt]);
	return;
}

void rsp_interp_sw(u32 opcode)
{
	OPCODE_PARTS;
	u32 addr = se16to32(offset) + (u32)rsp.R[rs];
	write32_rsp(addr&~3, rsp.R[rt]);
	return;
}

void rsp_interp_addiu(u32 opcode)
{
	OPCODE_PARTS;
	if( rt ) rsp.R[rt] = (u32)rsp.R[rs] + se16to32(offset);
	return;
}

void rsp_interp_mfc0(u32 opcode)
{
	SPECIAL_PARTS;  // not actually SPECIAL, but uses rt and rd that match that opcode format
	//todo: these are mmio on rsp
	// Rrt = Crd
	if( rd < 8 )
	{ //RSP regs
		rsp.R[rt] = sp_reg_read32(rd<<2);
		return;
	}
	printf("RSP: Unhandled MMIO rd = %i\n", rd);

	return;
}

void rsp_interp_mtc0(u32 opcode)
{
	SPECIAL_PARTS;  // not actually SPECIAL, but uses rt and rd that match that opcode format
	//todo: these are mmio on rsp
	if( rd < 8 )
	{ //RSP regs
		sp_reg_write32(rd<<2, rsp.R[rt]);
		return;
	}
	printf("RSP: Unhandled MMIO wr = %i\n", rd);
	return;
}

void rsp_interp_bc2(u32 opcode)
{
	u32 subop = (opcode>>16)&0x1F;
	s32 offset =(s32) (s16)(opcode&0xffff);
	offset <<= 2;

	switch( subop )
	{
	case 0: //BC1F branch on cop2 false
		break;
	case 1: //BC1T
		break;
	case 2: //BC1FL branch on cop2 false likely
		break;
	case 3: //BC1TL
		break;
	}

	return;
}

void rsp_interp_mfc2(u32 opcode)
{
	printf("RSP: MFC2\n");
	return;
}

void rsp_interp_cfc2(u32 opcode)
{
	SPECIAL_PARTS;
	switch( rd )
	{
	case 0:
		rsp.R[rt] = rsp.VCO;
		return;
	case 1:
		rsp.R[rt] = rsp.VCC;
		return;
	case 2:
		rsp.R[rt] = rsp.VCE;
		return;
	default: return;
	}
	printf("RSP: CFC2\n");
	return;
}

void rsp_interp_mtc2(u32 opcode)
{
	printf("RSP: MTC2\n");
	return;
}

void rsp_interp_ctc2(u32 opcode)
{
	printf("RSP: CTC2\n");
	return;
}

interpptr rsp_special_ops[] = { 
	rsp_interp_sll, rsp_undef_opcode, rsp_interp_srl, rsp_interp_sra, rsp_interp_sllv, rsp_undef_opcode, rsp_interp_srlv, rsp_interp_srav,
	rsp_interp_jr, rsp_interp_jalr, rsp_undef_opcode, rsp_undef_opcode, rsp_undef_opcode, rsp_interp_break, rsp_undef_opcode, rsp_undef_opcode,
	rsp_undef_opcode, rsp_undef_opcode, rsp_undef_opcode, rsp_undef_opcode, rsp_undef_opcode, rsp_undef_opcode, rsp_undef_opcode, rsp_undef_opcode,
	rsp_undef_opcode, rsp_undef_opcode, rsp_undef_opcode, rsp_undef_opcode, rsp_undef_opcode, rsp_undef_opcode, rsp_undef_opcode, rsp_undef_opcode,
	rsp_interp_addu, rsp_interp_addu, rsp_interp_subu, rsp_interp_subu, rsp_interp_and, rsp_interp_or, rsp_interp_xor, rsp_interp_nor, 
	rsp_undef_opcode, rsp_undef_opcode, rsp_interp_slt, rsp_interp_sltu, rsp_undef_opcode, rsp_undef_opcode, rsp_undef_opcode, rsp_undef_opcode,
	rsp_undef_opcode, rsp_undef_opcode, rsp_undef_opcode, rsp_undef_opcode, rsp_undef_opcode, rsp_undef_opcode, rsp_undef_opcode, rsp_undef_opcode,
	rsp_undef_opcode, rsp_undef_opcode, rsp_undef_opcode, rsp_undef_opcode, rsp_undef_opcode, rsp_undef_opcode, rsp_undef_opcode, rsp_undef_opcode
};

interpptr rsp_cop0_ops[] = { 
	rsp_interp_mfc0, rsp_undef_opcode, rsp_undef_opcode, rsp_undef_opcode, rsp_interp_mtc0, rsp_undef_opcode, rsp_undef_opcode, rsp_undef_opcode,
	rsp_undef_opcode /* bcc0? */, rsp_undef_opcode,  rsp_undef_opcode,  rsp_undef_opcode,  rsp_undef_opcode,  rsp_undef_opcode,  rsp_undef_opcode,  rsp_undef_opcode, 
};

interpptr rsp_cop2_ops[] = { 
	rsp_interp_mfc2, rsp_undef_opcode, rsp_interp_cfc2, rsp_undef_opcode, rsp_interp_mtc2, rsp_undef_opcode, rsp_interp_ctc2, rsp_undef_opcode,
	rsp_interp_bc2, rsp_undef_opcode, rsp_undef_opcode, rsp_undef_opcode, rsp_undef_opcode, rsp_undef_opcode, rsp_undef_opcode, rsp_undef_opcode,
	rsp_undef_opcode, rsp_undef_opcode, rsp_undef_opcode, rsp_undef_opcode, rsp_undef_opcode, rsp_undef_opcode, rsp_undef_opcode, rsp_undef_opcode, 
	rsp_undef_opcode, rsp_undef_opcode, rsp_undef_opcode, rsp_undef_opcode, rsp_undef_opcode, rsp_undef_opcode, rsp_undef_opcode, rsp_undef_opcode
};

#define VECPARTS int J = 0; \
		if( e == 0 ) {J = S;} \
		else if( (e&0xE) == 2 ) { J = (e&1)+(S&0xE); } \
		else if( (e&0xC) == 4 ) { J = (e&3)+(S&0xC); } \
		else if( e&8 )  J = (e&7)


void vnor(u32 opcode)
{
	COP2_PARTS;

	for(int S = 0; S < 8; ++S)
	{
		VECPARTS;
		u16 res = ~( *(u16*)(rsp.V+(vs*16)+(S<<1)) | *(u16*)(rsp.V+(vt*16)+(J<<1)) );
		rsp.A[S] = (rsp.A[S]&~0xffffull) | res;
		*(u16*)(rsp.V+(vd*16)+(S<<1)) = res;
	}

	return;
}

void vxor(u32 opcode)
{
	COP2_PARTS;

	for(int S = 0; S < 8; ++S)
	{
		VECPARTS;
		u16 res = *(u16*)(rsp.V+(vs*16)+(S<<1)) ^ *(u16*)(rsp.V+(vt*16)+(J<<1));
		rsp.A[S] = (rsp.A[S]&~0xffffull) | res;
		*(u16*)(rsp.V+(vd*16)+(S<<1)) = res;
	}

	return;
}

void vor(u32 opcode)
{
	COP2_PARTS;

	for(int S = 0; S < 8; ++S)
	{
		VECPARTS;
		u16 res = *(u16*)(rsp.V+(vs*16)+(S<<1)) | *(u16*)(rsp.V+(vt*16)+(J<<1));
		rsp.A[S] = (rsp.A[S]&~0xffffull) | res;
		*(u16*)(rsp.V+(vd*16)+(S<<1)) = res;
	}

	return;
}

void vand(u32 opcode)
{
	COP2_PARTS;

	for(int S = 0; S < 8; ++S)
	{
		VECPARTS;
		u16 res = *(u16*)(rsp.V+(vs*16)+(S<<1)) & *(u16*)(rsp.V+(vt*16)+(J<<1));
		rsp.A[S] = (rsp.A[S]&~0xffffull) | res;
		*(u16*)(rsp.V+(vd*16)+(S<<1)) = res;
	}

	return;
}

void vnand(u32 opcode)
{
	COP2_PARTS;

	for(int S = 0; S < 8; ++S)
	{
		VECPARTS;
		u16 res = *(u16*)(rsp.V+(vs*16)+(S<<1)) & *(u16*)(rsp.V+(vt*16)+(J<<1));
		res = ~res;
		rsp.A[S] = (rsp.A[S]&~0xffffull) | res;
		*(u16*)(rsp.V+(vd*16)+(S<<1)) = res;
	}

	return;
}

void vnxor(u32 opcode)
{
	COP2_PARTS;

	for(int S = 0; S < 8; ++S)
	{
		VECPARTS;
		u16 res = ~ ( *(u16*)(rsp.V+(vs*16)+(S<<1)) ^ *(u16*)(rsp.V+(vt*16)+(J<<1)) );
		rsp.A[S] = (rsp.A[S]&~0xffffull) | res;
		*(u16*)(rsp.V+(vd*16)+(S<<1)) = res;
	}

	return;
}

void vadd(u32 opcode)
{
	COP2_PARTS;

	for(int S = 0; S < 8; ++S)
	{
		VECPARTS;
		s32 res = *(s16*)(rsp.V+(vs*16)+(S<<1));
		res += *(s16*)(rsp.V+(vt*16)+(J<<1));
		res += (rsp.VCO>>S)&1;
		rsp.A[S] = (rsp.A[S]&~0xffffull);
		rsp.A[S] |= (res&0xffff);
		if( res > 32767 ) 
			res = 32767; 
		else if( res < -32768 ) 
			res = -32768;
		
		*(u16*)(rsp.V+(vd*16)+(S<<1)) = res&0xffff;
	}

	rsp.VCO = 0;
	return;
}

void vaddc(u32 opcode)
{
	COP2_PARTS;
	rsp.VCO = 0;

	for(int S = 0; S < 8; ++S)
	{
		VECPARTS;
		u32 res = *(u16*)(rsp.V+(vs*16)+(S<<1));
		res += *(u16*)(rsp.V+(vt*16)+(J<<1));
		//res += (rsp.VCO>>S)&1;
		rsp.A[S] = (rsp.A[S]&~0xffffull);
		rsp.A[S] |= (res&0xffff);
		
		*(u16*)(rsp.V+(vd*16)+(S<<1)) = res&0xffff;
		if( res&BIT(16) )
		{
			rsp.VCO |= BIT(S^7);
		}
	}

	return;
}

void vsub(u32 opcode)
{
	COP2_PARTS;

	for(int S = 0; S < 8; ++S)
	{
		VECPARTS;
		s32 res = *(s16*)(rsp.V+(vs*16)+(S<<1));
		res -= *(s16*)(rsp.V+(vt*16)+(J<<1));
		res -= (rsp.VCO>>S)&1;
		rsp.A[S] = (rsp.A[S]&~0xffffull);
		rsp.A[S] |= (res&0xffff);
		if( res > 32767 ) 
			res = 32767; 
		else if( res < -32768 ) 
			res = -32768;
		
		*(u16*)(rsp.V+(vd*16)+(S<<1)) = res&0xffff;
	}

	rsp.VCO = 0;
	return;
}

void vsubc(u32 opcode)
{
	COP2_PARTS;
	rsp.VCO = 0;

	for(int S = 0; S < 8; ++S)
	{
		VECPARTS;
		u32 res = *(u16*)(rsp.V+(vs*16)+(S<<1));
		res -= *(u16*)(rsp.V+(vt*16)+(J<<1));
		//res += (rsp.VCO>>S)&1;

		rsp.A[S] = (rsp.A[S]&~0xffffull);
		rsp.A[S] |= (res&0xffff);
		
		*(u16*)(rsp.V+(vd*16)+(S<<1)) = res&0xffff;

		if( res & BIT(16) )
		{
			rsp.VCO |= BIT(S^7)|BIT(((S^7)+8));
		} else if( (res != 0) && !(res&BIT(16)) ) {
			rsp.VCO |= BIT(((S^7)+8));
		}
	}

	return;
}

void vmov(u32 opcode)
{
	COP2_PARTS;
	e ^= 7;
	vs ^= 7;
	u16 res = *(u16*)(rsp.V+(vt*16)+(e<<1));
	*(u16*)(rsp.V+(vd*16)+(vs<<1)) = res;
	rsp.A[e] = (rsp.A[e]&~0xffffull) | (u64)res;
	return;
}

void vmrg(u32 opcode)
{
	COP2_PARTS;
	for(int S = 0; S < 8; ++S)
	{
		VECPARTS;
		u16 res;
		if( rsp.VCC & BIT(S) )
		{
			res = *(u16*)(rsp.V+(vs*16)+(S<<1));
		} else {
			res = *(u16*)(rsp.V+(vt*16)+(S<<1));
		}
		*(u16*)(rsp.V+(vd*16)+(S<<1)) = res;
		rsp.A[S] = (rsp.A[S]&~0xffff) | res;
	}

	return;
}

void vsar(u32 opcode)
{
	COP2_PARTS;
	//printf("VSAR e = %i\n", e);
	for(int S = 0; S < 8; ++S)
	{
		if( e == 8 )
		{
			*(u16*)(rsp.V+(vd*16)+(S<<1)) = rsp.A[S]>>32;
			rsp.A[S] &= 0xFFFFFFFFull;
			rsp.A[S] |= (u64)(*(u16*)(rsp.V+(vs*16)+(S<<1))) << 32;
		} else if( e == 9 ) {
			*(u16*)(rsp.V+(vd*16)+(S<<1)) = rsp.A[S]>>16;
			rsp.A[S] &= 0xFFFF0000FFFFull;
			rsp.A[S] |= (u32)(*(u16*)(rsp.V+(vs*16)+(S<<1))) << 16;
		} else if( e == 10 ) {
			*(u16*)(rsp.V+(vd*16)+(S<<1)) = rsp.A[S];
			rsp.A[S] &= 0xFFFFFFFF0000ull;
			rsp.A[S] |= (*(u16*)(rsp.V+(vs*16)+(S<<1)));
		}
	}

	return;
}

void vabs(u32 opcode)
{
	COP2_PARTS;

	printf("VABS, opcode = %x, e = %x\n", opcode, e);

	for(int S = 0; S < 8; ++S)
	{
		VECPARTS;
		
		s16 tst = *(u16*)(rsp.V+(vs*16)+(S<<1));
		u16 res = 0;
		u16 VT = *(u16*)(rsp.V+(vt*16)+(J<<1));
		if( tst < 0 )
		{
			res = -VT;
		} else if( tst > 0 ) {
			res = VT;
		}
		*(u16*)(rsp.V+(vd*16)+(S<<1)) = res;
		rsp.A[S] = (rsp.A[S]&~0xffffull) | res;
	}

	return;
}

void rsp_interp_cop0(u32 opcode)
{
	if( (opcode>>25) & 1 )
	{
		return;
	}

	rsp_cop0_ops[(opcode>>21)&0x1F](opcode);
	return;
}

void rsp_interp_cop2(u32 opcode)
{
	if( (opcode>>25) & 1 )
	{
		switch( opcode&0x3F )
		{
		case 0x10: vadd(opcode); return;
		case 0x11: vsub(opcode); return;
		case 0x13: vabs(opcode); return;
		case 0x14: vaddc(opcode); return;
		case 0x15: vsubc(opcode); return;

		case 0x1D: vsar(opcode); return;

		case 0x27: vmrg(opcode); return;

		case 0x28: vand(opcode); return;
		case 0x29: vnand(opcode); return;
		case 0x2A: vor(opcode); return;
		case 0x2B: vnor(opcode); return;
		case 0x2C: vxor(opcode); return;
		case 0x2D: vnxor(opcode); return;

		case 0x33: vmov(opcode); return;
		case 0x3F: //VNULL??
		case 0x37: return; //VNOP
		}
		printf("RSP: COP2! op = %x\n", opcode&0x3F); exit(1);
		return;
	}
	rsp_cop2_ops[(opcode>>21)&0x1F](opcode);
	return;
}

void rsp_interp_lwc2(u32 opcode)
{
	STORE_PARTS;
	int L = 0;
	u32 addr = 0;
	switch( sop )
	{
	case 0: //LBV
		rsp.V[(vt*16)+e] = read8_rsp(rsp.R[base] + offset);
		break;
	case 1: //LSV
		*(u16*)(rsp.V+(vt*16)+e) = read16_rsp(rsp.R[base] + (offset<<1));
		break;
	case 2: //LLV
		*(u32*)(rsp.V+(vt*16)+e) = read32_rsp(rsp.R[base] + (offset<<2));
		break;
	case 3: //LDV
		*(u64*)(rsp.V+(vt*16)+e) = __builtin_bswap64(*(u64*)(DMEM+((rsp.R[base]+(offset<<3))&0xfff)));
		break;
	case 4: //LQV docs don't mention offset<<4
		addr = rsp.R[base] + (offset<<4);
		do { rsp.V[(vt*16) + (15 - (L++))] = read8_rsp(addr++); } while( addr&15 );
		break;
	case 7: //LUV
		addr = rsp.R[base] + offset;
		for(int i = 0; i < 8; ++i)
			rsp.V[(vt*16)+(i<<1)] = read8_rsp(addr++);
		break;
	case 0xa:{ //LWV
		addr = rsp.R[base] + offset;
		for(int slice = 0; slice < 8; ++slice)
		{
			int regnum = (vt & 0x18) | ((slice + (e >> 1)) & 0x7);
			regnum *= 16;
			*(u16*)(rsp.V + regnum + e) = read16_rsp(addr);
			addr += 2;	
		}
		}break;
	case 0xb: //LTV todo
	case 5: //LRV
	default:
		printf("RSP: LWC2, sub op = %x\n", sop);	
		break;
	}

	return;
}

void rsp_interp_swc2(u32 opcode)
{
	STORE_PARTS;
	int L = 0;
	u32 addr = 0;
	switch( sop )
	{
	case 0: //SBV
		write8_rsp(rsp.R[base] + offset, rsp.V[(vt*16)+e]);
		break;
	case 1: //SSV
		write16_rsp(rsp.R[base] + (offset<<1), *(u16*)(rsp.V+(vt*16)+e));
		break;
	case 2: //SLV
		write32_rsp(rsp.R[base] + (offset<<2), *(u32*)(rsp.V+(vt*16)+e));
		break;
	case 3: //SDV
		*(u64*)(DMEM+((rsp.R[base]+(offset<<3))&0xfff)) = __builtin_bswap64(*(u64*)(rsp.V+(vt*16)+e));
		break;
	case 4: //SQV
		addr = rsp.R[base] + (offset<<4);
		do { write8_rsp(addr++, rsp.V[vt*16 + (15 - L++)]); } while( addr&15 );
		break;
	case 7: //SUV
		addr = rsp.R[base] + offset;
		for(int i = 0; i < 8; ++i)
			write8_rsp(addr++, rsp.V[(vt*16)+(i<<1)]);
		break;
	case 0xa:{ //SWV
		addr = rsp.R[base] + offset;
		for(int slice = 0; slice < 8; ++slice)
		{
			int regnum = (vt & 0x18) | ((slice + (e>>1)) & 7);
			regnum *= 16;
			write16_rsp(addr, *(u16*)(rsp.V+regnum+e));
			addr+=2;
		}
		}break;
	case 0xb: //LTV todo
	case 5: //SRV
	default:
		printf("RSP: LWC2, sub op = %x\n", sop);	
		break;
	}
	return;
}

void rsp_interp_special(u32 opcode)
{
	rsp_special_ops[opcode & 0x3F](opcode);
	return;
}

interpptr rsp_opcodes[] = { 
	rsp_interp_special, rsp_interp_regimm, rsp_interp_j, rsp_interp_jal, rsp_interp_beq, rsp_interp_bne, rsp_interp_blez, rsp_interp_bgtz,
	rsp_interp_addiu, rsp_interp_addiu, rsp_interp_slti, rsp_interp_sltiu, rsp_interp_andi, rsp_interp_ori, rsp_interp_xori, rsp_interp_lui,			
	rsp_interp_cop0, rsp_undef_opcode, rsp_interp_cop2, rsp_undef_opcode, rsp_undef_opcode, rsp_undef_opcode, rsp_undef_opcode, rsp_undef_opcode,
	rsp_undef_opcode, rsp_undef_opcode, rsp_undef_opcode, rsp_undef_opcode, rsp_undef_opcode, rsp_undef_opcode, rsp_undef_opcode, rsp_undef_opcode,
	rsp_interp_lb, rsp_interp_lh, rsp_undef_opcode, rsp_interp_lw, rsp_interp_lbu, rsp_interp_lhu, rsp_undef_opcode, rsp_undef_opcode,
	rsp_interp_sb, rsp_interp_sh, rsp_undef_opcode, rsp_interp_sw, rsp_undef_opcode, rsp_undef_opcode, rsp_undef_opcode, rsp_undef_opcode,
	rsp_undef_opcode, rsp_undef_opcode, rsp_interp_lwc2, rsp_undef_opcode, rsp_undef_opcode, rsp_undef_opcode, rsp_undef_opcode, rsp_undef_opcode,
	rsp_undef_opcode, rsp_undef_opcode, rsp_interp_swc2, rsp_undef_opcode, rsp_undef_opcode, rsp_undef_opcode, rsp_undef_opcode, rsp_undef_opcode
};

void interp_rsp_run()
{
	u32 opcode = __builtin_bswap32(*(u32*)(IMEM+(rsp.PC&0xFFC)));
	rsp.R[0] = 0;
	rsp_opcodes[(opcode>>26)](opcode);
	rsp.PC += 4;
	if( branch_delay > 0 )
	{
		branch_delay--;
		if( branch_delay == 0 ) rsp.PC = branch_target;
	}
	return;
}





