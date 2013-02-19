/*
 *   mash16 - the chip16 emulator
 *   Copyright (C) 2012-2013 tykel
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

 const char* str_ops[] = 
 {
    /* 0x */
    "nop","cls","vblnk","bgc","spr","drw","drw","rnd","flip","snd0","snd1","snd2","snd3","snp","sng","___",
    /* 1x */
    "jmp","jmc","j","jme","call","ret","jmp","c","call","___","___","___","___","___","___","___",
    /* 2x */
    "ldi","ldi","ldm","ldm","mov","___","___","___","___","___","___","___","___","___","___","___",
    /* 3x */
    "stm","stm","___","___","___","___","___","___","___","___","___","___","___","___","___","___",
    /* 4x */
    "addi","add","add","___","___","___","___","___","___","___","___","___","___","___","___","___",
    /* 5x */
    "subi","sub","sub","cmpi","cmp","___","___","___","___","___","___","___","___","___","___","___",
    /* 6x */
    "andi","and","and","tsti,","tst","___","___","___","___","___","___","___","___","___","___","___",
    /* 7x */
    "ori","or","or","___","___","___","___","___","___","___","___","___","___","___","___","___",
    /* 8x */
    "xori","xor","xor","___","___","___","___","___","___","___","___","___","___","___","___","___",
    /* 9x */
    "muli","mul","mul","___","___","___","___","___","___","___","___","___","___","___","___","___",
    /* Ax */
    "divi","div","div","___","___","___","___","___","___","___","___","___","___","___","___","___",
    /* Bx */
    "shl","shr","sar","shl","shr","sar","___","___","___","___","___","___","___","___","___","___",
    /* Cx */
    "push","pop","pushall","popall","pushf","popf","___","___","___","___","___","___","___","___","___","___",
    /* Dx */
    "pal","pal","___","___","___","___","___","___","___","___","___","___","___","___","___","___"
 };

 const char* str_cond[] = 
 {
    "z","nz","n","nn","p","o","no","a","nc","c","be","g","ge","l","le","*"
 };