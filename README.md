# High-Frequency Statistical Arbitrage Engine

A low-latency quantitative finance pipeline that detects risk-free triangular and multi-leg arbitrage opportunities across fragmented order books. The system models live market data as a directed graph and utilizes negative-weight cycle detection algorithms to identify executable arbitrage paths in sub-millisecond timeframes.

Built with **C++** (Core Engine) and **Python** (Data Orchestrator).

---

## The Mathematics: Logarithmic Graph Reduction

In financial markets, an arbitrage opportunity exists if a closed sequence of cross-currency trades yields a net risk-free profit. Mathematically, the product of the exchange rates along the execution path must exceed 1:

$$R_1 \times R_2 \times R_3 \times \dots \times R_k > 1$$

Standard graph algorithms compute cumulative path sums, not multiplicative products. Applying the natural logarithm maps multiplication to addition:

$$\ln(R_1) + \ln(R_2) + \dots + \ln(R_k) > 0$$

Multiplying both sides by $-1$ reverses the inequality:

$$(-\ln R_1) + (-\ln R_2) + \dots + (-\ln R_k) < 0$$

By assigning edge weights as $w(u, v) = -\ln(\text{rate}(u, v))$, any sequence of trades yielding a profit multiplier $> 1$ corresponds directly to a **negative-weight cycle** in the directed graph.

---

## Core Algorithm: Bellman-Ford Negative Cycle Detection

The engine runs a modified Bellman-Ford algorithm to locate, isolate, and extract the profitable trading loop.

### 1. Edge Relaxation Phase ($V - 1$ Iterations)
In a graph with $V$ vertices, the longest simple path (a path with no repeated nodes) contains at most $V - 1$ edges. The algorithm initializes distance vectors with $dist[source] = 0.0$ and all other nodes to $\infty$, alongside a predecessor array $parent$ initialized to $-1$.

For $V - 1$ rounds, every directed edge $(u, v)$ with weight $w$ is relaxed:

$$\text{if } dist[u] + w < dist[v] \implies dist[v] = dist[u] + w, \quad parent[v] = u$$

After $V - 1$ passes, all shortest simple paths are guaranteed to have converged in the absence of negative cycles.

### 2. Tripping the Arbitrage Alarm ($V$-th Pass)
A final $V$-th pass iterates across all edges. If any edge can still be relaxed:

$$dist[u] + w < dist[v]$$

it proves the existence of a path containing at least $V$ edges. By the pigeonhole principle, a path of length $V$ in a graph of $V$ vertices must visit at least one vertex twice—mathematically proving the existence of a negative-weight cycle (arbitrage loop).

### 3. Cycle Isolation: The $V$-Step Predecessor Walkback
A common pitfall in financial graph implementations is assuming the node updated on the $V$-th pass ($v$) is inside the negative cycle. If a cycle feeds into a downstream path, nodes outside the cycle will also update.

To guarantee recovery of vertices strictly within the cycle:
1. Initialize a pointer at the tripped node: $curr = cycle\_start$.
2. Trace backward through the predecessor array exactly $V$ times:
   $$curr \leftarrow parent[curr] \quad (\times V)$$
   Because the graph contains only $V$ nodes, walking back $V$ steps forces the pointer up any downstream tail and guarantees it lands inside the closed loop.
3. Trace from this verified node using a `do-while` loop until it loops back onto itself, capturing the exact cycle.
4. Reverse the predecessor trace to produce the forward chronological sequence of executable trades.

---

## System Architecture

1. **Python Orchestrator (`orchestrator.py`):** 
   - Connects to exchange REST APIs to pull real-time L2 order book data.
   - Injects realistic market friction ($0.01\% - 0.10\%$ slippage and fee hurdles) to eliminate phantom arbitrage.
   - Streams formatted `Source,Destination,Rate` CSV records directly to `stdout`.
2. **C++ Core Engine (`check_arbitrage.cpp`):**
   - Ingests streaming data via `stdin` (live) or reads directly from static CSV files (backtesting).
   - Maps string tickers to contiguous integer IDs dynamically for $O(1)$ vertex indexing.
   - Executes the Bellman-Ford relaxation and extraction routine.
   - Reverses logarithmic edge weights via $\exp(-\sum w_i)$ to output the exact compounded multiplier and net profit percentage.

---

## Performance Benchmarks

Execution times measure pure algorithmic latency (`std::chrono::high_resolution_clock`) with I/O synchronization disabled (`ios_base::sync_with_stdio(false)`):

* **Live Market Stream (Sparse Graph, 7 Coins, 22 Edges):** Executes in **~47 microseconds**.
* **Synthetic Stress Test (Dense Graph, 500 Coins, 249,500 Edges):** Executes in **~3.37 seconds** on a single thread.

---

## Compilation & Usage

Compile using the `-O3` flag for aggressive vectorization and loop optimizations:

```bash
g++ -O3 setup.cpp -o arbitrage_engine
```
### Mode 1: Live Pipeline

Pipe the Python data fetcher into the C++ engine:

```bash
python orchestrator.py | ./check_arbitrage
```
### Mode 2: Historical Order Book Backtesting

Pass an order book CSV directly as a CLI argument:
```bash
./check_arbitrage stress_test_500.csv
```
