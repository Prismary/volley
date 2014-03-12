/*=============================================================================
Blobby Volley 2
Copyright (C) 2006 Jonathan Sieber (jonathan_sieber@yahoo.de)
Copyright (C) 2006 Daniel Knobe (daniel-knobe@web.de)

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
=============================================================================*/

/* header include */
#include "State.h"

/* includes */
#include <algorithm>
#include <cstdio>

#include "LocalGameState.h"
#include "ReplaySelectionState.h"
#include "NetworkState.h"
#include "NetworkSearchState.h"
#include "OptionsState.h"

#include "DuelMatch.h"
#include "SoundManager.h"
#include "IMGUI.h"
#include "TextManager.h"
#include "SpeedController.h"
#include "Blood.h"
#include "MatchEvents.h"
#include "PhysicWorld.h"


/* implementation */

// static state functions
State* State::mCurrentState = 0;

void State::step()
{
	getCurrentState()->step_impl();
}

State* State::getCurrentState()
{
	if (mCurrentState == 0) {
		mCurrentState = new MainMenuState;
	}
	return mCurrentState;
}

const char* State::getCurrenStateName()
{
	return getCurrentState()->getStateName();
}




State::State()
{

}

void State::deleteCurrentState()
{
	/// \todo well, the deleteCurrentState/setCurrentState as we have it now
	///			seems to have several flaws. First, we have to delete our
	///			current state BEFORE we create the next one. If I recall right
	///			this was because some destructors wrote things too disk which
	///			other constructors had to load (?), so they had to be called
	///			first.
	///			So, if the construction of the new state fails, the old is
	///			already deleted and we have now way to roll back.
	///			Second, we need two methods were one should be sufficient.
	delete mCurrentState;
	mCurrentState = 0;

	RenderManager::getSingleton().drawGame(false);
}
void State::setCurrentState(State* newState)
{
	assert(!mCurrentState);
	mCurrentState = newState;
}
/*
void switchState(State* newState)
{
	delete mCurrentState;
	mCurrentState = newState;
}
*/
void State::presentGame(const DuelMatch& match)
{
	RenderManager& rmanager = RenderManager::getSingleton();
	SoundManager& smanager = SoundManager::getSingleton();

	// enable game drawing
	rmanager.drawGame(true);


	rmanager.setBlob(LEFT_PLAYER, match.getBlobPosition(LEFT_PLAYER), match.getWorld().getBlobState(LEFT_PLAYER));
	rmanager.setBlob(RIGHT_PLAYER, match.getBlobPosition(RIGHT_PLAYER),	match.getWorld().getBlobState(RIGHT_PLAYER));

	if(match.getPlayer(LEFT_PLAYER).getOscillating())
	{
		rmanager.setBlobColor(LEFT_PLAYER, rmanager.getOscillationColor());
	}
	 else
	{
		rmanager.setBlobColor(LEFT_PLAYER, match.getPlayer(LEFT_PLAYER).getStaticColor());
	}

	if(match.getPlayer(RIGHT_PLAYER).getOscillating())
	{
		rmanager.setBlobColor(RIGHT_PLAYER, rmanager.getOscillationColor());
	}
	 else
	{
		rmanager.setBlobColor(RIGHT_PLAYER, match.getPlayer(RIGHT_PLAYER).getStaticColor());
	}

	rmanager.setBall(match.getBallPosition(), match.getWorld().getBallRotation());

	int events = match.getEvents();
	if(events & EVENT_LEFT_BLOBBY_HIT)
	{
		smanager.playSound("sounds/bums.wav", match.getWorld().getLastHitIntensity() + BALL_HIT_PLAYER_SOUND_VOLUME);
		Vector2 hitPos = match.getBallPosition() +
				(match.getBlobPosition(LEFT_PLAYER) - match.getBallPosition()).normalise().scale(31.5);
		BloodManager::getSingleton().spillBlood(hitPos, match.getWorld().getLastHitIntensity(), 0);
	}

	if (events & EVENT_RIGHT_BLOBBY_HIT)
	{
		smanager.playSound("sounds/bums.wav", match.getWorld().getLastHitIntensity() + BALL_HIT_PLAYER_SOUND_VOLUME);
		Vector2 hitPos = match.getBallPosition() +
				(match.getBlobPosition(RIGHT_PLAYER) - match.getBallPosition()).normalise().scale(31.5);
		BloodManager::getSingleton().spillBlood(hitPos, match.getWorld().getLastHitIntensity(), 1);
	}

	if (events & EVENT_ERROR)
		smanager.playSound("sounds/pfiff.wav", ROUND_START_SOUND_VOLUME);
}

