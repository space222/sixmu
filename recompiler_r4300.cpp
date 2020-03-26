#include <vector>
#include <string>
#include <cstdlib>
#include <cmath>
#include <unordered_map>
#include "fmt/format.h"
#include "types.h"
#include "libtcc.h"

extern regs cpu;
using namespace std::literals::string_literals;
typedef u32(*recompfunc)(u64*);

int run_from_elsewhere(std::vector<BasicBlock*>&, u32);
bool compile_op(u32 opcode, BasicBlock* BB, std::string& func);
u64 read64(u32);
u32 read32(u32);
u16 read16(u32);
u8 read8(u32);
void write64(u32, u64);
void write32(u32, u32);
void write16(u32, u16);
void write8(u32, u8);
u32 check_vaddr(u32, u32);
void empty_deletion_queue();

std::unordered_map<u32, BasicBlock*> blocks_by_addr;
std::vector<std::vector<BasicBlock*> > blocks_by_page((8*1024*1024)/4096);
std::vector<BasicBlock*> blocks_in_pif;
std::vector<BasicBlock*> blocks_in_spmem;
std::vector<BasicBlock*> deletionQ;
BasicBlock* bb_last_run = nullptr;

BasicBlock* recompile_at(u32 addr)
{
	//printf("about to compile @%x...\n", addr);
	u32 op = read32(addr);
	BasicBlock *BB = new BasicBlock();
	BB->start_addr = BB->end_addr = addr;
	std::string func("unsigned int check_vaddr(unsigned int, unsigned int);\n"
			   "double trunc(double);\ndouble ceil(double);\n" //float ceilf(float);\n"
			   "double sqrt(double);\ndouble floor(double);\n"
			   "long long llround(double);\ndouble abs(double);\n"
			   "void write64(unsigned int, unsigned long long);\n"
			   "void write32(unsigned int, unsigned int);\n"
			   "void write16(unsigned int, unsigned short);\n"
			   "void write8(unsigned int, unsigned char);\n"
			   "unsigned char read8(unsigned int);\n"
			   "unsigned short read16(unsigned int);\n"
			   "unsigned int read32(unsigned int);\n"
			   "unsigned long long read64(unsigned int);\n"
			   "unsigned int recomp_func(unsigned long long* R) {\n"s);
	bool is_jump = compile_op(op, BB, func);
	BB->end_addr += 4;

	while( !is_jump && (BB->end_addr-BB->start_addr < 1000) )
	{
		op = read32(BB->end_addr);
		is_jump = compile_op(op, BB, func);
		BB->end_addr += 4;
	}

	func += fmt::format("return {:#x};}}", BB->end_addr);
	//printf("done.\n");

	TCCState *s = tcc_new();
	tcc_set_output_type(s, TCC_OUTPUT_MEMORY);
	if( tcc_compile_string(s, func.c_str()) )
	{
		printf("\n\n--------------\n\n");
		printf("Compilation error, listing follows:\n%s\n", func.c_str());
		exit(1);
	}
	tcc_add_symbol(s, "write8", (void*)&write8);
	tcc_add_symbol(s, "write16", (void*)&write16);
	tcc_add_symbol(s, "write32", (void*)&write32);
	tcc_add_symbol(s, "write64", (void*)&write64);
	tcc_add_symbol(s, "read64", (void*)&read64);
	tcc_add_symbol(s, "read32", (void*)&read32);
	tcc_add_symbol(s, "read16", (void*)&read16);
	tcc_add_symbol(s, "read8", (void*)&read8);
	tcc_add_symbol(s, "sqrt", (void*)&sqrt);
	tcc_add_symbol(s, "floor", (void*)&floor);
	tcc_add_symbol(s, "ceil", (void*)&ceil);
	//tcc_add_symbol(s, "ceilf", (void*)&ceilf);
	tcc_add_symbol(s, "trunc", (void*)&trunc);
	tcc_add_symbol(s, "llround", (void*)&llround);
	tcc_add_symbol(s, "abs", (void*)&abs);
	tcc_add_symbol(s, "check_vaddr", (void*)&check_vaddr);
	BB->mem =(void*) new u8[tcc_relocate(s, nullptr)]; //malloc(tcc_relocate(s, NULL));
	tcc_relocate(s, BB->mem);
	BB->fptr = tcc_get_symbol(s, "recomp_func");
	if( BB->fptr == nullptr )
	{
		printf("fatal: tcc get_symbol error\n");
		exit(1);
	}
	tcc_delete(s);

	//511 magic value below is length of the preamble above recomp_func
	printf("/* function at 0x%x */\n%s\n", BB->start_addr, func.c_str()+511); 

	return BB;
}


