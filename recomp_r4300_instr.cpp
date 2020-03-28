#include <vector>
#include <string>
#include <cstdlib>
#include <unordered_map>
#include <set>
#include "fmt/format.h"
#include "types.h"
#include "libtcc.h"

regs cpu;

#define FPU_PARTS  int ft = 32+((opcode>>16)&0x1F); int fs = 32+((opcode>>11)&0x1F); int fd = 32+((opcode>>6)&0x1F); int mop = opcode&0x3F
#define SPECIAL_PARTS int rs = (opcode>>21)&0x1F; int rt = (opcode>>16)&0x1F; int rd = (opcode>>11)&0x1F
#define OPCODE_PARTS  int rs = (opcode>>21)&0x1F; int rt = (opcode>>16)&0x1F; u16 offset = (opcode&0xFFFF)
#define REGIMM_PARTS  int rs = (opcode>>21)&0x1F; u16 offset = (opcode&0xFFFF);

using namespace std::literals::string_literals;
typedef bool(*compptr)(u32, BasicBlock*, std::string&);
u32 check_vaddr(u32, u32);
bool compile_op(u32 opcode, BasicBlock* BB, std::string& func);
u32 read32(u32);

bool undef_opcode(u32 opcode, BasicBlock* BB, std::string& func)
{
	func += "/* undef opcode */\n";
	return false;
}

bool c_trap(u32 opcode, BasicBlock* BB, std::string& func)
{
	return undef_opcode(opcode, BB, func);
}

bool c_bne(u32 opcode, BasicBlock* BB, std::string& func)
{
	OPCODE_PARTS;

	func += fmt::format("{{int cond = (R[{0}] != R[{1}]);\n", rs, rt);

	BB->end_addr += 4;
	compile_op(read32(BB->end_addr), BB, func);

	func += fmt::format("if( cond ) return {0:#x};}}\n", BB->end_addr+(((s32)(s16)offset)<<2));
	return true;
}

bool c_bnel(u32 opcode, BasicBlock* BB, std::string& func)
{
	OPCODE_PARTS;

	func += fmt::format("{{int cond = (R[{0}] != R[{1}]);\nif( cond ) {{", rs, rt);

	BB->end_addr += 4;
	compile_op(read32(BB->end_addr), BB, func);

	func += fmt::format("return {0:#x};}} }}\n", BB->end_addr+(((s32)(s16)offset)<<2));
	return true;
}

bool c_bltzl(u32 opcode, BasicBlock* BB, std::string& func)
{
	REGIMM_PARTS;

	func += fmt::format("{{int cond = R[{0}]>>63;\nif( cond ) {{\n", rs);

	BB->end_addr += 4;
	compile_op(read32(BB->end_addr), BB, func);

	func += fmt::format("return {0:#x};}} }}\n", BB->end_addr+(((s32)(s16)offset)<<2));
	return true;
}

bool c_bltzal(u32 opcode, BasicBlock* BB, std::string& func)
{
	REGIMM_PARTS;

	func += fmt::format("{{int cond = R[{0}]>>63;\nR[31] = {1:#x};\n", rs, BB->end_addr+8);

	BB->end_addr += 4;
	compile_op(read32(BB->end_addr), BB, func);

	func += fmt::format("if( cond ) return {0:#x};}}\n", BB->end_addr+(((s32)(s16)offset)<<2));
	return true;
}

static bool ignore_once = true;

bool c_bgezal(u32 opcode, BasicBlock* BB, std::string& func)
{
	REGIMM_PARTS;
	//todo: This if ignores the bgezal that prevents booting when CIC values are not right.
	//	This likely works only for particular bootcode(s), and I should figure out what values
	//	are wrong and preventing true boot up.
	if( ignore_once && BB->start_addr == 0x800001c8 )
	{
		ignore_once = false;
		return false;
	}

	func += fmt::format("{{int cond = R[{0}]>>63;\nR[31] = {1:#x};\n", rs, BB->end_addr+8);

	BB->end_addr += 4;
	compile_op(read32(BB->end_addr), BB, func);

	func += fmt::format("if( !cond ) return {0:#x};}}\n", BB->end_addr+(((s32)(s16)offset)<<2));
	return true;
}

bool c_bltzall(u32 opcode, BasicBlock* BB, std::string& func)
{
	REGIMM_PARTS;

	func += fmt::format("{{int cond = R[{0}]>>63;\nR[31] = {1:#x};\nif( cond ) {{\n", rs, BB->end_addr+8);

	BB->end_addr += 4;
	compile_op(read32(BB->end_addr), BB, func);

	func += fmt::format("return {0:#x};}} }}\n", BB->end_addr+(((s32)(s16)offset)<<2));
	return true;
}

bool c_bgezall(u32 opcode, BasicBlock* BB, std::string& func)
{
	REGIMM_PARTS;

	func += fmt::format("{{int cond = R[{0}]>>63;\nR[31] = {1:#x};\nif( !cond ) {{\n", rs, BB->end_addr+8);

	BB->end_addr += 4;
	compile_op(read32(BB->end_addr), BB, func);

	func += fmt::format("return {0:#x};}} }}\n", BB->end_addr+(((s32)(s16)offset)<<2));
	return true;
}

bool c_blezl(u32 opcode, BasicBlock* BB, std::string& func)
{
	OPCODE_PARTS;

	func += fmt::format("{{int cond = (R[{0}]>>63) || (R[{0}]==0);\nif( cond ) {{\n", rs);

	BB->end_addr += 4;
	compile_op(read32(BB->end_addr), BB, func);

	func += fmt::format("return {0:#x};}} }}\n", BB->end_addr+(((s32)(s16)offset)<<2));
	return true;
}

bool c_bgtzl(u32 opcode, BasicBlock* BB, std::string& func)
{
	OPCODE_PARTS;

	func += fmt::format("{{int cond = ((R[{0}]>>63)==0) && (R[{0}]!=0);\nif( cond ) {{\n", rs);

	BB->end_addr += 4;
	compile_op(read32(BB->end_addr), BB, func);

	func += fmt::format("return {0:#x};}} }}\n", BB->end_addr+(((s32)(s16)offset)<<2));
	return true;
}

bool c_bgezl(u32 opcode, BasicBlock* BB, std::string& func)
{
	REGIMM_PARTS;

	func += fmt::format("{{int cond = R[{0}]>>63;\nif( !cond ) {{\n", rs);

	BB->end_addr += 4;
	compile_op(read32(BB->end_addr), BB, func);

	func += fmt::format("return {0:#x};}} }}\n", BB->end_addr+(((s32)(s16)offset)<<2));
	return true;
}

bool c_beql(u32 opcode, BasicBlock* BB, std::string& func)
{
	OPCODE_PARTS;
	func += fmt::format("{{int cond = (R[{0}] == R[{1}]);\nif( cond ) {{\n", rs, rt);

	BB->end_addr += 4;
	compile_op(read32(BB->end_addr), BB, func);

	func += fmt::format("return {0:#x};}} }}\n", BB->end_addr+(((s32)(s16)offset)<<2));
	return true;
}

bool c_beq(u32 opcode, BasicBlock* BB, std::string& func)
{
	OPCODE_PARTS;

	func += fmt::format("{{int cond = (R[{0}] == R[{1}]);\n", rs, rt);

	BB->end_addr += 4;
	compile_op(read32(BB->end_addr), BB, func);

	func += fmt::format("if( cond ) return {0:#x};}}\n", BB->end_addr+(((s32)(s16)offset)<<2));

	return true;
}

bool c_blez(u32 opcode, BasicBlock* BB, std::string& func)
{
	OPCODE_PARTS;

	func += fmt::format("{{int cond = (R[{0}]>>63) || (R[{0}]==0);\n", rs);

	BB->end_addr += 4;
	compile_op(read32(BB->end_addr), BB, func);

	func += fmt::format("if( cond ) return {0:#x};}}\n", BB->end_addr+(((s32)(s16)offset)<<2));
	return true;
}

bool c_bltz(u32 opcode, BasicBlock* BB, std::string& func)
{
	REGIMM_PARTS;

	func += fmt::format("{{int cond = (R[{0}]>>63);\n", rs);

	BB->end_addr += 4;
	compile_op(read32(BB->end_addr), BB, func);

	func += fmt::format("if( cond ) return {0:#x};}}\n", BB->end_addr+(((s32)(s16)offset)<<2));
	return true;
}

bool c_bgez(u32 opcode, BasicBlock* BB, std::string& func)
{
	REGIMM_PARTS;

	func += fmt::format("{{int cond = R[{0}]>>63;\n", rs);

	BB->end_addr += 4;
	compile_op(read32(BB->end_addr), BB, func);

	func += fmt::format("if( !cond ) return {0:#x};}}\n", BB->end_addr+(((s32)(s16)offset)<<2));
	return true;
}

