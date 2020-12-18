#include "menu_state.h"

MenuState::MenuState(gef::Platform * platform,
					 gef::InputManager * input_manager,
					 gef::AudioManager * audio_manager,
					 gef::Renderer3D * renderer_3D,
					 gef::SpriteRenderer * sprite_renderer,
					 gef::Font * font) :
	State(platform, input_manager, audio_manager, renderer_3D, sprite_renderer, font)
{
}


MenuState::~MenuState()
{
}

bool MenuState::HandleInput()
{
	return false;
}

void MenuState::Update(float delta_time)
{
}

void MenuState::Render()
{
}
