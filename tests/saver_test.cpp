#include <cassert>
#include <cstdlib>
#include <iostream>
#include <string>
#include <unordered_set>
#include <vector>

#include "gen_regex.h"

const long N = 300;
const long MaxDepth = 10;
const bool VERBOSE = false;
const bool KEEP = false;

using namespace std;
using namespace tree_guide;

vector<string> FNs;
vector<string> Generated;

const string Prefix("// ");

void save_choices() {
  DefaultGuide G1;
  SaverGuide G2(&G1, Prefix);
  for (int i = 0; i < N; ++i) {
    long Depth = 1 + (i % MaxDepth);
    auto C1 = G2.makeChooser();
    assert(C1);
    auto C2 = static_cast<SaverChooser *>(C1.get());
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
    if (VERBOSE) {
      cout << fn << " : " << Str << "\n\n";
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
    FileGuide G;
    if (VERBOSE)
      cout << "about to parse choices from " << FNs.at(i) << "\n";
    if (!G.parseChoices(FNs.at(i), Prefix))
      exit(-1);
    auto C = G.makeChooser();
    assert(C);
    if (VERBOSE)
      cout << "about to generate\n";
    auto Str = gen(*C, Depth);
    if (VERBOSE)
      cout << "generated " << Str << "\n";
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
