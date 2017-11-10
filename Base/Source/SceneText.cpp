#include "SceneText.h"
#include "GL\glew.h"

#include "shader.hpp"
#include "MeshBuilder.h"
#include "Application.h"
#include "Utility.h"
#include "LoadTGA.h"
#include <sstream>
#include "KeyboardController.h"
#include "MouseController.h"
#include "SceneManager.h"
#include "GraphicsManager.h"
#include "ShaderProgram.h"
#include "EntityManager.h"

#include "GenericEntity.h"
#include "GroundEntity.h"
#include "TextEntity.h"
#include "SpriteEntity.h"
#include "Light.h"
#include "SkyBox/SkyBoxEntity.h"
#include "Minimap\Minimap.h"
#include "../Source/WeaponInfo/WeaponInfo.h"
#include "SceneGraph\SceneGraph.h"
#include "SceneGraph\SceneNode.h"
#include <iostream>
using namespace std;

SceneText* SceneText::sInstance = new SceneText(SceneManager::GetInstance());

SceneText::SceneText()
	: theMinimap(NULL)
	, theCameraEffects(NULL)
{
}

SceneText::SceneText(SceneManager* _sceneMgr)
	: theMinimap(NULL)
	, theCameraEffects(NULL)
{
	_sceneMgr->AddScene("Start", this);
}

SceneText::~SceneText()
{
	if (theCameraEffects)
	{
		delete theCameraEffects;
		theCameraEffects = NULL;
	}
	if (theMinimap)
	{
		delete theMinimap;
		theMinimap = NULL;
	}
	if (theMouse)
	{
		delete theMouse;
		theMouse = NULL;
	}
	if (theKeyboard)
	{
		delete theKeyboard;
		theKeyboard = NULL;
	}
}

