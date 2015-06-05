#include <tlocCore/tloc_core.h>
#include <tlocGraphics/tloc_graphics.h>
#include <tlocMath/tloc_math.h>
#include <tlocPrefab/tloc_prefab.h>

#include <tlocInput/tloc_input.h>

#include <gameAssetsPath.h>

#include "PathNode.h"
#include "AStarPathfinder.h"

#pragma warning(disable: 4530)
#pragma warning(disable: 4244)
using namespace tloc;

#define MOVE_DIR_NONE -1
#define MOVE_DIR_LEFT 0
#define MOVE_DIR_RIGHT 1
#define MOVE_DIR_UP 2
#define MOVE_DIR_DOWN 3

#define CELL_WIDTH 68.2666f
#define CELL_OFFSET 34.1333f

#define GAME_STATE_STARTING 0
#define GAME_STATE_RUNNING 1
#define GAME_STATE_GAMEOVER -1

namespace {
	int gameState = GAME_STATE_STARTING;
	int powerModeCurInterval = 0;
	int powerModeUpdateInterval = 40;
	int pelletsConsumed = 0;

	bool hasCherrySpawned = false;
	bool hasCherryBeenEaten = false;
	int cherryCurInterval = 0;
	int cherrySpawnInterval = core::rng::g_defaultRNG.GetRandomInteger(40, 150);
	int cherryDespawnInterval = core::rng::g_defaultRNG.GetRandomInteger(75, 150);
	PathNode* cherryNode= nullptr;

	int score = 0;
	int highScore = 500;

	bool isPacmanMoving;
	int pacmanMoveDir;
	int pacmanNextMoveDir;
	int lives = 2;
	bool pacmanSprite = 0;

	AStarPathFinder* astar;
	core_conts::Array<core_conts::Array<PathNode*>> pathNodes;

	// BLINKY
	core_conts::Array<math_t::Vec2f> lowestPathBlinky;
	bool isBlinkyMoving;
	int blinkyMoveDir;
	int blinkyLowestPathIndex;
	int blinkyUpdateInterval = 5;
	int blinkyCurInterval = 0;
	bool isBlinkyScatter = false;
	int BlinkyScatterPathPhase = 0;

	// PINKY
	core_conts::Array<math_t::Vec2f> lowestPathPinky;
	bool isPinkyMoving;
	int pinkyMoveDir;
	int pinkyLowestPathIndex;
	int pinkyUpdateInterval = 5;
	int pinkyCurInterval = 0;
	bool isPinkyScatter = false;
	int PinkyScatterPathPhase = 0;

	// INKY
	core_conts::Array<math_t::Vec2f> lowestPathInky;
	bool isInkyMoving;
	int inkyMoveDir;
	int inkyLowestPathIndex;
	int inkyUpdateInterval = 25;
	int inkyCurInterval = 0;
	bool isInkyScatter = false;
	int InkyScatterPathPhase = 0;

	// CLYDE
	core_conts::Array<math_t::Vec2f> lowestPathClyde;
	bool isClydeMoving;
	int clydeMoveDir;
	int clydeLowestPathIndex;
	int clydeUpdateInterval = 10;
	int clydeCurInterval = 0;
	bool isClydeScatter = false;
	bool isClydeFalseScatter = false;
	int ClydeScatterPathPhase = 0;

	const core_str::StringW
		g_symbols = L"ABCDEFGHIJKLMNOPQRSTUVWXYZ"
		L"abcdefghijklmnopqrstuvwxyz"
		L"1234567890!@#$%^&*()_+-=[]"
		L"{}\\|;:'\",<.>/?`~\n ";

	core_str::StringW strWScore;
	core_str::StringW strWMessage;
	core_str::StringW strWHighScore;
};

class WindowCallback
{
public:
  WindowCallback()
    : m_endProgram(false)
  { }

  core_dispatch::Event 
    OnWindowEvent(const gfx_win::WindowEvent& a_event)
  {
    if (a_event.m_type == gfx_win::WindowEvent::close)
    { m_endProgram = true; }

    return core_dispatch::f_event::Continue();
  }

  bool  m_endProgram;
};
TLOC_DEF_TYPE(WindowCallback);


math_t::Vec2f Screen2Grid(float x, float y)
{
	return math_t::Vec2f(((((-y * .5f + .5f) * 2048) + CELL_OFFSET) / CELL_WIDTH) - 1,
						((((x * .5f + .5f) * 2048) + CELL_OFFSET) / CELL_WIDTH) - 1);
}

math_t::Vec2f Grid2Screen(float x, float y)
{
	return math_t::Vec2f(((((x * CELL_WIDTH) + CELL_OFFSET) / 2048) * 2.f - 1.f),
						-((((y * CELL_WIDTH) + CELL_OFFSET) / 2048) * 2.f - 1.f));
}


int GetGhostMoveDir(math_t::Vec2f moveVec)
{
	int result = MOVE_DIR_NONE;
	if (moveVec[0] == 1 && moveVec[1] == 0)
	{
		result = MOVE_DIR_RIGHT;
	}
	else if (moveVec[0] == -1 && moveVec[1] == 0)
	{
		result = MOVE_DIR_LEFT;
	}
	else if (moveVec[0] == 0 && moveVec[1] == -1)
	{
		result = MOVE_DIR_UP;
	}
	else if (moveVec[0] == 0 && moveVec[1] == 1)
	{
		result = MOVE_DIR_DOWN;
	}
	return result;
}

void CreateBlinkyPath(math_t::Vec2f start, math_t::Vec2f target, bool allowBackwards)
{
	//if (start[1] != target[0] && start[0] != target[1])
	//{
		//PATHFINDING

		bool wasWalkable = false;
		PathNode* backNode = nullptr;

		if (!allowBackwards)
		{
			if (blinkyMoveDir == MOVE_DIR_RIGHT)
			{
				backNode = astar->pathGrid[start[1]][start[0] - 1];
				wasWalkable = backNode->isWalkable;
				backNode->isWalkable = false;
			}
			else if (blinkyMoveDir == MOVE_DIR_LEFT)
			{
				backNode = astar->pathGrid[start[1]][start[0] + 1];
				wasWalkable = backNode->isWalkable;
				backNode->isWalkable = false;
			}
			else if (blinkyMoveDir == MOVE_DIR_UP)
			{
				backNode = astar->pathGrid[start[1] + 1][start[0]];
				wasWalkable = backNode->isWalkable;
				backNode->isWalkable = false;
			}
			else if (blinkyMoveDir == MOVE_DIR_DOWN)
			{
				backNode = astar->pathGrid[start[1] - 1][start[0]];
				wasWalkable = backNode->isWalkable;
				backNode->isWalkable = false;
			}
		}
		astar->findPath(start[1]+1, start[0]+1, target[0]+1, target[1]+1);
		////astar->printPath();

		auto blinkyTargetCell = astar->end;

		lowestPathBlinky.clear();
		while (blinkyTargetCell != astar->start && blinkyTargetCell != nullptr)
		{
			math_t::Vec2f pos = math_t::Vec2f(blinkyTargetCell->x, blinkyTargetCell->y);
			lowestPathBlinky.push_back(pos);

			blinkyTargetCell = blinkyTargetCell->parent;
		}
		//math_t::Vec2f pos = math_t::Vec2f(blinkyTargetCell->x, blinkyTargetCell->y);
		//lowestPathBlinky.push_back(pos);

		for (unsigned int i = 0; i < lowestPathBlinky.size(); i++)
		{
			//TLOC_LOG_CORE_DEBUG() << lowestPathBlinky[i][0] << ", " << lowestPathBlinky[i][1];
		}
		blinkyLowestPathIndex = lowestPathBlinky.size() - 1;

		if (blinkyLowestPathIndex > -1)
		{
			isBlinkyMoving = true;

			blinkyMoveDir = GetGhostMoveDir(lowestPathBlinky[blinkyLowestPathIndex] - start);
		}

		if (wasWalkable)
			backNode->isWalkable = true;

	//}
}

void CreatePinkyPath(math_t::Vec2f start, math_t::Vec2f target, bool allowBackwards)
{
	bool wasWalkable = false;
	PathNode* backNode = nullptr;

	if (!allowBackwards)
	{
		if (pinkyMoveDir == MOVE_DIR_RIGHT)
		{
			backNode = astar->pathGrid[start[1]][start[0] - 1];
			wasWalkable = backNode->isWalkable;
			backNode->isWalkable = false;
		}
		else if (pinkyMoveDir == MOVE_DIR_LEFT)
		{
			backNode = astar->pathGrid[start[1]][start[0] + 1];
			wasWalkable = backNode->isWalkable;
			backNode->isWalkable = false;
		}
		else if (pinkyMoveDir == MOVE_DIR_UP)
		{
			backNode = astar->pathGrid[start[1] + 1][start[0]];
			wasWalkable = backNode->isWalkable;
			backNode->isWalkable = false;
		}
		else if (pinkyMoveDir == MOVE_DIR_DOWN)
		{
			backNode = astar->pathGrid[start[1] - 1][start[0]];
			wasWalkable = backNode->isWalkable;
			backNode->isWalkable = false;
		}
	}
	
	if (!isPinkyScatter)
	{
		if (pacmanMoveDir == MOVE_DIR_RIGHT)
		{
			for (int i = 4; i > 0; i--)
			{
				if (astar->findPath(start[1] + 1, start[0] + 1, target[0] + 1, target[1] + i))
				{
					////astar->printPath();
					break;
				}
			}

		}
		else if (pacmanMoveDir == MOVE_DIR_LEFT)
		{
			for (int i = -2; i < +2; i++)
			{
				if (astar->findPath(start[1] + 1, start[0] + 1, target[0] + 1, target[1] + i))
				{
					////astar->printPath();
					break;
				}
			}
		}
		else if (pacmanMoveDir == MOVE_DIR_UP)
		{
			for (int i = -2; i < +2; i++)
			{
				if (astar->findPath(start[1] + 1, start[0] + 1, target[0] + i, target[1] + 1))
				{
					////astar->printPath();
					break;
				}
			}
		}
		else if (pacmanMoveDir == MOVE_DIR_DOWN)
		{
			for (int i = 4; i > 0; i--)
			{
				if (astar->findPath(start[1] + 1, start[0] + 1, target[0] + i, target[1] + 1))
				{
					////astar->printPath();
					break;
				}
			}
		}
		else
			astar->findPath(start[1] + 1, start[0] + 1, target[0] + 1, target[1] + 1);
	}
	else
		astar->findPath(start[1] + 1, start[0] + 1, target[0] + 1, target[1] + 1);

	////astar->printPath();

	auto pinkyTargetCell = astar->end;

	lowestPathPinky.clear();
	while (pinkyTargetCell != astar->start && pinkyTargetCell != nullptr)
	{
		math_t::Vec2f pos = math_t::Vec2f(pinkyTargetCell->x, pinkyTargetCell->y);
		lowestPathPinky.push_back(pos);

		pinkyTargetCell = pinkyTargetCell->parent;
	}
	//math_t::Vec2f pos = math_t::Vec2f(pinkyTargetCell->x, pinkyTargetCell->y);
	//lowestPathBlinky.push_back(pos);

	for (unsigned int i = 0; i < lowestPathPinky.size(); i++)
	{
		//TLOC_LOG_CORE_DEBUG() << lowestPathBlinky[i][0] << ", " << lowestPathBlinky[i][1];
	}
	pinkyLowestPathIndex = lowestPathPinky.size() - 1;

	if (pinkyLowestPathIndex > -1)
	{
		isPinkyMoving = true;

		pinkyMoveDir = GetGhostMoveDir(lowestPathPinky[pinkyLowestPathIndex] - start);
	}

	if (wasWalkable)
		backNode->isWalkable = true;
}

