#include <cpf/Clause.hpp>

namespace cpf {

Clause::Clause(Variable const& v) : variables{ v } {}

Clause::Clause(std::initializer_list<Variable> variables_) : variables{ variables_ } {}

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


} // namespace cpf