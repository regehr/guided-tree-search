#include <cassert>
#include <cstdlib>
#include <iostream>
#include <string>
#include <sstream>
#include <unordered_set>
#include <vector>

#include "gen_regex.h"
#include "mutate.h"

const long N = 300;
const long MaxDepth = 10;
const bool VERBOSE = true;

using namespace std;
using namespace tree_guide;

vector<string> Generated, Choices;

const string Prefix("// ");

void make_choices() {
  DefaultGuide G1;
  SaverGuide G2(&G1, Prefix);
  for (int i = 0; i < N; ++i) {
    long Depth = 1 + (i % MaxDepth);
    auto C1 = G2.makeChooser();
    assert(C1);
    auto C2 = static_cast<SaverChooser *>(C1.get());
    assert(C2);
    auto S1 = gen(*C2, Depth);
    Generated.push_back(S1);
    auto S2 = formatChoices(C2->getChoices(), C2->getPrefix());
    Choices.push_back(S2);
    if (VERBOSE) {
      cout << i << ":\n";
      cout << S1 << "\n\n";
      cout << S2 << "\n\n";
    }
  }
}

void printChoices(const vector<rec> &C) {
  for (auto r : C) {
    switch (r.k) {
    case RecKind::START:
      cout << "{";
      break;
    case RecKind::END:
      cout << "}";
      break;
    case RecKind::NUM:
      cout << r.v;
      break;
    default:
      assert(false);
    }
    cout << " ";
  }
  cout << "\n\n";
}

int use_choices() {
  int pass = 0;
  for (int i = 0; i < N; ++i) {
    long Depth = 1 + (i % MaxDepth);
    FileGuide FG;
    FG.setSync(Sync::RESYNC);
    stringstream s(Choices.at(i));
    if (!FG.parseChoices(s, Prefix))
      exit(-1);

    if (VERBOSE)
      cout << "-------------------- " << i << " --------------------\n";
    
    auto Ch = FG.getChoices();
    if (VERBOSE) {
      cout << "original choices:\n";
      printChoices(Ch);
    }

    mutator::mutate_choices(Ch);
    
    if (VERBOSE) {
      cout << "mutated choices:\n";
      printChoices(Ch);
    }

    FG.replaceChoices(Ch);

    tree_guide::SaverGuide SG(&FG, Prefix);
    auto Ch1 = SG.makeChooser();
    auto Ch2 = static_cast<tree_guide::SaverChooser *>(Ch1.get());
    assert(Ch2);

    auto Str = gen(*Ch2, Depth);
    if (VERBOSE)
      cout << "generated: " << Str << "\n";

    auto C3 = Ch2->getChoices();
    
    if (VERBOSE) {
      cout << "actual choices:\n";
      printChoices(C3);
    }

    ++pass;
    if (VERBOSE)
      cout <<  "\n\n";
  }
  return pass;
}

int main() {
  make_choices();
  auto pass = use_choices();
  cout << pass << " tests passed.\n";
}
