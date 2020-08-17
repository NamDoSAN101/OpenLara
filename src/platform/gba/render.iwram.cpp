#include "common.h"

#define DIV_TABLE_SIZE 512

uint16 divTable[DIV_TABLE_SIZE];

#ifdef _WIN32
    uint8 fb[WIDTH * HEIGHT * 2];
#else
    uint32 fb = VRAM;
#endif

#define FixedInvS(x) ((x < 0) ? -divTable[abs(x)] : divTable[x])
#define FixedInvU(x) divTable[x]

#if defined(USE_MODE_5) || defined(_WIN32)
    extern uint16 palette[256];
#endif

extern uint8            lightmap[256 * 32];
extern const uint8*     tiles[15];
extern const Texture*   textures;

uint32 gVerticesCount = 0;
Vertex gVertices[MAX_VERTICES];

int32 gFacesCount = 0;
EWRAM_DATA Face gFaces[MAX_FACES];
EWRAM_DATA Face* gFacesSorted[MAX_FACES];

const uint8* curTile;
uint16 mipMask;

Rect clip;


const int16 sin_table[] = { // 1025
    0x0000, 0x0019, 0x0032, 0x004B, 0x0065, 0x007E, 0x0097, 0x00B0,
    0x00C9, 0x00E2, 0x00FB, 0x0114, 0x012E, 0x0147, 0x0160, 0x0179,
    0x0192, 0x01AB, 0x01C4, 0x01DD, 0x01F7, 0x0210, 0x0229, 0x0242,
    0x025B, 0x0274, 0x028D, 0x02A6, 0x02C0, 0x02D9, 0x02F2, 0x030B,
    0x0324, 0x033D, 0x0356, 0x036F, 0x0388, 0x03A1, 0x03BB, 0x03D4,
    0x03ED, 0x0406, 0x041F, 0x0438, 0x0451, 0x046A, 0x0483, 0x049C,
    0x04B5, 0x04CE, 0x04E7, 0x0500, 0x051A, 0x0533, 0x054C, 0x0565,
    0x057E, 0x0597, 0x05B0, 0x05C9, 0x05E2, 0x05FB, 0x0614, 0x062D,
    0x0646, 0x065F, 0x0678, 0x0691, 0x06AA, 0x06C3, 0x06DC, 0x06F5,
    0x070E, 0x0727, 0x0740, 0x0759, 0x0772, 0x078B, 0x07A4, 0x07BD,
    0x07D6, 0x07EF, 0x0807, 0x0820, 0x0839, 0x0852, 0x086B, 0x0884,
    0x089D, 0x08B6, 0x08CF, 0x08E8, 0x0901, 0x0919, 0x0932, 0x094B,
    0x0964, 0x097D, 0x0996, 0x09AF, 0x09C7, 0x09E0, 0x09F9, 0x0A12,
    0x0A2B, 0x0A44, 0x0A5C, 0x0A75, 0x0A8E, 0x0AA7, 0x0AC0, 0x0AD8,
    0x0AF1, 0x0B0A, 0x0B23, 0x0B3B, 0x0B54, 0x0B6D, 0x0B85, 0x0B9E,
    0x0BB7, 0x0BD0, 0x0BE8, 0x0C01, 0x0C1A, 0x0C32, 0x0C4B, 0x0C64,
    0x0C7C, 0x0C95, 0x0CAE, 0x0CC6, 0x0CDF, 0x0CF8, 0x0D10, 0x0D29,
    0x0D41, 0x0D5A, 0x0D72, 0x0D8B, 0x0DA4, 0x0DBC, 0x0DD5, 0x0DED,
    0x0E06, 0x0E1E, 0x0E37, 0x0E4F, 0x0E68, 0x0E80, 0x0E99, 0x0EB1,
    0x0ECA, 0x0EE2, 0x0EFB, 0x0F13, 0x0F2B, 0x0F44, 0x0F5C, 0x0F75,
    0x0F8D, 0x0FA5, 0x0FBE, 0x0FD6, 0x0FEE, 0x1007, 0x101F, 0x1037,
    0x1050, 0x1068, 0x1080, 0x1099, 0x10B1, 0x10C9, 0x10E1, 0x10FA,
    0x1112, 0x112A, 0x1142, 0x115A, 0x1173, 0x118B, 0x11A3, 0x11BB,
    0x11D3, 0x11EB, 0x1204, 0x121C, 0x1234, 0x124C, 0x1264, 0x127C,
    0x1294, 0x12AC, 0x12C4, 0x12DC, 0x12F4, 0x130C, 0x1324, 0x133C,
    0x1354, 0x136C, 0x1384, 0x139C, 0x13B4, 0x13CC, 0x13E4, 0x13FB,
    0x1413, 0x142B, 0x1443, 0x145B, 0x1473, 0x148B, 0x14A2, 0x14BA,
    0x14D2, 0x14EA, 0x1501, 0x1519, 0x1531, 0x1549, 0x1560, 0x1578,
    0x1590, 0x15A7, 0x15BF, 0x15D7, 0x15EE, 0x1606, 0x161D, 0x1635,
    0x164C, 0x1664, 0x167C, 0x1693, 0x16AB, 0x16C2, 0x16DA, 0x16F1,
    0x1709, 0x1720, 0x1737, 0x174F, 0x1766, 0x177E, 0x1795, 0x17AC,
    0x17C4, 0x17DB, 0x17F2, 0x180A, 0x1821, 0x1838, 0x184F, 0x1867,
    0x187E, 0x1895, 0x18AC, 0x18C3, 0x18DB, 0x18F2, 0x1909, 0x1920,
    0x1937, 0x194E, 0x1965, 0x197C, 0x1993, 0x19AA, 0x19C1, 0x19D8,
    0x19EF, 0x1A06, 0x1A1D, 0x1A34, 0x1A4B, 0x1A62, 0x1A79, 0x1A90,
    0x1AA7, 0x1ABE, 0x1AD4, 0x1AEB, 0x1B02, 0x1B19, 0x1B30, 0x1B46,
    0x1B5D, 0x1B74, 0x1B8A, 0x1BA1, 0x1BB8, 0x1BCE, 0x1BE5, 0x1BFC,
    0x1C12, 0x1C29, 0x1C3F, 0x1C56, 0x1C6C, 0x1C83, 0x1C99, 0x1CB0,
    0x1CC6, 0x1CDD, 0x1CF3, 0x1D0A, 0x1D20, 0x1D36, 0x1D4D, 0x1D63,
    0x1D79, 0x1D90, 0x1DA6, 0x1DBC, 0x1DD3, 0x1DE9, 0x1DFF, 0x1E15,
    0x1E2B, 0x1E42, 0x1E58, 0x1E6E, 0x1E84, 0x1E9A, 0x1EB0, 0x1EC6,
    0x1EDC, 0x1EF2, 0x1F08, 0x1F1E, 0x1F34, 0x1F4A, 0x1F60, 0x1F76,
    0x1F8C, 0x1FA2, 0x1FB7, 0x1FCD, 0x1FE3, 0x1FF9, 0x200F, 0x2024,
    0x203A, 0x2050, 0x2065, 0x207B, 0x2091, 0x20A6, 0x20BC, 0x20D1,
    0x20E7, 0x20FD, 0x2112, 0x2128, 0x213D, 0x2153, 0x2168, 0x217D,
    0x2193, 0x21A8, 0x21BE, 0x21D3, 0x21E8, 0x21FE, 0x2213, 0x2228,
    0x223D, 0x2253, 0x2268, 0x227D, 0x2292, 0x22A7, 0x22BC, 0x22D2,
    0x22E7, 0x22FC, 0x2311, 0x2326, 0x233B, 0x2350, 0x2365, 0x237A,
    0x238E, 0x23A3, 0x23B8, 0x23CD, 0x23E2, 0x23F7, 0x240B, 0x2420,
    0x2435, 0x244A, 0x245E, 0x2473, 0x2488, 0x249C, 0x24B1, 0x24C5,
    0x24DA, 0x24EF, 0x2503, 0x2518, 0x252C, 0x2541, 0x2555, 0x2569,
    0x257E, 0x2592, 0x25A6, 0x25BB, 0x25CF, 0x25E3, 0x25F8, 0x260C,
    0x2620, 0x2634, 0x2648, 0x265C, 0x2671, 0x2685, 0x2699, 0x26AD,
    0x26C1, 0x26D5, 0x26E9, 0x26FD, 0x2711, 0x2724, 0x2738, 0x274C,
    0x2760, 0x2774, 0x2788, 0x279B, 0x27AF, 0x27C3, 0x27D6, 0x27EA,
    0x27FE, 0x2811, 0x2825, 0x2838, 0x284C, 0x2860, 0x2873, 0x2886,
    0x289A, 0x28AD, 0x28C1, 0x28D4, 0x28E7, 0x28FB, 0x290E, 0x2921,
    0x2935, 0x2948, 0x295B, 0x296E, 0x2981, 0x2994, 0x29A7, 0x29BB,
    0x29CE, 0x29E1, 0x29F4, 0x2A07, 0x2A1A, 0x2A2C, 0x2A3F, 0x2A52,
    0x2A65, 0x2A78, 0x2A8B, 0x2A9D, 0x2AB0, 0x2AC3, 0x2AD6, 0x2AE8,
    0x2AFB, 0x2B0D, 0x2B20, 0x2B33, 0x2B45, 0x2B58, 0x2B6A, 0x2B7D,
    0x2B8F, 0x2BA1, 0x2BB4, 0x2BC6, 0x2BD8, 0x2BEB, 0x2BFD, 0x2C0F,
    0x2C21, 0x2C34, 0x2C46, 0x2C58, 0x2C6A, 0x2C7C, 0x2C8E, 0x2CA0,
    0x2CB2, 0x2CC4, 0x2CD6, 0x2CE8, 0x2CFA, 0x2D0C, 0x2D1E, 0x2D2F,
    0x2D41, 0x2D53, 0x2D65, 0x2D76, 0x2D88, 0x2D9A, 0x2DAB, 0x2DBD,
    0x2DCF, 0x2DE0, 0x2DF2, 0x2E03, 0x2E15, 0x2E26, 0x2E37, 0x2E49,
    0x2E5A, 0x2E6B, 0x2E7D, 0x2E8E, 0x2E9F, 0x2EB0, 0x2EC2, 0x2ED3,
    0x2EE4, 0x2EF5, 0x2F06, 0x2F17, 0x2F28, 0x2F39, 0x2F4A, 0x2F5B,
    0x2F6C, 0x2F7D, 0x2F8D, 0x2F9E, 0x2FAF, 0x2FC0, 0x2FD0, 0x2FE1,
    0x2FF2, 0x3002, 0x3013, 0x3024, 0x3034, 0x3045, 0x3055, 0x3066,
    0x3076, 0x3087, 0x3097, 0x30A7, 0x30B8, 0x30C8, 0x30D8, 0x30E8,
    0x30F9, 0x3109, 0x3119, 0x3129, 0x3139, 0x3149, 0x3159, 0x3169,
    0x3179, 0x3189, 0x3199, 0x31A9, 0x31B9, 0x31C8, 0x31D8, 0x31E8,
    0x31F8, 0x3207, 0x3217, 0x3227, 0x3236, 0x3246, 0x3255, 0x3265,
    0x3274, 0x3284, 0x3293, 0x32A3, 0x32B2, 0x32C1, 0x32D0, 0x32E0,
    0x32EF, 0x32FE, 0x330D, 0x331D, 0x332C, 0x333B, 0x334A, 0x3359,
    0x3368, 0x3377, 0x3386, 0x3395, 0x33A3, 0x33B2, 0x33C1, 0x33D0,
    0x33DF, 0x33ED, 0x33FC, 0x340B, 0x3419, 0x3428, 0x3436, 0x3445,
    0x3453, 0x3462, 0x3470, 0x347F, 0x348D, 0x349B, 0x34AA, 0x34B8,
    0x34C6, 0x34D4, 0x34E2, 0x34F1, 0x34FF, 0x350D, 0x351B, 0x3529,
    0x3537, 0x3545, 0x3553, 0x3561, 0x356E, 0x357C, 0x358A, 0x3598,
    0x35A5, 0x35B3, 0x35C1, 0x35CE, 0x35DC, 0x35EA, 0x35F7, 0x3605,
    0x3612, 0x3620, 0x362D, 0x363A, 0x3648, 0x3655, 0x3662, 0x366F,
    0x367D, 0x368A, 0x3697, 0x36A4, 0x36B1, 0x36BE, 0x36CB, 0x36D8,
    0x36E5, 0x36F2, 0x36FF, 0x370C, 0x3718, 0x3725, 0x3732, 0x373F,
    0x374B, 0x3758, 0x3765, 0x3771, 0x377E, 0x378A, 0x3797, 0x37A3,
    0x37B0, 0x37BC, 0x37C8, 0x37D5, 0x37E1, 0x37ED, 0x37F9, 0x3805,
    0x3812, 0x381E, 0x382A, 0x3836, 0x3842, 0x384E, 0x385A, 0x3866,
    0x3871, 0x387D, 0x3889, 0x3895, 0x38A1, 0x38AC, 0x38B8, 0x38C3,
    0x38CF, 0x38DB, 0x38E6, 0x38F2, 0x38FD, 0x3909, 0x3914, 0x391F,
    0x392B, 0x3936, 0x3941, 0x394C, 0x3958, 0x3963, 0x396E, 0x3979,
    0x3984, 0x398F, 0x399A, 0x39A5, 0x39B0, 0x39BB, 0x39C5, 0x39D0,
    0x39DB, 0x39E6, 0x39F0, 0x39FB, 0x3A06, 0x3A10, 0x3A1B, 0x3A25,
    0x3A30, 0x3A3A, 0x3A45, 0x3A4F, 0x3A59, 0x3A64, 0x3A6E, 0x3A78,
    0x3A82, 0x3A8D, 0x3A97, 0x3AA1, 0x3AAB, 0x3AB5, 0x3ABF, 0x3AC9,
    0x3AD3, 0x3ADD, 0x3AE6, 0x3AF0, 0x3AFA, 0x3B04, 0x3B0E, 0x3B17,
    0x3B21, 0x3B2A, 0x3B34, 0x3B3E, 0x3B47, 0x3B50, 0x3B5A, 0x3B63,
    0x3B6D, 0x3B76, 0x3B7F, 0x3B88, 0x3B92, 0x3B9B, 0x3BA4, 0x3BAD,
    0x3BB6, 0x3BBF, 0x3BC8, 0x3BD1, 0x3BDA, 0x3BE3, 0x3BEC, 0x3BF5,
    0x3BFD, 0x3C06, 0x3C0F, 0x3C17, 0x3C20, 0x3C29, 0x3C31, 0x3C3A,
    0x3C42, 0x3C4B, 0x3C53, 0x3C5B, 0x3C64, 0x3C6C, 0x3C74, 0x3C7D,
    0x3C85, 0x3C8D, 0x3C95, 0x3C9D, 0x3CA5, 0x3CAD, 0x3CB5, 0x3CBD,
    0x3CC5, 0x3CCD, 0x3CD5, 0x3CDD, 0x3CE4, 0x3CEC, 0x3CF4, 0x3CFB,
    0x3D03, 0x3D0B, 0x3D12, 0x3D1A, 0x3D21, 0x3D28, 0x3D30, 0x3D37,
    0x3D3F, 0x3D46, 0x3D4D, 0x3D54, 0x3D5B, 0x3D63, 0x3D6A, 0x3D71,
    0x3D78, 0x3D7F, 0x3D86, 0x3D8D, 0x3D93, 0x3D9A, 0x3DA1, 0x3DA8,
    0x3DAF, 0x3DB5, 0x3DBC, 0x3DC2, 0x3DC9, 0x3DD0, 0x3DD6, 0x3DDD,
    0x3DE3, 0x3DE9, 0x3DF0, 0x3DF6, 0x3DFC, 0x3E03, 0x3E09, 0x3E0F,
    0x3E15, 0x3E1B, 0x3E21, 0x3E27, 0x3E2D, 0x3E33, 0x3E39, 0x3E3F,
    0x3E45, 0x3E4A, 0x3E50, 0x3E56, 0x3E5C, 0x3E61, 0x3E67, 0x3E6C,
    0x3E72, 0x3E77, 0x3E7D, 0x3E82, 0x3E88, 0x3E8D, 0x3E92, 0x3E98,
    0x3E9D, 0x3EA2, 0x3EA7, 0x3EAC, 0x3EB1, 0x3EB6, 0x3EBB, 0x3EC0,
    0x3EC5, 0x3ECA, 0x3ECF, 0x3ED4, 0x3ED8, 0x3EDD, 0x3EE2, 0x3EE7,
    0x3EEB, 0x3EF0, 0x3EF4, 0x3EF9, 0x3EFD, 0x3F02, 0x3F06, 0x3F0A,
    0x3F0F, 0x3F13, 0x3F17, 0x3F1C, 0x3F20, 0x3F24, 0x3F28, 0x3F2C,
    0x3F30, 0x3F34, 0x3F38, 0x3F3C, 0x3F40, 0x3F43, 0x3F47, 0x3F4B,
    0x3F4F, 0x3F52, 0x3F56, 0x3F5A, 0x3F5D, 0x3F61, 0x3F64, 0x3F68,
    0x3F6B, 0x3F6E, 0x3F72, 0x3F75, 0x3F78, 0x3F7B, 0x3F7F, 0x3F82,
    0x3F85, 0x3F88, 0x3F8B, 0x3F8E, 0x3F91, 0x3F94, 0x3F97, 0x3F99,
    0x3F9C, 0x3F9F, 0x3FA2, 0x3FA4, 0x3FA7, 0x3FAA, 0x3FAC, 0x3FAF,
    0x3FB1, 0x3FB4, 0x3FB6, 0x3FB8, 0x3FBB, 0x3FBD, 0x3FBF, 0x3FC1,
    0x3FC4, 0x3FC6, 0x3FC8, 0x3FCA, 0x3FCC, 0x3FCE, 0x3FD0, 0x3FD2,
    0x3FD4, 0x3FD5, 0x3FD7, 0x3FD9, 0x3FDB, 0x3FDC, 0x3FDE, 0x3FE0,
    0x3FE1, 0x3FE3, 0x3FE4, 0x3FE6, 0x3FE7, 0x3FE8, 0x3FEA, 0x3FEB,
    0x3FEC, 0x3FED, 0x3FEF, 0x3FF0, 0x3FF1, 0x3FF2, 0x3FF3, 0x3FF4,
    0x3FF5, 0x3FF6, 0x3FF7, 0x3FF7, 0x3FF8, 0x3FF9, 0x3FFA, 0x3FFA,
    0x3FFB, 0x3FFC, 0x3FFC, 0x3FFD, 0x3FFD, 0x3FFE, 0x3FFE, 0x3FFE,
    0x3FFF, 0x3FFF, 0x3FFF, 0x4000, 0x4000, 0x4000, 0x4000, 0x4000, 0x4000
};

