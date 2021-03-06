﻿// Low-level DSP core

#pragma once

#include <vector>
#include <list>
#include <map>
#include <string>
#include <atomic>
#include "../Common/Thread.h"

namespace DSP
{
	typedef uint32_t DspAddress;		// in halfwords slots 

	#pragma warning (push)
	#pragma warning (disable: 4201)

	#pragma pack (push, 1)

	union DspLongAccumulator
	{
		struct
		{
			uint16_t	l;
			union
			{
				struct
				{
					uint16_t	m;
					uint16_t	h;
				};
				uint32_t hm;
				int32_t shm;
			};
		};
		uint64_t	bits;
		int64_t		sbits;
	};

	union DspShortAccumulator
	{
		struct
		{
			uint16_t	l;
			uint16_t	h;
		};
		uint32_t	bits;
		int32_t		sbits;
	};

	struct DspProduct
	{
		struct
		{
			uint16_t l;
			uint16_t m1;
			uint16_t h;
			uint16_t m2;
		};
		uint64_t bitsPacked;
	};

	union DspStatus
	{
		struct
		{
			unsigned c : 1;			// Carry
			unsigned o : 1;			// Overﬂow 
			unsigned z : 1;			// Arithmetic zero 
			unsigned s : 1;		// Sign
			unsigned as : 1;	// Above s32 
			unsigned tt : 1;	// Top two bits are equal 
			unsigned ok : 1;	// 1: Bit test OK, 0: Bit test not OK
			unsigned os : 1;	// Overflow (sticky)
			unsigned hwz : 1;	// Hardwired to 0? 
			unsigned acie : 1;	// Accelerator/Decoder Interrupt enable 
			unsigned unk10 : 1;
			unsigned eie : 1;		// External interrupt enable 
			unsigned ge : 1;		// Global interrupts enabler
			// Not actually status, but ALU control
			unsigned am : 1;		// Product multiply result by 2 (when AM = 0)  (0 = M2, 1 = M0)
			unsigned sxm : 1;	// Sign extension mode for loading in Middle regs (0 = clr40, 1 = set40) 
			unsigned su : 1;	// Operands are signed (1 = unsigned)
		};

		uint16_t bits;

	};

	#pragma pack (pop)

	enum class DspRegister
	{
		ar0 = 0,		// Addressing register 0 
		ar1,			// Addressing register 1 
		ar2,			// Addressing register 2 
		ar3,			// Addressing register 3 
		indexRegs,
		ix0 = indexRegs,	// Indexing register 0 
		ix1,			// Indexing register 1
		ix2,			// Indexing register 2
		ix3,			// Indexing register 3
		limitRegs,
		lm0 = limitRegs, // Limit register 0 
		lm1,			// Limit register 1
		lm2,			// Limit register 2
		lm3,			// Limit register 3
		stackRegs,
		st0 = stackRegs,	// Call stack register 
		st1,			// Data stack register 
		st2,			// Loop address stack register 
		st3,			// Loop counter register 
		ac0h,			// 40-bit Accumulator 0 (high) 
		ac1h,			// 40-bit Accumulator 1 (high) 
		bank,			// Bank register (LRS/SRS)
		sr,				// Status register 
		prodl,			// Product register (low) 
		prodm1,			// Product register (mid 1) 
		prodh,			// Product register (high) 
		prodm2,			// Product register (mid 2) 
		ax0l,			// 32-bit Accumulator 0 (low) 
		ax1l,			// 32-bit Accumulator 1 (low) 
		ax0h,			// 32-bit Accumulator 0 (high) 
		ax1h,			// 32-bit Accumulator 1 (high)
		ac0l,			// 40-bit Accumulator 0 (low) 
		ac1l,			// 40-bit Accumulator 1 (low) 
		ac0m,			// 40-bit Accumulator 0 (mid)
		ac1m,			// 40-bit Accumulator 1 (mid)
	};