bool c_bgtz(u32 opcode, BasicBlock* BB, std::string& func)
{
	OPCODE_PARTS;

	func += fmt::format("{{int cond = ((R[{0}]>>63)==0) && R[{0}]!=0;\n", rs);

	BB->end_addr += 4;
	compile_op(read32(BB->end_addr), BB, func);

	func += fmt::format("if( cond ) return {0:#x};}}\n", BB->end_addr+(((s32)(s16)offset)<<2));
	return true;
}

compptr regimm_ops[] = { 
	c_bltz, c_bgez, c_bltzl, c_bgezl, undef_opcode, undef_opcode, undef_opcode, undef_opcode,
	c_trap, c_trap, c_trap, c_trap, c_trap, undef_opcode, c_trap, undef_opcode,
	c_bltzal, c_bgezal, c_bltzall, c_bgezall, undef_opcode, undef_opcode, undef_opcode, undef_opcode,
	undef_opcode, undef_opcode, undef_opcode, undef_opcode, undef_opcode, undef_opcode, undef_opcode, undef_opcode
};

bool compile_regimm(u32 opcode, BasicBlock* BB, std::string& func)
{
	return regimm_ops[(opcode>>16)&0x1F](opcode, BB, func);
}

bool c_cache(u32 opcode, BasicBlock* BB, std::string& func)
{
	// cache not implemented
	func += "/* unimpl. CACHE */\n";
	return false;
}

bool c_sync(u32 opcode, BasicBlock* BB, std::string& func)
{
	//unimplemented
	func += "/* unimpl. SYNC */\n";
	return false;
}

bool c_ll(u32 opcode, BasicBlock* BB, std::string& func)
{
	//unimplemented
	func += "/* unimpl. LL */\n";
	return false;
}

bool c_lld(u32 opcode, BasicBlock* BB, std::string& func)
{
	//unimplemented
	func += "/* unimpl. LLD */\n";
	return false;
}

bool c_sc(u32 opcode, BasicBlock* BB, std::string& func)
{
	//unimplemented
	func += "/* unimpl. SC */\n";
	return false;
}

bool c_break(u32 opcode, BasicBlock* BB, std::string& func)
{
	//todo
	func += "/* unimpl. BREAK */\n";
	return true;
}

bool c_syscall(u32 opcode, BasicBlock* BB, std::string& func)
{
	//todo
	func += "/* unimpl. SYSCALL */\n";
	return true;
}

bool c_j(u32 opcode, BasicBlock* BB, std::string& func)
{
	u32 orig = BB->end_addr;
	BB->end_addr += 4;

	u32 target = (opcode<<2)&0x0FFFFFFF;
	target |= BB->end_addr & 0xF0000000;

	opcode = read32(BB->end_addr);
	bool is_jump = compile_op(opcode, BB, func);
	if( is_jump )
	{
		printf("Fatal error: branch/jump in delay slot! @%x under @%x\n", BB->end_addr, orig);
		exit(1);
	}

	func += fmt::format("return {0:#x}; /*J*/\n", target);

	return true;
}

bool c_jal(u32 opcode, BasicBlock* BB, std::string& func)
{
	BB->end_addr += 4;

	u32 target = (opcode&0x03FFFFFF)<<2;
	target |= BB->end_addr & 0xF0000000;

	func += fmt::format("R[31] =(signed long long)(signed int) {0:#x};\n", BB->end_addr+4);

	u32 delay_slot = read32(BB->end_addr);
	compile_op(delay_slot, BB, func);

	func += fmt::format("return {0:#x}; /*JAL*/\n", target);
	return true;
}

bool c_jalr(u32 opcode, BasicBlock* BB, std::string& func)
{
	SPECIAL_PARTS;

	BB->end_addr += 4;

	if( rd )
	{
		func += fmt::format("R[{0}] =(signed long long)(signed int) {0:#x};\n", rd, BB->end_addr+4);
	}

	func += fmt::format("unsigned int rtemp =(int) R[{0}];\n", rs);

	u32 delay_slot = read32(BB->end_addr);
	compile_op(delay_slot, BB, func);

	func += "return rtemp; /*JALR*/\n";
	return true;
}

bool c_jr(u32 opcode, BasicBlock* BB, std::string& func)
{
	SPECIAL_PARTS;

	BB->end_addr += 4;

	func += fmt::format("unsigned int rtemp =(int) R[{0}];\n", rs);

	u32 delay_slot = read32(BB->end_addr);
	compile_op(delay_slot, BB, func);

	func += "return rtemp; /*JR*/\n";
	return true;
}

bool c_and(u32 opcode, BasicBlock* BB, std::string& func)
{
	SPECIAL_PARTS;
	if( rd )
	{
		func += fmt::format("R[{0}] = R[{1}] & R[{2}];\n", rd, rs, rt);
	}
	return false;
}

bool c_or(u32 opcode, BasicBlock* BB, std::string& func)
{
	SPECIAL_PARTS;
	if( rd )
	{
		func += fmt::format("R[{0}] = R[{1}] | R[{2}];\n", rd, rs, rt);
	}
	return false;
}

bool c_xor(u32 opcode, BasicBlock* BB, std::string& func)
{
	SPECIAL_PARTS;
	if( rd )
	{
		func += fmt::format("R[{0}] = R[{1}] ^ R[{2}];\n", rd, rs, rt);
	}
	return false;
}

bool c_nor(u32 opcode, BasicBlock* BB, std::string& func)
{
	SPECIAL_PARTS;
	if( rd )
	{
		func += fmt::format("R[{0}] = ~(R[{1}] | R[{2}]);\n", rd, rs, rt);
	}
	return false;
}

bool c_addu(u32 opcode, BasicBlock* BB, std::string& func)
{
	SPECIAL_PARTS;
	if( rd )
	{
		func += fmt::format("R[{0}] =(signed long long) (((signed int)R[{1}])+((signed int)R[{2}]));\n", rd, rs, rt);
	}
	return false;
}

bool c_subu(u32 opcode, BasicBlock* BB, std::string& func)
{
	SPECIAL_PARTS;
	if( rd )
	{
		func += fmt::format("R[{0}] =(signed long long) (((signed int)R[{1}])-((signed int)R[{2}]));\n", rd, rs, rt);
	}
	return false;
}

bool c_dmult(u32 opcode, BasicBlock* BB, std::string& func)
{
	SPECIAL_PARTS;
	func += fmt::format("asm(\"imul %%rdx\" : \"a\"(R[99]), \"d\"(R[98]) : \"a\"(R[{0}]), \"d\"(R[{1}]));\n", rs, rt);
	return false;
}

bool c_dmultu(u32 opcode, BasicBlock* BB, std::string& func)
{
	SPECIAL_PARTS;
	func += fmt::format("asm(\"mul %%rdx\" : \"a\"(R[99]), \"d\"(R[98]) : \"a\"(R[{0}]), \"d\"(R[{1}]));\n", rs, rt);
	return false;
}

bool c_daddu(u32 opcode, BasicBlock* BB, std::string& func)
{
	SPECIAL_PARTS;
	if( rd )
	{
		func += fmt::format("R[{0}] = R[{1}] + R[{2}];\n", rd, rs, rt);
	}
	return false;
}

bool c_dsubu(u32 opcode, BasicBlock* BB, std::string& func)
{
	SPECIAL_PARTS;
	if( rd )
	{
		func += fmt::format("R[{0}] = R[{1}] - R[{2}];\n", rd, rs, rt);
	}
	return false;
}

bool c_lui(u32 opcode, BasicBlock* BB, std::string& func)
{
	OPCODE_PARTS;
	if( rt )
	{
		func += fmt::format("R[{0}] = (signed long long)(signed int) {1:#x};\n", rt, (u32)(offset<<16));
	}
	return false;
}

bool c_andi(u32 opcode, BasicBlock* BB, std::string& func)
{
	OPCODE_PARTS;
	if( rt )
	{
		func += fmt::format("R[{0}] = R[{1}] & {2:#x};\n", rt, rs, (u16)offset);
	}
	return false;
}

bool c_ori(u32 opcode, BasicBlock* BB, std::string& func)
{
	OPCODE_PARTS;
	if( rt )
	{
		func += fmt::format("R[{0}] = R[{1}] | {2:#x};\n", rt, rs, (u16)offset);
	}
	return false;
}

bool c_xori(u32 opcode, BasicBlock* BB, std::string& func)
{
	OPCODE_PARTS;
	if( rt )
	{
		func += fmt::format("R[{0}] = R[{1}] ^ {2:#x};\n", rt, rs, (u16)offset);
	}
	return false;
}

bool c_srlv(u32 opcode, BasicBlock* BB, std::string& func)
{
	SPECIAL_PARTS;
	if( rd )
	{
		func += fmt::format("R[{0}] =(signed long long)(signed int) (((unsigned int)R[{1}])>>(R[{2}]&0x1F));\n", rd, rt, rs);
	}
	return false;
}

