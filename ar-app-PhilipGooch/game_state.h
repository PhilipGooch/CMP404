#pragma once
#include "state.h"

#include <vector>
#include <maths\matrix44.h>
#include <graphics\sprite.h>
#include <platform/vita/graphics/texture_vita.h>

namespace gef
{
	class Platform;
	class InputManager;
	class AudioManager;
	class SpriteRenderer;
	class Renderer3D;
	class Font;
}

class GameState : public State
{
public:
	GameState(gef::Platform * platform,
			  gef::InputManager * input_manager,
			  gef::AudioManager * audio_manager,
			  gef::Renderer3D * renderer_3D,
			  gef::SpriteRenderer * sprite_renderer,
			  gef::Font * font);
	~GameState();

protected:
	bool HandleInput() override;
	void Update(float delta_time) override;
	void Render() override;

	void SetupLights();

	gef::Matrix44 projection_matrix_;
	gef::Matrix44 view_matrix_;
	gef::Matrix44 ortho_matrix_;
	gef::Sprite camera_feed_sprite_;
	gef::TextureVita* camera_feed_texture_;

	bool init = true;
	bool markers_in_view_[6] = { false };
	int anchor_ = 2;

};

