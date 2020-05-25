#pragma once

#include <iostream>
#include <vector>
#include <utility>

#include "Graph.hpp"
#include "Agent.hpp"

namespace cpf {

std::pair<Graph, std::vector<Agent>> deserialize(std::istream& is);

void serialize(std::ostream& os, Graph const& graph, std::vector<Agent> const& agents);

}