#include "state_machine.h"
#include <input/input_manager.h>
#include "game_state.h"
#include "menu_state.h"
#include "options_state.h"

StateMachine::StateMachine(gef::Platform * platform, 
						   gef::InputManager * input_manager, 
						   gef::AudioManager * audio_manager, 
						   gef::Renderer3D * renderer_3D,
						   gef::SpriteRenderer * sprite_renderer, 
						   gef::Font * font)
{
	game_state_ = new GameState(platform, input_manager, audio_manager, renderer_3D, sprite_renderer, font);
	menu_state_ = new MenuState(platform, input_manager, audio_manager, renderer_3D, sprite_renderer, font);
	options_state_ = new OptionsState(platform, input_manager, audio_manager, renderer_3D, sprite_renderer, font);

	SetState(STATE::GAME);
}

StateMachine::~StateMachine()
{
}

void StateMachine::SetState(STATE state)
{
	switch (state)
	{
	case STATE::GAME:
		state_ = game_state_;
		break;
	case STATE::MENU:
		state_ = menu_state_;
		break;
	case STATE::OPTIONS:
		state_ = options_state_;
		break;
	}
}
