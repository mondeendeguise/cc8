/**
 * TODO: sync VM to 500 hz
 *       decrement timers at 60 hz
 *       sound timer audio
 */

#include "cc8.h"

#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

void cc8_load_default_font(Cc8_Context *ctx, uint16_t addr)
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
        memset(ctx->memory + addr + i, glyphs[i], 1);
    }
}

void cc8_init(Cc8_Context *ctx)
{
    /*ctx->SP = malloc(sizeof(uint16_t) * STACK_SIZE);*/

    memset(ctx->memory, 0, RAM_SIZE);
    memset(ctx->framebuffer, 0, DISPLAY_SIZE);
    memset(ctx->V, 0, 0xF);

    ctx->SP = (uint16_t *) ctx->memory + (RAM_SIZE - (sizeof(uint16_t) * STACK_SIZE));

    ctx->key = CC8_KEY_NONE;
    ctx->keys = 0;

    cc8_load_default_font(ctx, 0x50);
    
    ctx->PC = 0x200;

    srand(time(NULL));
}

void cc8_tick_timers(Cc8_Context *ctx)
{
    if(ctx->DT > 0) --ctx->DT;
    if(ctx->ST > 0) --ctx->ST;
}

void cc8_set_key(Cc8_Context *ctx, Cc8_Key key)
{
    ctx->key = key;
    if(key != CC8_KEY_NONE) ctx->keys |= 1 << key;
}

void cc8_unset_key(Cc8_Context *ctx, Cc8_Key key)
{
    if(ctx->key == key) ctx->key = CC8_KEY_NONE;
    if(key != CC8_KEY_NONE) ctx->keys &= ~(1 << key);
}

bool cc8_cell_is_on_xy(Cc8_Context *ctx, uint8_t x, uint8_t y)
{
    uint8_t x_bytes = (x % DISPLAY_COLUMNS) / 8;
    uint8_t y_bytes = (y % DISPLAY_LINES) * DISPLAY_BYTES_PER_LINE;
    uint8_t x_bits  = (x % DISPLAY_COLUMNS) % 8;

    uint8_t mask = 0x80 >> x_bits;

    return (ctx->framebuffer[x_bytes + y_bytes] & mask) == mask;
}

void cc8_print_display(Cc8_Context *ctx)
{
    for(size_t y = 0; y < DISPLAY_LINES; ++y)
    {
        printf("%03zu: ", y);
        for(size_t x = 0; x < DISPLAY_COLUMNS; ++x)
        {
            printf(cc8_cell_is_on_xy(ctx, x, y) ? "#" : "-");
        }
        printf("\n");
    }
}

uint16_t cc8_fetch(Cc8_Context *ctx)
{
    /*uint16_t instruction = (ctx->memory[ctx->PC] << 8) | (ctx->memory[ctx->PC+1]);*/
    uint16_t instruction = (ctx->memory[ctx->PC]) | (ctx->memory[ctx->PC+1] << 8);  // reverse byte order
    ctx->PC+=2;
    return instruction;
}

