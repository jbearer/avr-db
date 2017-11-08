/**
 * Simple example program demonstrating simple branching and arithmetic instructions.
 */

static int x;       // Uninitialized global (.bss)
static int y = 1;   // Initialized global   (.data)

int main()
{
    if (y > 0) {
        x = y + 1;
    } else {
        x = y << 1;
    }
}
