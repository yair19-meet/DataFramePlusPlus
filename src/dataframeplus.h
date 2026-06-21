#pragma once
#include <vector>
#include <string>
#include <algorithm>
#include <cctype>
#include <cstdint>
#include <memory>
#include <sstream>
#include <stdexcept>
#include <charconv>
#include <string>
#include <system_error>

/**
 * enum for specifying supported data types
 */
enum class DataType {
    kInt64,
    kFloat64,
    kString,
    kDate
};


/**
 * TODO
 */
class ColumnBase
{
public:
    virtual ~ColumnBase() = default;
    int getMaxElementLength() { return this->_maxElementLength; }
    std::string getHeader() { return _header; }
    void rename(std::string name) { _header = name; updateMaxElementLength(); }
    virtual double getAsDouble(size_t index) const = 0;
    virtual std::string getAsString(size_t index) const = 0;
    virtual int64_t getAsInt(size_t index) const = 0;
    virtual std::vector<std::string> getColAsStrings() const = 0;
    virtual double sum() = 0;
    virtual double mean() = 0;
    virtual double max() = 0;
    virtual double min() = 0;
    virtual double mode() = 0;
    virtual double stdDev() = 0;
    virtual void print() = 0;
    virtual void updateMaxElementLength() = 0;
    virtual void printElement(int index) = 0;
    virtual int len() = 0;
    virtual std::unique_ptr<ColumnBase> clone() = 0;
    virtual std::unique_ptr<ColumnBase> cutClone(size_t startIndex, size_t endIndex) = 0;
    virtual std::vector<size_t> getOrderedPermutation(bool ascending) = 0;
    virtual std::unique_ptr<ColumnBase> cloneOrdered(const std::vector<size_t>& indexOrder) = 0;
    virtual int compare(size_t i, size_t j) const = 0;
    DataType getType() { return _type; }
    
protected:
    ColumnBase(DataType type, std::string header) : _type(type), _header(header) {}
    std::string _header;
    int _maxElementLength;
    DataType _type;
};

/**
 * TODO
 */
template <typename T, DataType DT>
class Column : public ColumnBase
{
public:
    Column(const std::vector<T>& data, std::string name);
    void print() override;
    double getAsDouble(size_t index) const override;
    std::string getAsString(size_t index) const override;
    int64_t getAsInt(size_t index) const override;
    std::vector<std::string> getColAsStrings() const override;
    const std::vector<T>& getData() { return this->_data; }
    void setData(std::vector<T> v) { this->_data = v; }
    void updateMaxElementLength() override;
    int len() override { return this->_data.size(); }
    void printElement(int index) override;
    double sum() override;
    double stdDev() override;
    double mean() override;
    double mode() override;
    double max() override;
    double min() override;
    std::unique_ptr<ColumnBase> clone() override;
    std::unique_ptr<ColumnBase> cutClone(size_t startIndex, size_t endIndex) override;
    std::vector<size_t> getOrderedPermutation(bool ascending) override;
    std::unique_ptr<ColumnBase> cloneOrdered(const std::vector<size_t>& indexOrder) override;
    int compare(size_t i, size_t j) const override;
private:
    std::vector<T> _data;
};

using DateColumn = Column<int64_t, DataType::kDate>;
using IntColumn = Column<int64_t, DataType::kInt64>;
using StringColumn = Column<std::string, DataType::kString>;
using FloatColumn = Column<double, DataType::kFloat64>;

class DataFrame 
{
private:
    std::vector<std::unique_ptr<ColumnBase>> _dataFrame;
    std::vector<std::unique_ptr<ColumnBase>> _index;
    std::pair<int, int> _dimensions;
    void buildDataFrame(std::string content, char seperator);
    std::unique_ptr<DataFrame> groupByOnly(std::vector<std::string> columnsToGroup, 
                                       std::vector<std::string> columnsToAggregate, std::vector<std::string> operations);
    std::unique_ptr<DataFrame> valueCountsOnly(std::string columnToGroup);
    std::vector<size_t> getOrderedPerm(std::vector<std::string> colNames, std::vector<bool> ascendingLst);
    std::unique_ptr<DataFrame> pivot();
    void plotFrequencies(int colIndex);
public:
    void print();
    DataFrame() {_dimensions = std::make_pair(0, 0); }
    DataFrame(std::string filePath, char seperator=',');
    std::pair<int, int>& getDimensions() { return this->_dimensions; }
    std::unique_ptr<DataFrame> pivotTable(std::string rows, std::string column, std::string values, std::string operation);
    std::vector<std::unique_ptr<ColumnBase>>& getData() { return _dataFrame; }
    std::vector<std::unique_ptr<ColumnBase>>& getIndex() { return _index; }
    void setDimensions(std::pair<int, int> dim) { _dimensions = dim; }
    void printTypes();
    // subsetting columns
    std::unique_ptr<DataFrame> operator[](std::vector<std::string> columnNames);
    std::unique_ptr<DataFrame> operator[](std::vector<size_t> perm);
    // subsetting rows
    std::unique_ptr<DataFrame> operator()(int starting_row, int end_row);
    void AddColumn(std::unique_ptr<ColumnBase>&& col);
    void AddIndexCol(std::unique_ptr<ColumnBase>&& col) { if (col->len() == this->_dimensions.first) this->_index.push_back(std::move(col)); }
    std::vector<double> mean();
    std::unique_ptr<DataFrame> groupBy(std::vector<std::string> columnsToGroup, 
                                       std::vector<std::string> columnsToAggregate, std::vector<std::string> operations);
    std::unique_ptr<DataFrame> valueCounts(std::string columnToGroup);
    std::unique_ptr<DataFrame> sortValues(std::string col, bool ascending);
    std::unique_ptr<DataFrame> sortValues(std::vector<std::string> colNames, std::vector<bool> ascendingLst);
    std::unique_ptr<DataFrame> concatVertically(DataFrame& df);
    void plotValueCount(std::string columnToGroup);
    void barPlot(std::string columnToPlot);
    void resetIndex();
    std::unique_ptr<DataFrame> clone() const;
    void dropIndex();
    std::vector<size_t> operator==(const std::string& target);
    std::vector<size_t> operator!=(const std::string& target);
    std::vector<size_t> operator==(const double target);
    std::vector<size_t> operator!=(const double target);
    std::vector<size_t> operator>(const double target);
    std::vector<size_t> operator<(const double target);
    std::vector<size_t> operator>=(const double target);
    std::vector<size_t> operator<=(const double target);
    void dropColumn(std::string colName);
    bool hasColumn(std::string colName) const;
    ColumnBase* getColumn(std::string colName) const;
    std::unique_ptr<DataFrame> agg(std::vector<std::string> columns, std::vector<std::string> operations);
    void toCsv(const std::string& filePath);
    static std::string formatDate(int64_t days_since_epoch);
    static int64_t parseDate(const std::string& s);
    void renameColumns(std::vector<std::string> header);
    void transitionGroupedColumns(const std::vector<std::string>& names);
    void convertDataType(std::string col, DataType dtype);
};

