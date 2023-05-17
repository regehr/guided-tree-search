template <typename T> class PriQ {
  struct Elt {
    std::vector<T> Vec;
    uint64_t StartPos;
  };
  std::vector<Elt> Data;
  uint64_t Highest = (uint64_t)-1;
  const uint64_t MaxFree = 256;

public:
  /*
   * insert element at given level
   */
  void insert(T t, uint64_t Level) {
    if (Level >= (uint64_t)Data.size())
      Data.resize(Level + 1);
    Data.at(Level).Vec.push_back(t);
    if (Highest == (uint64_t)-1 || Level < Highest)
      Highest = Level;
  }

  /*
   * remove item from the given level
   */
  std::optional<T> remove(uint64_t Level) {
    if (Level >= (uint64_t)Data.size())
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
      Highest = (uint64_t)-1;
      for (uint64_t L = Level + 1; L < (uint64_t)Data.size(); ++L) {
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
  std::pair<std::optional<T>, uint64_t> removeHead() {
    auto L = firstNonemptyLevel();
    if (L == (uint64_t)-1)
      return {{}, (uint64_t)-1};
    else
      return {remove(L), L};
  }

  /*
   * return number of items at this level
   */
  uint64_t count(uint64_t Level) {
    if (Level >= Data.size())
      return 0;
    return Data.at(Level).Vec.size() - Data.at(Level).StartPos;
  }

  /*
   * return the smallest level that is nonempty, or else -1 if all
   * levels are empty
   */
  uint64_t firstNonemptyLevel() { return Highest; }
};
