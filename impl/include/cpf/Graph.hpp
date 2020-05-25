#pragma once

#include <vector>
#include <limits>
#include <algorithm>

namespace cpf {

using node_t = std::size_t;
constexpr node_t INVALID_NODE = std::numeric_limits<node_t>::max();

class Graph {
public:

    Graph(std::size_t node_count_) noexcept;

    typename std::vector<bool>::const_reference operator[](std::pair<node_t, node_t> p) const noexcept;
    typename std::vector<bool>::reference operator[](std::pair<node_t, node_t> p) noexcept;

    std::size_t size() const noexcept;
    std::size_t edge_count() const noexcept;

    std::vector<node_t> neighbours_of(node_t node) const noexcept;

private:

    std::vector<bool> edges;
    std::size_t node_count;

};

}