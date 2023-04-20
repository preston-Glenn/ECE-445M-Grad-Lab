#include "io.h"

void main()
{
    gpio_pull(4, 0);
    gpio_set(4, 1);
    uart_init();
    uart_writeText("Hello world!\n");
    while (1);
}