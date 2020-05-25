#pragma once

#include "Agent.hpp"
#include "Graph.hpp"

#include <iostream>
#include <limits>
#include <vector>

namespace cpf {

/*
	Class encapsulating a MDD, which is also able to iteratively increment the distance
*/
class MDD {
private:
	struct Distance {
		std::size_t from_initial = std::numeric_limits<std::size_t>::max();
		std::size_t from_goal	 = std::numeric_limits<std::size_t>::max();
	};

	Agent agent;
	Graph const* graph;
	std::vector<Distance> nodes_to_distances;
	std::vector<node_t> next_nodes_initial;
	std::vector<node_t> next_nodes_goal;
	std::size_t next_distance = 0;

public:
	MDD(Graph const& graph_, Agent const& agent_) noexcept;

	void step() noexcept;

	void step_until(std::size_t makespan);

	bool accessible(node_t node, std::size_t time, std::size_t makespan);
};

} // namespace cpf