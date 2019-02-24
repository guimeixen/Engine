#pragma once

#include "Commands.h"
#include "Graphics/Lights.h"
#include "Graphics/Model.h"
#include "Game/ComponentManagers/TransformManager.h"
#include "Game/ComponentManagers/ModelManager.h"
#include "Game/ComponentManagers/ParticleManager.h"
#include "Game/ComponentManagers/SoundManager.h"
#include "Game/ComponentManagers/PhysicsManager.h"
#include "Game/UI/UIManager.h"

#include "include/glm/glm.hpp"

#include <stack>

class EditorManager;

class ObjectWindow
{
public:
	ObjectWindow();
	~ObjectWindow();

	void Init(Engine::Game *game, EditorManager *editorManager);
	void Render();

	void SetEntity(Engine::Entity entity);
	void DeselectEntity() { selected = false; selectedEntity.id = std::numeric_limits<unsigned int>::max(); }
	Engine::Entity GetSelectedEntity() const { return selectedEntity; }

	void Show(bool show) { showWindow = show; }
	bool IsVisible() const { return showWindow; }

	bool IsSelectingAsset() const { return isSelectingAsset; }

private:
	void HandleAI();

	void HandleTransform();
	void HandleModel();
	void HandleRigidBody();
	void HandleParticleSystem();
	void HandleTrigger();
	void HandleScript();
	void HandleCollider();
	void HandleLight();
	void HandleSoundSource();
	void HandleWidget();
	void HandleButton();
	void HandleText();
	void HandleTextures();
	void HandleImage();
	void HandleEditText();

	bool AddModel(bool animated);
	void AddRigidBody(Engine::ShapeType type);
	void AddParticleSystem();
	void AddTrigger(Engine::ShapeType type);
	void AddCollider(Engine::ShapeType type);
	bool AddScript();
	void AddLight(Engine::LightType type);
	void AddSoundSource();
	void AddWidget(Engine::WidgetType type);

	void AddTexture();

	void CreatePrefabFolder();

private:
	Engine::Game *game;
	EditorManager *editorManager;

	Engine::TransformManager*		transformManager;
	Engine::ModelManager*			modelManager;
	Engine::ParticleManager*		particleManager;
	Engine::PhysicsManager*			physicsManager;

	Engine::Entity selectedEntity;
	Engine::Model *selectedModel;
	Engine::ParticleSystem *selectedPS;
	Engine::RigidBody* selectedRB;
	Engine::Trigger* selectedTrigger;
	Engine::Collider* selectedCollider;
	Engine::Script* selectedScript;
	Engine::Light* selectedLight;
	Engine::SoundSource *selectedSoundSource;
	Engine::Widget *selectedWidget;

	bool showWindow = true;
	bool selected = false;

	std::vector<std::string> files;
	int nodeClicked = -1;

	bool isSelectingAsset;

	bool addModelSelected;
	bool addAnimModelSelected;
	bool addScriptSelected;

	bool modelOpen;
	bool rigidBodyOpen;
	bool particleSystemOpen;
	bool triggerOpen;
	bool scriptOpen;
	bool colliderOpen;
	bool lightOpen;
	bool soundSourceOpen;
	bool widgetOpen;

	int layerComboID = 0;

	// AI
	std::string targetName;
	glm::vec3 eyesOffset;
	float eyesRange;
	float attackRange;
	float attackDelay = 0.0f;
	float fov = 0.0f;

	// Transform
	glm::vec3 position;
	glm::vec3 rotation;
	glm::vec3 scale;

	// Model
	bool castShadows;
	float lodDistance;

	// Physics
	float friction = 0.0f;

	// Rigid body
	float mass = 1.0f;
	float restitution = 0.0f;
	glm::vec3 rigidBodyCenter;
	glm::vec3 angularFactor;
	float angSleepingThresh = 0.0f;
	float linSleepingThresh = 0.0f;
	bool visualizeRigidBody = true;
	bool kinematic = false;
	int rigidBodyShapeID = 0;
	glm::vec3 rigidBodySize;
	glm::vec3 linearFactor;
	float rigidBodySphereRadius = 0.0f;
	float rigidBodyCapsuleRadius = 0.0f;
	float rigidBodyCapsuleHeight = 0.0f;
	float linearDamping = 0.0f;
	float angularDamping = 0.0f;

	// Particle System
	std::string matNameString;
	float psLifetime = 1.0f;
	float size = 1.0f;
	int emission = 4;
	int maxParticles = 30;
	bool loopPs = false;
	float duration = 0.0f;
	glm::vec3 velocity;
	bool useAtlas = false;
	int atlasColumns = 0;
	int atlasRows = 0;
	float atlasCycles = 0.0f;
	int emissionShapeID = 0;
	glm::vec3 emissionBoxSize;
	float emissionRadius = 0.0f;
	glm::vec3 psCenter;
	bool randomVelocity = false;
	float velLow = 0.0f;
	float velHigh = 1.0f;
	bool separateAxis = false;
	glm::vec3 velLowSeparate;
	glm::vec3 velHighSeparate;
	glm::vec4 psColor;
	bool fadeAlpha = true;
	bool limitVelocity = false;
	float speedLimit = 0.0f;
	float dampen = 0.0f;
	float gravityModifier = 1.0f;

	//float zRotation = 0.0f;

	// Trigger
	int shapeComboId			 = 0;
	glm::vec3 triggerCenter;
	glm::vec3 triggerBoxSize;
	float triggerRadius			= 0.0f;
	float triggerCapsuleRadius	= 0.0f;
	float triggerCapsuleHeight	= 0.0f;
	bool visualize				= true;

	// Collider
	int colliderShapeID			= 0;
	glm::vec3 colliderCenter;
	glm::vec3 colliderBoxSize;
	float colliderRadius		= 0.0f;
	float colliderCapsuleRadius = 0.0f;
	float colliderCapsuleHeight = 0.0f;

	// Script
	int selectedScriptProperty	= 0;
	size_t propertyIndex		= 0;

	// Sound source
	int soundComboId			= 0;
	float volume				= 0.0f;
	float pitch					= 0.0f;
	bool is3D					= false;
	bool loopSound				= false;
	bool playOnStart			= false;
	float min3DDistance			= 1.0f;
	float max3DDistance			= 10000.0f;

	char buf[256];

	// Widget
	bool isEnabled = true;
	bool textScaleChanged = false;
	glm::vec2 widgetSize;
	float depth;
	glm::vec2 positionPercent;
	glm::vec4 imageColor;
	char text[256];
	glm::vec2 textScale;
	glm::vec2 textPosOffset;
	glm::vec4 textColor;
	int choosenTextureType = 0;
	bool addTextureSelected = false;
};

