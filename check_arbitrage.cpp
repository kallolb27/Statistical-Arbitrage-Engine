#include <iostream>
#include <vector>
#include <cmath>
#include <string>
#include <unordered_map>
#include <sstream>
#include <chrono>
#include <fstream>

using namespace std;

// Represents a single directional exchange rate
struct Edge {
    int src;
    int dest;
    double weight;
};

// Encapsulates the entire graph and Bellman-Ford logic
class ArbitrageEngine {
private:
    int V;
    vector<Edge> edges;
    unordered_map<string, int> ticker_to_id;
    unordered_map<int, string> id_to_ticker;

public:
    ArbitrageEngine() : V(0) {}

    // 1. Data Ingestion
    bool loadMarketData(istream& input) {
        string line, src_str, dest_str, rate_str;
        int node_count = 0;

        while (getline(input, line)) {
            if (line.empty()) continue;
            stringstream ss(line);
            getline(ss, src_str, ',');
            getline(ss, dest_str, ',');
            getline(ss, rate_str, ',');

            if (ticker_to_id.find(src_str) == ticker_to_id.end()) {
                ticker_to_id[src_str] = node_count;
                id_to_ticker[node_count] = src_str;
                node_count++;
            }
            if (ticker_to_id.find(dest_str) == ticker_to_id.end()) {
                ticker_to_id[dest_str] = node_count;
                id_to_ticker[node_count] = dest_str;
                node_count++;
            }

            double rate = stod(rate_str);
            edges.push_back({ticker_to_id[src_str], ticker_to_id[dest_str], -log(rate)});
        }
        
        V = node_count;
        return V > 0;
    }

    int getVertexCount() const { return V; }
    int getEdgeCount() const { return edges.size(); }

    // 2. Core Algorithm Execution
    bool findNegativeCycle(vector<int>& cycle) {
        if (V == 0) return false;

        const double INF = 1e9;
        vector<double> dist(V, INF);
        vector<int> parent(V, -1);
        
        dist[0] = 0.0;

        // V - 1 relaxations
        for (int i = 0; i < V - 1; i++) {
            for (const auto& edge : edges) {
                if (dist[edge.src] != INF && dist[edge.src] + edge.weight < dist[edge.dest]) {
                    dist[edge.dest] = dist[edge.src] + edge.weight;
                    parent[edge.dest] = edge.src;
                }
            }
        }

        // V-th iteration to trip the alarm
        int cycle_start = -1;
        for (const auto& edge : edges) {
            if (dist[edge.src] != INF && dist[edge.src] + edge.weight < dist[edge.dest]) {
                cycle_start = edge.dest;
                break;
            }
        }

        if (cycle_start == -1) return false;

        // Walk back V times to guarantee we are inside the negative cycle
        for (int i = 0; i < V; i++) {
            cycle_start = parent[cycle_start];
        }

        // Extract the cycle
        int curr = cycle_start;
        do {
            cycle.push_back(curr);
            curr = parent[curr];
        } while (curr != cycle_start);
        cycle.push_back(cycle_start);

        return true;
    }

    // 3. Profit Calculation and Output
    void printExecutionPathAndProfit(const vector<int>& cycle) {
        // Reverse to get chronological trade order
        vector<int> exec_order;
        for (int i = cycle.size() - 1; i >= 0; i--) {
            exec_order.push_back(cycle[i]);
        }

        cout << "Execution Path: ";
        for (size_t i = 0; i < exec_order.size(); i++) {
            cout << id_to_ticker[exec_order[i]];
            if (i + 1 < exec_order.size()) cout << " -> ";
        }
        cout << "\n";

        // Calculate compounded net yield across the loop
        double total_log_weight = 0.0;
        for (size_t i = 0; i + 1 < exec_order.size(); i++) {
            int u = exec_order[i];
            int v = exec_order[i + 1];
            for (const auto& edge : edges) {
                if (edge.src == u && edge.dest == v) {
                    total_log_weight += edge.weight;
                    break;
                }
            }
        }

        double multiplier = exp(-total_log_weight);
        double net_profit_pct = (multiplier - 1.0) * 100.0;

        cout << "Net Growth Multiplier: " << multiplier << "\n";
        cout << "Net Profit: " << net_profit_pct << "%" << "\n";
    }
};

#include <fstream> 

int main(int argc, char* argv[]) {
    ios_base::sync_with_stdio(false);
    cin.tie(NULL);

    ArbitrageEngine engine;
    
    // Default to reading from the live pipe (standard input)
    istream* input_stream = &cin;
    ifstream file_stream;

    // If a filename is passed as a command-line argument, read from the file instead
    if (argc > 1) {
        file_stream.open(argv[1]);
        if (!file_stream.is_open()) {
            cerr << "Error: Could not open file " << argv[1] << "\n";
            return 1;
        }
        input_stream = &file_stream;
        cout << "Reading market data from file: " << argv[1] << "\n";
    } else {
        cout << "Listening for live market data stream..." << "\n";
    }

    // Dereference the pointer to pass the correct stream to the engine
    if (!engine.loadMarketData(*input_stream)) {
        cout << "No market data received." << "\n";
        return 1;
    }

    cout << "Market initialized with " << engine.getVertexCount() 
         << " currencies and " << engine.getEdgeCount() << " directed edges." << "\n";

    vector<int> cycle;

    auto start_time = chrono::high_resolution_clock::now();
    bool has_arbitrage = engine.findNegativeCycle(cycle);
    auto end_time = chrono::high_resolution_clock::now();

    if (has_arbitrage) {
        cout << "Arbitrage cycle detected! Executing trades..." << "\n";
        auto duration = chrono::duration_cast<chrono::microseconds>(end_time - start_time).count();
        cout << "Engine Latency: " << duration << " microseconds." << "\n";
        
        engine.printExecutionPathAndProfit(cycle);
    } else {
        cout << "No arbitrage opportunities detected." << "\n";
    }

    return 0;
}