int cpu_run()
{
	cpu.R[0] = 0;

	u32 maskedpc = cpu.PC & 0x1FFFFFFF;

	if( bb_last_run && bb_last_run->start_addr == maskedpc )
	{ // recently if(0)'d out
		u32 retval = bb_last_run->end_addr - bb_last_run->start_addr; 
		cpu.PC = ((recompfunc)bb_last_run->fptr)((u64*)&cpu); //bb_last_run could be nullified during this
		return retval;
	}

	if( maskedpc >= 0x1fc00000 && maskedpc < 0x1fc00800 )
	{
		return run_from_elsewhere(blocks_in_pif, cpu.PC);
	} else if( maskedpc >= 0x04000000 && maskedpc < 0x04002000 ) {
		return run_from_elsewhere(blocks_in_spmem, cpu.PC);
	}

	BasicBlock* BB = nullptr;
	auto iter = blocks_by_addr.find(maskedpc);

	if( iter == blocks_by_addr.end() )
	{
		BB = recompile_at(cpu.PC);
		blocks_by_addr.insert(std::make_pair(maskedpc, BB));

		blocks_by_page[maskedpc>>12].push_back(BB);
		if( (maskedpc>>12) != ((BB->end_addr&0x1FFFFFFF)>>12) )
			blocks_by_page[(maskedpc>>12)+1].push_back(BB);		
	} else {
		BB = iter->second;
	}

	bb_last_run = BB;
	cpu.PC = ((recompfunc)BB->fptr)((u64*)&cpu);
	return BB->end_addr - BB->start_addr;
}

int run_from_elsewhere(std::vector<BasicBlock*>& blocks, u32 addr)
{
	BasicBlock* BB = nullptr;
	for(int i = 0; i < blocks.size(); ++i)
	{
		if( blocks[i]->start_addr == addr )
		{
			BB = blocks[i];
			break;
		}
	}

	if( !BB )
	{
		BB = recompile_at(addr);
		blocks.push_back(BB);
	}

	bb_last_run = BB;
	cpu.PC = ((recompfunc)BB->fptr)((u64*)&cpu);
	return BB->end_addr - BB->start_addr;
}


void invalidate_page(u32 page)
{
	auto& c = blocks_by_page[page];

	if( (bb_last_run->start_addr&0x7fffff) <= (page<<12) && (bb_last_run->end_addr&0x7fffff) >= (page<<12) )
	{
		bb_last_run = nullptr;
	}
	
	for(BasicBlock* b : c)
	{
		if( page > 0 )
		{
			auto& b4 = blocks_by_page[page-1];
			auto iter = std::find(b4.begin(), b4.end(), b);
			if( iter != b4.end() ) b4.erase(iter);
		}
		if( page < 0x7FF )
		{
			auto& aft = blocks_by_page[page+1];
			auto iter = std::find(aft.begin(), aft.end(), b);
			if( iter != aft.end() ) aft.erase(iter);
		}
		blocks_by_addr.erase(b->start_addr);
		deletionQ.push_back(b);
	}

	empty_deletion_queue();

	return;
}

void invalidate_code(u32 addr)
{
	if( addr >= 0xa0000000 )
	{
		return;
	}

	addr &= ~3;

	if( (bb_last_run->start_addr&0x7fffff) <= addr && (bb_last_run->end_addr&0x7fffff) > addr )
	{
		bb_last_run = nullptr;
	}

	u32 page = addr>>12;
	auto& c = blocks_by_page[page];
	auto range_check = [&](auto& e) { return addr >= (e->start_addr&0x7fffff) && addr < (e->end_addr&0x7fffff); };

	auto iter = std::find_if(std::begin(c), std::end(c), range_check);
	if( iter == std::end(c) ) return;
	blocks_by_addr.erase((*iter)->start_addr&0x7FFFFC);
	c.erase(iter);

	if( page > 0 )
	{
		auto& c1 = blocks_by_page[page-1];
		auto iter2 = std::find(std::begin(c1), std::end(c1), *iter);
		if( iter2 != std::end(c1) ) 
		{
			c1.erase(iter2);
		}
	}

	if( page < 0x7FF )
	{
		auto& c1 = blocks_by_page[page+1];
		auto iter2 = std::find(std::begin(c1), std::end(c1), *iter);
		if( iter2 != std::end(c1) ) 
		{
			c1.erase(iter2);
		}
	}

	deletionQ.push_back(*iter);

	return;
}

void empty_deletion_queue()
{
	for(BasicBlock* b : deletionQ)
	{
		delete (u8*)b->mem;
		delete b;
	}

	deletionQ.clear();
	return;
}

