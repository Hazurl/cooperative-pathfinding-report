#pragma once

#include <vector>
#include <limits>
#include <algorithm>

using node_t = std::size_t;
constexpr node_t INVALID_NODE = std::numeric_limits<node_t>::max();

class Graph {
public:

    inline Graph(std::size_t node_count) noexcept : edges(node_count * (node_count + 1) / 2), node_count{ node_count } {}

    inline typename std::vector<bool>::const_reference operator[](std::pair<node_t, node_t> p) const noexcept {
        auto f = p.first;
        auto s = p.second;
        if (f > s) std::swap(f, s);
        return edges[f + s * (s + 1) / 2];
    }

    inline typename std::vector<bool>::reference operator[](std::pair<node_t, node_t> p) noexcept {
        auto f = p.first;
        auto s = p.second;
        if (f > s) std::swap(f, s);
        return edges[f + s * (s + 1) / 2];
    }

    inline constexpr std::size_t size() const noexcept {
        return node_count;
    }

    inline constexpr std::size_t edge_count() const noexcept {
        return std::count(std::begin(edges), std::end(edges), true);
    }

    inline std::vector<node_t> neighbours_of(node_t node) const noexcept {
        std::vector<node_t> neighbours;
        neighbours.reserve(size());
        for(node_t neighbour = 0; neighbour < size(); ++neighbour) {
            if ((*this)[{node, neighbour}]) {
                neighbours.push_back(neighbour);
            }
        }

        return neighbours;
    }

private:

    std::vector<bool> edges;
    std::size_t node_count;

};