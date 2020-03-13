// DSP analyzer

#include "pch.h"

namespace DSP
{
	void Analyzer::ResetInfo(AnalyzeInfo& info)
	{
		info.sizeInBytes = 0;

		info.instr = DspInstruction::Unknown;
		info.instrEx = DspInstructionEx::Unknown;

		info.extendedOpcodePresent = false;
		info.numParameters = 0;
		info.numParametersEx = 0;

		info.flowControl = false;
		info.logic = false;
		info.madd = false;
	}

	bool Analyzer::Group0_Logic(uint8_t* instrPtr, size_t instrMaxSize, AnalyzeInfo& info, DspInstruction instr, bool logic)
	{
		if ((info.instrBits & 0xf) == 0)
		{
			if (instrMaxSize < sizeof(uint16_t))
				return false;

			int dd = ((info.instrBits & 0b100000000) != 0) ? 1 : 0;

			info.instr = instr;
			info.logic = logic;

			if (!AddParam(info, (DspParameter)((int)DspParameter::ac0m + dd), dd))
				return false;

			uint16_t imm = MEMSwapHalf(*(uint16_t*)instrPtr);
			if (!AddBytes(instrPtr, sizeof(uint16_t), info))
				return false;
			if (!AddImmOperand(info, DspParameter::UnsignedShort, imm))
				return false;
		}

		return true;
	}

