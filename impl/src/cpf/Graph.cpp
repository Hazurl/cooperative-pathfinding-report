#include <cpf/Graph.hpp>

namespace cpf {

Graph::Graph(std::size_t node_count_) noexcept
	: edges(node_count_ * (node_count_ + 1) / 2)
	, node_count{ node_count_ } {}

typename std::vector<bool>::const_reference Graph::operator[](std::pair<node_t, node_t> p) const noexcept {
	auto f = p.first;
	auto s = p.second;
	if (f > s)
		std::swap(f, s);
	return edges[f + s * (s + 1) / 2];
}

typename std::vector<bool>::reference Graph::operator[](std::pair<node_t, node_t> p) noexcept {
	auto f = p.first;
	auto s = p.second;
	if (f > s)
		std::swap(f, s);
	return edges[f + s * (s + 1) / 2];
}

std::size_t Graph::size() const noexcept {
	return node_count;
}

std::size_t Graph::edge_count() const noexcept {
	return std::count(std::begin(edges), std::end(edges), true);
}

std::vector<node_t> Graph::neighbours_of(node_t node) const noexcept {
	std::vector<node_t> neighbours;
	neighbours.reserve(size());
	for (node_t neighbour = 0; neighbour < size(); ++neighbour) {
		if ((*this)[{ node, neighbour }]) {
			neighbours.push_back(neighbour);
		}
	}

	return neighbours;
}


} // namespace cpf