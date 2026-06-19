#include <iostream>
#include <vector>
#include <string>
#include <fstream>
#include <sstream>
#include <charconv>
#include <algorithm>
#include <chrono>
#include <iomanip>
#include <stdexcept>
#include <cstdlib>
#include <cmath>
#include "dataframeplus.h"

int64_t DataFrame::parseDate(const std::string& s)
{
    int y = 0, m = 0, d = 0;
    
    // 1. Try YYYY-MM-DD or YYYY/MM/DD
    if (s.find('-') != std::string::npos || s.find('/') != std::string::npos) {
        char sep;
        std::istringstream iss(s);
        if (s.find('-') != std::string::npos) {
            if (!(iss >> y >> sep >> m >> sep >> d)) goto fallback;
        } else {
            // Handle M/D/YYYY or YYYY/MM/DD
            std::vector<int> parts;
            std::string part;
            std::istringstream pss(s);
            while (std::getline(pss, part, '/')) {
                try { parts.push_back(std::stoi(part)); } catch(...) {}
            }
            if (parts.size() >= 3) {
                if (parts[0] > 1000) { y = parts[0]; m = parts[1]; d = parts[2]; }
                else { m = parts[0]; d = parts[1]; y = parts[2]; }
            } else goto fallback;
        }
    } 
    // 2. Try YYYYMMDD (8 digits)
    else if (s.length() >= 8 && std::all_of(s.begin(), s.begin() + 8, ::isdigit)) {
        y = std::stoi(s.substr(0, 4));
        m = std::stoi(s.substr(4, 2));
        d = std::stoi(s.substr(6, 2));
    }
    else {
        fallback:
        // Try to just extract 3 numbers if possible
        std::istringstream iss(s);
        if (!(iss >> y >> m >> d)) {
             // Final fallback: Today's date
             auto now = std::chrono::system_clock::now();
             return std::chrono::duration_cast<std::chrono::seconds>(now.time_since_epoch()).count() / (24*3600);
        }
    }

    std::tm t = {};
    t.tm_year = y - 1900;
    t.tm_mon = m - 1;
    t.tm_mday = d;
    t.tm_isdst = -1;

#ifdef _WIN32
    time_t tt = _mkgmtime(&t);
#else
    time_t tt = timegm(&t);
#endif

    if (tt == -1) return 0; // Invalid date -> epoch
    return tt / (24 * 3600);
}


std::string DataFrame::formatDate(int64_t days_since_epoch)
{
    time_t tt = (time_t)days_since_epoch * 24 * 3600;
    std::tm* t = gmtime(&tt);
    if (!t) return "1970-01-01";

    std::ostringstream os;
    os << (t->tm_year + 1900) << "-"
       << std::setw(2) << std::setfill('0') << (t->tm_mon + 1) << "-"
       << std::setw(2) << std::setfill('0') << t->tm_mday;
    return os.str();
}



std::vector<int64_t> parseDateColumn(const std::vector<std::string>& dates)
{
    std::vector<int64_t> result;
    result.reserve(dates.size());

    for (size_t i = 0; i < dates.size(); ++i)
    {
        try {
            result.push_back(DataFrame::parseDate(dates[i]));
        }
        catch (const std::exception& e) {
            throw std::logic_error(
                "Failed to parse date at row " + std::to_string(i) +
                ": '" + dates[i] + "'"
            );
        }
    }

    return result;
}




std::vector<double> ParseStringDataToFloat(std::vector<std::string> column_str)
{

    // we will store the updated parsed values in new_data.
    std::vector<double> new_data;
    new_data.resize(column_str.size());

    // we iterate through each row of the column
    for (int row_index = 0; row_index < column_str.size(); ++row_index)
    {
        double num;
        auto [ptr_i, ec_i] = std::from_chars(column_str[row_index].data(), column_str[row_index].data() + column_str[row_index].size(), num);

        if (ec_i == std::errc() && ptr_i == column_str[row_index].data() + column_str[row_index].size()) {
            new_data[row_index] = num;
        } else {
            new_data[row_index] = 0.0;
        }
    }
    return new_data;
}


// in this function we convert each column values to be of its specified type.
std::vector<int64_t> ParseStringDataToInt(std::vector<std::string> column_str)
{
    // we will store the updated parsed values in new_data.
    std::vector<int64_t> new_data;
    new_data.resize(column_str.size());

    // we iterate through each row of the column
    for (int row_index = 0; row_index < column_str.size(); ++row_index)
    {

        // we parse the string column element to an int64
        int64_t num;
        auto [ptr_i, ec_i] = std::from_chars(column_str[row_index].data(), column_str[row_index].data() + column_str[row_index].size(), num);

        // we set the corresponding element in new_data to be the value we parsed.
        if (ec_i == std::errc() && ptr_i == column_str[row_index].data() + column_str[row_index].size()) {
            new_data[row_index] = num;
        } else {
            new_data[row_index] = 00;
        }
    }
    return new_data;
}

#include <numeric>

void DataFrame::buildDataFrame(std::string content, char seperator)
{
    std::istringstream stream(content);
    std::string line;
    std::string element;
    std::vector<std::vector<std::string>> data;
    bool header = true;
    int colsAmount = 0;
    int rowsAmount = 0;

    std::vector<DataType> colTypes;
    std::vector<std::string> col_names;

    while (std::getline(stream, line))
    {
        if (line.empty()) continue;
        if (header) {
            std::istringstream linestream(line);
            while (std::getline(linestream, element, seperator))
            {
                col_names.push_back(element);
            }
            colsAmount = col_names.size();
            data.resize(colsAmount);
            colTypes.resize(colsAmount, DataType::kInt64);
            header = false;
        } 
        else {
            std::istringstream linestream(line);
            for (int i = 0; i < colsAmount; ++i)
            {
                if (std::getline(linestream, element, seperator)) {
                    if (colTypes[i] == DataType::kInt64) {
                        int64_t num;
                        auto [ptr_i, ec_i] = std::from_chars(element.data(), element.data() + element.size(), num);
                        if (ec_i != std::errc() || ptr_i != element.data() + element.size()) {
                            colTypes[i] = DataType::kFloat64;
                        }
                    }
                    if (colTypes[i] == DataType::kFloat64) {
                        double f;
                        auto [ptr_f, ec_f] = std::from_chars(element.data(), element.data() + element.size(), f);
                        if (ec_f != std::errc() || ptr_f != element.data() + element.size()) {
                            colTypes[i] = DataType::kString;
                        }
                    }
                    data[i].push_back(element);
                } else {
                    data[i].push_back("");
                }
            }
            ++rowsAmount;
        }
    }

    this->_dimensions = std::make_pair(rowsAmount, colsAmount);
    std::vector<int64_t> indexes(rowsAmount);
    std::iota(indexes.begin(), indexes.end(), 0);
    this->_index.push_back(std::make_unique<Column<int64_t, DataType::kInt64>>(indexes, "Index"));

    for (int i = 0; i < colsAmount; ++i)
    {
        std::string lowerHeader = col_names[i];
        std::transform(lowerHeader.begin(), lowerHeader.end(), lowerHeader.begin(), ::tolower);
        if (lowerHeader.find("date") != std::string::npos) {
            if (colTypes[i] == DataType::kInt64) 
                this->_dataFrame.push_back(std::make_unique<Column<int64_t, DataType::kDate>>(ParseStringDataToInt(data[i]), col_names[i]));
            else
                this->_dataFrame.push_back(std::make_unique<Column<int64_t, DataType::kDate>>(parseDateColumn(data[i]), col_names[i]));
        }
        else if (colTypes[i] == DataType::kInt64) {
            this->_dataFrame.push_back(std::make_unique<Column<int64_t, DataType::kInt64>>(ParseStringDataToInt(data[i]), col_names[i]));
        } else if (colTypes[i] == DataType::kFloat64) {
            this->_dataFrame.push_back(std::make_unique<Column<double, DataType::kFloat64>>(ParseStringDataToFloat(data[i]), col_names[i]));
        } else {
            this->_dataFrame.push_back(std::make_unique<Column<std::string, DataType::kString>>(data[i], col_names[i]));
        }
    }
}

DataFrame::DataFrame(std::string filePath, char seperator)
{
    this->_dataFrame.clear();
    std::ifstream stream(filePath);
    if (!stream.is_open()) return;
    
    std::string content((std::istreambuf_iterator<char>(stream)), std::istreambuf_iterator<char>());
    buildDataFrame(content, seperator);
}