bool c_slti(u32 opcode, BasicBlock* BB, std::string& func)
{
	OPCODE_PARTS;
	if( rt )
	{
		func += fmt::format("R[{0}] = ((signed long long)R[{1}]) < {0}LL;\n", rt, rs, (s64)(s16)offset);
	}
	return false;
}

bool c_sltiu(u32 opcode, BasicBlock* BB, std::string& func)
{
	OPCODE_PARTS;
	if( rt )
	{
		func += fmt::format("R[{0}] = R[{1}] < {0:#x}ULL;\n", rt, rs, (u64)(s64)(s16)offset);
	}
	return false;
}

bool c_slt(u32 opcode, BasicBlock* BB, std::string& func)
{
	SPECIAL_PARTS;
	if( rd )
	{
		func += fmt::format("R[{0}] = ((signed long long)R[{1}]) < ((signed long long)R[{2}]);\n", rd, rs, rt);
	}
	return false;
}

bool c_sltu(u32 opcode, BasicBlock* BB, std::string& func)
{
	SPECIAL_PARTS;
	if( rd )
	{
		func += fmt::format("R[{0}] = R[{1}] < R[{2}];\n", rd, rs, rt);
	}
	return false;
}

bool c_sllv(u32 opcode, BasicBlock* BB, std::string& func)
{
	SPECIAL_PARTS;
	if( rd )
	{
		func += fmt::format("R[{0}] = (signed long long)( ((unsigned int)R[{1}]) << (R[{2}]&0x1F) );\n", rd, rt, rs);
	}
	return false;
}

bool c_srav(u32 opcode, BasicBlock* BB, std::string& func)
{
	SPECIAL_PARTS;
	if( rd )
	{
		func += fmt::format("R[{0}] = (signed long long)( ((signed int)R[{1}]) >> (R[{2}]&0x1F) );\n", rd, rt, rs);
	}
	return false;
}

bool c_srl(u32 opcode, BasicBlock* BB, std::string& func)
{
	SPECIAL_PARTS;
	int sa = (opcode>>6)&0x1F;
	if( rd )
	{
		func += fmt::format("R[{0}] = (signed long long)( ((unsigned int)R[{1}]) >> {2} );\n", rd, rt, sa);
	}
	return false;
}

bool c_sll(u32 opcode, BasicBlock* BB, std::string& func)
{
	SPECIAL_PARTS;
	int sa = (opcode>>6)&0x1F;
	if( rd )
	{
		func += fmt::format("R[{0}] =(signed long long) ( ((unsigned int)R[{1}]) << {2} );\n", rd, rt, sa);
	}
	return false;
}

bool c_dsllv(u32 opcode, BasicBlock* BB, std::string& func)
{
	SPECIAL_PARTS;
	if( rd )
	{
		func += fmt::format("R[{0}] = R[{1}] << (R[{2}] & 0x3F);\n", rd, rt, rs);
	}
	return false;
}

bool c_dsrlv(u32 opcode, BasicBlock* BB, std::string& func)
{
	SPECIAL_PARTS;
	if( rd )
	{
		func += fmt::format("R[{0}] = R[{1}] >> (R[{2}] & 0x3F);\n", rd, rt, rs);
	}
	return false;
}

bool c_dsrav(u32 opcode, BasicBlock* BB, std::string& func)
{
	SPECIAL_PARTS;
	if( rd )
	{
		func += fmt::format("R[{0}] = ((signed long long) R[{1}]) >> (R[{2}] & 0x3F);\n", rd, rt, rs);
	}
	return false;
}

bool c_dsll(u32 opcode, BasicBlock* BB, std::string& func)
{
	SPECIAL_PARTS;
	int sa = (opcode>>6)&0x1F;
	if( rd )
	{
		func += fmt::format("R[{0}] = R[{1}] << {2};\n", rd, rt, sa);
	}
	return false;
}

bool c_dsll32(u32 opcode, BasicBlock* BB, std::string& func)
{
	SPECIAL_PARTS;
	int sa = 32 + ((opcode>>6)&0x1F);
	if( rd )
	{
		func += fmt::format("R[{0}] = R[{1}] << {2};\n", rd, rt, sa);
	}
	return false;
}

bool c_sra(u32 opcode, BasicBlock* BB, std::string& func)
{
	SPECIAL_PARTS;
	int sa = (opcode>>6)&0x1F;
	if( rd )
	{
		func += fmt::format("R[{0}] = (signed long long)( ((signed int)R[{1}]) >> {2} );\n", rd, rt, sa);
	}
	return false;
}

bool c_dsra(u32 opcode, BasicBlock* BB, std::string& func)
{
	SPECIAL_PARTS;
	int sa = (opcode>>6)&0x1F;
	if( rd )
	{
		func += fmt::format("R[{0}] = ((signed long long) R[{1}]) >> {2};\n", rd, rt, sa);
	}
	return false;
}

bool c_dsrl(u32 opcode, BasicBlock* BB, std::string& func)
{
	SPECIAL_PARTS;
	int sa = (opcode>>6)&0x1F;
	if( rd )
	{
		func += fmt::format("R[{0}] = R[{1}] >> {2};\n", rd, rt, sa);
	}
	return false;
}

bool c_dsrl32(u32 opcode, BasicBlock* BB, std::string& func)
{
	SPECIAL_PARTS;
	int sa = 32 + ((opcode>>6)&0x1F);
	if( rd )
	{
		func += fmt::format("R[{0}] = R[{1}] >> {2};\n", rd, rt, sa);
	}
	return false;
}

bool c_dsra32(u32 opcode, BasicBlock* BB, std::string& func)
{
	SPECIAL_PARTS;
	int sa = 32 + ((opcode>>6)&0x1F);
	if( rd )
	{
		func += fmt::format("R[{0}] = ((signed long long) R[{1}]) >> {2};\n", rd, rt, sa);
	}
	return false;
}

bool c_mfhi(u32 opcode, BasicBlock* BB, std::string& func)
{
	SPECIAL_PARTS;
	if( rd )
	{
		func += fmt::format("R[{0}] = R[98];\n", rd);
	}
	return false;
}

bool c_mflo(u32 opcode, BasicBlock* BB, std::string& func)
{
	SPECIAL_PARTS;
	if( rd )
	{
		func += fmt::format("R[{0}] = R[99];\n", rd);
	}
	return false;
}

bool c_mthi(u32 opcode, BasicBlock* BB, std::string& func)
{
	SPECIAL_PARTS;
	func += fmt::format("R[98] = R[{0}];\n", rs);
	return false;
}

bool c_mtlo(u32 opcode, BasicBlock* BB, std::string& func)
{
	SPECIAL_PARTS;
	func += fmt::format("R[99] = R[{0}];\n", rs);
	return false;
}

bool c_lb(u32 opcode, BasicBlock* BB, std::string& func)
{
	OPCODE_PARTS;
	if( rt == 0 ) return false;

	func += fmt::format("{{ int addr = {0} + (unsigned int)R[{1}];\n"
			    "int temp = check_vaddr(addr, {2:#x});\n"
			    "if( temp != {2:#x} ) return temp;\n"
			    "R[{3}] = (signed long long)(signed char)read8(addr); }}\n",
				(s32)(s16)offset, rs, BB->end_addr, rt);

	return false;
}

bool c_lbu(u32 opcode, BasicBlock* BB, std::string& func)
{
	OPCODE_PARTS;
	if( rt == 0 ) return false;

	func += fmt::format("{{ int addr = {0} + (int)R[{1}];\n"
			    "int temp = check_vaddr(addr, {2:#x});\n"
			    "if( temp != {2:#x} ) return temp;\n"
			    "R[{3}] = (unsigned long long)read8(addr); }}\n",
				(s32)(s16)offset, rs, BB->end_addr, rt);

	return false;
}

bool c_lh(u32 opcode, BasicBlock* BB, std::string& func)
{
	OPCODE_PARTS;
	if( rt == 0 ) return false;

	func += fmt::format("{{ int addr = {0} + (int)R[{1}];\n"
			    "int temp = check_vaddr(addr, {2:#x});\n"
			    "if( temp != {2:#x} ) return temp;\n"
			    "R[{3}] = (signed long long)(signed short)read16(addr); }}\n",
				(s32)(s16)offset, rs, BB->end_addr, rt);

	return false;
}

bool c_lhu(u32 opcode, BasicBlock* BB, std::string& func)
{
	OPCODE_PARTS;
	if( rt == 0 ) return false;

	func += fmt::format("{{ int addr = {0:#x} + (int)R[{1}];\n"
			    "int temp = check_vaddr(addr, {2:#x});\n"
			    "if( temp != {2:#x} ) return temp;\n"
			    "R[{3}] = (unsigned long long)(unsigned short)read16(addr); }}\n",
				(u32)(s32)(s16)offset, rs, BB->end_addr, rt);
	return false;
}