	bool Analyzer::Group0(uint8_t* instrPtr, size_t instrMaxSize, AnalyzeInfo& info)
	{
		info.instr = DspInstruction::Unknown;

		if ((info.instrBits & 0b0000111100000000) == 0)
		{
			//NOP * 	0000 0000 [000]0 0000 
			//DAR * 	0000 0000 [000]0 01dd 
			//IAR * 	0000 0000 [000]0 10dd 
			//ADDARN * 	0000 0000 [000]1 ssdd

			//HALT * 	0000 0000 [001]0 0001 
			//LOOP * 	0000 0000 [010]r rrrr 
			//BLOOP * 	0000 0000 [011]r rrrr aaaa aaaa aaaa aaaa
			//LRI * 	0000 0000 [100]r rrrr iiii iiii iiii iiii 
			//LR * 		0000 0000 [110]r rrrr mmmm mmmm mmmm mmmm 
			//SR * 		0000 0000 [111]r rrrr mmmm mmmm mmmm mmmm 

			switch ((info.instrBits >> 5) & 7)
			{
				case 0b000:
					if (info.instrBits & 0b10000)
					{
						// ADDARN
						int dd = info.instrBits & 3;
						int ss = (info.instrBits >> 2) & 3;

						info.instr = DspInstruction::ADDARN;
						if (!AddParam(info, (DspParameter)((int)DspParameter::ar0 + dd), dd))
							return false;
						if (!AddParam(info, (DspParameter)((int)DspParameter::ix0 + ss), ss))
							return false;
					}
					else
					{
						switch ((info.instrBits >> 2) & 3)
						{
							case 0b00:		// NOP
								if ((info.instrBits & 3) == 0)
								{
									info.instr = DspInstruction::NOP;
								}
								break;
							case 0b01:		// DAR
							{
								int dd = info.instrBits & 3;

								info.instr = DspInstruction::DAR;
								if (!AddParam(info, (DspParameter)((int)DspParameter::ar0 + dd), dd))
									return false;
								break;
							}
							case 0b10:		// IAR
							{
								int dd = info.instrBits & 3;

								info.instr = DspInstruction::IAR;
								if (!AddParam(info, (DspParameter)((int)DspParameter::ar0 + dd), dd))
									return false;
								break;
							}

							default:
								break;
						}
					}
					break;
				case 0b001:		// HALT
					if ((info.instrBits & 0b11111) == 0b00001)
					{
						info.instr = DspInstruction::HALT;
						info.flowControl = true;
					}
					break;
				case 0b010:		// LOOP
				{
					uint16_t r = info.instrBits & 0b11111;
					info.instr = DspInstruction::LOOP;
					info.flowControl = true;
					if (!AddParam(info, (DspParameter)r, r))
						return false;
					break;
				}
				case 0b011:		// BLOOP
				{
					if (instrMaxSize < sizeof(uint16_t))
						return false;

					uint16_t r = info.instrBits & 0b11111;
					info.instr = DspInstruction::BLOOP;
					info.flowControl = true;
					if (!AddParam(info, (DspParameter)r, r))
						return false;

					uint16_t addr = MEMSwapHalf (*(uint16_t*)instrPtr);
					if (!AddBytes(instrPtr, sizeof(uint16_t), info))
						return false;
					if (!AddImmOperand(info, DspParameter::Address, (DspAddress)addr))
						return false;

					break;
				}
				case 0b100:		// LRI
				{
					if (instrMaxSize < sizeof(uint16_t))
						return false;

					uint16_t r = info.instrBits & 0b11111;
					info.instr = DspInstruction::LRI;
					if (!AddParam(info, (DspParameter)r, r))
						return false;

					uint16_t imm = MEMSwapHalf(*(uint16_t*)instrPtr);
					if (!AddBytes(instrPtr, sizeof(uint16_t), info))
						return false;
					if (!AddImmOperand(info, DspParameter::UnsignedShort, imm))
						return false;

					break;
				}
				case 0b110:		// LR
				{
					if (instrMaxSize < sizeof(uint16_t))
						return false;

					uint16_t r = info.instrBits & 0b11111;
					info.instr = DspInstruction::LR;
					if (!AddParam(info, (DspParameter)r, r))
						return false;

					uint16_t addr = MEMSwapHalf(*(uint16_t*)instrPtr);
					if (!AddBytes(instrPtr, sizeof(uint16_t), info))
						return false;
					if (!AddImmOperand(info, DspParameter::Address, (DspAddress)addr))
						return false;

					break;
				}
				case 0b111:		// SR
				{
					if (instrMaxSize < sizeof(uint16_t))
						return false;

					uint16_t addr = MEMSwapHalf(*(uint16_t*)instrPtr);
					if (!AddBytes(instrPtr, sizeof(uint16_t), info))
						return false;
					if (!AddImmOperand(info, DspParameter::Address, (DspAddress)addr))
						return false;

					uint16_t r = info.instrBits & 0b11111;
					info.instr = DspInstruction::SR;
					if (!AddParam(info, (DspParameter)r, r))
						return false;

					break;
				}

				default:
					break;
			}

			return true;
		}

		else if ((info.instrBits & 0b0000111000000000) == 0b0000001000000000)
		{
			//IF cc * 	0000 00[1]0 [0111] cccc 
			//JMP cc * 	0000 00[1]0 [1001] cccc aaaa aaaa aaaa aaaa
			//CALL cc * 0000 00[1]0 [1011] cccc aaaa aaaa aaaa aaaa
			//RET cc * 	0000 00[1]0 [1101] cccc 
			//ADDI * 	0000 00[1]d [0000] 0000 iiii iiii iiii iiii 
			//XORI * 	0000 00[1]d [0010] 0000 iiii iiii iiii iiii 
			//ANDI * 	0000 00[1]d [0100] 0000 iiii iiii iiii iiii 
			//ORI * 	0000 00[1]d [0110] 0000 iiii iiii iiii iiii 
			//CMPI * 	0000 00[1]d [1000] 0000 iiii iiii iiii iiii 
			//ANDCF * 	0000 00[1]d [1010] 0000 iiii iiii iiii iiii 
			//ANDF * 	0000 00[1]d [1100] 0000 iiii iiii iiii iiii 
			//ILRR * 	0000 00[1]d [0001] 00ss
			//ILRRD * 	0000 00[1]d [0001] 01ss
			//ILRRI * 	0000 00[1]d [0001] 10ss
			//ILRRN * 	0000 00[1]d [0001] 11ss

			switch ((info.instrBits >> 4) & 0xf)
			{
				case 0b0111:		// IF cc
				{
					info.instr = DspInstruction::IFcc;
					info.cc = (ConditionCode)(info.instrBits & 0xf);
					info.flowControl = true;
					break;
				}
				case 0b1001:		// JMP cc
				{
					if (instrMaxSize < sizeof(uint16_t))
						return false;

					info.instr = DspInstruction::Jcc;
					info.cc = (ConditionCode)(info.instrBits & 0xf);
					info.flowControl = true;

					uint16_t addr = MEMSwapHalf(*(uint16_t*)instrPtr);
					if (!AddBytes(instrPtr, sizeof(uint16_t), info))
						return false;
					if (!AddImmOperand(info, DspParameter::Address, (DspAddress)addr))
						return false;

					break;
				}
				case 0b1011:		// CALL cc
				{
					if (instrMaxSize < sizeof(uint16_t))
						return false;

					info.instr = DspInstruction::CALLcc;
					info.cc = (ConditionCode)(info.instrBits & 0xf);
					info.flowControl = true;

					uint16_t addr = MEMSwapHalf(*(uint16_t*)instrPtr);
					if (!AddBytes(instrPtr, sizeof(uint16_t), info))
						return false;
					if (!AddImmOperand(info, DspParameter::Address, (DspAddress)addr))
						return false;

					break;
				}
				case 0b1101:		// RET cc
				{
					info.instr = DspInstruction::RETcc;
					info.cc = (ConditionCode)(info.instrBits & 0xf);
					info.flowControl = true;
					break;
				}
				case 0b0000:		// ADDI
					return Group0_Logic(instrPtr, instrMaxSize, info, DspInstruction::ADDI, true);
					break;
				case 0b0010:		// XORI
					return Group0_Logic(instrPtr, instrMaxSize, info, DspInstruction::XORI, true);
					break;
				case 0b0100:		// ANDI
					return Group0_Logic(instrPtr, instrMaxSize, info, DspInstruction::ANDI, true);
					break;
				case 0b0110:		// ORI
					return Group0_Logic(instrPtr, instrMaxSize, info, DspInstruction::ORI, true);
					break;
				case 0b1000:		// CMPI
					return Group0_Logic(instrPtr, instrMaxSize, info, DspInstruction::CMPI, false);
					break;
				case 0b1010:		// ANDCF
					return Group0_Logic(instrPtr, instrMaxSize, info, DspInstruction::ANDCF, true);
					break;
				case 0b1100:		// ANDF
					return Group0_Logic(instrPtr, instrMaxSize, info, DspInstruction::ANDF, true);
					break;
				case 0b0001:		// ILRR's
				{
					switch ((info.instrBits >> 2) & 3)
					{
						case 0:
							info.instr = DspInstruction::ILRR;
							break;
						case 1:
							info.instr = DspInstruction::ILRRD;
							break;
						case 2:
							info.instr = DspInstruction::ILRRI;
							break;
						case 3:
							info.instr = DspInstruction::ILRRN;
							break;
					}

					int dd = ((info.instrBits & 0b100000000) != 0) ? 1 : 0;
					int ss = info.instrBits & 3;

					if (!AddParam(info, (DspParameter)((int)DspParameter::ac0m + dd), dd))
						return false;
					if (!AddParam(info, (DspParameter)((int)DspParameter::Indexed_ar0 + ss), ss))
						return false;

					break;
				}
			}

			return true;
		}

		else if (info.instrBits & 0b0000100000000000)
		{
			//LRIS * 	0000 1ddd iiii iiii 

			info.instr = DspInstruction::LRIS;

			int dd = (info.instrBits >> 8) & 7;
			int8_t ii = info.instrBits & 0xff;
			if (!AddParam(info, (DspParameter)(0x18 + dd), dd))
				return false;
			if (!AddImmOperand(info, DspParameter::SignedByte, ii))
				return false;

			return true;
		}

		else if (info.instrBits & 0b0000010000000000)
		{
			//ADDIS *	0000 01[0]d iiii iiii
			//CMPIS *	0000 01[1]d iiii iiii

			if (info.instrBits & 0b1000000000)
			{
				info.instr = DspInstruction::CMPIS;
			}
			else
			{
				info.instr = DspInstruction::ADDIS;
			}

			int dd = (info.instrBits >> 8) & 1;
			int8_t ii = info.instrBits & 0xff;
			if (!AddParam(info, (DspParameter)((int)DspParameter::ac0m + dd), dd))
				return false;
			if (!AddImmOperand(info, DspParameter::SignedByte, ii))
				return false;

			return true;
		}

		return true;
	}

