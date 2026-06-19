# DataFramePlusPlus
Pandas-inspired Dataframes framework for C++

This repository contains the source code of a C++ native data analysis library - an alternative to Pandas for C++ developers.

The framework contains three classes: ColumnBase, Column and DataFrame. 
ColumnBase is a pure virtual class. It includes the implementation of the common functionallity of all types of columns.
Column is a templated class that derives from ColumnBase.
A DataFrame object contains a collection of Column objects.
The user should solely interact with the DataFrame and Column classes.

Four Data types are supported for the data stored in the DataFrame: Integer, Float, String and Date.

A DataFrame is an object for storing tabular data. It can contain columns of different types.
A DataFrame contains an index object (which can be multi index), and column(s). 
Each Column has a corresponding header, corresponding value cells, and a data type.

Note: The data is stored column wise, not row wise. Meaning each column is an object. Cells are connected in rows implicitly.

The functionality of the DataFrame includes:

    - Aggregations (min, max, sum, avg, mode, count, standard deviation)

    - Vertical Concatenation (of two dataFrame objects)

    - GroupBy

    - ValueCounts

    - Subsetting of rows

    - Subsetting of columns

    - Filtering (==, <, >, <=, >=, !=)

    - Index Reset

    - Pivot table

    - Simple plots inside the terminal

    - Sorting

    - Reading/Writing from/to CSV

    - Converting the index into column(s)


A big portion of the DataFrame operations produce new DataFrames (they are not inplace operations).
For instance, when you perform GroupBy, the underlying DataFrame is not modified. Instead, a new grouped dataFrame is created, and is returned as a smart pointer.

The users of the framework should have basic familiarity of smart pointers due to its heavy usage.

A usage example of the framework is given in the file "main.cpp" inside the "usage" folder.
A comparison of the code in "main.cpp" with the syntax of Pandas is given in "example_code.py" inside the "usage" folder.

The library source code is free to use. The only requirement is to give credit to Yair Tal.
The use of this framework is suitable for data analysis tasks as well as for any other projects where usage of tabular data is heavy.

"src/library.h" is the header file of the framework.
"src/library.cpp" is the cpp implementation of the framework.

Static compilation example (Windows):

{path}\DataFramePlusPlus\src> g++ .\dataframeplus.cpp ..\usage\main.cpp -std=c++20

{path}\DataFramePlusPlus\src> .\a.exe

DataFrame++ is created by Yair Tal.