bool c_lw(u32 opcode, BasicBlock* BB, std::string& func)
{
	OPCODE_PARTS;
	if( rt == 0 ) return false;

	func += fmt::format("{{ int addr = {0} + (int)R[{1}];\n"
			    "int temp = check_vaddr(addr, {2:#x});\n"
			    "if( temp != {2:#x} ) return temp;\n"
			    "R[{3}] = (signed long long)(signed int)read32(addr); }}\n",
				(s16)offset, rs, BB->end_addr, rt);

	return false;
}

bool c_ld(u32 opcode, BasicBlock* BB, std::string& func)
{
	OPCODE_PARTS;
	if( rt == 0 ) return false;

	func += fmt::format("{{ int addr = {0} + (int)R[{1}];\n"
			    "int temp = check_vaddr(addr, {2:#x});\n"
			    "if( temp != {2:#x} ) return temp;\n"
			    "R[{3}] = read64(addr); }}\n",
				(s16)offset, rs, BB->end_addr, rt);

	return false;
}

bool c_sb(u32 opcode, BasicBlock* BB, std::string& func)
{
	OPCODE_PARTS;

	func += fmt::format("{{ int addr = {0} + (int)R[{1}];\n"
			    "int temp = check_vaddr(addr, {2:#x});\n"
			    "if( temp != {2:#x} ) return temp;\n"
			    "write8(addr, (unsigned char)R[{3}]); }}\n",
				(s32)(s16)offset, rs, BB->end_addr, rt);

	return false;
}

bool c_sh(u32 opcode, BasicBlock* BB, std::string& func)
{
	OPCODE_PARTS;

	func += fmt::format("{{ int addr = {0} + (int)R[{1}];\n"
			    "int temp = check_vaddr(addr, {2:#x});\n"
			    "if( temp != {2:#x} ) return temp;\n"
			    "write16(addr, (unsigned short)R[{3}]); }}\n",
				(s32)(s16)offset, rs, BB->end_addr, rt);

	return false;
}

bool c_sw(u32 opcode, BasicBlock* BB, std::string& func)
{
	OPCODE_PARTS;

	func += fmt::format("{{ int addr = {0} + (int)R[{1}];\n"
			    "int temp = check_vaddr(addr, {2:#x});\n"
			    "if( temp != {2:#x} ) return temp;\n"
			    "write32(addr, (unsigned int)R[{3}]); }}\n",
				(s32)(s16)offset, rs, BB->end_addr, rt);

	return false;
}

bool c_sd(u32 opcode, BasicBlock* BB, std::string& func)
{
	OPCODE_PARTS;

	func += fmt::format("{{ int addr = {0} + (int)R[{1}];\n"
			    "int temp = check_vaddr(addr, {2:#x});\n"
			    "if( temp != {2:#x} ) return temp;\n"
			    "write64(addr, R[{3}]); }}\n",
				(s32)(s16)offset, rs, BB->end_addr, rt);

	return false;
}


bool c_lwu(u32 opcode, BasicBlock* BB, std::string& func)
{
	OPCODE_PARTS;
	if( rt == 0 ) return false;

	func += fmt::format("{{ int addr = {0} + (int)R[{1}];\n"
			    "int temp = check_vaddr(addr, {2:#x});\n"
			    "if( temp != {2:#x} ) return temp;\n"
			    "R[{3}] = (unsigned long long)(unsigned int)read32(addr); }}\n",
				(s32)(s16)offset, rs, BB->end_addr, rt);

	return false;
}

bool c_daddiu(u32 opcode, BasicBlock* BB, std::string& func)
{
	OPCODE_PARTS;
	if( rt == 0 ) return false;

	func += fmt::format("R[{0}] = R[{1}] + {2:#x}ULL;\n", rt, rs, (u64)(s64)(s16)offset);
	return false;
}

bool c_addiu(u32 opcode, BasicBlock* BB, std::string& func)
{
	OPCODE_PARTS;
	if( rt == 0 ) return false;

	func += fmt::format("R[{0}] = (signed long long)(signed int)(((unsigned int)R[{1}]) + {2:#x});\n", rt, rs, (u32)(s32)(s16)offset);
	return false;
}

bool c_mult(u32 opcode, BasicBlock* BB, std::string& func)
{
	SPECIAL_PARTS;

	func += fmt::format("{{signed long long temp = ((signed long long)(signed int)R[{0}]) * "
			    "((signed long long)(signed int)R[{1}]);\n"
			    "R[98] = (signed long long)(signed int)(temp>>32);\n"
			    "R[99] = (signed long long)(signed int)temp;}}\n"
					, rs, rt);
	return false;
}

bool c_div(u32 opcode, BasicBlock* BB, std::string& func)
{
	SPECIAL_PARTS;

	func += fmt::format("if( R[{1}] == 0 ) {{ R[99] = (R[{0}]>>63) ? 1 : -1ull; R[98] =(signed long long)(signed int)R[{0}]; }} else {{\n"
			    "asm(\"movslq %%eax, %%rax\nmovslq %%esi, %%rsi\n.byte 0x48, 0x99\nidiv %%rsi\n\" : \"a\"(R[99]), \"d\"(R[98]) : \"a\"(R[{0}]), \"S\"(R[{1}]) );}}\n"
			    "R[99] = (signed long long)(signed int)R[99];\n"
			    "R[98] = (signed long long)(signed int)R[98];\n", rs, rt);

/*	
	As far as I can tell, this should work. I suspect a bug setting up divides in libtcc.
*/
//	func += fmt::format("if( R[{1}] == 0 ) {{ R[99] = (R[{0}]>>63) ? 1 : -1ull; R[98] =(signed long long)(signed int)R[{0}]; }} else {{\n"
//			    "R[99] =(signed long long) (((signed int)R[{0}]) / ((signed int)R[{1}]));\n"
//			    "R[98] =(signed long long) (((signed int)R[{0}]) % ((signed int)R[{1}]));}}\n"
//				, rs, rt);

	return false;
}

bool c_divu(u32 opcode, BasicBlock* BB, std::string& func)
{
	SPECIAL_PARTS;
	func += fmt::format("if( R[{1}] == 0 ) {{ R[99] = -1ull; R[98] =(signed long long)(signed int)R[{0}]; }} else {{\n"
			    "R[99] =(unsigned long long) (((unsigned int)R[{0}]) / ((unsigned int)R[{1}]));\n"
			    "R[98] =(unsigned long long) (((unsigned int)R[{0}]) % ((unsigned int)R[{1}]));}}\n"
				, rs, rt);

	return false;
}

bool c_ddiv(u32 opcode, BasicBlock* BB, std::string& func)
{
	SPECIAL_PARTS;

	func += fmt::format("if( R[{1}] == 0 ) {{ R[99] = -1ull; R[98] =(signed long long)(signed int)R[{0}]; }} else {{\n"
			    "asm(\"cdq\nidiv %%rsi\n\" : \"a\"(R[99]), \"d\"(R[98]) : \"a\"(R[{0}]), \"S\"(R[{1}]) );}}\n", rs, rt);

//	func += fmt::format("if( R[{1}] == 0 ) R[{1}] = 1;\n"
//			    "R[99] = ((signed long long)R[{0}]) / ((signed long long)R[{1}]);\n"
//			    "R[98] = ((signed long long)R[{0}]) % ((signed long long)R[{1}]);\n", rs, rt);

	return false;
}

bool c_ddivu(u32 opcode, BasicBlock* BB, std::string& func)
{
	SPECIAL_PARTS;
	func += fmt::format("if( R[{1}] == 0 ) {{ R[99] = -1ull; R[98] = R[{0}]; }} else {{\nR[99] = R[{0}] / R[{1}];\nR[98] = R[{0}] % R[{1}];}}\n", rs, rt);
	return false;
}

bool c_multu(u32 opcode, BasicBlock* BB, std::string& func)
{
	SPECIAL_PARTS;

	func += fmt::format("{{unsigned long long temp = "
			    "((unsigned long long)(unsigned int)R[{0}])*((unsigned long long)(unsigned int)R[{1}]);\n"
			    "R[98] = (signed long long)(signed int)(temp>>32);\nR[99] = (signed long long)(signed int)temp;}}\n"
					, rs, rt);

	return false;
}

bool c_mfc0(u32 opcode, BasicBlock* BB, std::string& func)
{
	SPECIAL_PARTS;  // not actually SPECIAL, but uses rt and rd that match that opcode format
	if( rt ) 
	{
		func += fmt::format("R[{0}] = (signed long long)(signed int) R[64 + {1}];\n", rt, rd);
	}
	return false;
}

bool c_mtc0(u32 opcode, BasicBlock* BB, std::string& func)
{
	SPECIAL_PARTS;  // not actually SPECIAL, but uses rt and rd that match that opcode format
	func += fmt::format("R[64 + {0}] = (int) R[{1}];\n", rd, rt);
	return false;
}

bool c_dmfc0(u32 opcode, BasicBlock* BB, std::string& func)
{
	func += "/* unimplemented dmfc0 */\n";
	return false;
}

