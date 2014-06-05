#ifndef READIN_H
#define READIN_H
#include <iostream>
#include <fstream>
#include <string>
#include <sstream>
#include <vector>
#include <stdlib.h>     /* atoi */
#include <unordered_map>
#include <memory>
#include <algorithm>
#include "Common.hpp"

using namespace std;

static inline VectorGraph* ReadFile(string path, int num);

#endif