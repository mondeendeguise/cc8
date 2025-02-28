const size_t abc_size = 36;
uint16_t abc[] = {
    0x00E0,   // CLS

    0x6100,   // V1 = 0
    0x6200,   // V2 = 0
    0x6300,   // V3 = 0
    0x6440,   // V4 = 64

    // OFFSET = 0x20A
    0xF329,   // I = FONT(V[3])
    0xD125,   // DRAW 5 (V[1], V[2])

    0x7105,   // INC V[1] by 5
    0x7301,   // INC V[3] by 1

    0x8001,   // V[0] = V[1]
    0x8044,   // V[0] = V[4] - V[0]
    0x4F01,   // if !(V[0] < 0) skip
    0x1222,   // JMP halt

    0x65FF,   // V[5] = 255
    0x8054,   // V[0] = V[0] + V[5]
    0x4F00,   // if V[0] == 0 skip
    0x120A,   // JMP top

    // OFFSET = 0x222
    0x1222,   // JMP halt

};
