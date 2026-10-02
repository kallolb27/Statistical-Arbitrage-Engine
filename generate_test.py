import random
import csv

def generate_synthetic_market(num_nodes=50, filename="sample_market_50.csv"):
    print(f"Generating synthetic market with {num_nodes} assets...")
    
    # Assign a random underlying "true value" to each asset (relative to Asset 0)
    true_values = {f"COIN_{i}": random.uniform(0.1, 1000.0) for i in range(num_nodes)}
    true_values["COIN_0"] = 1.0 # Base currency
    
    tickers = list(true_values.keys())
    edge_count = 0
    
    with open(filename, 'w', newline='') as f:
        writer = csv.writer(f)
        
        for i in range(num_nodes):
            for j in range(num_nodes):
                if i == j:
                    continue
                    
                src = tickers[i]
                dest = tickers[j]
                
                # The mathematically perfect exchange rate
                exact_rate = true_values[dest] / true_values[src]
                
                # Add market friction / noise (+/- 0.05%) to create arbitrage loops
                noise = random.uniform(0.9995, 1.0005)
                market_rate = exact_rate * noise
                
                writer.writerow([src, dest, market_rate])
                edge_count += 1

    print(f"Done! Wrote {edge_count} edges to {filename}")

if __name__ == "__main__":
    generate_synthetic_market(50, "sample_market_50.csv")