	bool Analyzer::Group1(uint8_t* instrPtr, size_t instrMaxSize, AnalyzeInfo& info)
	{
		//LOOPI *	0001 [00]00 iiii iiii
		//BLOOPI	0001 [00]01 iiii iiii aaaa aaaa aaaa aaaa 
		//SBSET *	0001 [00]10 0000 0iii 
		//SBCLR *	0001 [00]11 0000 0iii 

		//LSL 		0001 [01]0r 00ii iiii
		//LSR 		0001 [01]0r 01ii iiii
		//ASL 		0001 [01]0r 10ii iiii
		//ASR 		0001 [01]0r 11ii iiii
		//SI * 		0001 [01]10 mmmm mmmm iiii iiii iiii iiii
		//CALLR *	0001 [01]11 rrr1 1111 
		//JMPR * 	0001 [01]11 rrr0 1111 

		//LRR * 	0001 [10]00 0ssd dddd 
		//LRRD * 	0001 [10]00 1ssd dddd 
		//LRRI * 	0001 [10]01 0ssd dddd 
		//LRRN * 	0001 [10]01 1ssd dddd 
		//SRR * 	0001 [10]10 0ssd dddd 
		//SRRD * 	0001 [10]10 1ssd dddd 
		//SRRI * 	0001 [10]11 0ssd dddd 
		//SRRN * 	0001 [10]11 1ssd dddd 

		//MRR * 	0001 [11]dd ddds ssss 

		switch ((info.instrBits >> 10) & 3)
		{
			case 0:
				switch ((info.instrBits >> 8) & 3)
				{
					case 0:		// LOOPI
					{
						info.instr = DspInstruction::LOOPI;
						info.flowControl = true;

						if (!AddImmOperand(info, DspParameter::Byte, (uint8_t)(info.instrBits & 0xff)))
							return false;

						break;
					}
					case 1:		// BLOOPI
					{
						if (instrMaxSize < sizeof(uint16_t))
							return false;

						info.instr = DspInstruction::BLOOPI;
						info.flowControl = true;

						uint16_t addr = MEMSwapHalf(*(uint16_t*)instrPtr);
						if (!AddBytes(instrPtr, sizeof(uint16_t), info))
							return false;
						if (!AddImmOperand(info, DspParameter::Byte, (uint8_t)(info.instrBits & 0xff)))
							return false;
						if (!AddImmOperand(info, DspParameter::Address2, (DspAddress)addr))
							return false;
						break;
					}
					case 2:		// SBSET
						if ((info.instrBits & 0b11111000) == 0)
						{
							info.instr = DspInstruction::SBSET;

							int ii = 6 + (info.instrBits & 7);
							if (!AddImmOperand(info, DspParameter::Byte, (uint8_t)ii))
								return false;
						}
						break;
					case 3:		// SBCLR
						if ((info.instrBits & 0b11111000) == 0)
						{
							info.instr = DspInstruction::SBCLR;

							int ii = 6 + (info.instrBits & 7);
							if (!AddImmOperand(info, DspParameter::Byte, (uint8_t)ii))
								return false;
						}
						break;
				}
				break;
			case 1:
				if ((info.instrBits & 0b1000000000) != 0)
				{
					if ((info.instrBits & 0b100000000) != 0)
					{
						if ((info.instrBits & 0xf) == 0xf)
						{
							// CALLR, JMPR
							if ((info.instrBits & 0b10000) != 0)
							{
								info.instr = DspInstruction::CALLR;
							}
							else
							{
								info.instr = DspInstruction::JMPR;
							}
							info.flowControl = true;

							int rr = (info.instrBits >> 5) & 7;
							if (!AddParam(info, (DspParameter)rr, rr))
								return false;
						}
					}
					else
					{
						// SI
						if (instrMaxSize < sizeof(uint16_t))
							return false;

						info.instr = DspInstruction::SI;

						DspAddress mm = (DspAddress)(uint16_t)(int16_t)(int8_t)(uint8_t)(info.instrBits & 0xff);
						uint16_t imm = MEMSwapHalf(*(uint16_t*)instrPtr);
						if (!AddBytes(instrPtr, sizeof(uint16_t), info))
							return false;
						if (!AddImmOperand(info, DspParameter::Address, mm))
							return false;
						if (!AddImmOperand(info, DspParameter::UnsignedShort2, imm))
							return false;
					}
				}
				else
				{
					// LSL, LSR, ASL, ASR

					switch ((info.instrBits >> 6) & 3)
					{
						case 0:
							info.instr = DspInstruction::LSL;
							break;
						case 1:
							info.instr = DspInstruction::LSR;
							break;
						case 2:
							info.instr = DspInstruction::ASL;
							break;
						case 3:
							info.instr = DspInstruction::ASR;
							break;
					}
					info.logic = true;

					uint16_t rr = ((info.instrBits & 0b100000000) != 0) ? 1 : 0;
					uint8_t ii = info.instrBits & 0x3f;

					if (!AddParam(info, (DspParameter)((int)DspParameter::ac0 + rr), rr))
						return false;
					if (!AddImmOperand(info, DspParameter::Byte, ii))
						return false;
				}
				break;
			case 2:			// LRR / SRR
			{
				bool load = false;

				switch ((info.instrBits >> 7) & 7)
				{
					case 0b000:
						info.instr = DspInstruction::LRR;
						load = true;
						break;
					case 0b001:
						info.instr = DspInstruction::LRRD;
						load = true;
						break;
					case 0b010:
						info.instr = DspInstruction::LRRI;
						load = true;
						break;
					case 0b011:
						info.instr = DspInstruction::LRRN;
						load = true;
						break;
					case 0b100:
						info.instr = DspInstruction::SRR;
						load = false;
						break;
					case 0b101:
						info.instr = DspInstruction::SRRD;
						load = false;
						break;
					case 0b110:
						info.instr = DspInstruction::SRRI;
						load = false;
						break;
					case 0b111:
						info.instr = DspInstruction::SRRN;
						load = false;
						break;
				}

				if (load)
				{
					int dd = (info.instrBits & 0x1f);
					int ss = (info.instrBits >> 5) & 3;

					if (!AddParam(info, (DspParameter)dd, dd))
						return false;
					if (!AddParam(info, (DspParameter)((int)DspParameter::Indexed_regs + ss), ss))
						return false;
				}
				else
				{
					int ss = (info.instrBits & 0x1f);
					int dd = (info.instrBits >> 5) & 3;

					if (!AddParam(info, (DspParameter)((int)DspParameter::Indexed_regs + dd), dd))
						return false;
					if (!AddParam(info, (DspParameter)ss, ss))
						return false;
				}
				break;
			}
			case 3:		// MRR
			{
				int dd = (info.instrBits >> 5) & 0x1f;
				int ss = (info.instrBits & 0x1f);

				info.instr = DspInstruction::MRR;

				if (!AddParam(info, (DspParameter)dd, dd))
					return false;
				if (!AddParam(info, (DspParameter)ss, ss))
					return false;
				break;
			}
		}

		return true;
	}

