#include "global.h"
#include "admg_trainer_actor.h"
#include "admg_state.h"
#include "battle.h"
#include "battle_anim.h"
#include "battle_controllers.h"
#include "battle_interface.h"
#include "battle_message.h"
#include "battle_setup.h"
#include "battle_tower.h"
#include "battle_tv.h"
#include "bg.h"
#include "data.h"
#include "frontier_util.h"
#include "link.h"
#include "main.h"
#include "m4a.h"
#include "palette.h"
#include "pokeball.h"
#include "pokemon.h"
#include "recorded_battle.h"
#include "reshow_battle_screen.h"
#include "sound.h"
#include "string_util.h"
#include "task.h"
#include "text.h"
#include "util.h"
#include "window.h"
#include "constants/battle_anim.h"
#include "constants/songs.h"
#include "constants/trainers.h"
#include "recorded_battle.h"
#include "random.h"

static void LinkOpponentHandleDrawTrainerPic(enum BattlerId battler);
static void LinkOpponentHandleTrainerSlide(enum BattlerId battler);
static void LinkOpponentHandleTrainerSlideBack(enum BattlerId battler);
static void LinkOpponentHandleIntroTrainerBallThrow(enum BattlerId battler);
static void LinkOpponentHandleDrawPartyStatusSummary(enum BattlerId battler);
static void LinkOpponentHandleLinkStandbyMsg(enum BattlerId battler);
static void LinkOpponentHandleEndLinkBattle(enum BattlerId battler);

static void LinkOpponentBufferRunCommand(enum BattlerId battler);

static void (*const sLinkOpponentBufferCommands[CONTROLLER_CMDS_COUNT])(enum BattlerId battler) =
{
    [CONTROLLER_GETMONDATA]               = BtlController_HandleGetMonData,
    [CONTROLLER_GETRAWMONDATA]            = BtlController_Empty,
    [CONTROLLER_SETMONDATA]               = BtlController_HandleSetMonData,
    [CONTROLLER_SETRAWMONDATA]            = BtlController_HandleSetRawMonData,
    [CONTROLLER_LOADMONSPRITE]            = BtlController_HandleLoadMonSprite,
    [CONTROLLER_SWITCHINANIM]             = BtlController_HandleSwitchInAnim,
    [CONTROLLER_RETURNMONTOBALL]          = BtlController_HandleReturnMonToBall,
    [CONTROLLER_DRAWTRAINERPIC]           = LinkOpponentHandleDrawTrainerPic,
    [CONTROLLER_TRAINERSLIDE]             = LinkOpponentHandleTrainerSlide,
    [CONTROLLER_TRAINERSLIDEBACK]         = LinkOpponentHandleTrainerSlideBack,
    [CONTROLLER_FAINTANIMATION]           = BtlController_HandleFaintAnimation,
    [CONTROLLER_PALETTEFADE]              = BtlController_Empty,
    [CONTROLLER_BALLTHROWANIM]            = BtlController_Empty,
    [CONTROLLER_PAUSE]                    = BtlController_Empty,
    [CONTROLLER_MOVEANIMATION]            = BtlController_HandleMoveAnimation,
    [CONTROLLER_PRINTSTRING]              = BtlController_HandlePrintString,
    [CONTROLLER_PRINTSTRINGPLAYERONLY]    = BtlController_Empty,
    [CONTROLLER_CHOOSEACTION]             = BtlController_Empty,
    [CONTROLLER_YESNOBOX]                 = BtlController_Empty,
    [CONTROLLER_CHOOSEMOVE]               = BtlController_Empty,
    [CONTROLLER_OPENBAG]                  = BtlController_Empty,
    [CONTROLLER_CHOOSEPOKEMON]            = BtlController_Empty,
    [CONTROLLER_23]                       = BtlController_Empty,
    [CONTROLLER_HEALTHBARUPDATE]          = BtlController_HandleHealthBarUpdate,
    [CONTROLLER_EXPUPDATE]                = BtlController_Empty,
    [CONTROLLER_STATUSICONUPDATE]         = BtlController_HandleStatusIconUpdate,
    [CONTROLLER_STATUSANIMATION]          = BtlController_HandleStatusAnimation,
    [CONTROLLER_STATUSXOR]                = BtlController_Empty,
    [CONTROLLER_DATATRANSFER]             = BtlController_Empty,
    [CONTROLLER_DMA3TRANSFER]             = BtlController_Empty,
    [CONTROLLER_PLAYBGM]                  = BtlController_Empty,
    [CONTROLLER_32]                       = BtlController_Empty,
    [CONTROLLER_TWORETURNVALUES]          = BtlController_Empty,
    [CONTROLLER_CHOSENMONRETURNVALUE]     = BtlController_Empty,
    [CONTROLLER_ONERETURNVALUE]           = BtlController_Empty,
    [CONTROLLER_ONERETURNVALUE_DUPLICATE] = BtlController_Empty,
    [CONTROLLER_HITANIMATION]             = BtlController_HandleHitAnimation,
    [CONTROLLER_CANTSWITCH]               = BtlController_Empty,
    [CONTROLLER_PLAYSE]                   = BtlController_HandlePlaySE,
    [CONTROLLER_PLAYFANFAREORBGM]         = BtlController_HandlePlayFanfareOrBGM,
    [CONTROLLER_FAINTINGCRY]              = BtlController_HandleFaintingCry,
    [CONTROLLER_INTROSLIDE]               = BtlController_HandleIntroSlide,
    [CONTROLLER_INTROTRAINERBALLTHROW]    = LinkOpponentHandleIntroTrainerBallThrow,
    [CONTROLLER_DRAWPARTYSTATUSSUMMARY]   = LinkOpponentHandleDrawPartyStatusSummary,
    [CONTROLLER_HIDEPARTYSTATUSSUMMARY]   = BtlController_HandleHidePartyStatusSummary,
    [CONTROLLER_ENDBOUNCE]                = BtlController_Empty,
    [CONTROLLER_SPRITEINVISIBILITY]       = BtlController_HandleSpriteInvisibility,
    [CONTROLLER_BATTLEANIMATION]          = BtlController_HandleBattleAnimation,
    [CONTROLLER_LINKSTANDBYMSG]           = LinkOpponentHandleLinkStandbyMsg,
    [CONTROLLER_RESETACTIONMOVESELECTION] = BtlController_Empty,
    [CONTROLLER_ENDLINKBATTLE]            = LinkOpponentHandleEndLinkBattle,
    [CONTROLLER_DEBUGMENU]                = BtlController_Empty,
    [CONTROLLER_TERMINATOR_NOP]           = BtlController_TerminatorNop
};