template <typename T, DataType DT>
void Column<T, DT>::print()
{
    int len = 0;

    // determine the longest element
    if constexpr (std::is_same_v<T, std::string>) {
        for (auto& element : _data) {
            if (element.length() > len) len = element.length();
        }
    } else {
        for (auto& element : _data) {
            int elem_len = std::to_string(element).length();
            if (elem_len > len) len = elem_len;
        }
    }

    // consider header length
    if (_header.length() > len) 
        len = _header.length();

    // print header
    std::cout << _header;
    for (int i = _header.length(); i < len; ++i) {
        std::cout << " ";
    }
    std::cout << std::endl << std::endl;

    // print data
    for (auto& value : _data) {
        if constexpr (std::is_same_v<T, std::string>) {
            std::cout << value;
            for (int i = value.length(); i < len; ++i) {
                std::cout << " ";
            }
            std::cout << std::endl;
        } else {
            std::cout << value;
            for (int i = std::to_string(value).length(); i < len; ++i) {
                std::cout << " ";
            }
            std::cout << std::endl;
        }
    }
}

#include <limits>

// in each column we store the length of the longest element. This function updates the attribute _maxElementLength.
// The reason we have this attribute is for aligning properly the columns when printing.

template<>
void Column<std::string, DataType::kString>::updateMaxElementLength()
{
    int len = this->_header.length();
    for (auto& elem : this->_data) {
        int elem_len = elem.length();
        if (elem_len > len) 
            len = elem_len;
    }
    this->_maxElementLength = len;
}

template <typename T, DataType DT>
void Column<T, DT>::updateMaxElementLength() 
{
    int len = this->_header.length();
    for (auto& elem : this->_data) {
        std::stringstream ss;
        ss << std::fixed << std::setprecision(2) << elem;
        std::string roundedStr = ss.str();
        int elem_len = roundedStr.length();
        if (elem_len > len) 
            len = elem_len;    
    }
    this->_maxElementLength = len;
}

template <typename T, DataType DT>
void Column<T, DT>::printElement(int index)
{
    if constexpr (std::is_same_v<T, std::string>) {
        std::cout << this->_data[index];
        int padding_len = this->getMaxElementLength();
        for (int i = this->_data[index].length(); i < padding_len + 2; ++i)
        {
            std::cout << " ";
        }
    } else {
        if (_type == DataType::kDate) {
            std::cout << DataFrame::formatDate(this->_data[index]) << "  ";
        } else {
            // std::cout << std::fixed << std::setprecision(2) << this->_data[index];
            int padding_len = this->getMaxElementLength();
            std::stringstream ss;
            ss << std::fixed << std::setprecision(2) << this->_data[index];
            std::string roundedStr = ss.str();
            int elem_len = roundedStr.length();
            for (int i = elem_len; i < padding_len; ++i)
            {
                std::cout << " ";
            }
            std::cout << std::fixed << std::setprecision(2) << this->_data[index] << "  ";
        }
    }
}


// Constructor for creating a column. 
//In this constructor we call the constructor of ColumnBase which initializes the type of the column.
template <typename T, DataType DT>
Column<T, DT>::Column(const std::vector<T>& data, std::string name) : ColumnBase(DT, name), _data(data)
{
    this->updateMaxElementLength();
}


// The following function prints out the dataframe.
void DataFrame::print()
{

    if (this->_dimensions.second > 5) {
        for (int i = 0; i < this->_dimensions.second; i += 5)
        {
            std::vector<std::string> headers;
            for (int j = i; j < i + 5; ++j)
            {
                if (j >= this->_dimensions.second)
                    break;
                headers.push_back(_dataFrame[j]->getHeader());
            }
            (*this)[headers]->print();
        }
    } else {
        // ANSI Style Strings
        std::string HEADER_BG = "\033[48;5;238m\033[38;5;75m\033[1m"; // Dark grey background + Bold
        std::string RESET = "\033[0m";
        std::string BLUE_TEXT = "\033[1;34m";

        //std::cout << "DataFrame: \n";

        int paddingForIndex = 0;
        for (auto& col : _index)
        {
            paddingForIndex += col->getMaxElementLength();
        }
        // padding
        for (int i = 0; i < paddingForIndex + 2; ++i)
        {
            std::cout << " ";
        }
        for (auto& col : this->_dataFrame)
        {
            if (col->getType() == DataType::kDate) {
                std::cout << HEADER_BG << col->getHeader();
                ///padding
                for (int i = col->getHeader().length(); i < 12; ++i)
                {
                    std::cout << " ";
                }
                std::cout << RESET;
            } else {
                std::cout << HEADER_BG << col->getHeader();
                int padding_len = col->getMaxElementLength();

                ///padding
                for (int i = col->getHeader().length(); i < padding_len + 2; ++i)
                {
                    std::cout << " ";
                }
                std::cout << RESET;
            }
        }
        std::cout << std::endl;
        for (auto& col : this->_index)
        {
            std::cout << BLUE_TEXT << col->getHeader();
            int padding_len = col->getMaxElementLength();
            //padding
            for (int i = col->getHeader().length(); i < padding_len + 2; ++i)
            {
                std::cout << " ";
            }
            std::cout << RESET;
        }
        std::cout << std::endl;
        for (int i = 0; i < this->_dimensions.first; ++i)
        {
            std::cout << BLUE_TEXT;
            for (auto& col : this->_index)
            {
                col->printElement(i);
            }
            std::cout << RESET;
            for (auto& col : this->_dataFrame)
            {
                col->printElement(i);
            }
            std::cout << std::endl;
        }

        std::cout << std::endl;
        // printing the data type of each column.
        //this->printTypes();
    }

}


template<>
double Column<std::string, DataType::kString>::sum() 
{
    return std::numeric_limits<double>::quiet_NaN();
}

template<>
double Column<int64_t, DataType::kDate>::sum() 
{
    return std::numeric_limits<double>::quiet_NaN();
}

template<>
double Column<std::string, DataType::kString>::stdDev() 
{
    return std::numeric_limits<double>::quiet_NaN();
}

template<>
double Column<int64_t, DataType::kDate>::stdDev() 
{
    return std::numeric_limits<double>::quiet_NaN();
}

template<>
double Column<std::string, DataType::kString>::mean() 
{
    return std::numeric_limits<double>::quiet_NaN();
}

template<>
double Column<int64_t, DataType::kDate>::mean() 
{
    return std::numeric_limits<double>::quiet_NaN();
}



// function for caclulating the sum of all the elements in the column.
template <typename T, DataType DT>
double Column<T, DT>::sum()
{
    double sum = 0;
    for (auto& value : this->_data) {
        sum += value;
    }
    return sum;
}

// function for calculating the mean of all the elements in the column.
template <typename T, DataType DT>
double Column<T, DT>::mean()
{
    if (this->_data.size() == 0) return 0.0;
    double sum = 0;
    for (auto& value : this->_data) {
        sum += value;
    }
    return sum / (double)this->_data.size();
}

// function for calculating the standard deviation of all the elements in the column.
template <typename T, DataType DT>
double Column<T, DT>::stdDev()
{
    size_t n = this->_data.size();
    if (n < 2) return 0.0;
    
    double avg = this->mean();
    double sumSqDiff = 0;
    for (auto& value : this->_data) {
        double diff = (double)value - avg;
        sumSqDiff += diff * diff;
    }
    return std::sqrt(sumSqDiff / (double)(n - 1));
}

#include <map>


template<>
double Column<std::string, DataType::kString>::mode() 
{
    return std::numeric_limits<double>::quiet_NaN();
}

template <typename T, DataType DT>
double Column<T, DT>::mode()
{
    std::map<double, int> valueCounts;
    for (int i = 0; i < _data.size(); ++i)
    {
        double value = getAsDouble(i);
        valueCounts[value] += 1;
    }
    auto it = std::max_element(valueCounts.begin(), valueCounts.end(),
    [](const std::pair<double,int>& a, const std::pair<double,int>& b) {
        return a.second < b.second;
    });
    return it->first;
}


template<>
double Column<std::string, DataType::kString>::min() 
{
    return std::numeric_limits<double>::quiet_NaN();
}

template <typename T, DataType DT>
double Column<T, DT>::min()
{
    double min = std::numeric_limits<double>::infinity();
    for (int i = 0; i < _data.size(); ++i)
    {
        double value = getAsDouble(i);
        if (value < min) {
            min = value;
        }
    }
    return min;
}


template<>
double Column<std::string, DataType::kString>::max() 
{
    return std::numeric_limits<double>::quiet_NaN();
}

