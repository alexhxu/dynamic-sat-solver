#pragma once

#include <utility>
#include <vector>

#include "minisat/core/SolverTypes.h"

// A guiding path: the assumptions that restrict one search subspace.
struct Cube {
    std::vector<Minisat::Lit> assumptions;

    // Split this cube on variable x into (cube ∧ x) and (cube ∧ ¬x).
    std::pair<Cube, Cube> divide(Minisat::Var x) const;
};