bool c_dmtc0(u32 opcode, BasicBlock* BB, std::string& func)
{
	func += "/* unimplemented dmtc0 */\n";
	return false;
}

bool c_cfc0(u32 opcode, BasicBlock* BB, std::string& func)
{
	func += "/* invalid opcode cfc0 */\n";
	return false;
}

bool c_ctc0(u32 opcode, BasicBlock* BB, std::string& func)
{
	func += "/* invalid opcode ctc0 */\n";
	return false;
}

bool c_eret(u32 opcode, BasicBlock* BB, std::string& func)
{
	//don't need any info from the opcode encoding for this one

	func += "if( R[76]&4 ) {{ R[76] ^= 4; return (int) R[94]; }}\n"
		"R[76] &= ~2; return (int) R[78];\n";
	return true;
}

bool c_mfc1(u32 opcode, BasicBlock* BB, std::string& func)
{
	SPECIAL_PARTS;
	if( !rt ) return false;
	func += fmt::format("if( R[76]&(1<<26) ) R[{0}] = (long long)(int)R[{1}];\n"
			    "else R[{0}] = (long long)(int)*(long long*)(32+{1}+(int*)R);\n"
				, rt, rd+32);
	return false;
}

bool c_mtc1(u32 opcode, BasicBlock* BB, std::string& func)
{
	SPECIAL_PARTS;
	func += fmt::format("if( R[76]&(1<<26) ) *(int*)&R[{0}] = (int)R[{1}];\n"
			    "else *(32+{0}+(int*)R) = (int)R[{1}];\n"
				, rd+32, rt);
	return false;
}

bool c_dmfc1(u32 opcode, BasicBlock* BB, std::string& func)
{ //sceptical this (and dmtc1) is correct
	SPECIAL_PARTS;
	if( !rt ) return false;
	func += fmt::format("if( R[76]&(1<<26) ) R[{0}] = R[{1}];\n"
			    "else R[{0}] = *(long long*)(32+{1}+(int*)R);\n"
				, rt, rd+32);

	return false;
}

bool c_dmtc1(u32 opcode, BasicBlock* BB, std::string& func)
{ //sceptical this (and dmfc1) is correct
	SPECIAL_PARTS;
	func += fmt::format("if( R[76]&(1<<26) ) R[{0}] = R[{1}];\n"
			    "else *(long long*)(32+{0}+(int*)R) = R[{1}];\n"
				, rd+32, rt);
	return false;
}

bool c_ctc1(u32 opcode, BasicBlock* BB, std::string& func)
{
	SPECIAL_PARTS;
	if( rd == 31 ) 
	{
		func += fmt::format("R[97] = R[{0}];\n", rt);
	}

	return false;
}

bool c_cfc1(u32 opcode, BasicBlock* BB, std::string& func)
{
	SPECIAL_PARTS;
	if( !rt ) return false;
	if( rd == 0 )
	{
		func += ""; // the version register doesn't matter much
	} else if( rd == 31 ) {
		func += fmt::format("R[{0}] =(long long)(int)R[97];\n", rt);
	}
	return false;
}

bool c_cop1S(u32 opcode, BasicBlock* BB, std::string& func)
{
	FPU_PARTS;
	switch( mop )
	{
	case 0: //add.s
		func += fmt::format("if( R[76]&(1<<26) ) *(float*)(&R[{0}]) = *(float*)(&R[{1}]) + *(float*)(&R[{2}]);\n"
				    "else *(float*)({0}+(int*)R) = *(float*)({1}+(int*)R) + *(float*)({2}+(int*)R);\n"
						, fd, fs, ft);
		break;
	case 1: //sub.s
		func += fmt::format("if( R[76]&(1<<26) ) *(float*)(&R[{0}]) = *(float*)(&R[{1}]) - *(float*)(&R[{2}]);\n"
				    "else *(float*)({0}+(int*)R) = *(float*)({1}+(int*)R) - *(float*)({2}+(int*)R);\n"
						, fd, fs, ft);
		break;
	case 2: //mul.s
		func += fmt::format("if( R[76]&(1<<26) ) *(float*)(&R[{0}]) = *(float*)(&R[{1}]) * *(float*)(&R[{2}]);\n"
				    "else *(float*)({0}+(int*)R) = *(float*)({1}+(int*)R) * *(float*)({2}+(int*)R);\n"
						, fd, fs, ft);
		break;
	case 3: //div.s
		func += fmt::format("if( R[76]&(1<<26) ) *(float*)(&R[{0}]) = *(float*)(&R[{1}]) / *(float*)(&R[{2}]);\n"
				    "else *(float*)({0}+(int*)R) = *(float*)({1}+(int*)R) / *(float*)({2}+(int*)R);\n"
						, fd, fs, ft);
		break;
	case 4: //sqrt.s
		func += fmt::format("if( R[76]&(1<<26) ) *(float*)(&R[{0}]) = sqrt( *(float*)(&R[{1}]) );\n"
				    "else *(float*)({0}+(int*)R) = sqrt( *(float*)({1}+(int*)R) );\n"
						, fd, fs);
		break;
	case 5: //abs.s
		func += fmt::format("if( R[76]&(1<<26) ) *(int*)(&R[{0}]) = ~(1ull<<31) & *(int*)(&R[{1}]);\n"
				    "else *(float*)({0}+(int*)R) = ~(1ull<<31) & ( *(int*)({1}+(int*)R) );\n"
						, fd, fs);
		break;
	case 6: //mov.s
		func += fmt::format("if( R[76]&(1<<26) ) *(float*)(&R[{0}]) = ( *(float*)(&R[{1}]) );\n"
				    "else *(float*)({0}+(int*)R) = ( *(float*)({1}+(int*)R) );\n"
						, fd, fs);
		break;
	case 7: //neg.s
		func += fmt::format("if( R[76]&(1<<26) ) *(float*)(&R[{0}]) = -( *(float*)(&R[{1}]) );\n"
				    "else *(float*)({0}+(int*)R) = -( *(float*)({1}+(int*)R) );\n"
						, fd, fs);
		break;
	case 8: //round.l.s
		func += fmt::format("if( R[76]&(1<<26) ) R[{0}] = llround( *(float*)(&R[{1}]) );\n"
				    "else *(long long*)({0}+(int*)R) = llround( *(float*)({1}+(int*)R) );\n"
						, fd, fs);
		break;
	case 9: //trunc.l.s
		func += fmt::format("if( R[76]&(1<<26) ) R[{0}] =(long long) trunc( *(float*)(&R[{1}]) );\n"
				    "else *(long long*)({0}+(int*)R) =(long long) trunc( *(float*)({1}+(int*)R) );\n"
						, fd, fs);
		break;
	case 10: //ceil.l.s
		func += fmt::format("if( R[76]&(1<<26) ) R[{0}] =(long long) ceil( *(float*)(&R[{1}]) );\n"
				    "else *(long long*)({0}+(int*)R) =(long long) ceil( *(float*)({1}+(int*)R) );\n"
						, fd, fs);
		break;
	case 11: //floor.l.s
		func += fmt::format("if( R[76]&(1<<26) ) R[{0}] =(long long) floor( *(float*)(&R[{1}]) );\n"
				    "else *(long long*)({0}+(int*)R) =(long long) floor( *(float*)({1}+(int*)R) );\n"
						, fd, fs);
		break;
	case 12: //round.w.s
		func += fmt::format("if( R[76]&(1<<26) ) R[{0}] =(int) llround( *(float*)(&R[{1}]) );\n"
				    "else *(long long*)({0}+(int*)R) =(int) llround( *(float*)({1}+(int*)R) );\n"
						, fd, fs);
		break;
	case 13: //trunc.w.s
		func += fmt::format("if( R[76]&(1<<26) ) R[{0}] =(int) trunc( *(float*)(&R[{1}]) );\n"
				    "else *(long long*)({0}+(int*)R) =(int) trunc( *(float*)({1}+(int*)R) );\n"
						, fd, fs);
		break;
	case 14: //ceil.w.s
		func += fmt::format("if( R[76]&(1<<26) ) R[{0}] =(int) ceil( *(float*)(&R[{1}]) );\n"
				    "else *(long long*)({0}+(int*)R) =(int) ceil( *(float*)({1}+(int*)R) );\n"
						, fd, fs);
		break;
	case 15: //floor.w.s
		func += fmt::format("if( R[76]&(1<<26) ) R[{0}] =(int) floor( *(float*)(&R[{1}]) );\n"
				    "else *(long long*)({0}+(int*)R) =(int) floor( *(float*)({1}+(int*)R) );\n"
						, fd, fs);
		break;
	case 33: //cvt.d.s
		func += fmt::format("if( R[76]&(1<<26) ) *(double*)&R[{0}] = (double)( *(float*)(&R[{1}]) );\n"
				    "else *(double*)({0}+(int*)R) =(double) ( *(float*)({1}+(int*)R) );\n"
						, fd, fs);
		break;
	case 36: //cvt.w.s
		func += fmt::format("if( R[76]&(1<<26) ) R[{0}] = (int) *(float*)&R[{1}];\n"
				    "else *(long long*)({0}+(int*)R) =(int) ( *(float*)({1}+(int*)R) );\n"
						, fd, fs);
		break;
	case 37: //cvt.l.s
		func += fmt::format("if( R[76]&(1<<26) ) R[{0}] =(long long) ( *(float*)(&R[{1}]) );\n"
				    "else *(long long*)({0}+(int*)R) =(long long) ( *(float*)({1}+(int*)R) );\n"
						, fd, fs);
		break;
	case 48: //c.f.d
		break;
	case 49: //c.un.d
		break;
	case 50: //c.eq.d
		func += fmt::format("{{int cond = (*(float*)&R[{0}]) == (*(float*)&R[{1}]);\nR[97]=(R[97]&~(1<<23))|(cond?(1<<23):0);}}\n", fs, ft);
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
		func += fmt::format("{{int cond = (*(float*)&R[{0}]) == (*(float*)&R[{1}]);\nR[97]=(R[97]&~(1<<23))|(cond?(1<<23):0);}}\n", fs, ft);
		break;
	case 60: //c.lt.d
	case 61: //c.nge.d
		func += fmt::format("{{int cond = (*(float*)&R[{0}]) < (*(float*)&R[{1}]);\nR[97]=(R[97]&~(1<<23))|(cond?(1<<23):0);}}\n", fs, ft);
		break;
	case 62: //c.le.d
	case 63: //c.ngt.d
		func += fmt::format("{{int cond = (*(float*)&R[{0}]) <= (*(float*)&R[{1}]);\nR[97]=(R[97]&~(1<<23))|(cond?(1<<23):0);}}\n", fs, ft);
		break;

	}
	return false;
}