template <typename T, DataType DT>
double Column<T, DT>::max()
{
    double max = -std::numeric_limits<double>::infinity();
    for (int i = 0; i < _data.size(); ++i)
    {
        double value = getAsDouble(i);
        if (value > max) {
            max = value;
        }
    }
    return max;
}


// function that creates a copy of the column.
template <typename T, DataType DT>
std::unique_ptr<ColumnBase> Column<T, DT>::clone()
{
    return std::make_unique<Column<T, DT>>(*this);
}


// function that creates a copy of a subset of the column.
template <typename T, DataType DT>
std::unique_ptr<ColumnBase> Column<T, DT>::cutClone(size_t startIndex, size_t endIndex)
{
    return std::make_unique<Column<T, DT>>(std::vector<T>(this->_data.begin() + startIndex, this->_data.begin() + endIndex), this->_header);
}


// Function for comapring two elements in the column.
// This function is useful when sorting the dataframe.
template <typename T, DataType DT>
int Column<T, DT>::compare(size_t i, size_t j) const
{
    if (this->_data[i] < this->_data[j])
        return -1;
    else if (this->_data[i] == this->_data[j])
        return 0;
    return 1;
}

/**
 * Return a vector of the indexes of the data, ordered by the values in those indexes.
 * For example, if the column has the data vector <3, 2, 6, 5> then the function will
 * return <1, 0, 3, 2> for asending order or <2, 3, 0, 1> for descending order.
 */



template <typename T, DataType DT>
std::vector<size_t> Column<T, DT>::getOrderedPermutation(bool ascending)
{
    std::vector<size_t> perm(this->_data.size());
    std::iota(perm.begin(), perm.end(), 0);
    if (ascending) {
        std::sort(perm.begin(), perm.end(),
          [&](size_t a, size_t b) { return this->_data[a] < this->_data[b]; });
    } else {
        std::sort(perm.begin(), perm.end(),
          [&](size_t a, size_t b) { return this->_data[a] > this->_data[b]; });
    }
    return perm;
}

// function that creates a copy of a column and orders the column in a given order specified by indexOrder.
template <typename T, DataType DT>
std::unique_ptr<ColumnBase> Column<T, DT>::cloneOrdered(const std::vector<size_t>& indexOrder)
{
    const auto& data = this->getData();
    std::vector<T> orderedData;
    orderedData.reserve(indexOrder.size());
    for (size_t index : indexOrder)
    {
        orderedData.push_back(data[index]);
    }
    return std::make_unique<Column<T, DT>>(orderedData, this->_header);

}

// function for sorting a dataframe based on a single column col.
std::unique_ptr<DataFrame> DataFrame::sortValues(std::string col, bool ascending = true)
{
    // finding the index of the column to sort by.
    int sortColIndex = 0;
    bool found = false;
    for (int i = 0; i < this->_dimensions.second; ++i)
    {
        if (col == this->_dataFrame[i]->getHeader()) {
            sortColIndex = i;
            found = true;
            break;
        }
    }
    if (!found) {
        std::cout << "column not found.\n";
        return nullptr;
    }
    
    // calculating the desired order of indices.
    std::vector<size_t> perm = this->_dataFrame[sortColIndex]->getOrderedPermutation(ascending);

    // creating a new dataframe.
    std::unique_ptr<DataFrame> df = std::make_unique<DataFrame>();
    df->setDimensions(std::make_pair(perm.size(), 0));

    // iterating through the original dataframe's columns and index
    // and adding to the new dataframe sorted copies of the columns and index.
    for (auto& col : this->_dataFrame)
    {
        df->AddColumn(col->cloneOrdered(perm));
    }
    for (auto& col : this->_index)
    {
        df->AddIndexCol(col->cloneOrdered(perm));
    }
    return df;
}


// Function for printing the data types of the dataframe's columns.
void DataFrame::printTypes()
{
    for (auto& col : this->_dataFrame)
    {
        std::cout << col->getHeader();
        DataType type = col->getType();
        if (type == DataType::kInt64) {
            std::cout << " -> int64";
        } else if (type == DataType::kFloat64) {
            std::cout << " -> float64";
        } else {
            std::cout << " -> string";
        }
        std::cout << std::endl;
    }
}


// adding a new column to the dataframe. 
// Adding a column with number of rows different then the dimension of the dataframe is not allowed.
void DataFrame::AddColumn(std::unique_ptr<ColumnBase>&& col) 
{ 
    if (col->len() == this->_dimensions.first) {
        this->_dataFrame.push_back(std::move(col)); 
        this->_dimensions.second += 1;
    } else {
        std::cout << "cannot add column. Dimensions do not match.\n";
    }
}



// subsetting columns. This function creates a new dataframe only with the columns specified by columnNames.
std::unique_ptr<DataFrame> DataFrame::operator[](std::vector<std::string> columnNames)
{
    std::unique_ptr<DataFrame> df = std::make_unique<DataFrame>();
    df->setDimensions(std::make_pair(this->_dimensions.first, 0));

    // The following section finds the columns corresponding to columnNames and adds a copy of them to the new dataframe.
    for (auto& name : columnNames)
    {
        for (auto& col : this->_dataFrame)
        {
            if (col->getHeader() == name) {
                df->AddColumn(col->clone());
            }
        }
    }
    // keeping the original index in the new dataframe.
    for (auto& col : this->_index)
    {
        df->AddIndexCol(col->clone());
    }
    return df;
}

// subsetting rows.  This function creates a new dataframe 
// only with the rows that are within the interval specified by starting_row and end_row.
std::unique_ptr<DataFrame> DataFrame::operator()(int starting_row, int end_row)
{
    std::unique_ptr<DataFrame> df = std::make_unique<DataFrame>();
    df->setDimensions(std::make_pair(end_row - starting_row, 0));

    for (auto& col : this->_dataFrame)
    {
        df->AddColumn(col->cutClone(starting_row, end_row));
    }
    for (auto& col : this->_index)
    {
        df->AddIndexCol(col->cutClone(starting_row, end_row));
    }
    return df;
}


std::vector<double> DataFrame::mean()
{
    std::vector<double> means;
    std::cout << std::endl << "Averages: " << std::endl;
    for (auto& col : this->_dataFrame)
    {
        if (col->getType() != DataType::kString) {
            double mean = col->mean();
            std::cout << col->getHeader() << " : " << std::to_string(mean) << std::endl;
            means.push_back(mean);
        }
    }
    return means;
}


// Function for returning the ith element in the column as of type double.
template<>
double Column<std::string, DataType::kString>::getAsDouble(size_t index) const
{
    throw std::logic_error("getAsDouble on string column");
}

template <typename T, DataType DT>
double Column<T, DT>::getAsDouble(size_t index) const
{
    return static_cast<double>(_data[index]);
}

template <typename T, DataType DT>
std::string Column<T, DT>::getAsString(size_t index) const
{
    if constexpr (std::is_same_v<T, std::string>) {
        return _data[index];
    } else if constexpr (DT == DataType::kDate) {
        return DataFrame::formatDate(static_cast<int64_t>(_data[index]));
    } else {
        std::stringstream ss;
        ss << std::fixed << std::setprecision(2) << _data[index];
        std::string s = ss.str();
        // Remove trailing zeros for a cleaner string representation if it's a "whole" number
        if (s.find('.') != std::string::npos) {
            s.erase(s.find_last_not_of('0') + 1, std::string::npos);
            if (s.back() == '.') s.pop_back();
        }
        return s;
    }
}


template<>
int64_t Column<std::string, DataType::kString>::getAsInt(size_t index) const
{
    throw std::logic_error("getAsInt on string column");
}

template <typename T, DataType DT>
int64_t Column<T, DT>::getAsInt(size_t index) const
{ 
    if constexpr (std::is_same_v<T, double>) {
        return static_cast<int64_t>(_data[index]);  
    }
    else { 
        return _data[index];
    }
}


// Function for returning the raw column data (for columns of type string).
template <typename T, DataType DT>
std::vector<std::string> Column<T, DT>::getColAsStrings() const
{
    if constexpr (!std::is_same_v<T, std::string>) {
        throw std::logic_error("getColAsStrings on non-string column");
    } else {
        return _data;
    }
}


#include <map>
#include <unordered_map>


