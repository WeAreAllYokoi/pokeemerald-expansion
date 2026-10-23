#ifndef GUARD_CONFIG_TEXT_H
#define GUARD_CONFIG_TEXT_H

// Text settings:
#define ADMG_DEFAULT_AUTO_SCROLL_TEXT             FALSE   // If TRUE, text will automatically scroll to the next line after NUM_FRAMES_AUTO_SCROLL_DELAY. Players can still press A_BUTTON or B_BUTTON to scroll on their own.
#define AUTO_SCROLL_TEXT ADMG_CONFIG_EXTRA(AUTO_SCROLL_TEXT)
#define ADMG_DEFAULT_NUM_FRAMES_AUTO_SCROLL_DELAY 49
#define NUM_FRAMES_AUTO_SCROLL_DELAY ADMG_CONFIG_EXTRA(NUM_FRAMES_AUTO_SCROLL_DELAY)

// A note on the modifiers: they are roughly multiplicative, so having them set at 1 is vanilla speed. They also are used to calculate frame delays for the speed of the scroll effect and the animated down arrow, so to that end, they are capped at 31 to prevent the text printing from desyncing with A/B button inputs.
// From testing, a value of 18 to 20 is essentially equivalent to instant text.
#define ADMG_DEFAULT_TEXT_SPEED_SLOW_MODIFIER     1       // How fast the SLOW text speed option prints
#define TEXT_SPEED_SLOW_MODIFIER ADMG_CONFIG_EXTRA(TEXT_SPEED_SLOW_MODIFIER)
#define ADMG_DEFAULT_TEXT_SPEED_MEDIUM_MODIFIER   1       // How fast the MID text speed option prints
#define TEXT_SPEED_MEDIUM_MODIFIER ADMG_CONFIG_EXTRA(TEXT_SPEED_MEDIUM_MODIFIER)
#define ADMG_DEFAULT_TEXT_SPEED_FAST_MODIFIER     1       // How fast the FAST text speed option prints
#define TEXT_SPEED_FAST_MODIFIER ADMG_CONFIG_EXTRA(TEXT_SPEED_FAST_MODIFIER)
#define ADMG_DEFAULT_TEXT_SPEED_INSTANT_MODIFIER  12      // Needed only for the animation delays
#define TEXT_SPEED_INSTANT_MODIFIER ADMG_CONFIG_EXTRA(TEXT_SPEED_INSTANT_MODIFIER)
#define ADMG_DEFAULT_TEXT_SPEED_INSTANT           FALSE   // Renders all text as fast as it can, basically instant. Overrides FLAG_TEXT_SPEED_INSTANT and in-game player options menu setting.
#define TEXT_SPEED_INSTANT ADMG_CONFIG_EXTRA(TEXT_SPEED_INSTANT)

// Text speed flag:
#define FLAG_TEXT_SPEED_INSTANT      0       // Use this if you want to toggle instant text speed

#endif // GUARD_CONFIG_TEXT_H