	struct DspRegs
	{
		uint16_t ar[4];		// Addressing registers
		uint16_t ix[4];		// Indexing registers
		uint16_t lm[4];	// Limit registers
		std::vector<DspAddress> st[4];	// Stack registers
		DspLongAccumulator ac[2];		// 40-bit Accumulators
		DspShortAccumulator ax[2];		// 32-bit Accumulators
		DspProduct prod;		// Product register
		// https://github.com/dolphin-emu/dolphin/wiki/Zelda-Microcode#unknown-registers
		uint16_t bank;		// bank (lrs/srs)
		DspStatus sr;		// status
		DspAddress pc;		// Program counter
	};

	enum class DspHardwareRegs
	{
		CMBH = 0xFFFE,		// CPU->DSP Mailbox H 
		CMBL = 0xFFFF,		// CPU->DSP Mailbox L 
		DMBH = 0xFFFC,		// DSP->CPU Mailbox H 
		DMBL = 0xFFFD,		// DSP->CPU Mailbox L 

		DSMAH = 0xFFCE,		// Memory address H 
		DSMAL = 0xFFCF,		// Memory address L 
		DSPA = 0xFFCD,		// DSP memory address 
		DSCR = 0xFFC9,		// DMA control 
		DSBL = 0xFFCB,		// Block size 

		ACDAT2 = 0xFFD3,	// RAW accelerator data (R/W)
		ACSAH = 0xFFD4,		// Accelerator start address H 
		ACSAL = 0xFFD5,		// Accelerator start address L 
		ACEAH = 0xFFD6,		// Accelerator end address H 
		ACEAL = 0xFFD7,		// Accelerator end address L 
		ACCAH = 0xFFD8,		// Accelerator current address H  +  Acc Direction
		ACCAL = 0xFFD9,		// Accelerator current address L 
		AMDM = 0xFFEF,		// ARAM DMA Request Mask
		// From https://github.com/devkitPro/gamecube-tools/blob/master/gdopcode/disassemble.cpp
		ACFMT = 0xFFD1,			// sample format used
		ACPDS = 0xFFDA,			// predictor / scale combination
		ACYN1 = 0xFFDB,			// y[n - 1]
		ACYN2 = 0xFFDC,			// y[n - 2]
		ACDAT = 0xFFDD,		// Decoded Adpcm data (Read)  y[n]  (Read only)
		ACGAN = 0xFFDE,			// gain to be applied (PCM mode only)
		// ADPCM coef table. Coefficient selected by Adpcm Predictor
		ADPCM_A00 = 0xFFA0,		// Coef * Yn1[0]
		ADPCM_A10 = 0xFFA1,		// Coef * Yn2[0]
		ADPCM_A20 = 0xFFA2,		// Coef * Yn1[1]
		ADPCM_A30 = 0xFFA3,		// Coef * Yn2[1]
		ADPCM_A40 = 0xFFA4,		// Coef * Yn1[2]
		ADPCM_A50 = 0xFFA5,		// Coef * Yn2[2]
		ADPCM_A60 = 0xFFA6,		// Coef * Yn1[3]
		ADPCM_A70 = 0xFFA7,		// Coef * Yn2[3]
		ADPCM_A01 = 0xFFA8,		// Coef * Yn1[4]
		ADPCM_A11 = 0xFFA9,		// Coef * Yn2[4]
		ADPCM_A21 = 0xFFAA,		// Coef * Yn1[5]
		ADPCM_A31 = 0xFFAB,		// Coef * Yn2[5]
		ADPCM_A41 = 0xFFAC,		// Coef * Yn1[6]
		ADPCM_A51 = 0xFFAD,		// Coef * Yn2[6]
		ADPCM_A61 = 0xFFAE,		// Coef * Yn1[7]
		ADPCM_A71 = 0xFFAF,		// Coef * Yn2[7]
		// Unknown
		UNKNOWN_FFB0 = 0xFFB0,
		UNKNOWN_FFB1 = 0xFFB1,

		DIRQ = 0xFFFB,		// IRQ request
	};

