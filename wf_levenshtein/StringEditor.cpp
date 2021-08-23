#include "StringEditor.h"

string StringEditor(string strbase, size_t position, size_t operation, string symbol)
{
    int FlagLimit = strbase.length() - 1;
    try {
        if (operation > 2 || operation < 0) {
            throw "ERROR";
        }
    }
    catch (const char*) {
        cerr << "OPERATION ERROR" << endl;

    }
    try {
        if (position <= FlagLimit)
        {
            if (operation == 0)
            {
                if (position == 0)
                {
                    strbase = symbol + strbase;
                }
                strbase = strbase.substr(0, position) + symbol + strbase.substr(position, strbase.length());
            }
            if (operation == 1)
            {
                strbase = strbase.substr(0, position) + symbol + strbase.substr(position + 1, strbase.length());

            }
            if (operation == 2)
            {
                strbase = strbase.substr(0, position) + strbase.substr(position + 1, strbase.length());
            }
        }
        FlagLimit = strbase.length() - 1;
        return strbase;
    }
    catch (std::out_of_range) {
        throw "Position out of range";
    }
}

/*
int main() {
    cout << StringEditor("holas", 0, 0, "G") << endl;
    return 0;
}
*/