void SetControllerToLinkOpponent(enum BattlerId battler)
{
    gBattlerBattleController[battler] = BATTLE_CONTROLLER_LINK_OPPONENT;
    gBattlerControllerEndFuncs[battler] = LinkOpponentBufferExecCompleted;
    gBattlerControllerFuncs[battler] = LinkOpponentBufferRunCommand;
}

static void LinkOpponentBufferRunCommand(enum BattlerId battler)
{
    if (IsBattleControllerActiveOnLocal(battler))
    {
        if (gBattleResources->bufferA[battler][0] < ARRAY_COUNT(sLinkOpponentBufferCommands))
            sLinkOpponentBufferCommands[gBattleResources->bufferA[battler][0]](battler);
        else
            BtlController_Complete(battler);
    }
}

static void Intro_WaitForShinyAnimAndHealthbox(enum BattlerId battler)
{
    bool32 healthboxAnimDone = FALSE;
    bool32 twoMons = FALSE;

    if (!IsDoubleBattle() || (IsDoubleBattle() && (gBattleTypeFlags & BATTLE_TYPE_MULTI)))
    {
        if (gSprites[gHealthboxSpriteIds[battler]].callback == SpriteCallbackDummy)
            healthboxAnimDone = TRUE;
    }
    else
    {
        if (gSprites[gHealthboxSpriteIds[battler]].callback == SpriteCallbackDummy
         && gSprites[gHealthboxSpriteIds[GetPartnerBattler(battler)]].callback == SpriteCallbackDummy)
        {
            healthboxAnimDone = TRUE;
        }
        twoMons = TRUE;
    }

    if (healthboxAnimDone)
    {
        if (twoMons || !IsBattlerSpriteVisible(GetPartnerBattler(battler)))
        {
            if (!gBattleSpritesDataPtr->healthBoxesData[battler].finishedShinyMonAnim)
                return;
            if (!gBattleSpritesDataPtr->healthBoxesData[GetPartnerBattler(battler)].finishedShinyMonAnim)
                return;

            gBattleSpritesDataPtr->healthBoxesData[battler].triedShinyMonAnim = FALSE;
            gBattleSpritesDataPtr->healthBoxesData[battler].finishedShinyMonAnim = FALSE;

            gBattleSpritesDataPtr->healthBoxesData[GetPartnerBattler(battler)].triedShinyMonAnim = FALSE;
            gBattleSpritesDataPtr->healthBoxesData[GetPartnerBattler(battler)].finishedShinyMonAnim = FALSE;

            FreeShinyStars();
        }
        else
        {
            if (!gBattleSpritesDataPtr->healthBoxesData[battler].finishedShinyMonAnim)
                return;

            gBattleSpritesDataPtr->healthBoxesData[battler].triedShinyMonAnim = FALSE;
            gBattleSpritesDataPtr->healthBoxesData[battler].finishedShinyMonAnim = FALSE;

            if (GetBattlerPosition(battler) == B_POSITION_OPPONENT_RIGHT)
                FreeShinyStars();
        }

        gBattleSpritesDataPtr->healthBoxesData[battler].introEndDelay = 3;
        gBattlerControllerFuncs[battler] = BtlController_Intro_DelayAndEnd;
    }
}

