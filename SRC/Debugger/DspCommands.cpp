// Dsp debug commands

#include "pch.h"

namespace Debug
{

    void dsp_init_handlers()
    {
        con.cmds["dspdisa"] = cmd_dspdisa;
        con.cmds["dregs"] = cmd_dregs;
        con.cmds["dreg"] = cmd_dreg;
        con.cmds["dmem"] = cmd_dmem;
        con.cmds["imem"] = cmd_imem;
        con.cmds["drun"] = cmd_drun;
        con.cmds["dstop"] = cmd_dstop;
        con.cmds["dstep"] = cmd_dstep;
        con.cmds["dbrk"] = cmd_dbrk;
        con.cmds["dcan"] = cmd_dcan;
        con.cmds["dlist"] = cmd_dlist;
        con.cmds["dbrkclr"] = cmd_dbrkclr;
        con.cmds["dcanclr"] = cmd_dcanclr;
        con.cmds["dpc"] = cmd_dpc;
        con.cmds["dreset"] = cmd_dreset;
        con.cmds["du"] = cmd_du;
        con.cmds["dst"] = cmd_dst;
        con.cmds["difx"] = cmd_difx;
        con.cmds["cpumbox"] = cmd_cpumbox;
        con.cmds["dspmbox"] = cmd_dspmbox;
        con.cmds["dspint"] = cmd_dspint;
    }

    void dsp_help()
    {
        DBReport("--- dsp debug commands --------------------------------------------------------\n");
        DBReport("    dspdisa              - Disassemble DSP code into text file\n");
        DBReport("    dregs                - Show DSP registers\n");
        DBReport("    dreg <reg> <value>   - Modify DSP register\n");
        DBReport("    dmem                 - Dump DSP DMEM\n");
        DBReport("    imem                 - Dump DSP IMEM\n");
        DBReport("    drun                 - Run DSP thread until break, halt or dstop\n");
        DBReport("    dstop                - Stop DSP thread\n");
        DBReport("    dstep [n]            - Step DSP instruction(s)\n");
        DBReport("    dbrk                 - Add IMEM breakpoint\n");
        DBReport("    dcan                 - Add IMEM canary\n");
        DBReport("    dlist                - List IMEM breakpoints\n");
        DBReport("    dbrkclr              - Clear all IMEM breakpoints\n");
        DBReport("    dcanclr              - Clear all IMEM canaries\n");
        DBReport("    dpc                  - Set DSP program counter\n");
        DBReport("    dreset               - Issue DSP reset\n");
        DBReport("    du [addr] [count]    - Disassemble some DSP instructions at pc / address\n");
        DBReport("    dst                  - Dump DSP call stack\n");
        DBReport("    difx                 - Dump DSP IFX (internal hardware)\n");
        DBReport("    cpumbox              - Write message to CPU Mailbox\n");
        DBReport("    dspmbox              - Read message from DSP Mailbox\n");
        DBReport("    dspint               - Send CPU->DSP interrupt\n");
        DBReport("\n");
    }

    // disasm dsp ucode to file
    void cmd_dspdisa(std::vector<std::string>& args)
    {
        if (args.size() < 2)
        {
            DBReport("syntax: dspdisa <dsp_ucode.bin> [start_addr]\n");
            DBReport("disassemble dsp ucode from binary file and dump it into dspdisa.txt\n");
            DBReport("start_addr in DSP slots;\n");
            DBReport("example of use: dspdisa Data\\dsp_irom.bin 0x8000\n");
            return;
        }

        size_t start_addr = 0;      // in DSP slots (halfwords)

        if (args.size() >= 3)
        {
            start_addr = strtoul(args[2].c_str(), nullptr, 0);
        }

        uint32_t ucodeSize = 0;
        uint8_t* ucode = (uint8_t*)FileLoad(args[1].c_str(), &ucodeSize);
        if (!ucode)
        {
            DBReport("Failed to load %s\n", args[1].c_str());
            return;
        }

        FILE* f = nullptr;
        fopen_s(&f, "Data\\dspdisa.txt", "wt");
        if (!f)
        {
            free(ucode);
            DBReport("Failed to create dsp_disa.txt\n");
            return;
        }

        uint8_t* ucodePtr = ucode;
        size_t bytesLeft = ucodeSize;
        size_t offset = 0;      // in DSP slots (halfwords)

        if (f)
        {
            fprintf(f, "// Disassembled %s\n\n", args[1].c_str());
        }

        while (bytesLeft != 0)
        {
            DSP::AnalyzeInfo info = { 0 };

            // Analyze

            bool result = DSP::Analyzer::Analyze(ucodePtr, ucodeSize - 2 * offset, info);
            if (!result)
            {
                DBReport("DSP::Analyze failed at offset: 0x%08X\n", offset);
                break;
            }

            // Disassemble

            std::string text = DSP::DspDisasm::Disasm((uint16_t)(offset + start_addr), info);

            if (f)
            {
                fprintf(f, "%s\n", text.c_str());
            }

            offset += (info.sizeInBytes / sizeof(uint16_t));
            bytesLeft -= info.sizeInBytes;
            ucodePtr += info.sizeInBytes;
        }

        free(ucode);

        if (f)
        {
            fflush(f);
            fclose(f);
        }

        DBReport("Done.\n");
    }