void SceneText::Init()
{
	currProg = GraphicsManager::GetInstance()->LoadShader("default", "Shader//Texture.vertexshader", "Shader//Texture.fragmentshader");
	
	// Tell the shader program to store these uniform locations
	currProg->AddUniform("MVP");
	currProg->AddUniform("MV");
	currProg->AddUniform("MV_inverse_transpose");
	currProg->AddUniform("material.kAmbient");
	currProg->AddUniform("material.kDiffuse");
	currProg->AddUniform("material.kSpecular");
	currProg->AddUniform("material.kShininess");
	currProg->AddUniform("lightEnabled");
	currProg->AddUniform("numLights");
	currProg->AddUniform("lights[0].type");
	currProg->AddUniform("lights[0].position_cameraspace");
	currProg->AddUniform("lights[0].color");
	currProg->AddUniform("lights[0].power");
	currProg->AddUniform("lights[0].kC");
	currProg->AddUniform("lights[0].kL");
	currProg->AddUniform("lights[0].kQ");
	currProg->AddUniform("lights[0].spotDirection");
	currProg->AddUniform("lights[0].cosCutoff");
	currProg->AddUniform("lights[0].cosInner");
	currProg->AddUniform("lights[0].exponent");
	currProg->AddUniform("lights[1].type");
	currProg->AddUniform("lights[1].position_cameraspace");
	currProg->AddUniform("lights[1].color");
	currProg->AddUniform("lights[1].power");
	currProg->AddUniform("lights[1].kC");
	currProg->AddUniform("lights[1].kL");
	currProg->AddUniform("lights[1].kQ");
	currProg->AddUniform("lights[1].spotDirection");
	currProg->AddUniform("lights[1].cosCutoff");
	currProg->AddUniform("lights[1].cosInner");
	currProg->AddUniform("lights[1].exponent");
	currProg->AddUniform("colorTextureEnabled");
	currProg->AddUniform("colorTexture");
	currProg->AddUniform("textEnabled");
	currProg->AddUniform("textColor");
	
	// Tell the graphics manager to use the shader we just loaded
	GraphicsManager::GetInstance()->SetActiveShader("default");

	lights[0] = new Light();
	GraphicsManager::GetInstance()->AddLight("lights[0]", lights[0]);
	lights[0]->type = Light::LIGHT_DIRECTIONAL;
	lights[0]->position.Set(0, 20, 0);
	lights[0]->color.Set(1, 1, 1);
	lights[0]->power = 1;
	lights[0]->kC = 1.f;
	lights[0]->kL = 0.01f;
	lights[0]->kQ = 0.001f;
	lights[0]->cosCutoff = cos(Math::DegreeToRadian(45));
	lights[0]->cosInner = cos(Math::DegreeToRadian(30));
	lights[0]->exponent = 3.f;
	lights[0]->spotDirection.Set(0.f, 1.f, 0.f);
	lights[0]->name = "lights[0]";

	lights[1] = new Light();
	GraphicsManager::GetInstance()->AddLight("lights[1]", lights[1]);
	lights[1]->type = Light::LIGHT_DIRECTIONAL;
	lights[1]->position.Set(1, 1, 0);
	lights[1]->color.Set(1, 1, 0.5f);
	lights[1]->power = 0.4f;
	lights[1]->name = "lights[1]";

	currProg->UpdateInt("numLights", 1);
	currProg->UpdateInt("textEnabled", 0);
	
	// Create the playerinfo instance, which manages all information about the player
	playerInfo = CPlayerInfo::GetInstance();
	playerInfo->Init();

	// Create and attach the camera to the scene
	//camera.Init(Vector3(0, 0, 10), Vector3(0, 0, 0), Vector3(0, 1, 0));
	camera.Init(playerInfo->GetPos(), playerInfo->GetTarget(), playerInfo->GetUp());
	playerInfo->AttachCamera(&camera);
	GraphicsManager::GetInstance()->AttachCamera(&camera);

	// Load all the meshes
	MeshBuilder::GetInstance()->GenerateAxes("reference");
	MeshBuilder::GetInstance()->GenerateCrossHair("crosshair");
	MeshBuilder::GetInstance()->GenerateQuad("quad", Color(1, 1, 1), 1.f);
	MeshBuilder::GetInstance()->GetMesh("quad")->textureID = LoadTGA("Image//calibri.tga");
	MeshBuilder::GetInstance()->GenerateText("text", 16, 16);
	MeshBuilder::GetInstance()->GetMesh("text")->textureID = LoadTGA("Image//calibri.tga");
	MeshBuilder::GetInstance()->GetMesh("text")->material.kAmbient.Set(1, 0, 0);
	MeshBuilder::GetInstance()->GenerateOBJ("Chair", "OBJ//chair.obj");
	MeshBuilder::GetInstance()->GetMesh("Chair")->textureID = LoadTGA("Image//chair.tga");
	MeshBuilder::GetInstance()->GenerateRing("ring", Color(1, 0, 1), 36, 1, 0.5f);
	MeshBuilder::GetInstance()->GenerateSphere("lightball", Color(1, 1, 1), 18, 36, 1.f);
	MeshBuilder::GetInstance()->GenerateSphere("sphere", Color(1, 0, 0), 18, 36, 10.f);
	MeshBuilder::GetInstance()->GenerateCone("cone", Color(0.5f, 1, 0.3f), 36, 10.f, 10.f);
	MeshBuilder::GetInstance()->GenerateCube("cube", Color(1.0f, 1.0f, 0.0f), 1.0f);
	MeshBuilder::GetInstance()->GetMesh("cone")->material.kDiffuse.Set(0.99f, 0.99f, 0.99f);
	MeshBuilder::GetInstance()->GetMesh("cone")->material.kSpecular.Set(0.f, 0.f, 0.f);
	MeshBuilder::GetInstance()->GenerateQuad("GRASS_DARKGREEN", Color(1, 1, 1), 1.f);
	MeshBuilder::GetInstance()->GetMesh("GRASS_DARKGREEN")->textureID = LoadTGA("Image//grass_darkgreen.tga");
	MeshBuilder::GetInstance()->GenerateQuad("GEO_GRASS_LIGHTGREEN", Color(1, 1, 1), 1.f);
	MeshBuilder::GetInstance()->GetMesh("GEO_GRASS_LIGHTGREEN")->textureID = LoadTGA("Image//grass_lightgreen.tga");

	MeshBuilder::GetInstance()->GenerateQuad("GEO_FLOOR", Color(1, 1, 1), 1.f);
	MeshBuilder::GetInstance()->GetMesh("GEO_FLOOR")->textureID = LoadTGA("Image//floor_2.tga");

	MeshBuilder::GetInstance()->GenerateQuad("SKYBOX_FRONT", Color(1, 1, 1), 1.f);
	MeshBuilder::GetInstance()->GenerateQuad("SKYBOX_BACK", Color(1, 1, 1), 1.f);
	MeshBuilder::GetInstance()->GenerateQuad("SKYBOX_LEFT", Color(1, 1, 1), 1.f);
	MeshBuilder::GetInstance()->GenerateQuad("SKYBOX_RIGHT", Color(1, 1, 1), 1.f);
	MeshBuilder::GetInstance()->GenerateQuad("SKYBOX_TOP", Color(1, 1, 1), 1.f);
	MeshBuilder::GetInstance()->GenerateQuad("SKYBOX_BOTTOM", Color(1, 1, 1), 1.f);
	MeshBuilder::GetInstance()->GetMesh("SKYBOX_FRONT")->textureID = LoadTGA("Image//SkyBox//skybox_front.tga");
	MeshBuilder::GetInstance()->GetMesh("SKYBOX_BACK")->textureID = LoadTGA("Image//SkyBox//skybox_back.tga");
	MeshBuilder::GetInstance()->GetMesh("SKYBOX_LEFT")->textureID = LoadTGA("Image//SkyBox//skybox_left.tga");
	MeshBuilder::GetInstance()->GetMesh("SKYBOX_RIGHT")->textureID = LoadTGA("Image//SkyBox//skybox_right.tga");
	MeshBuilder::GetInstance()->GetMesh("SKYBOX_TOP")->textureID = LoadTGA("Image//SkyBox//skybox_top.tga");
	MeshBuilder::GetInstance()->GetMesh("SKYBOX_BOTTOM")->textureID = LoadTGA("Image//SkyBox//skybox_bottom.tga");

	MeshBuilder::GetInstance()->GenerateOBJ("PISTOL_BULLET", "OBJ//pistol_bullet.obj");
	MeshBuilder::GetInstance()->GetMesh("PISTOL_BULLET")->textureID = LoadTGA("Image//pistol_bullet.tga");

	MeshBuilder::GetInstance()->GenerateOBJ("BULLET", "OBJ//bullet.obj");
	MeshBuilder::GetInstance()->GetMesh("BULLET")->textureID = LoadTGA("Image//bullet.tga");

	MeshBuilder::GetInstance()->GenerateOBJ("TARGET", "OBJ//target.obj");
	MeshBuilder::GetInstance()->GetMesh("TARGET")->textureID = LoadTGA("Image//floor_2.tga");

	

	

	//theCameraEffects->RifleMesh = MeshBuilder::GetInstance()->GenerateQuad("ARicon", Color(1, 1, 1), 1.f);
	//theCameraEffects->RifleMesh->textureID = LoadTGA("Image//rifle_icon.tga");

	// Create entities into the scene
	Create::Entity("reference", Vector3(0.0f, 0.0f, 0.0f)); // Reference
	Create::Entity("lightball", Vector3(lights[0]->position.x, lights[0]->position.y, lights[0]->position.z)); // Lightball
	//GenericEntity* aCube = Create::Entity("cube", Vector3(-20.0f, 0.0f, -20.0f));
	Create::Entity("ring", Vector3(0.0f, 0.0f, 0.0f)); // Reference

	
	groundEntity = Create::Ground("GEO_FLOOR","GEO_FLOOR");
//	Create::Text3DObject("text", Vector3(0.0f, 0.0f, 0.0f), "DM2210", Vector3(10.0f, 10.0f, 10.0f), Color(0, 1, 1));
	Create::Sprite2DObject("crosshair", Vector3(0.0f, 0.0f, 0.0f), Vector3(10.0f, 10.0f, 10.0f));

	SkyBoxEntity* theSkyBox = Create::SkyBox("SKYBOX_FRONT", "SKYBOX_BACK",
											 "SKYBOX_LEFT", "SKYBOX_RIGHT",
											 "SKYBOX_TOP", "SKYBOX_BOTTOM");

	// Customise the ground entity
	groundEntity->SetPosition(Vector3(0, -10, 0));
	groundEntity->SetScale(Vector3(100.0f, 100.0f, 100.0f));
	groundEntity->SetGrids(Vector3(10.0f, 1.0f, 10.0f));
	playerInfo->SetTerrain(groundEntity);

	// Create a CEnemy instance
	anEnemy3D = Create::Enemy3D("TARGET", Vector3(-20.0f, 0.0f, -20.0f));
	anEnemy3D->Init();
	anEnemy3D->SetPos(Vector3(-30.0f, 0.0f, -100));
	anEnemy3D->SetTerrain(groundEntity);
	anEnemy3D->SetPAABB(Vector3(anEnemy3D->GetScale().x, anEnemy3D->GetScale().y, anEnemy3D->GetScale().z), anEnemy3D->GetPosition());
	Enemy.push_back(anEnemy3D);

	CSceneNode*theNode = CSceneGraph::GetInstance()->AddNode(anEnemy3D);
	if (theNode == NULL)
	{
		cout << "EntityManager::AddEntity: Unable to add to scene graph!" << endl;
	}
	anEnemy3D = Create::Enemy3D("TARGET", Vector3(-20.0f, 0.0f, -20.0f));
	anEnemy3D->Init();
	anEnemy3D->SetPos(Vector3(-00.0f, 0.0f, -100));
	anEnemy3D->SetTerrain(groundEntity);
	anEnemy3D->SetPAABB(Vector3(anEnemy3D->GetScale().x, anEnemy3D->GetScale().y, anEnemy3D->GetScale().z), anEnemy3D->GetPosition());
	Enemy.push_back(anEnemy3D);

	CSceneNode* anotherNode = theNode->AddChild(anEnemy3D);
	if (anotherNode == NULL)
	{
		cout << "EntityManager::AddEntity: Unable to add to scene graph!" << endl;
	}

	anEnemy3D = Create::Enemy3D("TARGET", Vector3(30.0f, 0.0f, -100.0f));
	anEnemy3D->Init();
	anEnemy3D->SetPos(Vector3(30.0f, 0.0f, -100));
	anEnemy3D->SetTerrain(groundEntity);
	anEnemy3D->SetPAABB(Vector3(anEnemy3D->GetScale().x, anEnemy3D->GetScale().y, anEnemy3D->GetScale().z), anEnemy3D->GetPosition());
	Enemy.push_back(anEnemy3D);

	//theCube = Create::Entity("Left_Wall", Vector3(0, 0, 0));
	//theCube->SetPosition(0,0,0)


	// Setup the 2D entities
	float halfWindowWidth = Application::GetInstance().GetWindowWidth() / 3.3f;
	float halfWindowHeight = Application::GetInstance().GetWindowHeight() / 2.3f;
	float fontSize = 40.0f;
	float halfFontSize = fontSize / 2.0f;
	for (int i = 0; i < 4; ++i)
	{
		textObj[i] = Create::Text2DObject("text", Vector3(-halfWindowWidth, -halfWindowHeight + fontSize*i + halfFontSize, 0.0f), "", Vector3(fontSize, fontSize, fontSize), Color(0.0f,1.0f,0.0f));
	}
	

	// Hardware Abstraction
	theKeyboard = new CKeyboard();
	theKeyboard->Create(playerInfo);

	// Activate the Blend Function
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	// Minimap
	theMinimap = Create::Minimap(false);
	theMinimap->SetBackground(MeshBuilder::GetInstance()->GenerateQuad("MINIMAP", Color(1, 1, 1), 1.f));
	theMinimap->GetBackground()->textureID = LoadTGA("Image//floor_2.tga");
	theMinimap->SetBorder(MeshBuilder::GetInstance()->GenerateCircle("MINIMAPBORDER", Color(1, 1, 1), 1.05f));
	theMinimap->SetAvatar(MeshBuilder::GetInstance()->GenerateQuad("MINIMAPAVATAR", Color(1, 1, 0), 0.25f));
	theMinimap->GetAvatar()->textureID = LoadTGA("Image//Avatar.tga");
	theMinimap->SetStencil(MeshBuilder::GetInstance()->GenerateCircle("MINIMAP_STENCIL", Color(1, 1, 1), 1.0f));

	// CameraEffects
	theCameraEffects = Create::CameraEffects(false);
	theCameraEffects->SetBloodScreen(MeshBuilder::GetInstance()->GenerateQuad("CAMERAEFFECTS_BLOODSCREEN", 
										Color(1, 1, 1), 1.f));
	theCameraEffects->GetBloodScreen()->textureID = LoadTGA("Image//CameraEffects_Blood.tga");
	theCameraEffects->SetStatus_BloodScreen(false);

	theCameraEffects->PistolMesh = MeshBuilder::GetInstance()->GenerateQuad("PistolMesh", Color(1, 1, 1), 1.f);
	theCameraEffects->PistolMesh->textureID = LoadTGA("Image//pistol.tga");


	theCameraEffects->PistolMeshFired = MeshBuilder::GetInstance()->GenerateQuad("PistolMeshFired", Color(1, 1, 1), 1.f);
	theCameraEffects->PistolMeshFired->textureID = LoadTGA("Image//pistol_shot.tga");

	theCameraEffects->RifleMesh = MeshBuilder::GetInstance()->GenerateQuad("RifleMesh", Color(1, 1, 1), 1.f);
	theCameraEffects->RifleMesh->textureID = LoadTGA("Image//rifle.tga");

	theCameraEffects->RifleMeshFired = MeshBuilder::GetInstance()->GenerateQuad("RifleMeshFired", Color(1, 1, 1), 1.f);
	theCameraEffects->RifleMeshFired->textureID = LoadTGA("Image//rifle_fired.tga");


	theCameraEffects->Crosshair = MeshBuilder::GetInstance()->GenerateQuad("Crosshair", Color(1, 1, 1), 1.f);
	theCameraEffects->Crosshair->textureID = LoadTGA("Image//crosshair.tga");

	theCameraEffects->FullHP = MeshBuilder::GetInstance()->GenerateQuad("FullHP", Color(1, 1, 1), 1.f);
	theCameraEffects->FullHP->textureID = LoadTGA("Image//full_hp.tga");

	theCameraEffects->One_Third_HP = MeshBuilder::GetInstance()->GenerateQuad("One_Third_HP", Color(1, 1, 1), 1.f);
	theCameraEffects->One_Third_HP->textureID = LoadTGA("Image//one_third_hp.tga");

	theCameraEffects->Half_HP = MeshBuilder::GetInstance()->GenerateQuad("Half_HP", Color(1, 1, 1), 1.f);
	theCameraEffects->Half_HP->textureID = LoadTGA("Image//half_hp.tga");

	theCameraEffects->Two_third_HP = MeshBuilder::GetInstance()->GenerateQuad("Two_third_HP", Color(1, 1, 1), 1.f);
	theCameraEffects->Two_third_HP->textureID = LoadTGA("Image//two_third_hp.tga");

	theCameraEffects->Scope = MeshBuilder::GetInstance()->GenerateQuad("Scope", Color(1, 1, 1), 1.f);
	theCameraEffects->Scope->textureID = LoadTGA("Image//scope.tga");


	theCameraEffects->ammo = MeshBuilder::GetInstance()->GenerateQuad("ammo", Color(1, 1, 1), 1.f);
	theCameraEffects->ammo->textureID = LoadTGA("Image//ammo.tga");


	theCameraEffects->ammo_empty = MeshBuilder::GetInstance()->GenerateQuad("ammo_empty", Color(1, 1, 1), 1.f);
	theCameraEffects->ammo_empty->textureID = LoadTGA("Image//ammo_empty.tga");

	//theCameraEffects = Create::CameraEffects(false);
	//theCameraEffects->SetBloodScreen(MeshBuilder::GetInstance()->GenerateQuad("CAMERAEFFECTS_BLOODSCREEN", 
	//									Color(1, 1, 1), 1.f));
	//theCameraEffects->GetBloodScreen()->textureID = LoadTGA("Image//CameraEffects_Blood.tga");
	//theCameraEffects->SetStatus_BloodScreen(false);
	


	// Hardware Abstraction
	theKeyboard = new CKeyboard();
	theKeyboard->Create(playerInfo);

	theMouse = new CMouse();
	theMouse->Create(playerInfo);
}

