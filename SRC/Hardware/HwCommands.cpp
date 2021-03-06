// Various commands for debugging hardware (Flipper). Available only after emulation has been started.
#include "pch.h"

namespace Flipper
{
	// Load binary file to main memory
	static Json::Value* cmd_ramload(std::vector<std::string>& args)
	{
		uint32_t address = (uint32_t)strtoul(args[2].c_str(), nullptr, 0) & RAMMASK;
		auto data = UI::FileLoad(args[1].c_str());

		if (address >= mi.ramSize || (address + data.size()) >= mi.ramSize)
		{
			DBReport("Address out of range!\n");
			return nullptr;
		}

		if (data.empty())
		{
			DBReport("Failed to load: %s\n", args[1].c_str());
			return nullptr;
		}

		std::memcpy(&mi.ram[address], data.data(), data.size());
		return nullptr;
	}

	// Save main memory content to file
	static Json::Value* cmd_ramsave(std::vector<std::string>& args)
	{
		uint32_t address = (uint32_t)strtoul(args[2].c_str(), nullptr, 0) & RAMMASK;
		uint32_t dataSize = (uint32_t)strtoul(args[3].c_str(), nullptr, 0);

		if (address >= mi.ramSize || (address + dataSize) >= mi.ramSize)
		{
			DBReport("Address out of range!\n");
			return nullptr;
		}

		auto ptr = &mi.ram[address];
		auto buffer = std::vector<uint8_t>();
		buffer.assign(ptr, ptr + dataSize);

		if (!UI::FileSave(args[1].c_str(), buffer))
		{
			DBReport("Failed to save: %s\n", args[1].c_str());
		}
		return nullptr;
	}

	// Load binary file to ARAM
	static Json::Value* cmd_aramload(std::vector<std::string>& args)
	{
		uint32_t address = (uint32_t)strtoul(args[2].c_str(), nullptr, 0);
		auto data = UI::FileLoad(args[1].c_str());

		if (address >= ARAMSIZE || (address + data.size()) >= ARAMSIZE)
		{
			DBReport("Address out of range!\n");
			return nullptr;
		}

		if (data.empty())
		{
			DBReport("Failed to load: %s\n", args[1].c_str());
			return nullptr;
		}

		std::memcpy(&aram.mem[address], data.data(), data.size());
		return nullptr;
	}

	// Save ARAM content to file
	static Json::Value* cmd_aramsave(std::vector<std::string>& args)
	{
		uint32_t address = (uint32_t)strtoul(args[2].c_str(), nullptr, 0);
		uint32_t dataSize = (uint32_t)strtoul(args[3].c_str(), nullptr, 0);

		if (address >= ARAMSIZE || (address + dataSize) >= ARAMSIZE)
		{
			DBReport("Address out of range!\n");
			return nullptr;
		}

		auto ptr = &aram.mem[address];
		auto buffer = std::vector<uint8_t>();
		buffer.assign(ptr, ptr + dataSize);

		if (!UI::FileSave(args[1].c_str(), buffer))
		{
			DBReport("Failed to save: %s\n", args[1].c_str());
		}
		return nullptr;
	}

	// Dump PI/CP FIFO configuration
	static Json::Value* DumpFifo(std::vector<std::string>& args)
	{
		DumpPIFIFO();
		DumpCPFIFO();
		return nullptr;
	}

	void hw_init_handlers()
	{
		Debug::Hub.AddCmd("ramload", cmd_ramload);
		Debug::Hub.AddCmd("ramsave", cmd_ramsave);
		Debug::Hub.AddCmd("aramload", cmd_aramload);
		Debug::Hub.AddCmd("aramsave", cmd_aramsave);
		Debug::Hub.AddCmd("DumpFifo", DumpFifo);
	}
};
