#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>  // Added for size_t

#define VGA_WIDTH 80
#define VGA_HEIGHT 25
#define VGA_ADDRESS 0xB8000
#define WHITE_ON_BLACK 0x0F
#define BLUE_ON_BLACK 0x01
#define PROMPT_COLOR WHITE_ON_BLACK

// Filesystem constants
#define MAX_FILES 16
#define MAX_FILE_SIZE 1024
#define MAX_FILENAME_LEN 12

volatile uint16_t* vga_buffer = (volatile uint16_t*)VGA_ADDRESS;
uint8_t cursor_x = 0;
uint8_t cursor_y = 0;
char input_buffer[128];
uint8_t input_pos = 0;
bool show_cursor = true;

// File structure definition
typedef struct {
    char name[MAX_FILENAME_LEN];
    char content[MAX_FILE_SIZE];
    uint16_t size;
    bool exists;
} File;

// Simple filesystem structure
typedef struct {
    File files[MAX_FILES];
    uint8_t file_count;
} Filesystem;

// Global filesystem instance
Filesystem fs;

// Forward declarations
void print(const char* str);
void putchar(char c);
void clear_screen();
void show_prompt();
bool create_file(const char* name, const char* content);

// String function implementations
int strcmp(const char* a, const char* b) {
    while (*a && *b && *a == *b) { a++; b++; }
    return *(unsigned char*)a - *(unsigned char*)b;
}

int strncmp(const char* a, const char* b, unsigned int n) {
    while (n-- && *a && *b && *a == *b) { a++; b++; }
    return n ? *(unsigned char*)a - *(unsigned char*)b : 0;
}

size_t strlen(const char* str) {
    const char* s = str;
    while (*s) s++;
    return s - str;
}

char* strcpy(char* dest, const char* src) {
    char* orig_dest = dest;
    while ((*dest++ = *src++));
    return orig_dest;
}

char* strchr(const char* s, int c) {
    while (*s != (char)c) {
        if (!*s++) return 0;
    }
    return (char*)s;
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
    print("  clear          - Clear screen\n");
    print("  echo text      - Print text\n");
    print("  minifetch      - System info\n");
    print("  help           - Show help\n");
    print("  ls             - List files\n");
    print("  cat filename   - Show file contents\n");
    print("  create f c     - Create file f with content c\n\n");
}

void minifetch() {
    print("\n   /\\_/\\    Foxix OS\n");
    print("  ( o.o )   Version 1.0\n");
    print("   > ^ <    CPU: i386\n");
    print("            MEM: 640K\n\n");
}

bool create_file(const char* name, const char* content) {
    if (fs.file_count >= MAX_FILES) {
        print("\nError: Maximum files reached\n");
        return false;
    }

    if (strlen(name) >= MAX_FILENAME_LEN) {
        print("\nError: Filename too long\n");
        return false;
    }

    for (int i = 0; i < MAX_FILES; i++) {
        if (fs.files[i].exists && strcmp(fs.files[i].name, name) == 0) {
            print("\nError: File already exists\n");
            return false;
        }
    }

    // Find empty slot
    for (int i = 0; i < MAX_FILES; i++) {
        if (!fs.files[i].exists) {
            strcpy(fs.files[i].name, name);
            strcpy(fs.files[i].content, content);
            fs.files[i].size = strlen(content);
            fs.files[i].exists = true;
            fs.file_count++;
            return true;
        }
    }

    return false;
}

void init_filesystem() {
    fs.file_count = 0;
    for (int i = 0; i < MAX_FILES; i++) {
        fs.files[i].exists = false;
        fs.files[i].size = 0;
        fs.files[i].name[0] = '\0';
    }
}

// List all files
void list_files() {
    print("\nFiles:\n");
    print("------\n");

    for (int i = 0; i < MAX_FILES; i++) {
        if (fs.files[i].exists) {
            print(fs.files[i].name);
            print("\n");
        }
    }
    print("\n");
}

// Display file contents
void display_file(const char* name) {
    for (int i = 0; i < MAX_FILES; i++) {
        if (fs.files[i].exists && strcmp(fs.files[i].name, name) == 0) {
            print("\n");
            print(fs.files[i].content);
            print("\n");
            return;
        }
    }
    print("\nError: File not found\n");
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
    else if (strcmp(input_buffer, "ls") == 0) {
        list_files();
    }
    else if (strncmp(input_buffer, "cat ", 4) == 0) {
        display_file(input_buffer + 4);
    }
    else if (strncmp(input_buffer, "create ", 7) == 0) {
        // Simple create command: create filename content
        char* space = strchr(input_buffer + 7, ' ');
        if (space) {
            *space = '\0'; // Split filename and content
            create_file(input_buffer + 7, space + 1);
        } else {
            print("\nUsage: create filename content\n");
        }
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

    // Initialize filesystem
    init_filesystem();

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
