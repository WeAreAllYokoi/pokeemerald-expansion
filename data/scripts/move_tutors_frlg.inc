VictoryRoad_2F_EventScript_DoubleEdgeTutor::
	lock
	faceplayer
	@ Skip the tutor-used flag check when I_REUSABLE_TMS is TRUE
	callnative ADMG_ScriptTestConfig, requests_effects=1
	.4byte ADMG_CONFIG_TAG_I_REUSABLE_TMS
	goto_if_eq EventScript_AdmgReusableTmsSkip1
	goto_if_set FLAG_TUTOR_DOUBLE_EDGE, EventScript_DoubleEdgeTaught
EventScript_AdmgReusableTmsSkip1:
	msgbox Text_DoubleEdgeTeach, MSGBOX_YESNO
	goto_if_eq VAR_RESULT, NO, EventScript_DoubleEdgeDeclined
	call EventScript_CanOnlyBeLearnedOnce
	goto_if_eq VAR_RESULT, NO, EventScript_DoubleEdgeDeclined
	msgbox Text_DoubleEdgeWhichMon
	setvar VAR_0x8005, MOVE_DOUBLE_EDGE
	call EventScript_ChooseMoveTutorMon
	goto_if_eq VAR_RESULT, FALSE, EventScript_DoubleEdgeDeclined
	@ Don't set the tutor-used flag when I_REUSABLE_TMS is TRUE
	callnative ADMG_ScriptTestConfig, requests_effects=1
	.4byte ADMG_CONFIG_TAG_I_REUSABLE_TMS
	goto_if_eq EventScript_AdmgReusableTmsSkip2
	setflag FLAG_TUTOR_DOUBLE_EDGE
EventScript_AdmgReusableTmsSkip2:
	goto EventScript_DoubleEdgeTaught
	end

EventScript_DoubleEdgeDeclined::
	msgbox Text_DoubleEdgeDeclined
	release
	end

EventScript_DoubleEdgeTaught::
	msgbox Text_DoubleEdgeTaught
	release
	end

EventScript_ThunderWaveTutor::
	lock
	faceplayer
	@ Skip the tutor-used flag check when I_REUSABLE_TMS is TRUE
	callnative ADMG_ScriptTestConfig, requests_effects=1
	.4byte ADMG_CONFIG_TAG_I_REUSABLE_TMS
	goto_if_eq EventScript_AdmgReusableTmsSkip3
	goto_if_set FLAG_TUTOR_THUNDER_WAVE, EventScript_ThunderWaveTaught
EventScript_AdmgReusableTmsSkip3:
	msgbox Text_ThunderWaveTeach, MSGBOX_YESNO
	goto_if_eq VAR_RESULT, NO, EventScript_ThunderWaveDeclined
	call EventScript_CanOnlyBeLearnedOnce
	goto_if_eq VAR_RESULT, NO, EventScript_ThunderWaveDeclined
	msgbox Text_ThunderWaveWhichMon
	setvar VAR_0x8005, MOVE_THUNDER_WAVE
	call EventScript_ChooseMoveTutorMon
	goto_if_eq VAR_RESULT, FALSE, EventScript_ThunderWaveDeclined
	@ Don't set the tutor-used flag when I_REUSABLE_TMS is TRUE
	callnative ADMG_ScriptTestConfig, requests_effects=1
	.4byte ADMG_CONFIG_TAG_I_REUSABLE_TMS
	goto_if_eq EventScript_AdmgReusableTmsSkip4
	setflag FLAG_TUTOR_THUNDER_WAVE
EventScript_AdmgReusableTmsSkip4:
	goto EventScript_ThunderWaveTaught
	end

EventScript_ThunderWaveDeclined::
	msgbox Text_ThunderWaveDeclined
	release
	end

EventScript_ThunderWaveTaught::
	msgbox Text_ThunderWaveTaught
	release
	end

