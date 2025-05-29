#include "splashkit.h"
#include "dynamic_array.hpp"
#include <iostream>
#include <fstream>
#include <sstream>
#include <cmath>
#include <iomanip>
#include <string>
#include <algorithm>

using namespace std;

// Enums and structures
enum PredictionModel { LINEAR_REGRESSION, MOVING_AVERAGE, EXPONENTIAL_SMOOTHING };

struct StockData {
    string date;
    double open, high, low, close, volume;
    double sma5, prediction;
    
    StockData() : open(0), high(0), low(0), close(0), volume(0), sma5(0), prediction(0) {}
};

struct PredictionStats {
    double slope, intercept, r_squared, next_prediction, confidence;
    
    PredictionStats() : slope(0), intercept(0), r_squared(0), next_prediction(0), confidence(0) {}
};

struct StockPredictor {
    dynamic_array<StockData> data;
    PredictionModel model;
    PredictionStats stats;
    string company_name;
    string filename;
    
    StockPredictor() : data(50, StockData()), model(LINEAR_REGRESSION), stats(), company_name("Unknown"), filename("") {}
};

// Constants
const int WINDOW_WIDTH = 1200;
const int WINDOW_HEIGHT = 700;
const int MARGIN = 60;
const int CHART_WIDTH = WINDOW_WIDTH - 2 * MARGIN - 200;
const int CHART_HEIGHT = 450;
const color BG_COLOR = rgb_color(245, 245, 250);
const color GRID_COLOR = rgb_color(220, 220, 225);
const color UP_COLOR = rgb_color(34, 197, 94);
const color DOWN_COLOR = rgb_color(239, 68, 68);

// Utility functions
string extract_company_name(const string& filename) {
    // Extract company ticker from filename like "STOCK_US_XNAS_GOOG.csv"
    size_t last_underscore = filename.find_last_of('_');
    size_t dot_pos = filename.find_last_of('.');
    
    if (last_underscore != string::npos && dot_pos != string::npos && dot_pos > last_underscore) {
        return filename.substr(last_underscore + 1, dot_pos - last_underscore - 1);
    }
    
    // Fallback: use filename without extension
    size_t last_slash = filename.find_last_of("/\\");
    string base_name = (last_slash != string::npos) ? filename.substr(last_slash + 1) : filename;
    size_t ext_pos = base_name.find_last_of('.');
    return (ext_pos != string::npos) ? base_name.substr(0, ext_pos) : base_name;
}

void reverse_data_order(StockPredictor& predictor) {
    // Reverse the data since CSV is from latest to oldest, but we need oldest to latest for predictions
    unsigned int size = predictor.data.size;
    for (unsigned int i = 0; i < size / 2; i++) {
        StockData temp = predictor.data.get(i);
        predictor.data.set(i, predictor.data.get(size - 1 - i));
        predictor.data.set(size - 1 - i, temp);
    }
}

string clean_volume_string(const string& str) {
    string clean;
    for (size_t i = 0; i < str.length(); i++) {
        char c = str[i];
        if (isdigit(c) || c == '.') {
            clean += c;
        }
    }
    return clean.empty() ? "0" : clean;
}

string remove_quotes(const string& str) {
    string result = str;
    if (!result.empty() && result.front() == '"') {
        result.erase(0, 1);
    }
    if (!result.empty() && result.back() == '"') {
        result.pop_back();
    }
    return result;
}

double safe_stod(const string& str, double default_val = 0.0) {
    try {
        string clean_str = remove_quotes(str);
        return stod(clean_str);
    } catch (...) {
        return default_val;
    }
}

// Improved data loading with better error handling
bool load_stock_data(StockPredictor& predictor, const string& filename) {
    predictor.filename = filename;
    predictor.company_name = extract_company_name(filename);
    
    std::ifstream file(filename);
    if (!file.is_open()) {
        write_line("Error: Cannot open " + filename);
        return false;
    }
    
    string line;
    getline(file, line); // Skip header
    
    int valid_rows = 0;
    int skipped_rows = 0;
    
    while (getline(file, line)) {
        if (line.empty()) continue;
        
        StockData stock;
        std::stringstream ss(line);
        string temp;
        
        try {
            getline(ss, stock.date, ',');
            getline(ss, temp, ','); stock.open = safe_stod(temp);
            getline(ss, temp, ','); stock.high = safe_stod(temp);
            getline(ss, temp, ','); stock.low = safe_stod(temp);
            getline(ss, temp, ','); stock.close = safe_stod(temp);
            getline(ss, temp); stock.volume = safe_stod(clean_volume_string(temp));
            
            if (stock.open > 0 && stock.close > 0) {
                predictor.data.add(stock);
                valid_rows++;
            } else {
                skipped_rows++;
                if (valid_rows == 0 && skipped_rows <= 3) {
                    write_line("Debug - Skipped row: date=" + stock.date + 
                              " open=" + std::to_string(stock.open) + 
                              " close=" + std::to_string(stock.close));
                }
            }
        } catch (...) {
            skipped_rows++;
        }
    }
    
    file.close();
    
    // Reverse data order since CSV is from latest to oldest, but we need oldest to latest
    if (predictor.data.size > 0) {
        reverse_data_order(predictor);
        write_line("Loaded " + std::to_string(valid_rows) + " rows for " + predictor.company_name + 
                  ", skipped " + std::to_string(skipped_rows) + " (data reversed to chronological order)");
    }
    
    return predictor.data.size > 0;
}

