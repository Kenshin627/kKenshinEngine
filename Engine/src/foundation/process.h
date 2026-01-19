#pragma once

#include "platform.h"

KENSHIN_BEGIN

bool    processExecute(cstring working_directory, cstring process_fullpath, cstring arguments, cstring search_error_string = "");
cstring processGetOutput();

KENSHIN_END