RockTunnel_B1F_EventScript_RockSlideTutor::
	lock
	faceplayer
	@ Skip the tutor-used flag check when I_REUSABLE_TMS is TRUE
	callnative ADMG_ScriptTestConfig, requests_effects=1
	.4byte ADMG_CONFIG_TAG_I_REUSABLE_TMS
	goto_if_eq EventScript_AdmgReusableTmsSkip5
	goto_if_set FLAG_TUTOR_ROCK_SLIDE, EventScript_RockSlideTaught
EventScript_AdmgReusableTmsSkip5:
	msgbox Text_RockSlideTeach, MSGBOX_YESNO
	goto_if_eq VAR_RESULT, NO, EventScript_RockSlideDeclined
	call EventScript_CanOnlyBeLearnedOnce
	goto_if_eq VAR_RESULT, NO, EventScript_RockSlideDeclined
	msgbox Text_RockSlideWhichMon
	setvar VAR_0x8005, MOVE_ROCK_SLIDE
	call EventScript_ChooseMoveTutorMon
	goto_if_eq VAR_RESULT, FALSE, EventScript_RockSlideDeclined
	@ Don't set the tutor-used flag when I_REUSABLE_TMS is TRUE
	callnative ADMG_ScriptTestConfig, requests_effects=1
	.4byte ADMG_CONFIG_TAG_I_REUSABLE_TMS
	goto_if_eq EventScript_AdmgReusableTmsSkip6
	setflag FLAG_TUTOR_ROCK_SLIDE
EventScript_AdmgReusableTmsSkip6:
	goto EventScript_RockSlideTaught
	end

EventScript_RockSlideDeclined::
	msgbox Text_RockSlideDeclined
	release
	end

EventScript_RockSlideTaught::
	msgbox Text_RockSlideTaught
	release
	end

MtEmber_Exterior_EventScript_ExplosionTutor::
	lock
	faceplayer
	@ Skip the tutor-used flag check when I_REUSABLE_TMS is TRUE
	callnative ADMG_ScriptTestConfig, requests_effects=1
	.4byte ADMG_CONFIG_TAG_I_REUSABLE_TMS
	goto_if_eq EventScript_AdmgReusableTmsSkip7
	goto_if_set FLAG_TUTOR_EXPLOSION, EventScript_ExplosionTaught
EventScript_AdmgReusableTmsSkip7:
	msgbox Text_ExplosionTeach, MSGBOX_YESNO
	goto_if_eq VAR_RESULT, NO, EventScript_ExplosionDeclined
	call EventScript_CanOnlyBeLearnedOnce
	goto_if_eq VAR_RESULT, NO, EventScript_ExplosionDeclined
	msgbox Text_ExplosionWhichMon
	setvar VAR_0x8005, MOVE_EXPLOSION
	call EventScript_ChooseMoveTutorMon
	goto_if_eq VAR_RESULT, FALSE, EventScript_ExplosionDeclined
	@ Don't set the tutor-used flag when I_REUSABLE_TMS is TRUE
	callnative ADMG_ScriptTestConfig, requests_effects=1
	.4byte ADMG_CONFIG_TAG_I_REUSABLE_TMS
	goto_if_eq EventScript_AdmgReusableTmsSkip8
	setflag FLAG_TUTOR_EXPLOSION
EventScript_AdmgReusableTmsSkip8:
	goto EventScript_ExplosionTaught
	end

EventScript_ExplosionDeclined::
	msgbox Text_ExplosionDeclined
	release
	end

EventScript_ExplosionTaught::
	msgbox Text_ExplosionTaught
	release
	end

Route4_EventScript_MegaPunchTutor::
	lock
	faceplayer
	@ Skip the tutor-used flag check when I_REUSABLE_TMS is TRUE
	callnative ADMG_ScriptTestConfig, requests_effects=1
	.4byte ADMG_CONFIG_TAG_I_REUSABLE_TMS
	goto_if_eq EventScript_AdmgReusableTmsSkip9
	goto_if_set FLAG_TUTOR_MEGA_PUNCH, EventScript_MegaPunchTaught
