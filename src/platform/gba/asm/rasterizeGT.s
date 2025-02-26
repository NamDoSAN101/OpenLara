#include "common_asm.inc"

pixel   .req r0
L       .req r1
R       .req r2

Lh      .req r3
Rh      .req r4

Lx      .req r5
Rx      .req r6

Lg      .req r7
Rg      .req r8

Lt      .req r9
Rt      .req r10

tmp     .req r11
N       .req r12

TILE    .req lr

h       .req N

LMAP    .req tmp

Ldx     .req h
Rdx     .req h

Ldg     .req h
Rdg     .req h

Ldt     .req h
Rdt     .req h

indexA  .req Lh
indexB  .req Rh

Rxy     .req tmp
Ry2     .req Rh
Lxy     .req tmp
Ly2     .req Lh

inv     .req Lh
DIVLUT  .req N
DIVLUTi .req tmp

ptr     .req Lx
width   .req Rx

g       .req Lg
dgdx    .req Rg

t       .req Lt
dtdx    .req Rt

duv     .req R
du      .req L
dv      .req R

Lduv    .req N
Ldu     .req TILE
Ldv     .req N

Rduv    .req N
Rdu     .req TILE
Rdv     .req N

Rti     .req tmp
Rgi     .req tmp

sLdx    .req L
sLdg    .req R
sLdt    .req Lh
sRdx    .req Rh
sRdg    .req tmp
sRdt    .req N    // not used in ldm due h collision

SP_LDX = 0
SP_LDG = 4
SP_LDT = 8
SP_RDX = 12
SP_RDG = 16
SP_RDT = 20

.macro PUT_PIXELS
    bic LMAP, g, #255
    add g, dgdx

    tex indexA, t
    lit indexA

#ifndef TEX_2PX
    add t, dtdx

    tex indexB, t
    lit indexB

    add t, dtdx

    orr indexA, indexB, lsl #8
    strh indexA, [ptr], #2
#else
    add t, dtdx, lsl #1

    //orr indexA, indexA, lsl #8
    strb indexA, [ptr], #2  // writing a byte to GBA VRAM will write a half word for free
#endif
.endm

.global rasterizeGT_asm
rasterizeGT_asm:
    stmfd sp!, {r4-r11, lr}
    sub sp, #24 // reserve stack space for [Ldx, Ldg, Ldt, Rdx, Rdg, Rdt]

    mov Lh, #0                      // Lh = 0
    mov Rh, #0                      // Rh = 0

