/*
 *    ascii.h  --  ASCII table
 *
 *    Copyright (C) 2001, 2002, 2003
 *      Tomi Manninen (oh2bns@sral.fi)
 *
 *    This file is part of gMFSK.
 *
 *    gMFSK is free software; you can redistribute it and/or modify
 *    it under the terms of the GNU General Public License as published by
 *    the Free Software Foundation; either version 2 of the License, or
 *    (at your option) any later version.
 *
 *    gMFSK is distributed in the hope that it will be useful,
 *    but WITHOUT ANY WARRANTY; without even the implied warranty of
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *    GNU General Public License for more details.
 *
 *    You should have received a copy of the GNU General Public License
 *    along with gMFSK; if not, write to the Free Software
 *    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */

static char *ascii[256] = {
	"<NUL>", "<SOH>", "<STX>", "<ETX>",
	"<EOT>", "<ENQ>", "<ACK>", "<BEL>",
	"<BS>",  "\t",    "\n",    "<VT>", 
	"<FF>",  "\n",    "<SO>",  "<SI>",
	"<DLE>", "<DC1>", "<DC2>", "<DC3>",
	"<DC4>", "<NAK>", "<SYN>", "<ETB>",
	"<CAN>", "<EM>",  "<SUB>", "<ESC>",
	"<FS>",  "<GS>",  "<RS>",  "<US>",
	" ",     "!",     "\"",    "#",    
	"$",     "%",     "&",     "'",   
	"(",     ")",     "*",     "+",    
	",",     "-",     ".",     "/",   
	"0",     "1",     "2",     "3",    
	"4",     "5",     "6",     "7",   
	"8",     "9",     ":",     ";",    
	"<",     "=",     ">",     "?",   
	"@",     "A",     "B",     "C",    
	"D",     "E",     "F",     "G",   
	"H",     "I",     "J",     "K",    
	"L",     "M",     "N",     "O",   
	"P",     "Q",     "R",     "S",    
	"T",     "U",     "V",     "W",   
	"X",     "Y",     "Z",     "[",    
	"\\",    "]",     "^",     "_",   
	"`",     "a",     "b",     "c",    
	"d",     "e",     "f",     "g",   
	"h",     "i",     "j",     "k",    
	"l",     "m",     "n",     "o",   
	"p",     "q",     "r",     "s",    
	"t",     "u",     "v",     "w",   
	"x",     "y",     "z",     "{",    
	"|",     "}",     "~",     "<DEL>",
	"<200>", "<201>", "<202>", "<203>",
	"<204>", "<205>", "<206>", "<207>",
	"<210>", "<211>", "<212>", "<213>",
	"<214>", "<215>", "<216>", "<217>",
	"<220>", "<221>", "<222>", "<223>",
	"<224>", "<225>", "<226>", "<227>",
	"<230>", "<231>", "<232>", "<233>",
	"<234>", "<235>", "<236>", "<237>",
	"\240",  "\241",  "\242",  "\243", 
	"\244",  "\245",  "\246",  "\247",
	"\250",  "\251",  "\252",  "\253", 
	"\254",  "\255",  "\256",  "\257",
	"\260",  "\261",  "\262",  "\263", 
	"\264",  "\265",  "\266",  "\267",
	"\270",  "\271",  "\272",  "\273", 
	"\274",  "\275",  "\276",  "\277",
	"\300",  "\301",  "\302",  "\303", 
	"\304",  "\305",  "\306",  "\307",
	"\310",  "\311",  "\312",  "\313", 
	"\314",  "\315",  "\316",  "\317",
	"\320",  "\321",  "\322",  "\323", 
	"\324",  "\325",  "\326",  "\327",
	"\330",  "\331",  "\332",  "\333", 
	"\334",  "\335",  "\336",  "\337",
	"\340",  "\341",  "\342",  "\343", 
	"\344",  "\345",  "\346",  "\347",
	"\350",  "\351",  "\352",  "\353", 
	"\354",  "\355",  "\356",  "\357",
	"\360",  "\361",  "\362",  "\363", 
	"\364",  "\365",  "\366",  "\367",
	"\370",  "\371",  "\372",  "\373", 
	"\374",  "\375",  "\376",  "\377"
};
