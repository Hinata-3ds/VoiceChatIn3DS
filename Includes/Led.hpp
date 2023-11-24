#pragma once

#include <CTRPluginFramework.hpp>
#include "types.h"

namespace CTRPluginFramework
{
  class Led
  {
  public:
    Led(MenuEntry *entry);
    void setSmoothing(u8 smoothing);
    void setColor(u8 r, u8 g, u8 b);
    void update();

  private:
    // https://www.3dbrew.org/wiki/MCURTC:SetInfoLEDPattern
    struct Data
    {
      u8 delay = 0;
      u8 smoothing = 0;
      u8 loop_dlay = 0xFF;
      u8 unused = 0;
      u8 r[32];
      u8 g[32];
      u8 b[32];
    };

    MenuEntry *entry;
    Data data;
    u8 r;
    u8 g;
    u8 b;
    bool running;

    bool hasColor();
  };
}