uint16_t cc8_fetch_debug(Cc8_Context *ctx)
{
    uint16_t instruction = (ctx->memory[ctx->PC]) | (ctx->memory[ctx->PC + 1] << 8);
    printf("%03X:   %04X\n", ctx->PC, instruction);
    ctx->PC += 2;
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

void cc8_cls(Cc8_Context *ctx)
{
    memset(ctx->framebuffer, 0, DISPLAY_SIZE);
}

void cc8_ret(Cc8_Context *ctx)
{
    ctx->PC = cc8_stack_pop(&ctx->SP);
}

void cc8_jp(Cc8_Context *ctx, uint16_t addr)
{
    ctx->PC = addr;
}

void cc8_call(Cc8_Context *ctx, uint16_t addr)
{
    cc8_stack_push(&ctx->SP, ctx->PC);
    ctx->PC = addr;
}

void cc8_se_rb(Cc8_Context *ctx, uint8_t reg, uint8_t byte)
{
    if(ctx->V[reg] == byte) ctx->PC += 2;
}

void cc8_sne_rb(Cc8_Context *ctx, uint8_t reg, uint8_t byte)
{
    if(ctx->V[reg] != byte) ctx->PC += 2;
}

void cc8_se_rr(Cc8_Context *ctx, uint8_t x, uint8_t y)
{
    if(ctx->V[x] == ctx->V[y]) ctx->PC += 2;
}

void cc8_ld_rb(Cc8_Context *ctx, uint8_t reg, uint8_t byte)
{
    ctx->V[reg] = byte;
}

void cc8_add_rb(Cc8_Context *ctx, uint8_t reg, uint8_t byte)
{
    ctx->V[reg] = ctx->V[reg] + byte;
}

void cc8_ld_rr(Cc8_Context *ctx, uint8_t x, uint8_t y)
{
    ctx->V[x] = ctx->V[y];
}

void cc8_or_rr(Cc8_Context *ctx, uint8_t x, uint8_t y)
{
    ctx->V[x] |= ctx->V[y];
}

void cc8_and_rr(Cc8_Context *ctx, uint8_t x, uint8_t y)
{
    ctx->V[x] &= ctx->V[y];
}

void cc8_xor_rr(Cc8_Context *ctx, uint8_t x, uint8_t y)
{
    ctx->V[x] ^= ctx->V[y];
}

void cc8_add_rr(Cc8_Context *ctx, uint8_t x, uint8_t y)
{
    uint8_t result = ctx->V[x] + ctx->V[y];
    uint16_t overflow_check = (uint16_t) ctx->V[x] + (uint16_t) ctx->V[y];
    if(result != overflow_check) ctx->V[0xF] = 1;
    else ctx->V[0xF] = 0;
    ctx->V[x] = result;
}

void cc8_sub_rr(Cc8_Context *ctx, uint8_t x, uint8_t y)
{
    if(ctx->V[x] > ctx->V[y]) ctx->V[0xF] = 1;
    else ctx->V[0xF] = 0;
    ctx->V[x] -= ctx->V[y];
}

void cc8_shr_rr(Cc8_Context *ctx, uint8_t x, uint8_t y)
{
    (void) y;
    if((ctx->V[x] | 1) == 1) ctx->V[0xF] = 1;
    else ctx->V[0xF] = 0;
    ctx->V[x] >>= 1;
}

void cc8_subn_rr(Cc8_Context *ctx, uint8_t x, uint8_t y)
{
    if(ctx->V[x] < ctx->V[y]) ctx->V[0xF] = 1;
    else ctx->V[0xF] = 0;
    ctx->V[x] = ctx->V[y] - ctx->V[x];
}

void cc8_shl_rr(Cc8_Context *ctx, uint8_t x, uint8_t y)
{
    (void) y;
    if((ctx->V[x] | 0x80) == 1) ctx->V[0xF] = 1;
    else ctx->V[0xF] = 0;
    ctx->V[x] <<= 1;
}

void cc8_sne_rr(Cc8_Context *ctx, uint8_t x, uint8_t y)
{
    if(ctx->V[x] != ctx->V[y]) ctx->PC += 2;
}

void cc8_ldi(Cc8_Context *ctx, uint16_t addr)
{
    ctx->I = addr;
}

void cc8_jpv0_a(Cc8_Context *ctx, uint16_t addr)
{
    ctx->PC = addr + ctx->V[0];
}

void cc8_rnd_rb(Cc8_Context *ctx, uint8_t reg, uint8_t byte)
{
    ctx->V[reg] = (rand() % 256) & byte;
}

void cc8_drw_rrn(Cc8_Context *ctx, uint8_t vx, uint8_t vy, uint8_t n)
{
    ctx->V[0xF] = 0;
    for(uint8_t i = 0; i < n; ++i)
    {
        for(size_t b = 0; b < 4; ++b)
        {
            uint8_t x = ctx->V[vx];
            uint8_t y = ctx->V[vy];

            uint8_t y_bytes = ((y + i) % DISPLAY_LINES) * DISPLAY_BYTES_PER_LINE;
            uint8_t x_bytes = ((x + b) % DISPLAY_COLUMNS) / 8;
            /*uint8_t x_bits  = ((x + b) % DISPLAY_COLUMNS) % 8;*/

            uint8_t byte = x_bytes + y_bytes;

            uint8_t bit = (x + b) % 8;
            uint8_t mask = 0x80 >> bit;

            /*uint8_t x1 = ((x + b) % DISPLAY_COLUMNS) / 8;*/
            /*uint8_t y1 = (y + i) % DISPLAY_LINES;*/

            bool was_on = false;
            if((ctx->framebuffer[byte] & mask) == mask) was_on = true;

            uint8_t px0 = (ctx->framebuffer[byte] ^ (((ctx->memory[ctx->I + i] << b) & 0x80) >> bit)) & mask;
            uint8_t px1 = ctx->framebuffer[byte] & ~mask;
            uint8_t px2 = px0 | px1;

            if(px2 != mask) ctx->V[0xF] = 1;

            ctx->framebuffer[byte] = px2;

            if(was_on && ((ctx->framebuffer[byte] & mask) == mask)) ctx->V[0xF] = 1;
            else ctx->V[0xF] = 0;
        }
    }
}

void cc8_skp(Cc8_Context *ctx, uint8_t reg)
{
    if(ctx->keys & (1 << ctx->V[reg]))
    {
        ctx->PC += 2;
    }
}

void cc8_sknp(Cc8_Context *ctx, uint8_t reg)
{
    if(!(ctx->keys & (1 << ctx->V[reg])))
    {
        ctx->PC += 2;
    }
}

void cc8_ld_r_dt(Cc8_Context *ctx, uint8_t x)
{
    ctx->V[x] = ctx->DT;
}

void cc8_ldk_r(Cc8_Context *ctx, uint8_t x)
{
    while(ctx->key == CC8_KEY_NONE);
    ctx->V[x] = ctx->key;
}

void cc8_lddt_r(Cc8_Context *ctx, uint8_t x)
{
    ctx->DT = ctx->V[x];
}

void cc8_ldst_r(Cc8_Context *ctx, uint8_t x)
{
    ctx->ST = ctx->V[x];
}

void cc8_addi_r(Cc8_Context *ctx, uint8_t x)
{
    ctx->I += ctx->V[x];
}

void cc8_ldf_r(Cc8_Context *ctx, uint8_t x)
{
    ctx->I = 0x50 + (ctx->V[x] * 5);
}

void cc8_ldb_r(Cc8_Context *ctx, uint8_t x)
{
    uint8_t hundreds = (ctx->V[x] / 100) & 0xF;
    uint8_t tens = ((ctx->V[x] - hundreds) / 10) & 0xF;
    uint8_t ones = ((ctx->V[x] - hundreds - tens) / 1) & 0xF;

    ctx->memory[ctx->I + 0] = hundreds;
    ctx->memory[ctx->I + 1] = tens;
    ctx->memory[ctx->I + 2] = ones;
}

void cc8_ldi_r(Cc8_Context *ctx, uint8_t x)
{
    for(size_t i = 0; i < x; ++i)
    {
        ctx->memory[ctx->I + i] = ctx->V[i];
    }
}

void cc8_ld_r(Cc8_Context *ctx, uint8_t x)
{
    for(size_t i = 0; i < x; ++i)
    {
        ctx->V[i] = ctx->memory[ctx->I + i];
    }
}

// OPS }}}

void cc8_execute(Cc8_Context *ctx, uint16_t instruction)
{
    uint16_t x = (instruction & 0x0F00) >> 8;
    uint16_t y = (instruction & 0x00F0) >> 4;
    uint16_t n = (instruction & 0x000F) >> 0;
    switch((instruction & 0xF000) >> 12)
    {
        case 0: {
            if(x != 0) break;
            if(y != 0xE) break;

            if(n == 0x0) cc8_cls(ctx);
            else if(n == 0xE) cc8_ret(ctx);
        } break;

        case 1: {
            uint16_t addr = x << 8 | y << 4 | n;
            cc8_jp(ctx, addr);
        } break;

        case 2: {
            uint16_t addr = x << 8 | y << 4 | n;
            cc8_call(ctx, addr);
        } break;

        case 3: {
            uint8_t byte = y << 4 | n;
            cc8_se_rb(ctx, x, byte);
        } break;

        case 4: {
            uint8_t byte = y << 4 | n;
            cc8_sne_rb(ctx, x, byte);
        } break;

        case 5: {
            if(n != 0) break;
            cc8_se_rr(ctx, x, y);
        } break;

        case 6: {
            uint8_t byte = y << 4 | n;
            cc8_ld_rb(ctx, x, byte);
        } break;

        case 7: {
            uint8_t byte = y << 4 | n;
            cc8_add_rb(ctx, x, byte);
        } break;

        case 8: {
            switch(n)
            {
                case 0x0: cc8_ld_rr(ctx, x, y); break;

                case 0x1: cc8_or_rr(ctx, x, y); break;
                case 0x2: cc8_and_rr(ctx, x, y); break;
                case 0x3: cc8_xor_rr(ctx, x, y); break;

                case 0x4: cc8_add_rr(ctx, x, y); break;
                case 0x5: cc8_sub_rr(ctx, x, y); break;

                case 0x6: cc8_shr_rr(ctx, x, y); break;

                case 0x7: cc8_subn_rr(ctx, x, y); break;

                case 0xE: cc8_shl_rr(ctx, x, y); break;

            }
        } break;

        case 9: {
            if(n != 0) break;
            cc8_sne_rr(ctx, x, y);
        } break;

        case 0xA: {
            uint16_t addr = x << 8 | y << 4 | n;
            cc8_ldi(ctx, addr);
        } break;

        case 0xB: {
            uint16_t addr = x << 8 | y << 4 | n;
            cc8_jpv0_a(ctx, addr);
        } break;

        case 0xC: {
            uint8_t byte = y << 4 | n;
            cc8_rnd_rb(ctx, x, byte);
        } break;

        case 0xD: cc8_drw_rrn(ctx, x, y, n); break;

        case 0xE: {
            if(y == 9 && n == 0xE)
            {
                cc8_skp(ctx, x);
            }
            else if(y == 0xA && n == 1)
            {
                cc8_sknp(ctx, x);
            }
            else break;
        } break;

        case 0xF: {
            switch(y << 4 | n)
            {
                case 0x07: cc8_ld_r_dt(ctx, x); break;
                case 0x0A: cc8_ldk_r(ctx, x); break;
                case 0x15: cc8_lddt_r(ctx, x); break;
                case 0x18: cc8_ldst_r(ctx, x); break;
                case 0x1E: cc8_addi_r(ctx, x); break;
                case 0x29: cc8_ldf_r(ctx, x); break;
                case 0x33: cc8_ldb_r(ctx, x); break;
                case 0x55: cc8_ldi_r(ctx, x); break;
                case 0x65: cc8_ld_r(ctx, x); break;
            }
        } break;

        default: {
            fprintf(stderr, "INVALID INSTRUCTION: %X\n", instruction);
            exit(EXIT_FAILURE);
        } break;
    }
}