void State::presentGameUI(const DuelMatch& match)
{
	auto& imgui = IMGUI::getSingleton();

	// Scores
	char textBuffer[64];
	snprintf(textBuffer, 8, match.getServingPlayer() == LEFT_PLAYER ? "%02d!" : "%02d ", match.getScore(LEFT_PLAYER));
	imgui.doText(GEN_ID, Vector2(24, 24), textBuffer);
	snprintf(textBuffer, 8, match.getServingPlayer() == RIGHT_PLAYER ? "%02d!" : "%02d ", match.getScore(RIGHT_PLAYER));
	imgui.doText(GEN_ID, Vector2(800-24, 24), textBuffer, TF_ALIGN_RIGHT);

	// blob name / time textfields
	imgui.doText(GEN_ID, Vector2(12, 550), match.getPlayer(LEFT_PLAYER).getName());
	imgui.doText(GEN_ID, Vector2(788, 550), match.getPlayer(RIGHT_PLAYER).getName(), TF_ALIGN_RIGHT);
	imgui.doText(GEN_ID, Vector2(400, 24), match.getClock().getTimeString(), TF_ALIGN_CENTER);
}

// --------------------------------------------------------------------------------------------------------------------------------
//  concrete implementation of MainMenuState and CreditsState
// -----------------------------------------------------------

MainMenuState::MainMenuState()
{
	IMGUI::getSingleton().resetSelection();

	// set main menu fps
	SpeedController::getMainInstance()->setGameSpeed(75);
}

MainMenuState::~MainMenuState()
{
}

void MainMenuState::step_impl()
{
	IMGUI& imgui = IMGUI::getSingleton();

	imgui.doCursor();
	imgui.doImage(GEN_ID, Vector2(400.0, 300.0), "background");
	imgui.doOverlay(GEN_ID, Vector2(0.0, 0.0), Vector2(800.0, 600.0));
	imgui.doImage(GEN_ID, Vector2(250.0, 210.0), "gfx/titel.bmp");
	if (imgui.doButton(GEN_ID, Vector2(434, 300.0), TextManager::MNU_LABEL_ONLINE))
	{
		deleteCurrentState();
		setCurrentState(new OnlineSearchState());
		return;
	}
	if (imgui.doButton(GEN_ID, Vector2(434, 340.0), TextManager::MNU_LABEL_LAN))
	{
		deleteCurrentState();
		setCurrentState(new LANSearchState());
		return;
	}
	if (imgui.doButton(GEN_ID, Vector2(434.0, 380.0), TextManager::MNU_LABEL_START))
	{
		try
		{
			deleteCurrentState();
			setCurrentState(new LocalGameState());
			return;
		}
		catch (const ScriptException& except)
		{
			FILE* file = fopen("lualog.txt", "wb");
			fprintf(file, "Lua Error: %s\n",
				except.luaerror.c_str());
			fclose(file);
		}
	}

	if (imgui.doButton(GEN_ID, Vector2(434.0, 420.0), TextManager::MNU_LABEL_OPTIONS))
	{
		deleteCurrentState();
		setCurrentState(new OptionState());
		return;
	}

	if (imgui.doButton(GEN_ID, Vector2(434.0, 460.0), TextManager::MNU_LABEL_REPLAY))
	{
		deleteCurrentState();
		setCurrentState(new ReplaySelectionState());
		return;
	}

	if (imgui.doButton(GEN_ID, Vector2(434.0, 500.0), TextManager::MNU_LABEL_CREDITS))
	{
		deleteCurrentState();
		setCurrentState(new CreditsState());
		return;
	}

	if (imgui.doButton(GEN_ID, Vector2(434.0, 540.0), TextManager::MNU_LABEL_EXIT))
	{
		/// \todo This is not the right way to end Blobby!
		///		We have shutdown actions in main.cpp, if we change
		///		those, we'd have to update these here too.
		///		we should have this at just one place.
		RenderManager::getSingleton().deinit();
		SoundManager::getSingleton().deinit();
		deleteCurrentState();
		SDL_Quit();
		exit(0);
	}
}