EventScript_AdmgReusableTmsSkip9:
	msgbox Text_MegaPunchTeach, MSGBOX_YESNO
	goto_if_eq VAR_RESULT, NO, EventScript_MegaPunchDeclined
	call EventScript_CanOnlyBeLearnedOnce
	goto_if_eq VAR_RESULT, NO, EventScript_MegaPunchDeclined
	msgbox Text_MegaPunchWhichMon
	setvar VAR_0x8005, MOVE_MEGA_PUNCH
	call EventScript_ChooseMoveTutorMon
	goto_if_eq VAR_RESULT, FALSE, EventScript_MegaPunchDeclined
	@ Don't set the tutor-used flag when I_REUSABLE_TMS is TRUE
	callnative ADMG_ScriptTestConfig, requests_effects=1
	.4byte ADMG_CONFIG_TAG_I_REUSABLE_TMS
	goto_if_eq EventScript_AdmgReusableTmsSkip10
	setflag FLAG_TUTOR_MEGA_PUNCH
EventScript_AdmgReusableTmsSkip10:
	goto EventScript_MegaPunchTaught
	end

EventScript_MegaPunchDeclined::
	msgbox Text_MegaPunchDeclined
	release
	end

EventScript_MegaPunchTaught::
	msgbox Text_MegaPunchTaught
	release
	end

Route4_EventScript_MegaKickTutor::
	lock
	faceplayer
	@ Skip the tutor-used flag check when I_REUSABLE_TMS is TRUE
	callnative ADMG_ScriptTestConfig, requests_effects=1
	.4byte ADMG_CONFIG_TAG_I_REUSABLE_TMS
	goto_if_eq EventScript_AdmgReusableTmsSkip11
	goto_if_set FLAG_TUTOR_MEGA_KICK, EventScript_MegaKickTaught
EventScript_AdmgReusableTmsSkip11:
	msgbox Text_MegaKickTeach, MSGBOX_YESNO
	goto_if_eq VAR_RESULT, NO, EventScript_MegaKickDeclined
	call EventScript_CanOnlyBeLearnedOnce
	goto_if_eq VAR_RESULT, NO, EventScript_MegaKickDeclined
	msgbox Text_MegaKickWhichMon
	setvar VAR_0x8005, MOVE_MEGA_KICK
	call EventScript_ChooseMoveTutorMon
	goto_if_eq VAR_RESULT, FALSE, EventScript_MegaKickDeclined
	@ Don't set the tutor-used flag when I_REUSABLE_TMS is TRUE
	callnative ADMG_ScriptTestConfig, requests_effects=1
	.4byte ADMG_CONFIG_TAG_I_REUSABLE_TMS
	goto_if_eq EventScript_AdmgReusableTmsSkip12
	setflag FLAG_TUTOR_MEGA_KICK
EventScript_AdmgReusableTmsSkip12:
	goto EventScript_MegaKickTaught
	end

EventScript_MegaKickDeclined::
	msgbox Text_MegaKickDeclined
	release
	end

EventScript_MegaKickTaught::
	msgbox Text_MegaKickTaught
	release
	end

EventScript_DreamEaterTutor::
	lock
	faceplayer
	@ Skip the tutor-used flag check when I_REUSABLE_TMS is TRUE
	callnative ADMG_ScriptTestConfig, requests_effects=1
	.4byte ADMG_CONFIG_TAG_I_REUSABLE_TMS
	goto_if_eq EventScript_AdmgReusableTmsSkip13
	goto_if_set FLAG_TUTOR_DREAM_EATER, EventScript_DreamEaterTaught
EventScript_AdmgReusableTmsSkip13:
	msgbox Text_DreamEaterTeach, MSGBOX_YESNO
	goto_if_eq VAR_RESULT, NO, EventScript_DreamEaterDeclined
	call EventScript_CanOnlyBeLearnedOnce
	goto_if_eq VAR_RESULT, NO, EventScript_DreamEaterDeclined
	msgbox Text_DreamEaterWhichMon
	setvar VAR_0x8005, MOVE_DREAM_EATER
	call EventScript_ChooseMoveTutorMon
	goto_if_eq VAR_RESULT, FALSE, EventScript_DreamEaterDeclined
	@ Don't set the tutor-used flag when I_REUSABLE_TMS is TRUE
	callnative ADMG_ScriptTestConfig, requests_effects=1
	.4byte ADMG_CONFIG_TAG_I_REUSABLE_TMS
	goto_if_eq EventScript_AdmgReusableTmsSkip14
	setflag FLAG_TUTOR_DREAM_EATER
