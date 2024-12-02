//
// Created by charles on 01/12/24.
//

#include <spdlog/spdlog.h>

struct Point {
  int x;
  int y;
};

Point makePoint(int x, int y) {
  return {x, y};
}

float call(Point &&pnt) {
  spdlog::info("Call1  {}", pnt.x);
  return 0.0;
}

int call(const Point &pnt) {
  spdlog::info("Call2   {}", pnt.x);
  return 0;
}


int main()
{
  Point pnt = makePoint(1, 2);
  call(pnt);
  call(makePoint(3, 4));

  return 0;
}