void SceneText::Update(double dt)
{
	// Update our entities
	EntityManager::GetInstance()->Update(dt);

	// THIS WHOLE CHUNK TILL <THERE> CAN REMOVE INTO ENTITIES LOGIC! Or maybe into a scene function to keep the update clean
	if (KeyboardController::GetInstance()->IsKeyDown('1'))
		glEnable(GL_CULL_FACE);
	if (KeyboardController::GetInstance()->IsKeyDown('2'))
		glDisable(GL_CULL_FACE);
	if (KeyboardController::GetInstance()->IsKeyDown('3'))
		glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
	if (KeyboardController::GetInstance()->IsKeyDown('4'))
		glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);

	if (KeyboardController::GetInstance()->IsKeyDown('5'))
	{
		lights[0]->type = Light::LIGHT_POINT;
	}
	else if (KeyboardController::GetInstance()->IsKeyDown('6'))
	{
		lights[0]->type = Light::LIGHT_DIRECTIONAL;
	}
	else if (KeyboardController::GetInstance()->IsKeyDown('7'))
	{
		lights[0]->type = Light::LIGHT_SPOT;
	}

	if (KeyboardController::GetInstance()->IsKeyDown('I'))
		lights[0]->position.z -= (float)(10.f * dt);
	if (KeyboardController::GetInstance()->IsKeyDown('K'))
		lights[0]->position.z += (float)(10.f * dt);
	if (KeyboardController::GetInstance()->IsKeyDown('J'))
		lights[0]->position.x -= (float)(10.f * dt);
	if (KeyboardController::GetInstance()->IsKeyDown('L'))
		lights[0]->position.x += (float)(10.f * dt);
	if (KeyboardController::GetInstance()->IsKeyDown('O'))
		lights[0]->position.y -= (float)(10.f * dt);
	if (KeyboardController::GetInstance()->IsKeyDown('P'))
		lights[0]->position.y += (float)(10.f * dt);

	// if the left mouse button was released
	if (MouseController::GetInstance()->IsButtonReleased(MouseController::LMB))
	{
		cout << "Left Mouse Button was released!" << endl;
	}
	if (MouseController::GetInstance()->IsButtonReleased(MouseController::RMB))
	{
		cout << "Right Mouse Button was released!" << endl;
	}
	if (MouseController::GetInstance()->IsButtonReleased(MouseController::MMB))
	{
		cout << "Middle Mouse Button was released!" << endl;
	}
	if (MouseController::GetInstance()->GetMouseScrollStatus(MouseController::SCROLL_TYPE_XOFFSET) != 0.0)
	{
		cout << "Mouse Wheel has offset in X-axis of " << MouseController::GetInstance()->GetMouseScrollStatus(MouseController::SCROLL_TYPE_XOFFSET) << endl;
	}
	if (MouseController::GetInstance()->GetMouseScrollStatus(MouseController::SCROLL_TYPE_YOFFSET) != 0.0)
	{
		cout << "Mouse Wheel has offset in Y-axis of " << MouseController::GetInstance()->GetMouseScrollStatus(MouseController::SCROLL_TYPE_YOFFSET) << endl;
	}
	// <THERE>

	// Hardware Abstraction
	theKeyboard->Read(dt);
	theMouse->Read(dt);

	// Update the player position and other details based on keyboard and mouse inputs
	playerInfo->Update(dt);

	//camera.Update(dt); // Can put the camera into an entity rather than here (Then we don't have to write this)

	// Update NPC
	//enemyInfo->Update(dt);

	GraphicsManager::GetInstance()->UpdateLights(dt);


	// Update the 2 text object values. NOTE: Can do this in their own class but i'm lazy to do it now :P
	// Eg. FPSRenderEntity or inside RenderUI for LightEntity
	/*std::ostringstream ss;
	ss.precision(5);
	float fps = (float)(1.f / dt);
	ss << "FPS: " << fps;
	textObj[1]->SetText(ss.str());

	std::ostringstream ss1;
	ss1.precision(4);
	ss1 << "Sway:" << playerInfo->m_fCameraSwayAngle;
	textObj[2]->SetText(ss1.str());*/

	std::ostringstream ss2;
	ss2.precision(5);
	ss2 << " " << playerInfo->weaponManager[playerInfo->m_iCurrentWeapon]->magRounds << "/"<< playerInfo->weaponManager[playerInfo->m_iCurrentWeapon]->totalRounds;
	textObj[1]->SetText(ss2.str());


	/*cout << "CWeaponInfo::PrintSelf()" << endl;
	cout << "========================" << endl;
	cout << "magRounds\t\t:\t" << magRounds << endl;
	cout << "maxMagRounds\t\t:\t" << maxMagRounds << endl;
	cout << "totalRounds\t\t:\t" << totalRounds << endl;
	cout << "maxTotalRounds\t\t:\t" << maxTotalRounds << endl;
	cout << "timeBetweenShots\t:\t" << timeBetweenShots << endl;
	cout << "elapsedTime\t\t:\t" << elapsedTime << endl;
	cout << "bFire\t\t:\t" << bFire << endl;*/

	// Update camera effects
	theCameraEffects->Update(dt);
}