EventScript_AdmgReusableTmsSkip14:
	goto EventScript_DreamEaterTaught
	end

EventScript_DreamEaterDeclined::
	msgbox Text_DreamEaterDeclined
	release
	end

EventScript_DreamEaterTaught::
	msgbox Text_DreamEaterTaught
	release
	end

EventScript_SoftboiledTutor::
	lock
	faceplayer
	@ Skip the tutor-used flag check when I_REUSABLE_TMS is TRUE
	callnative ADMG_ScriptTestConfig, requests_effects=1
	.4byte ADMG_CONFIG_TAG_I_REUSABLE_TMS
	goto_if_eq EventScript_AdmgReusableTmsSkip15
	goto_if_set FLAG_TUTOR_SOFT_BOILED, EventScript_SoftboiledTaught
EventScript_AdmgReusableTmsSkip15:
	msgbox Text_SoftboiledTeach, MSGBOX_YESNO
	goto_if_eq VAR_RESULT, NO, EventScript_SoftboiledDeclined
	call EventScript_CanOnlyBeLearnedOnce
	goto_if_eq VAR_RESULT, NO, EventScript_SoftboiledDeclined
	msgbox Text_SoftboiledWhichMon
	setvar VAR_0x8005, MOVE_SOFT_BOILED
	call EventScript_ChooseMoveTutorMon
	goto_if_eq VAR_RESULT, FALSE, EventScript_SoftboiledDeclined
	@ Don't set the tutor-used flag when I_REUSABLE_TMS is TRUE
	callnative ADMG_ScriptTestConfig, requests_effects=1
	.4byte ADMG_CONFIG_TAG_I_REUSABLE_TMS
	goto_if_eq EventScript_AdmgReusableTmsSkip16
	setflag FLAG_TUTOR_SOFT_BOILED
EventScript_AdmgReusableTmsSkip16:
	goto EventScript_SoftboiledTaught
	end

EventScript_SoftboiledDeclined::
	msgbox Text_SoftboiledDeclined
	release
	end

EventScript_SoftboiledTaught::
	msgbox Text_SoftboiledTaught
	release
	end

FuchsiaCity_EventScript_SubstituteTutor::
	lock
	faceplayer
	@ Skip the tutor-used flag check when I_REUSABLE_TMS is TRUE
	callnative ADMG_ScriptTestConfig, requests_effects=1
	.4byte ADMG_CONFIG_TAG_I_REUSABLE_TMS
	goto_if_eq EventScript_AdmgReusableTmsSkip17
	goto_if_set FLAG_TUTOR_SUBSTITUTE, EventScript_SubstituteTaught
EventScript_AdmgReusableTmsSkip17:
	msgbox Text_SubstituteTeach, MSGBOX_YESNO
	goto_if_eq VAR_RESULT, NO, EventScript_SubstituteDeclined
	call EventScript_CanOnlyBeLearnedOnce
	goto_if_eq VAR_RESULT, NO, EventScript_SubstituteDeclined
	msgbox Text_SubstituteWhichMon
	setvar VAR_0x8005, MOVE_SUBSTITUTE
	call EventScript_ChooseMoveTutorMon
	goto_if_eq VAR_RESULT, FALSE, EventScript_SubstituteDeclined
	@ Don't set the tutor-used flag when I_REUSABLE_TMS is TRUE
	callnative ADMG_ScriptTestConfig, requests_effects=1
	.4byte ADMG_CONFIG_TAG_I_REUSABLE_TMS
	goto_if_eq EventScript_AdmgReusableTmsSkip18
	setflag FLAG_TUTOR_SUBSTITUTE
EventScript_AdmgReusableTmsSkip18:
	goto EventScript_SubstituteTaught
	end