static void Intro_TryShinyAnimShowHealthbox(enum BattlerId battler)
{
    bool32 bgmRestored = FALSE;

    if (!gBattleSpritesDataPtr->healthBoxesData[battler].ballAnimActive
        && !gBattleSpritesDataPtr->healthBoxesData[battler].triedShinyMonAnim
        && !gBattleSpritesDataPtr->healthBoxesData[battler].finishedShinyMonAnim)
    {
        TryShinyAnimation(battler, GetBattlerMon(battler));
    }
    if (!(gBattleTypeFlags & BATTLE_TYPE_MULTI)
        && !gBattleSpritesDataPtr->healthBoxesData[GetPartnerBattler(battler)].ballAnimActive
        && !gBattleSpritesDataPtr->healthBoxesData[GetPartnerBattler(battler)].triedShinyMonAnim
        && !gBattleSpritesDataPtr->healthBoxesData[GetPartnerBattler(battler)].finishedShinyMonAnim)
    {
        TryShinyAnimation(GetPartnerBattler(battler), GetBattlerMon(GetPartnerBattler(battler)));
    }

    if (!gBattleSpritesDataPtr->healthBoxesData[battler].ballAnimActive && !gBattleSpritesDataPtr->healthBoxesData[GetPartnerBattler(battler)].ballAnimActive)
    {
        if (!gBattleSpritesDataPtr->healthBoxesData[battler].healthboxSlideInStarted)
        {
            if (IsDoubleBattle() && !(gBattleTypeFlags & BATTLE_TYPE_MULTI))
            {
                UpdateHealthboxAttribute(gHealthboxSpriteIds[GetPartnerBattler(battler)], GetBattlerMon(GetPartnerBattler(battler)), HEALTHBOX_ALL);
                StartHealthboxSlideIn(GetPartnerBattler(battler));
                SetHealthboxSpriteVisible(gHealthboxSpriteIds[GetPartnerBattler(battler)]);
            }
            UpdateHealthboxAttribute(gHealthboxSpriteIds[battler], GetBattlerMon(battler), HEALTHBOX_ALL);
            StartHealthboxSlideIn(battler);
            SetHealthboxSpriteVisible(gHealthboxSpriteIds[battler]);
        }
        gBattleSpritesDataPtr->healthBoxesData[battler].healthboxSlideInStarted = TRUE;
    }

    if (!gBattleSpritesDataPtr->healthBoxesData[battler].waitForCry
        && gBattleSpritesDataPtr->healthBoxesData[battler].healthboxSlideInStarted
        && !gBattleSpritesDataPtr->healthBoxesData[GetPartnerBattler(battler)].waitForCry
        && !IsCryPlayingOrClearCrySongs())
    {
        if (!gBattleSpritesDataPtr->healthBoxesData[battler].bgmRestored)
        {
            if (gBattleTypeFlags & BATTLE_TYPE_MULTI && gBattleTypeFlags & BATTLE_TYPE_LINK)
            {
                if (GetBattlerPosition(battler) == B_POSITION_OPPONENT_LEFT)
                    m4aMPlayContinue(&gMPlayInfo_BGM);
            }
            else
            {
                m4aMPlayVolumeControl(&gMPlayInfo_BGM, TRACKS_ALL, 0x100);
            }
        }

        gBattleSpritesDataPtr->healthBoxesData[battler].bgmRestored = TRUE;
        bgmRestored = TRUE;
    }

    if (bgmRestored)
    {
        if (gSprites[gBattleControllerData[battler]].callback == SpriteCallbackDummy
            && gSprites[gBattlerSpriteIds[battler]].callback == SpriteCallbackDummy)
        {
            if (gBattleTypeFlags & BATTLE_TYPE_MULTI && GetBattlerPosition(battler) == B_POSITION_OPPONENT_RIGHT)
            {
                if (++gBattleSpritesDataPtr->healthBoxesData[battler].introEndDelay == 1)
                    return;
                gBattleSpritesDataPtr->healthBoxesData[battler].introEndDelay = 0;
            }

            if (IsDoubleBattle() && !(gBattleTypeFlags & BATTLE_TYPE_MULTI))
            {
                DestroySprite(&gSprites[gBattleControllerData[GetPartnerBattler(battler)]]);
                SetBattlerShadowSpriteCallback(GetPartnerBattler(battler), GetMonData(GetBattlerMon(GetPartnerBattler(battler)), MON_DATA_SPECIES));
            }


            DestroySprite(&gSprites[gBattleControllerData[battler]]);
            SetBattlerShadowSpriteCallback(battler, GetBattlerVisualSpecies(battler));

            gBattleSpritesDataPtr->animationData->introAnimActive = FALSE;
            gBattleSpritesDataPtr->healthBoxesData[battler].bgmRestored = FALSE;
            gBattleSpritesDataPtr->healthBoxesData[battler].healthboxSlideInStarted = FALSE;

            gBattlerControllerFuncs[battler] = Intro_WaitForShinyAnimAndHealthbox;
        }
    }
}

