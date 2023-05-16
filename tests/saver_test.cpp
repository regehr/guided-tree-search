#include <cassert>
#include <cstdlib>
#include <iostream>
#include <string>
#include <unordered_set>
#include <vector>

#include "regex.h"

const long N = 100;
const long Depth = 25;

using namespace std;
using namespace tree_guide;

vector<string> FNs;
vector<string> Generated;
int pass = 0;

void save_choices() {
  SaverGuide<DefaultGuide> G;
  for (int i = 0; i < N; ++i) {
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
    out.close();
  }
}

void use_choices() {
  for (int i = 0; i < N; ++i) {
    FileGuide G(FNs.at(i));
    auto C = G.makeChooser();
    assert(C);
    auto Str = gen(*C, Depth);
    assert(Str == Generated.at(i));
    ++pass;
    remove(FNs.at(i).c_str());
  }
}

int main() {
  save_choices();
  use_choices();
  cout << pass << " tests passed.\n";
}
