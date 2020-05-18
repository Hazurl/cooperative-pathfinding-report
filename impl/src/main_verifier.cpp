#include <iostream>
#include <fstream>
#include <vector>
#include <sstream>
#include <cmath>

#include <Graph.hpp>
#include <FileSerializer.hpp>
#include <Agent.hpp>
#include <CmdArg.hpp>

std::vector<std::vector<node_t>> parse_path(std::istream& is) {
    std::vector<std::vector<node_t>> all_path;
    std::string line;
    while(std::getline(is, line)) {
        std::stringstream ss(line);
        all_path.emplace_back();
        auto& path = all_path.back();
        node_t node;
        while(ss >> node) {
            std::cout << node << " ";
            path.push_back(node);
        }
        std::cout << '\n';
    }

    return all_path;
}

int main(int argc, char** argv) {
    auto args = parse_args(argc, argv);

    std::string graph_filename;
    std::string path_filename;

    if (!get_argument_as_string(args, "graph", graph_filename)) {
        std::cerr << "Parameter --graph required\n";
        return 1;
    }

    if (!get_argument_as_string(args, "result", path_filename)) {
        std::cerr << "Parameter --result required\n";
        return 1;
    }

    std::ifstream graph_file(graph_filename);
    std::ifstream path_file(path_filename);

    auto cpf = deserialize(graph_file);
    auto& graph = cpf.first;
    auto& agents = cpf.second;

    auto all_path = parse_path(path_file);

    if(all_path.size() != agents.size()) {
        std::cerr << "Result file doesn't have a path for all agents\n";
    } else {
        for(std::size_t agent = 0; agent < all_path.size(); ++agent) {
            auto const& path = all_path[agent];

            if (path.empty()) continue;

            if (path.front() != agents[agent].initial) {
                std::cerr << "Agent #" << agent << " doesn't start from the initial node #" << agents[agent].initial << ", was on #" << path.front() << '\n';
            }

            if (path.back() != agents[agent].goal) {
                std::cerr << "Agent #" << agent << " doesn't end on the final node #" << agents[agent].goal << ", but on #" << path.back() << '\n';
            }

            for(std::size_t i = 0; i < path.size() - 1; ++i) {
                node_t from = path[i];
                node_t to = path[i + 1];

                if (!graph[{from, to}] && from != to) {
                    std::cerr << "Agent #" << agent << " cross an edge that doesn't exists, ";
                    std::cout << "at timestamp #" << i << ", between node #" << from << " and #" << to << '\n';
                }

                for(std::size_t other_agent = agent + 1; other_agent < all_path.size(); ++other_agent) {
                    auto const& other_path = all_path[other_agent];
                    if (other_path[i + 1] == from && other_path[i] == to) {
                        std::cerr << "Agent #" << agent << " and #" << other_agent << " use the same edge between nodes #" << from << " and #" << to;
                        std::cerr << " at timestamp #" << i << '\n';
                    }

                    if (path[i] == other_path[i]) {
                        std::cerr << "Agent #" << agent << " and #" << other_agent << " use the same node #" << from;
                        std::cerr << " at timestamp #" << i << '\n';
                    }
                }
            }
        }
    }

    std::cout << "Verification done\n";

    auto size = std::sqrt(graph.size());
    std::size_t max_time = all_path.empty() ? 0 : all_path[0].size();
    for(std::size_t t = 0; t < max_time; ++t) {
        for(std::size_t x = 0; x < size; ++x) {
            for(std::size_t y = 0; y < size; ++y) {
                auto node = x + y * size;
                if (graph.neighbours_of(node).empty()) {
                    std::cout << "# ";
                } else {
                    bool found_agent = false;
                    for(std::size_t agent = 0; agent < all_path.size(); ++agent) {
                        auto const& path = all_path[agent];
                        if (path[t] == node) {
                            std::cout << static_cast<char>(agent + 'A') << ' ';
                            found_agent = true;
                            break;
                        }
                    }

                    if (!found_agent) {
                        std::cout << ". ";
                    }
                }
            }
            std::cout << '\n';
        }

        std::cin.get();
        std::cout << '\n';
    }


}