PewterCity_Mart_Frlg_MapScripts::
	.byte 0

PewterCity_Mart_EventScript_Youngster::
	msgbox PewterCity_Mart_Text_BoughtWeirdFishFromShadyGuy, MSGBOX_NPC
	end

PewterCity_Mart_EventScript_Boy::
	msgbox PewterCity_Mart_Text_GoodThingsIfRaiseMonsDiligently, MSGBOX_NPC
	end

PewterCity_Mart_EventScript_Clerk::
	lock
	faceplayer
	message gText_HowMayIServeYou
	waitmessage
	pokemart PewterCity_Mart_Items
	msgbox gText_PleaseComeAgain
	release
	end

	.align 2
PewterCity_Mart_Items::
	.4byte ITEM_POKE_BALL
	.4byte ITEM_POTION
	.4byte ITEM_ANTIDOTE
	.4byte ITEM_PARALYZE_HEAL
	.4byte ITEM_AWAKENING
	.4byte ITEM_BURN_HEAL
	.4byte ITEM_ESCAPE_ROPE
	.4byte ITEM_REPEL
	.4byte ITEM_NONE
	release
	end

PewterCity_Mart_Text_BoughtWeirdFishFromShadyGuy::
	.string "A shady old guy sucked me into\n"
	.string "buying this weird fish POKéMON!\p"
	.string "It's totally weak and it cost ¥500!$"

PewterCity_Mart_Text_GoodThingsIfRaiseMonsDiligently::
	.string "Good things can happen if you raise\n"
	.string "POKéMON diligently.\p"
	.string "Even the weak ones can surprise\n"
	.string "you if you don't give up on them.$"

