#include <combo.h>
#include <combo/item.h>
#include <combo/player.h>

void EnItem00_InitWrapper(Actor_EnItem00* this, GameState_Play* play)
{
    /* Forward */
    EnItem00_Init(this, play);

    /* Set extended fields */
    bzero(&this->xflag, sizeof(this->xflag));
    this->xflagGi = 0;
    this->isExtended = 0;
}

static void EnItem00_ItemQuery(ComboItemQuery* q, Actor_EnItem00* this, GameState_Play* play, s16 gi)
{
    memset(q, 0, sizeof(*q));

    if (this->isExtended)
    {
        comboXflagItemQuery(q, &this->xflag, gi);
        return;
    }

    switch (this->base.variable)
    {
    case 0x06:
        q->ovType = OV_COLLECTIBLE;
        q->sceneId = play->sceneId;
        q->id = this->collectibleFlag;
        q->gi = GI_OOT_HEART_PIECE;
        break;
    case 0x11:
        q->ovType = OV_COLLECTIBLE;
        q->sceneId = play->sceneId;
        q->id = this->collectibleFlag;
        q->gi = GI_OOT_SMALL_KEY;
        break;
    default:
        q->ovType = OV_NONE;
        q->gi = gi;
        break;
    }
}

void EnItem00_GiveItemDefaultRange(Actor_EnItem00* this, GameState_Play* play, s16 gi)
{
    ComboItemQuery q;
    Actor_Player* link;
    s16 itemId;

    EnItem00_ItemQuery(&q, this, play, gi);
    link = GET_LINK(play);
    itemId = -1;

    if (q.ovType == OV_NONE)
    {
        itemId = kExtendedGetItems[gi - 1].itemId;
        if (GetItemCollectBehavior(itemId) == 0xff)
            itemId = -1;
    }

    if (itemId >= 0)
    {
        this->base.attachedA = &link->base;
        AddItemWithIcon(play, link, &kExtendedGetItems[gi - 1]);
        return;
    }

     comboGiveItem(&this->base, play, &q, 50.f, 10.f);
}

PATCH_CALL(0x80012e4c, EnItem00_GiveItemDefaultRange);

void EnItem00_DrawHeartPieceSmallKey(Actor_EnItem00* this, GameState_Play* play)
{
    ComboItemQuery q;
    ComboItemOverride o;
    float scale;

    switch (this->base.variable)
    {
    case 0x06:
        scale = 12.5f;
        break;
    case 0x11:
        scale = 8.3333f;
        break;
    default:
        UNREACHABLE();
    }

    EnItem00_ItemQuery(&q, this, play, -1);
    comboItemOverride(&o, &q);
    ModelViewScale(scale, scale, scale, MAT_MUL);
    comboDrawGI(play, &this->base, o.gi, 0);
}

PATCH_FUNC(0x80013498, EnItem00_DrawHeartPieceSmallKey);

static s16 bombDrop(s16 dropId)
{
    int hasChuBag;
    int hasBombBag;
    u8  bombCount;
    u8  bombchuCount;

    if (!comboConfig(CFG_OOT_BOMBCHU_BAG))
        return dropId;

    hasChuBag = (gOotSave.inventory.items[ITS_OOT_BOMBCHU] == ITEM_OOT_BOMBCHU_10);
    hasBombBag = (gOotSave.inventory.upgrades.bombBag > 0);

    if (!hasChuBag)
    {
        if (!hasBombBag)
            return -1;
        return dropId;
    }

    if (!hasBombBag)
        return ITEM00_BOMBCHU;

    /* We have both, check for ammo */
    bombCount = gOotSave.inventory.ammo[ITS_OOT_BOMBS];
    bombchuCount = gOotSave.inventory.ammo[ITS_OOT_BOMBCHU];

    /* Low on ammo */
    if (bombCount < 15 || bombchuCount < 15)
    {
        if (bombchuCount < bombCount)
            return ITEM00_BOMBCHU;
        return dropId;
    }

    /* Not low, return at random */
    if (RandFloat() < 0.5f)
        return ITEM00_BOMBCHU;
    return dropId;
}

/* TODO: Flexible drops would ideally need to be patched on top of this */
static s16 EnItem00_FixDropWrapper(s16 dropId)
{
    switch (dropId)
    {
    case ITEM00_BOMB:
    case ITEM00_BOMBS_5:
    case ITEM00_BOMBS_5_ALT:
        dropId = bombDrop(dropId);
        break;
    default:
        break;
    }

    if (dropId == ITEM00_BOMBCHU)
        return dropId;

    return EnItem00_FixDrop(dropId);
}

PATCH_CALL(0x8001376c, EnItem00_FixDropWrapper);
PATCH_CALL(0x80013998, EnItem00_FixDropWrapper);
PATCH_CALL(0x80013dec, EnItem00_FixDropWrapper);

void EnItem00_DrawXflag(Actor_EnItem00* this, GameState_Play* play)
{
    ComboItemOverride o;

    comboXflagItemOverride(&o, &this->xflag, 0);
    ModelViewTranslate(this->base.position.x, this->base.position.y + 35.f, this->base.position.z, MAT_SET);
    ModelViewScale(0.35f, 0.35f, 0.35f, MAT_MUL);
    ModelViewRotateY(this->base.rot2.y * ((M_PI * 2.f) / 32767.f), MAT_MUL);
    comboDrawGI(play, &this->base, o.gi, 0);
}

void EnItem00_AddXflag(Actor_EnItem00* this, GameState_Play* play)
{
    ComboItemQuery q;

    if (!this->isExtended)
    {
        AddItem(play, 0x84);
        return;
    }

    EnItem00_ItemQuery(&q, this, play, 0);
    comboAddItemEx(play, &q, 0);
    comboXflagsSet(&this->xflag);
}

void EnItem00_XflagInitFreestanding(Actor_EnItem00* this, GameState_Play* play, u8 actorIndex, u8 slice)
{
    ComboItemOverride o;
    Xflag xflag;

    /* Setup the xflag */
    bzero(&xflag, sizeof(xflag));
    xflag.sceneId = play->sceneId;
    xflag.setupId = g.sceneSetupId;
    xflag.roomId = this->base.room;
    xflag.sliceId = slice;
    xflag.id = actorIndex;

    /* Query */
    comboXflagItemOverride(&o, &xflag, 0);

    /* Check if there is an override */
    if (o.gi == 0)
        return;
    if (comboXflagsGet(&xflag))
        return;

    /* It's an actual item - mark it as such */
    memcpy(&this->xflag, &xflag, sizeof(xflag));
    this->xflagGi = o.gi;
    this->isExtended = 1;

    /* Use our draw func */
    ActorSetScale(&this->base, 3.f);
    this->base.draw = EnItem00_DrawXflag;
    this->base.variable = 0;
}