// Simplified prediction calculations
void calculate_predictions(StockPredictor& predictor) {
    if (predictor.data.size < 2) return;
    
    switch (predictor.model) {
        case LINEAR_REGRESSION: {
            double x_mean = (predictor.data.size - 1) / 2.0;
            double y_mean = 0;
            for (unsigned int i = 0; i < predictor.data.size; i++) {
                y_mean += predictor.data.get(i).close;
            }
            y_mean /= predictor.data.size;
            
            double numerator = 0, denominator = 0;
            for (unsigned int i = 0; i < predictor.data.size; i++) {
                double x_diff = i - x_mean;
                double y_diff = predictor.data.get(i).close - y_mean;
                numerator += x_diff * y_diff;
                denominator += x_diff * x_diff;
            }
            
            predictor.stats.slope = (denominator != 0) ? numerator / denominator : 0;
            predictor.stats.intercept = y_mean - predictor.stats.slope * x_mean;
            predictor.stats.next_prediction = predictor.stats.slope * predictor.data.size + predictor.stats.intercept;
            predictor.stats.confidence = 0.8;
            break;
        }
        
        case MOVING_AVERAGE: {
            if (predictor.data.size >= 5) {
                double sum = 0;
                for (int i = 0; i < 5; i++) {
                    sum += predictor.data.get(predictor.data.size - 1 - i).close;
                }
                predictor.stats.next_prediction = sum / 5.0;
                predictor.stats.confidence = 0.7;
            }
            break;
        }
        
        case EXPONENTIAL_SMOOTHING: {
            double alpha = 0.3;
            double ema = predictor.data.get(0).close;
            for (unsigned int i = 1; i < predictor.data.size; i++) {
                ema = alpha * predictor.data.get(i).close + (1 - alpha) * ema;
            }
            predictor.stats.next_prediction = ema;
            predictor.stats.confidence = 0.75;
            break;
        }
    }
}

// Simplified drawing functions
void draw_trend_line(const StockPredictor& predictor, double min_price, double max_price) {
    if (predictor.data.size < 2 || predictor.model != LINEAR_REGRESSION) return;
    
    double y_scale = CHART_HEIGHT / (max_price - min_price);
    double bar_width = CHART_WIDTH / static_cast<double>(predictor.data.size);
    
    // Draw trend line using linear regression
    double x_start = MARGIN + bar_width / 2;
    double x_end = MARGIN + (predictor.data.size - 1) * bar_width + bar_width / 2;
    
    double y_start = predictor.stats.intercept;
    double y_end = predictor.stats.slope * (predictor.data.size - 1) + predictor.stats.intercept;
    
    // Convert to screen coordinates
    double screen_y_start = 80 + CHART_HEIGHT - (y_start - min_price) * y_scale;
    double screen_y_end = 80 + CHART_HEIGHT - (y_end - min_price) * y_scale;
    
    // Draw the trend line
    draw_line(COLOR_RED, x_start, screen_y_start, x_end, screen_y_end);
    
    // Draw trend line label
    draw_text("Trend Line", COLOR_RED, "Arial", 10, x_end - 60, screen_y_end - 15);
}