	bool Analyzer::Group2(AnalyzeInfo& info)
	{
		//LRS * 	0010 0ddd mmmm mmmm 
		//SRS * 	0010 1sss mmmm mmmm 

		if ((info.instrBits & 0b100000000000) != 0)
		{
			info.instr = DspInstruction::SRS;
			int ss = (info.instrBits >> 8) & 7;
			DspAddress addr = (DspAddress)(uint16_t)(int16_t)(int8_t)(uint8_t)(info.instrBits & 0xff);

			if (!AddImmOperand(info, DspParameter::Address, addr))
				return false;
			if (!AddParam(info, (DspParameter)(0x18 + ss), ss))
				return false;
		}
		else
		{
			info.instr = DspInstruction::LRS;
			int dd = (info.instrBits >> 8) & 7;
			DspAddress addr = (DspAddress)(uint16_t)(int16_t)(int8_t)(uint8_t)(info.instrBits & 0xff);

			if (!AddParam(info, (DspParameter)(0x18 + dd), dd))
				return false;
			if (!AddImmOperand(info, DspParameter::Address, addr))
				return false;
		}

		return true;
	}

	bool Analyzer::Group3(AnalyzeInfo& info)
	{
		return true;
	}

	bool Analyzer::Group4(AnalyzeInfo& info)
	{
		return true;
	}