void LinkOpponentBufferExecCompleted(enum BattlerId battler)
{
    gBattlerControllerFuncs[battler] = LinkOpponentBufferRunCommand;
    if (gBattleTypeFlags & BATTLE_TYPE_LINK)
    {
        u8 playerId = GetMultiplayerId();

        PrepareBufferDataTransferLink(battler, B_COMM_CONTROLLER_IS_DONE, 4, &playerId);
        gBattleResources->bufferA[battler][0] = CONTROLLER_TERMINATOR_NOP;
    }
    else
    {
        MarkBattleControllerIdleOnLocal(battler);
    }
}

static bool8 ADMG_TryGetLinkOpponentPicture(enum BattlerId battler, enum TrainerPicID *picture, s16 *x)
{
    s16 xPos;
    enum TrainerPicID trainerPicId;
    if (picture == NULL || x == NULL || (u32)battler >= MAX_BATTLERS_COUNT)
    { ADMG_StateAccessRejected((u32)battler, __func__); return FALSE; }
    struct ADMG_BattleTrainerActors snapshot;
    u64 generation = ADMG_BeginStateAccessCheck();
    if (!ADMG_TryCaptureBattleTrainerActors(gBattleTypeFlags, TRAINER_BATTLE_PARAM.opponentA,
        TRAINER_BATTLE_PARAM.opponentB, gPartnerTrainerId, &snapshot)) return FALSE;
    enum BattlerPosition position = GetBattlerPosition(battler);
    if ((u32)position >= MAX_BATTLERS_COUNT)
    { ADMG_StateAccessRejected((u32)position, __func__); return FALSE; }

    bool8 facility = (gBattleTypeFlags & BATTLE_TYPE_MULTI) && (gBattleTypeFlags & BATTLE_TYPE_BATTLE_TOWER);
    enum TrainerId identity = position == B_POSITION_OPPONENT_LEFT
        ? TRAINER_BATTLE_PARAM.opponentA : TRAINER_BATTLE_PARAM.opponentB;
    if (facility)
    {
        struct ADMG_TrainerActorRef actor;
        if (!ADMG_TryClassifyTrainerActor(identity, ADMG_ACTOR_FACILITY, ADMG_ACTOR_OPPONENT, &actor)) return FALSE;
    }
    if (gBattleTypeFlags & BATTLE_TYPE_MULTI)
    {
        if ((position & BIT_FLANK) != 0) // second mon
            xPos = 152;
        else // first mon
            xPos = 200;

        if (gBattleTypeFlags & BATTLE_TYPE_BATTLE_TOWER)
        {
            if (position == B_POSITION_OPPONENT_LEFT)
                trainerPicId = GetFrontierTrainerFrontSpriteId(TRAINER_BATTLE_PARAM.opponentA);
            else
                trainerPicId = GetFrontierTrainerFrontSpriteId(TRAINER_BATTLE_PARAM.opponentB);
        }
        else
        {
            s32 player = GetBattlerMultiplayerId(battler);
            if ((u32)player >= MAX_LINK_PLAYERS)
            { ADMG_StateAccessRejected((u32)player, __func__); return FALSE; }
            if ((gLinkPlayers[player].version & 0xFF) == VERSION_FIRE_RED
            || (gLinkPlayers[player].version & 0xFF) == VERSION_LEAF_GREEN)
            {
                if (gLinkPlayers[player].gender != MALE)
                    trainerPicId = gFacilityClassToPicIndex[FACILITY_CLASS_LEAF];
                else
                    trainerPicId = gFacilityClassToPicIndex[FACILITY_CLASS_RED];
            }
            else if ((gLinkPlayers[player].version & 0xFF) == VERSION_RUBY
                     || (gLinkPlayers[player].version & 0xFF) == VERSION_SAPPHIRE)
            {
                if (gLinkPlayers[player].gender != MALE)
                    trainerPicId = gFacilityClassToPicIndex[FACILITY_CLASS_RS_MAY];
                else
                    trainerPicId = gFacilityClassToPicIndex[FACILITY_CLASS_RS_BRENDAN];
            }
            else
            {
                trainerPicId = PlayerGenderToFrontTrainerPicId(gLinkPlayers[player].gender);
            }
        }
    }
    else
    {
        xPos = 176;
        if (TRAINER_BATTLE_PARAM.opponentA == TRAINER_UNION_ROOM)
        {
            trainerPicId = GetUnionRoomTrainerPic();
        }
        else
        {
            u32 player = GetMultiplayerId();
            if (player >= MAX_LINK_PLAYERS)
            { ADMG_StateAccessRejected(player, __func__); return FALSE; }
            player ^= BIT_SIDE;
            if ((gLinkPlayers[player].version & 0xFF) == VERSION_FIRE_RED
                     || (gLinkPlayers[player].version & 0xFF) == VERSION_LEAF_GREEN)
            {
                if (gLinkPlayers[player].gender != MALE)
                    trainerPicId = gFacilityClassToPicIndex[FACILITY_CLASS_LEAF];
                else
                    trainerPicId = gFacilityClassToPicIndex[FACILITY_CLASS_RED];
            }
            else if ((gLinkPlayers[player].version & 0xFF) == VERSION_RUBY
                     || (gLinkPlayers[player].version & 0xFF) == VERSION_SAPPHIRE)
            {
                if (gLinkPlayers[player].gender != MALE)
                    trainerPicId = gFacilityClassToPicIndex[FACILITY_CLASS_RS_MAY];
                else
                    trainerPicId = gFacilityClassToPicIndex[FACILITY_CLASS_RS_BRENDAN];
            }
            else
            {
                trainerPicId = PlayerGenderToFrontTrainerPicId(gLinkPlayers[player].gender);
            }
        }
    }

    if (!ADMG_EndStateAccessCheck(generation)
     || !ADMG_BattleTrainerActorsCurrent(&snapshot, gBattleTypeFlags,
        TRAINER_BATTLE_PARAM.opponentA, TRAINER_BATTLE_PARAM.opponentB, gPartnerTrainerId)) return FALSE;
    if (position != GetBattlerPosition(battler)
     || (facility && identity != (position == B_POSITION_OPPONENT_LEFT
        ? TRAINER_BATTLE_PARAM.opponentA : TRAINER_BATTLE_PARAM.opponentB)))
    { ADMG_StateAccessRejected(identity, __func__); return FALSE; }
    if (!IsTrainerPicIdValid(trainerPicId))
    { ADMG_StateAccessRejected((u32)trainerPicId, __func__); return FALSE; }
    *picture = trainerPicId; *x = xPos;
    return TRUE;
}

