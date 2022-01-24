template <typename T> class PriQ {
  struct Elt {
    std::vector<T> Vec;
    int StartPos;
  };
  std::vector<Elt> Data;
  int Highest = -1;
  const long MaxFree = 256;

public:
  /*
   * insert element at given level
   */
  void insert(T t, int Level) {
    if ((unsigned long)Level >= Data.size())
      Data.resize(Level + 1);
    Data.at(Level).Vec.push_back(t);
    if (Highest == -1 || Level < Highest)
      Highest = Level;
  }

  /*
   * remove item from the given level
   */
  std::optional<T> remove(int Level) {
    if (Level >= Data.size())
      return {};
    if (count(Level) < 1)
      return {};
    auto &Q = Data.at(Level);
    auto t = Q.Vec.at(Q.StartPos);
    Q.StartPos++;
    if (Q.StartPos > MaxFree) {
      Q.Vec.erase(Q.Vec.begin(), Q.Vec.begin() + Q.StartPos);
      Q.StartPos = 0;
    }
    if (Level == Highest && count(Level) == 0) {
      Highest = -1;
      for (int L = Level + 1; L < Data.size(); ++L) {
        if (count(L) > 0) {
          Highest = L;
          break;
        }
      }
    }
    return t;
  }

  /*
   * remove highest-priority item at any level
   */
  std::optional<T> removeHead() {
    auto L = firstNonemptyLevel();
    if (L == -1)
      return {};
    else
      return remove(L);
  }

  /*
   * return number of items at this level
   */
  int count(int Level) {
    if (Level >= Data.size())
      return 0;
    return Data.at(Level).Vec.size() - Data.at(Level).StartPos;
  }

  /*
   * return the smallest level that is nonempty, or else -1 if all
   * levels are empty
   */
  int firstNonemptyLevel() { return Highest; }
};
