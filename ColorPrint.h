#ifndef COLOR_PRINT
#define COLOR_PRINT

typedef enum _COLOR
{
	BLACK, NAVY, GREEN, TEAL,
	MAROON, PURPLE, OLIVE, SILVER, GRAY,
	BLUE, LIME, AQUA, RED, FUCHSIA,
	YELLOW, WHITE
}COLOR;

int ColorPrintf(COLOR color, const char *format, ...);

void PrintAllColor();

#endif