    // Show dsp registers
    void cmd_dregs(std::vector<std::string>& args)
    {
        if (!dspCore)
        {
            DBReport("DspCore not ready\n");
            return;
        }

        // A special trick to display all registers (we intentionally make them dirty by inverting bits)
        DSP::DspRegs regsChanged = dspCore->regs;

        regsChanged.pc = ~regsChanged.pc;
        regsChanged.prod.bitsUnpacked = ~regsChanged.prod.bitsUnpacked;
        regsChanged.cr = ~regsChanged.cr;
        regsChanged.sr.bits = ~regsChanged.sr.bits;

        for (int i = 0; i < 4; i++)
        {
            regsChanged.ar[i] = ~regsChanged.ar[i];
            regsChanged.ix[i] = ~regsChanged.ix[i];
            regsChanged.gpr[i] = ~regsChanged.gpr[i];
        }

        for (int i = 0; i < 2; i++)
        {
            regsChanged.ac[i].bits = ~regsChanged.ac[i].bits;
            regsChanged.ax[i].bits = ~regsChanged.ax[i].bits;
        }

        dspCore->DumpRegs(&regsChanged);
    }

    // Dump DSP DMEM
    void cmd_dmem(std::vector<std::string>& args)
    {
        if (!dspCore)
        {
            DBReport("DspCore not ready\n");
            return;
        }

        if (args.size() < 2)
        {
            DBReport("syntax: dmem <dsp_addr>, dmem .\n");
            DBReport("Dump 32 bytes of DMEM at dsp_addr. dsp_addr in halfword DSP slots.\n");
            DBReport("dmem . will dump 0x800 bytes at dmem address 0\n");
            DBReport("example of use: dmem 0x8000\n");
            return;
        }

        DSP::DspAddress dsp_addr = 0;
        size_t bytes = 32;

        if (args[1].c_str()[0] == '.')
        {
            dsp_addr = 0;
            bytes = 0x800;
        }
        else
        {
            dsp_addr = (DSP::DspAddress)strtoul(args[1].c_str(), nullptr, 0);
        }

        DBReport("DMEM Dump %i bytes\n", bytes);

        while (bytes != 0)
        {
            uint8_t* ptr = dspCore->TranslateDMem(dsp_addr);
            if (ptr == nullptr)
            {
                DBReport2(DbgChannel::DSP, "TranslateDMem failed on dsp addr: 0x%04X\n", dsp_addr);
                break;
            }

            DBReport("%04X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X\n",
                dsp_addr,
                ptr[0], ptr[1], ptr[2], ptr[3],
                ptr[4], ptr[5], ptr[6], ptr[7],
                ptr[8], ptr[9], ptr[0xa], ptr[0xb],
                ptr[0xc], ptr[0xd], ptr[0xe], ptr[0xf]);

            bytes -= 0x10;
            dsp_addr += 0x10;
        }
    }

