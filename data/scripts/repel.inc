.set LOCAL_VAR_SPRAY, VAR_0x8004
.set LOCAL_VAR_NUM_SPRAY_STRENGTH, VAR_0x8005
.set LOCAL_VAR_SPRAY_TYPE, VAR_0x8009
.set LOCAL_VAR_NEW_SPRAY, VAR_0x8007

EventScript_BufferSprayName::
	callnative ADMG_ScriptTestFeatureBinding, requests_effects=1
	.2byte ADMG_BINDING_VAR_LAST_REPEL_LURE_USED
	goto_if_ne EventScript_BufferSprayName_Unbound
	callnative ADMG_ScriptCopyFeatureVar, requests_effects=1
	.2byte ADMG_BINDING_VAR_LAST_REPEL_LURE_USED, LOCAL_VAR_SPRAY
	bufferitemname STR_VAR_1, LOCAL_VAR_SPRAY
	return
EventScript_BufferSprayName_Unbound:
	specialvar LOCAL_VAR_SPRAY_TYPE, GetLastUsedSprayType
	compare LOCAL_VAR_SPRAY_TYPE, ITEM_REPEL
	goto_if_eq EventScript_BufferSprayName_2
	bufferitemname STR_VAR_1, ITEM_LURE
EventScript_BufferSprayName_1:
	return

EventScript_BufferSprayName_2:
	bufferitemname STR_VAR_1, ITEM_REPEL
	goto EventScript_BufferSprayName_1


EventScript_SprayWoreOff::
	lock
	call EventScript_BufferSprayName
	specialvar LOCAL_VAR_NUM_SPRAY_STRENGTH, GetNumberSprayStrength
	switch LOCAL_VAR_NUM_SPRAY_STRENGTH
	case 0, EventScript_SprayWoreOffMessage
	case 1, EventScript_UseSingleSpray
	goto EventScript_HandleMultipleSprays
	return

EventScript_HandleMultipleSprays::
	goto EventScript_ChooseWhichSpray
	return


EventScript_SprayWoreOffMessage::
	msgbox Text_SprayWoreOff, MSGBOX_SIGN
	releaseall
	end


EventScript_UseSingleSpray::
	callnative ADMG_ScriptTestFeatureBinding, requests_effects=1
	.2byte ADMG_BINDING_VAR_LAST_REPEL_LURE_USED
	goto_if_ne EventScript_UseSingleSpray_Unbound
	callnative ADMG_ScriptCopyFeatureVar, requests_effects=1
	.2byte ADMG_BINDING_VAR_LAST_REPEL_LURE_USED, LOCAL_VAR_SPRAY
	checkitem LOCAL_VAR_SPRAY, 1
	compare VAR_RESULT, TRUE
	goto_if_eq EventScript_UseSingleSpray_2
EventScript_UseSingleSpray_Unbound:
	call EventScript_UseDifferentSpray
EventScript_UseSingleSpray_1:
	closemessage
	releaseall
	end

EventScript_UseSingleSpray_2:
	call EventScript_UseLastUsedSpray
	goto EventScript_UseSingleSpray_1


EventScript_SetSingleSprayAndUse::
	setvar VAR_RESULT, 0
	call EventScript_UseSpray
	return


EventScript_UseDifferentSpray::
	specialvar LOCAL_VAR_NEW_SPRAY, GetSprayId
	bufferitemname STR_VAR_2, LOCAL_VAR_NEW_SPRAY
	msgbox Text_SprayWoreOffAskUseX, MSGBOX_YESNO
	compare VAR_RESULT, YES
	goto_if_eq EventScript_UseDifferentSpray_2
EventScript_UseDifferentSpray_1:
	return

EventScript_UseDifferentSpray_2:
	copyvar LOCAL_VAR_SPRAY, LOCAL_VAR_NEW_SPRAY
	call EventScript_SetSingleSprayAndUse
	goto EventScript_UseDifferentSpray_1


EventScript_UseLastUsedSpray::
	msgbox Text_SprayWoreOffAskUseAnother, MSGBOX_YESNO
	compare VAR_RESULT, YES
	goto_if_eq EventScript_UseLastUsedSpray_2
EventScript_UseLastUsedSpray_1:
	return

EventScript_UseLastUsedSpray_2:
	callnative ADMG_ScriptCopyFeatureVar, requests_effects=1
	.2byte ADMG_BINDING_VAR_LAST_REPEL_LURE_USED, LOCAL_VAR_SPRAY
	call EventScript_SetSingleSprayAndUse
	goto EventScript_UseLastUsedSpray_1


EventScript_ChooseWhichSpray::
	message Text_SprayWoreOffAskUseAnother
	waitmessage
	callnative DrawSprayMenu
	waitstate
	compare VAR_RESULT, MULTI_B_PRESSED
	goto_if_ne EventScript_ChooseWhichSpray_3
EventScript_ChooseWhichSpray_1:
	closemessage
	releaseall
	end

EventScript_ChooseWhichSpray_2:
	call EventScript_UseSpray
	goto EventScript_ChooseWhichSpray_1

EventScript_ChooseWhichSpray_3:
	compare VAR_RESULT, VAR_0x8003
	goto_if_ne EventScript_ChooseWhichSpray_2
	goto EventScript_ChooseWhichSpray_1


EventScript_UseSpray::
	playse SE_REPEL
	waitse
	callnative HandleSprayMenuChoice
	removeitem LOCAL_VAR_SPRAY, 1
	bufferitemname STR_VAR_2, LOCAL_VAR_SPRAY
	msgbox Text_SprayWoreOffUseX, MSGBOX_SIGN
	return


Text_SprayWoreOff::
	.string "{STR_VAR_1}'s effect wore off…$"

Text_SprayWoreOffAskUseAnother::
	.string "{STR_VAR_1}'s effect wore off!\n"
	.string "Use another?$"

Text_SprayWoreOffAskUseX::
	.string "{STR_VAR_1}'s effect wore off…\n"
	.string "Use a {STR_VAR_2}?$"

Text_SprayWoreOffUseX::
	.string "{PLAYER} used the\n"
	.string "{STR_VAR_2}.$"