inline std::string to_lowercase(std::string s) {
    std::transform(s.begin(), s.end(), s.begin(),
        [](unsigned char c) { return std::tolower(c); });
    return s;
}

inline std::vector<double> IntsToDoubles(const std::vector<int64_t>& in) {
    std::vector<double> out;
    out.reserve(in.size());
    for (auto v : in) out.push_back(static_cast<double>(v));
    return out;
}

inline std::vector<int64_t> DoublesToInts(const std::vector<double>& in) {
    std::vector<int64_t> out;
    out.reserve(in.size());
    for (auto v : in) out.push_back(static_cast<int64_t>(v));
    return out;
}

inline std::vector<std::string> IntsToStrings(const std::vector<int64_t>& in) {
    std::vector<std::string> out;
    out.reserve(in.size());
    for (auto v : in) out.push_back(std::to_string(v));
    return out;
}

inline std::vector<std::string> DoublesToStrings(const std::vector<double>& in) {
    std::vector<std::string> out;
    out.reserve(in.size());
    for (auto v : in) {
        std::ostringstream ss;
        ss << v;
        out.push_back(ss.str());
    }
    return out;
}

inline int64_t StringToInt(const std::string& s) {
    if (s.empty()) {
        throw std::invalid_argument("Cannot convert empty string to int64_t");
    }

    int64_t value = 0;
    // from_chars takes pointers to the beginning and end of the buffer
    auto [ptr, ec] = std::from_chars(s.data(), s.data() + s.size(), value);

    if (ec == std::errc::invalid_argument) {
        throw std::invalid_argument("Invalid integer string: '" + s + "'");
    } else if (ec == std::errc::result_out_of_range) {
        throw std::out_of_range("Integer out of range: '" + s + "'");
    } else if (ptr != s.data() + s.size()) {
        throw std::invalid_argument("Trailing characters found in: '" + s + "'");
    }

    return value;
}

inline std::vector<int64_t> StringsToInts(const std::vector<std::string>& in) {
    std::vector<int64_t> out;
    out.reserve(in.size());
    for (auto v : in) out.push_back(StringToInt(v));
    return out;
}

inline double StringToDouble(const std::string& s) {
    if (s.empty()) {
        throw std::invalid_argument("Cannot convert empty string to double");
    }

    // Trim trailing whitespace to ensure strict exact-match parsing.
    // (std::stod naturally skips leading whitespace, so we only need to worry about the end)
    size_t end_pos = s.find_last_not_of(" \t\n\r\f\v");
    if (end_pos == std::string::npos) {
        throw std::invalid_argument("String contains only whitespace");
    }
    
    // Create a view of the string without trailing spaces
    std::string trimmed = s.substr(0, end_pos + 1);

    size_t idx = 0;
    double value = 0.0;

    // Parse and catch specific exceptions so callers know exactly what failed
    try {
        value = std::stod(trimmed, &idx);
    } catch (const std::invalid_argument&) {
        throw std::invalid_argument("Invalid double string: '" + s + "'");
    } catch (const std::out_of_range&) {
        throw std::out_of_range("Double out of range (overflow/underflow): '" + s + "'");
    }

    // Strict check for trailing garbage characters (e.g., "3.14abc")
    if (idx != trimmed.size()) {
        throw std::invalid_argument("Trailing characters found: '" + s + "'");
    }

    return value;
}

inline std::vector<double> StringsToDoubles(const std::vector<std::string>& in) {
    std::vector<double> out;
    out.reserve(in.size());
    for (auto v : in) out.push_back(StringToDouble(v));
    return out;
}


inline std::vector<std::string> IntsToDateStrings(const std::vector<int64_t>& in) {
    std::vector<std::string> out;
    out.reserve(in.size());
    for (auto v : in) {
        out.push_back(DataFrame::formatDate(v));
    }
    return out;
}

inline std::vector<int64_t> StringsToDateInts(const std::vector<std::string>& in) {
    std::vector<int64_t> out;
    out.reserve(in.size());
    for (auto v : in) {
        out.push_back(DataFrame::parseDate(v));
    }
    return out;
}

/*
TO DO:
printElement - should get output stream , and padding parameter.
MaxLength should be an attribute, not a function you alwyas use to compute.
There should be a function updateMaxLength getMaxLength thats t=gets the attribute maxlength' value.
Rename the functions to be lowercase.
make the parsing functions static.
*/