int32 phd_sin(int32 x)
{
    x &= 0xFFFF;
    bool neg = (x > 0x8000);
    x &= 0x7FFF;

    if (x >= 0x4000) {
        x = 0x8000 - x;
    }

    x = sin_table[x >> 4];

    return neg ? -x : x;
}

int32 phd_cos(int32 x)
{
    return phd_sin(x + 0x4000);
}

Matrix matrixStack[MAX_MATRICES];
int32  matrixStackIndex = 0;
vec3i  viewPos;

Matrix& matrixGet() {
    return matrixStack[matrixStackIndex];
}

void matrixPush() {
#ifdef _WIN32
    if (matrixStackIndex >= MAX_MATRICES - 1) {
        DebugBreak();
        return;
    }
#endif
    Matrix &a = matrixStack[matrixStackIndex++];
    Matrix &b = matrixStack[matrixStackIndex];
    memcpy(b, a, sizeof(Matrix));
}

void matrixPop() {
#ifdef _WIN32
    if (matrixStackIndex <= 0) {
        DebugBreak();
        return;
    }
#endif
    matrixStackIndex--;
}

void matrixTranslate(const vec3i &offset) {
    Matrix &m = matrixGet();

    m[0].w += DP33(m[0], offset);
    m[1].w += DP33(m[1], offset);
    m[2].w += DP33(m[2], offset);
}