	bool Analyzer::Group5(AnalyzeInfo& info)
	{
		return true;
	}

	bool Analyzer::Group6(AnalyzeInfo& info)
	{
		return true;
	}

	bool Analyzer::Group7(AnalyzeInfo& info)
	{
		return true;
	}

	bool Analyzer::Group8(AnalyzeInfo& info)
	{
		//NX * 			1000 r000 xxxx xxxx			(possibly mov r, r)
		//CLR * 		1000 r001 xxxx xxxx 
		//CMP * 		1000 0010 xxxx xxxx 
		//UNUSED*		1000 0011 xxxx xxxx 
		//CLRP * 		1000 0100 xxxx xxxx 
		//TSTAXH 		1000 011r xxxx xxxx 
		//M2/M0 		1000 101x xxxx xxxx 
		//CLR15/SET15	1000 110x xxxx xxxx 
		//CLR40/SET40	1000 111x xxxx xxxx 

		switch ((info.instrBits >> 8) & 0xf)
		{
			case 0b0000:		// NX
			case 0b1000:
				info.instr = DspInstruction::NX;
				break;

			case 0b0001:		// CLR acR
			case 0b1001:
			{
				int r = (info.instrBits >> 11) & 1;
				info.instr = DspInstruction::CLR;
				if (!AddParam(info, (DspParameter)((int)DspParameter::ac0 + r), r))
					return false;
				break;
			}

			case 0b0010:		// CMP
				info.instr = DspInstruction::CMP;
				break;

			case 0b0100:		// CLRP
				info.instr = DspInstruction::CLRP;
				break;

			case 0b0110:		// TSTAXH axR.h 
			case 0b0111:
			{
				int r = (info.instrBits >> 8) & 1;
				info.instr = DspInstruction::TSTAXH;
				if (!AddParam(info, r ? DspParameter::ax1h : DspParameter::ax0h, r))
					return false;
				break;
			}

			case 0b1010:		// M2
				info.instr = DspInstruction::M2;
				break;
			case 0b1011:		// M0
				info.instr = DspInstruction::M0;
				break;

			case 0b1100:		// CLR15
				info.instr = DspInstruction::CLR15;
				break;
			case 0b1101:		// SET15
				info.instr = DspInstruction::SET15;
				break;

			case 0b1110:		// CLR40
				info.instr = DspInstruction::CLR40;
				break;
			case 0b1111:		// SET40
				info.instr = DspInstruction::SET40;
				break;
		}

		return true;
	}