bool c_cop1D(u32 opcode, BasicBlock* BB, std::string& func)
{
	FPU_PARTS;
	switch( mop )
	{
	case 0: //add.d
		func += fmt::format("if( R[76]&(1<<26) ) *(double*)(&R[{0}]) = *(double*)(&R[{1}]) + *(double*)(&R[{2}]);\n"
				    "else *(double*)({0}+(int*)R) = *(double*)({1}+(int*)R) + *(double*)({2}+(int*)R);\n"
						, fd, fs, ft);
		break;
	case 1: //sub.d
		func += fmt::format("if( R[76]&(1<<26) ) *(double*)(&R[{0}]) = *(double*)(&R[{1}]) - *(double*)(&R[{2}]);\n"
				    "else *(double*)({0}+(int*)R) = *(double*)({1}+(int*)R) - *(double*)({2}+(int*)R);\n"
						, fd, fs, ft);
		break;
	case 2: //mul.d
		func += fmt::format("if( R[76]&(1<<26) ) *(double*)(&R[{0}]) = *(double*)(&R[{1}]) * *(double*)(&R[{2}]);\n"
				    "else *(double*)({0}+(int*)R) = *(double*)({1}+(int*)R) * *(double*)({2}+(int*)R);\n"
						, fd, fs, ft);
		break;
	case 3: //div.d
		func += fmt::format("if( R[76]&(1<<26) ) *(double*)(&R[{0}]) = *(double*)(&R[{1}]) / *(double*)(&R[{2}]);\n"
				    "else *(double*)({0}+(int*)R) = *(double*)({1}+(int*)R) / *(double*)({2}+(int*)R);\n"
						, fd, fs, ft);
		break;
	case 4: //sqrt.d
		func += fmt::format("if( R[76]&(1<<26) ) *(double*)(&R[{0}]) = sqrt( *(double*)(&R[{1}]) );\n"
				    "else *(double*)({0}+(int*)R) = sqrt( *(double*)({1}+(int*)R) );\n"
						, fd, fs);
		break;
	case 5: //abs.d
		func += fmt::format("if( R[76]&(1<<26) ) R[{0}] = R[{1}]&~(1ull<<63);\n"  //" *(double*)(&R[{0}]) = abs( *(double*)(&R[{1}]) );\n"
				    "else *(long long*)({0}+(int*)R) = *(long long*)({1}+(int*)R) &~(1ull<<63);\n" // "abs( *(double*)({1}+(int*)R) );\n"
						, fd, fs);
		break;
	case 6: //mov.d
		func += fmt::format("if( R[76]&(1<<26) ) *(double*)(&R[{0}]) = ( *(double*)(&R[{1}]) );\n"
				    "else *(double*)({0}+(int*)R) = ( *(double*)({1}+(int*)R) );\n"
						, fd, fs);
		break;
	case 7: //neg.d
		func += fmt::format("if( R[76]&(1<<26) ) *(double*)(&R[{0}]) = -( *(double*)(&R[{1}]) );\n"
				    "else *(double*)({0}+(int*)R) = -( *(double*)({1}+(int*)R) );\n"
						, fd, fs);
		break;
	case 8: //round.l.d
		func += fmt::format("if( R[76]&(1<<26) ) R[{0}] = llround( *(double*)(&R[{1}]) );\n"
				    "else *(long long*)({0}+(int*)R) = llround( *(double*)({1}+(int*)R) );\n"
						, fd, fs);
		break;
	case 9: //trunc.l.d
		func += fmt::format("if( R[76]&(1<<26) ) R[{0}] =(long long) trunc( *(double*)(&R[{1}]) );\n"
				    "else *(long long*)({0}+(int*)R) =(long long) trunc( *(double*)({1}+(int*)R) );\n"
						, fd, fs);
		break;
	case 10: //ceil.l.d
		func += fmt::format("if( R[76]&(1<<26) ) R[{0}] =(long long) ceil( *(double*)(&R[{1}]) );\n"
				    "else *(long long*)({0}+(int*)R) =(int) ceil( *(double*)({1}+(int*)R) );\n"
						, fd, fs);
		break;
	case 11: //floor.l.d
		func += fmt::format("if( R[76]&(1<<26) ) R[{0}] =(long long) floor( *(double*)(&R[{1}]) );\n"
				    "else *(long long*)({0}+(int*)R) =(long long) floor( *(double*)({1}+(int*)R) );\n"
						, fd, fs);
		break;
	case 12: //round.w.d //what happens to upper 32bits of things?
		func += fmt::format("if( R[76]&(1<<26) ) R[{0}] =(int) llround( *(double*)(&R[{1}]) );\n"
				    "else *(long long*)({0}+(int*)R) =(int) llround( *(double*)({1}+(int*)R) );\n"
						, fd, fs);
		break;
	case 13: //trunc.w.d
		func += fmt::format("if( R[76]&(1<<26) ) R[{0}] =(int) trunc( *(double*)(&R[{1}]) );\n"
				    "else *(long long*)({0}+(int*)R) =(int) trunc( *(double*)({1}+(int*)R) );\n"
						, fd, fs);
		break;
	case 14: //ceil.w.d
		func += fmt::format("if( R[76]&(1<<26) ) R[{0}] =(int) ceil( *(double*)(&R[{1}]) );\n"
				    "else *(long long*)({0}+(int*)R) =(int) ceil( *(double*)({1}+(int*)R) );\n"
						, fd, fs);
		break;
	case 15: //floor.w.d
		func += fmt::format("if( R[76]&(1<<26) ) R[{0}] =(int) floor( *(double*)(&R[{1}]) );\n"
				    "else *(long long*)({0}+(int*)R) =(int) floor( *(double*)({1}+(int*)R) );\n"
						, fd, fs);
		break;
	case 32: //cvt.s.d
		func += fmt::format("if( R[76]&(1<<26) ) *(float*)&R[{0}] = (float) *(double*)(&R[{1}]);\n"
				    "else *({0}+(float*)R) = (float) *(double*)({1}+(int*)R);\n"
						, fd, fs);
		break;
	case 36: //cvt.w.d
		func += fmt::format("if( R[76]&(1<<26) ) *(int*)&R[{0}] = (int) *(double*)(&R[{1}]);\n"
				    "else *({0}+(int*)R) = (int) *(double*)({1}+(int*)R);\n"
						, fd, fs);
		break;
	case 37: //cvt.l.d
		func += fmt::format("if( R[76]&(1<<26) ) R[{0}] =(long long) ( *(double*)(&R[{1}]) );\n"
				    "else *(long long*)({0}+(int*)R) = (long long) *(double*)({1}+(int*)R);\n"
						, fd, fs);
		break;
	case 48: //c.f.d
		break;
	case 49: //c.un.d
		break;
	case 50: //c.eq.d
		func += fmt::format("{{int cond = (*(double*)&R[{0}]) == (*(double*)&R[{1}]);\nR[97]=(R[97]&~(1<<23))|(cond?(1<<23):0);}}\n", fs, ft);
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
		func += fmt::format("{{int cond = (*(double*)&R[{0}]) == (*(double*)&R[{1}]);\nR[97]=(R[97]&~(1<<23))|(cond?(1<<23):0);}}\n", fs, ft);
		break;
	case 60: //c.lt.d
	case 61: //c.nge.d
		func += fmt::format("{{int cond = (*(double*)&R[{0}]) < (*(double*)&R[{1}]);\nR[97]=(R[97]&~(1<<23))|(cond?(1<<23):0);}}\n", fs, ft);
		break;
	case 62: //c.le.d
	case 63: //c.ngt.d
		func += fmt::format("{{int cond = (*(double*)&R[{0}]) <= (*(double*)&R[{1}]);\nR[97]=(R[97]&~(1<<23))|(cond?(1<<23):0);}}\n", fs, ft);
		break;

	}
	return false;
}

