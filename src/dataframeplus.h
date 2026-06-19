#pragma once
#include <vector>
#include <string>
#include <algorithm>
#include <cctype>
#include <cstdint>
#include <memory>

inline std::string to_lowercase(std::string s) {
    std::transform(s.begin(), s.end(), s.begin(),
        [](unsigned char c) { return std::tolower(c); });
    return s;
}

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
    std::unique_ptr<DataFrame> concatVertically(const DataFrame& df);
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
    std::string toJson() const; // Helper for Wasm/UI
    std::string toCsvString() const; // Helper for Wasm saving
};


/*
TO DO:
printElement - should get output stream , and padding parameter.
MaxLength should be an attribute, not a function you alwyas use to compute.
There should be a function updateMaxLength getMaxLength thats t=gets the attribute maxlength' value.
Rename the functions to be lowercase.
make the parsing functions static.
*/