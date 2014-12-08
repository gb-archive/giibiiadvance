/*
    GiiBiiAdvance - GBA/GB  emulator
    Copyright (C) 2011-2014 Antonio Ni�o D�az (AntonioND)

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "../build_options.h"
#include "../debug_utils.h"

#include "gameboy.h"
#include "cpu.h"
#include "memory.h"
#include "general.h"
#include "ppu.h"
#include "video.h"
#include "interrupts.h"
#include "gb_main.h"

//----------------------------------------------------------------

extern _GB_CONTEXT_ GameBoy;

//----------------------------------------------------------------

void GB_PPUUpdateClocks_GBC(int increment_clocks)
{
    _GB_MEMORY_ * mem = &GameBoy.Memory;

    GameBoy.Emulator.ly_clocks += increment_clocks;

    int done = 0;
    while(done == 0)
    {
        switch(GameBoy.Emulator.ScreenMode)
        {
            case 2:
                if(GameBoy.Emulator.ly_clocks >= (82<<GameBoy.Emulator.DoubleSpeed) )
                {
                    GameBoy.Emulator.ScreenMode = 3;
                    mem->IO_Ports[STAT_REG-0xFF00] |= 0x03;

                    GB_PPUCheckStatSignal();
                }
                else
                {
                    done = 1;
                }
                break;
            case 3:
                if(GameBoy.Emulator.ly_clocks >= (252<<GameBoy.Emulator.DoubleSpeed) )
                {
                    GameBoy.Emulator.DrawScanlineFn(GameBoy.Emulator.CurrentScanLine);

                    GameBoy.Emulator.ScreenMode = 0;
                    mem->IO_Ports[STAT_REG-0xFF00] &= 0xFC;

                    GameBoy.Emulator.ly_drawn = 1;

                    GB_PPUCheckStatSignal();
                }
                else
                {
                    done = 1;
                }
                break;
            case 0:
                if(GameBoy.Emulator.ly_drawn == 0)
                {
                    if(GameBoy.Emulator.ly_clocks >= 2)
                    {
                        GB_PPUCheckLYC();

                        if(GameBoy.Emulator.CurrentScanLine == 144)
                        {
                            GameBoy.Emulator.ScreenMode = 1;
                            mem->IO_Ports[STAT_REG-0xFF00] &= 0xFC;
                            mem->IO_Ports[STAT_REG-0xFF00] |= 0x01;

                            GB_SetInterrupt(I_VBLANK);
                        }
                        else
                        {
                            GameBoy.Emulator.ScreenMode = 2;
                            mem->IO_Ports[STAT_REG-0xFF00] &= 0xFC;
                            mem->IO_Ports[STAT_REG-0xFF00] |= 0x02;
                        }

                        GB_PPUCheckStatSignal();
                    }
                    else
                    {
                        done = 1;
                    }
                }
                else
                {
                    if(GameBoy.Emulator.ly_clocks >= (456<<GameBoy.Emulator.DoubleSpeed) )
                    {
                        GameBoy.Emulator.ly_clocks -= (456<<GameBoy.Emulator.DoubleSpeed);

                        GameBoy.Emulator.CurrentScanLine ++;
                        GameBoy.Emulator.ly_drawn = 0;

                        mem->IO_Ports[LY_REG-0xFF00] = GameBoy.Emulator.CurrentScanLine;
                    }
                    else
                    {
                        done = 1;
                    }
                }
                break;
            case 1:
                if(GameBoy.Emulator.ly_clocks >= (456<<GameBoy.Emulator.DoubleSpeed) )
                {
                    GameBoy.Emulator.ly_clocks -= (456<<GameBoy.Emulator.DoubleSpeed);

                    if(GameBoy.Emulator.CurrentScanLine == 0)
                    {
                        GameBoy.Emulator.ScreenMode = 2;
                        mem->IO_Ports[STAT_REG-0xFF00] &= 0xFC;
                        mem->IO_Ports[STAT_REG-0xFF00] |= 0x02;
                        GB_PPUCheckStatSignal();
                        break;
                    }

                    GameBoy.Emulator.CurrentScanLine ++;

                    if(GameBoy.Emulator.CurrentScanLine == 153)
                    {
                        GameBoy.Emulator.ly_clocks += ((456-8)<<GameBoy.Emulator.DoubleSpeed); // 8 clocks this scanline
                    }
                    else if(GameBoy.Emulator.CurrentScanLine == 154)
                    {
                        GameBoy.Emulator.CurrentScanLine = 0;
                        GameBoy.Emulator.FrameDrawn = 1;

                        GameBoy.Emulator.ly_clocks += (8<<GameBoy.Emulator.DoubleSpeed); // 456 - 8 cycles left of vblank...
                    }

                    mem->IO_Ports[LY_REG-0xFF00] = GameBoy.Emulator.CurrentScanLine;

                    GB_PPUCheckLYC();

                    GB_PPUCheckStatSignal();
                }
                else
                {
                    done = 1;
                }
                break;
            default:
                done = 1;
                break;
        }
    }
}

int GB_PPUGetClocksToNextEvent_GBC(void)
{
    int clocks_to_next_event = 0x7FFFFFFF;

    if(GameBoy.Emulator.lcd_on)
    {
        switch(GameBoy.Emulator.ScreenMode)
        {
            case 2:
                clocks_to_next_event = (82<<GameBoy.Emulator.DoubleSpeed) - GameBoy.Emulator.ly_clocks;
                break;
            case 3:
                clocks_to_next_event = 2;
                break;
            case 0:
                clocks_to_next_event = 2;
                //clocks_to_next_event = (456<<GameBoy.Emulator.DoubleSpeed) - GameBoy.Emulator.ly_clocks;
                break;
            case 1:
                clocks_to_next_event = (456<<GameBoy.Emulator.DoubleSpeed) - GameBoy.Emulator.ly_clocks;
                break;
            default:
                break;
        }
    }

    return clocks_to_next_event;
}

//----------------------------------------------------------------
