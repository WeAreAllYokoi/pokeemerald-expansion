#ifndef GUARD_CONFIG_FISHING_H
#define GUARD_CONFIG_FISHING_H

#define ADMG_DEFAULT_I_FISHING_BITE_ODDS         GEN_LATEST // Base bite chances for Old, Good and Super Rods:
// Before GEN_3: 100%, 33%, 50%; GEN_3: 50% for all three; GEN_4+: 25%, 50%, 75%
#define I_FISHING_BITE_ODDS ADMG_CONFIG_EXTRA(I_FISHING_BITE_ODDS)
#define ADMG_DEFAULT_I_FISHING_MINIGAME          GEN_3      // Each generation uses a variation of reeling in Pokémon once they have been hooked. NOTE: Only the Gen 1/2 and Gen 3 minigames are implemented right now!
#define I_FISHING_MINIGAME ADMG_CONFIG_EXTRA(I_FISHING_MINIGAME)
#define ADMG_DEFAULT_I_FISHING_ENVIRONMENT       GEN_LATEST // In Gen 3, the battle environment when fighting a hooked Pokémon is based on the tile the player is standing on. In Gen 4 onwards, the environment is based on tile that is being fished in, resulting in it usually being a water environment.
#define I_FISHING_ENVIRONMENT ADMG_CONFIG_EXTRA(I_FISHING_ENVIRONMENT)
#define ADMG_DEFAULT_I_FISHING_STICKY_BOOST      GEN_LATEST // Suction Cups or Sticky Hold in the first party slot boosts bite odds, excluding Eggs
// Before GEN_4, try an 85% bite roll first, then the normal roll if it fails
// From GEN_4 onward, double the calculated bite chance, up to 100%
#define I_FISHING_STICKY_BOOST ADMG_CONFIG_EXTRA(I_FISHING_STICKY_BOOST)
#define ADMG_DEFAULT_I_FISHING_FOLLOWER_BOOST    FALSE      // Boost bite odds using the friendship of the Pokémon chosen by GetFirstLiveMon()
// That lookup follows the follower eligibility rules
#define I_FISHING_FOLLOWER_BOOST ADMG_CONFIG_EXTRA(I_FISHING_FOLLOWER_BOOST)
#define I_FISHING_CHAIN             FALSE      // Introduced in XY, hooking the same Pokémon repeatedly will increase the odds of that mon being shiny. NOTE: This implementation is an approximation of the actual feature, as XY have not been throughoutly documented or datamined.
#define ADMG_DEFAULT_I_FISHING_PROXIMITY         FALSE      // In XY, bite chance is boosted by the number of adjacent non-surfable tiles next to your fishing line
#define I_FISHING_PROXIMITY ADMG_CONFIG_EXTRA(I_FISHING_PROXIMITY)
#define ADMG_DEFAULT_I_FISHING_TIME_OF_DAY_BOOST FALSE      // In XY, bite chance is boosted during morning and evening
#define I_FISHING_TIME_OF_DAY_BOOST ADMG_CONFIG_EXTRA(I_FISHING_TIME_OF_DAY_BOOST)

#endif // GUARD_CONFIG_FISHING_H
