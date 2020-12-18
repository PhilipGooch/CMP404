#ifndef _RENDER_TARGET_APP_H
#define _RENDER_TARGET_APP_H

#include <system/application.h>

// FRAMEWORK FORWARD DECLARATIONS
namespace gef
{
	class Platform;
	class InputManager;
	class AudioManager;
	class SpriteRenderer;
	class Renderer3D;
	class Font;
}
class PrimitiveBuilder;
class StateMachine;

class ARApp : public gef::Application
{
public:
	ARApp(gef::Platform& platform);
	void Init();
	void CleanUp();
	bool Update(float frame_time);
	void Render();

private:
	gef::InputManager* input_manager_;
	gef::AudioManager* audio_manager_;
	gef::SpriteRenderer* sprite_renderer_;
	gef::Renderer3D* renderer_3D_;
	PrimitiveBuilder* primitive_builder_;
	gef::Font* font_;
	StateMachine* state_machine_;
};




#endif // _RENDER_TARGET_APP_H