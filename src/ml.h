#pragma once
#include <vector>
#include <string>
#include "dataframeplus.h"

class Matrix {
private:
    std::vector<double> _data; 
    size_t _rows;
    size_t _cols;

    std::vector<double> _means;
    std::vector<double> _std_devs;
    bool _is_scaled = false;

public:
    Matrix(size_t rows, size_t cols) : _rows(rows), _cols(cols) {
        _data.resize(rows * cols, 0.0);
    }
    
    inline double operator()(size_t row, size_t col) const {
        return _data[row * _cols + col];
    }

    inline double& operator()(size_t row, size_t col) {
        return _data[row * _cols + col];
    }

    void modify(size_t row, size_t col, double value) {
        _data[row * _cols + col] = value;
    }

    void zScaleFeatures() {
        _means.assign(_cols, 0.0);
        _std_devs.assign(_cols, 0.0);

        // Compute means for each feature column
        for (size_t col = 0; col < _cols; ++col) {
            double sum = 0.0;
            for (size_t row = 0; row < _rows; ++row) {
                sum += (*this)(row, col);
            }
            _means[col] = sum / _rows;
        }

        // Compute standard deviations
        for (size_t col = 0; col < _cols; ++col) {
            double variance_sum = 0.0;
            for (size_t row = 0; row < _rows; ++row) {
                double diff = (*this)(row, col) - _means[col];
                variance_sum += diff * diff;
            }
            _std_devs[col] = std::sqrt(variance_sum / _rows);
            if (_std_devs[col] == 0.0) _std_devs[col] = 1.0; // Avoid division by zero
        }

        // Apply the transformation
        for (size_t row = 0; row < _rows; ++row) {
            for (size_t col = 0; col < _cols; ++col) {
                (*this)(row, col) = ((*this)(row, col) - _means[col]) / _std_devs[col];
            }
        }
        _is_scaled = true;
    }
    
    size_t rows() const { return _rows; }
    size_t cols() const { return _cols; }
};


class SOM {
private:
    Matrix _data;
    double _alpha;
    double _sigma;
};