// dynamic_array.hpp - Reusable template container
#ifndef DYNAMIC_ARRAY_HPP
#define DYNAMIC_ARRAY_HPP

#include <algorithm>

template <typename T>
struct dynamic_array {
    unsigned int size;
    unsigned int capacity;
    T *data;
    T default_value;
    
    // Constructor
    dynamic_array(unsigned int initial_capacity, T default_val) {
        size = 0;
        capacity = initial_capacity;
        default_value = default_val;
        
        if (capacity > 0) {
            data = new T[capacity];
        } else {
            data = nullptr;
        }
    }
    
    // Destructor
    ~dynamic_array() {
        if (data) {
            delete[] data;
            data = nullptr;
        }
    }
    
    // Add an element
    bool add(T value) {
        if (size >= capacity) {
            unsigned int new_capacity = (capacity > 0) ? capacity * 2 : 1;
            if (!resize(new_capacity)) {
                return false;
            }
        }
        
        data[size] = value;
        size++;
        return true;
    }
    
    // Remove an element at index
    bool remove(unsigned int index) {
        if (index >= size) {
            return false;
        }
        
        for (unsigned int i = index; i < size - 1; i++) {
            data[i] = data[i + 1];
        }
        size--;
        return true;
    }
    
    // Get element at index
    T get(unsigned int index) const {
        if (index >= size) {
            return default_value;
        }
        return data[index];
    }
    
    // Set element at index
    bool set(unsigned int index, T value) {
        if (index >= size) {
            return false;
        }
        data[index] = value;
        return true;
    }
    
    // Clear all elements
    void clear() {
        size = 0;
    }
    
    // Get last element
    T last() const {
        if (size > 0) {
            return data[size - 1];
        }
        return default_value;
    }
    
    // Check if empty
    bool empty() const {
        return size == 0;
    }
    
    // Find min/max (useful for price ranges)
    T min() const {
        if (size == 0) return default_value;
        T min_val = data[0];
        for (unsigned int i = 1; i < size; i++) {
            if (data[i] < min_val) min_val = data[i];
        }
        return min_val;
    }
    
    T max() const {
        if (size == 0) return default_value;
        T max_val = data[0];
        for (unsigned int i = 1; i < size; i++) {
            if (data[i] > max_val) max_val = data[i];
        }
        return max_val;
    }
    
private:
    bool resize(unsigned int new_capacity) {
        T *new_data = new T[new_capacity];
        if (!new_data) {
            return false;
        }
        
        unsigned int items_to_copy = (size < new_capacity) ? size : new_capacity;
        for (unsigned int i = 0; i < items_to_copy; i++) {
            new_data[i] = data[i];
        }
        
        if (data) {
            delete[] data;
        }
        
        data = new_data;
        capacity = new_capacity;
        
        if (size > capacity) {
            size = capacity;
        }
        
        return true;
    }
};

#endif