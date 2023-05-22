#include <cassert>
#include <cstdlib>
#include <iostream>
#include <string>
#include <unordered_set>
#include <vector>

#include "regex.h"

const long N = 1000;
const long MaxDepth = 50;
const bool PRINT = true;
const bool KEEP = true;

using namespace std;
using namespace tree_guide;

vector<string> FNs;
vector<string> Generated;

void save_choices() {
  SaverGuide<DefaultGuide> G;
  for (int i = 0; i < N; ++i) {
    long Depth = 1 + (i % MaxDepth);
    auto C1 = G.makeChooser();
    assert(C1);
    auto C2 = static_cast<SaverChooser<DefaultGuide> *>(C1.get());
    assert(C2);
    auto Str = gen(*C2, Depth);
    ofstream out;
    string fn = "test" + to_string(i) + ".txt";
    FNs.push_back(fn);
    Generated.push_back(Str);
    out.open(fn);
    assert(out.is_open());
    out << Str << "\n\n";
    out << C2->formatChoices();
    out << "\n";
    if (PRINT) {
      cout << Str << "\n\n";
      cout << C2->formatChoices();
      cout << "\n";
    }
    out.close();
  }
}

int use_choices() {
  int pass = 0;
  for (int i = 0; i < N; ++i) {
    long Depth = 1 + (i % MaxDepth);
    FileGuide G(FNs.at(i));
    auto C = G.makeChooser();
    assert(C);
    auto Str = gen(*C, Depth);
    assert(Str == Generated.at(i));
    ++pass;
    if (!KEEP)
      remove(FNs.at(i).c_str());
  }
  return pass;
}

int main() {
  save_choices();
  int pass = use_choices();
  cout << pass << " tests passed.\n";
}
