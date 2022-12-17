
// 16 single cycle instructions = 1us at 16Mhz
void delay_1us()
{
    __asm__ __volatile__(
        "nop"
        "\n\t"
        "nop"
        "\n\t"
        "nop"
        "\n\t"
        "nop"
        "\n\t"
        "nop"
        "\n\t"
        "nop"
        "\n\t"
        "nop"
        "\n\t"
        "nop"
        "\n\t"
        "nop"
        "\n\t"
        "nop"
        "\n\t"
        "nop"
        "\n\t"
        "nop"
        "\n\t"
        "nop"
        "\n\t"
        "nop"
        "\n\t"
        "nop"
        "\n\t"
        "nop");
}
