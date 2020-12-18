#include "options_state.h"



OptionsState::OptionsState(gef::Platform * platform,
						   gef::InputManager * input_manager,
						   gef::AudioManager * audio_manager,
						   gef::Renderer3D * renderer_3D,
						   gef::SpriteRenderer * sprite_renderer,
						   gef::Font * font) :
	State(platform, input_manager, audio_manager, renderer_3D, sprite_renderer, font)
{
}


OptionsState::~OptionsState()
{
}

bool OptionsState::HandleInput()
{
	return false;
}

void OptionsState::Update(float delta_time)
{
}

void OptionsState::Render()
{
}
