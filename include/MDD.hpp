#pragma once

#include "Graph.hpp"
#include "Agent.hpp"

#include <vector>
#include <limits>
#include <iostream>

struct MDD {
    struct Distance {
        std::size_t from_initial = std::numeric_limits<std::size_t>::max();
        std::size_t from_goal = std::numeric_limits<std::size_t>::max();
    };

    Agent agent;
    Graph const* graph;
    std::vector<Distance> nodes_to_distances;
    std::vector<node_t> next_nodes_initial;
    std::vector<node_t> next_nodes_goal;
    std::size_t next_distance = 0;

    inline MDD(Graph const& graph, Agent const& agent) noexcept 
    :   agent{ agent }
    ,   graph{ &graph }
    ,   nodes_to_distances(graph.size())
    ,   next_nodes_initial{{ agent.initial }}
    ,   next_nodes_goal{{ agent.goal }} {}

    inline void step() noexcept {
        std::vector<node_t> next_next_initial;
        std::vector<node_t> next_next_goal;

        for(auto node : next_nodes_initial) {
            nodes_to_distances[node].from_initial = next_distance;
            for(auto neighbour : graph->neighbours_of(node)) {
                if (nodes_to_distances[neighbour].from_initial == std::numeric_limits<std::size_t>::max()) {
                    next_next_initial.push_back(neighbour);
                }
            }
        }

        next_nodes_initial = std::move(next_next_initial);

        for(auto node : next_nodes_goal) {
            nodes_to_distances[node].from_goal = next_distance;
            for(auto neighbour : graph->neighbours_of(node)) {
                if (nodes_to_distances[neighbour].from_goal == std::numeric_limits<std::size_t>::max()) {
                    next_next_goal.push_back(neighbour);
                }
            }
        }

        next_nodes_goal = std::move(next_next_goal);
        ++next_distance;
    }

    inline void step_until(std::size_t makespan) {
        while(next_distance <= makespan) {
            step();
        }
    }

    inline bool accessible(node_t node, std::size_t time, std::size_t makespan) {
        auto const& dist = nodes_to_distances[node];
        return time >= dist.from_initial && (makespan - time) >= dist.from_goal;
    }

};