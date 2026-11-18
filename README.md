# dynamic-sat-solver

Shared-memory parallel SAT solver for 15-418. MiniSat is the sequential engine. The project is the scheduler: static guiding-path partitioning versus dynamic cube division, then flip-count splitting, learned-clause sharing, and a portfolio baseline.

## Layout

```
third_party/minisat/   MiniSat 2.2 (niklasso/minisat @ 37dc6c67)
src/Cube.*             guiding path (assumption literals)
src/Worker.*           one MiniSat instance, load CNF, solve under a cube
src/Master.*           scheduler (sequential path only)
src/Metrics.*          wall time, idle time, cubes, steals
src/main.cpp           CLI
benches/               tiny SAT and UNSAT CNFs
```

Two compile fixes in the vendored tree, both so AppleClang accepts it: `memUsedPeak` takes the `bool` argument the header declares, and `mkLit` is an ordinary function instead of a friend with a default argument.

## Build

```bash
cmake -S . -B build
cmake --build build
```

## Run

Exit codes follow the SAT competition convention: 10 satisfiable, 20 unsatisfiable.

```bash
./build/dssolver benches/smoke-sat.cnf
./build/dssolver benches/smoke-unsat.cnf
```

```
--workers N
--mode sequential|static|dynamic
--min-runtime SEC
--heuristic activity|flip
--share-clauses
```

`sequential` with `--workers 1` solves the formula on one MiniSat instance. `static`, `dynamic`, `--workers` other than 1, and `--share-clauses` parse and then fail with a clear error. `Cube::divide` is implemented. Victim selection, interruption-driven splitting, clause sharing, and the flip-count heuristic are not.

## Next

1. Static partitioning: split the root cube until each worker has a subspace, solve with assumptions, cancel on SAT, wait for every cube on UNSAT.
2. Dynamic division: idle worker steals by splitting the longest-running cube at a safe `interrupt()` point, with a minimum runtime before another split.
3. Metrics for idle time, steals, and interrupt latency.
4. Stretch: activity then flip-count variable choice, learned-clause sharing, portfolio mode.