void matrixTranslateAbs(const vec3i &offset) {
    vec3i d;
    d.x = offset.x - viewPos.x;
    d.y = offset.y - viewPos.y;
    d.z = offset.z - viewPos.z;

    Matrix &m = matrixGet();
    m[0].w = DP33(m[0], d);
    m[1].w = DP33(m[1], d);
    m[2].w = DP33(m[2], d);
}

void matrixRotate(int16 rotX, int16 rotY, int16 rotZ) {}

void matrixSetView(const vec3i &pos, int16 rotX, int16 rotY) {
    int32 sx = phd_sin(rotX);
    int32 cx = phd_cos(rotX);
    int32 sy = phd_sin(rotY);
    int32 cy = phd_cos(rotY);

    Matrix &m = matrixGet();

    m[0].x = cy;
    m[0].y = 0;
    m[0].z = -sy;
    m[0].w = pos.x;

    m[1].x = (sx * sy) >> FIXED_SHIFT;
    m[1].y = cx;
    m[1].z = (sx * cy) >> FIXED_SHIFT;
    m[1].w = pos.y;

    m[2].x = (cx * sy) >> FIXED_SHIFT;
    m[2].y = -sx;
    m[2].z = (cx * cy) >> FIXED_SHIFT;
    m[2].w = pos.z;

    viewPos = pos;
}

