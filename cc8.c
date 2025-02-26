/**
 * TODO: sync VM to 500 hz
 *       decrement timers at 60 hz
 *       sound timer audio
 */

#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define DISPLAY_BYTES_PER_LINE 8
#define DISPLAY_COLUMNS (DISPLAY_BYTES_PER_LINE * 8)
#define DISPLAY_LINES 32
#define DISPLAY_SIZE (DISPLAY_BYTES_PER_LINE * DISPLAY_LINES)
#define RAM_SIZE 4096
#define STACK_SIZE 16

typedef enum {
    /*CC8_KEY_NONE = 0,*/
    /*CC8_KEY_1 = 0x0001, CC8_KEY_2 = 0x0002, CC8_KEY_3 = 0x0004, CC8_KEY_C = 0x0008,*/
    /*CC8_KEY_4 = 0x0010, CC8_KEY_5 = 0x0020, CC8_KEY_6 = 0x0040, CC8_KEY_D = 0x0080,*/
    /*CC8_KEY_7 = 0x0100, CC8_KEY_8 = 0x0200, CC8_KEY_9 = 0x0400, CC8_KEY_E = 0x0800,*/
    /*CC8_KEY_A = 0x1000, CC8_KEY_0 = 0x2000, CC8_KEY_B = 0x4000, CC8_KEY_F = 0x8000,*/
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
} State;

void cc8_load_default_font(State *state, uint16_t addr)
{
    uint16_t glyphs[] = {
        0xF0, 0x90, 0x90, 0x90, 0xF0, // 0
        0x20, 0x60, 0x20, 0x20, 0x70, // 1
        0xF0, 0x10, 0xF0, 0x80, 0xF0, // 2
        0xF0, 0x10, 0xF0, 0x10, 0xF0, // 3
        0x90, 0x90, 0xF0, 0x10, 0x10, // 4
        0xF0, 0x80, 0xF0, 0x10, 0xF0, // 5
        0xF0, 0x80, 0xF0, 0x90, 0xF0, // 6
        0xF0, 0x10, 0x20, 0x40, 0x40, // 7
        0xF0, 0x90, 0xF0, 0x90, 0xF0, // 8
        0xF0, 0x90, 0xF0, 0x10, 0xF0, // 9
        0xF0, 0x90, 0xF0, 0x90, 0x90, // A
        0xE0, 0x90, 0xE0, 0x90, 0xE0, // B
        0xF0, 0x80, 0x80, 0x80, 0xF0, // C
        0xE0, 0x90, 0x90, 0x90, 0xE0, // D
        0xF0, 0x80, 0xF0, 0x80, 0xF0, // E
        0xF0, 0x80, 0xF0, 0x80, 0x80, // F
    };
    
    for(size_t i = 0; i < sizeof(glyphs)/sizeof(glyphs[0]); ++i)
    {
        memset(state->memory + addr + i, glyphs[i], 1);
    }
}

void cc8_init(State *state)
{
    /*state->SP = malloc(sizeof(uint16_t) * STACK_SIZE);*/

    memset(state->memory, 0, RAM_SIZE);
    memset(state->framebuffer, 0, DISPLAY_SIZE);
    memset(state->V, 0, 0xF);

    state->SP = (uint16_t *) state->memory + (RAM_SIZE - (sizeof(uint16_t) * STACK_SIZE));

    state->key = CC8_KEY_NONE;
    state->keys = 0;

    cc8_load_default_font(state, 0x50);
    
    state->PC = 0x200;

    srand(time(NULL));
}

void cc8_tick_timers(State *state)
{
    if(state->DT > 0) --state->DT;
    if(state->ST > 0) --state->ST;
}

void cc8_set_key(State *state, Cc8_Key key)
{
    state->key = key;
    if(key != CC8_KEY_NONE) state->keys |= 1 << key;
}

void cc8_unset_key(State *state, Cc8_Key key)
{
    if(state->key == key) state->key = CC8_KEY_NONE;
    if(key != CC8_KEY_NONE) state->keys &= ~(1 << key);
}

