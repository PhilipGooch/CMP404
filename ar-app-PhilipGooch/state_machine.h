
namespace gef
{
	class Platform;
	class InputManager;
	class AudioManager;
	class SpriteRenderer;
	class Renderer3D;
	class Font;
}
class State;
class GameState;
class MenuState;
class OptionsState;

class StateMachine
{
public:
	StateMachine(gef::Platform * platform, 
				 gef::InputManager * input_manager, 
				 gef::AudioManager * audio_manager, 
				 gef::Renderer3D * renderer_3D,
				 gef::SpriteRenderer * sprite_renderer,
				 gef::Font * font);
	~StateMachine();

	enum STATE
	{
		MENU,
		GAME,
		OPTIONS
	};

	void SetState(STATE state);

	inline State* GetState() { return state_; }

private:
	gef::InputManager* input_manager_;

	State* state_;
	GameState* game_state_;
	MenuState* menu_state_;
	OptionsState* options_state_;
};