int32 clamp(int32 x, int32 a, int32 b) {
    return x < a ? a : (x > b ? b : x);
}

template <class T>
inline void swap(T &a, T &b) {
    T tmp = a;
    a = b;
    b = tmp;
}

bool checkBackface(const Vertex *a, const Vertex *b, const Vertex *c) {
    return (b->x - a->x) * (c->y - a->y) -
           (c->x - a->x) * (b->y - a->y) <= 0;
}

INLINE void sortVertices(Vertex *&t, Vertex *&m, Vertex *&b) {
    if (t->y > m->y) swap(t, m);
    if (t->y > b->y) swap(t, b);
    if (m->y > b->y) swap(m, b);
}

INLINE void sortVertices(Vertex *&t, Vertex *&m, Vertex *&o, Vertex *&b) {
    if (t->y > m->y) swap(t, m);
    if (o->y > b->y) swap(o, b);
    if (t->y > o->y) swap(t, o);
    if (m->y > b->y) swap(m, b);
    if (m->y > o->y) swap(m, o);
}

INLINE int32 classify(const Vertex* v) {
    return (v->x < clip.x0 ? 1 : 0) |
           (v->x > clip.x1 ? 2 : 0) |
           (v->y < clip.y0 ? 4 : 0) |
           (v->y > clip.y1 ? 8 : 0);
}