bool cc8_cell_is_on_xy(State *state, uint8_t x, uint8_t y)
{
    uint8_t x_bytes = (x % DISPLAY_COLUMNS) / 8;
    uint8_t y_bytes = (y % DISPLAY_LINES) * DISPLAY_BYTES_PER_LINE;
    uint8_t x_bits  = (x % DISPLAY_COLUMNS) % 8;

    uint8_t mask = 0x80 >> x_bits;

    return (state->framebuffer[x_bytes + y_bytes] & mask) == mask;
}

void cc8_print_display(State *state)
{
    for(size_t y = 0; y < DISPLAY_LINES; ++y)
    {
        printf("%03zu: ", y);
        for(size_t x = 0; x < DISPLAY_COLUMNS; ++x)
        {
            printf(cc8_cell_is_on_xy(state, x, y) ? "#" : "-");
        }
        printf("\n");
    }
}

uint16_t cc8_fetch(State *state)
{
    /*uint16_t instruction = (state->memory[state->PC] << 8) | (state->memory[state->PC+1]);*/
    uint16_t instruction = (state->memory[state->PC]) | (state->memory[state->PC+1] << 8);  // reverse byte order
    state->PC+=2;
    return instruction;
}

uint16_t cc8_fetch_debug(State *state)
{
    uint16_t instruction = (state->memory[state->PC]) | (state->memory[state->PC + 1] << 8);
    printf("%03X:   %04X\n", state->PC, instruction);
    state->PC += 2;
    return instruction;
}

void cc8_stack_push(uint16_t **sp, uint16_t item)
{
    *((*sp)++) = item;
}

uint16_t cc8_stack_pop(uint16_t **sp)
{
    return *(--(*sp));
}

// OPS {{{

void cc8_cls(State *state)
{
    memset(state->framebuffer, 0, DISPLAY_SIZE);
}

void cc8_ret(State *state)
{
    state->PC = cc8_stack_pop(&state->SP);
}

void cc8_jp(State *state, uint16_t addr)
{
    state->PC = addr;
}

void cc8_call(State *state, uint16_t addr)
{
    cc8_stack_push(&state->SP, state->PC);
    state->PC = addr;
}

void cc8_se_rb(State *state, uint8_t reg, uint8_t byte)
{
    if(state->V[reg] == byte) state->PC += 2;
}

void cc8_sne_rb(State *state, uint8_t reg, uint8_t byte)
{
    if(state->V[reg] != byte) state->PC += 2;
}

void cc8_se_rr(State *state, uint8_t x, uint8_t y)
{
    if(state->V[x] == state->V[y]) state->PC += 2;
}

void cc8_ld_rb(State *state, uint8_t reg, uint8_t byte)
{
    state->V[reg] = byte;
}

void cc8_add_rb(State *state, uint8_t reg, uint8_t byte)
{
    state->V[reg] = state->V[reg] + byte;
}

void cc8_ld_rr(State *state, uint8_t x, uint8_t y)
{
    state->V[x] = state->V[y];
}

void cc8_or_rr(State *state, uint8_t x, uint8_t y)
{
    state->V[x] |= state->V[y];
}

void cc8_and_rr(State *state, uint8_t x, uint8_t y)
{
    state->V[x] &= state->V[y];
}

void cc8_xor_rr(State *state, uint8_t x, uint8_t y)
{
    state->V[x] ^= state->V[y];
}

void cc8_add_rr(State *state, uint8_t x, uint8_t y)
{
    uint8_t result = state->V[x] + state->V[y];
    uint16_t overflow_check = (uint16_t) state->V[x] + (uint16_t) state->V[y];
    if(result != overflow_check) state->V[0xF] = 1;
    else state->V[0xF] = 0;
    state->V[x] = result;
}

void cc8_sub_rr(State *state, uint8_t x, uint8_t y)
{
    if(state->V[x] > state->V[y]) state->V[0xF] = 1;
    else state->V[0xF] = 0;
    state->V[x] -= state->V[y];
}

void cc8_shr_rr(State *state, uint8_t x, uint8_t y)
{
    (void) y;
    if((state->V[x] | 1) == 1) state->V[0xF] = 1;
    else state->V[0xF] = 0;
    state->V[x] >>= 1;
}

