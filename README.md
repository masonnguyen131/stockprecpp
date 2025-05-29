# Stock Price Predictor

A C++ application that analyzes historical stock data and predicts future prices using machine learning models.

## Features

- 📈 **Multiple ML Models**: Linear Regression, Moving Average, Exponential Smoothing
- 📊 **Interactive Charts**: Candlestick visualization with trend lines
- 🎯 **Real-time Predictions**: Click to switch models instantly
- 🏢 **Universal Support**: Works with any stock CSV file
- 📋 **Confidence Metrics**: Visual confidence indicators

## Quick Start

### Prerequisites
- [SplashKit](https://splashkit.io) graphics library
- C++11 or later
- `dynamic_array.hpp` file

### Build & Run
```bash
# Compile
clang++ hd -l SplashKit -o hd

# Run with your CSV file
./hd AAPL.csv

```

## CSV Format

Your CSV file should look like this:
```csv
Date,Open,High,Low,Close,Volume
05/23/2024,"178.78","179.91","174.54","175.06","14,928,360"
05/22/2024,"178.40","178.85","176.78","178.00","16,189,400"
```

**Note**: Data can be in any chronological order - the program auto-corrects it.

## How to Use

1. **Run the program** with your CSV file
2. **Click model buttons** (right panel) to switch between prediction algorithms
3. **View predictions** in the right panel with confidence levels
4. **Analyze trends** using the red trend line (Linear Regression mode)

## Prediction Models

| Model | Best For | Confidence | Visual |
|-------|----------|------------|--------|
| **Linear Regression** | Clear trends | 80% | Red trend line |
| **Moving Average** | Stable stocks | 70% | - |
| **Exponential Smoothing** | Recent-focused | 75% | - |

## Interface

- **Left**: Candlestick chart (🟢 = price up, 🔴 = price down)
- **Right**: Model controls and predictions
- **Bottom**: Latest trading data

## Troubleshooting

| Issue | Solution |
|-------|----------|
| "Cannot open file" | Check filename and file exists |
| "Loaded 0 rows" | Verify CSV format matches example |
| Won't compile | Install SplashKit, check `dynamic_array.hpp` |

## File Naming

For best results, include ticker symbol in filename:
- `AAPL_data.csv` → "AAPL_data"

## License

MIT License - feel free to use and modify!
