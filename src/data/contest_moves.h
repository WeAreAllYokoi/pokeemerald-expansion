// Keep both versions of these descriptions available for gContestEffects
// C_UPDATED_MOVE_EFFECTS selects the Gen 6+ text or the earlier text
// Retain both strings in each pair, whichever one the compiled table uses
const u8 gADMG_ContestEffectDescription_HIGHLY_APPEALING[] = _("Quite the appealing move.");
const u8 gADMG_ContestEffectDescription_HIGHLY_APPEALING_Gen1To5[] = _("A highly appealing move.");
const u8 gADMG_ContestEffectDescription_STARTLE_PREV_MON[] = _("Startles the last Pokémon\nto act before the user.");
const u8 gADMG_ContestEffectDescription_STARTLE_PREV_MON_Gen1To5[] = _("Startles the POKéMON that\nappealed before the user.");
const u8 gADMG_ContestEffectDescription_STARTLE_PREV_MONS[] = _("Startles all of the Pokémon\nto act before the user.");
const u8 gADMG_ContestEffectDescription_STARTLE_PREV_MONS_Gen1To5[] = _("Startles all POKéMON that\nhave done their appeals.");
const u8 gADMG_ContestEffectDescription_SHIFT_JUDGE_ATTENTION[] = _("Makes audience expect\nlittle of other contestants.");
const u8 gADMG_ContestEffectDescription_SHIFT_JUDGE_ATTENTION_Gen1To5[] = _("Shifts the JUDGE's\nattention from others.");
const u8 gADMG_ContestEffectDescription_MAKE_FOLLOWING_MONS_NERVOUS[] = _("Makes the remaining\nPokémon nervous.");
const u8 gADMG_ContestEffectDescription_MAKE_FOLLOWING_MONS_NERVOUS_Gen1To5[] = _("Makes all POKéMON after\nthe user nervous.");
const u8 gADMG_ContestEffectDescription_QUALITY_DEPENDS_ON_TIMING[] = _("Effectiveness varies\ndepending on when it is used.");
const u8 gADMG_ContestEffectDescription_QUALITY_DEPENDS_ON_TIMING_Gen1To5[] = _("The appeal's quality varies\ndepending on its timing.");
const u8 gADMG_ContestEffectDescription_AFFECTED_BY_PREV_APPEAL[] = _("Affected by how well the\nprevious Pokémon's move went.");
const u8 gADMG_ContestEffectDescription_AFFECTED_BY_PREV_APPEAL_Gen1To5[] = _("Affected by how well the\nappeal in front goes.");
const u8 gADMG_ContestEffectDescription_NEXT_APPEAL_EARLIER[] = _("Causes the user to move\nearlier on the next turn.");
const u8 gADMG_ContestEffectDescription_NEXT_APPEAL_EARLIER_Gen1To5[] = _("The next appeal can be\nmade earlier next turn.");
const u8 gADMG_ContestEffectDescription_NEXT_APPEAL_LATER[] = _("Causes the user to move\nlater on the next turn.");
const u8 gADMG_ContestEffectDescription_NEXT_APPEAL_LATER_Gen1To5[] = _("The next appeal can be\nmade later next turn.");
ADMG_RETAIN_DATA(gADMG_ContestEffectDescription_HIGHLY_APPEALING);
ADMG_RETAIN_DATA(gADMG_ContestEffectDescription_HIGHLY_APPEALING_Gen1To5);
ADMG_RETAIN_DATA(gADMG_ContestEffectDescription_STARTLE_PREV_MON);
ADMG_RETAIN_DATA(gADMG_ContestEffectDescription_STARTLE_PREV_MON_Gen1To5);
ADMG_RETAIN_DATA(gADMG_ContestEffectDescription_STARTLE_PREV_MONS);
ADMG_RETAIN_DATA(gADMG_ContestEffectDescription_STARTLE_PREV_MONS_Gen1To5);
ADMG_RETAIN_DATA(gADMG_ContestEffectDescription_SHIFT_JUDGE_ATTENTION);
ADMG_RETAIN_DATA(gADMG_ContestEffectDescription_SHIFT_JUDGE_ATTENTION_Gen1To5);
ADMG_RETAIN_DATA(gADMG_ContestEffectDescription_MAKE_FOLLOWING_MONS_NERVOUS);
ADMG_RETAIN_DATA(gADMG_ContestEffectDescription_MAKE_FOLLOWING_MONS_NERVOUS_Gen1To5);
ADMG_RETAIN_DATA(gADMG_ContestEffectDescription_QUALITY_DEPENDS_ON_TIMING);
ADMG_RETAIN_DATA(gADMG_ContestEffectDescription_QUALITY_DEPENDS_ON_TIMING_Gen1To5);
ADMG_RETAIN_DATA(gADMG_ContestEffectDescription_AFFECTED_BY_PREV_APPEAL);
ADMG_RETAIN_DATA(gADMG_ContestEffectDescription_AFFECTED_BY_PREV_APPEAL_Gen1To5);
ADMG_RETAIN_DATA(gADMG_ContestEffectDescription_NEXT_APPEAL_EARLIER);
ADMG_RETAIN_DATA(gADMG_ContestEffectDescription_NEXT_APPEAL_EARLIER_Gen1To5);
ADMG_RETAIN_DATA(gADMG_ContestEffectDescription_NEXT_APPEAL_LATER);
ADMG_RETAIN_DATA(gADMG_ContestEffectDescription_NEXT_APPEAL_LATER_Gen1To5);

