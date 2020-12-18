#include "game_state.h"

#include <gxm.h>
#include <motion.h>
#include <libdbg.h>
#include <libsmart.h>
#include <sony_sample_framework.h>
#include <sony_tracking.h>
#include <system/platform.h>
#include <input/input_manager.h>
#include <audio/audio_manager.h>
#include <graphics/renderer_3d.h>
#include <graphics/sprite_renderer.h>
#include <graphics/font.h>



GameState::GameState(gef::Platform * platform,
					 gef::InputManager * input_manager,
					 gef::AudioManager * audio_manager,
					 gef::Renderer3D * renderer_3D,
					 gef::SpriteRenderer * sprite_renderer,
					 gef::Font * font) :
	State(platform, input_manager, audio_manager, renderer_3D, sprite_renderer, font)
{
	// INITIALIZE SONY FRAMEWORK
	sampleInitialize();
	smartInitialize();

	// PROJECTION, VIEW AND ORTHOGRAPHIC MATRICES
	float camera_image_scale_factor = (960.f / 544.f) / (640.f / 480.f);
	gef::Matrix44 scale_matrix;
	scale_matrix.SetIdentity();
	scale_matrix.Scale(gef::Vector4(1.f, camera_image_scale_factor, 1.f, 1.f));
	projection_matrix_ = platform_->PerspectiveProjectionFov(SCE_SMART_IMAGE_FOV, (float)SCE_SMART_IMAGE_WIDTH / (float)SCE_SMART_IMAGE_HEIGHT, .1f, 10.f);
	projection_matrix_ = projection_matrix_ * scale_matrix;
	view_matrix_.SetIdentity();
	ortho_matrix_.SetIdentity();
	ortho_matrix_ = platform_->OrthographicFrustum(-1.f, 1.f, -1.f, 1.f, -1.f, 1.f);

	// RESET SONY TRACKING
	AppData* dat = sampleUpdateBegin();
	smartTrackingReset();
	sampleUpdateEnd(dat);

	// CAMERA FEED IMAGE
	camera_feed_texture_ = new gef::TextureVita;
	camera_feed_sprite_.set_width(2.f);
	camera_feed_sprite_.set_height(2.f * camera_image_scale_factor);
	camera_feed_sprite_.set_position(gef::Vector4(0.f, 0.f, 1.f, 1.f));
	sprite_renderer_->set_projection_matrix(ortho_matrix_);

}

GameState::~GameState()
{
	smartRelease();
	sampleRelease();
}

bool GameState::HandleInput()
{
	return true;
}

