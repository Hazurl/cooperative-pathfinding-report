#pragma once

#include "Variable.hpp"
#include <vector>

namespace cpf {

/*
 	A clause is a series of `or` where each term is a variable negated or not
	It need to be this way because the solver expect clause in conjunction normal form
*/

class Clause {
public:
	Clause() = default;
	Clause(Variable const& v);
	Clause(std::initializer_list<Variable> variables_);

	std::vector<Variable> variables;
};

/*
	Syntax appealing methods to 'or' together multiples variables
*/
Clause operator|(Variable const& lhs, Variable const& rhs);
Clause& operator|=(Clause& lhs, Variable const& rhs);
Clause&& operator|(Clause&& lhs, Variable const& rhs);

} // namespace cpf