const struct ContestEffect gContestEffects[] =
{
    [CONTEST_EFFECT_HIGHLY_APPEALING] =
    {
        .description = C_UPDATED_MOVE_EFFECTS >= GEN_6 ? gADMG_ContestEffectDescription_HIGHLY_APPEALING : gADMG_ContestEffectDescription_HIGHLY_APPEALING_Gen1To5,
        .effectType = CONTEST_EFFECT_TYPE_APPEAL,
        .appeal = 40,
        .jam = 0,
        .function = ContestEffect_HighlyAppealing,
    },
    [CONTEST_EFFECT_USER_MORE_EASILY_STARTLED] =
    {
        .description = COMPOUND_STRING("After this move, the user is\nmore easily startled."),
        .effectType = CONTEST_EFFECT_TYPE_APPEAL,
        .appeal = 60,
        .jam = 0,
        .function = ContestEffect_UserMoreEasilyStartled,
    },
    [CONTEST_EFFECT_GREAT_APPEAL_BUT_NO_MORE_MOVES] =
    {
        .description = COMPOUND_STRING("Makes a great appeal, but\nallows no more to the end."),
        .effectType = CONTEST_EFFECT_TYPE_APPEAL,
        .appeal = 80,
        .jam = 0,
        .function = ContestEffect_GreatAppealButNoMoreMoves,
    },
    [CONTEST_EFFECT_REPETITION_NOT_BORING] =
    {
        .description = COMPOUND_STRING("Can be repeatedly used\nwithout boring the JUDGE."),
        .effectType = CONTEST_EFFECT_TYPE_APPEAL,
        .appeal = 30,
        .jam = 0,
        .function = ContestEffect_RepetitionNotBoring,
    },
    [CONTEST_EFFECT_AVOID_STARTLE_ONCE] =
    {
        .description = COMPOUND_STRING("Can avoid being startled\nby others once."),
        .effectType = CONTEST_EFFECT_TYPE_AVOID_STARTLE,
        .appeal = 20,
        .jam = 0,
        .function = ContestEffect_AvoidStartleOnce,
    },
    [CONTEST_EFFECT_AVOID_STARTLE] =
    {
        .description = COMPOUND_STRING("Can avoid being startled\nby others."),
        .effectType = CONTEST_EFFECT_TYPE_AVOID_STARTLE,
        .appeal = 10,
        .jam = 0,
        .function = ContestEffect_AvoidStartle,
    },
    [CONTEST_EFFECT_AVOID_STARTLE_SLIGHTLY] =
    {
        .description = COMPOUND_STRING("Can avoid being startled\nby others a little."),
        .effectType = CONTEST_EFFECT_TYPE_AVOID_STARTLE,
        .appeal = 30,
        .jam = 0,
        .function = ContestEffect_AvoidStartleSlightly,
    },
    [CONTEST_EFFECT_USER_LESS_EASILY_STARTLED] =
    {
        .description = COMPOUND_STRING("After this move, the user is\nless likely to be startled."),
        .effectType = CONTEST_EFFECT_TYPE_AVOID_STARTLE,
        .appeal = 30,
        .jam = 0,
        .function = ContestEffect_UserLessEasilyStartled,
    },
    [CONTEST_EFFECT_STARTLE_FRONT_MON] =
    {
        .description = COMPOUND_STRING("Slightly startles the\nPOKéMON in front."),
        .effectType = CONTEST_EFFECT_TYPE_STARTLE_MON,
        .appeal = 30,
        .jam = 20,
        .function = ContestEffect_StartleFrontMon,
    },
    [CONTEST_EFFECT_SLIGHTLY_STARTLE_PREV_MONS] =
    {
        .description = COMPOUND_STRING("Slightly startles those\nthat have made appeals."),
        .effectType = CONTEST_EFFECT_TYPE_STARTLE_MONS,
        .appeal = 30,
        .jam = 10,
        .function = ContestEffect_StartlePrevMons,
    },
    [CONTEST_EFFECT_STARTLE_PREV_MON] =
    {
        .description = C_UPDATED_MOVE_EFFECTS >= GEN_6 ? gADMG_ContestEffectDescription_STARTLE_PREV_MON : gADMG_ContestEffectDescription_STARTLE_PREV_MON_Gen1To5,
        .effectType = CONTEST_EFFECT_TYPE_STARTLE_MON,
        .appeal = 20,
        .jam = 30,
        .function = ContestEffect_StartleFrontMon,
    },
    [CONTEST_EFFECT_STARTLE_PREV_MONS] =
    {
        .description = C_UPDATED_MOVE_EFFECTS >= GEN_6 ? gADMG_ContestEffectDescription_STARTLE_PREV_MONS : gADMG_ContestEffectDescription_STARTLE_PREV_MONS_Gen1To5,
        .effectType = CONTEST_EFFECT_TYPE_STARTLE_MONS,
        .appeal = 20,
        .jam = 20,
        .function = ContestEffect_StartlePrevMons,
    },
    [CONTEST_EFFECT_BADLY_STARTLE_FRONT_MON] =
    {
        .description = COMPOUND_STRING("Badly startles the\nPOKéMON in front."),
        .effectType = CONTEST_EFFECT_TYPE_STARTLE_MON,
        .appeal = 10,
        .jam = 40,
        .function = ContestEffect_StartleFrontMon,
    },
    [CONTEST_EFFECT_BADLY_STARTLE_PREV_MONS] =
    {
        .description = COMPOUND_STRING("Badly startles those that\nhave made appeals."),
        .effectType = CONTEST_EFFECT_TYPE_STARTLE_MONS,
        .appeal = 10,
        .jam = 30,
        .function = ContestEffect_StartlePrevMons,
    },
    [CONTEST_EFFECT_STARTLE_PREV_MON_2] =
    {
        .description = COMPOUND_STRING("Startles the POKéMON that\nappealed before the user."),
        .effectType = CONTEST_EFFECT_TYPE_STARTLE_MON,
        .appeal = 30,
        .jam = 20,
        .function = ContestEffect_StartlePrevMon2,
    },
    [CONTEST_EFFECT_STARTLE_PREV_MONS_2] =
    {
        .description = COMPOUND_STRING("Startles all POKéMON that\nhave done their appeals."),
        .effectType = CONTEST_EFFECT_TYPE_STARTLE_MONS,
        .appeal = 30,
        .jam = 10,
        .function = ContestEffect_StartlePrevMons2,
    },
    [CONTEST_EFFECT_SHIFT_JUDGE_ATTENTION] =
    {
        .description = C_UPDATED_MOVE_EFFECTS >= GEN_6 ? gADMG_ContestEffectDescription_SHIFT_JUDGE_ATTENTION : gADMG_ContestEffectDescription_SHIFT_JUDGE_ATTENTION_Gen1To5,
        .effectType = CONTEST_EFFECT_TYPE_WORSEN,
        .appeal = 30,
        .jam = 0,
        .function = ContestEffect_ShiftJudgeAttention,
    },
    [CONTEST_EFFECT_STARTLE_MON_WITH_JUDGES_ATTENTION] =
    {
        .description = COMPOUND_STRING("Startles the POKéMON that\nhas the JUDGE's attention."),
        .effectType = CONTEST_EFFECT_TYPE_STARTLE_MONS,
        .appeal = 20,
        .jam = 10,
        .function = ContestEffect_StartleMonWithJudgesAttention,
    },
    [CONTEST_EFFECT_JAMS_OTHERS_BUT_MISS_ONE_TURN] =
    {
        .description = COMPOUND_STRING("Jams the others, and misses\none turn of appeals."),
        .effectType = CONTEST_EFFECT_TYPE_STARTLE_MONS,
        .appeal = 40,
        .jam = 40,
        .function = ContestEffect_JamsOthersButMissOneTurn,
    },
    [CONTEST_EFFECT_STARTLE_MONS_SAME_TYPE_APPEAL] =
    {
        .description = COMPOUND_STRING("Startles POKéMON that\nmade a same-type appeal."),
        .effectType = CONTEST_EFFECT_TYPE_STARTLE_MONS,
        .appeal = 20,
        .jam = 10,
        .function = ContestEffect_StartleMonsSameTypeAppeal,
    },
    [CONTEST_EFFECT_STARTLE_MONS_COOL_APPEAL] =
    {
        .description = COMPOUND_STRING("Badly startles POKéMON\nthat made COOL appeals."),
        .effectType = CONTEST_EFFECT_TYPE_STARTLE_MONS,
        .appeal = 20,
        .jam = 10,
        .function = ContestEffect_StartleMonsCoolAppeal,
    },
    [CONTEST_EFFECT_STARTLE_MONS_BEAUTY_APPEAL] =
    {
        .description = COMPOUND_STRING("Badly startles POKéMON\nthat made BEAUTY appeals."),
        .effectType = CONTEST_EFFECT_TYPE_STARTLE_MONS,
        .appeal = 20,
        .jam = 10,
        .function = ContestEffect_StartleMonsBeautyAppeal,
    },
    [CONTEST_EFFECT_STARTLE_MONS_CUTE_APPEAL] =
    {
        .description = COMPOUND_STRING("Badly startles POKéMON\nthat made CUTE appeals."),
        .effectType = CONTEST_EFFECT_TYPE_STARTLE_MONS,
        .appeal = 20,
        .jam = 10,
        .function = ContestEffect_StartleMonsCuteAppeal,
    },
    [CONTEST_EFFECT_STARTLE_MONS_SMART_APPEAL] =
    {
        .description = COMPOUND_STRING("Badly startles POKéMON\nthat made SMART appeals."),
        .effectType = CONTEST_EFFECT_TYPE_STARTLE_MONS,
        .appeal = 20,
        .jam = 10,
        .function = ContestEffect_StartleMonsSmartAppeal,
    },
    [CONTEST_EFFECT_STARTLE_MONS_TOUGH_APPEAL] =
    {
        .description = COMPOUND_STRING("Badly startles POKéMON\nthat made TOUGH appeals."),
        .effectType = CONTEST_EFFECT_TYPE_STARTLE_MONS,
        .appeal = 20,
        .jam = 10,
        .function = ContestEffect_StartleMonsToughAppeal,
    },
    [CONTEST_EFFECT_MAKE_FOLLOWING_MON_NERVOUS] =
    {
        .description = COMPOUND_STRING("Makes one POKéMON after\nthe user nervous."),
        .effectType = CONTEST_EFFECT_TYPE_WORSEN,
        .appeal = 20,
        .jam = 0,
        .function = ContestEffect_MakeFollowingMonNervous,
    },
    [CONTEST_EFFECT_MAKE_FOLLOWING_MONS_NERVOUS] =
    {
        .description = C_UPDATED_MOVE_EFFECTS >= GEN_6 ? gADMG_ContestEffectDescription_MAKE_FOLLOWING_MONS_NERVOUS : gADMG_ContestEffectDescription_MAKE_FOLLOWING_MONS_NERVOUS_Gen1To5,
        .effectType = CONTEST_EFFECT_TYPE_WORSEN,
        .appeal = 20,
        .jam = 0,
        .function = ContestEffect_MakeFollowingMonsNervous,
    },
    [CONTEST_EFFECT_WORSEN_CONDITION_OF_PREV_MONS] =
    {
        .description = COMPOUND_STRING("Worsens the condition of\nthose that made appeals."),
        .effectType = CONTEST_EFFECT_TYPE_WORSEN,
        .appeal = 30,
        .jam = 0,
        .function = ContestEffect_WorsenConditionOfPrevMons,
    },
    [CONTEST_EFFECT_BADLY_STARTLES_MONS_IN_GOOD_CONDITION] =
    {
        .description = COMPOUND_STRING("Badly startles POKéMON in\ngood condition."),
        .effectType = CONTEST_EFFECT_TYPE_STARTLE_MONS,
        .appeal = 30,
        .jam = 10,
        .function = ContestEffect_BadlyStartlesMonsInGoodCondition,
    },
    [CONTEST_EFFECT_BETTER_IF_FIRST] =
    {
        .description = COMPOUND_STRING("The appeal works great if\nperformed first."),
        .effectType = CONTEST_EFFECT_TYPE_SPECIAL_APPEAL,
        .appeal = 20,
        .jam = 0,
        .function = ContestEffect_BetterIfFirst,
    },
    [CONTEST_EFFECT_BETTER_IF_LAST] =
    {
        .description = COMPOUND_STRING("The appeal works great if\nperformed last."),
        .effectType = CONTEST_EFFECT_TYPE_SPECIAL_APPEAL,
        .appeal = 20,
        .jam = 0,
        .function = ContestEffect_BetterIfLast,
    },
    [CONTEST_EFFECT_APPEAL_AS_GOOD_AS_PREV_ONES] =
    {
        .description = COMPOUND_STRING("Makes the appeal as good\nas those before it."),
        .effectType = CONTEST_EFFECT_TYPE_SPECIAL_APPEAL,
        .appeal = 10,
        .jam = 0,
        .function = ContestEffect_AppealAsGoodAsPrevOnes,
    },
    [CONTEST_EFFECT_APPEAL_AS_GOOD_AS_PREV_ONE] =
    {
        .description = COMPOUND_STRING("Makes the appeal as good\nas the one before it."),
        .effectType = CONTEST_EFFECT_TYPE_SPECIAL_APPEAL,
        .appeal = 10,
        .jam = 0,
        .function = ContestEffect_AppealAsGoodAsPrevOne,
    },
    [CONTEST_EFFECT_BETTER_WHEN_LATER] =
    {
        .description = COMPOUND_STRING("The appeal works better\nthe later it is performed."),
        .effectType = CONTEST_EFFECT_TYPE_SPECIAL_APPEAL,
        .appeal = 10,
        .jam = 0,
        .function = ContestEffect_BetterWhenLater,
    },
    [CONTEST_EFFECT_QUALITY_DEPENDS_ON_TIMING] =
    {
        .description = C_UPDATED_MOVE_EFFECTS >= GEN_6 ? gADMG_ContestEffectDescription_QUALITY_DEPENDS_ON_TIMING : gADMG_ContestEffectDescription_QUALITY_DEPENDS_ON_TIMING_Gen1To5,
        .effectType = CONTEST_EFFECT_TYPE_SPECIAL_APPEAL,
        .appeal = 10,
        .jam = 0,
        .function = ContestEffect_QualityDependsOnTiming,
    },
    [CONTEST_EFFECT_BETTER_IF_SAME_TYPE] =
    {
        .description = COMPOUND_STRING("Works well if it's the same\ntype as the one before."),
        .effectType = CONTEST_EFFECT_TYPE_SPECIAL_APPEAL,
        .appeal = 20,
        .jam = 0,
        .function = ContestEffect_BetterIfSameType,
    },
    [CONTEST_EFFECT_BETTER_IF_DIFF_TYPE] =
    {
        .description = COMPOUND_STRING("Works well if different in\ntype than the one before."),
        .effectType = CONTEST_EFFECT_TYPE_SPECIAL_APPEAL,
        .appeal = 20,
        .jam = 0,
        .function = ContestEffect_BetterIfDiffType,
    },
    [CONTEST_EFFECT_AFFECTED_BY_PREV_APPEAL] =
    {
        .description = C_UPDATED_MOVE_EFFECTS >= GEN_6 ? gADMG_ContestEffectDescription_AFFECTED_BY_PREV_APPEAL : gADMG_ContestEffectDescription_AFFECTED_BY_PREV_APPEAL_Gen1To5,
        .effectType = CONTEST_EFFECT_TYPE_SPECIAL_APPEAL,
        .appeal = 30,
        .jam = 0,
        .function = ContestEffect_AffectedByPrevAppeal,
    },
    [CONTEST_EFFECT_IMPROVE_CONDITION_PREVENT_NERVOUSNESS] =
    {
        .description = COMPOUND_STRING("Ups the user's condition.\nHelps prevent nervousness."),
        .effectType = CONTEST_EFFECT_TYPE_SPECIAL_APPEAL,
        .appeal = 10,
        .jam = 0,
        .function = ContestEffect_ImproveConditionPreventNervousness,
    },
    [CONTEST_EFFECT_BETTER_WITH_GOOD_CONDITION] =
    {
        .description = COMPOUND_STRING("The appeal works well if the\nuser's condition is good."),
        .effectType = CONTEST_EFFECT_TYPE_SPECIAL_APPEAL,
        .appeal = 10,
        .jam = 0,
        .function = ContestEffect_BetterWithGoodCondition,
    },
    [CONTEST_EFFECT_NEXT_APPEAL_EARLIER] =
    {
        .description = C_UPDATED_MOVE_EFFECTS >= GEN_6 ? gADMG_ContestEffectDescription_NEXT_APPEAL_EARLIER : gADMG_ContestEffectDescription_NEXT_APPEAL_EARLIER_Gen1To5,
        .effectType = CONTEST_EFFECT_TYPE_TURN_ORDER,
        .appeal = 30,
        .jam = 0,
        .function = ContestEffect_NextAppealEarlier,
    },
    [CONTEST_EFFECT_NEXT_APPEAL_LATER] =
    {
        .description = C_UPDATED_MOVE_EFFECTS >= GEN_6 ? gADMG_ContestEffectDescription_NEXT_APPEAL_LATER : gADMG_ContestEffectDescription_NEXT_APPEAL_LATER_Gen1To5,
        .effectType = CONTEST_EFFECT_TYPE_TURN_ORDER,
        .appeal = 30,
        .jam = 0,
        .function = ContestEffect_NextAppealLater,
    },
    [CONTEST_EFFECT_MAKE_SCRAMBLING_TURN_ORDER_EASIER] =
    {
        .description = COMPOUND_STRING("Makes the next turn's order\nmore easily scrambled."),
        .effectType = CONTEST_EFFECT_TYPE_TURN_ORDER,
        .appeal = 30,
        .jam = 0,
        .function = ContestEffect_MakeScramblingTurnOrderEasier,
    },
    [CONTEST_EFFECT_SCRAMBLE_NEXT_TURN_ORDER] =
    {
        .description = COMPOUND_STRING("Scrambles the order of\nappeals on the next turn."),
        .effectType = CONTEST_EFFECT_TYPE_TURN_ORDER,
        .appeal = 30,
        .jam = 0,
        .function = ContestEffect_ScrambleNextTurnOrder,
    },
    [CONTEST_EFFECT_EXCITE_AUDIENCE_IN_ANY_CONTEST] =
    {
        .description = COMPOUND_STRING("An appeal that excites the\naudience in any CONTEST."),
        .effectType = CONTEST_EFFECT_TYPE_SPECIAL_APPEAL,
        .appeal = C_UPDATED_MOVE_EFFECTS >= GEN_6 ? 20 : 10,
        .jam = 0,
        .function = ContestEffect_ExciteAudienceInAnyContest,
    },
    [CONTEST_EFFECT_BADLY_STARTLE_MONS_WITH_GOOD_APPEALS] =
    {
        .description = COMPOUND_STRING("Badly startles all POKéMON\nthat made good appeals."),
        .effectType = CONTEST_EFFECT_TYPE_STARTLE_MONS,
        .appeal = 20,
        .jam = 10,
        .function = ContestEffect_BadlyStartleMonsWithGoodAppeals,
    },
    [CONTEST_EFFECT_BETTER_WHEN_AUDIENCE_EXCITED] =
    {
        .description = COMPOUND_STRING("The appeal works best the\nmore the crowd is excited."),
        .effectType = CONTEST_EFFECT_TYPE_SPECIAL_APPEAL,
        .appeal = 10,
        .jam = 0,
        .function = ContestEffect_BetterWhenAudienceExcited,
    },
    [CONTEST_EFFECT_DONT_EXCITE_AUDIENCE] =
    {
        .description = COMPOUND_STRING("Temporarily stops the\ncrowd from growing excited."),
        .effectType = CONTEST_EFFECT_TYPE_WORSEN,
        .appeal = 30,
        .jam = 0,
        .function = ContestEffect_DontExciteAudience,
    },
};
