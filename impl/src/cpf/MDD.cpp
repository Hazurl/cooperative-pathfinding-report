#include <cpf/MDD.hpp>

namespace cpf {

MDD::MDD(Graph const& graph_, Agent const& agent_) noexcept
	: agent{ agent_ }
	, graph{ &graph_ }
	, nodes_to_distances(graph_.size())
	, next_nodes_initial{ { agent_.initial } }
	, next_nodes_goal{ { agent_.goal } } {}


/*
	Perform two breadth-first-search, one from the end and one from the start
	essentially calculating if `dist(start, v) + dist(v, end) <= makespan` for each nodes
*/
void MDD::step() noexcept {
	std::vector<node_t> next_next_initial;
	std::vector<node_t> next_next_goal;

	for (auto node : next_nodes_initial) {
		nodes_to_distances[node].from_initial = next_distance;
		for (auto neighbour : graph->neighbours_of(node)) {
			if (nodes_to_distances[neighbour].from_initial == std::numeric_limits<std::size_t>::max()) {
				next_next_initial.push_back(neighbour);
			}
		}
	}

	next_nodes_initial = std::move(next_next_initial);

	for (auto node : next_nodes_goal) {
		nodes_to_distances[node].from_goal = next_distance;
		for (auto neighbour : graph->neighbours_of(node)) {
			if (nodes_to_distances[neighbour].from_goal == std::numeric_limits<std::size_t>::max()) {
				next_next_goal.push_back(neighbour);
			}
		}
	}

	next_nodes_goal = std::move(next_next_goal);
	++next_distance;
}

void MDD::step_until(std::size_t makespan) {
	while (next_distance <= makespan) { step(); }
}

bool MDD::accessible(node_t node, std::size_t time, std::size_t makespan) {
	auto const& dist = nodes_to_distances[node];
	return time >= dist.from_initial && (makespan - time) >= dist.from_goal;
}

} // namespace cpf