void draw_chart(const StockPredictor& predictor) {
    if (predictor.data.size == 0) return;
    
    // Find price range
    double min_price = 999999, max_price = 0;
    for (unsigned int i = 0; i < predictor.data.size; i++) {
        min_price = std::min(min_price, predictor.data.get(i).low);
        max_price = std::max(max_price, predictor.data.get(i).high);
    }
    
    double y_scale = CHART_HEIGHT / (max_price - min_price);
    double bar_width = CHART_WIDTH / static_cast<double>(predictor.data.size);
    
    // Draw candlesticks
    for (unsigned int i = 0; i < predictor.data.size; i++) {
        const StockData& data = predictor.data.get(i);
        double x = MARGIN + i * bar_width + bar_width / 2;
        
        color candle_color = (data.close >= data.open) ? UP_COLOR : DOWN_COLOR;
        double high_y = 80 + CHART_HEIGHT - (data.high - min_price) * y_scale;
        double low_y = 80 + CHART_HEIGHT - (data.low - min_price) * y_scale;
        double open_y = 80 + CHART_HEIGHT - (data.open - min_price) * y_scale;
        double close_y = 80 + CHART_HEIGHT - (data.close - min_price) * y_scale;
        
        draw_line(candle_color, x, high_y, x, low_y);
        fill_rectangle(candle_color, x - bar_width/3, std::min(open_y, close_y), 
                      2*bar_width/3, abs(close_y - open_y));
    }
    
    // Draw trend line
    draw_trend_line(predictor, min_price, max_price);
}

void draw_background(const StockPredictor& predictor) {
    clear_screen(BG_COLOR);
    
    // Title
    draw_text("Stock Price Predictor - Machine Learning", COLOR_BLACK, 
              "Arial", 24, MARGIN, 20);
    
    // Subtitle with dynamic company name
    string subtitle = predictor.company_name + " Historical Data Analysis";
    draw_text(subtitle, COLOR_GRAY, "Arial", 14, MARGIN, 48);
}

void draw_grid_and_axes(const StockPredictor& predictor, double min_price, double max_price) {
    // Draw chart background
    fill_rectangle(COLOR_WHITE, MARGIN, 80, CHART_WIDTH, CHART_HEIGHT);
    
    // Grid lines
    for (int i = 0; i <= 5; i++) {
        double y = 80 + i * (CHART_HEIGHT / 5.0);
        draw_line(GRID_COLOR, MARGIN, y, MARGIN + CHART_WIDTH, y);
        
        // Price labels
        double price = max_price - (i * (max_price - min_price) / 5.0);
        draw_text("$" + std::to_string(static_cast<int>(price)), COLOR_GRAY, 
                  "Arial", 11, MARGIN - 45, y - 6);
    }
    
    // Date grid (vertical)
    int date_count = std::min(10, static_cast<int>(predictor.data.size));
    for (int i = 0; i <= date_count; i++) {
        double x = MARGIN + i * (CHART_WIDTH / static_cast<double>(date_count));
        draw_line(GRID_COLOR, x, 80, x, 80 + CHART_HEIGHT);
    }
    
    // Axes
    draw_line(COLOR_BLACK, MARGIN, 80, MARGIN, 80 + CHART_HEIGHT);
    draw_line(COLOR_BLACK, MARGIN, 80 + CHART_HEIGHT, 
              MARGIN + CHART_WIDTH, 80 + CHART_HEIGHT);
}

void draw_controls(const StockPredictor& predictor) {
    int panel_x = MARGIN + CHART_WIDTH + 30;
    int panel_y = 80;
    
    // Control panel background
    fill_rectangle(COLOR_WHITE, panel_x, panel_y, 160, 400);
    draw_rectangle(COLOR_LIGHT_GRAY, panel_x, panel_y, 160, 400);
    
    // Model selection
    draw_text("Prediction Model", COLOR_BLACK, "Arial", 14, panel_x + 10, panel_y + 10);
    
    string models[] = {"Linear Regression", "Moving Average", "Exp. Smoothing"};
    color colors[] = {COLOR_BLUE, COLOR_GREEN, COLOR_PURPLE};
    
    for (int i = 0; i < 3; i++) {
        color btn_color = (predictor.model == i) ? colors[i] : COLOR_LIGHT_GRAY;
        fill_rectangle(btn_color, panel_x + 10, panel_y + 40 + i * 40, 140, 30);
        
        color text_color = (predictor.model == i) ? COLOR_WHITE : COLOR_BLACK;
        draw_text(models[i], text_color, "Arial", 11, 
                  panel_x + 20, panel_y + 48 + i * 40);
    }
    
    // Prediction display
    draw_text("Next Prediction", COLOR_BLACK, "Arial", 14, 
              panel_x + 10, panel_y + 180);
    
    if (predictor.stats.next_prediction > 0) {
        // Predicted value
        draw_text("$" + std::to_string(static_cast<int>(predictor.stats.next_prediction)), 
                  COLOR_BLUE, "Arial", 20, panel_x + 10, panel_y + 210);
        
        // Change from last close
        if (predictor.data.size > 0) {
            double last_close = predictor.data.get(predictor.data.size - 1).close;
            double change = predictor.stats.next_prediction - last_close;
            double change_pct = (change / last_close) * 100;
            
            color change_color = (change >= 0) ? UP_COLOR : DOWN_COLOR;
            string sign = (change >= 0) ? "+" : "";
            
            draw_text(sign + std::to_string(static_cast<int>(change_pct)) + "%", 
                      change_color, "Arial", 16, panel_x + 10, panel_y + 240);
        }
        
        // Confidence
        draw_text("Confidence", COLOR_BLACK, "Arial", 12, panel_x + 10, panel_y + 280);
        
        // Confidence bar
        fill_rectangle(COLOR_LIGHT_GRAY, panel_x + 10, panel_y + 300, 140, 20);
        fill_rectangle(COLOR_BLUE, panel_x + 10, panel_y + 300, 
                      140 * predictor.stats.confidence, 20);
        
        draw_text(std::to_string(static_cast<int>(predictor.stats.confidence * 100)) + "%", 
                  COLOR_BLACK, "Arial", 11, panel_x + 60, panel_y + 303);
    }
    
    // Stats for linear regression
    if (predictor.model == LINEAR_REGRESSION) {
        draw_text("R² = " + std::to_string(predictor.stats.r_squared).substr(0, 5), 
                  COLOR_GRAY, "Arial", 11, panel_x + 10, panel_y + 340);
    }
}

