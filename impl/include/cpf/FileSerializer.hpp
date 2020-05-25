#pragma once

#include <iostream>
#include <utility>
#include <vector>

#include "Agent.hpp"
#include "Graph.hpp"

namespace cpf {

/*
    Read and write CPF files
*/
std::pair<Graph, std::vector<Agent>> deserialize(std::istream& is);

void serialize(std::ostream& os, Graph const& graph, std::vector<Agent> const& agents);

} // namespace cpf