// function for counting the amount of ocurrances of each label in a given sorted column of type string.
// This function should be executed after the column to group is sorted.
std::unique_ptr<DataFrame> DataFrame::valueCountsOnly(std::string columnToGroup)
{
    // now we find the index for the column to group by. We make sure that it is of type string.
    int groupCol = 0;
    bool found = false;
    for (int i = 0; i < this->_dimensions.second; ++i)
    {
        if (columnToGroup == this->_dataFrame[i]->getHeader()) {
            groupCol = i;
            found = true;
            break;
        }
    }
    if (!found) {
        std::cout << "column not found.\n";
        return nullptr;
    } 

    // getting the column.
    ColumnBase* basePtr = this->_dataFrame[groupCol].get();

    if (this->getDimensions().first == 0) {
        auto df = std::make_unique<DataFrame>();
        df->setDimensions({0, 0});
        return df;
    }

    // calculating the index boundaries of each unique label. 
    // (we assume the column is already sorted, so the same labels will appear in consecutive order).
    std::vector<int> groupBoundaries = {0};
    std::string label = basePtr->getAsString(0);
    // In labels we store the unique values (labels) of the column.
    std::vector<std::string> labels = {label};
    for (int i = 1; i < getDimensions().first; ++i)
    {
        if (basePtr->getAsString(i) != label) {
            groupBoundaries.push_back(i);
            labels.push_back(basePtr->getAsString(i));
            label = basePtr->getAsString(i);
        }
    }
    groupBoundaries.push_back(getDimensions().first);

    // in groupCounts we store the actual value-counts.
    std::vector<int64_t> groupCounts;

    // we calculate the value-counts as the lengths of the intervals between two consecutive indexes stored in groupBoundaries.
    for (int index = 0; index < groupBoundaries.size() - 1; ++index)
    {
        groupCounts.push_back(groupBoundaries[index + 1] - groupBoundaries[index]);
    }

    // we create a new dataframe.
    std::unique_ptr<DataFrame> df = std::make_unique<DataFrame>();
    df->setDimensions(std::make_pair(labels.size(), 0));
    // add the unqiue values of the column as the new dataframe's index with preserved type.
    DataType type = this->_dataFrame[groupCol]->getType();
    if (type == DataType::kString) {
        df->AddIndexCol(std::make_unique<Column<std::string, DataType::kString>>(labels, "Index"));
    } else if (type == DataType::kFloat64) {
        std::vector<double> vals;
        for (int b = 0; b < (int)groupBoundaries.size() - 1; ++b) {
            vals.push_back(this->_dataFrame[groupCol]->getAsDouble(groupBoundaries[b]));
        }
        df->AddIndexCol(std::make_unique<Column<double, DataType::kFloat64>>(vals, "Index"));
    } else if (type == DataType::kInt64) {
        std::vector<int64_t> vals;
        for (int b = 0; b < (int)groupBoundaries.size() - 1; ++b) {
            vals.push_back(this->_dataFrame[groupCol]->getAsInt(groupBoundaries[b]));
        }
        df->AddIndexCol(std::make_unique<Column<int64_t, DataType::kInt64>>(vals, "Index"));
    } else if (type == DataType::kDate) {
        std::vector<int64_t> vals;
        for (int b = 0; b < (int)groupBoundaries.size() - 1; ++b) {
            vals.push_back(this->_dataFrame[groupCol]->getAsInt(groupBoundaries[b]));
        }
        df->AddIndexCol(std::make_unique<Column<int64_t, DataType::kDate>>(vals, "Index"));
    }
    // add the value-counts as a new column in the new dataframe.
    df->AddColumn(std::make_unique<Column<int64_t, DataType::kInt64>>(groupCounts, "value_counts"));

    // return the new dataframe.
    return df;
}

// function for calculating value-counts for a given column in the dataframe: columnToGroup.
std::unique_ptr<DataFrame> DataFrame::valueCounts(std::string columnToGroup)
{
    // we first sort the dataframe by the column to group, then we call the function that calculates the value-counts.
    // the result is returned as a new dataframe.
    auto sorted = this->sortValues(columnToGroup);
    if (!sorted) return nullptr;
    return sorted->valueCountsOnly(columnToGroup);
}


// Funtion for performing GroupBy by multiple columns. 
// When executing this function, we assume the dataframe is already sorted by the columns to group.
// As arguments to the function the columns to aggregate are specified as well, 
// and also the corresponding operations that should be performed on the columns to aggregate.
// Valid operations include sum, mean, max, min, mode and std.
std::unique_ptr<DataFrame> DataFrame::groupByOnly(std::vector<std::string> columnsToGroup, 
                                       std::vector<std::string> columnsToAggregate, std::vector<std::string> operations)
{

    // making sure that for each column to aggregate there is a corresponding operation.
    if (operations.size() != columnsToAggregate.size())
        throw std::invalid_argument("operations size mismatch");

    if (this->_dimensions.first == 0)
        return std::make_unique<DataFrame>();

    // we store in aggregated_cols the indexes of the columns to aggregate
    std::vector<int> aggregated_cols;

    // in this for loop we find the columns' indexes (for aggregation).
    int colIndex = 0;
    for (auto& col : columnsToAggregate)
    {
        // we specify a boolean to know if we found the corresponding column AND the column is not of type string.
        bool found = false;
        for (int i = 0; i < this->_dimensions.second; ++i)
        {
            if (col == this->_dataFrame[i]->getHeader()) {
                found = true;
                if (this->_dataFrame[i]->getType() == DataType::kString && operations[colIndex] != "count") {
                    throw std::logic_error("Cannot aggregate string column for non-count operations");
                }
                if (std::find(aggregated_cols.begin(), aggregated_cols.end(), i) == aggregated_cols.end()) {
                    aggregated_cols.push_back(i);
                }
            }
        }
        if (!found) {
            std::cout << "column not found.\n";
            return nullptr;
        }
        colIndex++;
    }

    // now we find the index for the columns to group by. We make sure that it is of type string.
    std::vector<int> groupCols;
    for (auto& columnToGroup : columnsToGroup)
    {
        bool found = false;
        for (int i = 0; i < this->_dimensions.second; ++i)
        {
            if (columnToGroup == this->_dataFrame[i]->getHeader()) {
                groupCols.push_back(i);
                found = true;
                break; 
            }
        }
        if (!found) {
            std::cout << "column not found.\n";
            return nullptr;
        }
    }

    // claculating the index boundaries of each unique set of labels. 
    // (we assume the columns are already sorted, so the same labels will appear in consecutive order).
    // We treat each set of labels as a vector of strings because we group by multiple columns.
    std::vector<int> groupBoundaries = {0};
    std::vector<std::string> label;
    for (auto& index : groupCols)
    {
        label.push_back(this->_dataFrame[index]->getAsString(0));
    }
    // In labels we store the unique values (labels) of the columns grouped together.
    std::vector<std::vector<std::string>> labels = {label};
    for (int i = 1; i < this->_dimensions.first; ++i)
    {
        std::vector<std::string> nextLabel;
        for (auto& index : groupCols)
        {
            nextLabel.push_back(this->_dataFrame[index]->getAsString(i));
        }       
        if (nextLabel != label) {
            groupBoundaries.push_back(i);
            labels.push_back(nextLabel);
            label.clear();
            label = nextLabel;
        }
    }
    groupBoundaries.push_back(this->_dimensions.first);

    // In aggregations we store the aggregated results. Each nested vector will be a column in the new dataframe.
    std::vector<std::vector<double>> aggregations;
    int numGroups = labels.size();
  
    colIndex = 0;
    for (auto& col : aggregated_cols)
    {
        // For each column to aggregate we create a vector of type double. 
        // Each element in the vector will correspond to a value for a unique set of labels.
        std::vector<double> agg(numGroups, 0.0);
        for (int index = 0; index < groupBoundaries.size() - 1; ++index)
        {
            if (operations[colIndex] == "sum") {
                for (int i = groupBoundaries[index]; i < groupBoundaries[index + 1]; ++i)
                {
                    agg[index] += this->_dataFrame[col]->getAsDouble(i);
                }
            } else if (operations[colIndex] == "mean") {
                for (int i = groupBoundaries[index]; i < groupBoundaries[index + 1]; ++i)
                {
                    agg[index] += this->_dataFrame[col]->getAsDouble(i);
                }
                agg[index] = agg[index] / (groupBoundaries[index + 1] - groupBoundaries[index]);
            } else if (operations[colIndex] == "max") {
                double max = -std::numeric_limits<double>::infinity();
                for (int i = groupBoundaries[index]; i < groupBoundaries[index + 1]; ++i)
                {
                    double value = this->_dataFrame[col]->getAsDouble(i);
                    if (value > max) {
                        max = value;
                    }
                }
                agg[index] = max;
            } else if (operations[colIndex] == "min") {
                double min = std::numeric_limits<double>::infinity();
                for (int i = groupBoundaries[index]; i < groupBoundaries[index + 1]; ++i)
                {
                    double value = this->_dataFrame[col]->getAsDouble(i);
                    if (value < min) {
                        min = value;
                    }
                }
                agg[index] = min;
            } else if (operations[colIndex] == "mode") {
                std::map<double, int> valueCounts;
                for (int i = groupBoundaries[index]; i < groupBoundaries[index + 1]; ++i)
                {
                    double value = this->_dataFrame[col]->getAsDouble(i);
                    valueCounts[value] += 1;
                }
                auto it = std::max_element(valueCounts.begin(), valueCounts.end(),
                [](const std::pair<double,int>& a, const std::pair<double,int>& b) {
                    return a.second < b.second;
                });
                agg[index] = it->first;
            } else if (operations[colIndex] == "count") {
                agg[index] = (double)(groupBoundaries[index + 1] - groupBoundaries[index]);
            } else if (operations[colIndex] == "std") {
                int count = groupBoundaries[index + 1] - groupBoundaries[index];
                if (count < 2) {
                    agg[index] = 0.0;
                } else {
                    double avg = 0;
                    for (int i = groupBoundaries[index]; i < groupBoundaries[index + 1]; ++i)
                        avg += this->_dataFrame[col]->getAsDouble(i);
                    avg /= (double)count;

                    double sumSqDiff = 0;
                    for (int i = groupBoundaries[index]; i < groupBoundaries[index + 1]; ++i) {
                        double diff = this->_dataFrame[col]->getAsDouble(i) - avg;
                        sumSqDiff += diff * diff;
                    }
                    agg[index] = std::sqrt(sumSqDiff / (double)(count - 1));
                }
            }
        }
        aggregations.push_back(agg);
        ++colIndex;
    }

    // creating a new dataframe.
    std::unique_ptr<DataFrame> df = std::make_unique<DataFrame>();
    df->setDimensions(std::make_pair(labels.size(), 0));

    // Creating the indexes of the new dataframe with preserved types.
    for (int i = 0; i < (int)groupCols.size(); ++i)
    {
        int originalColIdx = groupCols[i];
        DataType type = this->_dataFrame[originalColIdx]->getType();
        
        if (type == DataType::kString) {
            std::vector<std::string> vals;
            for (auto& label : labels) vals.push_back(label[i]);
            df->AddIndexCol(std::make_unique<Column<std::string, DataType::kString>>(vals, "Index"));
        } else if (type == DataType::kFloat64) {
            std::vector<double> vals;
            for (int b = 0; b < (int)groupBoundaries.size() - 1; ++b) {
                vals.push_back(this->_dataFrame[originalColIdx]->getAsDouble(groupBoundaries[b]));
            }
            df->AddIndexCol(std::make_unique<Column<double, DataType::kFloat64>>(vals, "Index"));
        } else if (type == DataType::kInt64) {
            std::vector<int64_t> vals;
            for (int b = 0; b < (int)groupBoundaries.size() - 1; ++b) {
                vals.push_back(this->_dataFrame[originalColIdx]->getAsInt(groupBoundaries[b]));
            }
            df->AddIndexCol(std::make_unique<Column<int64_t, DataType::kInt64>>(vals, "Index"));
        } else if (type == DataType::kDate) {
            std::vector<int64_t> vals;
            for (int b = 0; b < (int)groupBoundaries.size() - 1; ++b) {
                vals.push_back(this->_dataFrame[originalColIdx]->getAsInt(groupBoundaries[b]));
            }
            df->AddIndexCol(std::make_unique<Column<int64_t, DataType::kDate>>(vals, "Index"));
        }
    }

    // Adding to the new dataframe the aggregated data.
    int i = 0;
    for (auto& col : aggregations)
    {
        df->AddColumn(std::make_unique<Column<double, DataType::kFloat64>>(col, columnsToAggregate[i]));
        ++i;
    }

    // returning the new dataframe.
    return df;  
}