	bool Analyzer::Group9(AnalyzeInfo& info)
	{
		return true;
	}

	bool Analyzer::GroupAB(AnalyzeInfo& info)
	{
		return true;
	}

	bool Analyzer::GroupCD(AnalyzeInfo& info)
	{
		return true;
	}

	bool Analyzer::GroupE(AnalyzeInfo& info)
	{
		return true;
	}

	bool Analyzer::GroupF(AnalyzeInfo& info)
	{
		return true;
	}

	// DSP instructions are in a hybrid format: some instructions occupy a full 16-bit word, and some can be packed as two 8-bit instructions per word.
	// Extended opcodes represents lower-part of instruction pair.

	bool Analyzer::GroupPacked(AnalyzeInfo& info)
	{
		//?? * 		xxxx xxxx 0000 00rr		// NOP2
		//DR * 		xxxx xxxx 0000 01rr		// DR $arR 
		//IR * 		xxxx xxxx 0000 10rr		// IR $arR 
		//NR * 		xxxx xxxx 0000 11rr		// NR $arR, ixR
		//MV * 		xxxx xxxx 0001 ddss 	// MV $(0x18+D), $(0x1c+S) 
		//S * 		xxxx xxxx 001s s0dd		// S @$rD, $(0x1c+s)  
		//SN * 		xxxx xxxx 001s s1dd		// SN @$rD, $(0x1c+s)
		//L * 		xxxx xxxx 01dd d0ss 	// L $(0x18+D), @$rS 
		//LN * 		xxxx xxxx 01dd d1ss 	// LN $(0x18+D), @$rS 

		//LS * 		xxxx xxxx 10dd 000s 	// LS $(0x18+D), $acS.m 
		//SL * 		xxxx xxxx 10dd 001s		// SL $acS.m, $(0x18+D)  
		//LSN * 	xxxx xxxx 10dd 010s		// LSN $(0x18+D), $acS.m 
		//SLN * 	xxxx xxxx 10dd 011s		// SLN $acS.m, $(0x18+D)
		//LSM * 	xxxx xxxx 10dd 100s		// LSM $(0x18+D), $acS.m 
		//SLM * 	xxxx xxxx 10dd 101s		// SLM $acS.m, $(0x18+D)
		//LSNM * 	xxxx xxxx 10dd 110s		// LSNM $(0x18+D), $acS.m 
		//SLNM * 	xxxx xxxx 10dd 111s		// SLNM $acS.m, $(0x18+D)

		//LD 		xxxx xxxx 11dr 00ss		// LD $ax0.d, $ax1.r, @$arS 
		//LDN 		xxxx xxxx 11dr 01ss		// LDN $ax0.d, $ax1.r, @$arS
		//LDM 		xxxx xxxx 11dr 10ss		// LDM $ax0.d, $ax1.r, @$arS
		//LDNM 		xxxx xxxx 11dr 11ss		// LDNM $ax0.d, $ax1.r, @$arS

		//LDAX 		xxxx xxxx 11sr 0011		// LDAX $axR, @$arS
		//LDAXN 	xxxx xxxx 11sr 0111		// LDAXN $axR, @$arS
		//LDAXM 	xxxx xxxx 11sr 1011		// LDAXM $axR, @$arS
		//LDAXNM 	xxxx xxxx 11sr 1111		// LDAXNM $axR, @$arS

		info.extendedOpcodePresent = true;

		info.instrExBits = info.instrBits & 0xff;

		switch (info.instrExBits >> 6)
		{
			case 0:
			{
				if ((info.instrExBits & 0b100000) != 0)		// S, SN
				{
					int ss = (info.instrExBits >> 3) & 3;
					int dd = info.instrExBits & 3;

					if ((info.instrExBits & 0b100) != 0)
					{
						info.instrEx = DspInstructionEx::SN;
					}
					else
					{
						info.instrEx = DspInstructionEx::S;
					}

					if (!AddParamEx(info, (DspParameter)((int)DspParameter::Indexed_regs + dd), dd))
						return false;
					if (!AddParamEx(info, (DspParameter)(0x1c + ss), ss))
						return false;
				}
				else if ((info.instrExBits & 0b10000) != 0)		// MV
				{
					int dd = (info.instrExBits >> 2) & 3;
					int ss = info.instrExBits & 3;

					info.instrEx = DspInstructionEx::MV;

					if (!AddParamEx(info, (DspParameter)(0x18 + dd), dd))
						return false;
					if (!AddParamEx(info, (DspParameter)(0x1c + ss), ss))
						return false;
				}
				else	// NOP2, DR, IR, NR
				{
					int rr = info.instrExBits & 3;

					switch ((info.instrExBits >> 2) & 3)
					{
						case 0:
							info.instrEx = DspInstructionEx::NOP2;
							break;
						case 1:
							info.instrEx = DspInstructionEx::DR;
							if (!AddParamEx(info, (DspParameter)((int)DspParameter::ar0 + rr), rr))
								return false;
							break;
						case 2:
							info.instrEx = DspInstructionEx::IR;
							if (!AddParamEx(info, (DspParameter)((int)DspParameter::ar0 + rr), rr))
								return false;
							break;
						case 3:
							info.instrEx = DspInstructionEx::NR;
							if (!AddParamEx(info, (DspParameter)((int)DspParameter::ar0 + rr), rr))
								return false;
							if (!AddParamEx(info, (DspParameter)((int)DspParameter::ix0 + rr), rr))
								return false;
							break;
					}
				}

				break;
			}
			case 1:		// L, LN
			{
				int dd = (info.instrExBits >> 3) & 7;
				int ss = info.instrExBits & 3;

				if (info.instrExBits & 0b100)
				{
					info.instrEx = DspInstructionEx::LN;
				}
				else
				{
					info.instrEx = DspInstructionEx::L;
				}

				if (!AddParamEx(info, (DspParameter)(0x18 + dd), dd))
					return false;
				if (!AddParamEx(info, (DspParameter)((int)DspParameter::Indexed_regs + ss), ss))
					return false;

				break;
			}
			case 2:		// LSx, SLx
			{
				int dd = (info.instrExBits >> 4) & 3;
				int ss = info.instrExBits & 1;
				bool load = false;

				switch ((info.instrExBits >> 1) & 7)
				{
					case 0b000:
						info.instrEx = DspInstructionEx::LS;
						load = true;
						break;
					case 0b001:
						info.instrEx = DspInstructionEx::SL;
						load = false;
						break;
					case 0b010:
						info.instrEx = DspInstructionEx::LSN;
						load = true;
						break;
					case 0b011:
						info.instrEx = DspInstructionEx::SLN;
						load = false;
						break;
					case 0b100:
						info.instrEx = DspInstructionEx::LSM;
						load = true;
						break;
					case 0b101:
						info.instrEx = DspInstructionEx::SLM;
						load = false;
						break;
					case 0b110:
						info.instrEx = DspInstructionEx::LSNM;
						load = true;
						break;
					case 0b111:
						info.instrEx = DspInstructionEx::SLNM;
						load = false;
						break;
				}

				if (load)
				{
					if (!AddParamEx(info, (DspParameter)(0x18 + dd), dd))
						return false;
					if (!AddParamEx(info, (DspParameter)((int)DspParameter::ac0m + ss), ss))
						return false;
				}
				else
				{
					if (!AddParamEx(info, (DspParameter)((int)DspParameter::ac0m + ss), ss))
						return false;
					if (!AddParamEx(info, (DspParameter)(0x18 + dd), dd))
						return false;
				}

				break;
			}
			case 3:		// LDx
			{
				if ((info.instrExBits & 3) == 0b11)
				{
					int ss = (info.instrExBits >> 5) & 1;
					int rr = (info.instrExBits >> 4) & 1;

					switch ((info.instrExBits >> 2) & 3)
					{
						case 0:
							info.instrEx = DspInstructionEx::LDAX;
							break;
						case 1:
							info.instrEx = DspInstructionEx::LDAXN;
							break;
						case 2:
							info.instrEx = DspInstructionEx::LDAXM;
							break;
						case 3:
							info.instrEx = DspInstructionEx::LDAXNM;
							break;
					}

					if (!AddParamEx(info, (DspParameter)((int)DspParameter::ax0 + rr), rr))
						return false;
					if (!AddParamEx(info, (DspParameter)((int)DspParameter::Indexed_ar0 + ss), ss))
						return false;
				}
				else
				{
					int dd = (info.instrExBits >> 5) & 1;
					int rr = (info.instrExBits >> 4) & 1;
					int ss = info.instrExBits & 3;

					switch ((info.instrExBits >> 2) & 3)
					{
						case 0:
							info.instrEx = DspInstructionEx::LD;
							break;
						case 1:
							info.instrEx = DspInstructionEx::LDN;
							break;
						case 2:
							info.instrEx = DspInstructionEx::LDM;
							break;
						case 3:
							info.instrEx = DspInstructionEx::LDNM;
							break;
					}

					if (!AddParamEx(info, (DspParameter)((int)DspParameter::ax0l + dd), dd))
						return false;
					if (!AddParamEx(info, (DspParameter)((int)DspParameter::ax1l + rr), rr))
						return false;
					if (!AddParamEx(info, (DspParameter)((int)DspParameter::Indexed_ar0 + ss), ss))
						return false;
				}
				break;
			}
		}

		return true;
	}

