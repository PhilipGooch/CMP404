#pragma once
#include "state.h"

namespace gef
{
	class Platform;
	class InputManager;
	class AudioManager;
	class SpriteRenderer;
	class Renderer3D;
	class Font;
}

class OptionsState : public State
{
public:
	OptionsState(gef::Platform * platform,
			     gef::InputManager * input_manager,
			     gef::AudioManager * audio_manager,
			     gef::Renderer3D * renderer_3D,
			     gef::SpriteRenderer * sprite_renderer,
			     gef::Font * font);
	~OptionsState();

protected:
	bool HandleInput() override;
	void Update(float delta_time) override;
	void Render() override;
};

