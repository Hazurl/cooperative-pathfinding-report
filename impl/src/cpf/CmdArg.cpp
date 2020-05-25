#include <cpf/CmdArg.hpp>

namespace cpf {

CmdArg parse_single_arg(std::string const& arg) {
    if (arg.empty()) {
        return {};
    }

    std::size_t start_name = arg.find_first_not_of('-');
    std::size_t equal_idx = arg.find_first_of('=');

    return {
        arg.substr(start_name, equal_idx - start_name),
        equal_idx == std::string::npos ? std::string{} : arg.substr(equal_idx + 1)
    };
}

CmdArgMap parse_args(int argc, char const* const* argv) {
    CmdArgMap cmd_args;
    for(int i = 1; i < argc; ++i) {
        auto cmd_arg = parse_single_arg(argv[i]);
        if (cmd_arg.name.empty()) {
            continue;
        }

        cmd_args.emplace(std::move(cmd_arg.name), std::move(cmd_arg.value));
    }

    return cmd_args;
}

bool get_argument_as_string(CmdArgMap const& args, std::string const& name, std::string& out) {
    auto it = args.find(name);
    if (it != std::end(args)) {
        out = it->second;
        return true;
    }

    return false;
}

bool has_argument(CmdArgMap const& args, std::string const& name) {
    auto it = args.find(name);
    return it != std::end(args);
}

bool get_argument_as_long(CmdArgMap const& args, std::string const& name, long& out) {
    auto it = args.find(name);
    if (it != std::end(args)) {
        out = std::stol(it->second);
        return true;
    }

    return false;
}



}