void cc8_subn_rr(State *state, uint8_t x, uint8_t y)
{
    if(state->V[x] < state->V[y]) state->V[0xF] = 1;
    else state->V[0xF] = 0;
    state->V[x] = state->V[y] - state->V[x];
}

void cc8_shl_rr(State *state, uint8_t x, uint8_t y)
{
    (void) y;
    if((state->V[x] | 0x80) == 1) state->V[0xF] = 1;
    else state->V[0xF] = 0;
    state->V[x] <<= 1;
}

void cc8_sne_rr(State *state, uint8_t x, uint8_t y)
{
    if(state->V[x] != state->V[y]) state->PC += 2;
}

void cc8_ldi(State *state, uint16_t addr)
{
    state->I = addr;
}

void cc8_jpv0_a(State *state, uint16_t addr)
{
    state->PC = addr + state->V[0];
}

void cc8_rnd_rb(State *state, uint8_t reg, uint8_t byte)
{
    state->V[reg] = (rand() % 256) & byte;
}

void cc8_drw_rrn(State *state, uint8_t vx, uint8_t vy, uint8_t n)
{
    state->V[0xF] = 0;
    for(uint8_t i = 0; i < n; ++i)
    {
        for(size_t b = 0; b < 4; ++b)
        {
            uint8_t x = state->V[vx];
            uint8_t y = state->V[vy];

            uint8_t y_bytes = ((y + i) % DISPLAY_LINES) * DISPLAY_BYTES_PER_LINE;
            uint8_t x_bytes = ((x + b) % DISPLAY_COLUMNS) / 8;
            /*uint8_t x_bits  = ((x + b) % DISPLAY_COLUMNS) % 8;*/

            uint8_t byte = x_bytes + y_bytes;

            uint8_t bit = (x + b) % 8;
            uint8_t mask = 0x80 >> bit;

            /*uint8_t x1 = ((x + b) % DISPLAY_COLUMNS) / 8;*/
            /*uint8_t y1 = (y + i) % DISPLAY_LINES;*/

            bool was_on = false;
            if((state->framebuffer[byte] & mask) == mask) was_on = true;

            uint8_t px0 = (state->framebuffer[byte] ^ (((state->memory[state->I + i] << b) & 0x80) >> bit)) & mask;
            uint8_t px1 = state->framebuffer[byte] & ~mask;
            uint8_t px2 = px0 | px1;

            if(px2 != mask) state->V[0xF] = 1;

            state->framebuffer[byte] = px2;

            if(was_on && ((state->framebuffer[byte] & mask) == mask)) state->V[0xF] = 1;
            else state->V[0xF] = 0;
        }
    }
}

void cc8_skp(State *state, uint8_t reg)
{
    if(state->keys & (1 << state->V[reg]))
    {
        state->PC += 2;
    }
}

void cc8_sknp(State *state, uint8_t reg)
{
    if(!(state->keys & (1 << state->V[reg])))
    {
        state->PC += 2;
    }
}

void cc8_ld_r_dt(State *state, uint8_t x)
{
    state->V[x] = state->DT;
}

void cc8_ldk_r(State *state, uint8_t x)
{
    while(state->key == CC8_KEY_NONE);
    state->V[x] = state->key;
}

void cc8_lddt_r(State *state, uint8_t x)
{
    state->DT = state->V[x];
}

void cc8_ldst_r(State *state, uint8_t x)
{
    state->ST = state->V[x];
}

void cc8_addi_r(State *state, uint8_t x)
{
    state->I += state->V[x];
}

void cc8_ldf_r(State *state, uint8_t x)
{
    state->I = 0x50 + (state->V[x] * 5);
}

void cc8_ldb_r(State *state, uint8_t x)
{
    uint8_t hundreds = (state->V[x] / 100) & 0xF;
    uint8_t tens = ((state->V[x] - hundreds) / 10) & 0xF;
    uint8_t ones = ((state->V[x] - hundreds - tens) / 1) & 0xF;

    state->memory[state->I + 0] = hundreds;
    state->memory[state->I + 1] = tens;
    state->memory[state->I + 2] = ones;
}