    // Dump DSP IMEM
    void cmd_imem(std::vector<std::string>& args)
    {
        if (!dspCore)
        {
            DBReport("DspCore not ready\n");
            return;
        }

        if (args.size() < 2)
        {
            DBReport("syntax: imem <dsp_addr>, imem .\n");
            DBReport("Dump 32 bytes of IMEM at dsp_addr. dsp_addr in halfword DSP slots.\n");
            DBReport("imem . will dump 32 bytes of imem at program counter address.\n");
            DBReport("example of use: imem 0\n");
            return;
        }

        DSP::DspAddress dsp_addr = 0;
        size_t bytes = 32;

        if (args[1].c_str()[0] == '.')
        {
            dsp_addr = dspCore->regs.pc;
        }
        else
        {
            dsp_addr = (DSP::DspAddress)strtoul(args[1].c_str(), nullptr, 0);
        }

        DBReport("IMEM Dump %i bytes\n", bytes);

        while (bytes != 0)
        {
            uint8_t* ptr = dspCore->TranslateIMem(dsp_addr);
            if (ptr == nullptr)
            {
                DBReport2(DbgChannel::DSP, "TranslateIMem failed on dsp addr: 0x%04X\n", dsp_addr);
                break;
            }

            DBReport("%04X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X\n",
                dsp_addr,
                ptr[0], ptr[1], ptr[2], ptr[3],
                ptr[4], ptr[5], ptr[6], ptr[7],
                ptr[8], ptr[9], ptr[0xa], ptr[0xb],
                ptr[0xc], ptr[0xd], ptr[0xe], ptr[0xf]);

            bytes -= 0x10;
            dsp_addr += 0x10;
        }
    }

    // Run DSP thread until break, halt or dstop
    void cmd_drun(std::vector<std::string>& args)
    {
        if (!dspCore)
        {
            DBReport("DspCore not ready\n");
            return;
        }

        dspCore->Run();
    }

    // Stop DSP thread
    void cmd_dstop(std::vector<std::string>& args)
    {
        if (!dspCore)
        {
            DBReport("DspCore not ready\n");
            return;
        }

        dspCore->Suspend();
    }

    // Step DSP instruction
    void cmd_dstep(std::vector<std::string>& args)
    {
        if (!dspCore)
        {
            DBReport("DspCore not ready\n");
            return;
        }

        if (dspCore->IsRunning())
        {
            DBReport2(DbgChannel::DSP, "It is impossible while running DSP thread.\n");
            return;
        }

        int n = 1;

        if (args.size() > 1)
        {
            n = atoi(args[1].c_str());
        }

        while (n--)
        {
            DSP::DspRegs prevRegs = dspCore->regs;

            // Show instruction to be executed

            DSP::DspAddress pcAddr = dspCore->regs.pc;

            uint8_t* imemPtr = dspCore->TranslateIMem(pcAddr);
            if (imemPtr == nullptr)
            {
                DBReport2(DbgChannel::DSP, "TranslateIMem failed on dsp addr: 0x%04X\n", pcAddr);
                return;
            }

            DSP::AnalyzeInfo info = { 0 };

            if (!DSP::Analyzer::Analyze(imemPtr, DSP::DspCore::MaxInstructionSizeInBytes, info))
            {
                DBReport2(DbgChannel::DSP, "DSP Analyzer failed on dsp addr: 0x%04X\n", pcAddr);
                return;
            }

            std::string code = DSP::DspDisasm::Disasm(pcAddr, info);

            DBReport("%s\n", code.c_str());

            dspCore->Step();

            // Dump modified regs
            dspCore->DumpRegs(&prevRegs);
        }
    }

    // Add IMEM breakpoint
    void cmd_dbrk(std::vector<std::string>& args)
    {
        if (!dspCore)
        {
            DBReport("DspCore not ready\n");
            return;
        }

        if (args.size() < 2)
        {
            DBReport("syntax: dbrk <dsp_addr>\n");
            DBReport("Add breakpoint at dsp_addr. dsp_addr in halfword DSP slots.\n");
            DBReport("example of use: dbrk 0x8020\n");
            return;
        }

        DSP::DspAddress dsp_addr = (DSP::DspAddress)strtoul(args[1].c_str(), nullptr, 0);

        dspCore->AddBreakpoint(dsp_addr);

        DBReport("DSP breakpoint added: 0x%04X\n", dsp_addr);
    }

