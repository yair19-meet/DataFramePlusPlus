import pandas as pd
import matplotlib.pyplot as plt

"""
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
"""


df = pd.read_csv("../data/Coffe_sales.csv")
print(df.head())
print(df.value_counts("coffee_name"))
newDf = df.groupby(by=["coffee_name", "Time_of_Day"]).agg({"money": "sum"})
newDf.sort_values(by="money", ascending=False, inplace=True)
newDf = newDf[newDf["money"] >= 5400]
print(newDf)
newDf.plot(kind="bar")
plt.show()
pivoted_df = df.pivot_table(index="coffee_name", columns="Weekday", values="money", aggfunc="sum")
print(pivoted_df)