// Function for performing GroupBy by mutliple columns.
// As arguments to the function the columns to aggregate are specified as well, 
// and aslo the corresponding operations that should be performed on the columns to aggregate.
// Valid operations include sum, mean, max, min, mode and std.
std::unique_ptr<DataFrame> DataFrame::groupBy(std::vector<std::string> columnsToGroup, 
                                       std::vector<std::string> columnsToAggregate, std::vector<std::string> operations)
{
    // First create a sorted dataframe (sorted by the columns to group by), then call the function that performs the group by.
    // We return a new dataframe.
    int len = columnsToGroup.size();
    std::vector<bool> v(len, true);
    auto sortedDf = this->sortValues(columnsToGroup, v);
    if (!sortedDf) return nullptr;
    return sortedDf->groupByOnly(columnsToGroup, columnsToAggregate, operations);
}


void DataFrame::resetIndex()
{
    this->_index.clear();
    std::vector<int64_t> indexes(this->_dimensions.first);
    std::iota(indexes.begin(), indexes.end(), 0);
    this->AddIndexCol(std::make_unique<Column<int64_t, DataType::kInt64>>(indexes, "Index"));
}

void DataFrame::transitionGroupedColumns(const std::vector<std::string>& names)
{
    std::vector<std::unique_ptr<ColumnBase>> newDataFrame;
    
    // Rename and move index columns to the FRONT
    for (size_t i = 0; i < names.size() && i < _index.size(); ++i) {
        auto col = std::move(_index[i]);
        col->rename(names[i]);
        newDataFrame.push_back(std::move(col));
    }
    
    // Append the existing aggregated columns
    for (auto& col : _dataFrame) {
        newDataFrame.push_back(std::move(col));
    }
    
    // Swap the new layout into place
    _dataFrame = std::move(newDataFrame);
    _dimensions.second = _dataFrame.size();
    
    // Clean up the index area and add a standard integer index
    this->resetIndex();
}

void DataFrame::dropIndex()
{
    this->_index.clear();
}


// Function for creating a permutation of indices corresponding to a sorted dataframe (sorted by colNames).
// As a parameeter to the function we pass a vector of boolean values each one corresponds to a column to sort by, 
// it tells us if the sorting by the corresponding column should be ascending or descending.
std::vector<size_t> DataFrame::getOrderedPerm(std::vector<std::string> colNames, std::vector<bool> ascendingLst)
{

    // we store here the indexes of the columns to sort by.
    std::vector<int> colsToSort;

    for (auto& col : colNames)
    {
        bool found = false;
        for (int i = 0; i < this->_dimensions.second; ++i)
        {
            if (col == this->_dataFrame[i]->getHeader()) {
                found = true;
                if (std::find(colsToSort.begin(), colsToSort.end(), i) == colsToSort.end()) {
                    colsToSort.push_back(i);
                }
            }
        }
        if (!found) {
            std::cout << "column not found: " << col << ".\n";
            return {}; // Returns empty vector
        }
    }

    // in perm we store the indexes in sorted order.
    std::vector<size_t> perm(this->_dimensions.first);
    std::iota(perm.begin(), perm.end(), 0); 

    std::sort(perm.begin(), perm.end(), [&](size_t a, size_t b) {
        for (size_t i = 0; i < colsToSort.size(); ++i) {
            int colIndex = colsToSort[i];
            int cmp = this->_dataFrame[colIndex]->compare(a, b); 
            if (cmp != 0) return ascendingLst[i] ? (cmp < 0) : (cmp > 0);
        }
        return false; 
    });

    return perm;
}

// function for sorting a dataframe based on multiple columns colNames.
std::unique_ptr<DataFrame> DataFrame::sortValues(std::vector<std::string> colNames, std::vector<bool> ascendingLst)
{
    if (ascendingLst.size() != colNames.size()) {
        throw std::invalid_argument("ascendingLst must match colNames size");
    }

    // calculating the desired order of indices.
    std::vector<size_t> perm = this->getOrderedPerm(colNames, ascendingLst);
    if (perm.empty() && this->_dimensions.first > 0) {
        return nullptr;
    }

    // creating a new dataframe.
    std::unique_ptr<DataFrame> df = std::make_unique<DataFrame>();
    df->setDimensions(std::make_pair(this->_dimensions.first, 0));

    // iterating through the original dataframe's columns and index
    // and adding to the new dataframe sorted copies of the columns and index.
    for (auto& col : this->_dataFrame)
    {
        df->AddColumn(col->cloneOrdered(perm));
    }
    for (auto& col : this->_index)
    {
        df->AddIndexCol(col->cloneOrdered(perm));
    }
    return df;
}