void CreateInkyPath(math_t::Vec2f start, math_t::Vec2f target, bool allowBackwards)
{
	bool wasWalkable = false;
	PathNode* backNode = nullptr;

	if (!allowBackwards)
	{
		if (inkyMoveDir == MOVE_DIR_RIGHT)
		{
			backNode = astar->pathGrid[start[1]][start[0] - 1];
			wasWalkable = backNode->isWalkable;
			backNode->isWalkable = false;
		}
		else if (inkyMoveDir == MOVE_DIR_LEFT)
		{
			backNode = astar->pathGrid[start[1]][start[0] + 1];
			wasWalkable = backNode->isWalkable;
			backNode->isWalkable = false;
		}
		else if (inkyMoveDir == MOVE_DIR_UP)
		{
			backNode = astar->pathGrid[start[1] + 1][start[0]];
			wasWalkable = backNode->isWalkable;
			backNode->isWalkable = false;
		}
		else if (inkyMoveDir == MOVE_DIR_DOWN)
		{
			backNode = astar->pathGrid[start[1] - 1][start[0]];
			wasWalkable = backNode->isWalkable;
			backNode->isWalkable = false;
		}
	}
	astar->findPath(start[1] + 1, start[0] + 1, target[0] + 1, target[1] + 1);
	////astar->printPath();

	auto inkyTargetCell = astar->end;

	lowestPathInky.clear();
	while (inkyTargetCell != astar->start && inkyTargetCell != nullptr)
	{
		math_t::Vec2f pos = math_t::Vec2f(inkyTargetCell->x, inkyTargetCell->y);
		lowestPathInky.push_back(pos);

		inkyTargetCell = inkyTargetCell->parent;
	}

	inkyLowestPathIndex = lowestPathInky.size() - 1;

	if (inkyLowestPathIndex > -1)
	{
		isInkyMoving = true;

		inkyMoveDir = GetGhostMoveDir(lowestPathInky[inkyLowestPathIndex] - start);
	}

	if (wasWalkable)
		backNode->isWalkable = true;
}

void CreateInkyRandomPath(math_t::Vec2f start)
{
	PathNode* randomPathNode = pathNodes[core::rng::g_defaultRNG.GetRandomInteger(1, 28)][core::rng::g_defaultRNG.GetRandomInteger(1, 28)];
	while (!randomPathNode->isWalkable)
	{
		randomPathNode = pathNodes[core::rng::g_defaultRNG.GetRandomInteger(1, 28)][core::rng::g_defaultRNG.GetRandomInteger(1, 28)];
	}
	CreateInkyPath(start, math_t::Vec2f(randomPathNode->y, randomPathNode->x), true);
}

void CreateClydePath(math_t::Vec2f start, math_t::Vec2f target, bool allowBackwards)
{
	bool wasWalkable = false;
	PathNode* backNode = nullptr;

	if (!allowBackwards)
	{
		if (clydeMoveDir == MOVE_DIR_RIGHT)
		{
			backNode = astar->pathGrid[start[1]][start[0] - 1];
			wasWalkable = backNode->isWalkable;
			backNode->isWalkable = false;
		}
		else if (clydeMoveDir == MOVE_DIR_LEFT)
		{
			backNode = astar->pathGrid[start[1]][start[0] + 1];
			wasWalkable = backNode->isWalkable;
			backNode->isWalkable = false;
		}
		else if (clydeMoveDir == MOVE_DIR_UP)
		{
			backNode = astar->pathGrid[start[1] + 1][start[0]];
			wasWalkable = backNode->isWalkable;
			backNode->isWalkable = false;
		}
		else if (clydeMoveDir == MOVE_DIR_DOWN)
		{
			backNode = astar->pathGrid[start[1] - 1][start[0]];
			wasWalkable = backNode->isWalkable;
			backNode->isWalkable = false;
		}
	}
	astar->findPath(start[1] + 1, start[0] + 1, target[0] + 1, target[1] + 1);

	auto clydeTargetCell = astar->end;

	lowestPathClyde.clear();
	while (clydeTargetCell != astar->start && clydeTargetCell != nullptr)
	{
		math_t::Vec2f pos = math_t::Vec2f(clydeTargetCell->x, clydeTargetCell->y);
		lowestPathClyde.push_back(pos);

		clydeTargetCell = clydeTargetCell->parent;
	}

	clydeLowestPathIndex = lowestPathClyde.size() - 1;

	if (clydeLowestPathIndex > -1)
	{
		isClydeMoving = true;

		clydeMoveDir = GetGhostMoveDir(lowestPathClyde[clydeLowestPathIndex] - start);
	}

	if (wasWalkable)
		backNode->isWalkable = true;
}

