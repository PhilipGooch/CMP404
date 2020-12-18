#include "ar_app.h"

#include <input/input_manager.h>
#include <audio/audio_manager.h>
#include <graphics/sprite_renderer.h>
#include <graphics/renderer_3d.h>
#include <graphics/font.h>
#include "primitive_builder.h"
#include "state_machine.h"
#include "state.h"

ARApp::ARApp(gef::Platform& platform) :
	Application(platform)
{
}

void ARApp::Init()
{
	input_manager_ = gef::InputManager::Create(platform_);
	audio_manager_ = gef::AudioManager::Create();
	sprite_renderer_ = gef::SpriteRenderer::Create(platform_);
	renderer_3D_ = gef::Renderer3D::Create(platform_);
	primitive_builder_ = new PrimitiveBuilder(platform_);
	font_ = new gef::Font(platform_);
	font_->Load("comic_sans");
	state_machine_ = new StateMachine(&platform_, input_manager_, audio_manager_, renderer_3D_, sprite_renderer_, font_);
}

void ARApp::CleanUp()
{
	
}

bool ARApp::Update(float delta_time)
{
	input_manager_->Update();

	State* state = state_machine_->GetState();
	if (state)
	{
		if (!state->HandleInput()) return false;
		state->Update(delta_time);
	}

	return true;
}

void ARApp::Render()
{
	State* state = state_machine_->GetState();
	if (state)
	{
		state->Render();
	}
}
