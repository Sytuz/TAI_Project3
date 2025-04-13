#include "interface_utils.h"
#include <iostream>
#include <limits>

using namespace std;

// Function to get valid integer input
int getIntInput(const string &prompt, int minValue, int maxValue)
{
    int value;
    while (true)
    {
        cout << prompt;
        if (cin >> value && value >= minValue && value <= maxValue)
        {
            break;
        }
        cout << "Please enter a valid integer between " << minValue << " and " << maxValue << endl;
        cin.clear();
        cin.ignore(numeric_limits<streamsize>::max(), '\n');
    }
    return value;
}

// Function to get valid double input
double getDoubleInput(const string &prompt, double minValue, double maxValue)
{
    double value;
    while (true)
    {
        cout << prompt;
        if (cin >> value && value >= minValue && value <= maxValue)
        {
            break;
        }
        cout << "Please enter a valid number between " << minValue << " and " << maxValue << endl;
        cin.clear();
        cin.ignore(numeric_limits<streamsize>::max(), '\n');
    }
    return value;
}

// Function to get string input
string getStringInput(const string &prompt)
{
    string value;
    cout << prompt;
    cin.ignore(numeric_limits<streamsize>::max(), '\n');
    getline(cin, value);
    return value;
}

// Function to ask yes/no question with yes as default
bool askYesNo(const string &prompt)
{
    char response;
    while (true)
    {
        cout << prompt << " (Y/n): ";
        string line;
        getline(cin, line);

        // If user just pressed enter, default to yes
        if (line.empty())
        {
            return true;
        }

        response = tolower(line[0]);
        if (response == 'y' || response == 'n')
        {
            break;
        }
        cout << "Please enter 'y' or 'n'" << endl;
    }
    return (response == 'y');
}