void transform(const vec3s &v, int32 vg) {
#ifdef _WIN32
    if (gVerticesCount >= MAX_VERTICES) {
        DebugBreak();
        return;
    }
#endif
    const Matrix &m = matrixStack[matrixStackIndex];

    Vertex &res = gVertices[gVerticesCount++];

    int32 z = DP43(m[2], v);
    // znear / zfar clip
    if (z < MIN_DIST || z >= MAX_DIST) {
        res.z = -1;
        return;
    }

    int32 x = DP43(m[0], v);
    int32 y = DP43(m[1], v);

    int32 fog = (8191 - vg) - (SQR(z >> FIXED_SHIFT) >> 15);
    if (fog < 0) {
        fog = 0;
    }

    z >>= FOV_SHIFT;
    x = (x / z) + (FRAME_WIDTH  / 2);
    y = (y / z) + (FRAME_HEIGHT / 2);

    res.x = x;
    res.y = y;
    res.z = z >> (FIXED_SHIFT - FOV_SHIFT);;
    res.clip = classify(&res);

    res.g = uint32(255 - (fog >> 5)) >> 3;
}

#define FETCH_T()               curTile[(t & 0xFF00) | (t >> 24)]
#define FETCH_T_MIP()           curTile[(t & 0xFF00) | (t >> 24) & mipMask]
#define FETCH_GT()              lightmap[(g & 0x1F00) | FETCH_T()]
#define FETCH_G(palIndex)       lightmap[(g & 0x1F00) | palIndex]
#define FETCH_GT_PAL()          palette[FETCH_GT()]
#define FETCH_G_PAL(palIndex)   palette[FETCH_G(palIndex)]

struct Edge {
    int32  h;
    int32  x;
    int32  g;
    uint32 t;
    int32  dx;
    int32  dg;
    uint32 dt;

    int32   index;
    Vertex* vert[8];

    Edge() : h(0), dx(0), dg(0), dt(0) {}

    INLINE void stepG() {
        x += dx;
        g += dg;
    }

    INLINE void stepGT() {
        x += dx;
        g += dg;
        t += dt;
    }

    INLINE bool nextG() {
        if (index == 0) {
            return false;
        }

        Vertex* v1 = vert[index--];
        Vertex* v2 = vert[index];

        h = v2->y - v1->y;
        x = v1->x << 16;
        g = v1->g << 16;

        if (h > 1) {
            uint32 d = FixedInvU(h);

            dx = d * (v2->x - v1->x);
            dg = d * (v2->g - v1->g);
        }

        return true;
    }

    INLINE bool nextGT() {
        if (index == 0) {
            return false;
        }

        Vertex* v1 = vert[index--];
        Vertex* v2 = vert[index];

        h = v2->y - v1->y;
        x = v1->x << 16;
        g = v1->g << 16;
        t = (v1->u << 24) | (v1->v << 8);

        if (h > 1) {
            uint32 d = FixedInvU(h);

            dx = d * (v2->x - v1->x);
            dg = d * (v2->g - v1->g);

            int32 du = d * (v2->u - v1->u);
            int32 dv = d * (v2->v - v1->v);

            dt = ((du << 8) & 0xFFFF0000) | int16(dv >> 8);
        }

        return true;
    }

    void build(int32 start, int32 count, int32 t, int32 b, int32 incr) {
        vert[index = 0] = gVertices + start + b;

        for (int i = 1; i < count; i++) {
            b = (b + incr) % count;

            Vertex *v = gVertices + start + b;

            if (vert[index]->x != v->x || vert[index]->y != v->y) {
                vert[++index] = v;
            }

            if (b == t) {
                break;
            }
        }
    }
};

INLINE void scanlineG(uint16* buffer, int32 x1, int32 x2, uint8 palIndex, uint32 g, int32 dgdx) {
    #ifdef USE_MODE_5
        uint16* pixel = buffer + x1;
        int32 width = (x2 - x1);

        while (width--)
        {
            *pixel++ = FETCH_G_PAL(palIndex);
            g += dgdx;
        }
    #else
        if (x1 & 1)
        {
            uint16 &p = *(uint16*)((uint8*)buffer + x1 - 1);
            p = (p & 0x00FF) | (FETCH_G(palIndex) << 8);
            g += dgdx;
            x1++;
        }

        int32 width = (x2 - x1) >> 1;
        uint16* pixel = (uint16*)((uint8*)buffer + x1);

        dgdx <<= 1;

        if (width && (x1 & 3))
        {
            uint16 p = FETCH_G(palIndex);
            *pixel++ = p | (FETCH_G(palIndex) << 8);

            g += dgdx;

            width--;
        }

        while (width-- > 0)
        {
            uint32 p = FETCH_G(palIndex);
            p |= (FETCH_G(palIndex) << 8);

            g += dgdx;

            if (width-- > 0)
            {
                p |= (FETCH_G(palIndex) << 16);
                p |= (FETCH_G(palIndex) << 24);

                g += dgdx;

                *(uint32*)pixel = p;
                pixel += 2;
            } else {
                *(uint16*)pixel = p;
                pixel += 1;
            }
        }

        if (x2 & 1)
        {
            *pixel = (*pixel & 0xFF00) | FETCH_G(palIndex);
        }
    #endif
}

INLINE void scanlineGT(uint16* buffer, int32 x1, int32 x2, uint32 g, uint32 t, int32 dgdx, uint32 dtdx) {
    #ifdef USE_MODE_5
        uint16* pixel = buffer + x1;
        int32 width = (x2 - x1);

        while (width--)
        {
            *pixel++ = FETCH_GT_PAL();
            t += dtdx;
            g += dgdx;
        }
    #else
        if (x1 & 1)
        {
            uint16 &p = *(uint16*)((uint8*)buffer + x1 - 1);
            p = (p & 0x00FF) | (FETCH_GT() << 8);
            t += dtdx;
            g += dgdx;
            x1++;
        }

        int32 width = (x2 - x1) >> 1;
        uint16* pixel = (uint16*)((uint8*)buffer + x1);

        dgdx <<= 1;

        if (width && (x1 & 3))
        {
            uint16 p = FETCH_GT();
            t += dtdx;
            *pixel++ = p | (FETCH_GT() << 8);
            t += dtdx;

            g += dgdx;

            width--;
        }

        while (width-- > 0)
        {
            uint32 p = FETCH_GT();
            t += dtdx;
            p |= (FETCH_GT() << 8);
            t += dtdx;

            g += dgdx;

            if (width-- > 0)
            {
                p |= (FETCH_GT() << 16);
                t += dtdx;
                p |= (FETCH_GT() << 24);
                t += dtdx;

                g += dgdx;

                *(uint32*)pixel = p;
                pixel += 2;
            } else {
                *(uint16*)pixel = p;
                pixel += 1;
            }
        }

        if (x2 & 1)
        {
            *pixel = (*pixel & 0xFF00) | FETCH_GT();
        }
    #endif
}