EventScript_SubstituteDeclined::
	msgbox Text_SubstituteDeclined
	release
	end

EventScript_SubstituteTaught::
	msgbox Text_SubstituteTaught
	release
	end

SevenIsland_EventScript_SwordsDanceTutor::
	lock
	faceplayer
	@ Skip the tutor-used flag check when I_REUSABLE_TMS is TRUE
	callnative ADMG_ScriptTestConfig, requests_effects=1
	.4byte ADMG_CONFIG_TAG_I_REUSABLE_TMS
	goto_if_eq EventScript_AdmgReusableTmsSkip19
	goto_if_set FLAG_TUTOR_SWORDS_DANCE, EventScript_SwordsDanceTaught
EventScript_AdmgReusableTmsSkip19:
	msgbox Text_SwordsDanceTeach, MSGBOX_YESNO
	goto_if_eq VAR_RESULT, NO, EventScript_SwordsDanceDeclined
	call EventScript_CanOnlyBeLearnedOnce
	goto_if_eq VAR_RESULT, NO, EventScript_SwordsDanceDeclined
	msgbox Text_SwordsDanceWhichMon
	setvar VAR_0x8005, MOVE_SWORDS_DANCE
	call EventScript_ChooseMoveTutorMon
	goto_if_eq VAR_RESULT, FALSE, EventScript_SwordsDanceDeclined
	@ Don't set the tutor-used flag when I_REUSABLE_TMS is TRUE
	callnative ADMG_ScriptTestConfig, requests_effects=1
	.4byte ADMG_CONFIG_TAG_I_REUSABLE_TMS
	goto_if_eq EventScript_AdmgReusableTmsSkip20
	setflag FLAG_TUTOR_SWORDS_DANCE
EventScript_AdmgReusableTmsSkip20:
	goto EventScript_SwordsDanceTaught
	end

EventScript_SwordsDanceDeclined::
	msgbox Text_SwordsDanceDeclined
	release
	end

EventScript_SwordsDanceTaught::
	msgbox Text_SwordsDanceTaught
	release
	end

PewterCity_Museum_1F_EventScript_SeismicTossTutor::
	lock
	faceplayer
	@ Skip the tutor-used flag check when I_REUSABLE_TMS is TRUE
	callnative ADMG_ScriptTestConfig, requests_effects=1
	.4byte ADMG_CONFIG_TAG_I_REUSABLE_TMS
	goto_if_eq EventScript_AdmgReusableTmsSkip21
	goto_if_set FLAG_TUTOR_SEISMIC_TOSS, EventScript_SeismicTossTaught
EventScript_AdmgReusableTmsSkip21:
	msgbox Text_SeismicTossTeach, MSGBOX_YESNO
	goto_if_eq VAR_RESULT, NO, EventScript_SeismicTossDeclined
	call EventScript_CanOnlyBeLearnedOnce
	goto_if_eq VAR_RESULT, NO, EventScript_SeismicTossDeclined
	msgbox Text_SeismicTossWhichMon
	setvar VAR_0x8005, MOVE_SEISMIC_TOSS
	call EventScript_ChooseMoveTutorMon
	goto_if_eq VAR_RESULT, FALSE, EventScript_SeismicTossDeclined
	@ Don't set the tutor-used flag when I_REUSABLE_TMS is TRUE
	callnative ADMG_ScriptTestConfig, requests_effects=1
	.4byte ADMG_CONFIG_TAG_I_REUSABLE_TMS
	goto_if_eq EventScript_AdmgReusableTmsSkip22
	setflag FLAG_TUTOR_SEISMIC_TOSS
EventScript_AdmgReusableTmsSkip22:
	goto EventScript_SeismicTossTaught
	end

EventScript_SeismicTossDeclined::
	msgbox Text_SeismicTossDeclined
	release
	end

EventScript_SeismicTossTaught::
	msgbox Text_SeismicTossTaught
	release
	end

