#include <combo.h>
#include <combo/custom.h>

#define M_SQRT1_2 0.707106781186547524401

void Shader_Opa0_Xlu1(GameState*, s16);
void Shader_Opa0_Xlu12(GameState*, s16);
void Shader_Opa0(GameState*, s16);
void Shader_Opa01(GameState*, s16);
void Shader_Opa1023(GameState*, s16);
void Shader_Opa10_Xlu2(GameState*, s16);
void Shader_Opa10_Xlu234(GameState*, s16);
void Shader_Opa10_Xlu32(GameState*, s16);
void Shader_Opa10234567(GameState*, s16);
void Shader_Xlu01(GameState*, s16);
void Shader_BlueFire(GameState*, s16);
void Shader_BombchuMask(GameState*, s16);
void Shader_Compass(GameState*, s16);
void Shader_DekuNut(GameState*, s16);
void Shader_Fairy(GameState*, s16);
void Shader_Fish(GameState*, s16);
void Shader_GiantKnife(GameState*, s16);
void Shader_GS(GameState*, s16);
void Shader_Heart(GameState*, s16);
void Shader_Medallion(GameState*, s16);
void Shader_MirrorShield(GameState*, s16);
void Shader_Poe(GameState*, s16);
void Shader_Potion(GameState*, s16);
void Shader_Rupee(GameState*, s16);
void Shader_Scale(GameState*, s16);
void Shader_SoldOut(GameState*, s16);
void Shader_Spell(GameState*, s16);
void Shader_MoonTear(GameState*, s16);

static void color4(u8* r, u8* g, u8* b, u8* a, u32 color)
{
    *r = (color >> 24) & 0xff;
    *g = (color >> 16) & 0xff;
    *b = (color >> 8) & 0xff;
    *a = color & 0xff;
}

/* Custom Shaders */
void Shader_Xlu0(GameState* gs, s16 shaderId)
{
    const Shader* shader;

    shader = &kShaders[shaderId];
    OPEN_DISPS(gs->gfx);
    InitListPolyXlu(gs->gfx);
    gSPMatrix(POLY_XLU_DISP++, GetMatrixMV(gs->gfx), G_MTX_PUSH | G_MTX_LOAD | G_MTX_MODELVIEW);
    gSPDisplayList(POLY_XLU_DISP++, shader->lists[0]);
    CLOSE_DISPS();
}

void Shader_CustomNote(GameState* gs, s16 shaderId)
{
    static const u32 kColors[] = {
        0x8000ffff /* Purple */,
        0x0000ffff /* Blue */,
        0x00ff00ff /* Green */,
        0xffff00ff /* Yellow */,
        0xff8000ff /* Orange */,
        0xff0000ff /* Red */,
    };

    const Shader* shader;
    float angle;
    u8 r;
    u8 g;
    u8 b;
    u8 a;

    shader = &kShaders[shaderId];
    angle = M_PI / 16;
    if (shader->lists[1] & 0x10)
        angle += M_PI;
    color4(&r, &g, &b, &a, kColors[shader->lists[1] & 0xf]);

    ModelViewRotateZ(angle, MAT_MUL);

    OPEN_DISPS(gs->gfx);
    InitListPolyXlu(gs->gfx);
    gSPMatrix(POLY_XLU_DISP++, GetMatrixMV(gs->gfx), G_MTX_PUSH | G_MTX_LOAD | G_MTX_MODELVIEW);
    gDPSetEnvColor(POLY_XLU_DISP++, r, g, b, a);
    gSPDisplayList(POLY_XLU_DISP++, shader->lists[0]);
    CLOSE_DISPS();
}