	// Known DSP exceptions

	enum class DspException
	{
		RESET = 0,
		STOVF,			// Stack underflow/overflow
		Unknown2,
		ACR_OVF,		// Acclerator current address = Start address (RAW Read mode)
		ACW_OVF,		// Acclerator current address = End address (RAW Write mode)
		ADP_OVF,		// Acclerator current address = End address (ADPCM Decoder mode)
		Unknown6,
		INT,			// External interrupt (from CPU)
	};

	// Accelerator sample format

	enum class AccelFormat
	{
		RawByte = 0x0005,		// Seen in IROM
		RawUInt16 = 0x0006,		// 
		Pcm16 = 0x000A,			// Signed 16 bit PCM mono
		Pcm8 = 0x0019,			// Signed 8 bit PCM mono
		Adpcm = 0x0000,			// ADPCM encoded (both standard & extended)
	};

	class DspInterpreter;

	class DspCore
	{
		friend DspInterpreter;

	public:
		std::list<DspAddress> breakpoints;		// IMEM breakpoints
		SpinLock breakPointsSpinLock;
		DspAddress oneShotBreakpoint = 0xffff;

		std::map<DspAddress, std::string> canaries;		// When the PC is equal to the canary address, a debug message is displayed
		SpinLock canariesSpinLock;

		const uint32_t GekkoTicksPerDspInstruction = 5;		// How many Gekko ticks should pass so that we can execute one DSP instruction
		const uint32_t GekkoTicksPerDspSegment = 100;		// How many Gekko ticks should pass so that we can execute one DSP segment (in case of Jitc)

		uint64_t savedGekkoTicks = 0;

		Thread* dspThread = nullptr;
		static void DspThreadProc(void* Parameter);

		DspInterpreter* interp;

		volatile uint16_t DspToCpuMailbox[2];		// DMBH, DMBL
		SpinLock DspToCpuLock[2];

		volatile uint16_t CpuToDspMailbox[2];		// CMBH, CMBL
		SpinLock CpuToDspLock[2];

		bool haltOnUnmappedMemAccess = false;

		struct
		{
			union
			{
				struct
				{
					uint16_t	l;
					uint16_t	h;
				};
				uint32_t	bits;
			} mmemAddr;
			DspAddress  dspAddr;
			uint16_t	blockSize;
			union
			{
				struct
				{
					unsigned Dsp2Mmem : 1;		// 0: MMEM -> DSP, 1: DSP -> MMEM
					unsigned Imem : 1;			// 0: DMEM, 1: IMEM
				};
				uint16_t	bits;
			} control;
		} DmaRegs;

		struct
		{
			uint16_t Fmt;					// Sample format
			uint16_t AdpcmCoef[16];			
			uint16_t AdpcmPds;				// predictor / scale combination
			uint16_t AdpcmYn1;				// y[n - 1]
			uint16_t AdpcmYn2;				// y[n - 2]
			uint16_t AdpcmGan;				// gain to be applied
			union
			{
				struct
				{
					uint16_t l;
					uint16_t h;
				};
				uint32_t addr;
			} StartAddress;
			union
			{
				struct
				{
					uint16_t l;
					uint16_t h;
				};
				uint32_t addr;
			} EndAddress;
			union
			{
				struct
				{
					uint16_t l;
					uint16_t h;
				};
				uint32_t addr;
			} CurrAddress;
			
			bool pendingOverflow;
			DspException overflowVector;
		} Accel;

		void ResetIfx();
		void DoDma();
		uint16_t AccelReadData(bool raw);
		uint16_t AccelFetch();
		void AccelWriteData(uint16_t data);
		void ResetAccel();
		uint16_t DecodeAdpcm(uint16_t nibble);

		bool pendingInterrupt = false;
		int pendingInterruptDelay = 2;
		bool pendingSoftReset = false;

