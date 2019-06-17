TARGET_LIB = libVitaEngine.a
OBJS       = Engine/PSVitaApplication.o Engine/Program/Random.o Engine/Program/Log.o Engine/Program/Input.o Engine/Program/Serializer.o \
				Engine/Graphics/Camera/Frustum.o Engine/Graphics/Camera/Camera.o Engine/Game/EntityManager.o Engine/Game/ComponentManagers/TransformManager.o  \
				Engine/Game/Script.o Engine/Graphics/Camera/FPSCamera.o Engine/Sound/SoundSource.o Engine/Physics/Ray.o Engine/Physics/RigidBody.o \
				Engine/Physics/Ray.o Engine/Physics/Collider.o Engine/Physics/Ray.o Engine/Physics/Trigger.o Engine/Graphics/ResourcesLoader.o \
				Engine/Program/Utils.o Engine/AI/AIObject.o Engine/AI/AISystem.o Engine/AI/AStarGrid.o Engine/AI/AStarNodeHeap.o \
				Engine/Game/ComponentManagers/LightManager.o Engine/Game/ComponentManagers/ModelManager.o Engine/Game/ComponentManagers/ParticleManager.o \
				Engine/Game/ComponentManagers/PhysicsManager.o Engine/Game/ComponentManagers/ScriptManager.o Engine/Game/ComponentManagers/SoundManager.o \
				Engine/Game/UI/Button.o Engine/Game/UI/EditText.o Engine/Game/UI/Image.o Engine/Game/UI/StaticText.o Engine/Game/UI/UIManager.o \
				Engine/Game/UI/Widget.o Engine/Game/Game.o Engine/Graphics/Animation/AnimatedModel.o Engine/Graphics/Animation/AnimationController.o \
				Engine/Graphics/Effects/CascadedShadowMap.o Engine/Graphics/Effects/DebugDrawManager.o Engine/Graphics/Effects/ForwardRenderer.o \
				Engine/Graphics/Effects/ProjectedGridWater.o Engine/Graphics/Effects/TimeOfDayManager.o Engine/Graphics/Effects/RenderingPath.o Engine/Graphics/Effects/VCTGI.o \
				Engine/Graphics/Effects/VolumetricClouds.o Engine/Graphics/Terrain/Terrain.o Engine/Graphics/Terrain/TerrainNode.o Engine/Graphics/Font.o \
				Engine/Graphics/FrameGraph.o Engine/Graphics/Model.o Engine/Graphics/Material.o Engine/Graphics/MeshDefaults.o Engine/Graphics/ParticleSystem.o Engine/Graphics/Shader.o \
				Engine/Graphics/Texture.o Engine/Graphics/VertexArray.o Engine/Graphics/Renderer.o Engine/Graphics/GXM/GXMRenderer.o Engine/Graphics/GXM/GXMFramebuffer.o \
				Engine/Graphics/GXM/GXMUtils.o Engine/stb.o Engine/Graphics/Effects/ForwardPlusRenderer.o Engine/Graphics/Effects/PSVitaRenderer.o Engine/Graphics/GXM/GXMVertexArray.o \
				Engine/Graphics/GXM/GXMVertexBuffer.o Engine/Graphics/GXM/GXMIndexBuffer.o Engine/Program/FileManager.o Engine/Graphics/GXM/GXMShader.o Engine/Graphics/GXM/GXMTexture2D.o \
				Engine/Graphics/GXM/GXMUniformBuffer.o
				

INCLUDES		= -I$(CURDIR) -IEngine -Iinclude/bullet

#LIBS = -lSceGxm_stub lSceDisplay_stub lSceCtrl_stub

PREFIX  ?= ${VITASDK}/arm-vita-eabi
CXX      = arm-vita-eabi-g++
AR      = arm-vita-eabi-ar
CFLAGS  = -Wl,-q -Wall -O3
CXXFLAGS = -Wl,-q -Wall -O3 -std=c++11 $(INCLUDES) -DGLM_ENABLE_EXPERIMENTAL -DGLM_FORCE_RADIANS -DGLM_FORCE_DEPTH_ZERO_TO_ONE -DVITA
ASFLAGS = $(CXXFLAGS)

all: $(TARGET_LIB)

#debug: CXXFLAGS += -DDEBUG_BUILD
#debug: all

$(TARGET_LIB): $(SHADERS) $(OBJS)
	$(AR) -rc $@ $^

clean:
	rm -rf $(TARGET_LIB) $(OBJS)

