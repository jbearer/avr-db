/**
 * Simple example program demonstrating simple branching and arithmetic instructions.
 */

static char x;       // Uninitialized global (.bss)
static char y = 1;   // Initialized global   (.data)

int main()
{
    if (y > 0) {
        x = y + 1;
    } else {
        x = y << 1;
    }
}
