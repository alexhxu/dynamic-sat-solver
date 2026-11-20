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
tests/                 interrupt test
```

Changes to the vendored tree: `memUsedPeak` takes the `bool` argument the header declares and `mkLit` is an ordinary function instead of a friend with a default argument (both so AppleClang accepts it), and `asynch_interrupt` is atomic (see Interrupt).

## Build

```bash
cmake -S . -B build
cmake --build build
ctest --test-dir build --output-on-failure
```

## Interrupt

`Worker::requestInterrupt()` can be called from any thread. The running `Worker::solve()` returns `l_Undef` at MiniSat's next safe point, with the solver back at decision level 0, and `Worker::currentCube()` still holds the unfinished cube. `solve()` does not clear a pending interrupt: call `clearInterrupt()` before handing a worker new work. `tests/test_interrupt.cpp` checks this on a pigeonhole instance.

The vendored `Solver::asynch_interrupt` is a `std::atomic<bool>` so the cross-thread write is not a data race.

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

`sequential` with `--workers 1` solves the formula on one MiniSat instance. `static` splits the root cube on variables 0, 1, … until each worker has one subspace, solves those in parallel, cancels the rest when one cube is satisfiable, and returns unsatisfiable only after every cube is refuted. `dynamic`, `sequential` with `--workers` other than 1, and `--share-clauses` parse and then fail with a clear error. A static run with more workers than the formula has splits to give out fails the same way.

```bash
./build/dssolver --mode static --workers 4 benches/smoke-sat.cnf
./build/dssolver --mode static --workers 4 benches/smoke-unsat.cnf
```

## Next

1. Dynamic division: idle worker steals by splitting the longest-running cube at a safe `interrupt()` point, with a minimum runtime before another split.
2. Metrics for idle time, steals, and interrupt latency.
3. Flip-count variable choice, learned-clause sharing, and a portfolio baseline. These are part of the planned system, compared against activity-based dynamic division and static partitioning.
