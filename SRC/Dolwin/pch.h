#pragma once

// ---------------------------------------------------------------------------
// compiler and SDK include files.

#include <assert.h>
#include <cstdint>
#include <direct.h>
#include <float.h>
#include <math.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <windows.h>
#include <commctrl.h>
#include <intrin.h>
#include <tchar.h>
#include <filesystem>
#include <fstream>
#include <string>
#include <codecvt>
#include "../../RES/resource.h"

// ---------------------------------------------------------------------------
// Dolwin includes

#include "../Common/Spinlock.h"
#include "../Common/Jdi.h"
#include "../Common/String.h"
#include "../Core/Gekko.h"
#include "../Core/Interpreter.h"
#include "../HighLevel/HighLevel.h"
#include "../Hardware/Hardware.h"
#include "../Debugger/Debugger.h"
#include "../Debugger/DspDebugger.h"
#include "../Debugger/EventLog.h"
#include "../UI/User.h"

#include "Loader.h"
#include "Emulator.h"

#include "../../ThirdParty/fmt/fmt/format.h"
#include "../../ThirdParty/fmt/fmt/printf.h"