.loop:

    cmp Lh, #0
      bne .calc_left_end        // if (Lh != 0) end with left

    .calc_left_start:
        ldrsb N, [L, #VERTEX_PREV]  // N = L + L->prev
        add N, L, N, lsl #VERTEX_SIZEOF_SHIFT
        ldr Lxy, [L, #VERTEX_X]     // Lxy = (L->v.y << 16) | (L->v.x)
        ldrsh Ly2, [N, #VERTEX_Y]   // Ly2 = N->v.y

        subs Lh, Ly2, Lxy, asr #16  // Lh = N->v.y - L->v.y
          blt .exit                 // if (Lh < 0) return
          ldrneb Lg, [L, #VERTEX_G] // Lg = L->v.g
          ldrne Lt, [L, #VERTEX_T]  // Lt = L->t
          mov L, N                  // L = N
          beq .calc_left_start

        lsl Lx, Lxy, #16            // Lx = L->v.x << 16
        lsl Lg, #8                  // Lg <<= 8
        cmp Lh, #1                  // if (Lh <= 1) skip Ldx calc
          beq .calc_left_end

        lsl tmp, Lh, #1
        mov DIVLUT, #DIVLUT_ADDR
        ldrh tmp, [DIVLUT, tmp]     // tmp = FixedInvU(Lh)

        ldrsh Ldx, [L, #VERTEX_X]
        sub Ldx, Lx, asr #16
        mul Ldx, tmp                // Ldx = tmp * (N->v.x - Lx)
        str Ldx, [sp, #SP_LDX]      // store Ldx to stack

        ldrb Ldg, [L, #VERTEX_G]
        sub Ldg, Lg, lsr #8
        mul Ldg, tmp                // Ldg = tmp * (N->v.g - Lg)
        asr Ldg, #8                 // 8-bit for fractional part
        str Ldg, [sp, #SP_LDG]      // store Ldg to stack

        ldr Lduv, [L, #VERTEX_T]
        sub Lduv, Lt                // Lduv = N->v.t - Lt
        asr Ldu, Lduv, #16
        mul Ldu, tmp                // Rdu = tmp * int16(Lduv >> 16)
        lsl Ldv, Lduv, #16
        asr Ldv, #16
        mul Ldv, tmp                // Rdv = tmp * int16(Lduv)
        lsr Ldu, #16
        lsl Ldu, #16
        orr Ldt, Ldu, Ldv, lsr #16  // Ldt = (Rdu & 0xFFFF0000) | (Rdv >> 16)
        str Ldt, [sp, #SP_LDT]      // store Ldt to stack
    .calc_left_end:

    cmp Rh, #0
      bne .calc_right_end       // if (Rh != 0) end with right

    .calc_right_start:
        ldrsb N, [R, #VERTEX_NEXT]  // N = R + R->next
        add N, R, N, lsl #VERTEX_SIZEOF_SHIFT
        ldr Rxy, [R, #VERTEX_X]     // Rxy = (R->v.y << 16) | (R->v.x)
        ldrsh Ry2, [N, #VERTEX_Y]   // Ry2 = N->v.y

        subs Rh, Ry2, Rxy, asr #16  // Rh = N->v.y - R->v.y
          blt .exit                 // if (Rh < 0) return
          ldrneb Rg, [R, #VERTEX_G] // Rg = R->v.g
          ldrne Rt, [R, #VERTEX_T]  // Rt = R->t
          mov R, N                  // R = N
          beq .calc_right_start

        lsl Rx, Rxy, #16            // Rx = R->v.x << 16
        lsl Rg, #8                  // Rg <<= 8
        cmp Rh, #1                  // if (Rh <= 1) skip Rdx calc
          beq .calc_right_end

        lsl tmp, Rh, #1
        mov DIVLUT, #DIVLUT_ADDR
        ldrh tmp, [DIVLUT, tmp]     // tmp = FixedInvU(Rh)

        ldrsh Rdx, [R, #VERTEX_X]
        sub Rdx, Rx, asr #16
        mul Rdx, tmp                // Rdx = tmp * (N->v.x - Rx)
        str Rdx, [sp, #SP_RDX]      // store Rdx to stack

        ldrb Rdg, [R, #VERTEX_G]
        sub Rdg, Rg, lsr #8
        mul Rdg, tmp                // Rdg = tmp * (N->v.g - Rg)
        asr Rdg, #8                 // 8-bit for fractional part
        str Rdg, [sp, #SP_RDG]      // store Ldg to stack

        ldr Rduv, [R, #VERTEX_T]
        sub Rduv, Rt                // Rduv = N->v.t - Rt
        asr Rdu, Rduv, #16
        mul Rdu, tmp                // Rdu = tmp * int16(Rduv >> 16)
        lsl Rdv, Rduv, #16
        asr Rdv, #16
        mul Rdv, tmp                // Rdv = tmp * int16(Rduv)
        lsr Rdu, #16
        lsl Rdu, #16
        orr Rdt, Rdu, Rdv, lsr #16  // Rdt = (Rdu & 0xFFFF0000) | (Rdv >> 16)
        str Rdt, [sp, #SP_RDT]      // store Rdt to stack
    .calc_right_end:

    orr Lg, #LMAP_ADDR
    orr Rg, #LMAP_ADDR

    cmp Rh, Lh              // if (Rh < Lh)
      movlt h, Rh           //      h = Rh
      movge h, Lh           // else h = Lh
    sub Lh, h               // Lh -= h
    sub Rh, h               // Rh -= h

    ldr TILE, =gTile
    ldr TILE, [TILE]

    stmfd sp!, {L,R,Lh,Rh}  // sp-16

.scanline_start:
    stmfd sp!, {Lx,Rx,Lg,Rg,Lt,Rt}  // sp-24

    asr tmp, Lx, #16                // x1 = (Lx >> 16)
    rsbs width, tmp, Rx, asr #16    // width = (Rx >> 16) - x1
      ble .scanline_end             // if (width <= 0) go next scanline

    add ptr, pixel, tmp             // ptr = pixel + x1

    mov DIVLUTi, #DIVLUT_ADDR
    lsl inv, width, #1
    ldrh inv, [DIVLUTi, inv]        // inv = FixedInvU(width)

    sub dgdx, Rg, Lg                // dgdx = Rg - Lg
    mul dgdx, inv                   // dgdx *= FixedInvU(width)
    asr dgdx, #15                   // dgdx >>= 15
    // g == Lg (alias)

    sub duv, Rt, Lt                 // duv = Rt - Lt
    asr du, duv, #16
    mul du, inv                     // du = inv * int16(duv >> 16)
    lsl dv, duv, #16
    asr dv, #16
    mul dv, inv                     // dv = inv * int16(duv)
    lsr du, #16
    lsl du, #16
    orr dtdx, du, dv, lsr #16       // dtdx = (du & 0xFFFF0000) | (dv >> 16)
    // t == Lt (alias)

    // 2 bytes alignment (VRAM write requirement)
.align_left:
    tst ptr, #1                   // if (ptr & 1)
      beq .align_right
    ldrb indexB, [ptr, #-1]!      // read pal index from VRAM (byte)

    bic LMAP, g, #255
    add g, dgdx, asr #1

    and indexA, t, #0xFF00
    orr indexA, t, lsr #24        // res = (t & 0xFF00) | (t >> 24)
    ldrb indexA, [TILE, indexA]
    ldrb indexA, [LMAP, indexA]

    orr indexB, indexA, lsl #8
    strh indexB, [ptr], #2
    add t, dtdx

    subs width, #1              // width--
      beq .scanline_end         // if (width == 0)

.align_right:
    tst width, #1
      beq .align_block_4px
    ldrb indexB, [ptr, width]

    subs width, #1              // width--

    mla Rti, width, dtdx, t     // Rti = width * dtdx + t
    and indexA, Rti, #0xFF00
    orr indexA, Rti, lsr #24    // res = (t & 0xFF00) | (t >> 24)
    ldrb indexA, [TILE, indexA]

    asr Rgi, dgdx, #1
    mla Rgi, width, Rgi, g      // Rgi = width * (dgdx / 2) + g
    bic LMAP, Rgi, #255

    ldrb indexA, [LMAP, indexA]

    orr indexB, indexA, indexB, lsl #8
    strh indexB, [ptr, width]

      beq .scanline_end         // if (width == 0)

.align_block_4px:
    tst width, #2
      beq .align_block_8px

    PUT_PIXELS

    subs width, #2
      beq .scanline_end

.align_block_8px:
    tst width, #4
      beq .scanline_block_8px

    PUT_PIXELS
    PUT_PIXELS

    subs width, #4
      beq .scanline_end

.scanline_block_8px:
    PUT_PIXELS
    PUT_PIXELS
    PUT_PIXELS
    PUT_PIXELS

    subs width, #8
      bne .scanline_block_8px

.scanline_end:
    ldmfd sp!, {Lx,Rx,Lg,Rg,Lt,Rt}  // sp+24

    add tmp, sp, #16
    ldmia tmp, {sLdx, sLdg, sLdt, sRdx, sRdg}

    add Lx, sLdx
    add Lg, sLdg
    add Lt, sLdt
    add Rx, sRdx
    add Rg, sRdg

    ldr tmp, [sp, #(SP_RDT + 16)]
    add Rt, tmp                     // Rt += Rdt from stack

    add pixel, #FRAME_WIDTH         // pixel += FRAME_WIDTH (240)

    subs h, #1
      bne .scanline_start

    ldmfd sp!, {L,R,Lh,Rh}          // sp+16
    b .loop

.exit:
    add sp, #24                 // revert reserved space for [Ldx, Ldg, Ldt, Rdx, Rdg, Rdt]
    ldmfd sp!, {r4-r11, pc}