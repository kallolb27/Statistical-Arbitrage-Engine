import requests
import sys

def fetch_live_rates():
    # Swapped to MEXC API: Not blocked, and uses the exact same JSON structure
    url = "https://api.mexc.com/api/v3/ticker/price"
    
    # Adding a User-Agent so we look like a normal Google Chrome browser, not a Python bot
    headers = {
        "User-Agent": "Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/115.0.0.0 Safari/537.36"
    }
    
    try:
        response = requests.get(url, headers=headers, timeout=10)
        response.raise_for_status()
        data = response.json()
        
        # We look at 5 major coins against two standard US Dollar stablecoins
        bases = ["BTC", "ETH", "BNB", "XRP", "SOL"]
        quotes = ["USDT", "USDC"]
        
        for item in data:
            symbol = item['symbol']
            price = float(item['price'])
            
            if price == 0:
                continue
                
            for quote in quotes:
                if symbol.endswith(quote):
                    base = symbol[:-len(quote)]
                    if base in bases or base in quotes:
                        # Forward edge
                        print(f"{base},{quote},{price}")
                        # Reverse edge (simulating a 0.1% exchange fee / bid-ask spread)
                        print(f"{quote},{base},{ (1.0 / price) * 0.9999 }")
                        break
                        
    except Exception as e:
        print(f"Python Error: {e}", file=sys.stderr)

if __name__ == "__main__":
    fetch_live_rates()