bool c_cop1W(u32 opcode, BasicBlock* BB, std::string& func)
{
	FPU_PARTS;
	switch( mop )
	{
	case 32: //cvt.s.w
		func += fmt::format("if( R[76]&(1<<26) ) *(float*)&R[{0}] = (float) *(int*)&R[{1}];\n"
				    "else *({0}+(int*)R) = (int) *(float*)({1}+(int*)R);\n"
						, fd, fs);
		break;
	case 33: //cvt.d.w
		func += fmt::format("if( R[76]&(1<<26) ) *(double*)&R[{0}] = (double)*(int*)&R[{1}];\n"
				    "else *({0}+(float*)R) = (float) *(int*)({1}+(int*)R);\n"
						, fd, fs);
		break;
	}
	return false;
}

bool c_cop1L(u32 opcode, BasicBlock* BB, std::string& func)
{
	FPU_PARTS;
	switch( mop )
	{
	case 32: //cvt.s.l
		func += fmt::format("if( R[76]&(1<<26) ) *(float*)&R[{0}] = (float) (long long)R[{1}];\n"
				    "else *({0}+(float*)R) = (float) *(long long*)({1}+(int*)R);\n"
						, fd, fs);
		break;
	case 33: //cvt.d.l
		func += fmt::format("if( R[76]&(1<<26) ) *(double*)&R[{0}] = (double) (long long)R[{1}];\n"
				    "else *(double*)({0}+(float*)R) = (double) *(long long*)({1}+(int*)R);\n"
						, fd, fs);
		break;
	}
	return false;
}

bool c_bc1(u32 opcode, BasicBlock* BB, std::string& func)
{
	u32 subop = (opcode>>16)&0x1F;
	s32 offset =(s32) (s16)(opcode&0xffff);
	offset <<= 2;

	switch( subop )
	{
	case 0: //BC1F branch on cop1 false
		func += "{int cond = R[97]&(1<<23);\n";
		BB->end_addr += 4;
		compile_op(read32(BB->end_addr), BB, func);
		func += fmt::format("if( !cond ) return {0:#x}; }}\n", BB->end_addr+offset);		
		break;
	case 1: //BC1T
		func += "{int cond = R[97]&(1<<23);\n";
		BB->end_addr += 4;
		compile_op(read32(BB->end_addr), BB, func);
		func += fmt::format("if( cond ) return {0:#x}; }}\n", BB->end_addr+offset);		
		break;
	case 2: //BC1FL branch on cop1 false likely
		func += "if( !(R[97]&(1<<23)) ) {\n";
		BB->end_addr += 4;
		compile_op(read32(BB->end_addr), BB, func);
		func += fmt::format("return {0:#x}; }}\n", BB->end_addr+offset);		
		break;
	case 3: //BC1TL
		func += "if( R[97]&(1<<23) ) {\n";
		BB->end_addr += 4;
		compile_op(read32(BB->end_addr), BB, func);
		func += fmt::format("return {0:#x}; }}\n", BB->end_addr+offset);		
		break;
	}

	return true;
}

compptr spec_ops[] = { c_sll, undef_opcode, c_srl, c_sra, c_sllv, undef_opcode, c_srlv, c_srav,
	c_jr, c_jalr, undef_opcode, undef_opcode, c_syscall, c_break, undef_opcode, c_sync,
	c_mfhi, c_mthi, c_mflo, c_mtlo, c_dsllv, undef_opcode, c_dsrlv, c_dsrav,
	c_mult, c_multu, c_div, c_divu, c_dmult, c_dmultu, c_ddiv, c_ddivu,
	c_addu, c_addu, c_subu, c_subu, c_and, c_or, c_xor, c_nor, 
	undef_opcode, undef_opcode, c_slt, c_sltu, c_daddu, c_daddu, c_dsubu, c_dsubu,
	c_trap, c_trap, c_trap, c_trap, c_trap, undef_opcode, c_trap, undef_opcode,
	c_dsll, undef_opcode, c_dsrl, c_dsra, c_dsll32, undef_opcode, c_dsrl32, c_dsra32
};

compptr cop0_ops[] = { c_mfc0, c_dmfc0, c_cfc0, undef_opcode, c_mtc0, c_dmtc0, c_ctc0, undef_opcode,
	undef_opcode /* bcc0? */, undef_opcode,  undef_opcode,  undef_opcode,  undef_opcode,  undef_opcode,  undef_opcode,  undef_opcode, 
};

compptr cop1_ops[] = { c_mfc1, c_dmfc1, c_cfc1, undef_opcode, c_mtc1, c_dmtc1, c_ctc1, undef_opcode,
	c_bc1, undef_opcode, undef_opcode, undef_opcode, undef_opcode, undef_opcode, undef_opcode, undef_opcode,
	c_cop1S, c_cop1D, undef_opcode, undef_opcode, c_cop1W, c_cop1L, undef_opcode, undef_opcode, 
	undef_opcode, undef_opcode, undef_opcode, undef_opcode, undef_opcode, undef_opcode, undef_opcode, undef_opcode
};

bool c_cop0(u32 opcode, BasicBlock* BB, std::string& func)
{
	if( (opcode>>25) & 1 )
	{
		switch( opcode&0x3F )
		{
		//todo: tlb
		case 0x18: return c_eret(opcode, BB, func);
		default: break;
		}
		return false;
	}

	return cop0_ops[(opcode>>21)&0x1F](opcode, BB, func);
}

bool c_cop1(u32 opcode, BasicBlock* BB, std::string& func)
{
	return cop1_ops[(opcode>>21)&0x1F](opcode, BB, func);
}

bool c_cop2(u32 opcode, BasicBlock* BB, std::string& func)
{
	func += "/* COP2 Doesn't exist */\n";
	return false;
}

bool c_ldc1(u32 opcode, BasicBlock* BB, std::string& func)
{
	OPCODE_PARTS;
	func += fmt::format("{{unsigned int addr = R[{0}] + {1}LL;\n"
		    "unsigned int exc = check_vaddr(addr, {2:#x});\n"
		    "if( exc != {2:#x} ) return exc;\n"
		    "if( R[76]&(1<<26) ) R[{3}] = read64(addr);\n"
		    "else *(unsigned long long*)(32 + {3} + (unsigned int*)R) = read64(addr);}}\n"
				, rs, (s32)(s16)offset, BB->end_addr, 32 + rt);
	return false;
}

bool c_lwc1(u32 opcode, BasicBlock* BB, std::string& func)
{
	OPCODE_PARTS;
	func += fmt::format("{{unsigned int addr = R[{0}] + {1}LL;\n"
		    "unsigned int exc = check_vaddr(addr, {2:#x});\n"
		    "if( exc != {2:#x} ) return exc;\n"
		    "if( R[76]&(1<<26) ) R[{3}] = read32(addr);\n"
		    "else *(32 + {3} + (unsigned int*)R) = read32(addr);}}\n"
				, rs, (s32)(s16)offset, BB->end_addr, 32 + rt);
	return false;
}

bool c_swc1(u32 opcode, BasicBlock* BB, std::string& func)
{
	OPCODE_PARTS;
	func += fmt::format("{{unsigned int addr = R[{0}] + {1}LL;\n"
		    "unsigned int exc = check_vaddr(addr, {2:#x});\n"
		    "if( exc != {2:#x} ) return exc;\n"
		    "if( R[76]&(1<<26) ) write32(addr, R[{3}]);\n"
		    "else write32(addr, *(32 + {3} + (unsigned int*)R));}}\n"
				, rs, (s32)(s16)offset, BB->end_addr, 32 + rt);
	return false;
}

bool c_sdc1(u32 opcode, BasicBlock* BB, std::string& func)
{
	OPCODE_PARTS;
	func += fmt::format("{{unsigned int addr = R[{0}] + {1}LL;\n"
		    "unsigned int exc = check_vaddr(addr, {2:#x});\n"
		    "if( exc != {2:#x} ) return exc;\n"
		    "if( R[76]&(1<<26) ) write64(addr, R[{3}]);\n"
		    "else write64(addr, *(unsigned long long*)(32 + {3} + (unsigned int*)R));}}\n"
				, rs, (s32)(s16)offset, BB->end_addr, 32 + rt);
	return false;
}

