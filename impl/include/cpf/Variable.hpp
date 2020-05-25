#pragma once

namespace cpf {


constexpr int INVALID_VARIABLE_ID = -1;

class Variable {
public:

    constexpr Variable(int id_, bool negated_ = false) noexcept : id{ id_ }, negated{ negated_ } {}

    constexpr Variable operator!() const noexcept {
        return Variable(id, !negated);
    }

    int id;
    bool negated;

};

}