#include <map>

// pivot shuold be called on a dataframe with two indexes of type string and one column.
std::unique_ptr<DataFrame> DataFrame::pivot()
{
    // getting the raw index data.
    std::vector<std::string> rowLabels = this->_index[0]->getColAsStrings();
    std::vector<std::string> colLabels = this->_index[1]->getColAsStrings();

    // erasing duplicate values from the indexes.
    std::sort(rowLabels.begin(), rowLabels.end());
    auto lastLabels = std::unique(rowLabels.begin(), rowLabels.end());
    rowLabels.erase(lastLabels, rowLabels.end());

    std::sort(colLabels.begin(), colLabels.end());
    auto last = std::unique(colLabels.begin(), colLabels.end());
    colLabels.erase(last, colLabels.end());

    // in this map we store the values of the dataframe for each combination of index labels.
    std::map<std::pair<std::string, std::string>, double> dataPoints;

    // initializing the map.
    for (auto& label1 : rowLabels)
    {
        for (auto& label2 : colLabels)
        {
            dataPoints[std::make_pair(label1, label2)] = 0;
        }
    }

    // interating through the rows 
    // and inserting into the map the corresponding value for each combination of index labels.
    for (int i = 0; i < this->_dimensions.first; ++i)
    {
        dataPoints[std::make_pair(this->_index[0]->getAsString(i), this->_index[1]->getAsString(i))] 
        = this->_dataFrame[0]->getAsDouble(i);
    }

    // creating a new dataframe.
    std::unique_ptr<DataFrame> df = std::make_unique<DataFrame>();
    df->setDimensions(std::make_pair(rowLabels.size(), 0));

    // creating the columns of the new dataframe.

    // iterating through the "columns labels".
    // for each "column label" we iterate through all "row labels", 
    // finding the corresponding value in the map and adding it to the new column col.
    // Then we add the new created column to the new dataframe.
    for (auto& colLabel : colLabels)
    {
        std::vector<double> col;
        for (auto& rowLabel : rowLabels)
        {
            col.push_back(dataPoints.at(std::make_pair(rowLabel, colLabel)));
        }
        df->AddColumn(std::make_unique<Column<double, DataType::kFloat64>>(col, colLabel));
    }

    // here we add the "row labels" as the index of the new dataframe.
    df->AddIndexCol(std::make_unique<Column<std::string, DataType::kString>>(rowLabels, "Index"));

    return df;
}

// Function for creating a Pivot Table.
std::unique_ptr<DataFrame> DataFrame::pivotTable(std::string rows, std::string column, std::string values, std::string operation)
{
    // We first create a new dataframe by grouping the original dataframe by the column that corresponds to the rows
    // and the column that corresponds to the columns. We aggregate by the column that corresponds to the values
    // (with the operation specified).
    // Then we create the resulting dataframe by pivoting the grouped by dataframe.
    return this->groupBy({rows, column}, {values}, {operation})->pivot();
}


// The following function assumes that the column names in both dataframes are distinct.
std::unique_ptr<DataFrame> DataFrame::concatVertically(const DataFrame& df)
{
    // here we store the column indices of the new dataframe to be created
    // mapped to the column indices of the existing dataframes.
    std::map<int, int> indicesMap1;
    std::map<int, int> indicesMap2;

    int index = 0;
    std::vector<int> indicesFromDfInsertedIntoMap;
    for (int i = 0; i < _dimensions.second; ++i)
    {
        bool found = false;
        for (int j = 0; j < df._dimensions.second && j < df._dataFrame.size(); ++j)
        {
            // if both dataframes have a column with the same header
            if (_dataFrame[i]->getHeader() == df._dataFrame[j]->getHeader()) {
                // we first check that the types are aligned.
                if (_dataFrame[i]->getType() == DataType::kString && df._dataFrame[j]->getType() != DataType::kString ||
                    _dataFrame[i]->getType() != DataType::kString && df._dataFrame[j]->getType() == DataType::kString) {
                    std::cout << "column types do not match. cannot concatenate.\n";
                    return nullptr;
                }
                // we map the index to the column index in this.
                indicesMap1[index] = i;
                // we map the index to the column index in df.
                indicesMap2[index] = j;
                indicesFromDfInsertedIntoMap.push_back(j);
                found = true;
                ++index;
            } 
        }
        // if the ith column in this dont have a corresponding column in df, we map index to i.
        if (!found) {
            indicesMap1[index] = i;
            ++index;
        }
    }
    // for the columns in df that don't have a corresponding column in this, we map indices to them in the map.
    for (int j = 0; j < df._dimensions.second && j < df._dataFrame.size(); ++j)
    {
        if (std::find(indicesFromDfInsertedIntoMap.begin(), indicesFromDfInsertedIntoMap.end(), j) == indicesFromDfInsertedIntoMap.end()) {
            indicesMap2[index] = j;
            indicesFromDfInsertedIntoMap.push_back(j);
            ++index;
        }
    }

    // create a new dataframe.
    std::unique_ptr<DataFrame> dfNew = std::make_unique<DataFrame>();
    dfNew->setDimensions(std::make_pair(_dimensions.first + df._dimensions.first, 0));

    // iterating through the indices, 
    // creating in each iteration a new column and adding it to the new dataframe dfNew.
    for (int i = 0; i < index; ++i)
    {
        // if the column to be inserted is a concatenation of a column in this and a column in df.
        if (indicesMap1.find(i) != indicesMap1.end() && indicesMap2.find(i) != indicesMap2.end()) {
            DataType colType = _dataFrame[indicesMap1[i]]->getType();
            if (colType == DataType::kString) {
                // new column
                std::vector<std::string> newCol;
                // adding to newCol the elements from the column in this
                for (int j = 0; j < _dimensions.first; ++j)
                {
                    newCol.push_back(_dataFrame[indicesMap1[i]]->getAsString(j));
                }
                // adding to newCol the elements from the column in df.
                for (int j = 0; j < df._dimensions.first; ++j)
                {
                    newCol.push_back(df._dataFrame[indicesMap2[i]]->getAsString(j));
                }
                // adding to dfNew the new column.
                dfNew->AddColumn(std::make_unique<Column<std::string, DataType::kString>>
                                (newCol, _dataFrame[indicesMap1[i]]->getHeader()));
            } 
            // the types are int64 or float64
            else if (colType == DataType::kInt64 || colType == DataType::kFloat64) {
                // new column
                std::vector<double> newCol;
                // adding to newCol the elements from the column in this
                for (int j = 0; j < _dimensions.first; ++j)
                {
                    newCol.push_back(_dataFrame[indicesMap1[i]]->getAsDouble(j));
                }
                // adding to newCol the elements from the column in df.
                for (int j = 0; j < df._dimensions.first; ++j)
                {
                    newCol.push_back(df._dataFrame[indicesMap2[i]]->getAsDouble(j));
                }
                // adding to dfNew the new column.
                dfNew->AddColumn(std::make_unique<Column<double, DataType::kFloat64>>
                                (newCol, _dataFrame[indicesMap1[i]]->getHeader()));
            } 
            // the type is date
            else {
               // new column
                std::vector<int64_t> newCol;
                // adding to newCol the elements from the column in this
                for (int j = 0; j < _dimensions.first; ++j)
                {
                    newCol.push_back(_dataFrame[indicesMap1[i]]->getAsInt(j));
                }
                // adding to newCol the elements from the column in df.
                for (int j = 0; j < df._dimensions.first; ++j)
                {
                    newCol.push_back(df._dataFrame[indicesMap2[i]]->getAsInt(j));
                }
                // adding to dfNew the new column.
                dfNew->AddColumn(std::make_unique<Column<int64_t, DataType::kInt64>>
                                (newCol, _dataFrame[indicesMap1[i]]->getHeader()));
            }
        } 
        // else if the column to be inserted is only from this.
        else if (indicesMap1.find(i) != indicesMap1.end()) {
            DataType colType = _dataFrame[indicesMap1[i]]->getType();
            if (colType == DataType::kString) {
                // new column
                std::vector<std::string> newCol;
                // adding to newCol the elements from the column in this
                for (int j = 0; j < _dimensions.first; ++j)
                {
                    newCol.push_back(_dataFrame[indicesMap1[i]]->getAsString(j));
                }
                // adding to newCol empty elements.
                for (int j = 0; j < df._dimensions.first; ++j)
                {
                    newCol.push_back("");
                }
                // adding to dfNew the new column.
                dfNew->AddColumn(std::make_unique<Column<std::string, DataType::kString>>
                                (newCol, _dataFrame[indicesMap1[i]]->getHeader()));
            } 
            // it is int64 or float64
             else if (colType == DataType::kInt64 || colType == DataType::kFloat64) {
                // new column
                std::vector<double> newCol;
                for (int j = 0; j < _dimensions.first; ++j)
                {
                    newCol.push_back(_dataFrame[indicesMap1[i]]->getAsDouble(j));
                }
                // adding to newCol empty elements.
                for (int j = 0; j < df._dimensions.first; ++j)
                {
                    newCol.push_back(std::numeric_limits<double>::quiet_NaN());
                }
                // adding to dfNew the new column.
                dfNew->AddColumn(std::make_unique<Column<double, DataType::kFloat64>>
                                (newCol, _dataFrame[indicesMap1[i]]->getHeader()));
            }      
            // type is date
            else {
                // new column
                std::vector<int64_t> newCol;
                for (int j = 0; j < _dimensions.first; ++j)
                {
                    newCol.push_back(_dataFrame[indicesMap1[i]]->getAsInt(j));
                }
                // adding to newCol empty elements.
                for (int j = 0; j < df._dimensions.first; ++j)
                {
                    newCol.push_back(0);
                }
                // adding to dfNew the new column.
                dfNew->AddColumn(std::make_unique<Column<int64_t, DataType::kDate>>
                                (newCol, _dataFrame[indicesMap1[i]]->getHeader()));

            }      
        } 
        // else the column to be inserted is only from df.
        else {
            DataType colType = df._dataFrame[indicesMap2[i]]->getType();
            if (colType == DataType::kString) {
                // new column
                std::vector<std::string> newCol;
                // adding to newCol empty elements.
                for (int j = 0; j < _dimensions.first; ++j)
                {
                    newCol.push_back("");
                }
                // adding to newCol the elements from the column in df.
                for (int j = 0; j < df._dimensions.first; ++j)
                {
                    newCol.push_back(df._dataFrame[indicesMap2[i]]->getAsString(j));
                }
                // adding to dfNew the new column.
                dfNew->AddColumn(std::make_unique<Column<std::string, DataType::kString>>
                                (newCol, df._dataFrame[indicesMap2[i]]->getHeader()));
            } 
            // it is int64 or float64
             else if (colType == DataType::kInt64 || colType == DataType::kFloat64) {
                // new column
                std::vector<double> newCol;
                // adding to newCol empty elements.
                for (int j = 0; j < _dimensions.first; ++j)
                {
                    newCol.push_back(std::numeric_limits<double>::quiet_NaN());
                }
                // adding to newCol the elements from the column in df.
                for (int j = 0; j < df._dimensions.first; ++j)
                {
                    newCol.push_back(df._dataFrame[indicesMap2[i]]->getAsDouble(j));
                }
                // adding to dfNew the new column.
                dfNew->AddColumn(std::make_unique<Column<double, DataType::kFloat64>>
                                (newCol, df._dataFrame[indicesMap2[i]]->getHeader()));
            }  
            // type is date
            else {
                // new column
                std::vector<int64_t> newCol;
                // adding to newCol empty elements.
                for (int j = 0; j < _dimensions.first; ++j)
                {
                    newCol.push_back(0);
                }
                // adding to newCol the elements from the column in df.
                for (int j = 0; j < df._dimensions.first; ++j)
                {
                    newCol.push_back(df._dataFrame[indicesMap2[i]]->getAsInt(j));
                }
                // adding to dfNew the new column.
                dfNew->AddColumn(std::make_unique<Column<int64_t, DataType::kDate>>
                                (newCol, df._dataFrame[indicesMap2[i]]->getHeader()));

            }          
        }
    }

    // for now we reset the index.
    std::vector<int64_t> indexCol(dfNew->_dimensions.first);
    std::iota(indexCol.begin(), indexCol.end(), 0);

    dfNew->AddIndexCol(std::make_unique<Column<int64_t, DataType::kInt64>>(indexCol, "Index"));

    dfNew->setDimensions(std::make_pair(dfNew->getDimensions().first, index));
    return dfNew;

}