void SceneText::RenderMeshOnScreen(Mesh* mesh, float x, float y, float sizeX, float sizeY)
{
	glDisable(GL_DEPTH_TEST);
	Mtx44 ortho;
	ortho.SetToOrtho(0, 80, 0, 60, -10, 10); //size of screen UI
	projectionStack.PushMatrix();
	projectionStack.LoadMatrix(ortho);
	viewStack.PushMatrix();
	viewStack.LoadIdentity(); //No need camera for ortho mode

	modelStack.PushMatrix();
	modelStack.LoadIdentity();
	modelStack.Translate((float)x, (float)y, 0);
	modelStack.Scale((float)sizeX, (float)sizeY, 1);
	//to do: scale and translate accordingly
	//RenderMesh(mesh, false); //UI should not have light
	projectionStack.PopMatrix();
	viewStack.PopMatrix();
	modelStack.PopMatrix();
	glEnable(GL_DEPTH_TEST);

}

void SceneText::Render()
{
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

	GraphicsManager::GetInstance()->UpdateLightUniforms();

	// Setup 3D pipeline then render 3D
	GraphicsManager::GetInstance()->SetPerspectiveProjection(45.0f, 4.0f / 3.0f, 0.1f, 10000.0f);
	if (MouseController::GetInstance()->IsButtonDown(MouseController::RMB) && !CPlayerInfo::GetInstance()->m_iCurrentWeapon == 1)
	{
		GraphicsManager::GetInstance()->SetPerspectiveProjection(15, 4.0f / 3.0f, 0.1f, 10000.0f);
	}
	GraphicsManager::GetInstance()->AttachCamera(&camera);
	EntityManager::GetInstance()->Render();

	
	// Enable blend mode
	glEnable(GL_BLEND);

		// Setup 2D pipeline then render 2D
		int halfWindowWidth = Application::GetInstance().GetWindowWidth() / 2;
		int halfWindowHeight = Application::GetInstance().GetWindowHeight() / 2;
		GraphicsManager::GetInstance()->SetOrthographicProjection(-halfWindowWidth, halfWindowWidth, -halfWindowHeight, halfWindowHeight, -10, 10);
		GraphicsManager::GetInstance()->DetachCamera();
		EntityManager::GetInstance()->RenderUI();


		if (KeyboardController::GetInstance()->IsKeyDown('9'))
			theCameraEffects->SetStatus_BloodScreen(true);
		// Render Camera Effects
		theCameraEffects->RenderUI();

		// Render Minimap
		theMinimap->RenderUI();

		RenderMeshOnScreen(meshList[SceneText::GEO_BACKGROUND], 0, 0, 200, 200);

	// Disable blend mode
	glDisable(GL_BLEND);
}

void SceneText::Exit()
{
	// Detach camera from other entities
	GraphicsManager::GetInstance()->DetachCamera();
	playerInfo->DetachCamera();

	if (playerInfo->DropInstance() == false)
	{
#if _DEBUGMODE==1
		cout << "Unable to drop PlayerInfo class" << endl;
#endif
	}

	// Delete the lights
	delete lights[0];
	delete lights[1];
}
