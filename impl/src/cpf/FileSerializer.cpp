#include <cpf/FileSerializer.hpp>

namespace cpf {

bool get_next_line(std::istream& is, std::string& line, std::size_t& line_num) {
    do {
        ++line_num;
    } while(!std::getline(is, line) || line.empty() || line.front() ==  '#');
    return static_cast<bool>(is);
} 

std::string get_next_line_or_throw(std::string const& error_hint, std::istream& is, std::size_t& line_num) {
    std::string line;
    if (get_next_line(is, line, line_num)) {
        return line;
    }

    throw std::runtime_error("Couldn't parse file at line " + std::to_string(line_num) + "; Hint: " + error_hint);
}

std::pair<Graph, std::vector<Agent>> deserialize(std::istream& is) {
    std::size_t line_num = 0;

    auto str_graph_size = get_next_line_or_throw("Expecting number of nodes in graph", is, line_num);
    std::size_t graph_size = std::stol(str_graph_size);
    //std::cout << "Graph size: " << graph_size << '\n';

    auto str_graph_edge_count = get_next_line_or_throw("Expecting number of edges in graph", is, line_num);
    std::size_t graph_edge_count = std::stol(str_graph_edge_count);
    //std::cout << "Graph edge count: " << graph_edge_count << '\n';

    Graph graph(graph_size);

    for(std::size_t e = 0; e < graph_edge_count; ++e) {
        auto str_edge = get_next_line_or_throw("Expecting edge #" + std::to_string(e), is, line_num);
        std::size_t column = 0;
        std::size_t edge_first = std::stol(str_edge, &column);
        std::size_t edge_second = std::stol(str_edge.c_str() + column);

        //std::cout << "Graph edge #" << e << ": " << edge_first << ", " << edge_second << '\n';

        graph[{edge_first, edge_second}] = true;
    }

    auto str_agent_count = get_next_line_or_throw("Expecting number of agents", is, line_num);
    std::size_t agent_count = std::stol(str_agent_count);
    //std::cout << "Agent count: " << agent_count << '\n';

    std::vector<Agent> agents;
    agents.reserve(agent_count);

    for(std::size_t a = 0; a < agent_count; ++a) {
        auto str_agent = get_next_line_or_throw("Expecting agent #" + std::to_string(a), is, line_num);
        std::size_t column = 0;
        std::size_t agent_initial = std::stol(str_agent, &column);
        std::size_t agent_goal = std::stol(str_agent.c_str() + column);

        //std::cout << "Agent #" << a << ": " << agent_initial << ", " << agent_goal << '\n';

        agents.push_back({ agent_initial, agent_goal });
    }


    return std::make_pair(
        std::move(graph),
        std::move(agents)
    );
}

void serialize(std::ostream& os, Graph const& graph, std::vector<Agent> const& agents) {
    os << "# Number of nodes\n";
    os << graph.size() << '\n';

    os << "\n# Number of edges\n";
    os << graph.edge_count() << '\n';

    os << "\n# Graph's edges\n";
    for(node_t f = 0; f < graph.size(); ++f) {
        for(node_t s = f; s < graph.size(); ++s) {  
            if (graph[{f, s}]) {
                os << f << ' ' << s << '\n';
            }
        }
    }

    os << "\n# Number of agents\n";
    os << agents.size() << '\n';

    os << "\n# Agents initial and goal nodes\n";
    for(auto const& agent : agents) {
        os << agent.initial << ' ' << agent.goal << '\n';
    }
}

}