int TLOC_MAIN(int argc, char *argv[])
{
  TLOC_UNUSED_2(argc, argv);

  gfx_win::Window win;
  WindowCallback  winCallback;

  win.Register(&winCallback);
  win.Create(gfx_win::Window::graphics_mode::Properties(512, 512),
    gfx_win::WindowSettings("2LoC Engine") );

  // -----------------------------------------------------------------------
  // Initialize OpenGL for the current platform

  if (gfx_gl::InitializePlatform() != ErrorSuccess)
  { 
    TLOC_LOG_GFX_ERR() << "Renderer failed to initialize"; 
    return 1;
  }

  using namespace gfx_rend::p_renderer;
  gfx_rend::renderer_sptr renderer = win.GetRenderer();

  gfx_rend::Renderer::Params p(renderer->GetParams());
  p.SetClearColor(gfx_t::Color(0.5f, 0.5f, 1.0f, 1.0f));
  //p.Enable<enable_disable::Blend>();
  p.AddClearBit<clear::ColorBufferBit>();
  p.SetBlendFunction < blend_function::SourceAlpha,
	  blend_function::OneMinusSourceAlpha > ();
  renderer->SetParams(p);

  //gfx_rend::renderer_sptr renderer2 = win.GetRenderer();
  //renderer2->SetParams(p);
  //------------------------------------------------------------------------
  // Creating InputManager - This manager will handle all of our HIDs during
  // its lifetime. More than one InputManager can be instantiated.
  ParamList<core_t::Any> params;
  params.m_param1 = win.GetWindowHandle();

  input::input_mgr_i_ptr inputMgr =
	  core_sptr::MakeShared<input::InputManagerI>(params);

  //------------------------------------------------------------------------
  // Creating a keyboard and mouse HID
  input_hid::keyboard_i_vptr keyboard =
	  inputMgr->CreateHID<input_hid::KeyboardI>();

  TLOC_ASSERT_NOT_NULL(keyboard);

  //------------------------------------------------------------------------
  // A component pool manager manages all the components in a particular
  // session/level/section.
  core_cs::component_pool_mgr_vso cpoolMgr;

  //------------------------------------------------------------------------
  // All systems in the engine require an event manager and an entity manager
  core_cs::event_manager_vso  eventMgr;
  core_cs::entity_manager_vso entityMgr( MakeArgs(eventMgr.get()) );

  //------------------------------------------------------------------------
  // To render a fan, we need a fan render system - this is a specialized
  // system to render this primitive
  gfx_cs::QuadRenderSystem quadSys(eventMgr.get(), entityMgr.get());
  quadSys.SetRenderer(renderer);

  gfx_cs::FanRenderSystem fanSys(eventMgr.get(), entityMgr.get());
  fanSys.SetRenderer(renderer);

  // -----------------------------------------------------------------------
  // Camera system
  gfx_cs::CameraSystem      camSys(eventMgr.get(), entityMgr.get());

  // SceneNodes require the SceneGraphSystem
  gfx_cs::SceneGraphSystem  sgSys(eventMgr.get(), entityMgr.get());
  // -----------------------------------------------------------------------
  // Text render system
  gfx_cs::dyn_text_render_system_vso
	  textSys(MakeArgs(eventMgr.get(), entityMgr.get()));
  textSys->SetRenderer(renderer);

  //------------------------------------------------------------------------
  // We cannot render anything without materials and its system
  gfx_cs::MaterialSystem    matSys(eventMgr.get(), entityMgr.get());

  // NOTE: The fan render system expects a few shader variables to be declared
  //       and used by the shader (i.e. not compiled out). See the listed
  //       vertex and fragment shaders for more info.
#if defined (TLOC_OS_WIN)
    core_str::String shaderPathVS("/shaders/tlocOneTextureVS.glsl");
#elif defined (TLOC_OS_IPHONE)
    core_str::String shaderPathVS("/shaders/tlocOneTextureVS_gl_es_2_0.glsl");
#endif

#if defined (TLOC_OS_WIN)
    core_str::String shaderPathFS("/shaders/tlocOneTextureFS.glsl");
#elif defined (TLOC_OS_IPHONE)
    core_str::String shaderPathFS("/shaders/tlocOneTextureFS_gl_es_2_0.glsl");
#endif

	core_str::String vsContents, fsContents;
	{
		core_io::Path path(core_str::String(GetAssetsPath()) + shaderPathVS);
		core_io::FileIO_ReadA f(path);

		f.Open();
		f.GetContents(vsContents);
	}
	{
		core_io::Path path(core_str::String(GetAssetsPath()) + shaderPathFS);
		core_io::FileIO_ReadA f(path);

		f.Open();
		f.GetContents(fsContents);
	}

	//------------------------------------------------------------------------
	// Load the required font

	core_io::Path fontPath((core_str::String(GetAssetsPath()) +
		"fonts/VeraMono-Bold.ttf").c_str());

	core_io::FileIO_ReadB rb(fontPath);
	rb.Open();

	core_str::String fontContents;
	rb.GetContents(fontContents);

	gfx_med::font_sptr f = core_sptr::MakeShared<gfx_med::Font>();
	f->Initialize(fontContents);

	using gfx_med::FontSize;
	FontSize fSize(FontSize::em(12),
		FontSize::dpi(win.GetDPI()));

	gfx_med::Font::Params fontParams(fSize);
	fontParams.BgColor(gfx_t::Color(0.0f, 0.0f, 0.0f, 0.0f))
		.PaddingColor(gfx_t::Color(0.0f, 0.0f, 0.0f, 0.0f))
		.PaddingDim(core_ds::MakeTuple(0, 0));

	f->GenerateGlyphCache(g_symbols.c_str(), fontParams);

	// -----------------------------------------------------------------------
	// material will require the correct texture object

	gfx_gl::texture_object_vso to;

	// without specifying the nearest filter, the font will appear blurred in 
	// some cases (especially on smaller sizes)
	gfx_gl::TextureObject::Params toParams;
	toParams.MinFilter<gfx_gl::p_texture_object::filter::Nearest>();
	toParams.MagFilter<gfx_gl::p_texture_object::filter::Nearest>();
	to->SetParams(toParams);

	to->Initialize(*f->GetSpriteSheetPtr()->GetSpriteSheet());

	gfx_gl::uniform_vso u_to;
	u_to->SetName("s_texture").SetValueAs(*to);

	//------------------------------------------------------------------------
	// The prefab library has some prefabricated entities for us

	core_cs::entity_vptr dTextScore =
		pref_gfx::DynamicText(entityMgr.get(), cpoolMgr.get())
		.Alignment(gfx_cs::alignment::k_align_right)
		.Create(L"", f);
	dTextScore->GetComponent<math_cs::Transform>()->
		SetPosition(math_t::Vec3f32(-110, 240, 0));

	pref_gfx::Material(entityMgr.get(), cpoolMgr.get())
		.AssetsPath(GetAssetsPath())
		.AddUniform(u_to.get())
		.Add(dTextScore, core_io::Path(shaderPathVS), core_io::Path(shaderPathFS));

	core_cs::entity_vptr dTextMessage =
		pref_gfx::DynamicText(entityMgr.get(), cpoolMgr.get())
		.Alignment(gfx_cs::alignment::k_align_center)
		.Create(L"Get Ready!", f);
	dTextMessage->GetComponent<math_cs::Transform>()->
		SetPosition(math_t::Vec3f32(0, -45, 0)); 
	entityMgr->InsertComponent
		(core_cs::EntityManager::Params(dTextMessage,
		dTextScore->GetComponent<gfx_cs::Material>()));

	core_cs::entity_vptr dTextHighScore =
		pref_gfx::DynamicText(entityMgr.get(), cpoolMgr.get())
		.Alignment(gfx_cs::alignment::k_align_center)
		.Create(L"Highscore: ", f);
	dTextHighScore->GetComponent<math_cs::Transform>()->
		SetPosition(math_t::Vec3f32(110, 240, 0));
	entityMgr->InsertComponent
		(core_cs::EntityManager::Params(dTextHighScore,
		dTextScore->GetComponent<gfx_cs::Material>()));

	// SET HIGHSCORE
	core_io::Path highscorePath((core_str::String(GetAssetsPath()) +
		"misc/highscore.txt").c_str());
	core_io::FileIO_ReadB rbHighscore(highscorePath);
	rbHighscore.Open();
	core_str::String highscoreContents;
	rbHighscore.GetContents(highscoreContents);
	highScore = atoi(highscoreContents.c_str());

	core_str::String numStr = core_str::Format("Highscore: %i", highScore);
	strWHighScore = core_str::CharAsciiToWide(numStr);
	dTextHighScore->GetComponent<gfx_cs::DynamicText>()->Set(strWHighScore);
	
  //------------------------------------------------------------------------
  // Prepare a texture for the material

	gfx_med::ImageLoaderPng pngLevel;
	core_io::Path pathLevel((core_str::String(GetAssetsPath()) +
		//"/images/pacman_board_mask_DEBUG.png").c_str());
		"/images/pacman_board.png").c_str());
		//"/images/pacman_board_mask2.png").c_str());
		//"/images/pacman_board_mask3.png").c_str());

	if (pngLevel.Load(pathLevel) != ErrorSuccess)
	{
		TLOC_ASSERT_FALSE("Image did not load!");
	}

	gfx_gl::texture_object_vso toLevel;
	toLevel->Initialize(pngLevel.GetImage());

	gfx_gl::uniform_vso  u_toLevel;
	u_toLevel->SetName("s_texture").SetValueAs(*toLevel);

	// 
	gfx_med::ImageLoaderPng pngLevelMask;
	core_io::Path pathLevelMask((core_str::String(GetAssetsPath()) +
		"/images/pacman_board_mask_GRID.png").c_str());

	if (pngLevelMask.Load(pathLevelMask) != ErrorSuccess)
	{
		TLOC_ASSERT_FALSE("Image did not load!");
	}

	//gfx_gl::texture_object_vso toLevelMask;
	//toLevelMask->Initialize(pngLevelMask.GetImage());

	//gfx_gl::uniform_vso  u_to;
	//u_to->SetName("s_texture").SetValueAs(*toLevelMask);



	core_str::String outstring;
	outstring.append("\n");

	unsigned int y = 0;
	unsigned int x = 0;

	for (float row = CELL_OFFSET; row < pngLevelMask.GetImage().GetHeight(); row += CELL_WIDTH)
	{
		pathNodes.push_back(core_conts::Array<PathNode*>());

		x = 0;
		for (float col = CELL_OFFSET; col < pngLevelMask.GetImage().GetWidth(); col += CELL_WIDTH)
		{
			auto pix = pngLevelMask.GetImage().GetPixel((int)row, (int)col).Get();

			PathNode* temp = new PathNode((float)x, (float)y);

			if (pix.m_values[2] < 128)
			{
				if (pix.m_values[0] > 100)
				{
					outstring.append("X");
					temp->hasPowerPellet = true;
				}
				else if (pix.m_values[1] > 100)
				{
					outstring.append("Z");
					temp->hasPellet = true;
				}
				else
					outstring.append("1");
			}
			else
			{
				temp->isWalkable = false;
				outstring.append("0");
			}

			pathNodes[y].push_back(temp);
			x++;
		}

		y++;
		outstring.append("\n");
	}
	TLOC_LOG_CORE_INFO() << outstring;



  // PACMAN
  gfx_med::ImageLoaderPng pngPacman;
  core_io::Path pathPacman((core_str::String(GetAssetsPath()) +
	  "/images/pacman.png").c_str());

  if (pngPacman.Load(pathPacman) != ErrorSuccess)
  {
	  TLOC_ASSERT_FALSE("Image did not load!");
  }

  gfx_gl::texture_object_vso toPacman;
  toPacman->Initialize(pngPacman.GetImage());

  gfx_gl::uniform_vso  u_toPacman;
  u_toPacman->SetName("s_texture").SetValueAs(*toPacman);

  gfx_med::ImageLoaderPng pngPacman1;
  core_io::Path pathPacman1((core_str::String(GetAssetsPath()) +
	  "/images/pacman1.png").c_str());

  if (pngPacman1.Load(pathPacman1) != ErrorSuccess)
  {
	  TLOC_ASSERT_FALSE("Image did not load!");
  }

  gfx_gl::texture_object_vso toPacman1;
  toPacman1->Initialize(pngPacman1.GetImage());

  gfx_gl::uniform_vso  u_toPacman1;
  u_toPacman1->SetName("s_texture").SetValueAs(*toPacman1);

  // BLINKY
  gfx_med::ImageLoaderPng pngGhostBlinky;
  core_io::Path pathGhostBlinky((core_str::String(GetAssetsPath()) +
	  "/images/blinky.png").c_str());

  if (pngGhostBlinky.Load(pathGhostBlinky) != ErrorSuccess)
  {
	  TLOC_ASSERT_FALSE("Image did not load!");
  }

  gfx_gl::texture_object_vso toGhostBlinky;
  toGhostBlinky->Initialize(pngGhostBlinky.GetImage());

  gfx_gl::uniform_vso  u_toGhostBlinky;
  u_toGhostBlinky->SetName("s_texture").SetValueAs(*toGhostBlinky);

  // PINKY
  gfx_med::ImageLoaderPng pngGhostPinky;
  core_io::Path pathGhostPinky((core_str::String(GetAssetsPath()) +
	  "/images/pinky.png").c_str());

  if (pngGhostPinky.Load(pathGhostPinky) != ErrorSuccess)
  {
	  TLOC_ASSERT_FALSE("Image did not load!");
  }

  gfx_gl::texture_object_vso toGhostPinky;
  toGhostPinky->Initialize(pngGhostPinky.GetImage());

  gfx_gl::uniform_vso  u_toGhostPinky;
  u_toGhostPinky->SetName("s_texture").SetValueAs(*toGhostPinky);

  // INKY
  gfx_med::ImageLoaderPng pngGhostInky;
  core_io::Path pathGhostInky((core_str::String(GetAssetsPath()) +
	  "/images/inky.png").c_str());

  if (pngGhostInky.Load(pathGhostInky) != ErrorSuccess)
  {
	  TLOC_ASSERT_FALSE("Image did not load!");
  }

  gfx_gl::texture_object_vso toGhostInky;
  toGhostInky->Initialize(pngGhostInky.GetImage());

  gfx_gl::uniform_vso  u_toGhostInky;
  u_toGhostInky->SetName("s_texture").SetValueAs(*toGhostInky);

  // CLYDE
  gfx_med::ImageLoaderPng pngGhostClyde;
  core_io::Path pathGhostClyde((core_str::String(GetAssetsPath()) +
	  "/images/clyde.png").c_str());

  if (pngGhostClyde.Load(pathGhostClyde) != ErrorSuccess)
  {
	  TLOC_ASSERT_FALSE("Image did not load!");
  }

  gfx_gl::texture_object_vso toGhostClyde;
  toGhostClyde->Initialize(pngGhostClyde.GetImage());

  gfx_gl::uniform_vso  u_toGhostClyde;
  u_toGhostClyde->SetName("s_texture").SetValueAs(*toGhostClyde);

  // GHOST
  gfx_med::ImageLoaderPng pngGhost;
  core_io::Path pathGhost((core_str::String(GetAssetsPath()) +
	  "/images/ghost.png").c_str());

  if (pngGhost.Load(pathGhost) != ErrorSuccess)
  {
	  TLOC_ASSERT_FALSE("Image did not load!");
  }

  gfx_gl::texture_object_vso toGhost;
  toGhost->Initialize(pngGhost.GetImage());

  gfx_gl::uniform_vso  u_toGhost;
  u_toGhost->SetName("s_texture").SetValueAs(*toGhost);

  // PELLET
  gfx_med::ImageLoaderPng pngPellet;
  core_io::Path pathPellet((core_str::String(GetAssetsPath()) +
	  "/images/pellet.png").c_str());

  if (pngPellet.Load(pathPellet) != ErrorSuccess)
  {
	  TLOC_ASSERT_FALSE("Image did not load!");
  }

  gfx_gl::texture_object_vso toPellet;
  toPellet->Initialize(pngPellet.GetImage());

  gfx_gl::uniform_vso  u_toPellet;
  u_toPellet->SetName("s_texture").SetValueAs(*toPellet);

  // CHERRY
  gfx_med::ImageLoaderPng pngCherry;
  core_io::Path pathCherry((core_str::String(GetAssetsPath()) +
	  "/images/cherry.png").c_str());

  if (pngCherry.Load(pathCherry) != ErrorSuccess)
  {
	  TLOC_ASSERT_FALSE("Image did not load!");
  }

  gfx_gl::texture_object_vso toCherry;
  toCherry->Initialize(pngCherry.GetImage());

  gfx_gl::uniform_vso  u_toCherry;
  u_toCherry->SetName("s_texture").SetValueAs(*toCherry);

  //------------------------------------------------------------------------
  // The prefab library has some prefabricated entities for us

	//math_t::Rectf_c rect(math_t::Rectf_c::width((tl_float)win.GetWidth()),
	//	math_t::Rectf_c::height(1));
	math_t::Rectf_c rect(math_t::Rectf_c::width(2),
	math_t::Rectf_c::height(win.GetAspectRatio().Get() *2));

	core_cs::entity_vptr q = pref_gfx::Quad(entityMgr.get(), cpoolMgr.get()).
		Dimensions(rect).Create();

	pref_gfx::Material(entityMgr.get(), cpoolMgr.get())
		.AssetsPath(GetAssetsPath())
		.AddUniform(u_toLevel.get())
		.Add(q, core_io::Path(shaderPathVS), core_io::Path(shaderPathFS));

	// Pacman
	core_cs::entity_vptr dummyPacman = entityMgr->CreateEntity();

	auto matPacman0 = pref_gfx::Material(entityMgr.get(), cpoolMgr.get())
		.AssetsPath(GetAssetsPath())
		.AddUniform(u_toPacman.get())
		.Construct(vsContents, fsContents);

	auto matPacman1 = pref_gfx::Material(entityMgr.get(), cpoolMgr.get())
		.AssetsPath(GetAssetsPath())
		.AddUniform(u_toPacman1.get())
		.Construct(vsContents, fsContents);

	entityMgr.get()->InsertComponent(core_cs::EntityManager::Params(dummyPacman, matPacman0));
	entityMgr.get()->InsertComponent(core_cs::EntityManager::Params(dummyPacman, matPacman1));

	math_t::Circlef32 circPacman(math_t::Circlef32::radius(.045f));
	core_cs::entity_vptr qPacman =
		pref_gfx::Fan(entityMgr.get(), cpoolMgr.get())
		.Sides(64).Circle(circPacman).Create();

	pref_gfx::Material(entityMgr.get(), cpoolMgr.get())
		.AssetsPath(GetAssetsPath())
		.AddUniform(u_toPacman.get())
		.Add(qPacman, core_io::Path(shaderPathVS), core_io::Path(shaderPathFS));

	//auto screenCoord = Grid2Screen(16, 23);
	auto screenCoord = Grid2Screen(15, 22);
	//screenCoord = Grid2Screen(0, 29);
	//screenCoord = Grid2Screen(29, 0);
	//screenCoord = Grid2Screen(29, 29);
	//auto gridCoord = Screen2Grid(screenCoord[0], screenCoord[1]);
	qPacman->GetComponent<math_cs::Transform>()->SetPosition(math_t::Vec3f32(screenCoord[0], screenCoord[1], 0));

	// pacman lives

	core_conts::Array<core_cs::entity_vptr> extraLives;

	for (int i = 0; i < lives; i++)
	{
		math_t::Circlef32 circLife(math_t::Circlef32::radius(.03f));
		core_cs::entity_vptr life =
			pref_gfx::Fan(entityMgr.get(), cpoolMgr.get())
			.Sides(64).Circle(circLife).Create();
		auto lifeScreenCoord = Grid2Screen(1 + i, 29);
		//auto lifeScreenCoord = Grid2Screen(15, 22);

		life->GetComponent<math_cs::Transform>()->SetPosition(math_t::Vec3f32(lifeScreenCoord[0], lifeScreenCoord[1], 0));
		entityMgr.get()->InsertComponent(core_cs::EntityManager::Params(life, matPacman0));

		extraLives.push_back(life);
	}


	// PELLETS
	math_t::Circlef32 circPellet(math_t::Circlef32::radius(.01f));
	core_cs::entity_vptr qPellet0 = pref_gfx::Fan(entityMgr.get(), cpoolMgr.get())
		.Sides(64).Circle(circPellet).Create();
	pref_gfx::Material(entityMgr.get(), cpoolMgr.get())
		.AssetsPath(GetAssetsPath())
		.AddUniform(u_toPellet.get())
		.Add(qPellet0, core_io::Path(shaderPathVS), core_io::Path(shaderPathFS));
	qPellet0->GetComponent<math_cs::Transform>()->SetPosition(math_t::Vec3f32(2, 2, 0));

	core_conts::Array<core_cs::entity_vptr> pellets;
	for (int j = 0; j < 30; j++)
	{
		for (int i = 0; i < 30; i++)
		{
			auto node = pathNodes[j][i];
			if (node->hasPellet)
			{
				core_cs::entity_vptr qPellet1 = pref_gfx::Fan(entityMgr.get(), cpoolMgr.get())
					.Sides(64).Circle(circPellet).Create();
				entityMgr.get()->InsertComponent(core_cs::EntityManager::Params(qPellet1, qPellet0->GetComponent<gfx_cs::Material>()));
				screenCoord = Grid2Screen(i, j);
				qPellet1->GetComponent<math_cs::Transform>()->SetPosition(math_t::Vec3f32(screenCoord[0], screenCoord[1], 0));

				pellets.push_back(qPellet1);
			}

		}
	}

	//POWER PELLETS
	circPellet = (math_t::Circlef32::radius(.02f));

	core_conts::Array<core_cs::entity_vptr> powerPellets;
	for (int j = 0; j < 30; j++)
	{
		for (int i = 0; i < 30; i++)
		{
			auto node = pathNodes[j][i];
			if (node->hasPowerPellet)
			{
				core_cs::entity_vptr qPellet1 = pref_gfx::Fan(entityMgr.get(), cpoolMgr.get())
					.Sides(64).Circle(circPellet).Create();
				entityMgr.get()->InsertComponent(core_cs::EntityManager::Params(qPellet1, qPellet0->GetComponent<gfx_cs::Material>()));
				screenCoord = Grid2Screen(i, j);
				qPellet1->GetComponent<math_cs::Transform>()->SetPosition(math_t::Vec3f32(screenCoord[0], screenCoord[1], 0));

				powerPellets.push_back(qPellet1);
			}

		}
	}

	// CHERRY
	math_t::Rectf_c recCherry(math_t::Rectf_c::width(.07f),
		math_t::Rectf_c::height(.07f));
	core_cs::entity_vptr qCherry0 = pref_gfx::Quad(entityMgr.get(), cpoolMgr.get()).
		Dimensions(recCherry).Create();

	//core_cs::entity_vptr qCherry0 = pref_gfx::Fan(entityMgr.get(), cpoolMgr.get())
	//	.Sides(64).Circle(circCherry).Create();
	pref_gfx::Material(entityMgr.get(), cpoolMgr.get())
		.AssetsPath(GetAssetsPath())
		.AddUniform(u_toCherry.get())
		.Add(qCherry0, core_io::Path(shaderPathVS), core_io::Path(shaderPathFS));
	qCherry0->GetComponent<math_cs::Transform>()->SetPosition(math_t::Vec3f32(2, 2, 0));


	// GHOST MATERIALS
	core_cs::entity_vptr ghost = entityMgr->CreateEntity();

	auto matGhost = pref_gfx::Material(entityMgr.get(), cpoolMgr.get())
		.AssetsPath(GetAssetsPath())
		.AddUniform(u_toGhost.get())
		.Construct(vsContents, fsContents);

	auto matBlinky = pref_gfx::Material(entityMgr.get(), cpoolMgr.get())
		.AssetsPath(GetAssetsPath())
		.AddUniform(u_toGhostBlinky.get())
		.Construct(vsContents, fsContents);

	auto matPinky = pref_gfx::Material(entityMgr.get(), cpoolMgr.get())
		.AssetsPath(GetAssetsPath())
		.AddUniform(u_toGhostPinky.get())
		.Construct(vsContents, fsContents);

	auto matInky = pref_gfx::Material(entityMgr.get(), cpoolMgr.get())
		.AssetsPath(GetAssetsPath())
		.AddUniform(u_toGhostInky.get())
		.Construct(vsContents, fsContents);	
	
	auto matClyde = pref_gfx::Material(entityMgr.get(), cpoolMgr.get())
		.AssetsPath(GetAssetsPath())
		.AddUniform(u_toGhostClyde.get())
		.Construct(vsContents, fsContents);

	entityMgr.get()->InsertComponent(core_cs::EntityManager::Params(ghost, matGhost));
	entityMgr.get()->InsertComponent(core_cs::EntityManager::Params(ghost, matBlinky));
	entityMgr.get()->InsertComponent(core_cs::EntityManager::Params(ghost, matPinky));
	entityMgr.get()->InsertComponent(core_cs::EntityManager::Params(ghost, matInky));
	entityMgr.get()->InsertComponent(core_cs::EntityManager::Params(ghost, matClyde));

	//math_t::Rectf_c recGhost(math_t::Rectf_c::width(.06f),
	//	math_t::Rectf_c::height(.06f));
	//core_cs::entity_vptr qBlinky = pref_gfx::Quad(entityMgr.get(), cpoolMgr.get()).
	//	Dimensions(recGhost).Create();
	//entityMgr.get()->InsertComponent(core_cs::EntityManager::Params(qBlinky, matBlinky));

	//GHOST 1, BLINKY
	math_t::Circlef32 cirGhost(math_t::Circlef32::radius(.04f));
	core_cs::entity_vptr qBlinky = pref_gfx::Fan(entityMgr.get(), cpoolMgr.get())
		.Sides(64).Circle(cirGhost).Create();

	pref_gfx::Material(entityMgr.get(), cpoolMgr.get())
		.AssetsPath(GetAssetsPath())
		.AddUniform(u_toGhostBlinky.get())
		.Add(qBlinky, core_io::Path(shaderPathVS), core_io::Path(shaderPathFS));

	auto blinkyScreenCoord = Grid2Screen(15, 11);
	qBlinky->GetComponent<math_cs::Transform>()->SetPosition(math_t::Vec3f32(blinkyScreenCoord[0], blinkyScreenCoord[1], 0));

	//GHOST 2, PINKY
	core_cs::entity_vptr qPinky = pref_gfx::Fan(entityMgr.get(), cpoolMgr.get())
		.Sides(64).Circle(cirGhost).Create();

	pref_gfx::Material(entityMgr.get(), cpoolMgr.get())
		.AssetsPath(GetAssetsPath())
		.AddUniform(u_toGhostPinky.get())
		.Add(qPinky, core_io::Path(shaderPathVS), core_io::Path(shaderPathFS));

	auto pinkyScreenCoord = Grid2Screen(14, 11);
	qPinky->GetComponent<math_cs::Transform>()->SetPosition(math_t::Vec3f32(pinkyScreenCoord[0], pinkyScreenCoord[1], 0));

	//GHOST 3, INKY
	core_cs::entity_vptr qInky = pref_gfx::Fan(entityMgr.get(), cpoolMgr.get())
		.Sides(64).Circle(cirGhost).Create();

	pref_gfx::Material(entityMgr.get(), cpoolMgr.get())
		.AssetsPath(GetAssetsPath())
		.AddUniform(u_toGhostInky.get())
		.Add(qInky, core_io::Path(shaderPathVS), core_io::Path(shaderPathFS));

	auto inkyScreenCoord = Grid2Screen(12, 14);
	qInky->GetComponent<math_cs::Transform>()->SetPosition(math_t::Vec3f32(inkyScreenCoord[0], inkyScreenCoord[1], 0));

	//GHOST 5, CLYDE
	core_cs::entity_vptr qClyde = pref_gfx::Fan(entityMgr.get(), cpoolMgr.get())
		.Sides(64).Circle(cirGhost).Create();

	pref_gfx::Material(entityMgr.get(), cpoolMgr.get())
		.AssetsPath(GetAssetsPath())
		.AddUniform(u_toGhostClyde.get())
		.Add(qClyde, core_io::Path(shaderPathVS), core_io::Path(shaderPathFS));

	auto clydeScreenCoord = Grid2Screen(17, 14);
	qClyde->GetComponent<math_cs::Transform>()->SetPosition(math_t::Vec3f32(clydeScreenCoord[0], clydeScreenCoord[1], 0));

	// PATHFINDING
	astar = new AStarPathFinder(pathNodes);
	//CreateBlinkyChasePath(math_t::Vec2f(0, 0), math_t::Vec2f(15, 22));
	CreateBlinkyPath(math_t::Vec2f(15, 11), math_t::Vec2f(22, 15), false);
	////astar->printPath();

	CreatePinkyPath(math_t::Vec2f(14, 11), math_t::Vec2f(22, 15), false);
	////astar->printPath();

	CreateInkyRandomPath(math_t::Vec2f(12, 14));
	////astar->printPath();

	CreateClydePath(math_t::Vec2f(17, 14), math_t::Vec2f(22, 15), false);
	////astar->printPath();

	// -----------------------------------------------------------------------
	// create a camera

	core_cs::entity_vptr camEnt =
		pref_gfx::Camera(entityMgr.get(), cpoolMgr.get())
		.Near(0.1f)
		.Far(100.0f)
		.Position(math_t::Vec3f(0, 0, 1.0f))
		.Perspective(false)
		.Create(win.GetDimensions());

	textSys->SetCamera(camEnt);

  //------------------------------------------------------------------------
  // All systems need to be initialized once

  quadSys.Initialize();
  fanSys.Initialize();
  matSys.Initialize();
  textSys->Initialize();
  sgSys.Initialize();
  camSys.Initialize();

  // In order to update at a pre-defined time interval, a timer must be created
  core_time::Timer32 inputFrameTime;
  inputFrameTime.Reset();
  core_time::Timer32 startingPhaseFrameTime;
  startingPhaseFrameTime.Reset();

  // PACMAN
  isPacmanMoving = false;
  pacmanMoveDir = MOVE_DIR_DOWN;
  pacmanNextMoveDir = MOVE_DIR_DOWN;

  tl_float sumDeltaPos = 0.0f;
  const tl_float deltaPos = 0.0085f;

  // BLINKY
  isBlinkyMoving = true;
  blinkyMoveDir = MOVE_DIR_NONE;
  tl_float blinkySumDeltaPos = 0.0f;
  const tl_float blinkyDeltaPos = 0.0085f;

  // PINKY
  isPinkyMoving = true;
  pinkyMoveDir = MOVE_DIR_NONE;
  tl_float pinkySumDeltaPos = 0.0f;
  const tl_float pinkyDeltaPos = 0.0085f;

  // INKY
  isInkyMoving = true;
  inkyMoveDir = MOVE_DIR_NONE;
  tl_float inkySumDeltaPos = 0.0f;
  const tl_float inkyDeltaPos = 0.0085f;

  // CLYDE
  isClydeMoving = true;
  clydeMoveDir = MOVE_DIR_NONE;
  tl_float clydeSumDeltaPos = 0.0f;
  const tl_float clydeDeltaPos = 0.0085f;

  //
  auto deltaOffset = (CELL_WIDTH / 2048.f)*1.5;
  auto ghostDeltaOffset = (CELL_WIDTH / 2048.f)*2.f;


  ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
  // Main loop
  while (win.IsValid() && !winCallback.m_endProgram)
  {
    gfx_win::WindowEvent  evt;
    while (win.GetEvent(evt))
    { }
	
	//------------------------------------------------------------------------
	// Update Input
	if (win.IsValid() && inputFrameTime.ElapsedMilliSeconds() > 16)
	{
		inputFrameTime.Reset();

		inputMgr->Update();

		if (keyboard->IsKeyDown(input_hid::KeyboardEvent::left))
		{
			pacmanNextMoveDir = MOVE_DIR_LEFT;
		}
		if (keyboard->IsKeyDown(input_hid::KeyboardEvent::right))
		{
			pacmanNextMoveDir = MOVE_DIR_RIGHT;
		}
		if (keyboard->IsKeyDown(input_hid::KeyboardEvent::up))
		{
			pacmanNextMoveDir = MOVE_DIR_UP;
		}
		if (keyboard->IsKeyDown(input_hid::KeyboardEvent::down))
		{
			pacmanNextMoveDir = MOVE_DIR_DOWN;
		}
		if (keyboard->IsKeyDown(input_hid::KeyboardEvent::space))
		{
			gameState = GAME_STATE_RUNNING;
			core_str::String numStr = core_str::Format("", score);
			strWMessage = core_str::CharAsciiToWide(numStr);
			dTextMessage->GetComponent<gfx_cs::DynamicText>()->Set(strWMessage);
		}

		if (gameState == GAME_STATE_RUNNING)
		{
			//auto test1 = Grid2Screen(1, 1);
			//auto test2 = Grid2Screen(2, 1);

			//test1 = test2 - test1;

			auto pacManTrans = qPacman->GetComponent<math_cs::Transform>();
			auto pacManScreenCoord = pacManTrans->GetPosition();
			auto pacManGridCoord = Screen2Grid(pacManScreenCoord[0], pacManScreenCoord[1]);

			auto blinkyTrans = qBlinky->GetComponent<math_cs::Transform>();
			auto blinkyScreenCoord = blinkyTrans->GetPosition();
			auto blinkyGridCoord = Screen2Grid(blinkyScreenCoord[0], blinkyScreenCoord[1]);

			auto pinkyTrans = qPinky->GetComponent<math_cs::Transform>();
			auto pinkyScreenCoord = pinkyTrans->GetPosition();
			auto pinkyGridCoord = Screen2Grid(pinkyScreenCoord[0], pinkyScreenCoord[1]);

			auto inkyTrans = qInky->GetComponent<math_cs::Transform>();
			auto inkyScreenCoord = inkyTrans->GetPosition();
			auto inkyGridCoord = Screen2Grid(inkyScreenCoord[0], inkyScreenCoord[1]);

			auto clydeTrans = qClyde->GetComponent<math_cs::Transform>();
			auto clydeScreenCoord = clydeTrans->GetPosition();
			auto clydeGridCoord = Screen2Grid(clydeScreenCoord[0], clydeScreenCoord[1]);

//PACMAN///////////////////////////////////////////////////////////////////////////////////////////////////
			if (!isPacmanMoving)
			{
				// direction switching
				switch (pacmanNextMoveDir)
				{
				case MOVE_DIR_LEFT:
				{
									  auto nextGridSpace = pathNodes[pacManGridCoord[0]][pacManGridCoord[1] - 1];

									  if (nextGridSpace->isWalkable)
									  {
										  pacmanMoveDir = MOVE_DIR_LEFT;
										  pacmanNextMoveDir = MOVE_DIR_NONE;
									  }
									  break;
				}
				case MOVE_DIR_RIGHT:
				{
									   auto nextGridSpace = pathNodes[pacManGridCoord[0]][pacManGridCoord[1] + 1];

									   if (nextGridSpace->isWalkable)
									   {
										   pacmanMoveDir = MOVE_DIR_RIGHT;
										   pacmanNextMoveDir = MOVE_DIR_NONE;
									   }
									   break;
				}
				case MOVE_DIR_UP:
				{
									auto nextGridSpace = pathNodes[pacManGridCoord[0] - 1][pacManGridCoord[1]];

									if (nextGridSpace->isWalkable)
									{
										pacmanMoveDir = MOVE_DIR_UP;
										pacmanNextMoveDir = MOVE_DIR_NONE;
									}
									break;
				}
				case MOVE_DIR_DOWN:
				{
									  auto nextGridSpace = pathNodes[pacManGridCoord[0] + 1][pacManGridCoord[1]];

									  if (nextGridSpace->isWalkable)
									  {
										  pacmanMoveDir = MOVE_DIR_DOWN;
										  pacmanNextMoveDir = MOVE_DIR_NONE;
									  }
									  break;
				}
				case MOVE_DIR_NONE:
					break;
				}

				// moving forward
				switch (pacmanMoveDir)
				{
				case MOVE_DIR_LEFT:
				{
									  auto nextGridSpace = pathNodes[pacManGridCoord[0]][pacManGridCoord[1] - 1];

									  if (nextGridSpace->isWalkable)
									  {
										  sumDeltaPos = 0;

										  pacManScreenCoord[0] -= deltaPos;
										  pacManTrans->SetPosition(pacManScreenCoord);
										  isPacmanMoving = true;
									  }
									  //else
									  //{
									  // if (core::rng::g_defaultRNG.GetRandomInteger(0, 2) == 1)
									  //nextMoveDir = MOVE_DIR_UP;
									  // else
									  //  nextMoveDir = MOVE_DIR_DOWN;
									  //}
									  break;
				}
				case MOVE_DIR_RIGHT:
				{
									   auto nextGridSpace = pathNodes[pacManGridCoord[0]][pacManGridCoord[1] + 1];

									   if (nextGridSpace->isWalkable)
									   {
										   sumDeltaPos = 0;

										   pacManScreenCoord[0] += deltaPos;
										   pacManTrans->SetPosition(pacManScreenCoord);

										   isPacmanMoving = true;
									   }
									   //else
									   //{
									   // if (core::rng::g_defaultRNG.GetRandomInteger(0, 2) == 1)
									   //  nextMoveDir = MOVE_DIR_UP;
									   // else
									   //  nextMoveDir = MOVE_DIR_DOWN;
									   //}
									   break;
				}
				case MOVE_DIR_UP:
				{
									auto nextGridSpace = pathNodes[pacManGridCoord[0] - 1][pacManGridCoord[1]];

									if (nextGridSpace->isWalkable)
									{
										sumDeltaPos = 0;

										pacManScreenCoord[1] += deltaPos;
										pacManTrans->SetPosition(pacManScreenCoord);

										isPacmanMoving = true;
									}
									else
										//{
										//	if (core::rng::g_defaultRNG.GetRandomInteger(0, 2) == 1)
										//		nextMoveDir = MOVE_DIR_LEFT;
										//	else
										//		nextMoveDir = MOVE_DIR_RIGHT;
										//}
										break;
				}
				case MOVE_DIR_DOWN:
				{
									  auto nextGridSpace = pathNodes[pacManGridCoord[0] + 1][pacManGridCoord[1]];

									  if (nextGridSpace->isWalkable)
									  {
										  sumDeltaPos = 0;

										  pacManScreenCoord[1] -= deltaPos;
										  pacManTrans->SetPosition(pacManScreenCoord);

										  isPacmanMoving = true;
									  }
									  //else
									  //{
									  // if (core::rng::g_defaultRNG.GetRandomInteger(0, 2) == 1)
									  //  nextMoveDir = MOVE_DIR_LEFT;
									  // else
									  //  nextMoveDir = MOVE_DIR_RIGHT;
									  //}
									  break;
				}

				case MOVE_DIR_NONE:
					break;
				}
			}
			else
			{
				// moving forward
				switch (pacmanMoveDir)
				{
				case MOVE_DIR_LEFT:
				{
									  pacManScreenCoord[0] -= deltaPos;

									  math_t::Mat3f32 newRot;
									  newRot.MakeRotationZ(3.14159265359 * -1.f);
									  pacManTrans->SetOrientation(newRot);

									  break;
				}
				case MOVE_DIR_RIGHT:
				{
									   pacManScreenCoord[0] += deltaPos;

									   math_t::Mat3f32 newRot;
									   newRot.MakeRotationZ(0.f);
									   pacManTrans->SetOrientation(newRot);
									   break;
				}
				case MOVE_DIR_UP:
				{
									pacManScreenCoord[1] += deltaPos;

									math_t::Mat3f32 newRot;
									newRot.MakeRotationZ(3.14159265359 * 0.5f);
									pacManTrans->SetOrientation(newRot);
									break;
				}
				case MOVE_DIR_DOWN:
				{
									  pacManScreenCoord[1] -= deltaPos;

									  math_t::Mat3f32 newRot;
									  newRot.MakeRotationZ(3.14159265359 * -0.5f);
									  pacManTrans->SetOrientation(newRot);
									  break;
				}
				case MOVE_DIR_NONE:
				{
									  sumDeltaPos -= deltaPos;
									  break;
				}
				}

				pacManTrans->SetPosition(pacManScreenCoord);

				sumDeltaPos += deltaPos;

				if (sumDeltaPos > deltaOffset)
				{
					isPacmanMoving = false;

					auto exactCoordSnap = Grid2Screen((int)(pacManGridCoord[1] + 0.5f), (int)(pacManGridCoord[0] + 0.5f));
					pacManTrans->SetPosition(math_t::Vec3f32(exactCoordSnap[0], exactCoordSnap[1], 0));

					auto curGridSpace = pathNodes[(int)(pacManGridCoord[0] + 0.5f)][(int)(pacManGridCoord[1] + 0.5f)];
					if (curGridSpace->hasPellet)
					{
						for (unsigned int i = 0; i < pellets.size(); i++)
						{
							auto pelletTrans = pellets[i]->GetComponent<math_cs::Transform>();
							auto pelletScreenCoord = pelletTrans->GetPosition();
							auto pelletGridCoord = Screen2Grid(pelletScreenCoord[0], pelletScreenCoord[1]);

							if (pelletGridCoord[0] == (int)(pacManGridCoord[0] + 0.5f) && pelletGridCoord[1] == (int)(pacManGridCoord[1] + 0.5f))
							{
								score += 10;
								pelletTrans->SetPosition(math_t::Vec3f32(99, 99, 0));
								curGridSpace->hasPellet = false;
								pelletsConsumed++;
								
								if (pelletsConsumed >= 248)
								{
									gameState = GAME_STATE_GAMEOVER;
								}
								break;
							}
						}
					}
					if (curGridSpace->hasPowerPellet)
					{
						for (unsigned int i = 0; i < powerPellets.size(); i++)
						{
							auto powerPelletsTrans = powerPellets[i]->GetComponent<math_cs::Transform>();
							auto powerPelletsScreenCoord = powerPelletsTrans->GetPosition();
							auto powerPelletsGridCoord = Screen2Grid(powerPelletsScreenCoord[0], powerPelletsScreenCoord[1]);

							if (powerPelletsGridCoord[0] == (int)(pacManGridCoord[0] + 0.5f) && powerPelletsGridCoord[1] == (int)(pacManGridCoord[1] + 0.5f))
							{
								score += 50;
								powerPelletsTrans->SetPosition(math_t::Vec3f32(99, 99, 0));
								curGridSpace->hasPowerPellet = false;

								////BLINKY
								isBlinkyScatter = true;

								//swap ghost texture
								auto matTemp = qBlinky->GetComponent<gfx_cs::Material>();
								if (matTemp != matGhost)
								*matTemp = *matGhost;
								//auto blinkyCurGridCoord = Screen2Grid(blinkyScreenCoord[0], blinkyScreenCoord[1]);
								CreateBlinkyPath(math_t::Vec2f(blinkyGridCoord[1], blinkyGridCoord[0]), math_t::Vec2f(1, 25), true);
								blinkyCurInterval = 0;

								////PINKY
								isPinkyScatter = true;

								//swap ghost texture
								matTemp = qPinky->GetComponent<gfx_cs::Material>();
								if (matTemp != matGhost)
									*matTemp = *matGhost;
								CreatePinkyPath(math_t::Vec2f(pinkyGridCoord[1], pinkyGridCoord[0]), math_t::Vec2f(1, 4), true);
								pinkyCurInterval = 0;

								////INKY
								isInkyScatter = true;

								//swap ghost texture
								matTemp = qInky->GetComponent<gfx_cs::Material>();
								if (matTemp != matGhost)
									*matTemp = *matGhost;
								CreateInkyPath(math_t::Vec2f(inkyGridCoord[1], inkyGridCoord[0]), math_t::Vec2f(25, 1), true);
								inkyCurInterval = 0;

								////CLYDE
								isClydeScatter = true;

								//swap ghost texture
								matTemp = qClyde->GetComponent<gfx_cs::Material>();
								if (matTemp != matGhost)
									*matTemp = *matGhost;
								CreateClydePath(math_t::Vec2f(clydeGridCoord[1], clydeGridCoord[0]), math_t::Vec2f(25, 28), true);
								clydeCurInterval = 0;


								powerModeCurInterval = 0;

								break;
							}
						}
					}
					if (curGridSpace->hasCherry)
					{
						auto cherryTrans = qCherry0->GetComponent<math_cs::Transform>();
						auto cherryScreenCoord = cherryTrans->GetPosition();
						auto cherryGridCoord = Screen2Grid(cherryScreenCoord[0], cherryScreenCoord[1]);

						if (cherryGridCoord[0] == (int)(pacManGridCoord[0] + 0.5f) && cherryGridCoord[1] == (int)(pacManGridCoord[1] + 0.5f))
						{
							hasCherryBeenEaten = true;
							curGridSpace->hasCherry = false;
							score += 100;

							auto cherryExactCoordSnap = Grid2Screen(28, 29);
							cherryTrans->SetPosition(math_t::Vec3f32(cherryExactCoordSnap[0], cherryExactCoordSnap[1], 0));
						}
						
					}

					// Spawn cherry to open space
					if (!hasCherrySpawned)
					{
						cherryCurInterval++;
						if (cherryCurInterval >= cherrySpawnInterval)
						{
							int randCol = core::rng::g_defaultRNG.GetRandomInteger(1, 28);
							int randRow = core::rng::g_defaultRNG.GetRandomInteger(1, 28);

							cherryNode = pathNodes[randCol][randRow];
							
							if (cherryNode->isWalkable && !cherryNode->hasPellet && !cherryNode->hasPowerPellet)
							{
								cherryNode->hasCherry = true;
								auto cherryExactCoordSnap = Grid2Screen(randRow, randCol);
								auto test321 = Screen2Grid(cherryExactCoordSnap[0], cherryExactCoordSnap[1]);

								qCherry0->GetComponent<math_cs::Transform>()->SetPosition(math_t::Vec3f32(cherryExactCoordSnap[0], cherryExactCoordSnap[1], 0));

								hasCherrySpawned = true;
								cherryCurInterval = 0;
							}

						}
					}
					else if (!hasCherryBeenEaten)
					{
						cherryCurInterval++;

						if (cherryCurInterval >= cherryDespawnInterval)
						{
							hasCherryBeenEaten = true;
							cherryNode->hasCherry = false;
							qCherry0->GetComponent<math_cs::Transform>()->SetPosition(math_t::Vec3f32(99, 99, 0));
						}

					}

					// Update scatter mode for all ghosts
					if (isBlinkyScatter || isPinkyScatter || isInkyScatter || isClydeScatter)
					{
						powerModeCurInterval += 1;

						if (powerModeCurInterval >= powerModeUpdateInterval)
						{
							powerModeCurInterval = 0;
							isBlinkyScatter = false;
							isPinkyScatter = false;
							isInkyScatter = false;
							isClydeScatter = false;

							auto matTemp = qBlinky->GetComponent<gfx_cs::Material>();
							if (matTemp != matBlinky)
								*matTemp = *matBlinky;

							matTemp = qPinky->GetComponent<gfx_cs::Material>();
							if (matTemp != matPinky)
								*matTemp = *matPinky;

							matTemp = qInky->GetComponent<gfx_cs::Material>();
							if (matTemp != matInky)
								*matTemp = *matInky;

							matTemp = qClyde->GetComponent<gfx_cs::Material>();
							if (matTemp != matClyde)
								*matTemp = *matClyde;

						}
					}

					if (pacmanSprite)
					{
						auto matTemp = qPacman->GetComponent<gfx_cs::Material>();
						if (matTemp != matPacman0)
							*matTemp = *matPacman0;
						pacmanSprite = !pacmanSprite;
					}
					else
					{
						auto matTemp = qPacman->GetComponent<gfx_cs::Material>();
						if (matTemp != matPacman1)
							*matTemp = *matPacman1;
						pacmanSprite = !pacmanSprite;
					}
				}
			}

//BLINKY//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
			if (!isBlinkyMoving && !isBlinkyScatter)
			{
				if (blinkyLowestPathIndex >= 1 && blinkyCurInterval <= blinkyUpdateInterval)// && lowestPathBlinky.size() - blinkyLowestPathIndex <= 10)
				{
					// direction of next move
					math_t::Vec2f blinkyCurCell = lowestPathBlinky[blinkyLowestPathIndex];
					math_t::Vec2f blinkyNextCell = lowestPathBlinky[blinkyLowestPathIndex - 1];
					auto blinkyNextDir = blinkyNextCell - blinkyCurCell;
					blinkyNextDir;

					blinkyMoveDir = GetGhostMoveDir(blinkyNextDir);
					if (blinkyMoveDir > -1)
						isBlinkyMoving = true;

					blinkyLowestPathIndex--;
					blinkyCurInterval++;
				}
				else
				{
					blinkyCurInterval = 0;
					CreateBlinkyPath(math_t::Vec2f(blinkyGridCoord[1], blinkyGridCoord[0]), math_t::Vec2f(pacManGridCoord[0], pacManGridCoord[1]), false);
					blinkyLowestPathIndex = lowestPathBlinky.size() - 1;
				}

				blinkySumDeltaPos = 0;
			}
			else if (!isBlinkyMoving && isBlinkyScatter)
			{
				if (blinkyLowestPathIndex >= 1)
				{
					// direction of next move
					math_t::Vec2f blinkyCurCell = lowestPathBlinky[blinkyLowestPathIndex];
					math_t::Vec2f blinkyNextCell = lowestPathBlinky[blinkyLowestPathIndex - 1];
					auto blinkyNextDir = blinkyNextCell - blinkyCurCell;
					blinkyNextDir;

					blinkyMoveDir = GetGhostMoveDir(blinkyNextDir);
					if (blinkyMoveDir > -1)
						isBlinkyMoving = true;

					blinkyLowestPathIndex--;
					blinkyCurInterval++;
				}
				else
				{
					blinkyCurInterval = 0;

					auto blinkyCurGridCoord = Screen2Grid(blinkyScreenCoord[0], blinkyScreenCoord[1]);

					if (BlinkyScatterPathPhase == 0)
					{
						CreateBlinkyPath(math_t::Vec2f(blinkyCurGridCoord[1], blinkyCurGridCoord[0]), math_t::Vec2f(1, 25), false);

						BlinkyScatterPathPhase++;
					}
					else if (BlinkyScatterPathPhase == 1)
					{
						CreateBlinkyPath(math_t::Vec2f(blinkyCurGridCoord[1], blinkyCurGridCoord[0]), math_t::Vec2f(1, 27), false);
						BlinkyScatterPathPhase++;
					}
					else if (BlinkyScatterPathPhase == 2)
					{
						CreateBlinkyPath(math_t::Vec2f(blinkyCurGridCoord[1], blinkyCurGridCoord[0]), math_t::Vec2f(5, 25), false);
						BlinkyScatterPathPhase++;
					}

					if (BlinkyScatterPathPhase > 2)
						BlinkyScatterPathPhase = 0;

					blinkyLowestPathIndex = lowestPathBlinky.size() - 1;
				}

				blinkySumDeltaPos = 0;
			}
			else
			{
				// moving forward
				switch (blinkyMoveDir)
				{
					case MOVE_DIR_LEFT:
					{
						blinkyScreenCoord[0] -= deltaPos;
						break;
					}
					case MOVE_DIR_RIGHT:
					{
						blinkyScreenCoord[0] += deltaPos;
						break;
					}
					case MOVE_DIR_UP:
					{
						blinkyScreenCoord[1] += deltaPos;
						break;
					}
					case MOVE_DIR_DOWN:
					{
						blinkyScreenCoord[1] -= deltaPos;
						break;
					}
					case MOVE_DIR_NONE:
					{
						sumDeltaPos -= deltaPos;
						break;
					}
				}

				blinkyTrans->SetPosition(blinkyScreenCoord);

				blinkySumDeltaPos += blinkyDeltaPos;

				if (blinkySumDeltaPos > ghostDeltaOffset)
				{
					//blinkyMoveDir = MOVE_DIR_NONE;
					isBlinkyMoving = false;

					auto blinkyExactCoordSnap = Grid2Screen(lowestPathBlinky[blinkyLowestPathIndex][0], lowestPathBlinky[blinkyLowestPathIndex][1]);
					blinkyTrans->SetPosition(math_t::Vec3f32(blinkyExactCoordSnap[0], blinkyExactCoordSnap[1], 0));
				}
			}

//PINKY//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
			if (!isPinkyMoving && !isPinkyScatter)
			{
				if (pinkyLowestPathIndex >= 1 && pinkyCurInterval <= pinkyUpdateInterval)// && lowestPathPinky.size() - pinkyLowestPathIndex <= 10)
				{
					// direction of next move
					math_t::Vec2f pinkyCurCell = lowestPathPinky[pinkyLowestPathIndex];
					math_t::Vec2f pinkyNextCell = lowestPathPinky[pinkyLowestPathIndex - 1];
					auto pinkyNextDir = pinkyNextCell - pinkyCurCell;
					pinkyNextDir;

					pinkyMoveDir = GetGhostMoveDir(pinkyNextDir);
					if (pinkyMoveDir > -1)
						isPinkyMoving = true;

					pinkyLowestPathIndex--;
					pinkyCurInterval++;
				}
				else
				{
					pinkyCurInterval = 0;
					CreatePinkyPath(math_t::Vec2f(pinkyGridCoord[1], pinkyGridCoord[0]), math_t::Vec2f(pacManGridCoord[0], pacManGridCoord[1]), false);
					pinkyLowestPathIndex = lowestPathPinky.size() - 1;
				}

				pinkySumDeltaPos = 0;
			}
			else if (!isPinkyMoving && isPinkyScatter)
			{
				if (pinkyLowestPathIndex >= 1)
				{
					// direction of next move
					math_t::Vec2f pinkyCurCell = lowestPathPinky[pinkyLowestPathIndex];
					math_t::Vec2f pinkyNextCell = lowestPathPinky[pinkyLowestPathIndex - 1];
					auto pinkyNextDir = pinkyNextCell - pinkyCurCell;
					pinkyNextDir;

					pinkyMoveDir = GetGhostMoveDir(pinkyNextDir);
					if (pinkyMoveDir > -1)
						isPinkyMoving = true;

					pinkyLowestPathIndex--;
					pinkyCurInterval++;
				}
				else
				{
					pinkyCurInterval = 0;

					auto pinkyCurGridCoord = Screen2Grid(pinkyScreenCoord[0], pinkyScreenCoord[1]);

					if (PinkyScatterPathPhase == 0)
					{
						CreatePinkyPath(math_t::Vec2f(pinkyCurGridCoord[1], pinkyCurGridCoord[0]), math_t::Vec2f(1, 4), false);
						PinkyScatterPathPhase++;
					}
					else if (PinkyScatterPathPhase == 1)
					{
						CreatePinkyPath(math_t::Vec2f(pinkyCurGridCoord[1], pinkyCurGridCoord[0]), math_t::Vec2f(1, 1), false);
						PinkyScatterPathPhase++;
					}
					else if (PinkyScatterPathPhase == 2)
					{
						CreatePinkyPath(math_t::Vec2f(pinkyCurGridCoord[1], pinkyCurGridCoord[0]), math_t::Vec2f(5, 4), false);
						PinkyScatterPathPhase++;
					}

					if (PinkyScatterPathPhase > 2)
						PinkyScatterPathPhase = 0;

					pinkyLowestPathIndex = lowestPathPinky.size() - 1;
				}

				pinkySumDeltaPos = 0;
			}
			else
			{
				// moving forward
				switch (pinkyMoveDir)
				{
				case MOVE_DIR_LEFT:
				{
					pinkyScreenCoord[0] -= deltaPos;
					break;
				}
				case MOVE_DIR_RIGHT:
				{
					pinkyScreenCoord[0] += deltaPos;
					break;
				}
				case MOVE_DIR_UP:
				{
					pinkyScreenCoord[1] += deltaPos;
					break;
				}
				case MOVE_DIR_DOWN:
				{
					pinkyScreenCoord[1] -= deltaPos;
					break;
				}
				case MOVE_DIR_NONE:
				{
					sumDeltaPos -= deltaPos;
					break;
				}
				}

				pinkyTrans->SetPosition(pinkyScreenCoord);

				pinkySumDeltaPos += pinkyDeltaPos;

				if (pinkySumDeltaPos > ghostDeltaOffset)
				{
					//pinkyMoveDir = MOVE_DIR_NONE;
					isPinkyMoving = false;

					auto pinkyExactCoordSnap = Grid2Screen(lowestPathPinky[pinkyLowestPathIndex][0], lowestPathPinky[pinkyLowestPathIndex][1]);
					pinkyTrans->SetPosition(math_t::Vec3f32(pinkyExactCoordSnap[0], pinkyExactCoordSnap[1], 0));
				}
			}

//INKY//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
			if (!isInkyMoving && !isInkyScatter)
			{
				if (inkyLowestPathIndex >= 1 && inkyCurInterval <= inkyUpdateInterval)// && lowestPathInky.size() - inkyLowestPathIndex <= 10)
				{
					// direction of next move
					math_t::Vec2f inkyCurCell = lowestPathInky[inkyLowestPathIndex];
					math_t::Vec2f inkyNextCell = lowestPathInky[inkyLowestPathIndex - 1];
					auto inkyNextDir = inkyNextCell - inkyCurCell;
					inkyNextDir;

					inkyMoveDir = GetGhostMoveDir(inkyNextDir);
					if (inkyMoveDir > -1)
						isInkyMoving = true;

					inkyLowestPathIndex--;
					inkyCurInterval++;
				}
				else
				{
					inkyCurInterval = 0;
					CreateInkyRandomPath(math_t::Vec2f(inkyGridCoord[1], inkyGridCoord[0]));
					////astar->printPath();
					//CreateInkyPath(math_t::Vec2f(inkyGridCoord[1], inkyGridCoord[0]), math_t::Vec2f(pacManGridCoord[0], pacManGridCoord[1]), false);
					inkyLowestPathIndex = lowestPathInky.size() - 1;
				}

				inkySumDeltaPos = 0;
			}
			else if (!isInkyMoving && isInkyScatter)
			{
				if (inkyLowestPathIndex >= 1)
				{
					// direction of next move
					math_t::Vec2f inkyCurCell = lowestPathInky[inkyLowestPathIndex];
					math_t::Vec2f inkyNextCell = lowestPathInky[inkyLowestPathIndex - 1];
					auto inkyNextDir = inkyNextCell - inkyCurCell;
					inkyNextDir;

					inkyMoveDir = GetGhostMoveDir(inkyNextDir);
					if (inkyMoveDir > -1)
						isInkyMoving = true;

					inkyLowestPathIndex--;
					inkyCurInterval++;
				}
				else
				{
					inkyCurInterval = 0;

					auto inkyCurGridCoord = Screen2Grid(inkyScreenCoord[0], inkyScreenCoord[1]);

					if (InkyScatterPathPhase == 0)
					{
						CreateInkyPath(math_t::Vec2f(inkyCurGridCoord[1], inkyCurGridCoord[0]), math_t::Vec2f(25, 1), false);
						InkyScatterPathPhase++;
					}
					else if (InkyScatterPathPhase == 1)
					{
						CreateInkyPath(math_t::Vec2f(inkyCurGridCoord[1], inkyCurGridCoord[0]), math_t::Vec2f(28, 13), false);
						InkyScatterPathPhase++;
					}
					else if (InkyScatterPathPhase == 2)
					{
						CreateInkyPath(math_t::Vec2f(inkyCurGridCoord[1], inkyCurGridCoord[0]), math_t::Vec2f(25, 13), false);
						InkyScatterPathPhase++;
					}

					if (InkyScatterPathPhase > 2)
						InkyScatterPathPhase = 0;

					inkyLowestPathIndex = lowestPathInky.size() - 1;
				}

				inkySumDeltaPos = 0;
			}
			else
			{
				// moving forward
				switch (inkyMoveDir)
				{
				case MOVE_DIR_LEFT:
				{
					inkyScreenCoord[0] -= deltaPos;
					break;
				}
				case MOVE_DIR_RIGHT:
				{
					inkyScreenCoord[0] += deltaPos;
					break;
				}
				case MOVE_DIR_UP:
				{
					inkyScreenCoord[1] += deltaPos;
					break;
				}
				case MOVE_DIR_DOWN:
				{
					inkyScreenCoord[1] -= deltaPos;
					break;
				}
				case MOVE_DIR_NONE:
				{
					sumDeltaPos -= deltaPos;
					break;
				}
				}

				inkyTrans->SetPosition(inkyScreenCoord);

				inkySumDeltaPos += inkyDeltaPos;

				if (inkySumDeltaPos > ghostDeltaOffset)
				{
					//inkyMoveDir = MOVE_DIR_NONE;
					isInkyMoving = false;

					auto inkyExactCoordSnap = Grid2Screen(lowestPathInky[inkyLowestPathIndex][0], lowestPathInky[inkyLowestPathIndex][1]);
					inkyTrans->SetPosition(math_t::Vec3f32(inkyExactCoordSnap[0], inkyExactCoordSnap[1], 0));
				}
			}

//CLYDE//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
			if (!isClydeMoving && !isClydeScatter &&!isClydeFalseScatter)
			{
				//if (clydeLowestPathIndex >= 8 && clydeCurInterval <= clydeUpdateInterval)
				//{

				//}
				if (clydeLowestPathIndex >= 8 && clydeCurInterval <= clydeUpdateInterval)// && lowestPathClyde.size() - clydeLowestPathIndex <= 10)
				{
					// direction of next move
					math_t::Vec2f clydeCurCell = lowestPathClyde[clydeLowestPathIndex];
					math_t::Vec2f clydeNextCell = lowestPathClyde[clydeLowestPathIndex - 1];
					auto clydeNextDir = clydeNextCell - clydeCurCell;
					clydeNextDir;

					clydeMoveDir = GetGhostMoveDir(clydeNextDir);
					if (clydeMoveDir > -1)
						isClydeMoving = true;

					clydeLowestPathIndex--;
					clydeCurInterval++;
				}
				else 
				{
					clydeCurInterval = 0;
					CreateClydePath(math_t::Vec2f(clydeGridCoord[1], clydeGridCoord[0]), math_t::Vec2f(pacManGridCoord[0], pacManGridCoord[1]), false);
					clydeLowestPathIndex = lowestPathClyde.size() - 1;

					if (clydeLowestPathIndex <= 8)
					{
						isClydeFalseScatter = true;
						CreateClydePath(math_t::Vec2f(clydeGridCoord[1], clydeGridCoord[0]), math_t::Vec2f(25, 28), true);
						clydeLowestPathIndex = lowestPathClyde.size() - 1;
					}

				}

				clydeSumDeltaPos = 0;
			}
			else if (!isClydeMoving && (isClydeScatter || isClydeFalseScatter))
			{
				//if (clydeLowestPathIndex >= 8 && clydeCurInterval <= clydeUpdateInterval)
				if (clydeLowestPathIndex >= 8 )
				{
					// direction of next move
					math_t::Vec2f clydeCurCell = lowestPathClyde[clydeLowestPathIndex];
					math_t::Vec2f clydeNextCell = lowestPathClyde[clydeLowestPathIndex - 1];
					auto clydeNextDir = clydeNextCell - clydeCurCell;
					clydeNextDir;

					clydeMoveDir = GetGhostMoveDir(clydeNextDir);
					if (clydeMoveDir > -1)
						isClydeMoving = true;

					clydeLowestPathIndex--;
					clydeCurInterval++;
				}
				else
				{
					clydeCurInterval = 0;

					CreateClydePath(math_t::Vec2f(clydeGridCoord[1], clydeGridCoord[0]), math_t::Vec2f(pacManGridCoord[0], pacManGridCoord[1]), false);
					clydeLowestPathIndex = lowestPathClyde.size() - 1;

					if (!isClydeFalseScatter || clydeLowestPathIndex <= 8)
					{
						auto clydeCurGridCoord = Screen2Grid(clydeScreenCoord[0], clydeScreenCoord[1]);

						if (ClydeScatterPathPhase == 0)
						{
							CreateClydePath(math_t::Vec2f(clydeCurGridCoord[1], clydeCurGridCoord[0]), math_t::Vec2f(25, 28), false);
							////astar->printPath();
							ClydeScatterPathPhase++;
						}
						else if (ClydeScatterPathPhase == 1)
						{
							CreateClydePath(math_t::Vec2f(clydeCurGridCoord[1], clydeCurGridCoord[0]), math_t::Vec2f(28, 17), false);
							//astar->printPath();
							ClydeScatterPathPhase++;
						}
						else if (ClydeScatterPathPhase == 2)
						{
							CreateClydePath(math_t::Vec2f(clydeCurGridCoord[1], clydeCurGridCoord[0]), math_t::Vec2f(25, 17), false);
							//astar->printPath();

							ClydeScatterPathPhase++;
						}

						if (ClydeScatterPathPhase > 2)
							ClydeScatterPathPhase = 0;

						clydeLowestPathIndex = lowestPathClyde.size() - 1;
					}
					else if (isClydeFalseScatter)
					{
						isClydeFalseScatter = false;
					}
				}

				clydeSumDeltaPos = 0;
			}
			else
			{
				// moving forward
				switch (clydeMoveDir)
				{
				case MOVE_DIR_LEFT:
				{
					clydeScreenCoord[0] -= deltaPos;
					break;
				}
				case MOVE_DIR_RIGHT:
				{
					clydeScreenCoord[0] += deltaPos;
					break;
				}
				case MOVE_DIR_UP:
				{
					clydeScreenCoord[1] += deltaPos;
					break;
				}
				case MOVE_DIR_DOWN:
				{
					clydeScreenCoord[1] -= deltaPos;
					break;
				}
				case MOVE_DIR_NONE:
				{
					sumDeltaPos -= deltaPos;
					break;
				}
				}

				clydeTrans->SetPosition(clydeScreenCoord);

				clydeSumDeltaPos += clydeDeltaPos;

				if (clydeSumDeltaPos > ghostDeltaOffset)
				{
					//clydeMoveDir = MOVE_DIR_NONE;
					isClydeMoving = false;

					auto clydeExactCoordSnap = Grid2Screen(lowestPathClyde[clydeLowestPathIndex][0], lowestPathClyde[clydeLowestPathIndex][1]);
					clydeTrans->SetPosition(math_t::Vec3f32(clydeExactCoordSnap[0], clydeExactCoordSnap[1], 0));
				}
			}


/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
			// PACMAN COLLISION WITH GHOSTS
			bool isBlinkyColliding = (int)(blinkyGridCoord[0] + 0.5f) == (int)(pacManGridCoord[0] + 0.5f) && (int)(blinkyGridCoord[1] + 0.5f) == (int)(pacManGridCoord[1] + 0.5f);
			bool isPinkyColliding = (int)(pinkyGridCoord[0] + 0.5f) == (int)(pacManGridCoord[0] + 0.5f) && (int)(pinkyGridCoord[1] + 0.5f) == (int)(pacManGridCoord[1] + 0.5f);
			bool isInkyColliding = (int)(inkyGridCoord[0] + 0.5f) == (int)(pacManGridCoord[0] + 0.5f) && (int)(inkyGridCoord[1] + 0.5f) == (int)(pacManGridCoord[1] + 0.5f);
			bool isClydeColliding = (int)(clydeGridCoord[0] + 0.5f) == (int)(pacManGridCoord[0] + 0.5f) && (int)(clydeGridCoord[1] + 0.5f) == (int)(pacManGridCoord[1] + 0.5f);

			if (isBlinkyColliding || isPinkyColliding || isInkyColliding || isClydeColliding)
			{
				if (isBlinkyColliding && isBlinkyScatter)
				{
					TLOC_LOG_CORE_DEBUG() << "BLINKY EATEN";
					//respawn ghost
					auto matTemp = qBlinky->GetComponent<gfx_cs::Material>();
					if (matTemp != matBlinky)
						*matTemp = *matBlinky;

					auto blinkySpawnScreenCoord = Grid2Screen(15, 11);
					blinkyTrans->SetPosition(math_t::Vec3f32(blinkySpawnScreenCoord[0], blinkySpawnScreenCoord[1], 0));
					CreateBlinkyPath(math_t::Vec2f(15, 11), math_t::Vec2f(pacManGridCoord[0], pacManGridCoord[1]), false);

					isBlinkyScatter = false;
					isBlinkyMoving = false;
					//blinkyMoveDir = MOVE_DIR_NONE;
					blinkyLowestPathIndex = lowestPathBlinky.size() - 1;

					score += 200;
				}
				else if (isPinkyColliding && isPinkyScatter)
				{
					TLOC_LOG_CORE_DEBUG() << "PINKY EATEN";
					//respawn ghost
					auto matTemp = qPinky->GetComponent<gfx_cs::Material>();
					if (matTemp != matPinky)
						*matTemp = *matPinky;

					auto pinkySpawnScreenCoord = Grid2Screen(15, 11);
					pinkyTrans->SetPosition(math_t::Vec3f32(pinkySpawnScreenCoord[0], pinkySpawnScreenCoord[1], 0));
					CreatePinkyPath(math_t::Vec2f(14, 11), math_t::Vec2f(pacManGridCoord[0], pacManGridCoord[1]), false);

					isPinkyScatter = false;
					isPinkyMoving = false;
					//pinkyMoveDir = MOVE_DIR_NONE;
					pinkyLowestPathIndex = lowestPathPinky.size() - 1;

					score += 200;
				}
				else if (isInkyColliding && isInkyScatter)
				{
					TLOC_LOG_CORE_DEBUG() << "INKY EATEN";
					//respawn ghost
					auto matTemp = qInky->GetComponent<gfx_cs::Material>();
					if (matTemp != matInky)
						*matTemp = *matInky;

					auto inkySpawnScreenCoord = Grid2Screen(15, 11);
					inkyTrans->SetPosition(math_t::Vec3f32(inkySpawnScreenCoord[0], inkySpawnScreenCoord[1], 0));
					CreateInkyPath(math_t::Vec2f(14, 11), math_t::Vec2f(pacManGridCoord[0], pacManGridCoord[1]), false);

					isInkyScatter = false;
					isInkyMoving = false;
					//inkyMoveDir = MOVE_DIR_NONE;
					inkyLowestPathIndex = lowestPathInky.size() - 1;

					score += 200;
				}
				else if (isClydeColliding && isClydeScatter)
				{
					TLOC_LOG_CORE_DEBUG() << "PINKY EATEN";
					//respawn ghost
					auto matTemp = qClyde->GetComponent<gfx_cs::Material>();
					if (matTemp != matClyde)
						*matTemp = *matClyde;

					auto clydeSpawnScreenCoord = Grid2Screen(17, 14);
					clydeTrans->SetPosition(math_t::Vec3f32(clydeSpawnScreenCoord[0], clydeSpawnScreenCoord[1], 0));
					CreateClydePath(math_t::Vec2f(17, 14), math_t::Vec2f(pacManGridCoord[0], pacManGridCoord[1]), false);

					isClydeScatter = false;
					isClydeMoving = false;
					//clydeMoveDir = MOVE_DIR_NONE;
					clydeLowestPathIndex = lowestPathClyde.size() - 1;

					score += 200;
				}
				else
				{
					isPacmanMoving = false;
					pacmanMoveDir = MOVE_DIR_NONE;
					pacmanNextMoveDir = MOVE_DIR_NONE;

					sumDeltaPos = 0;

					lives--;
					if (lives > -1)
					{
						extraLives[lives]->GetComponent<math_cs::Transform>()->SetPosition(math_t::Vec3f32(99, 99, 0));

						//respawn pacman
						auto pacmanSpawnScreenCoord = Grid2Screen(15, 22);
						qPacman->GetComponent<math_cs::Transform>()->SetPosition(math_t::Vec3f32(pacmanSpawnScreenCoord[0], pacmanSpawnScreenCoord[1], 0));

						//respawn ghosts
						auto blinkySpawnScreenCoord = Grid2Screen(15, 11);
						qBlinky->GetComponent<math_cs::Transform>()->SetPosition(math_t::Vec3f32(blinkySpawnScreenCoord[0], blinkySpawnScreenCoord[1], 0));
						CreateBlinkyPath(math_t::Vec2f(15, 11), math_t::Vec2f(pacManGridCoord[0], pacManGridCoord[1]), false);
						isBlinkyScatter = false;
						isBlinkyMoving = false;
						blinkyLowestPathIndex = lowestPathBlinky.size() - 1;
						blinkyCurInterval = 0;
						auto matTemp = qBlinky->GetComponent<gfx_cs::Material>();
						if (matTemp != matBlinky)
							*matTemp = *matBlinky;

						auto pinkySpawnScreenCoord = Grid2Screen(14, 11);
						qPinky->GetComponent<math_cs::Transform>()->SetPosition(math_t::Vec3f32(pinkySpawnScreenCoord[0], pinkySpawnScreenCoord[1], 0));
						CreatePinkyPath(math_t::Vec2f(14, 11), math_t::Vec2f(pacManGridCoord[0], pacManGridCoord[1]), false);
						isPinkyScatter = false;
						isPinkyMoving = false;
						pinkyLowestPathIndex = lowestPathPinky.size() - 1;
						pinkyCurInterval = 0;
						matTemp = qPinky->GetComponent<gfx_cs::Material>();
						if (matTemp != matPinky)
							*matTemp = *matPinky;

						auto inkySpawnScreenCoord = Grid2Screen(12, 14);
						qInky->GetComponent<math_cs::Transform>()->SetPosition(math_t::Vec3f32(inkySpawnScreenCoord[0], inkySpawnScreenCoord[1], 0));
						CreateInkyRandomPath(math_t::Vec2f(12, 14));
						isInkyScatter = false;
						isInkyMoving = false;
						inkyLowestPathIndex = lowestPathInky.size() - 1;
						inkyCurInterval = 0;
						matTemp = qInky->GetComponent<gfx_cs::Material>();
						if (matTemp != matInky)
							*matTemp = *matInky;

						auto clydeSpawnScreenCoord = Grid2Screen(17, 14);
						qClyde->GetComponent<math_cs::Transform>()->SetPosition(math_t::Vec3f32(clydeSpawnScreenCoord[0], clydeSpawnScreenCoord[1], 0));
						CreateClydePath(math_t::Vec2f(17, 14), math_t::Vec2f(pacManGridCoord[0], pacManGridCoord[1]), false);
						isClydeScatter = false;
						isClydeMoving = false;
						clydeLowestPathIndex = lowestPathClyde.size() - 1;
						clydeCurInterval = 0;
						matTemp = qClyde->GetComponent<gfx_cs::Material>();
						if (matTemp != matClyde)
							*matTemp = *matClyde;
					}
					else
					{
						gameState = GAME_STATE_GAMEOVER;
						TLOC_LOG_CORE_DEBUG() << "GAMEOVER";
					}

					TLOC_LOG_CORE_DEBUG() << "LIVES: " << lives;
				}
			}

			core_str::String numStr = core_str::Format("Score %i", score);
			strWScore = core_str::CharAsciiToWide(numStr);
			dTextScore->GetComponent<gfx_cs::DynamicText>()->Set(strWScore);

			inputMgr->Reset();
		}
		else if (gameState == GAME_STATE_STARTING)
		{
			core_str::String numStr = core_str::Format("Get Ready!", score);
			strWMessage = core_str::CharAsciiToWide(numStr);

			if (startingPhaseFrameTime.ElapsedSeconds() > 5)
			{
				gameState = GAME_STATE_RUNNING;
				core_str::String numStr = core_str::Format("", score);
				strWMessage = core_str::CharAsciiToWide(numStr);
			}

			dTextMessage->GetComponent<gfx_cs::DynamicText>()->Set(strWMessage);
		}
		else if (gameState == GAME_STATE_GAMEOVER)
		{
			strWMessage = core_str::CharAsciiToWide("GAME OVER!");
			dTextMessage->GetComponent<gfx_cs::DynamicText>()->Set(strWMessage);
		}
	}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

    renderer->ApplyRenderSettings();
	camSys.ProcessActiveEntities();
	sgSys.ProcessActiveEntities();

    quadSys.ProcessActiveEntities();
	fanSys.ProcessActiveEntities();
	textSys->ProcessActiveEntities();

    win.SwapBuffers();
  }

  //------------------------------------------------------------------------
  // Exiting
  TLOC_LOG_CORE_INFO() << "Exiting normally";

  if (score > highScore)
  {
	  // Update highscore.txt
	  TLOC_LOG_CORE_INFO() << "NEW HIGH SCORE";

	  core_io::FileIO_WriteB wbHighscore(highscorePath);
	  wbHighscore.Open();
	  core_str::String newHighScore = core_str::Format("%i", score);
	  wbHighscore.Write(newHighScore);
  }
  return 0;

}