void rasterizeG(uint16* buffer, int32 palIndex, Edge &L, Edge &R) {
    while (1)
    {
        while (L.h <= 0)
        {
            if (!L.nextG())
            {
                return;
            }
        }

        while (R.h <= 0)
        {
            if (!R.nextG())
            {
                return;
            }
        }

        int32 h = MIN(L.h, R.h);
        L.h -= h;
        R.h -= h;

        while (h--) {
            int32 x1 = L.x >> 16;
            int32 x2 = R.x >> 16;

            if (x2 > x1)
            {
                int32 d = FixedInvU(x2 - x1);

                int32 dgdx = d * ((R.g - L.g) >> 8) >> 16;

                scanlineG(buffer, x1, x2, palIndex, L.g >> 8, dgdx);
            }

            buffer += WIDTH / PIXEL_SIZE;

            L.stepG();
            R.stepG();
        }
    }
}

void rasterizeGT(uint16* buffer, Edge &L, Edge &R)
{
    while (1)
    {
        while (L.h <= 0)
        {
            if (!L.nextGT())
            {
                return;
            }
        }

        while (R.h <= 0)
        {
            if (!R.nextGT())
            {
                return;
            }
        }

        int32 h = MIN(L.h, R.h);
        L.h -= h;
        R.h -= h;

        while (h--) {
            int32 x1 = L.x >> 16;
            int32 x2 = R.x >> 16;

            if (x2 > x1)
            {
                uint32 d = FixedInvU(x2 - x1);

                int32  dgdx = d * ((R.g - L.g) >> 8) >> 16;

                uint32 u = d * ((R.t >> 16) - (L.t >> 16));
                uint32 v = d * ((R.t & 0xFFFF) - (L.t & 0xFFFF));
                uint32 dtdx = (u & 0xFFFF0000) | (v >> 16);

                scanlineGT(buffer, x1, x2, L.g >> 8, L.t, dgdx, dtdx);
            }

            buffer += WIDTH / PIXEL_SIZE;

            L.stepGT();
            R.stepGT();
        }
    }
}

void drawTriangle(const Face* face) {
    Vertex *v1, *v2, *v3;

    bool clipped = face->indices[0] == face->indices[1];

    if (clipped) {
        v1 = gVertices + face->start;
        v2 = v1 + 1;
        v3 = v1 + 2;
    } else {
        v1 = gVertices + face->start;
        v2 = v1 + face->indices[1];
        v3 = v1 + face->indices[2];
    }

    uint16 palIndex = face->flags & FACE_TEXTURE;

    if (!(face->flags & FACE_COLORED)) {
        const Texture &tex = textures[palIndex];
        palIndex = 0xFFFF;
        curTile = tiles[tex.tile];
        if (!clipped) {
            v1->u = tex.x0;
            v1->v = tex.y0;
            v2->u = tex.x1;
            v2->v = tex.y1;
            v3->u = tex.x2;
            v3->v = tex.y2;
        }
    }

    sortVertices(v1, v2, v3);

    int32 temp = (v2->y - v1->y) * FixedInvU(v3->y - v1->y);

    int32 longest = ((temp * (v3->x - v1->x)) >> 16) + (v1->x - v2->x);
    if (longest == 0)
    {
        return;
    }

    Edge L, R;

    if (longest < 0)
    {
        R.vert[0] = v3;
        R.vert[1] = v2;
        R.vert[2] = v1;
        R.index   = 2;
        L.vert[0] = v3;
        L.vert[1] = v1;
        L.index   = 1;
    }
    else
    {
        L.vert[0] = v3;
        L.vert[1] = v2;
        L.vert[2] = v1;
        L.index   = 2;
        R.vert[0] = v3;
        R.vert[1] = v1;
        R.index   = 1;
    }

    if (palIndex != 0xFFFF) {
        rasterizeG((uint16*)fb + v1->y * (WIDTH / PIXEL_SIZE), palIndex, L, R);
    } else {
        rasterizeGT((uint16*)fb + v1->y * (WIDTH / PIXEL_SIZE), L, R);
    }
}

void drawQuad(const Face* face) {
    Vertex *v1, *v2, *v3, *v4;
    bool clipped = face->indices[0] == face->indices[1];

    if (clipped) {
        v1 = gVertices + face->start;
        v2 = v1 + 1;
        v3 = v1 + 2;
        v4 = v1 + 3;
    } else {
        v1 = gVertices + face->start;
        v2 = v1 + face->indices[1];
        v3 = v1 + face->indices[2];
        v4 = v1 + face->indices[3];
    }

    uint16 palIndex = face->flags & FACE_TEXTURE;

    if (!(face->flags & FACE_COLORED)) {
        const Texture &tex = textures[palIndex];
        palIndex = 0xFFFF;
        curTile = tiles[tex.tile];
        if (!clipped) {
            v1->u = tex.x0;
            v1->v = tex.y0;
            v2->u = tex.x1;
            v2->v = tex.y1;
            v3->u = tex.x2;
            v3->v = tex.y2;
            v4->u = tex.x3;
            v4->v = tex.y3;
        }
    }

    sortVertices(v1, v2, v3, v4);

    int32 temp = (v2->y - v1->y) * FixedInvU(v4->y - v1->y);

    int32 longest = ((temp * (v4->x - v1->x)) >> 16) + (v1->x - v2->x);
    if (longest == 0)
    {
        return;
    }

    Edge L, R;

    if (checkBackface(v1, v4, v2) == checkBackface(v1, v4, v3))
    {
        if (longest < 0)
        {
            L.vert[0] = v4;
            L.vert[1] = v1;
            L.index   = 1;
            R.vert[0] = v4;
            R.vert[1] = v3;
            R.vert[2] = v2;
            R.vert[3] = v1;
            R.index   = 3;
        }
        else
        {
            R.vert[0] = v4;
            R.vert[1] = v1;
            R.index   = 1;
            L.vert[0] = v4;
            L.vert[1] = v3;
            L.vert[2] = v2;
            L.vert[3] = v1;
            L.index   = 3;
        }
    }
    else
    {
        R.vert[0] = v4;
        R.vert[1] = v3;
        R.vert[2] = v1;
        R.index   = 2;
        L.vert[0] = v4;
        L.vert[1] = v2;
        L.vert[2] = v1;
        L.index   = 2;

        if (longest < 0)
        {
            swap(L.vert[1], R.vert[1]);
        }
    }

    if (palIndex != 0xFFFF) {
        rasterizeG((uint16*)fb + v1->y * (WIDTH / PIXEL_SIZE), palIndex, L, R);
    } else {
        rasterizeGT((uint16*)fb + v1->y * (WIDTH / PIXEL_SIZE), L, R);
    }
}

