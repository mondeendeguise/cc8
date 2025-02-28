#ifndef CC8_H_
#define CC8_H_

#include <stdbool.h>
#include <stdint.h>

#define DISPLAY_BYTES_PER_LINE 8
#define DISPLAY_COLUMNS (DISPLAY_BYTES_PER_LINE * 8)
#define DISPLAY_LINES 32
#define DISPLAY_SIZE (DISPLAY_BYTES_PER_LINE * DISPLAY_LINES)
#define RAM_SIZE 4096
#define STACK_SIZE 16

typedef enum {
    CC8_KEY_NONE  = -1,

    CC8_KEY_0     = 0x0,
    CC8_KEY_1     = 0x1,
    CC8_KEY_2     = 0x2,
    CC8_KEY_3     = 0x3,
    CC8_KEY_4     = 0x4,
    CC8_KEY_5     = 0x5,
    CC8_KEY_6     = 0x6,
    CC8_KEY_7     = 0x7,
    CC8_KEY_8     = 0x8,
    CC8_KEY_9     = 0x9,
    CC8_KEY_A     = 0xA,
    CC8_KEY_B     = 0xB,
    CC8_KEY_C     = 0xC,
    CC8_KEY_D     = 0xD,
    CC8_KEY_E     = 0xE,
    CC8_KEY_F     = 0xF,
} Cc8_Key;

typedef struct {
    uint8_t framebuffer[DISPLAY_SIZE]; // 64 * 32 binary display

    uint8_t memory[RAM_SIZE];
    uint8_t V[0xF]; // general purpose registers
    uint8_t DT;     // delay timer
    uint8_t ST;     // sound timer
    uint16_t I;     // only 12 lowest bits are used

    uint16_t PC;    // program counter
    uint16_t *SP;     // stack pointer

    Cc8_Key key;
    uint16_t keys;
} Cc8_Context;

void cc8_init(Cc8_Context *ctx);

void cc8_print_display(Cc8_Context *ctx);

void cc8_tick_timers(Cc8_Context *ctx);

bool cc8_cell_is_on_xy(Cc8_Context *ctx, uint8_t x, uint8_t y);

void cc8_set_key(Cc8_Context *ctx, Cc8_Key key);
void cc8_unset_key(Cc8_Context *ctx, Cc8_Key key);

void cc8_stack_push(uint16_t **sp, uint16_t item);
uint16_t cc8_stack_pop(uint16_t **sp);

uint16_t cc8_fetch(Cc8_Context *ctx);
uint16_t cc8_fetch_debug(Cc8_Context *ctx);

void cc8_execute(Cc8_Context *ctx, uint16_t instruction);

bool cc8_read_file(Cc8_Context *ctx, const char *file);

#endif // CC8_H_
