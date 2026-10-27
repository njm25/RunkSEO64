#pragma once

#include <string_view>

//hover description for a cfg var, nullptr if it has none
const char *getTooltip(std::string_view cfg_var_name);
