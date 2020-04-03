#pragma once
#include <cstdint>

typedef uint8_t u8;
typedef int8_t  s8;

typedef uint16_t u16;
typedef int16_t  s16;

typedef uint32_t u32;
typedef int32_t  s32;

typedef uint64_t u64;
typedef int64_t  s64;

#define BIT(a) ((1ULL<<(a)))

#pragma pack(push, 1)
struct regs
{
	u64 R[32];
	u64 FR[32];

	u64 C[32];

	u64 FC0, FC31; // 96, 97
	u64 HI, LO;  // 98, 99

	u32 PC;
};

struct rspregs
{
	u32 R[32];
	u8 V[32*16];
	u64 A[8];
	u32 PC;
	u16 VCC, VCO, VCE;
};
#pragma pack(pop)

struct BasicBlock
{
	u32 start_addr, end_addr, crc32;
	void* fptr;
	void* mem; // the pointer to the block that contains fptr. needed in order to free it.
};

struct rdp_other_modes 
{
	bool atomic_primitive;
	u32 cycle_type;
	bool perspective_correct, detail_texture, sharpen_texture;
	bool enable_texture_lod, tlut_enable;
	u32 tlut_format, sample_type;
	bool mid_texel_enable;
	bool bilinear_interp_0, bilinear_interp_1;
	bool color_convert;
	u32 rgb_dither, alpha_dither;
	u32 blend_m1a0, blend_m1a1;
	u32 blend_m1b0, blend_m1b1;
	u32 blend_m2a0, blend_m2a1;
	u32 blend_m2b0, blend_m2b1;
	bool force_blend, enable_coverage, alpha_multiply;
	u32 z_mode, coverage_mode;
	bool color_on_coverage, image_read_enable;
	bool update_z, z_compare, anti_alias;
	u32 z_source, alpha_comp_type;
	bool alpha_comp_enable;
};

const int CP0_Index = 0;
const int CP0_Random = 1;
const int CP0_EntryLo0 = 2;
const int CP0_EntryLo1 = 3;
const int CP0_Context = 4;
const int CP0_PageMask = 5;
const int CP0_Wired = 6;
const int CP0_BadVAddr = 8;
const int CP0_Count = 9;
const int CP0_EntryHi = 10;
const int CP0_Compare = 11;
const int CP0_Status = 12;
const int CP0_STATUS_ERL = 4;
const int CP0_STATUS_EXL = 2;
const int CP0_STATUS_FR = BIT(26);
const int CP0_Cause = 13;
const int CP0_EPC = 14;
const int CP0_PrID = 15;
const int CP0_ErrorEPC = 30;


