#include <string>
#include <vector>
#include <optional>
#include <iostream>
#include <fstream>
#include <cstring>
#include <random>
#include <unordered_map>

#include <cpf/Clause.hpp>
#include <cpf/Variable.hpp>
#include <cpf/Context.hpp>
#include <cpf/Graph.hpp>
#include <cpf/Agent.hpp>
#include <cpf/FileSerializer.hpp>
#include <cpf/CmdArg.hpp>

void print_help(char const* prog_name) {
    std::cerr << "Usage: " << prog_name << " <options>\n";
    std::cerr << "Options:\n";
    std::cerr << "\t--output=<file>  File writen to, in CPF format, otherwise write to standard output stream\n";
    std::cerr << "\t--size=<value>   Size of the grid's size [DEFAULT: 4]\n";
    std::cerr << "\t--wall%=<value>  Percentage of walls in the grid [DEFAULT: 20]\n";
    std::cerr << "\t--agent%=<value> Percentage of agents in the grid [DEFAULT: 10]\n";
    std::cerr << "\t--display=<file> Display the grid in the file, no value will write to standard output stream\n";
}

int get_size(cpf::CmdArgMap const& args) {
    long o;
    if (cpf::get_argument_as_long(args, "size", o)) {
        return static_cast<int>(o);
    } else {
        return 4;
    }
}

int get_wall_percent(cpf::CmdArgMap const& args) {
    long o;
    if (cpf::get_argument_as_long(args, "wall%", o)) {
        return static_cast<int>(o);
    } else {
        return 20;
    }
}

int get_agent_percent(cpf::CmdArgMap const& args) {
    long o;
    if (cpf::get_argument_as_long(args, "agent%", o)) {
        return static_cast<int>(o);
    } else {
        return 10;
    }
}


int main(int argc, char** argv)
{
    auto args = cpf::parse_args(argc, argv);
    if (cpf::has_argument(args, "help") || cpf::has_argument(args, "h")) {
        print_help(argv[0]);
        return 0;
    }

    std::ofstream ofile;
    std::string output_filename;

    std::ostream* output_stream;

    if (cpf::get_argument_as_string(args, "output", output_filename)) {
        ofile.open(output_filename);
        if (!ofile) {
            std::cerr << "Unable to read file '" << output_filename << "'\n";
            return 2;
        }

        output_stream = &ofile;
    } else {
        output_stream = &std::cout;
    }

    std::ofstream displayfile;
    std::string display_filename;

    std::ostream* display_stream = nullptr;

    if (cpf::get_argument_as_string(args, "display", display_filename)) {
        if (display_filename.empty()) {
            display_stream = &std::cout;
        } else {
            displayfile.open(display_filename);
            if (!displayfile) {
                std::cerr << "Unable to read file '" << display_filename << "'\n";
                return 2;
            }

            display_stream = &displayfile;
        }
    }

    int size = get_size(args);
    int wall_percent = get_wall_percent(args);
    int agent_percent = get_agent_percent(args);

    if (size <= 0) {
        std::cerr << "Size must not be negative\n";
        print_help(argv[0]);
        return 1;
    } 

    if (wall_percent < 0 || wall_percent > 100) {
        std::cerr << "The wall percentage must be in range 0..100\n";
        print_help(argv[0]);
        return 1;
    } 

    if (agent_percent < 0 || agent_percent > 100) {
        std::cerr << "The agent percentage must be in range 0..100\n";
        print_help(argv[0]);
        return 1;
    } 

    int nodes_count = size * size;
    int wall_count = nodes_count * wall_percent / 100;
    int agent_count = nodes_count * agent_percent / 100;

    if (wall_count + agent_count >= nodes_count) {
        std::cerr << "There's too much agents and walls\n";
        print_help(argv[0]);
        return 1;
    }

    std::vector<bool> is_wall(static_cast<std::size_t>(nodes_count));
    std::vector<cpf::Agent> agents(static_cast<std::size_t>(agent_count));
    cpf::Graph graph(static_cast<std::size_t>(nodes_count));

    {
        std::mt19937 eng(std::random_device{}());
        std::vector<std::size_t> nodes_id(static_cast<std::size_t>(nodes_count));
        std::iota(std::begin(nodes_id), std::end(nodes_id), std::size_t{ 0 });
        std::shuffle(std::begin(nodes_id), std::end(nodes_id), eng);
        for(std::size_t i = 0; i < static_cast<std::size_t>(wall_count); ++i) {
            is_wall[nodes_id[i]] = true;
        }

        for(std::size_t i = 0; i < static_cast<std::size_t>(agent_count); ++i) {
            agents[i].initial = nodes_id[wall_count + i];
        }

        std::shuffle(std::begin(nodes_id) + wall_count, std::end(nodes_id), eng);

        for(std::size_t i = 0; i < static_cast<std::size_t>(agent_count); ++i) {
            agents[i].goal = nodes_id[wall_count + i];
        }
    }

    for(std::size_t x = 0; x < static_cast<std::size_t>(size); ++x) {
        for(std::size_t y = 0; y < static_cast<std::size_t>(size); ++y) {
            auto node = x + y * size;

            if (is_wall[node]) {
                continue;
            }

            if (y > 0) {
                auto neighbour = x + (y - 1) * size;
                if (!is_wall[neighbour]) {
                    graph[{node, neighbour}] = true;
                }
            }

            if (y < static_cast<std::size_t>(size) - 1) {
                auto neighbour = x + (y + 1) * size;
                if (!is_wall[neighbour]) {
                    graph[{node, neighbour}] = true;
                }
            }

            if (x > 0) {
                auto neighbour = x - 1 + y * size;
                if (!is_wall[neighbour]) {
                    graph[{node, neighbour}] = true;
                }
            }

            if (x < static_cast<std::size_t>(size) - 1) {
                auto neighbour = x + 1 + y * size;
                if (!is_wall[neighbour]) {
                    graph[{node, neighbour}] = true;
                }
            }
        }
    }

    if (display_stream != nullptr) {
        std::unordered_map<std::size_t, std::size_t> agents_initial;
        std::unordered_map<std::size_t, std::size_t> agents_goal;
        for(std::size_t a = 0; a < agents.size(); ++a) {
            auto const& agent = agents[a];
            agents_initial[agent.initial] = a;
            agents_goal[agent.goal] = a;
        }
        for(std::size_t x = 0; x < static_cast<std::size_t>(size); ++x) {
            for(std::size_t y = 0; y < static_cast<std::size_t>(size); ++y) {
                auto node = x + y * size;
                if (is_wall[node]) {
                    *display_stream << "## ";
                } else {
                    *display_stream << static_cast<char>(agents_initial.count(node) ? agents_initial[node] + 'A' : '.');
                    *display_stream << static_cast<char>(agents_goal.count(node) ? agents_goal[node] + 'a' : '.') << ' ';
                }
            }
            *display_stream << '\n';
        }
    }

    serialize(*output_stream, graph, agents);

    return 0;
}