void Shader_CustomHeartContainer(GameState* gs, s16 shaderId)
{
    static const u32 colors[] = {
        /* Normal */
        0xa0ffffff,
        0x0064ffff,
        0xff0064ff,
        0x640032ff,

        /* Double Defense */
        0xffff00ff,
        0xcccc00ff,
        0xffffffff,
        0xffffffff,
    };

    const Shader* shader;
    u8 r;
    u8 g;
    u8 b;
    u8 a;
    u32 c;

    shader = &kShaders[shaderId];
    c = shader->lists[0];

    OPEN_DISPS(gs->gfx);
    InitListPolyXlu(gs->gfx);
    gSPMatrix(POLY_XLU_DISP++, GetMatrixMV(gs->gfx), G_MTX_PUSH | G_MTX_LOAD | G_MTX_MODELVIEW);
    color4(&r, &g, &b, &a, colors[c * 4 + 0]);
    gDPSetPrimColor(POLY_XLU_DISP++, 0, 0x80, r, g, b, a);
    color4(&r, &g, &b, &a, colors[c * 4 + 1]);
    gDPSetEnvColor(POLY_XLU_DISP++, r, g, b, a);
    gSPDisplayList(POLY_XLU_DISP++, shader->lists[1]);
    color4(&r, &g, &b, &a, colors[c * 4 + 2]);
    gDPSetPrimColor(POLY_XLU_DISP++, 0, 0x80, r, g, b, a);
    color4(&r, &g, &b, &a, colors[c * 4 + 3]);
    gDPSetEnvColor(POLY_XLU_DISP++, r, g, b, a);
    gSPDisplayList(POLY_XLU_DISP++, shader->lists[2]);
    CLOSE_DISPS();
}

static void shaderFlameEffect(GameState* gs, int colorIndex)
{
#if defined(GAME_OOT)
    static const u32 kMatTransformOffset = 0x11da0;
    static const u32 kFlameDlist = 0x52a10;
#else
    static const u32 kMatTransformOffset = 0x187fc;
    static const u32 kFlameDlist = 0x7d590;
#endif

    static const float flameScale = 0.0055f;

    static const u32 kPrimColors[] = {
        0x00ffffc0,
        0xff00ffc0,
        0xff0000c0,
    };

    static const u32 kEnvColors[] = {
        0x0000ffc0,
        0xff0000c0,
        0xffff00c0,
    };

    u8 r;
    u8 g;
    u8 b;
    u8 a;

    OPEN_DISPS(gs->gfx);
    ModelViewUnkTransform((float*)((char*)gs + kMatTransformOffset));
    ModelViewTranslate(0.f, -30.f, -15.f, MAT_MUL);
    ModelViewScale(flameScale * 1.7f, flameScale, flameScale, MAT_MUL);
    gSPSegment(POLY_XLU_DISP++, 0x08, GetSegment(gs->gfx, 0, 0, 0, 0x20, 0x40, 1, 0, (-gs->frameCount & 0x7f) << 2, 0x20, 0x80));
    gSPMatrix(POLY_XLU_DISP++, GetMatrixMV(gs->gfx), G_MTX_PUSH | G_MTX_LOAD | G_MTX_MODELVIEW);
    color4(&r, &g, &b, &a, kPrimColors[colorIndex]);
    gDPSetPrimColor(POLY_XLU_DISP++, 0x80, 0x80, r, g, b, a);
    color4(&r, &g, &b, &a, kEnvColors[colorIndex]);
    gDPSetEnvColor(POLY_XLU_DISP++, r, g, b, a);
    gSPDisplayList(POLY_XLU_DISP++, 0x04000000 | kFlameDlist);
    CLOSE_DISPS();
}

static const u32 kNutStickPrimColors[] = {
    0xa06428ff,
    0xffffffff,
    0xffffbbff,
};
static const u32 kNutStickEnvColors[] = {
    0x280a00ff,
    0x505050ff,
    0xaaaa00ff,
};

void Shader_CustomStick(GameState* gs, s16 shaderId)
{
    const Shader* shader;
    u8 r;
    u8 g;
    u8 b;
    u8 a;

    shader = &kShaders[shaderId];

    OPEN_DISPS(gs->gfx);
    InitListPolyOpa(gs->gfx);
    gSPMatrix(POLY_OPA_DISP++, GetMatrixMV(gs->gfx), G_MTX_PUSH | G_MTX_LOAD | G_MTX_MODELVIEW);
    color4(&r, &g, &b, &a, kNutStickPrimColors[shader->lists[1]]);
    gDPSetPrimColor(POLY_OPA_DISP++, 0, 0, r, g, b, a);
    color4(&r, &g, &b, &a, kNutStickEnvColors[shader->lists[1]]);
    gDPSetEnvColor(POLY_OPA_DISP++, r, g, b, a);
    gSPDisplayList(POLY_OPA_DISP++, shader->lists[0]);

    /* Draw fire */
    if (shader->lists[1])
    {
        InitListPolyXlu(gs->gfx);
        shaderFlameEffect(gs, shader->lists[1] - 1);
    }

    CLOSE_DISPS();
}

