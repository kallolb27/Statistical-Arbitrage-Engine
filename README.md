# Deterministic Cyclic Arbitrage Engine

A quantitative finance pipeline that detects deterministic triangular and multi-leg arbitrage opportunities. The system models a snapshot of market prices as a directed graph and utilizes negative-weight cycle detection algorithms to identify theoretical arbitrage paths.

Built with **C++** (Core Engine) and **Python** (Data Orchestrator).

---

## The Mathematics: Logarithmic Graph Reduction

In financial markets, an arbitrage opportunity exists if a closed sequence of cross-currency trades yields a net positive profit. Mathematically, the product of the exchange rates along the execution path must exceed 1:

$$R_1 \times R_2 \times R_3 \times \dots \times R_k > 1$$

Standard graph algorithms compute cumulative path sums, not multiplicative products. Applying the natural logarithm maps multiplication to addition:

$$\ln(R_1) + \ln(R_2) + \dots + \ln(R_k) > 0$$

Multiplying both sides by $-1$ reverses the inequality:

$$(-\ln R_1) + (-\ln R_2) + \dots + (-\ln R_k) < 0$$

By assigning edge weights as $w(u, v) = -\ln(\text{rate}(u, v))$, any sequence of trades yielding a profit multiplier $> 1$ corresponds directly to a **negative-weight cycle** in the directed graph.

---

## Core Algorithm: Bellman-Ford Negative Cycle Detection

The engine runs a modified Bellman-Ford algorithm to locate, isolate, and extract the profitable trading loop.

### 1. Edge Relaxation Phase
The algorithm initializes distance vectors from a virtual super-source to $0.0$, allowing it to reach all disconnected components of the graph. Alongside this, a predecessor array $parent$ is initialized to $-1$.

For up to $V$ rounds, every directed edge $(u, v)$ with weight $w$ is relaxed, utilizing a floating-point epsilon ($1e^{-12}$) to prevent precision-based phantom loops:

$$\text{if } dist[u] + w < dist[v] - \epsilon \implies dist[v] = dist[u] + w, \quad parent[v] = u$$

The loop incorporates an early exit if the graph converges before $V$ iterations.

### 2. Cycle Isolation: The $V$-Step Predecessor Walkback
If a cycle is detected, tracing backward from the tripped node does not guarantee immediate entry into the closed loop due to potential downstream tails. To safely recover vertices strictly within the cycle:
1. Trace backward through the updated predecessor array exactly $V$ times:
   $$curr \leftarrow parent[curr] \quad (\times V)$$
2. Trace from this verified node using a `do-while` loop until it loops back onto itself, capturing the exact cycle.
3. Reverse the trace to produce the forward chronological sequence of trades.

---

## System Architecture

1. **Python Orchestrator (`orchestrator.py`):** 
   - Connects to the MEXC REST API to pull a snapshot of last-traded prices.
   - Injects a fixed 1 bps (0.01%) synthetic fee hurdle into reverse edges to filter unprofitable cycles.
   - Streams formatted `Source,Destination,Rate` CSV records directly to `stdout`.
2. **C++ Core Engine (`check_arbitrage.cpp`):**
   - Ingests streaming data via `stdin` or reads directly from static CSV files (backtesting).
   - Maps string tickers to contiguous integer IDs dynamically.
   - Executes the Bellman-Ford relaxation and extraction routine.
   - Reverses logarithmic edge weights via $\exp(-\sum w_i)$ to output the exact theoretical multiplier.

---

## Performance Benchmarks

Execution times measure algorithmic latency (`std::chrono::high_resolution_clock`) excluding network I/O and depth/execution risk. Results will vary heavily by hardware:

* **Live Market Snapshot (~22 Edges):** Executes in sub-millisecond timeframes.
* **Synthetic Stress Test (500 Nodes):** Executes in approximately **0.3 seconds** on standard modern CPUs.

---

## Compilation & Usage

Compile using the `-O3` flag for aggressive vectorization and loop optimizations:

```bash
g++ -O3 check_arbitrage.cpp -o check_arbitrage
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
