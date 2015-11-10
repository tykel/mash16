/*
 *   mash16 - the chip16 emulator
 *   Copyright (C) 2012-2015 tykel
 *
 *   mash16 is free software: you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation, either version 3 of the License, or
 *   (at your option) any later version.
 *
 *   mash16 is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with mash16.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "jit.h"

static int cur_col = -1;
static uint8_t col_bv = 0;
static int col_map[16];

void jit_regs_init()
{
    int i;

    for(i = 0; i < 16; i++) {
        col_map[i] = -1;
    }
}

void jit_regs_alloc(jit_insn *is, int num_insns)
{
    int i;

    // Pass 1: naive register coloring
    // Assign a new color to each register we encounter
    // So there is a 1-to-1 mapping between Chip16 and x86_64
    for(i = 0; i < num_insns; ++i) {
        jit_insn *ji = &is[i];
        if(!ji->has_regs) {
            continue;
        }
        if(ji->num_r_to != 0) {
            int c = col_map[ji->r_to];
            if(c == -1) {
                int newc = ++cur_col;
                // We haven't seen this register before, so assign
                // it a new color
                col_map[ji->color_to] = newc;
                ji->color_to = newc; 
            } else {
                ji->color_to = col_map[ji->r_to];
            }
        }
        if(ji->num_r_from > 0) {
            int c = col_map[ji->r_from[0]];
            if(c == -1) {
                int newc = ++cur_col;
                // We haven't seen this register before, so assign
                // it a new color
                col_map[ji->r_from[0]] = newc;
                ji->color_from[0] = newc;
            } else {
                ji->color_from[0] = col_map[ji->r_from[0]];
            }
        }
        if(ji->num_r_from > 1) {
            int c = col_map[ji->r_from[1]];
            if(c == -1) {
                int newc = ++cur_col;
                // We haven't seen this register before, so assign
                // it a new color
                col_map[ji->r_from[1]] = newc;
                ji->color_from[1] = newc;
            } else {
                ji->color_from[1] = col_map[ji->r_from[1]];
            }
        }
    }
}