EventScript_CounterTutor::
	lock
	faceplayer
	@ Skip the tutor-used flag check when I_REUSABLE_TMS is TRUE
	callnative ADMG_ScriptTestConfig, requests_effects=1
	.4byte ADMG_CONFIG_TAG_I_REUSABLE_TMS
	goto_if_eq EventScript_AdmgReusableTmsSkip23
	goto_if_set FLAG_TUTOR_COUNTER, EventScript_CounterTaught
EventScript_AdmgReusableTmsSkip23:
	msgbox Text_CounterTeach, MSGBOX_YESNO
	goto_if_eq VAR_RESULT, NO, EventScript_CounterDeclined
	call EventScript_CanOnlyBeLearnedOnce
	goto_if_eq VAR_RESULT, NO, EventScript_CounterDeclined
	msgbox Text_CounterWhichMon
	setvar VAR_0x8005, MOVE_COUNTER
	call EventScript_ChooseMoveTutorMon
	goto_if_eq VAR_RESULT, FALSE, EventScript_CounterDeclined
	@ Don't set the tutor-used flag when I_REUSABLE_TMS is TRUE
	callnative ADMG_ScriptTestConfig, requests_effects=1
	.4byte ADMG_CONFIG_TAG_I_REUSABLE_TMS
	goto_if_eq EventScript_AdmgReusableTmsSkip24
	setflag FLAG_TUTOR_COUNTER
EventScript_AdmgReusableTmsSkip24:
	goto EventScript_CounterTaught
	end

EventScript_CounterDeclined::
	msgbox Text_CounterDeclined
	release
	end

EventScript_CounterTaught::
	msgbox Text_CounterTaught
	release
	end

EventScript_MetronomeTutor::
	lock
	faceplayer
	@ Skip the tutor-used flag check when I_REUSABLE_TMS is TRUE
	callnative ADMG_ScriptTestConfig, requests_effects=1
	.4byte ADMG_CONFIG_TAG_I_REUSABLE_TMS
	goto_if_eq EventScript_AdmgReusableTmsSkip25
	goto_if_set FLAG_TUTOR_METRONOME, EventScript_MetronomeTaught
EventScript_AdmgReusableTmsSkip25:
	msgbox Text_MetronomeTeach, MSGBOX_YESNO
	goto_if_eq VAR_RESULT, NO, EventScript_MetronomeDeclined
	call EventScript_CanOnlyBeLearnedOnce
	goto_if_eq VAR_RESULT, NO, EventScript_MetronomeDeclined
	msgbox Text_MetronomeWhichMon
	setvar VAR_0x8005, MOVE_METRONOME
	call EventScript_ChooseMoveTutorMon
	goto_if_eq VAR_RESULT, FALSE, EventScript_MetronomeDeclined
	@ Don't set the tutor-used flag when I_REUSABLE_TMS is TRUE
	callnative ADMG_ScriptTestConfig, requests_effects=1
	.4byte ADMG_CONFIG_TAG_I_REUSABLE_TMS
	goto_if_eq EventScript_AdmgReusableTmsSkip26
	setflag FLAG_TUTOR_METRONOME
EventScript_AdmgReusableTmsSkip26:
	goto EventScript_MetronomeTaught
	end

EventScript_MetronomeDeclined::
	msgbox Text_MetronomeDeclined
	release
	end

EventScript_MetronomeTaught::
	msgbox Text_MetronomeTaught
	release
	end

EventScript_MimicTutor::
	@ Skip the tutor-used flag check when I_REUSABLE_TMS is TRUE
	callnative ADMG_ScriptTestConfig, requests_effects=1
	.4byte ADMG_CONFIG_TAG_I_REUSABLE_TMS
	goto_if_eq EventScript_AdmgReusableTmsSkip27
	goto_if_set FLAG_TUTOR_MIMIC, EventScript_MimicTaught
