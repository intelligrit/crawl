#include "catch_amalgamated.hpp"

#include "AppHdr.h"
#include "game-type.h"
#include "hiscores.h"
#include "initfile.h"
#include "player.h"
#include "state.h"
#include "unwind.h"

#include "test_player_fixture.h"

// Every game-type switch in the codebase has a `default:` label, so a new
// mode that is missing from one compiles fine and silently behaves like a
// normal game (shared save directory, shared scorefile). These pin each one.
TEST_CASE("Onward mode is a distinct game type everywhere", "[single-file]")
{
    unwind_var<game_type> gt(crawl_state.type, GAME_TYPE_ONWARD);

    REQUIRE(crawl_state.game_is_onward());
    REQUIRE_FALSE(crawl_state.game_is_normal());
    REQUIRE_FALSE(crawl_state.game_is_sprint());
    REQUIRE_FALSE(crawl_state.game_is_descent());

    // Behaves like a normal dungeon game apart from death.
    REQUIRE(crawl_state.game_has_random_floors());
    REQUIRE(crawl_state.game_saves_prefs());

    // Never shares files with real games.
    REQUIRE(gametype_to_str(GAME_TYPE_ONWARD) == "onward");
    REQUIRE(crawl_state.game_type_name() == "Onward");
    REQUIRE(crawl_state.game_savedir_path() == "onward/");
    REQUIRE(crawl_state.game_type_qualifier() == "-onward");
}

TEST_CASE("Onward continues halve the score each time", "[single-file]")
{
    REQUIRE(apply_onward_penalty(1000, 0) == 1000);
    REQUIRE(apply_onward_penalty(1000, 1) == 500);
    REQUIRE(apply_onward_penalty(1000, 3) == 125);
    REQUIRE(apply_onward_penalty(3, 2) == 0);
    // Far more continues than bits in an int must not be undefined behaviour.
    REQUIRE(apply_onward_penalty(1000000, 100) == 0);
}

TEST_CASE("Onward continue is refused when the score cannot pay",
          "[single-file]")
{
    // Need at least 2 points: half rounds down to >= 1 spent and >= 1 kept.
    REQUIRE_FALSE(onward_can_afford_continue(0));
    REQUIRE_FALSE(onward_can_afford_continue(1));
    REQUIRE(onward_can_afford_continue(2));
    // Cost rounds up (remainder rounds down): 3 -> keep 1, spend 2.
    REQUIRE(apply_onward_penalty(3, 1) == 1);
    REQUIRE(onward_can_afford_continue(3));
}

TEST_CASE_METHOD(MockPlayerYouTestsFixture,
                 "Onward continues are counted on the player", "[single-file]")
{
    REQUIRE(you.onward_continues() == 0);
    you.props[ONWARD_CONTINUES_KEY].get_int()++;
    you.props[ONWARD_CONTINUES_KEY].get_int()++;
    REQUIRE(you.onward_continues() == 2);
}

TEST_CASE("Onward continues survive an xlog round trip and are described",
          "[single-file]")
{
    unwind_var<game_type> gt(crawl_state.type, GAME_TYPE_ONWARD);

    scorefile_entry se;
    REQUIRE(se.parse("v=0.34.1:lv=0.1:name=Test:race=Minotaur:cls=Fighter"
                     ":xl=5:sc=250:ktyp=mon:killer=an ogre:onward=2"
                     ":start=20260830000000S:end=20260830000100S"));
    REQUIRE(se.raw_string().find(":onward=2") != string::npos);
    // Two paid continues plus the death that ended the game.
    REQUIRE(hiscores_format_single_long(se, true).find("250 (3 deaths) Test")
            != string::npos);
    REQUIRE(hiscores_format_single(se).find("250 (3 deaths) Test")
            != string::npos);

    scorefile_entry win;
    REQUIRE(win.parse("v=0.34.1:lv=0.1:name=Win:race=Minotaur:cls=Fighter"
                      ":xl=27:sc=900000:ktyp=winning:onward=1"
                      ":start=20260830000000S:end=20260830000100S"));
    REQUIRE(hiscores_format_single(win).find("900000 (1 death) Win")
            != string::npos);

    scorefile_entry clean;
    REQUIRE(clean.parse("v=0.34.1:lv=0.1:name=Clean:race=Minotaur:cls=Fighter"
                        ":xl=27:sc=900000:ktyp=winning"
                        ":start=20260830000000S:end=20260830000100S"));
    REQUIRE(hiscores_format_single(clean).find("900000 (0 deaths) Clean")
            != string::npos);
}
