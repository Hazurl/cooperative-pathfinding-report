#pragma once

#include "Clause.hpp"
#include "Graph.hpp"

#include <cassert>
#include <iostream>

namespace cpf {

class Context {
public:

    Context() = default;
    Context(std::size_t makespan, std::size_t agent_count_, std::size_t node_count_);

    Variable create_var(std::size_t time, std::size_t agent_id, node_t node) noexcept;
    Variable get_var(std::size_t time, std::size_t agent_id, node_t node) const noexcept;
    bool contains(std::size_t time, std::size_t agent_id, node_t node) const noexcept;

    Context& push(Clause clause);

    std::size_t variables_count() const noexcept;
    std::size_t clauses_count() const noexcept;

    std::vector<Clause>::const_iterator begin() const noexcept;
    std::vector<Clause>::const_iterator end() const noexcept;
    
private:

    std::vector<int> variables;
    std::size_t agent_count;
    std::size_t node_count;

    int next_variable_id = 0;
    std::vector<Clause> clauses;

};

}