#include "guide.h"
#include <string>

static const long RegexDepth = 6;

std::string gen(tree_guide::Chooser &C, long Depth);
