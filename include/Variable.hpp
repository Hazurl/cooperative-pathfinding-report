#pragma once

class Variable {
public:

    constexpr Variable(int id, bool negated = false) noexcept : id{ id }, negated{ negated } {}

    constexpr Variable operator!() const noexcept {
        return Variable(id, !negated);
    }

    int id;
    bool negated;

};