void GameState::Update(float delta_time)
{
	fps_ = 1.f / delta_time;

	AppData* dat = sampleUpdateBegin();
	smartUpdate(dat->currentImage);

	
	if (init)
	{
		// Find anchor marker initially to anchor everything to it.
		init = (sampleIsMarkerFound(anchor_));
	}
	else
	{
		// If anchor marker is not found.
		if (!sampleIsMarkerFound(anchor_))
		{
			// Find a new marker.
			bool marker_found = false;
			for (int i = 0; i < 6; i++)
			{
				if (sampleIsMarkerFound(i))
				{
					marker_found = true;
					anchor_ = i;
					break;
				}
			}
			// If no marker has been found.
			if (!marker_found)
			{
				// TODO: display a message.

			}
		}
	}

	sampleUpdateEnd(dat);


	//if (playing_)
	//{
	//	// after all markers are hidden and then one is shown again, this gets called before 
	//	if (!markers_in_view_[selected_marker_->ID_])
	//	{
	//		for (int i = 0; i < 6; i++)
	//		{
	//			if (i == selected_marker_->ID_) continue;
	//			if (i == wolf_marker_->ID_) continue;
	//			if (i == tree_marker_->ID_) continue;

	//			if (sampleIsMarkerFound(i))
	//			{
	//				markers_in_view_[i] = true;
	//				gef::Matrix44 previous_marker_matrix = selected_marker_->world_matrix_;
	//				Marker::CHILD previous_anchor = selected_marker_->child_;
	//				selected_marker_->child_ = Marker::CHILD::NONE;
	//				selected_marker_ = markers_[i];
	//				cow_marker_ = selected_marker_;
	//				selected_marker_->child_ = previous_anchor;

	//				sampleGetTransform(selected_marker_->ID_, &selected_marker_->world_matrix_);

	//				gef::Matrix44 selected_marker_inverse_world_matrix;
	//				selected_marker_inverse_world_matrix.AffineInverse(selected_marker_->world_matrix_);

	//				for (Boid* boid : cows_)
	//				{
	//					Cow* cow = (Cow*)boid;
	//					cow->marker_matrix_ = cow_marker_->world_matrix_;
	//					cow->local_marker_matrix_ = cow->local_marker_matrix_ * previous_marker_matrix * selected_marker_inverse_world_matrix;
	//				}



	//				break;
	//			}
	//		}
	//	}

	//	if (markers_in_view_[selected_marker_->ID_])
	//	{
	//		sampleGetTransform(selected_marker_->ID_, &selected_marker_->world_matrix_);
	//	}

	//	if (markers_in_view_[wolf_marker_->ID_])
	//	{
	//		sampleGetTransform(wolf_marker_->ID_, &wolf_marker_->world_matrix_);
	//	}

	//	if (markers_in_view_[tree_marker_->ID_])
	//	{
	//		sampleGetTransform(tree_marker_->ID_, &tree_marker_->world_matrix_);
	//	}
	//}

	//sampleUpdateEnd(dat);

	//if (playing_)
	//{
	//	for (Marker* marker : markers_)
	//	{
	//		marker->Update();
	//	}

	//	for (Boid* boid : cows_)
	//	{
	//		Cow* cow = (Cow*)boid;
	//		cow->marker_matrix_ = cow_marker_->world_matrix_;
	//		cow->Flock(cows_, delta_time);
	//		cow->Update(delta_time);
	//	}

	//	for (Boid* boid : wolves_)
	//	{
	//		Wolf* wolf = (Wolf*)boid;
	//		wolf->marker_matrix_ = wolf_marker_->world_matrix_;
	//		wolf->Flock(wolves_, delta_time);
	//		wolf->Update(delta_time);
	//	}

	//	for (Boid* boid : trees_)
	//	{
	//		Tree* tree = (Tree*)boid;
	//		tree->marker_matrix_ = tree_marker_->world_matrix_;
	//	}

	//}
}

void GameState::Render()
{
	renderer_3D_->set_projection_matrix(projection_matrix_);
	renderer_3D_->set_view_matrix(view_matrix_);

	AppData* dat = sampleRenderBegin();
	ortho_matrix_ = platform_->OrthographicFrustum(-1.f, 1.f, -1.f, 1.f, -1.f, 1.f);		// why do i need to update this every frame?
	sprite_renderer_->set_projection_matrix(ortho_matrix_);
	if (dat->currentImage)
	{
		camera_feed_texture_->set_texture(dat->currentImage->tex_yuv);
		camera_feed_sprite_.set_texture(camera_feed_texture_);
	}
	sprite_renderer_->Begin(true);
	sprite_renderer_->DrawSprite(camera_feed_sprite_);
	sprite_renderer_->End();

	/*if (playing_)
	{

		renderer_3D_->Begin(false);

		for (int i = 0; i < number_of_markers_; i++)
		{
			if (markers_in_view_[i])
			{
				if (markers_[i]->child_ == Marker::CHILD::COW ||
					markers_[i]->child_ == Marker::CHILD::WOLF ||
					markers_[i]->child_ == Marker::CHILD::TREE)
				{
					markers_[i]->Render();
				}
			}
		}

		for (Boid* boid : cows_)
		{
			Cow* cow = (Cow*)boid;
			cow->Render();
		}

		for (Boid* boid : wolves_)
		{
			Wolf* wolf = (Wolf*)boid;
			wolf->Render();
		}

		for (Boid* boid : trees_)
		{
			Tree* tree = (Tree*)boid;
			tree->Render(true);
		}

		renderer_3D_->DrawMesh(debugCube);

		renderer_3D_->End();

	}

	sampleRenderEnd();

	DrawHUD();*/
}

void GameState::SetupLights()
{
	gef::PointLight default_point_light;
	default_point_light.set_colour(gef::Colour(0.7f, 0.7f, 1.0f, 1.0f));
	default_point_light.set_position(gef::Vector4(450.f, 100.0f, 250.f));
	gef::Default3DShaderData& default_shader_data = renderer_3D_->default_shader_data();
	float light = 0.5f;
	default_shader_data.set_ambient_light_colour(gef::Colour(light, light, light, 1.0f));
	//default_shader_data.AddPointLight(default_point_light);
}


