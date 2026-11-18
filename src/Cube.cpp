#include "Cube.h"

std::pair<Cube, Cube> Cube::divide(Minisat::Var x) const {
    Cube positive = *this;
    Cube negative = *this;
    positive.assumptions.push_back(Minisat::mkLit(x, false));
    negative.assumptions.push_back(Minisat::mkLit(x, true));
    return {positive, negative};
}