const char* MainMenuState::getStateName() const
{
	return "MainMenuState";
}

CreditsState::CreditsState()
{
	IMGUI::getSingleton().resetSelection();
	mYPosition = 600;
}

void CreditsState::step_impl()
{
	IMGUI& imgui = IMGUI::getSingleton();
	imgui.doCursor();
	imgui.doImage(GEN_ID, Vector2(400.0, 300.0), "background");
	imgui.doOverlay(GEN_ID, Vector2(0.0, 0.0), Vector2(800.0, 600.0));

	const float xPosition = 50;

	imgui.doText(GEN_ID, Vector2(xPosition, mYPosition), TextManager::CRD_PROGRAMMERS);
	imgui.doText(GEN_ID, Vector2(xPosition, mYPosition+30), "Daniel Knobe");
	imgui.doText(GEN_ID, Vector2(xPosition, mYPosition+60), "  (daniel-knobe(at)web.de)", TF_SMALL_FONT);
	imgui.doText(GEN_ID, Vector2(xPosition, mYPosition+85), "Jonathan Sieber");
	imgui.doText(GEN_ID, Vector2(xPosition, mYPosition+115), "  (jonathan_sieber(at)yahoo.de)", TF_SMALL_FONT);
	imgui.doText(GEN_ID, Vector2(xPosition, mYPosition+140), "Sven Rech");
	imgui.doText(GEN_ID, Vector2(xPosition, mYPosition+170), "  (svenrech(at)gmx.de)", TF_SMALL_FONT);
	imgui.doText(GEN_ID, Vector2(xPosition, mYPosition+195), "Erik Schultheis");
	imgui.doText(GEN_ID, Vector2(xPosition, mYPosition+225), "  (erik-schultheis(at)freenet.de)", TF_SMALL_FONT);

	imgui.doText(GEN_ID, Vector2(xPosition, mYPosition+255), TextManager::CRD_GRAPHICS);
	imgui.doText(GEN_ID, Vector2(xPosition, mYPosition+285), "Silvio Mummert");
	imgui.doText(GEN_ID, Vector2(xPosition, mYPosition+315), "  (mummertathome(at)t-online.de)", TF_SMALL_FONT);
	imgui.doText(GEN_ID, Vector2(xPosition, mYPosition+340), "Richard Bertrand");
	imgui.doText(GEN_ID, Vector2(xPosition, mYPosition+370), "  (ricbertrand(at)hotmail.com)", TF_SMALL_FONT);

	imgui.doText(GEN_ID, Vector2(xPosition, mYPosition+415), TextManager::CRD_THX);
	imgui.doText(GEN_ID, Vector2(xPosition, mYPosition+445), "Daniel Skoraszewsky");
	imgui.doText(GEN_ID, Vector2(xPosition, mYPosition+475), "  (skoraszewsky(at)t-online.de)", TF_SMALL_FONT);

	if (mYPosition > 20)
		mYPosition -= 2.5;

	if (imgui.doButton(GEN_ID, Vector2(400.0, 560.0), TextManager::LBL_CANCEL))
	{
		deleteCurrentState();
		setCurrentState(new MainMenuState());
		return;
	}
}

const char* CreditsState::getStateName() const
{
	return "CreditsState";
}

