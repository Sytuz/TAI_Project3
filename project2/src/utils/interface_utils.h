#ifndef INTERFACE_UTILS_H
#define INTERFACE_UTILS_H

#include <string>

// User interface functions
int getIntInput(const std::string& prompt, int minValue = 1, int maxValue = 100);
double getDoubleInput(const std::string& prompt, double minValue = 0.0, double maxValue = 1.0);
std::string getStringInput(const std::string& prompt);
bool askYesNo(const std::string& prompt);

#endif // INTERFACE_UTILS_H