void drawPoly(const Face* face) {
    uint16 palIndex = face->flags & FACE_TEXTURE;
    
    if (!(face->flags & FACE_COLORED)) {
        const Texture &tex = textures[palIndex];
        palIndex = 0xFFFF;
        curTile = tiles[tex.tile];
    }

    int32 start = face->start;
    int32 count = face->indices[0];

    int32 minY =  0x7FFF;
    int32 maxY = -0x7FFF;
    int32 t = start, b = start;

    for (int i = 0; i < count; i++) {
        Vertex *v = gVertices + start + i;

        if (v->y < minY) {
            minY = v->y;
            t = i;
        }

        if (v->y > maxY) {
            maxY = v->y;
            b = i;
        }
    }

    Edge L, R;

    L.build(start, count, t, b, count + 1);
    R.build(start, count, t, b, count - 1);

    Vertex *v1 = gVertices + start + t;

    if (palIndex != 0xFFFF) {
        rasterizeG((uint16*)fb + v1->y * (WIDTH / PIXEL_SIZE), palIndex, L, R);
    } else {
        rasterizeGT((uint16*)fb + v1->y * (WIDTH / PIXEL_SIZE), L, R);
    }
}

void drawGlyph(const Sprite *sprite, int32 x, int32 y) {
    int32 w = sprite->r - sprite->l;
    int32 h = sprite->b - sprite->t;

    w = (w >> 1) << 1; // make it even

    int32 ix = x + sprite->l;
    int32 iy = y + sprite->t;

    uint16* ptr = (uint16*)fb + iy * (WIDTH / PIXEL_SIZE);

#ifdef USE_MODE_5
    ptr += ix;
#else
    ptr += ix >> 1;
#endif

    const uint8* glyphData = tiles[sprite->tile] + 256 * sprite->v + sprite->u;

    while (h--)
    {
    #ifdef USE_MODE_5
        for (int i = 0; i < w; i++) {
            if (glyphData[i] == 0) continue;

            ptr[i] = palette[glyphData[i]];
        }
    #else
        const uint8* p = glyphData;

        for (int i = 0; i < (w / 2); i++) {

            if (p[0] || p[1]) {
                uint16 d = ptr[i];

                if (p[0]) d = (d & 0xFF00) | p[0];
                if (p[1]) d = (d & 0x00FF) | (p[1] << 8);

                ptr[i] = d;
            }

            p += 2;
        }
    #endif

        ptr += WIDTH / PIXEL_SIZE;
        glyphData += 256;
    }
}

void faceAddPolyClip(uint16 flags, Vertex** poly, int32 pCount) {
    #define LERP(a,b,t) (b + ((a - b) * t >> 16))

    #define CLIP_AXIS(x, y, edge, output) {\
        uint32 t = ((edge - b->x) << 16) / (a->x - b->x);\
        Vertex* v = output + count++;\
        v->x = edge;\
        v->y = LERP(a->y, b->y, t);\
        v->z = LERP(a->z, b->z, t);\
        v->u = LERP(a->u, b->u, t);\
        v->v = LERP(a->v, b->v, t);\
        v->g = LERP(a->g, b->g, t);\
    }

    #define CLIP_VERTEX(x, y, x0, x1, input, output) {\
        const Vertex *a, *b = input[pCount - 1];\
        for (int32 i = 0; i < pCount; i++) {\
            a = b;\
            b = input[i];\
            if (a->x < x0) {\
                if (b->x < x0) continue;\
                CLIP_AXIS(x, y, x0, output);\
            } else if (a->x > x1) {\
                if (b->x > x1) continue;\
                CLIP_AXIS(x, y, x1, output);\
            }\
            if (b->x < x0) {\
                CLIP_AXIS(x, y, x0, output);\
            } else if (b->x > x1) {\
                CLIP_AXIS(x, y, x1, output);\
            } else {\
                output[count++] = *b;\
            }\
        }\
        if (count < 3) return;\
    }

    if (!(flags & FACE_COLORED)) {
        const Texture &tex = textures[flags & FACE_TEXTURE];
        curTile = tiles[tex.tile];
        poly[0]->u = tex.x0;
        poly[0]->v = tex.y0;
        poly[1]->u = tex.x1;
        poly[1]->v = tex.y1;
        poly[2]->u = tex.x2;
        poly[2]->v = tex.y2;
        if (pCount == 4) {
            poly[3]->u = tex.x3;
            poly[3]->v = tex.y3;
        }
    }

    Vertex tmp[8];
    int32 count = 0;

// clip x
    int32 x0 = clip.x0;
    int32 x1 = clip.x1;
    CLIP_VERTEX(x, y, x0, x1, poly, tmp);

    pCount = count;
    count = 0;

    Vertex* output = gVertices + gVerticesCount;