void cc8_ldi_r(State *state, uint8_t x)
{
    for(size_t i = 0; i < x; ++i)
    {
        state->memory[state->I + i] = state->V[i];
    }
}

void cc8_ld_r(State *state, uint8_t x)
{
    for(size_t i = 0; i < x; ++i)
    {
        state->V[i] = state->memory[state->I + i];
    }
}

// OPS }}}

void cc8_execute(State *state, uint16_t instruction)
{
    uint16_t x = (instruction & 0x0F00) >> 8;
    uint16_t y = (instruction & 0x00F0) >> 4;
    uint16_t n = (instruction & 0x000F) >> 0;
    switch((instruction & 0xF000) >> 12)
    {
        case 0: {
            if(x != 0) break;
            if(y != 0xE) break;

            if(n == 0x0) cc8_cls(state);
            else if(n == 0xE) cc8_ret(state);
        } break;

        case 1: {
            uint16_t addr = x << 8 | y << 4 | n;
            cc8_jp(state, addr);
        } break;

        case 2: {
            uint16_t addr = x << 8 | y << 4 | n;
            cc8_call(state, addr);
        } break;

        case 3: {
            uint8_t byte = y << 4 | n;
            cc8_se_rb(state, x, byte);
        } break;

        case 4: {
            uint8_t byte = y << 4 | n;
            cc8_sne_rb(state, x, byte);
        } break;

        case 5: {
            if(n != 0) break;
            cc8_se_rr(state, x, y);
        } break;

        case 6: {
            uint8_t byte = y << 4 | n;
            cc8_ld_rb(state, x, byte);
        } break;

        case 7: {
            uint8_t byte = y << 4 | n;
            cc8_add_rb(state, x, byte);
        } break;

        case 8: {
            switch(n)
            {
                case 0x0: cc8_ld_rr(state, x, y); break;

                case 0x1: cc8_or_rr(state, x, y); break;
                case 0x2: cc8_and_rr(state, x, y); break;
                case 0x3: cc8_xor_rr(state, x, y); break;

                case 0x4: cc8_add_rr(state, x, y); break;
                case 0x5: cc8_sub_rr(state, x, y); break;

                case 0x6: cc8_shr_rr(state, x, y); break;

                case 0x7: cc8_subn_rr(state, x, y); break;

                case 0xE: cc8_shl_rr(state, x, y); break;

            }
        } break;

        case 9: {
            if(n != 0) break;
            cc8_sne_rr(state, x, y);
        } break;

        case 0xA: {
            uint16_t addr = x << 8 | y << 4 | n;
            cc8_ldi(state, addr);
        } break;

        case 0xB: {
            uint16_t addr = x << 8 | y << 4 | n;
            cc8_jpv0_a(state, addr);
        } break;

        case 0xC: {
            uint8_t byte = y << 4 | n;
            cc8_rnd_rb(state, x, byte);
        } break;

        case 0xD: cc8_drw_rrn(state, x, y, n); break;

        case 0xE: {
            if(y == 9 && n == 0xE)
            {
                cc8_skp(state, x);
            }
            else if(y == 0xA && n == 1)
            {
                cc8_sknp(state, x);
            }
            else break;
        } break;

        case 0xF: {
            switch(y << 4 | n)
            {
                case 0x07: cc8_ld_r_dt(state, x); break;
                case 0x0A: cc8_ldk_r(state, x); break;
                case 0x15: cc8_lddt_r(state, x); break;
                case 0x18: cc8_ldst_r(state, x); break;
                case 0x1E: cc8_addi_r(state, x); break;
                case 0x29: cc8_ldf_r(state, x); break;
                case 0x33: cc8_ldb_r(state, x); break;
                case 0x55: cc8_ldi_r(state, x); break;
                case 0x65: cc8_ld_r(state, x); break;
            }
        } break;

        default: {
            fprintf(stderr, "INVALID INSTRUCTION: %X\n", instruction);
            exit(EXIT_FAILURE);
        } break;
    }
}
