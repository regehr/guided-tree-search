template <typename T> class PriQ {
  struct Elt {
    std::vector<T> Vec;
    long StartPos;
  };
  std::vector<Elt> Data;
  long Highest = -1;
  const long MaxFree = 256;

public:
  /*
   * insert element at given level
   */
  void insert(T t, long Level) {
    if (Level >= (long)Data.size())
      Data.resize(Level + 1);
    Data.at(Level).Vec.push_back(t);
    if (Highest == -1 || Level < Highest)
      Highest = Level;
  }

  /*
   * remove item from the given level
   */
  std::optional<T> remove(long Level) {
    if (Level >= (long)Data.size())
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
      for (long L = Level + 1; L < (long)Data.size(); ++L) {
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
  std::pair<std::optional<T>, long> removeHead() {
    auto L = firstNonemptyLevel();
    if (L == -1)
      return {{}, -1};
    else
      return {remove(L), L};
  }

  /*
   * return number of items at this level
   */
  long count(long Level) {
    if (Level >= (long)Data.size())
      return 0;
    return Data.at(Level).Vec.size() - Data.at(Level).StartPos;
  }

  /*
   * return the smallest level that is nonempty, or else -1 if all
   * levels are empty
   */
  long firstNonemptyLevel() { return Highest; }
};