// clip y
    int32 y0 = clip.y0;
    int32 y1 = clip.y1;
    CLIP_VERTEX(y, x, y0, y1, &tmp, output);

    Face *f = gFaces + gFacesCount;
    gFacesSorted[gFacesCount++] = f;
    f->flags      = flags;
    f->start      = gVerticesCount;
    f->indices[0] = count;
    f->indices[1] = count;

    if (count == 3) {
        f->flags |= FACE_TRIANGLE;
        f->depth = (output[0].z + output[1].z + output[2].z) / 3;
    } else if (count == 4) {
        f->depth = (output[0].z + output[1].z + output[2].z + output[3].z) >> 2;
    } else {
        int32 depth = output[0].z;
        for (int32 i = 1; i < count; i++) {
            depth = (depth + output[i].z) >> 1;
        }
        f->depth = depth;
    }

    gVerticesCount += count;
}

void faceAddQuad(uint16 flags, const Index* indices, int32 startVertex) {
#ifdef _WIN32
    if (gFacesCount >= MAX_FACES) {
        DebugBreak();
    }
#endif
    Vertex* v1 = gVertices + startVertex + indices[0];
    Vertex* v2 = gVertices + startVertex + indices[1];
    Vertex* v3 = gVertices + startVertex + indices[2];
    Vertex* v4 = gVertices + startVertex + indices[3];

    if (v1->z < 0 || v2->z < 0 || v3->z < 0 || v4->z < 0)
        return;

    if (!(v1->g | v2->g | v3->g | v4->g))
        return;

    if (checkBackface(v1, v2, v3))
        return;

    if (v1->clip & v2->clip & v3->clip & v4->clip)
        return;

    if (v1->clip | v2->clip | v3->clip | v4->clip) {
        Vertex* poly[4] = { v1, v2, v3, v4 };
        faceAddPolyClip(flags, poly, 4);
    } else {
        Face *f = gFaces + gFacesCount;
        gFacesSorted[gFacesCount++] = f;
        f->flags      = flags;
        f->depth      = (v1->z + v2->z + v3->z + v4->z) >> 2;
        f->start      = startVertex + indices[0];
        f->indices[0] = 0;
        f->indices[1] = indices[1] - indices[0];
        f->indices[2] = indices[2] - indices[0];
        f->indices[3] = indices[3] - indices[0];
    }
}

void faceAddTriangle(uint16 flags, const Index* indices, int32 startVertex) {
#ifdef _WIN32
    if (gFacesCount >= MAX_FACES) {
        DebugBreak();
    }
#endif
    Vertex* v1 = gVertices + startVertex + indices[0];
    Vertex* v2 = gVertices + startVertex + indices[1];
    Vertex* v3 = gVertices + startVertex + indices[2];

    if (v1->z < 0 || v2->z < 0 || v3->z < 0)
        return;

    if (!(v1->g | v2->g | v3->g))
        return;

    if (checkBackface(v1, v2, v3))
        return;

    if (v1->clip & v2->clip & v3->clip)
        return;

    if (v1->clip | v2->clip | v3->clip) {
        Vertex* poly[3] = { v1, v2, v3 };
        faceAddPolyClip(flags, poly, 3);
    } else {
        Face *f = gFaces + gFacesCount;
        gFacesSorted[gFacesCount++] = f;
        f->flags      = flags | FACE_TRIANGLE;
        f->depth      = (v1->z + v2->z + v3->z) / 3;
        f->start      = startVertex + indices[0];
        f->indices[0] = 0;
        f->indices[1] = indices[1] - indices[0];
        f->indices[2] = indices[2] - indices[0];
    }
}

void faceSort(Face** faces, int32 L, int32 R) {
    int32 i = L;
    int32 j = R;
    int16 depth = faces[(i + j) >> 1]->depth;

    while (i <= j) {
        while (faces[i]->depth > depth) i++;
        while (faces[j]->depth < depth) j--;

        if (i <= j) {
            swap(faces[i++], faces[j--]);
        }
    };

    if (L < j) faceSort(faces, L, j);
    if (R > i) faceSort(faces, i, R);
}

//int32 gFacesCountMax, gVerticesCountMax;

void flush() {
    if (gFacesCount) {
        faceSort(gFacesSorted, 0, gFacesCount - 1);

        //const uint16 mips[] = { 0xFFFF, 0xFEFE, 0xFCFC, 0xF8F8 };

        for (int32 i = 0; i < gFacesCount; i++) {
            const Face *f = gFacesSorted[i];

            // TODO
            //mipMask = mips[MIN(3, f.depth / 2048)];

            if (f->flags & FACE_TRIANGLE) {
                drawTriangle(f);
            } else {
                if (f->indices[0] == f->indices[1] /* && f.indices[0] > 4 */) {
                    drawPoly(f);
                } else {
                    drawQuad(f);
                }
            }
        }
    }

    //if (gFacesCount > gFacesCountMax) gFacesCountMax = gFacesCount;
    //if (gVerticesCount > gVerticesCountMax) gVerticesCountMax = gVerticesCount;
    //printf("%d %d\n", gFacesCountMax, gVerticesCountMax);

    gVerticesCount = 0;
    gFacesCount = 0;
}

void initRender() {
    divTable[0] = 0;
    for (uint32 i = 1; i < DIV_TABLE_SIZE; i++) {
        divTable[i] = (1 << 16) / i;
    }
}

void dmaClear(uint32 *dst, uint32 count) {
#ifdef WIN32
    memset(dst, 0, count * 4);
#else
    vu32 value = 0;
    REG_DMA3SAD	= (vu32)&value;
    REG_DMA3DAD	= (vu32)dst;
    REG_DMA3CNT	= count | (DMA_ENABLE | DMA32 | DMA_SRC_FIXED | DMA_DST_INC);
#endif
}

void clear() {
#ifdef USE_MODE_5
    dmaClear((uint32*)fb, (WIDTH * HEIGHT) >> 1);
#else
    dmaClear((uint32*)fb, (WIDTH * HEIGHT) >> 2);
#endif
}