		// Logging control
		bool logMailbox = false;
		bool logInsaneMailbox = false;
		bool logDspControlBits = false;
		bool logDspInterrupts = false;
		bool logNonconditionalCallJmp = false;
		bool logDspDma = false;
		bool logAccel = false;
		bool logAdpcm = false;
		bool dumpUcode = false;

	public:

		static const size_t MaxInstructionSizeInBytes = 4;		// max instruction size

		static const size_t IRAM_SIZE = (8 * 1024);
		static const size_t IROM_SIZE = (8 * 1024);
		static const size_t DRAM_SIZE = (8 * 1024);
		static const size_t DROM_SIZE = (4 * 1024);

		static const size_t IROM_START_ADDRESS = 0x8000;
		static const size_t DROM_START_ADDRESS = 0x1000;
		static const size_t IFX_START_ADDRESS = 0xFF00;		// Internal dsp "hardware"

		DspRegs regs;

		uint8_t iram[IRAM_SIZE] = { 0 };
		uint8_t irom[IROM_SIZE] = { 0 };
		uint8_t dram[DRAM_SIZE] = { 0 };
		uint8_t drom[DROM_SIZE] = { 0 };

		DspCore(HWConfig* config);
		~DspCore();

		void Exception(DspException id);
		void ReturnFromException();
		void SoftReset();
		void HardReset();

		void Run();
		bool IsRunning() { return dspThread->IsRunning(); }
		void Suspend();

		void Update();

		// Debug methods

		void AddBreakpoint(DspAddress imemAddress);
		void RemoveBreakpoint(DspAddress imemAddress);
		void ListBreakpoints();
		void ClearBreakpoints();
		bool TestBreakpoint(DspAddress imemAddress);
		void ToggleBreakpoint(DspAddress imemAddress);
		void AddOneShotBreakpoint(DspAddress imemAddress);
		void AddCanary(DspAddress imemAddress, std::string text);
		void ListCanaries();
		void ClearCanaries();
		bool TestCanary(DspAddress imemAddress);
		void Step();
		void DumpRegs(DspRegs *prevState);
		void DumpIfx();

		// Register access

		void MoveToReg(int reg, uint16_t val);
		uint16_t MoveFromReg(int reg);

		// Memory engine

		uint8_t* TranslateIMem(DspAddress addr);
		uint8_t* TranslateDMem(DspAddress addr);
		uint16_t ReadIMem(DspAddress addr);
		uint16_t ReadDMem(DspAddress addr);
		void WriteDMem(DspAddress addr, uint16_t value);

		// Flipper interface

		void DSPSetResetBit(bool val);
		bool DSPGetResetBit();
		void DSPSetIntBit(bool val);
		bool DSPGetIntBit();
		void DSPSetHaltBit(bool val);
		bool DSPGetHaltBit();

		// CPU->DSP Mailbox
		void CpuToDspWriteHi(uint16_t value);
		void CpuToDspWriteLo(uint16_t value);
		uint16_t CpuToDspReadHi(bool ReadByDsp);
		uint16_t CpuToDspReadLo(bool ReadByDsp);
		// DSP->CPU Mailbox
		void DspToCpuWriteHi(uint16_t value);
		void DspToCpuWriteLo(uint16_t value);
		uint16_t DspToCpuReadHi(bool ReadByDsp);
		uint16_t DspToCpuReadLo(bool ReadByDsp);

		// Multiplier and ALU
		
		static int64_t SignExtend40(int64_t);
		static int64_t SignExtend16(int16_t);

		static void PackProd(DspProduct& prod);
		static void UnpackProd(DspProduct& prod);
		static DspProduct Muls(int16_t a, int16_t b, bool scale);
		static DspProduct Mulu(uint16_t a, uint16_t b, bool scale);
		static DspProduct Mulus(uint16_t a, int16_t b, bool scale);

		void ArAdvance(int r, int16_t step);

		static void InitSubsystem();
		static void ShutdownSubsystem();
	};

	#pragma warning (pop)		// warning C4201: nonstandard extension used: nameless struct/union

}
