#ifndef HISTORY_H
#define HISTORY_H

#include "fx.h"

// Stopwatch history: a small fixed list of stop values, newest first.
// Plain data, no widgets, so it can be tested without a display.
class History {
public:
  enum{ MAXENTRIES=50 };
  struct Entry{
    FXTime when;      // Wall clock time of the stop
    FXTime elapsed;   // Stopwatch value at the stop
    };
private:
  Entry entries[MAXENTRIES];
  FXint count;
public:
  History();
  void clear();
  FXint size() const { return count; }
  const Entry& at(FXint i) const { return entries[i]; }
  void add(FXTime when,FXTime elapsed);
  void load(FXSettings& reg);
  void save(FXSettings& reg) const;
  static FXString formatElapsed(FXTime ns);
  static FXString formatWhen(FXTime when);
  };

#endif