static void LinkOpponentHandleDrawTrainerPic(enum BattlerId battler)
{
    enum TrainerPicID trainerPicId; s16 xPos;
    if (!ADMG_TryGetLinkOpponentPicture(battler, &trainerPicId, &xPos)) return;
    BtlController_HandleDrawTrainerPic(battler, trainerPicId, TRUE, xPos, 40, -1);
}

static void LinkOpponentHandleTrainerSlide(enum BattlerId battler)
{
    enum TrainerPicID trainerPicId; s16 xPos;
    u64 generation = ADMG_BeginStateAccessCheck();
    if (!ADMG_TryGetLinkOpponentPicture(battler, &trainerPicId, &xPos)) return;
    BtlController_HandleTrainerSlide(battler, trainerPicId);
    if (!ADMG_EndStateAccessCheck(generation)) return;
    BtlController_Complete(battler); // Possibly a bug, because execution should be completed after the slide in finishes. See Controller_WaitForTrainerPic.
}

static void LinkOpponentHandleTrainerSlideBack(enum BattlerId battler)
{
    BtlController_HandleTrainerSlideBack(battler, 35, FALSE);
}

static void LinkOpponentHandleIntroTrainerBallThrow(enum BattlerId battler)
{
    BtlController_HandleIntroTrainerBallThrow(battler, 0, NULL, 0, Intro_TryShinyAnimShowHealthbox);
}

