// The memory debugger (PS2 __mem_dbg__.cpp, the first of PS2's game files in their alphabetical order): the original's
// first file (0x401000-0x40102c, docs/game-files.md) has no code but the initializer of std::ctype<unsigned short>::id,
// the marker that ends every file that includes <string>. A release build compiles the debugger out; PS2's file has
// nothing of its own either (its static initializer is the engine headers' statics, the same as every file's).
// As the first object it decides the order of the default libraries: the original's code has libcp's objects after
// LS3DF's thunks, then IJoy's and rw_data's thunks, then LIBC's (game/CMakeLists.txt), so the first object asked for
// the C++ library, IJoy and rw_data in that order, before the compiler's LIBC. The includes give those requests: <string>
// libcp's (USE_ANSI.H), ijoy.h IJoy's and rw_data.h rw_data's.

#include <string>

#include "ijoy.h"
#include "rw_data.h"
