#include "types.h"
#include "x86.h"
#include "defs.h"
#include "kbd.h"

int kbdgetc(void)
{
	static uint shift, alt;
	static uchar *charcode[5] = {
		normalmap, shiftmap, ctlmap, ctlmap, shiftaltmap};
	uint st, data, c;

	st = inb(KBSTATP);
	if ((st & KBS_DIB) == 0)
		return -1;
	data = inb(KBDATAP);

	if (data == 0xE0)
	{
		shift |= E0ESC;
		return 0;
	}
	if (data == 0x38)
	{
		alt = 1;
	}
	if (data == 0xB8)
	{
		alt = 0;
	}
	else if (data & 0x80)
	{
		// Key released
		data = (shift & E0ESC ? data : data & 0x7F);
		shift &= ~(shiftcode[data] | E0ESC);
		return 0;
	}
	else if (shift & E0ESC)
	{
		// Last character was an E0 escape; or with 0x80
		data |= 0x80;
		shift &= ~E0ESC;
	}

	shift |= shiftcode[data];
	shift ^= togglecode[data];

	if (alt && shift && charcode[4][data] == NO)
		return NO;

	c = charcode[shift & (CTL | SHIFT)][data];
	if (shift & CAPSLOCK)
	{
		if ('a' <= c && c <= 'z')
			c += 'A' - 'a';
		else if ('A' <= c && c <= 'Z')
			c += 'a' - 'A';
	}
	return c;
}

void kbdintr(void)
{
	consoleintr(kbdgetc);
}
