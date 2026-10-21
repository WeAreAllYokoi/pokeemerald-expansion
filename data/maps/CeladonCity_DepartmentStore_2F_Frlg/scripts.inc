CeladonCity_DepartmentStore_2F_Frlg_MapScripts::
	.byte 0

CeladonCity_DepartmentStore_2F_EventScript_UnusedNPC::
	msgbox CeladonCity_DepartmentStore_2F_Text_SuperRepelMorePowerfulRepel, MSGBOX_NPC
	end

CeladonCity_DepartmentStore_2F_EventScript_Lass::
	msgbox CeladonCity_DepartmentStore_2F_Text_BuyReviveForLongOutings, MSGBOX_NPC
	end

CeladonCity_DepartmentStore_2F_EventScript_FloorSign::
	msgbox CeladonCity_DepartmentStore_2F_Text_FloorSign, MSGBOX_SIGN
	end

CeladonCity_DepartmentStore_2F_EventScript_ClerkItems::
	lock
	faceplayer
	message gText_HowMayIServeYou
	waitmessage
	pokemart CeladonCity_DepartmentStore_2F_Items
	msgbox gText_PleaseComeAgain
	release
	end

	.align 2
CeladonCity_DepartmentStore_2F_Items::
	.4byte ITEM_GREAT_BALL
	.4byte ITEM_SUPER_POTION
	.4byte ITEM_REVIVE
	.4byte ITEM_ANTIDOTE
	.4byte ITEM_PARALYZE_HEAL
	.4byte ITEM_AWAKENING
	.4byte ITEM_BURN_HEAL
	.4byte ITEM_ICE_HEAL
	.4byte ITEM_SUPER_REPEL
	.4byte ITEM_NONE
	release
	end

CeladonCity_DepartmentStore_2F_EventScript_ClerkTMs::
	lock
	faceplayer
	message gText_HowMayIServeYou
	waitmessage
	pokemart CeladonCity_DepartmentStore_2F_TMs
	msgbox gText_PleaseComeAgain
	release
	end

	.align 2
CeladonCity_DepartmentStore_2F_TMs::
	.4byte ITEM_TM05
	.4byte ITEM_TM15
	.4byte ITEM_TM28
	.4byte ITEM_TM31
	.4byte ITEM_TM43
	.4byte ITEM_TM45
	.4byte ITEM_NONE
	release
	end

CeladonCity_DepartmentStore_2F_Text_SuperRepelMorePowerfulRepel::
	.string "SUPER REPEL keeps weak POKéMON at\n"
	.string "bay…\p"
	.string "Hmm, it's a more powerful REPEL.$"

CeladonCity_DepartmentStore_2F_Text_BuyReviveForLongOutings::
	.string "For long outings, you should buy\n"
	.string "REVIVE.$"

CeladonCity_DepartmentStore_2F_Text_FloorSign::
	.string "Top-Grade Items for TRAINERS!\p"
	.string "2F: TRAINER'S MARKET$"

CeladonCity_DepartmentStore_2F_Text_LanceComesToBuyCapes::
	.string "We have a customer, LANCE, who\n"
	.string "occasionally comes.\p"
	.string "He always buys capes.\p"
	.string "I wonder… Does he have many\n"
	.string "identical capes at home?$"
