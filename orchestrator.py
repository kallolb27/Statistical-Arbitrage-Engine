import requests
import sys

def fetch_live_rates():
    # Fetching a single snapshot of the last traded prices from MEXC
    url = "https://api.mexc.com/api/v3/ticker/price"
    
    headers = {
        "User-Agent": "Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/115.0.0.0 Safari/537.36"
    }
    
    try:
        response = requests.get(url, headers=headers, timeout=10)
        response.raise_for_status()
        data = response.json()
        
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
                        print(f"{base},{quote},{price}")
                        # Reverse edge simulating a 1 bps (0.01%) fee hurdle 
                        # Note: This is an idealized execution assumption.
                        print(f"{quote},{base},{ (1.0 / price) * 0.9999 }")
                        break
                        
    except Exception as e:
        print(f"Python Error: {e}", file=sys.stderr)

if __name__ == "__main__":
    fetch_live_rates()