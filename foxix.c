#include <stdint.h>
#include <stdbool.h>

#define VGA_WIDTH 80
#define VGA_HEIGHT 25
#define VGA_ADDRESS 0xB8000
#define WHITE_ON_BLACK 0x0F
#define BLUE_ON_BLACK 0x01
#define PROMPT_COLOR WHITE_ON_BLACK

volatile uint16_t* vga_buffer = (volatile uint16_t*)VGA_ADDRESS;
uint8_t cursor_x = 0;
uint8_t cursor_y = 0;
char input_buffer[128];
uint8_t input_pos = 0;
bool show_cursor = true;

int strcmp(const char* a, const char* b) {
    while (*a && *b && *a == *b) { a++; b++; }
    return *(unsigned char*)a - *(unsigned char*)b;
}

int strncmp(const char* a, const char* b, unsigned int n) {
    while (n-- && *a && *b && *a == *b) { a++; b++; }
    return n ? *(unsigned char*)a - *(unsigned char*)b : 0;
}

void clear_screen() {
    for (int y = 0; y < VGA_HEIGHT; y++) {
        for (int x = 0; x < VGA_WIDTH; x++) {
            vga_buffer[y * VGA_WIDTH + x] = ' ' | (PROMPT_COLOR << 8);
        }
    }
    cursor_x = cursor_y = input_pos = 0;
}

void scroll_screen() {
    for (int y = 1; y < VGA_HEIGHT; y++) {
        for (int x = 0; x < VGA_WIDTH; x++) {
            vga_buffer[(y-1)*VGA_WIDTH + x] = vga_buffer[y*VGA_WIDTH + x];
        }
    }
    for (int x = 0; x < VGA_WIDTH; x++) {
        vga_buffer[(VGA_HEIGHT-1)*VGA_WIDTH + x] = ' ' | (PROMPT_COLOR << 8);
    }
    cursor_y = VGA_HEIGHT - 1;
}

void update_cursor() {
    vga_buffer[cursor_y * VGA_WIDTH + cursor_x] =
    show_cursor ? '_' | (PROMPT_COLOR << 8)
    : ' ' | (PROMPT_COLOR << 8);
}

void putchar(char c) {
    vga_buffer[cursor_y * VGA_WIDTH + cursor_x] = ' ' | (PROMPT_COLOR << 8);
    if (c == '\n') {
        cursor_x = 0;
        if (++cursor_y >= VGA_HEIGHT) scroll_screen();
    } else {
        vga_buffer[cursor_y * VGA_WIDTH + cursor_x] = c | (PROMPT_COLOR << 8);
        if (++cursor_x >= VGA_WIDTH) {
            cursor_x = 0;
            if (++cursor_y >= VGA_HEIGHT) scroll_screen();
        }
    }
    update_cursor();
}

void print(const char* str) {
    while (*str) putchar(*str++);
}

void backspace() {
    if (input_pos > 0) {
        input_pos--;
        if (cursor_x == 0) {
            cursor_x = VGA_WIDTH - 1;
            cursor_y--;
        } else {
            cursor_x--;
        }
        vga_buffer[cursor_y * VGA_WIDTH + cursor_x] = ' ' | (PROMPT_COLOR << 8);
        update_cursor();
    }
}

void show_prompt() {
    print("foxix> ");
}

void echo_command(const char* text) {
    putchar('\b');
    while (*text) {
        putchar(*text++);
    }
    putchar('\n');
}

void show_help() {
    print("\nAvailable commands:\n");
    print("  clear     - Clear screen\n");
    print("  echo text - Print text\n");
    print("  minifetch - System info\n");
    print("  help      - Show help\n\n");
}

void minifetch() {
    print("\n   /\\_/\\    Foxix OS\n");
    print("  ( o.o )   Version 1.0\n");
    print("   > ^ <    CPU: i386\n");
    print("            MEM: 640K\n\n");
}

void process_command() {
    input_buffer[input_pos] = '\0';

    if (strcmp(input_buffer, "clear") == 0) {
        clear_screen();
    }
    else if (strncmp(input_buffer, "echo ", 5) == 0) {
        echo_command(input_buffer + 5);
    }
    else if (strcmp(input_buffer, "minifetch") == 0) {
        minifetch();
    }
    else if (strcmp(input_buffer, "help") == 0) {
        show_help();
    }
    else if (input_pos > 0) {
        print("\nUnknown command. Type 'help' for options.\n");
    }

    input_pos = 0;
    show_prompt();
}

uint8_t inb(uint16_t port) {
    uint8_t ret;
    asm volatile("inb %1, %0" : "=a"(ret) : "Nd"(port));
    return ret;
}

bool keyboard_input_available() {
    return inb(0x64) & 1;
}

char get_key() {
    while (!keyboard_input_available());
    uint8_t scancode = inb(0x60);
    static const char scancodes[] = {
        0, 0, '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '-', '=', '\b',
        '\t', 'q', 'w', 'e', 'r', 't', 'y', 'u', 'i', 'o', 'p', '[', ']', '\n',
        0, 'a', 's', 'd', 'f', 'g', 'h', 'j', 'k', 'l', ';', '\'', '`',
        0, '\\', 'z', 'x', 'c', 'v', 'b', 'n', 'm', ',', '.', '/', 0,
        '*', 0, ' ', 0
    };
    if (scancode < sizeof(scancodes)) {
        if (scancode & 0x80) return 0;
        return scancodes[scancode];
    }
    return 0;
}

void foxix_main() {
    clear_screen();
    print("Foxix Kernel\n");
    print("============\n\n");
    show_prompt();

    while (1) {
        char c = get_key();
        if (c == '\n' || c == '\r') {
            putchar('\n');
            process_command();
        }
        else if (c == '\b') {
            backspace();
        }
        else if (c >= 32 && c <= 126 && input_pos < sizeof(input_buffer)-1) {
            putchar(c);
            input_buffer[input_pos++] = c;
        }
    }
}
