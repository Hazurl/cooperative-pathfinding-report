#pragma once

#include <vector>
#include "Variable.hpp"

// A clause is a series of `or` where each term is a variable negated or not
// it need to be this way because the solver expect clause in conjunction normal form

class Clause {
public:

    inline Clause() = default;
    inline Clause(Variable const& v) : variables{ v } {}
    inline Clause(std::initializer_list<Variable> variables) : variables{ variables } {}

    std::vector<Variable> variables;

};

Clause operator|(Variable const& lhs, Variable const& rhs) {
    return { lhs, rhs };
}

Clause& operator|=(Clause& lhs, Variable const& rhs) {
    lhs.variables.emplace_back(rhs);
    return lhs;
}
Clause&& operator|(Clause&& lhs, Variable const& rhs) {
    lhs.variables.emplace_back(rhs);
    return std::move(lhs);
}