// function to be called on sorted value-counts.
void DataFrame::plotFrequencies(int colIndex)
{
    std::string HEADER_BG = "\033[48;5;238m\033[38;5;75m\033[1m";
    std::string RESET = "\033[0m";
    std::string BLUE_TEXT = "\033[1;34m";

    double maxFreq = 0;
    for (int i = 0; i < _dimensions.first; ++i) {
        if (_dataFrame[colIndex]->getAsDouble(i) > maxFreq) 
            maxFreq = _dataFrame[colIndex]->getAsDouble(i);
    }

    int maxBarWidth = 50;

    for (int i = 0; i < _dimensions.first; ++i)
    {
        std::cout << BLUE_TEXT;
        _index[0]->printElement(i);
        std::cout << RESET;

        double val = _dataFrame[colIndex]->getAsDouble(i);
        double percentage = (maxFreq > 0) ? (val / maxFreq) : 0;
    
        // This formula maps 0-100% to a range of blues (21 to 45)
        int blueShade = 21 + (int)(percentage * 24); 
        
        std::string colorCode = "\033[48;5;" + std::to_string(blueShade) + "m";
        
        int scaledWidth = (int)(percentage * maxBarWidth);

        std::cout << colorCode;
        for (int h = 0; h < scaledWidth; ++h)
        {
            std::cout << " ";
        }
        std::cout << RESET << val << std::endl  << std::endl;
    }
}

void DataFrame::plotValueCount(std::string columnToGroup)
{
    valueCounts(columnToGroup)->sortValues("value_counts", false)->plotFrequencies(0);
}

void DataFrame::barPlot(std::string columnToPlot)
{
    int colIndex;
    bool found = false;
    for (int i = 0; i < this->_dimensions.second; ++i)
    {
        if (columnToPlot == _dataFrame[i]->getHeader()) {
            colIndex = i;
            found = true;
            if (_dataFrame[i]->getType() == DataType::kString) {
                std::cout << "column is of type string. Cannot plot column.\n";
                return;
            }
            break;
        }
    }
    if (!found) {
        std::cout << "column not found.\n";
        return;
    }
    plotFrequencies(colIndex);
}


std::vector<size_t> DataFrame::operator==(const std::string& target)
{
    if (_dimensions.second != 1)
        throw std::logic_error("Expected a dataframe with one column. Cannot make comparison.\n");
    else if (_dataFrame[0]->getType() != DataType::kString)
        throw std::logic_error("Expected a string column. Cannot make comparison.\n");

    std::vector<size_t> perm;
    
    for (int index = 0; index < _dataFrame[0]->len(); ++index)
    {
        if (_dataFrame[0]->getAsString(index) == target) {
            perm.push_back(index);
        }
    }
    return perm;
}


std::vector<size_t> DataFrame::operator!=(const std::string& target)
{
    if (_dimensions.second != 1)
        throw std::logic_error("Expected a dataframe with one column. Cannot make comparison.\n");
    else if (_dataFrame[0]->getType() != DataType::kString)
        throw std::logic_error("Expected a string column. Cannot make comparison.\n");

    std::vector<size_t> perm;
    
    for (int index = 0; index < _dataFrame[0]->len(); ++index)
    {
        if (_dataFrame[0]->getAsString(index) != target) {
            perm.push_back(index);
        }
    }
    return perm;
}



std::vector<size_t> DataFrame::operator==(const double target)
{
    if (_dimensions.second != 1)
        throw std::logic_error("Expected a dataframe with one column. Cannot make comparison.\n");
    else if (_dataFrame[0]->getType() == DataType::kString)
        throw std::logic_error("Expected a numeric column. Cannot make comparison.\n");

    std::vector<size_t> perm;
    
    for (int index = 0; index < _dataFrame[0]->len(); ++index)
    {
        if (_dataFrame[0]->getAsDouble(index) == target) {
            perm.push_back(index);
        }
    }
    return perm;   
}

std::vector<size_t> DataFrame::operator!=(const double target)
{
    if (_dimensions.second != 1)
        throw std::logic_error("Expected a dataframe with one column. Cannot make comparison.\n");
    else if (_dataFrame[0]->getType() == DataType::kString)
        throw std::logic_error("Expected a numeric column. Cannot make comparison.\n");

    std::vector<size_t> perm;
    
    for (int index = 0; index < _dataFrame[0]->len(); ++index)
    {
        if (_dataFrame[0]->getAsDouble(index) != target) {
            perm.push_back(index);
        }
    }
    return perm;   
}

