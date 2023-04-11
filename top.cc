/***************************************************************************
                                  top.cc                                   
                             -----------------
    begin                : Thu April 11 2023
    authors              : Yves Lhuillier
    email                : yves.lhuillier@gmail.com
***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License version 2        *
 *   as published by the Free Software Foundation.                         *
 *                                                                         *
 ***************************************************************************/

#include <iostream>
#include <set>
#include <map>


char const* symsel(std::initializer_list<char const*> symbols, std::initializer_list<unsigned> codes)
{
  static std::map<std::string, char const*> tr =
    {
      {"└", " "}, {"┘", " "}, {"┌", " "}, {"┐", " "},
      {"┗", "└"}, {"┛", "┘"}, {"┏", "┌"}, {"┓", "┐"},
      {"┕", "╶"}, {"┍", "╶"},
      {"┙", "╴"}, {"┑", "╴"},
      {"┖", "╵"}, {"┚", "╵"},
      {"┎", "╷"}, {"┒", "╷"},
      {"─", " "}, {"━", "─"},
      {"│", " "}, {"┃", "│"},
      {" ", " "}, {"o", "o"}
    };
  unsigned sel = 0;
  for (unsigned code : codes)
    sel = sel*2 | (code & 1);
  for (auto symbol : symbols)
    if (sel-- == 0) return tr[symbol];
  return "?";
}

struct Tile
{
  /* piece 0-3: walls {E,N,W,S}, piece 4: player */
  Tile() : pieces() {}

  int cmp(Tile const& rhs) const { return pieces - rhs.pieces; }

  void dump(std::ostream& sink) const {  sink << pieces; }
  
  void print(std::ostream& sink, unsigned y, unsigned x) const
  {
    switch (y*3+x)
      {
      case 0*3+0: sink << symsel({"└","┕","┖","┗"}, {pieces >> 2, pieces >> 3}); break;
      case 0*3+1: sink << symsel({"─","━"}, {pieces >> 3}); break;
      case 0*3+2: sink << symsel({"┘","┙","┚","┛"}, {pieces >> 0, pieces >> 3}); break;
      case 1*3+0: sink << symsel({"│","┃"}, {pieces >> 2}); break;
      case 1*3+1: sink << symsel({" ","o"}, {pieces >> 4}); break;
      case 1*3+2: sink << symsel({"│","┃"}, {pieces >> 0}); break;
      case 2*3+0: sink << symsel({"┌","┍","┎","┏"}, {pieces >> 2, pieces >> 1}); break;
      case 2*3+1: sink << symsel({"─","━"}, {pieces >> 1}); break;
      case 2*3+2: sink << symsel({"┐","┑","┒","┓"}, {pieces >> 0, pieces >> 1}); break;
      }
  }
  void set(unsigned bit, bool v)
  {
    unsigned mask = 1 << bit;
    if (v) pieces |=  mask;
    else   pieces &= ~mask;
  }
  bool has(unsigned bit) const { return pieces >> bit & 1; }
  unsigned pieces;
};

struct State
{
  enum { rows = 3, cols = 2, count = rows*cols };

  State() : map(), parent(0) {}
  struct From {};
  State(From, State const& _parent) : map(_parent.map), parent(&_parent) {}

  int cmp(State const& rhs) const
  {
    for (unsigned idx = 0; idx < count; ++idx)
      if (int delta = map[idx].cmp(rhs.map[idx]))
        return delta;
    return 0;
  }
  bool operator < (State const& rhs) const { return cmp(rhs) < 0; }

  template <class UnaryPredicate>
  Tile& cond_at(unsigned offset, UnaryPredicate p)
  {
    for (unsigned idx = 0; idx < count; ++idx)
      {
        Tile& tile = map[idx];
        if (p(tile.pieces) and offset-- == 0)
          return tile;
      }
    throw *this;
    return map[0];
  }
  Tile& at(unsigned idx) { return map[idx]; }
  Tile& at(unsigned y, unsigned x) { return map[y*cols+x]; }

  Tile const& at(unsigned y, unsigned x) const { return const_cast<State*>(this)->at(y, x); }

  void dump(std::ostream& sink) const
  {
    for (unsigned y = rows; y-- > 0;)
      {
        char const* sep = "";
        for (unsigned x = 0; x < cols; ++x)
          at(y,x).dump(sink << sep), sep = ", ";
        sink << '\n';
      }
  }
  