void draw_info_panel(const StockPredictor& predictor) {
    if (predictor.data.size == 0) return;
    
    // Latest data info
    const StockData& latest = predictor.data.get(predictor.data.size - 1);
    
    int info_y = 80 + CHART_HEIGHT + 30;
    
    fill_rectangle(COLOR_WHITE, MARGIN, info_y, CHART_WIDTH, 80);
    draw_rectangle(COLOR_LIGHT_GRAY, MARGIN, info_y, CHART_WIDTH, 80);
    
    draw_text("Latest: " + latest.date, COLOR_BLACK, "Arial", 12, MARGIN + 10, info_y + 10);
    
    string info = "Open: $" + std::to_string(static_cast<int>(latest.open)) +
                  "  High: $" + std::to_string(static_cast<int>(latest.high)) +
                  "  Low: $" + std::to_string(static_cast<int>(latest.low)) +
                  "  Close: $" + std::to_string(static_cast<int>(latest.close));
    
    draw_text(info, COLOR_GRAY, "Arial", 12, MARGIN + 10, info_y + 35);
    
    draw_text("Volume: " + std::to_string(static_cast<long>(latest.volume)) + " shares", 
              COLOR_GRAY, "Arial", 12, MARGIN + 10, info_y + 55);
}

// Main program
int main(int argc, char* argv[]) {
    // Default filename
    string filename = "stock_data.csv";
    
    // Check for command line arguments
    if (argc > 1) {
        // Concatenate all arguments after program name to handle spaces in filename
        filename = argv[1];
        for (int i = 2; i < argc; i++) {
            filename += " " + string(argv[i]);
        }
    }
    
    write_line("Starting Stock Price Predictor...");
    write_line("Loading data from: " + filename);
    
    open_window("Stock Predictor", WINDOW_WIDTH, WINDOW_HEIGHT);
    
    StockPredictor predictor;
    if (!load_stock_data(predictor, filename)) {
        write_line("Error: Could not load stock data from " + filename);
        write_line("Usage: " + string(argv[0]) + " [csv_filename]");
        write_line("Expected CSV format: Date,Open,High,Low,Close,Volume");
        delay(3000);
        return 1;
    }
    
    calculate_predictions(predictor);
    
    // Find price range for grid drawing
    double min_price = 999999, max_price = 0;
    for (unsigned int i = 0; i < predictor.data.size; i++) {
        min_price = std::min(min_price, predictor.data.get(i).low);
        max_price = std::max(max_price, predictor.data.get(i).high);
    }
    
    while (!quit_requested()) {
        process_events();
        
        if (mouse_clicked(LEFT_BUTTON)) {
            double mx = mouse_x();
            double my = mouse_y();
            
            int panel_x = MARGIN + CHART_WIDTH + 30;
            if (mx >= panel_x + 10 && mx <= panel_x + 150) {
                if (my >= 120 && my <= 150) predictor.model = LINEAR_REGRESSION;
                else if (my >= 160 && my <= 190) predictor.model = MOVING_AVERAGE;
                else if (my >= 200 && my <= 230) predictor.model = EXPONENTIAL_SMOOTHING;
                calculate_predictions(predictor);
            }
        }
        
        draw_background(predictor);
        draw_grid_and_axes(predictor, min_price, max_price);
        draw_chart(predictor);
        draw_controls(predictor);
        draw_info_panel(predictor);
        refresh_screen(60);
    }
    
    close_all_windows();
    return 0;
}