    // Add IMEM canary
    void cmd_dcan(std::vector<std::string>& args)
    {
        if (!dspCore)
        {
            DBReport("DspCore not ready\n");
            return;
        }

        if (args.size() < 3)
        {
            DBReport("syntax: dcan <dsp_addr> <message>\n");
            DBReport("Add canary at dsp_addr. dsp_addr in halfword DSP slots.\n");
            DBReport("When the PC is equal to the canary address, a debug message is displayed\n");
            DBReport("example of use: dcan 0x10 \"Ucode entrypoint\"\n");
            return;
        }

        DSP::DspAddress dsp_addr = (DSP::DspAddress)strtoul(args[1].c_str(), nullptr, 0);

        dspCore->AddCanary(dsp_addr, args[2]);

        DBReport("DSP canary added: 0x%04X\n", dsp_addr);
    }

    // List IMEM breakpoints and canaries
    void cmd_dlist(std::vector<std::string>& args)
    {
        if (!dspCore)
        {
            DBReport("DspCore not ready\n");
            return;
        }

        DBReport("DSP breakpoints:\n");

        dspCore->ListBreakpoints();

        DBReport("DSP canaries:\n");

        dspCore->ListCanaries();
    }

    // Clear all IMEM breakpoints
    void cmd_dbrkclr(std::vector<std::string>& args)
    {
        if (!dspCore)
        {
            DBReport("DspCore not ready\n");
            return;
        }

        dspCore->ClearBreakpoints();

        DBReport("DSP breakpoints cleared.\n");
    }

    // Clear all IMEM canaries
    void cmd_dcanclr(std::vector<std::string>& args)
    {
        if (!dspCore)
        {
            DBReport("DspCore not ready\n");
            return;
        }

        dspCore->ClearCanaries();

        DBReport("DSP canaries cleared.\n");
    }

    // Set DSP program counter
    void cmd_dpc(std::vector<std::string>& args)
    {
        if (!dspCore)
        {
            DBReport("DspCore not ready\n");
            return;
        }

        if (dspCore->IsRunning())
        {
            DBReport2(DbgChannel::DSP, "It is impossible while running DSP thread.\n");
            return;
        }

        if (args.size() < 2)
        {
            DBReport("syntax: dpc <dsp_addr>\n");
            DBReport("Set DSP program counter to dsp_addr. dsp_addr in halfword DSP slots.\n");
            DBReport("example of use: dpc 0x8000\n");
            return;
        }

        dspCore->regs.pc = (DSP::DspAddress)strtoul(args[1].c_str(), nullptr, 0);
    }

    // Issue DSP reset
    void cmd_dreset(std::vector<std::string>& args)
    {
        if (!dspCore)
        {
            DBReport("DspCore not ready\n");
            return;
        }

        dspCore->HardReset();
    }

    // Disassemble some DSP instructions at program counter
    void cmd_du(std::vector<std::string>& args)
    {
        if (!dspCore)
        {
            DBReport("DspCore not ready\n");
            return;
        }

        if (dspCore->IsRunning())
        {
            DBReport2(DbgChannel::DSP, "It is impossible while running DSP thread.\n");
            return;
        }

        size_t instrCount = 8;
        DSP::DspAddress addr = 0;

        if (args.size() < 2)
        {
            addr = dspCore->regs.pc;
        }
        else
        {
            addr = (DSP::DspAddress)strtoul(args[1].c_str(), nullptr, 0);
        }

        if (args.size() < 3)
        {
            instrCount = 8;
        }
        else
        {
            instrCount = atoi(args[2].c_str());
        }

        while (instrCount--)
        {
            uint8_t* imemPtr = dspCore->TranslateIMem(addr);
            if (imemPtr == nullptr)
            {
                DBReport2(DbgChannel::DSP, "TranslateIMem failed on dsp addr: 0x%04X\n", addr);
                break;
            }

            DSP::AnalyzeInfo info = { 0 };

            if (!DSP::Analyzer::Analyze(imemPtr, DSP::DspCore::MaxInstructionSizeInBytes, info))
            {
                DBReport2(DbgChannel::DSP, "DSP Analyzer failed on dsp addr: 0x%04X\n", addr);
                return;
            }

            std::string code = DSP::DspDisasm::Disasm(addr, info);

            DBReport("%s\n", code.c_str());

            addr += (DSP::DspAddress)(info.sizeInBytes >> 1);
        }
    }