  void print(std::ostream& sink) const
  {
    for (unsigned y = 3*rows; y-- > 0;)
      {
        for (unsigned x = 0; x < 3*cols; ++x)
          at(y/3,x/3).print(sink,y%3,x%3);
        sink << '\n';
      }
  }

  void awin_awin(std::ostream& sink) const
  {
    std::cout << "yes!\n";
    for (State const* current = this; current; current = current->parent)
      {
        current->print(std::cout << "======\n");
      }
    throw Tile();
  }

  void find_a_path(std::ostream& sink, unsigned moves) const
  {
    std::set<State> core, front, spread( {*this} );
    
    for (unsigned depth = 0; depth++ < moves;)
      {
        std::swap(front, spread);
        spread.clear();
        core.insert(front.begin(), front.end());

        for (auto const& pivot : front)
          {
            for (unsigned y = 0; y < rows; ++y)
              {
                for (unsigned x = 0; x < cols; ++x)
                  {
                    Tile const& tile = pivot.at(y, x);
                    for (unsigned dir = 0; dir < 4; ++dir)
                      {
                        unsigned ny = dir & 1 ? y + 1 - bool(dir & 2)*2 : y, nx = dir & 1 ? x : x + 1 - bool(dir & 2)*2;
                        // Look for a winner
                        if (ny == rows and tile.has(4) and not tile.has(1))
                          {
                            if (depth != moves)
                              return; // Early winner, bailing out
                            pivot.awin_awin(sink);
                            continue;
                          }
                        // All moves exhausted ?
                        if (depth == moves)
                          continue;
                        if (ny >= rows or nx >= cols)
                          continue;
                        if (tile.has(4))
                          {
                            Tile const& dst = pivot.at(ny, nx);
                            if (tile.has(dir) or dst.has(dir^2) or dst.pieces == 0)
                              continue;
                            State nstate(State::From(), *core.insert(pivot).first);
                            nstate.at( y,  x).set(4,false);
                            nstate.at(ny, nx).set(4,true);
                            if (core.count(nstate))
                              continue;
                            spread.insert(nstate);
                          }
                        else
                          {
                            if (pivot.at(ny, nx).pieces)
                              continue;
                            State nstate(State::From(), *core.insert(pivot).first);
                            std::swap(nstate.at(y,x).pieces, nstate.at(ny, nx).pieces);
                            if (core.count(nstate))
                              continue;
                            spread.insert(nstate);
                          }
                      }
                  }
              }
          }
      }
  }
  
  Tile map[count];
  State const* parent = 0;
};

int
main(int argc, char** argv)
{
  //  for (StateIterator itr; itr.next();)
  unsigned depth = argc == 2 ? atoi(argv[1]) : 5;

  try {
  for (unsigned apos = 0; apos < 6; ++apos)
    {
      for (unsigned bpos = apos + 1; bpos < 6; ++ bpos)
        {
          for (unsigned cpos = 0; cpos < 4; ++cpos)
            {
              for (unsigned dpos = cpos + 1; dpos < 4; ++dpos)
                {
                  for (unsigned rots = 0; rots < 4*4*2*2; ++rots)
                    {
                      State state;
                      for (unsigned idx = 0; idx < 2; ++idx)
                        {
                          unsigned rot = (rots >> (0+idx*4)) % 4;
                          state.at(idx ? apos : bpos).pieces = (0b110011 >> rot) & 0b1111;
                        }
                      for (unsigned idx = 0; idx < 2; ++idx)
                        {
                          unsigned rot = (rots >> (8+idx*2)) % 2;
                          state.cond_at(idx ? cpos : dpos, [](unsigned pieces) { return pieces == 0; }).pieces = rot ? 0b1010 : 0b0101;
                        }
                      for (unsigned ppos = 0, lppos = 0; ppos < 4; lppos = ppos, ++ppos)
                        {
                          state.cond_at(lppos, [](unsigned pieces) { return pieces != 0; }).set(4,false);
                          state.cond_at(ppos, [](unsigned pieces) { return pieces != 0; }).set(4,true);
                          state.find_a_path(std::cout, depth);
                        }
                    }
                }
            }
        }
    }
  }
  catch (Tile const&)
    {
      return 0;
    }

    // {
    //   Path path = itr.
    //   path.print(std::cout);
    // }
  
  return 0;
}
