#pragma once

#include <string>
#include <map>

namespace cpf {

struct CmdArg {
    std::string name;
    std::string value;
};

using CmdArgMap = std::map<std::string, std::string>;

CmdArg parse_single_arg(std::string const& arg);
CmdArgMap parse_args(int argc, char const* const* argv);
bool get_argument_as_string(CmdArgMap const& args, std::string const& name, std::string& out);
bool has_argument(CmdArgMap const& args, std::string const& name);
bool get_argument_as_long(CmdArgMap const& args, std::string const& name, long& out);

}