#ifndef BEHAVIORGENERATOR_H_
#define BEHAVIORGENERATOR_H_

#include <string>
#include "Global.h"

bool hkxcmdProcess(std::string xmlfile, std::string hkxfile, bool last = false);
bool hkxcmdXmlInput(std::string hkxfile, vecstr& fileline);

#endif