void Shader_CustomNut(GameState* gs, s16 shaderId)
{
    const Shader* shader;
    u8 r;
    u8 g;
    u8 b;
    u8 a;
    u32 fc;

    shader = &kShaders[shaderId];
    fc = gs->frameCount * 6;

    OPEN_DISPS(gs->gfx);
    InitListPolyOpa(gs->gfx);
    gSPSegment(POLY_OPA_DISP++, 0x09, GetSegment(gs->gfx, 0, fc, fc, 0x20, 0x20, 1, fc, fc, 0x20, 0x20));
    gSPMatrix(POLY_OPA_DISP++, GetMatrixMV(gs->gfx), G_MTX_PUSH | G_MTX_LOAD | G_MTX_MODELVIEW);
    color4(&r, &g, &b, &a, kNutStickPrimColors[shader->lists[1]]);
    gDPSetPrimColor(POLY_OPA_DISP++, 0, 0, r, g, b, a);
    color4(&r, &g, &b, &a, kNutStickEnvColors[shader->lists[1]]);
    gDPSetEnvColor(POLY_OPA_DISP++, r, g, b, a);
    gSPDisplayList(POLY_OPA_DISP++, shader->lists[0]);

    /* Draw fire */
    if (shader->lists[1])
    {
        InitListPolyXlu(gs->gfx);
        shaderFlameEffect(gs, shader->lists[1] - 1);
    }

    CLOSE_DISPS();
}

static void* pushMatrix(GfxContext* gfx, const float* mat)
{
    void* end = gfx->polyOpa.end;
    end = (char*)end - 0x40;
    gfx->polyOpa.end = end;

    ConvertMatrix(mat, end);

    return end;
}

static void* dummySegment(GfxContext* gfx)
{
    Gfx* end = gfx->polyOpa.end - 1;
    gfx->polyOpa.end = end;
    gSPEndDisplayList(end);
    return end;
}

void Shader_BossRemains(GameState* gs, s16 shaderId)
{
    static const float scale = 0.03f;
    static const float kMatrixScale[] = {
        scale, 0.f, 0.f, 0.f,
        0.f, scale, 0.f, 0.f,
        0.f, 0.f, scale, 0.f,
        0.f, 0.f, 0.f,   1.f,
    };
    const Shader* shader;

    shader = &kShaders[shaderId];
    OPEN_DISPS(gs->gfx);
    gSPMatrix(POLY_OPA_DISP++, GetMatrixMV(gs->gfx), G_MTX_PUSH | G_MTX_LOAD | G_MTX_MODELVIEW);
    gSPMatrix(POLY_OPA_DISP++, pushMatrix(gs->gfx, kMatrixScale), G_MTX_NOPUSH | G_MTX_MUL | G_MTX_MODELVIEW);
    InitListPolyOpa(gs->gfx);
    gSPDisplayList(POLY_OPA_DISP++, shader->lists[0]);
    CLOSE_DISPS();
}