std::vector<size_t> DataFrame::operator>(const double target)
{
    if (_dimensions.second != 1)
        throw std::logic_error("Expected a dataframe with one column. Cannot make comparison.\n");
    else if (_dataFrame[0]->getType() == DataType::kString)
        throw std::logic_error("Expected a numeric column. Cannot make comparison.\n");

    std::vector<size_t> perm;
    
    for (int index = 0; index < _dataFrame[0]->len(); ++index)
    {
        if (_dataFrame[0]->getAsDouble(index) > target) {
            perm.push_back(index);
        }
    }
    return perm;     
}

std::vector<size_t> DataFrame::operator<(const double target)
{
    if (_dimensions.second != 1)
        throw std::logic_error("Expected a dataframe with one column. Cannot make comparison.\n");
    else if (_dataFrame[0]->getType() == DataType::kString)
        throw std::logic_error("Expected a numeric column. Cannot make comparison.\n");

    std::vector<size_t> perm;
    
    for (int index = 0; index < _dataFrame[0]->len(); ++index)
    {
        if (_dataFrame[0]->getAsDouble(index) < target) {
            perm.push_back(index);
        }
    }
    return perm;     
}

std::vector<size_t> DataFrame::operator<=(const double target)
{
    if (_dimensions.second != 1)
        throw std::logic_error("Expected a dataframe with one column. Cannot make comparison.\n");
    else if (_dataFrame[0]->getType() == DataType::kString)
        throw std::logic_error("Expected a numeric column. Cannot make comparison.\n");

    std::vector<size_t> perm;
    
    for (int index = 0; index < _dataFrame[0]->len(); ++index)
    {
        if (_dataFrame[0]->getAsDouble(index) <= target) {
            perm.push_back(index);
        }
    }
    return perm;     
}

std::vector<size_t> DataFrame::operator>=(const double target)
{
    if (_dimensions.second != 1)
        throw std::logic_error("Expected a dataframe with one column. Cannot make comparison.\n");
    else if (_dataFrame[0]->getType() == DataType::kString)
        throw std::logic_error("Expected a numeric column. Cannot make comparison.\n");

    std::vector<size_t> perm;
    
    for (int index = 0; index < _dataFrame[0]->len(); ++index)
    {
        if (_dataFrame[0]->getAsDouble(index) >= target) {
            perm.push_back(index);
        }
    }
    return perm;     
}


std::unique_ptr<DataFrame> DataFrame::operator[](std::vector<size_t> perm)
{
    std::unique_ptr<DataFrame> df = std::make_unique<DataFrame>();
    df->setDimensions(std::make_pair(perm.size(), 0));

    for (auto& col : this->_dataFrame)
    {
        df->AddColumn(col->cloneOrdered(perm));
    }
    for (auto& col : this->_index)
    {
        df->AddIndexCol(col->cloneOrdered(perm));
    }
    return df;
}


std::unique_ptr<DataFrame> DataFrame::agg(std::vector<std::string> columns, std::vector<std::string> operations)
{

    if (columns.size() != operations.size())
        throw std::logic_error("vectors sizes do not match.\n");

    std::vector<int> colsToAgg;
    for (auto& col: columns)
    {
        bool found = false;
        for (int i = 0; i < this->_dimensions.second; ++i)
        {
            if (col == this->_dataFrame[i]->getHeader()) {
                colsToAgg.push_back(i);
                found = true;
                if (this->_dataFrame[i]->getType() == DataType::kString) {
                    std::cout << "Cannot aggregate column of type string.\n";
                    return nullptr;
                }
            }
        }
        if (!found) {
            std::cout << "column not found.\n";
            return nullptr;
        }
    }

    std::unique_ptr<DataFrame> df = std::make_unique<DataFrame>();
    int i = 0;
    for (auto& colIndex : colsToAgg)
    {
        std::unique_ptr<DataFrame> dfSub = std::make_unique<DataFrame>();
        dfSub->setDimensions(std::make_pair(1, 0));
        if (operations[i] == "mean") {
            std::vector<double> aggValue = {_dataFrame[colIndex]->mean()};
            dfSub->AddColumn(std::make_unique<Column<double, DataType::kFloat64>>(aggValue, "mean"));
            std::vector<std::string> header = {columns[i]};
            dfSub->AddIndexCol(std::make_unique<Column<std::string, DataType::kString>>(header, "Index"));
        }
        else if (operations[i] == "sum") {
            std::vector<double> aggValue = {_dataFrame[colIndex]->sum()};
            dfSub->AddColumn(std::make_unique<Column<double, DataType::kFloat64>>(aggValue, "sum"));
            std::vector<std::string> header = {columns[i]};
            dfSub->AddIndexCol(std::make_unique<Column<std::string, DataType::kString>>(header, "Index"));
        }
        else if (operations[i] == "min") {
            std::vector<double> aggValue = {_dataFrame[colIndex]->min()};
            dfSub->AddColumn(std::make_unique<Column<double, DataType::kFloat64>>(aggValue, "min"));
            std::vector<std::string> header = {columns[i]};
            dfSub->AddIndexCol(std::make_unique<Column<std::string, DataType::kString>>(header, "Index"));
        }
        else if (operations[i] == "max") {
            std::vector<double> aggValue = {_dataFrame[colIndex]->max()};
            dfSub->AddColumn(std::make_unique<Column<double, DataType::kFloat64>>(aggValue, "max"));
            std::vector<std::string> header = {columns[i]};
            dfSub->AddIndexCol(std::make_unique<Column<std::string, DataType::kString>>(header, "Index"));
        }
        else if (operations[i] == "mode") {
            std::vector<double> aggValue = {_dataFrame[colIndex]->mode()};
            dfSub->AddColumn(std::make_unique<Column<double, DataType::kFloat64>>(aggValue, "mode"));
            std::vector<std::string> header = {columns[i]};
            dfSub->AddIndexCol(std::make_unique<Column<std::string, DataType::kString>>(header, "Index"));
        }
        df = df->concatVertically(*dfSub);
        df->dropIndex();
        df->AddIndexCol(std::make_unique<Column<std::string, DataType::kString>>(columns, "Index"));
        ++i;
    }
    return df;
}


void DataFrame::toCsv(const std::string& filePath)
{
    std::ofstream file(filePath);
    if (!file) {
        std::cerr << "Failed to open file\n";
        return;
    }

    file << std::fixed;
    file << std::setprecision(0);

    for (auto& col : _dataFrame)
    {
        file << col->getHeader() << ",";
    }
    file << "\n";
    for (int i = 0; i < _dimensions.first; ++i)
    {
        for (auto& col : _dataFrame)
        {
            if (col->getType() != DataType::kString)
                file << col->getAsDouble(i) << ",";
            else
                file << col->getAsString(i) << ",";
        }
        file << "\n";
    }
    file.close();
    return;
}


void DataFrame::renameColumns(std::vector<std::string> header)
{
    if (_dimensions.second != header.size()) {
        throw std::logic_error("header sizes don't match.\n");
    }
    int i = 0;
    for (auto& col : _dataFrame)
    {
        col->rename(header[i]);
        ++i;
    }
    return;
}


void DataFrame::dropColumn(std::string colName) {
    for (auto it = _dataFrame.begin(); it != _dataFrame.end(); ++it) {
        if ((*it)->getHeader() == colName) {
            _dataFrame.erase(it);
            _dimensions.second--;
            return;
        }
    }
}

std::unique_ptr<DataFrame> DataFrame::clone() const {
    auto newDf = std::make_unique<DataFrame>();
    newDf->_dimensions = this->_dimensions;
    for (const auto& col : this->_dataFrame) {
        newDf->_dataFrame.push_back(col->clone());
    }
    for (const auto& idx : this->_index) {
        newDf->_index.push_back(idx->clone());
    }
    return newDf;
}

bool DataFrame::hasColumn(std::string colName) const {
    for (const auto& col : _dataFrame) {
        if (col->getHeader() == colName) return true;
    }
    return false;
}

ColumnBase* DataFrame::getColumn(std::string colName) const {
    for (const auto& col : _dataFrame) {
        if (col->getHeader() == colName) return col.get();
    }
    return nullptr;
}


// Explicit template instantiations for the linker
template class Column<long long, DataType::kInt64>;
template class Column<double, DataType::kFloat64>;
template class Column<std::string, DataType::kString>;
template class Column<long long, DataType::kDate>;
template class Column<int64_t, DataType::kInt64>;
template class Column<int64_t, DataType::kDate>;