static void LinkOpponentHandleDrawPartyStatusSummary(enum BattlerId battler)
{
    BtlController_HandleDrawPartyStatusSummary(battler, B_SIDE_OPPONENT, TRUE);
}

static void LinkOpponentHandleLinkStandbyMsg(enum BattlerId battler)
{
    if ((u32)battler >= MAX_BATTLERS_COUNT || gBattleResources == NULL )
    { ADMG_StateAccessRejected((u32)battler, __func__); return; }
    u64 generation = ADMG_BeginStateAccessCheck();

    RecordedBattle_RecordAllBattlerData(&gBattleResources->bufferA[battler][2]);
    if (!ADMG_EndStateAccessCheck(generation)) return;
    BtlController_Complete(battler);
}

static void LinkOpponentHandleEndLinkBattle(enum BattlerId battler)
{
    if ((u32)battler >= MAX_BATTLERS_COUNT || gBattleResources == NULL || gSaveBlock2Ptr == NULL )
    { ADMG_StateAccessRejected((u32)battler, __func__); return; }
    u64 generation = ADMG_BeginStateAccessCheck();

    RecordedBattle_RecordAllBattlerData(&gBattleResources->bufferA[battler][4]);
    if (!ADMG_EndStateAccessCheck(generation)) return;

    if (gBattleResources->bufferA[battler][1] == B_OUTCOME_DREW)
        gBattleOutcome = gBattleResources->bufferA[battler][1];
    else
        gBattleOutcome = gBattleResources->bufferA[battler][1] ^ B_OUTCOME_DREW;

    gSaveBlock2Ptr->frontier.disableRecordBattle = gBattleResources->bufferA[battler][2];
    FadeOutMapMusic(5);
    BeginFastPaletteFade(3);
    BtlController_Complete(battler);
    gBattlerControllerFuncs[battler] = SetBattleEndCallbacks;
}
