#pragma once

#include "Clause.hpp"
#include "Graph.hpp"

#include <cassert>
#include <iostream>

class Context {

    static inline constexpr int INVALID_VARIABLE_ID = -1;

public:

    inline Context() = default;

    inline Context(std::size_t makespan, std::size_t agent_count, std::size_t node_count) 
    :   variables(agent_count * node_count * (makespan + 1), INVALID_VARIABLE_ID)
    ,   agent_count{ agent_count }
    ,   node_count{ node_count } {}

    inline Variable create_var(std::size_t time, std::size_t agent_id, node_t node) noexcept {
        auto index = node + agent_id * node_count + time * (node_count * agent_count);
        if (variables[index] == INVALID_VARIABLE_ID) {
            variables[index] = next_variable_id++;
        }
        return Variable(variables[index]);
    }

    inline Variable get_var(std::size_t time, std::size_t agent_id, node_t node) const noexcept {
        assert(contains(time, agent_id, node));
        auto index = node + agent_id * node_count + time * (node_count * agent_count);
        return Variable(variables[index]);
    }

    inline bool contains(std::size_t time, std::size_t agent_id, node_t node) const noexcept {
        auto index = node + agent_id * node_count + time * (node_count * agent_count);
        return variables[index] != INVALID_VARIABLE_ID;
    }

    inline Context& push(Clause clause) {
        clauses.emplace_back(std::move(clause));
        return *this;
    }

    inline std::size_t variables_count() const noexcept {
        return next_variable_id;
    }

    inline std::size_t clauses_count() const noexcept {
        return clauses.size();
    }

    inline std::vector<Clause>::const_iterator begin() const noexcept {
        return std::begin(clauses);
    }

    inline std::vector<Clause>::const_iterator end() const noexcept {
        return std::end(clauses);
    }

private:

    std::vector<int> variables;
    std::size_t agent_count;
    std::size_t node_count;

    int next_variable_id = 0;
    std::vector<Clause> clauses;

};