void Shader_SpiritualStones(GameState* gs, s16 shaderId)
{
    static const float kMatrixRot[] = {
        1.f, 0.f, 0.f, 0.f,
        0.f, 0.f, 1.f, 0.f,
        0.f, -1.f, 0.f, 0.f,
        0.f, 0.f, 0.f, 1.f,
    };

    u8 primRed, primGreen, primBlue;
    u8 envRed, envGreen, envBlue;
    const Shader* shader;

    shader = &kShaders[shaderId];

    switch (shader->lists[0] & 0xffff)
    {
    case 0x1240:
        /* Emerald */
        primRed = 0xff;
        primGreen = 0xff;
        primBlue = 0xa0;
        envRed = 0x00;
        envGreen = 0xff;
        envBlue = 0x00;
        break;
    case 0x20a0:
        /* Ruby */
        primRed = 0xff;
        primGreen = 0xaa;
        primBlue = 0xff;
        envRed = 0xff;
        envGreen = 0x00;
        envBlue = 100;
        break;
    case 0x3530:
        /* Sapphire */
        primRed = 0x32;
        primGreen = 0xff;
        primBlue = 0xff;
        envRed = 0x32;
        envGreen = 0;
        envBlue = 0x96;
        break;
    }

    OPEN_DISPS(gs->gfx);

    /* Matrix setup */
    gSPMatrix(POLY_XLU_DISP++, GetMatrixMV(gs->gfx), G_MTX_PUSH | G_MTX_LOAD | G_MTX_MODELVIEW);
    gSPMatrix(POLY_XLU_DISP++, pushMatrix(gs->gfx, kMatrixRot), G_MTX_NOPUSH | G_MTX_MUL | G_MTX_MODELVIEW);
    gSPMatrix(POLY_OPA_DISP++, GetMatrixMV(gs->gfx), G_MTX_PUSH | G_MTX_LOAD | G_MTX_MODELVIEW);
    gSPMatrix(POLY_OPA_DISP++, pushMatrix(gs->gfx, kMatrixRot), G_MTX_NOPUSH | G_MTX_MUL | G_MTX_MODELVIEW);

    /* Segment setup */
    gSPSegment(POLY_XLU_DISP++, 9, dummySegment(gs->gfx));
    gSPSegment(POLY_OPA_DISP++, 8, dummySegment(gs->gfx));

    InitListPolyXlu(gs->gfx);
    gDPSetPrimColor(POLY_XLU_DISP++, 0x00, 0x80, primRed, primGreen, primBlue, 0xFF);
    gDPSetEnvColor(POLY_XLU_DISP++, envRed, envGreen, envBlue, 0xFF);
    gSPDisplayList(POLY_XLU_DISP++, shader->lists[0]);

    InitListPolyOpa(gs->gfx);
    gDPSetPrimColor(POLY_OPA_DISP++, 0x00, 0x80, 0xff, 0xff, 0xaa, 0xff);
    gDPSetEnvColor(POLY_OPA_DISP++, 0x96, 0x78, 0x00, 0xFF);
    gSPDisplayList(POLY_OPA_DISP++, shader->lists[1]);

    CLOSE_DISPS();
}

void Shader_MasterSword(GameState* gs, s16 shaderId)
{
    /* TODO: Pre-multiply the matrices */
    static const float scale = 0.07f;
    static const float kMatrixScale[] = {
        scale, 0.f, 0.f, 0.f,
        0.f, scale, 0.f, 0.f,
        0.f, 0.f, scale, 0.f,
        0.f, 0.f, 0.f,   1.f,
    };

    static const float kMatrixRot[] = {
        -M_SQRT1_2, -M_SQRT1_2, 0.f, 0.f,
        M_SQRT1_2, -M_SQRT1_2, 0.f, 0.f,
        0.f, 0.f, 1.f, 0.f,
        0.f, 0.f, 0.f, 1.f,
    };

    const Shader* shader;

    shader = &kShaders[shaderId];

    OPEN_DISPS(gs->gfx);
    gSPMatrix(POLY_OPA_DISP++, GetMatrixMV(gs->gfx), G_MTX_PUSH | G_MTX_LOAD | G_MTX_MODELVIEW);
    gSPMatrix(POLY_OPA_DISP++, pushMatrix(gs->gfx, kMatrixRot), G_MTX_NOPUSH | G_MTX_MUL | G_MTX_MODELVIEW);
    gSPMatrix(POLY_OPA_DISP++, pushMatrix(gs->gfx, kMatrixScale), G_MTX_NOPUSH | G_MTX_MUL | G_MTX_MODELVIEW);
    gSPSegment(POLY_OPA_DISP++, 8, dummySegment(gs->gfx));
    InitListPolyOpa(gs->gfx);
    gSPDisplayList(POLY_OPA_DISP++, shader->lists[0]);
    CLOSE_DISPS();
}

void Shader_CustomSpin(GameState* gs, s16 shaderId)
{
    const Shader* shader;

    shader = &kShaders[shaderId];
    float rot;


    rot = (gs->frameCount * 0.01f);
    OPEN_DISPS(gs->gfx);
    ModelViewRotateX(rot * 3, MAT_MUL);
    ModelViewRotateY(rot * 5, MAT_MUL);
    ModelViewRotateZ(rot * 7, MAT_MUL);
    gSPMatrix(POLY_OPA_DISP++, GetMatrixMV(gs->gfx), G_MTX_PUSH | G_MTX_LOAD | G_MTX_MODELVIEW);
    InitListPolyOpa(gs->gfx);
    gSPDisplayList(POLY_OPA_DISP++, shader->lists[0]);
    gSPDisplayList(POLY_OPA_DISP++, shader->lists[1]);

    InitListPolyXlu(gs->gfx);
    shaderFlameEffect(gs, 2);
    CLOSE_DISPS();
}

const Shader kShaders[] = {
#include "data/shaders.inc"
};