#include "dataframeplus.h"
#include <iostream>

int main() {
    DataFrame df = DataFrame("../data/Coffe_sales.csv", ',');
    df(0, 5)->print();
    df.valueCounts("coffee_name")->print();
    std::unique_ptr<DataFrame> newDf = df.groupBy({"coffee_name", "Time_of_Day"}, {"money"}, {"sum"});
    newDf = newDf->sortValues({"money"}, {false});
    auto moneyCol = (*newDf)[{"money"}];
    auto mask = (*moneyCol) >= 5400;
    newDf = (*newDf)[mask];
    std::cout << "Revenue from different types of coffee during different types of the day:" << std::endl;
    newDf->print();
    newDf->barPlot("money");

    std::cout << "Revenue from different types of coffee comparing the days of the week:" << std::endl;
    std::unique_ptr<DataFrame> pivotedDf = df.pivotTable("coffee_name", "Weekday", "money", "sum");
    pivotedDf->print();
}