bool c_swc2(u32 opcode, BasicBlock* BB, std::string& func)
{
	func += "/* COP2 Doesn't exist - SWC2 */\n";
	return false;
}

bool c_lwc2(u32 opcode, BasicBlock* BB, std::string& func)
{
	func += "/* COP2 Doesn't exist - LWC2 */\n";
	return false;
}

bool c_ldc2(u32 opcode, BasicBlock* BB, std::string& func)
{
	func += "/* COP2 Doesn't exist - LDC2 */\n";
	return false;
}


bool c_ldl(u32 opcode, BasicBlock* BB, std::string& func)
{
	OPCODE_PARTS;
	if( !rt ) return false;
	func += fmt::format("{{unsigned int addr = R[{0}] + {1}LL;\n"
			    "unsigned int exc = check_vaddr(addr, {2:#x});\n"
			    "if( exc != {2:#x} ) return exc;\n"
			    "if( (addr&7) == 0 ) {{ R[{3}] = read64(addr); }} else {{\n"
			    "unsigned int shft = (addr&7)<<3;\n"
			    "unsigned long long mask = (1ULL << shft) - 1;\n"
			    "R[{3}] = (R[{3}]&mask) | (read64(addr)<<shft);\n"
			    "}} }}\n", rs, (s16)offset, BB->end_addr, rt);

	return false;
}

bool c_ldr(u32 opcode, BasicBlock* BB, std::string& func)
{
	OPCODE_PARTS;
	if( !rt ) return false;
	
	func += fmt::format("{{unsigned int addr = R[{0}] + {1}LL;\n"
			    "unsigned int exc = check_vaddr(addr, {2:#x});\n"
			    "if( exc != {2:#x} ) return exc;\n"
			    "if( (addr&7) == 7 ) {{ R[{3}] = read64(addr); }} else {{\n"
			    "unsigned long long mask = (1ULL << (((addr&7)+1)<<3)) - 1;\n"
			    "unsigned int shft = (7 - (addr & 7)) << 3;\n"
			    "R[{3}] = (R[{3}]&~mask) | (read64(addr)>>shft);\n"
			    "}} }}\n", rs, (s16)offset, BB->end_addr, rt);
	return false;
}

bool c_lwl(u32 opcode, BasicBlock* BB, std::string& func)
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
	if( !rt ) return false;
	func += fmt::format("{{unsigned int addr = R[{0}] + {1};\n"
			    "unsigned int exc = check_vaddr(addr, {2:#x});\n"
			    "if( exc != {2:#x} ) return exc;\n"
			    "if( (addr&3) == 0 ) {{ R[{3}] = (signed long long)(signed int)read32(addr); }} else {{\n"
			    "unsigned int shft = (addr&3)<<3;\n"
			    "unsigned long long mask = (1ULL << shft) - 1;\n"
			    "R[{3}] = (signed long long)(signed int)((R[{3}]&mask) | (read32(addr)<<shft));\n"
			    "}} }}\n", rs, (s16)offset, BB->end_addr, rt);
	return false;
}

bool c_lwr(u32 opcode, BasicBlock* BB, std::string& func)
{
	OPCODE_PARTS;
	if( !rt ) return false;
	func += fmt::format("{{unsigned int addr = R[{0}] + {1};\n"
			    "unsigned int exc = check_vaddr(addr, {2:#x});\n"
			    "if( exc != {2:#x} ) return exc;\n"
			    "if( (addr&3) == 3 ) {{ R[{3}] = (signed long long)(signed int)read32(addr); }} else {{\n"
			    "unsigned long long mask = (1ULL << (((addr&3)+1)<<3)) - 1;\n"
			    "unsigned int shft = (3 - (addr & 3)) << 3;\n"
			    "R[{3}] = (signed long long)(signed int)((R[{3}]&~mask) | (read32(addr)>>shft));\n"
			    "}} }}\n", rs, (s16)offset, BB->end_addr, rt);
	return false;
}

bool c_swl(u32 opcode, BasicBlock* BB, std::string& func)
{
	OPCODE_PARTS;
	func += fmt::format("{{unsigned int addr = {1} + (int)R[{0}];\n"
			    "unsigned int exc = check_vaddr(addr, {2:#x});\n"
			    "if( exc != {2:#x} ) return exc;\n"
			    "if( (addr&3) == 0 ) {{ write32(addr, R[{3}]); }} else {{\n"	
			    "unsigned int mask = (1ULL << ((4 - (addr & 3)) * 8)) - 1;\n"
			    "int shift = (addr&3) * 8;\n"
			    "write32(addr, (read32(addr)&~mask) | (((unsigned int)R[{3}])>>shift));}} }}\n"
				, rs, (s32)(s16)offset, BB->end_addr, rt);

	return false;
}

bool c_swr(u32 opcode, BasicBlock* BB, std::string& func)
{
	OPCODE_PARTS;
	func += fmt::format("{{unsigned int addr = R[{0}] + {1};\n"
			    "unsigned int exc = check_vaddr(addr, {2:#x});\n"
			    "if( exc != {2:#x} ) return exc;\n"
			    "if( (addr&3) == 3 ) {{ write32(addr, R[{3}]); }} else {{\n"
			    "unsigned int shft = (3-(addr&3))<<3;\n"
			    "unsigned int mask = (1 << shft) - 1;\n"
			    "unsigned int stmp = read32(addr);\n"
			    "write32(addr, ((stmp&mask) | (R[{3}]<<shft)));\n"
			    "}} }}\n", rs, (s16)offset, BB->end_addr, rt);

	return false;
}

bool c_sdl(u32 opcode, BasicBlock* BB, std::string& func)
{
	OPCODE_PARTS;

	func += fmt::format("{{unsigned int addr = R[{0}] + {1:#x};\n"
			    "unsigned int exc = check_vaddr(addr, {2:#x});\n"
			    "if( exc != {2:#x} ) return exc;\n"
			    "if( (addr&7) == 0 ) {{ write64(addr, R[{3}]); }} else {{\n"
			    "unsigned long long mask = (1ULL << ((8 - (addr&7))*8)) - 1;\n"
			    "int shift = (addr&7)*8;\n"
			    "write64(addr, (read64(addr)&~mask) | (R[{3}]>>shift));}} }}\n"
				, rs, (u32)(s32)(s16)offset, BB->end_addr, rt);

	return false;
}

bool c_sdr(u32 opcode, BasicBlock* BB, std::string& func)
{
	OPCODE_PARTS;
	func += fmt::format("{{unsigned int addr = R[{0}] + {1:#x};\n"
			    "unsigned int exc = check_vaddr(addr, {2:#x});\n"
			    "if( exc != {2:#x} ) return exc;\n"
			    "if( (addr&7) == 7 ) {{ write64(addr, R[{3}]); }} else {{\n"
			    "unsigned long long old_word = read64(addr);\n"
			    "int new_shift = (7 - (addr & 7)) * 8;\n"
			    "unsigned long long old_mask = (1ULL << new_shift) - 1;\n"
			    "write64(addr, (old_word & old_mask) | (R[{3}] << new_shift));\n"
			    "}} }}\n", rs, (u32)(s32)(s16)offset, BB->end_addr, rt);

	return false;
}

bool c_sdc2(u32 opcode, BasicBlock* BB, std::string& func)
{
	func += "/* COP2 Doesn't exist - SDC2 */\n";
	return false;
}

bool c_scd(u32 opcode, BasicBlock* BB, std::string& func)
{
	func += "/* Unimpl. SCD */\n";
	return false;
}

bool compile_special(u32 opcode, BasicBlock* BB, std::string& func)
{
	return spec_ops[opcode & 0x3F](opcode, BB, func);
}

compptr opcodes[] = { compile_special, compile_regimm, c_j, c_jal, c_beq, c_bne, c_blez, c_bgtz,
	c_addiu, c_addiu, c_slti, c_sltiu, c_andi, c_ori, c_xori, c_lui,			
	c_cop0, c_cop1, c_cop2, undef_opcode, c_beql, c_bnel, c_blezl, c_bgtzl,
	c_daddiu, c_daddiu, c_ldl, c_ldr, undef_opcode, undef_opcode, undef_opcode, undef_opcode,
	c_lb, c_lh, c_lwl, c_lw, c_lbu, c_lhu, c_lwr, c_lwu,
	c_sb, c_sh, c_swl, c_sw, c_sdl, c_sdr, c_swr, c_cache,
	c_ll, c_lwc1, c_lwc2, undef_opcode, c_lld, c_ldc1, c_ldc2, c_ld,
	c_sc, c_swc1, c_swc2, undef_opcode, c_scd, c_sdc1, c_sdc2, c_sd
};

bool compile_op(u32 opcode, BasicBlock* BB, std::string& func)
{
	return opcodes[(opcode>>26)](opcode, BB, func);
}