	bool Analyzer::AddParam(AnalyzeInfo& info, DspParameter param, uint16_t paramBits)
	{
		if (info.numParameters >= _countof(info.params))
			return false;

		info.params[info.numParameters] = param;
		info.paramBits[info.numParameters] = paramBits;

		info.numParameters++;

		return true;
	}

	bool Analyzer::AddParamEx(AnalyzeInfo& info, DspParameter param, uint16_t paramBits)
	{
		if (info.numParametersEx >= _countof(info.paramsEx))
			return false;

		info.paramsEx[info.numParametersEx] = param;
		info.paramExBits[info.numParametersEx] = paramBits;

		info.numParametersEx++;

		return true;
	}

	bool Analyzer::AddBytes(uint8_t* instrPtr, size_t bytes, AnalyzeInfo& info)
	{
		if (info.sizeInBytes >= sizeof(info.bytes))
			return false;

		memcpy(&info.bytes[info.sizeInBytes], instrPtr, bytes);
		info.sizeInBytes += bytes;

		return true;
	}

	bool Analyzer::Analyze(uint8_t* instrPtr, size_t instrMaxSize, AnalyzeInfo& info)
	{
		// Reset info

		ResetInfo(info);

		// Get opcode and determine its group

		if (instrMaxSize < sizeof(uint16_t))
			return false;

		info.instrBits = MEMSwapHalf (*(uint16_t*)instrPtr);

		if (!AddBytes(instrPtr, sizeof(uint16_t), info))
			return false;

		instrPtr += sizeof(info.instrBits);
		instrMaxSize -= sizeof(info.instrBits);

		int group = (info.instrBits >> 12);

		// Select by group. Groups 3-F in packed format.

		switch (group)
		{
			case 0:
				return Group0(instrPtr, instrMaxSize, info);
			case 1:
				return Group1(instrPtr, instrMaxSize, info);
			case 2:
				return Group2(info);
			case 3:
				if (!Group3(info))
					return false;
				return GroupPacked(info);
			case 4:
				if (!Group4(info))
					return false;
				return GroupPacked(info);
			case 5:
				if (!Group5(info))
					return false;
				return GroupPacked(info);
			case 6:
				if (!Group6(info))
					return false;
				return GroupPacked(info);
			case 7:
				if (!Group7(info))
					return false;
				return GroupPacked(info);
			case 8:
				if (!Group8(info))
					return false;
				return GroupPacked(info);
			case 9:
				if (!Group9(info))
					return false;
				return GroupPacked(info);
			case 0xa:
			case 0xb:
				if (!GroupAB(info))
					return false;
				return GroupPacked(info);
			case 0xc:
			case 0xd:
				if (!GroupCD(info))
					return false;
				return GroupPacked(info);
			case 0xe:
				if (!GroupE(info))
					return false;
				return GroupPacked(info);
			case 0xf:
				if (!GroupF(info))
					return false;
				return GroupPacked(info);
		}

		return true;
	}

}
