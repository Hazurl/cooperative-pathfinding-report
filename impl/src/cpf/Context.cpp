#include <cpf/Context.hpp>

namespace cpf {

Context::Context(std::size_t makespan, std::size_t agent_count_, std::size_t node_count_) 
:   variables(agent_count_ * node_count_ * (makespan + 1), INVALID_VARIABLE_ID)
,   agent_count{ agent_count_ }
,   node_count{ node_count_ } {}

Variable Context::create_var(std::size_t time, std::size_t agent_id, node_t node) noexcept {
    auto index = node + agent_id * node_count + time * (node_count * agent_count);
    if (variables[index] == INVALID_VARIABLE_ID) {
        variables[index] = next_variable_id++;
    }
    return Variable(variables[index]);
}

Variable Context::get_var(std::size_t time, std::size_t agent_id, node_t node) const noexcept {
    assert(contains(time, agent_id, node));
    auto index = node + agent_id * node_count + time * (node_count * agent_count);
    return Variable(variables[index]);
}

bool Context::contains(std::size_t time, std::size_t agent_id, node_t node) const noexcept {
    auto index = node + agent_id * node_count + time * (node_count * agent_count);
    return variables[index] != INVALID_VARIABLE_ID;
}

Context& Context::push(Clause clause) {
    clauses.emplace_back(std::move(clause));
    return *this;
}

std::size_t Context::variables_count() const noexcept {
    return next_variable_id;
}

std::size_t Context::clauses_count() const noexcept {
    return clauses.size();
}

std::vector<Clause>::const_iterator Context::begin() const noexcept {
    return std::begin(clauses);
}

std::vector<Clause>::const_iterator Context::end() const noexcept {
    return std::end(clauses);
}

}