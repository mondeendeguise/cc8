#include "cc8_ops.h"
#include "cc8.h"

#include <stdint.h>
#include <stdlib.h>
#include <string.h>

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