EventScript_AdmgReusableTmsSkip27:
	msgbox Text_MimicTeach, MSGBOX_YESNO
	goto_if_eq VAR_RESULT, NO, EventScript_MimicDeclined
	call EventScript_CanOnlyBeLearnedOnce
	goto_if_eq VAR_RESULT, NO, EventScript_MimicDeclined
	msgbox Text_MimicWhichMon
	setvar VAR_0x8005, MOVE_MIMIC
	call EventScript_ChooseMoveTutorMon
	lock
	faceplayer
	goto_if_eq VAR_RESULT, 0, EventScript_MimicDeclined
	removeitem ITEM_POKE_DOLL
	@ Don't set the tutor-used flag when I_REUSABLE_TMS is TRUE
	callnative ADMG_ScriptTestConfig, requests_effects=1
	.4byte ADMG_CONFIG_TAG_I_REUSABLE_TMS
	goto_if_eq EventScript_AdmgReusableTmsSkip28
	setflag FLAG_TUTOR_MIMIC
EventScript_AdmgReusableTmsSkip28:
	goto EventScript_MimicTaught
	end

EventScript_MimicDeclined::
	msgbox Text_MimicDeclined
	release
	end

EventScript_MimicTaught::
	checkplayergender
	call_if_eq VAR_RESULT, MALE, EventScript_MimicTaughtMale
	call_if_eq VAR_RESULT, FEMALE, EventScript_MimicTaughtFemale
	release
	end

EventScript_MimicTaughtMale::
	msgbox Text_MimicTaughtMale
	return

EventScript_MimicTaughtFemale::
	msgbox Text_MimicTaughtFemale
	return

FourIsland_House1_EventScript_BodySlamTutor::
	lock
	faceplayer
	@ Skip the tutor-used flag check when I_REUSABLE_TMS is TRUE
	callnative ADMG_ScriptTestConfig, requests_effects=1
	.4byte ADMG_CONFIG_TAG_I_REUSABLE_TMS
	goto_if_eq EventScript_AdmgReusableTmsSkip29
	goto_if_set FLAG_TUTOR_BODY_SLAM, EventScript_BodySlamTaught
EventScript_AdmgReusableTmsSkip29:
	msgbox Text_BodySlamTeach, MSGBOX_YESNO
	goto_if_eq VAR_RESULT, NO, EventScript_BodySlamDeclined
	call EventScript_CanOnlyBeLearnedOnce
	goto_if_eq VAR_RESULT, NO, EventScript_BodySlamDeclined
	msgbox Text_BodySlamWhichMon
	setvar VAR_0x8005, MOVE_BODY_SLAM
	call EventScript_ChooseMoveTutorMon
	goto_if_eq VAR_RESULT, FALSE, EventScript_BodySlamDeclined
	@ Don't set the tutor-used flag when I_REUSABLE_TMS is TRUE
	callnative ADMG_ScriptTestConfig, requests_effects=1
	.4byte ADMG_CONFIG_TAG_I_REUSABLE_TMS
	goto_if_eq EventScript_AdmgReusableTmsSkip30
	setflag FLAG_TUTOR_BODY_SLAM
EventScript_AdmgReusableTmsSkip30:
	goto EventScript_BodySlamTaught
	end

EventScript_BodySlamDeclined::
	msgbox Text_BodySlamDeclined
	release
	end

EventScript_BodySlamTaught::
	msgbox Text_BodySlamTaught
	release
	end

TwoIsland_CapeBrink_House_EventScript_StarterTutor::
	lock
	faceplayer
	goto_if_set FLAG_LEARNED_ALL_MOVES_AT_CAPE_BRINK, CapeBrinkTutor_EventScript_TaughtAllMoves
	goto_if_set FLAG_TEMP_2, CapeBrinkTutor_EventScript_MoveJustTaught
	bufferleadmonspeciesname STR_VAR_1
	msgbox Text_UltimateMoveThatMon
	specialvar VAR_RESULT, CapeBrinkGetMoveToTeachLeadPokemon
	goto_if_eq VAR_RESULT, FALSE, CapeBrinkTutor_EventScript_NoLeadStarter
	copyvar VAR_0x8009, VAR_0x8005
	call_if_eq VAR_FACING, DIR_NORTH, CapeBrinkTutor_EventScript_JumpInPlaceDown
	call_if_eq VAR_FACING, DIR_SOUTH, CapeBrinkTutor_EventScript_JumpInPlaceUp
	call_if_eq VAR_FACING, DIR_EAST, CapeBrinkTutor_EventScript_JumpInPlaceLeft
	call_if_eq VAR_FACING, DIR_WEST, CapeBrinkTutor_EventScript_JumpInPlaceRight
	msgbox Text_AllowMeToTeachMonUltimateMove, MSGBOX_YESNO
	goto_if_eq VAR_RESULT, NO, CapeBrinkTutor_EventScript_DeclineMove
	call EventScript_CanOnlyBeLearnedOnce
	goto_if_eq VAR_RESULT, NO, CapeBrinkTutor_EventScript_DeclineMove
	msgbox Text_LetMeConferUltimateMove
	closemessage
	fadescreen FADE_TO_BLACK
	goto CapeBrinkTutor_EventScript_ChooseMon

