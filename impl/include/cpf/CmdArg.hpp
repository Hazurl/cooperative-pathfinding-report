#pragma once

#include <map>
#include <string>

namespace cpf {

/*
    Transform (argc, argv) into a map where each parameters of the form "xxx=yyy" set the value yyy at key xxx
*/
struct CmdArg {
	std::string name;
	std::string value;
};

using CmdArgMap = std::map<std::string, std::string>;

CmdArg parse_single_arg(std::string const& arg);
CmdArgMap parse_args(int argc, char const* const* argv);

/* Retrieve arguments from the map */
bool get_argument_as_string(CmdArgMap const& args, std::string const& name, std::string& out);
bool has_argument(CmdArgMap const& args, std::string const& name);
bool get_argument_as_long(CmdArgMap const& args, std::string const& name, long& out);

} // namespace cpf