    // Dump DSP call stack
    void cmd_dst(std::vector<std::string>& args)
    {
        if (!dspCore)
        {
            DBReport("DspCore not ready\n");
            return;
        }

        if (dspCore->IsRunning())
        {
            DBReport2(DbgChannel::DSP, "It is impossible while running DSP thread.\n");
            return;
        }

        DBReport("DSP Call Stack:\n");

        for (auto it = dspCore->regs.st[0].begin(); it != dspCore->regs.st[0].end(); ++it)
        {
            DBReport("0x%04X\n", *it);
        }
    }

    // Dump DSP IFX
    void cmd_difx(std::vector<std::string>& args)
    {
        if (!dspCore)
        {
            DBReport("DspCore not ready\n");
            return;
        }

        if (dspCore->IsRunning())
        {
            DBReport2(DbgChannel::DSP, "It is impossible while running DSP thread.\n");
            return;
        }

        DBReport("DSP IFX Dump:\n");

        dspCore->DumpIfx();
    }

    // Write message to CPU Mailbox
    void cmd_cpumbox(std::vector<std::string>& args)
    {
        if (!dspCore)
        {
            DBReport("DspCore not ready\n");
            return;
        }

        if (args.size() < 2)
        {
            DBReport("syntax: cpumbox <value>\n");
            DBReport("example of use: cpumbox 0x8001FEED\n");
            return;
        }

        uint32_t value = strtoul(args[1].c_str(), nullptr, 0);
        
        dspCore->CpuToDspWriteHi(value >> 16);
        dspCore->CpuToDspWriteLo((uint16_t)value);
    }

    // Read message from DSP Mailbox
    void cmd_dspmbox(std::vector<std::string>& args)
    {
        if (!dspCore)
        {
            DBReport("DspCore not ready\n");
            return;
        }

        uint32_t value = 0;

        value |= dspCore->DspToCpuReadHi(true) << 16;
        if ((value & 0x80000000) == 0)
        {
            DBReport("No DSP message.\n");
            return;
        }
        value |= dspCore->DspToCpuReadLo();
        DBReport("DSP Message: 0x%08X\n", value);
    }

    // Send CPU->DSP interrupt
    void cmd_dspint(std::vector<std::string>& args)
    {
        if (!dspCore)
        {
            DBReport("DspCore not ready\n");
            return;
        }

        dspCore->DSPSetIntBit(true);
    }

    // Modify DSP register
    void cmd_dreg(std::vector<std::string>& args)
    {
        if (!dspCore)
        {
            DBReport("DspCore not ready\n");
            return;
        }

        if (dspCore->IsRunning())
        {
            DBReport2(DbgChannel::DSP, "It is impossible while running DSP thread.\n");
            return;
        }

        static const char* dspRegNames[] = {
		    "ar0", "ar1", "ar2", "ar3",
            "ix0", "ix1", "ix2", "ix3",
            "r8", "r9", "r10", "r11",
            "st0", "st1", "st2", "st3",
            "ac0h", "ac1h",
            "config", "sr",
            "prodl", "prodm1", "prodh", "prodm2",
            "ax0l", "ax0h", "ax1l", "ax1h",
            "ac0l", "ac1l", "ac0m", "ac1m"
        };

        if (args.size() < 3)
        {
            std::string reglist = "";

            int regscount = 0;

            for (int i = 0; i < _countof(dspRegNames); i++)
            {
                reglist += std::string(dspRegNames[i]) + " ";
                regscount++;
                if (regscount >= 8)
                {
                    regscount = 0;
                    reglist += "\n";
                }
            }

            DBReport("syntax: dreg <register> <value>\n");
            DBReport("Register names: %s\n", reglist.c_str());
            DBReport("example of use: dreg ar0 0x300\n");

            return;
        }

        uint16_t value = (uint16_t)strtoul(args[2].c_str(), nullptr, 0);

        int regIndex = -1;

        for (int i = 0; i < _countof(dspRegNames); i++)
        {
            if (!_stricmp(args[1].c_str(), dspRegNames[i]))
            {
                regIndex = i;
                break;
            }
        }

        if (regIndex < 0)
        {
            DBReport("Invalid register name: %s\n", args[1].c_str());
            return;
        }

        dspCore->MoveToReg(regIndex, value);
    }

}