CapeBrinkTutor_EventScript_MoveJustTaught::
	msgbox Text_MakeBestUseOfMove
	release
	end

CapeBrinkTutor_EventScript_TaughtAllMoves::
	msgbox Text_FeelRejuvenated
	release
	end

CapeBrinkTutor_EventScript_TaughtMove::
	copyvar VAR_0x8005, VAR_0x8009
	specialvar VAR_RESULT, HasLearnedAllMovesFromCapeBrinkTutor
	goto_if_eq VAR_RESULT, TRUE, CapeBrinkTutor_EventScript_LearnedAllMoves
	msgbox Text_TaughtMove
	setflag FLAG_TEMP_2
	release
	end

CapeBrinkTutor_EventScript_LearnedAllMoves::
	msgbox Text_PassedOnEverythingIKnow
	setflag FLAG_LEARNED_ALL_MOVES_AT_CAPE_BRINK
	release
	end

CapeBrinkTutor_EventScript_ChooseMon::
	call EventScript_ChooseMoveTutorMon
	goto_if_eq VAR_RESULT, FALSE, CapeBrinkTutor_EventScript_DeclineMove
	goto CapeBrinkTutor_EventScript_TaughtMove
	end

CapeBrinkTutor_EventScript_JumpInPlaceDown::
	applymovement LOCALID_CAPE_BRINK_TUTOR, Movement_JumpInPlaceDown
	waitmovement 0
	return

CapeBrinkTutor_EventScript_JumpInPlaceUp::
	applymovement LOCALID_CAPE_BRINK_TUTOR, Movement_JumpInPlaceUp
	waitmovement 0
	return

CapeBrinkTutor_EventScript_JumpInPlaceLeft::
	applymovement LOCALID_CAPE_BRINK_TUTOR, Movement_JumpInPlaceLeft
	waitmovement 0
	return

CapeBrinkTutor_EventScript_JumpInPlaceRight::
	applymovement LOCALID_CAPE_BRINK_TUTOR, Movement_JumpInPlaceRight
	waitmovement 0
	return

CapeBrinkTutor_EventScript_DeclineMove::
	msgbox Text_YouRejectIt
	release
	end

CapeBrinkTutor_EventScript_NoLeadStarter::
	msgbox Text_JustMistaken
	release
	end

EventScript_ChooseMoveTutorMon::
	special ChooseMonForMoveTutor
	lock
	faceplayer
	return

EventScript_CanOnlyBeLearnedOnce::
	textcolor NPC_TEXT_COLOR_NEUTRAL
	special DisableMsgBoxWalkaway
	signmsg
	msgbox Text_MoveCanOnlyBeLearnedOnce, MSGBOX_YESNO
	normalmsg
	call EventScript_RestorePrevTextColor
	return

Movement_JumpInPlaceDown:
	jump_in_place_down
	step_end

Movement_JumpInPlaceUp:
	jump_in_place_up
	step_end

Movement_JumpInPlaceLeft:
	jump_in_place_left
	step_end

Movement_JumpInPlaceRight:
	jump_in_place_right
	step_end

Text_MoveCanOnlyBeLearnedOnce::
	.string "This move can be learned only\n"
	.string "once. Is that okay?$"
