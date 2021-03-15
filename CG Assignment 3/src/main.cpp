//Just a simple handler for simple initialization stuffs
#include "Utilities/BackendHandler.h"

#include <filesystem>
#include <json.hpp>
#include <fstream>

//TODO: New for this tutorial
#include <DirectionalLight.h>
#include <PointLight.h>
#include <UniformBuffer.h>
/////////////////////////////

#include <Texture2D.h>
#include <Texture2DData.h>
#include <MeshBuilder.h>
#include <MeshFactory.h>
#include <NotObjLoader.h>
#include <ObjLoader.h>
#include <VertexTypes.h>
#include <ShaderMaterial.h>
#include <RendererComponent.h>
#include <TextureCubeMap.h>
#include <TextureCubeMapData.h>

#include <Timing.h>
#include <GameObjectTag.h>
#include <InputHelpers.h>

#include <IBehaviour.h>
#include <CameraControlBehaviour.h>
#include <FollowPathBehaviour.h>
#include <SimpleMoveBehaviour.h>


void PlayerInput(GameObject& transform, float dt, float speed) {
	if (glfwGetKey(BackendHandler::window, GLFW_KEY_A) == GLFW_PRESS) {
		transform.get<Transform>().SetLocalRotation(90.0f, 0.0f, 90.0f);
		transform.get<Transform>().SetLocalPosition(transform.get<Transform>().GetLocalPosition() + glm::vec3(-1.5, 0, 0) * dt * speed);
	}
	if (glfwGetKey(BackendHandler::window, GLFW_KEY_D) == GLFW_PRESS) {
		transform.get<Transform>().SetLocalRotation(90.0f, 0.0f, 270.0f);
		transform.get<Transform>().SetLocalPosition(transform.get<Transform>().GetLocalPosition() + glm::vec3(1.5, 0, 0) * dt * speed);
	}
	if (glfwGetKey(BackendHandler::window, GLFW_KEY_W) == GLFW_PRESS) {
		transform.get<Transform>().SetLocalRotation(90.0f, 0.0f, 0.0f);
		transform.get<Transform>().SetLocalPosition(transform.get<Transform>().GetLocalPosition() + glm::vec3(0, 1.5, 0) * dt * speed);
	}
	if (glfwGetKey(BackendHandler::window, GLFW_KEY_S) == GLFW_PRESS) {
		transform.get<Transform>().SetLocalRotation(90.0f, 0.0f, 180.0f);
		transform.get<Transform>().SetLocalPosition(transform.get<Transform>().GetLocalPosition() + glm::vec3(0, -1.5, 0) * dt * speed);

	}
}

//Lerp Function
template<typename T>
T LERP(const T& p0, const T& p1, float t)
{
	return (1.0f - t) * p0 + t * p1;
}


int main() {
	int frameIx = 0;
	float fpsBuffer[128];
	float minFps, maxFps, avgFps;
	int selectedVao = 0; // select cube by default
	std::vector<GameObject> controllables;

	bool drawGBuffer = false;
	bool drawIllumBuffer = false;
	bool drawColorBuffer = false;
	bool drawDepthBuffer = false;
	bool drawNormalsBuffer = false;
	bool drawCompositedScene = false;

	BackendHandler::InitAll();

	// Let OpenGL know that we want debug output, and route it to our handler function
	glEnable(GL_DEBUG_OUTPUT);
	glDebugMessageCallback(BackendHandler::GlDebugMessage, nullptr);

	// Enable texturing
	glEnable(GL_TEXTURE_2D);

	// Push another scope so most memory should be freed *before* we exit the app
	{
		#pragma region Shader and ImGui
		Shader::sptr passthroughShader = Shader::Create();
		passthroughShader->LoadShaderPartFromFile("shaders/passthrough_vert.glsl", GL_VERTEX_SHADER);
		passthroughShader->LoadShaderPartFromFile("shaders/passthrough_frag.glsl", GL_FRAGMENT_SHADER);
		passthroughShader->Link();

		Shader::sptr simpleDepthShader = Shader::Create();
		simpleDepthShader->LoadShaderPartFromFile("shaders/simple_depth_vert.glsl", GL_VERTEX_SHADER);
		simpleDepthShader->LoadShaderPartFromFile("shaders/simple_depth_frag.glsl", GL_FRAGMENT_SHADER);
		simpleDepthShader->Link();

		//Init gBUffer shader
		Shader::sptr gBufferShader = Shader::Create();
		gBufferShader->LoadShaderPartFromFile("shaders/vertex_shader.glsl", GL_VERTEX_SHADER);
		gBufferShader->LoadShaderPartFromFile("shaders/gBuffer_pass_frag.glsl", GL_FRAGMENT_SHADER);
		gBufferShader->Link();

		// Load our shaders
		Shader::sptr shader = Shader::Create();
		shader->LoadShaderPartFromFile("shaders/vertex_shader.glsl", GL_VERTEX_SHADER);
		//Directional Light Shader
		shader->LoadShaderPartFromFile("shaders/directional_blinn_phong_frag.glsl", GL_FRAGMENT_SHADER);
		shader->Link();

		//Basic effect for drawing to
		PostEffect* basicEffect;
		Framebuffer* shadowBuffer;
		GBuffer* gBuffer;
		IlluminationBuffer* illuminationBuffer;
		
		//Post Processing Effects
		int activeEffect = 0;
		std::vector<PostEffect*> effects;
		SepiaEffect* sepiaEffect;
		GreyscaleEffect* greyscaleEffect;
		ColorCorrectEffect* colorCorrectEffect;
		FilmGrainEffect* filmGrainEffect;
		BloomEffect* bloomEffect;
		PixelateEffect* pixelateEffect;

		bool test = false;
		
		// We'll add some ImGui controls to control our shader
		BackendHandler::imGuiCallbacks.push_back([&]() {
			if (ImGui::CollapsingHeader("Effect controls"))
			{
				ImGui::SliderInt("Chosen Effect", &activeEffect, 0, effects.size() - 1);

				if (activeEffect == 0)
				{
					ImGui::Text("Active Effect: Sepia Effect");

					SepiaEffect* temp = (SepiaEffect*)effects[activeEffect];

					float intensity = temp->GetIntensity();

					if (ImGui::SliderFloat("Intensity", &intensity, 0.0f, 1.0f))
					{
						temp->SetIntensity(intensity);
					}
				}
				if (activeEffect == 1)
				{
					ImGui::Text("Active Effect: Bloom Effect");

					BloomEffect* temp = (BloomEffect*)effects[activeEffect];
					float passes = temp->GetPasses();
					float threshold = temp->GetThreshold();

					if (ImGui::SliderFloat("Blur Value", &passes, 0.0f, 40.0f)) {
						temp->SetPasses(passes);
					}
					if (ImGui::SliderFloat("Brightness Threshold", &threshold, 0.0f, 2.0f)) {
						temp->SetThreshold(threshold);
					}
				}
				if (activeEffect == 2)
				{
					ImGui::Text("Active Effect: Pixelate Effect");

					PixelateEffect* temp = (PixelateEffect*)effects[activeEffect];

					float intensity = temp->GetIntensity();

					if (ImGui::SliderFloat("Intensity (Lower Means Less Pixels)", &intensity, 400.0f, 3000.0f))
					{
						temp->SetIntensity(intensity);
					}
				}
				if (activeEffect == 3)
				{
					ImGui::Text("Active Effect: Film Grain Effect");

					FilmGrainEffect* temp = (FilmGrainEffect*)effects[activeEffect];

					float intensity = temp->GetIntensity();

					if (ImGui::SliderFloat("Intensity", &intensity, 0.1f, 1.0f))
					{
						temp->SetIntensity(intensity);
					}
				}
			}
			if (ImGui::CollapsingHeader("Environment generation"))
			{
				if (ImGui::Button("Regenerate Environment", ImVec2(200.0f, 40.0f)))
				{
					EnvironmentGenerator::RegenerateEnvironment();
				}
			}
			if (ImGui::CollapsingHeader("Light Level Lighting Settings"))
			{
				if (ImGui::DragFloat3("Light Direction/Position", glm::value_ptr(illuminationBuffer->GetSunRef()._lightDirection), 0.01f, -10.0f, 10.0f)) 
				{
				}
			}
			if (ImGui::Checkbox("Draw Composited Scene", &drawCompositedScene))
			{
				drawNormalsBuffer = false;
				drawDepthBuffer = false;
				drawIllumBuffer = false;
				drawColorBuffer = false;
				drawGBuffer = false;
			}
			if (ImGui::Checkbox("Draw Depth Buffer", &drawDepthBuffer))
			{
				drawNormalsBuffer = false;
				drawColorBuffer = false;
				drawIllumBuffer = false;
				drawGBuffer = false;
			}
			if (ImGui::Checkbox("Draw Normal Buffer", &drawNormalsBuffer))
			{
				drawDepthBuffer = false;
				drawColorBuffer = false;
				drawIllumBuffer = false;
				drawGBuffer = false;
			}
			if (ImGui::Checkbox("Draw Color Buffer", &drawColorBuffer))
			{
				drawNormalsBuffer = false;
				drawDepthBuffer = false;
				drawIllumBuffer = false;
				drawGBuffer = false;
			}
			if (ImGui::Checkbox("Show Light Accumulation Buffer", &drawIllumBuffer))
			{
				drawNormalsBuffer = false;
				drawDepthBuffer = false;
				drawColorBuffer = false;
				drawGBuffer = false;
			}
			if (ImGui::Checkbox("Draw All Buffers", &drawGBuffer))
			{
				drawNormalsBuffer = false;
				drawDepthBuffer = false;
				drawIllumBuffer = false;
				drawColorBuffer = false;
			}

			auto name = controllables[selectedVao].get<GameObjectTag>().Name;
			ImGui::Text(name.c_str());
			auto behaviour = BehaviourBinding::Get<SimpleMoveBehaviour>(controllables[selectedVao]);
			ImGui::Checkbox("Relative Rotation", &behaviour->Relative);

			ImGui::Text("Q/E -> Yaw\nLeft/Right -> Roll\nUp/Down -> Pitch\nY -> Toggle Mode");
		
			minFps = FLT_MAX;
			maxFps = 0;
			avgFps = 0;
			for (int ix = 0; ix < 128; ix++) {
				if (fpsBuffer[ix] < minFps) { minFps = fpsBuffer[ix]; }
				if (fpsBuffer[ix] > maxFps) { maxFps = fpsBuffer[ix]; }
				avgFps += fpsBuffer[ix];
			}
			ImGui::PlotLines("FPS", fpsBuffer, 128);
			ImGui::Text("MIN: %f MAX: %f AVG: %f", minFps, maxFps, avgFps / 128.0f);
			});

		#pragma endregion 

		// GL states
		glEnable(GL_DEPTH_TEST);
		glEnable(GL_CULL_FACE);
		glDepthFunc(GL_LEQUAL); // New 

		///////////////////////////////////// Texture Loading //////////////////////////////////////////////////
		#pragma region Texture

		// Load some textures from files
		Texture2D::sptr stone = Texture2D::LoadFromFile("images/Stone_001_Diffuse.png");
		Texture2D::sptr stoneSpec = Texture2D::LoadFromFile("images/Stone_001_Specular.png");
		Texture2D::sptr grass = Texture2D::LoadFromFile("images/grass.jpg");
		Texture2D::sptr noSpec = Texture2D::LoadFromFile("images/grassSpec.png");
		Texture2D::sptr box = Texture2D::LoadFromFile("images/box.bmp");
		Texture2D::sptr boxSpec = Texture2D::LoadFromFile("images/box-reflections.bmp");
		Texture2D::sptr simpleFlora = Texture2D::LoadFromFile("images/SimpleFlora.png");
		Texture2D::sptr cloud = Texture2D::LoadFromFile("images/WhiteColour.png");
		Texture2D::sptr rocket = Texture2D::LoadFromFile("images/SpaceShipColours.png");
		Texture2D::sptr PineTree = Texture2D::LoadFromFile("images/PineTreeColours.png");
		Texture2D::sptr tree4 = Texture2D::LoadFromFile("images/tree4_texture.png");
		Texture2D::sptr Player = Texture2D::LoadFromFile("images/NewCharacterTexture.png");
		LUT3D testCube("cubes/BrightenedCorrection.cube");

		// Load the cube map
		//TextureCubeMap::sptr environmentMap = TextureCubeMap::LoadFromImages("images/cubemaps/skybox/sample.jpg");
		TextureCubeMap::sptr environmentMap = TextureCubeMap::LoadFromImages("images/cubemaps/skybox/ToonSky.jpg"); 

		// Creating an empty texture
		Texture2DDescription desc = Texture2DDescription();  
		desc.Width = 1;
		desc.Height = 1;
		desc.Format = InternalFormat::RGB8;
		Texture2D::sptr texture2 = Texture2D::Create(desc);
		// Clear it with a white colour
		texture2->Clear();

		#pragma endregion
		//////////////////////////////////////////////////////////////////////////////////////////

		///////////////////////////////////// Scene Generation //////////////////////////////////////////////////
		#pragma region Scene Generation
		
		// We need to tell our scene system what extra component types we want to support
		GameScene::RegisterComponentType<RendererComponent>();
		GameScene::RegisterComponentType<BehaviourBinding>();
		GameScene::RegisterComponentType<Camera>();

		// Create a scene, and set it to be the active scene in the application
		GameScene::sptr scene = GameScene::Create("test");
		Application::Instance().ActiveScene = scene;

		// We can create a group ahead of time to make iterating on the group faster
		entt::basic_group<entt::entity, entt::exclude_t<>, entt::get_t<Transform>, RendererComponent> renderGroup =
			scene->Registry().group<RendererComponent>(entt::get_t<Transform>());

		// Create a material and set some properties for it
		ShaderMaterial::sptr stoneMat = ShaderMaterial::Create();
		stoneMat->Shader = gBufferShader;
		stoneMat->Set("s_Diffuse", stone);
		stoneMat->Set("s_Specular", stoneSpec);
		stoneMat->Set("u_Shininess", 2.0f);
		stoneMat->Set("u_TextureMix", 0.0f);

		ShaderMaterial::sptr grassMat = ShaderMaterial::Create();
		grassMat->Shader = gBufferShader;
		grassMat->Set("s_Diffuse", grass);
		grassMat->Set("s_Specular", noSpec);
		grassMat->Set("u_Shininess", 2.0f);
		grassMat->Set("u_TextureMix", 0.0f);

		ShaderMaterial::sptr boxMat = ShaderMaterial::Create();
		boxMat->Shader = gBufferShader;
		boxMat->Set("s_Diffuse", box);
		boxMat->Set("s_Specular", boxSpec);
		boxMat->Set("u_Shininess", 8.0f);
		boxMat->Set("u_TextureMix", 0.0f);

		ShaderMaterial::sptr simpleFloraMat = ShaderMaterial::Create();
		simpleFloraMat->Shader = gBufferShader;
		simpleFloraMat->Set("s_Diffuse", simpleFlora);
		simpleFloraMat->Set("s_Specular", noSpec);
		simpleFloraMat->Set("u_Shininess", 8.0f);
		simpleFloraMat->Set("u_TextureMix", 0.0f);


		ShaderMaterial::sptr cloudMat = ShaderMaterial::Create();
		cloudMat->Shader = gBufferShader;
		cloudMat->Set("s_Diffuse", cloud);
		cloudMat->Set("s_Specular", noSpec);
		cloudMat->Set("u_Shininess", 8.0f);
		cloudMat->Set("u_TextureMix", 0.0f);

		ShaderMaterial::sptr rocketMat = ShaderMaterial::Create();
		rocketMat->Shader = gBufferShader;
		rocketMat->Set("s_Diffuse", rocket);
		rocketMat->Set("s_Specular", noSpec);
		rocketMat->Set("u_Shininess", 8.0f);
		rocketMat->Set("u_TextureMix", 0.0f);

		ShaderMaterial::sptr PineTreeMat = ShaderMaterial::Create();
		PineTreeMat->Shader = gBufferShader;
		PineTreeMat->Set("s_Diffuse", PineTree);
		PineTreeMat->Set("s_Specular", noSpec);
		PineTreeMat->Set("u_Shininess", 8.0f);
		PineTreeMat->Set("u_TextureMix", 0.0f);

		ShaderMaterial::sptr Tree4Mat = ShaderMaterial::Create();
		Tree4Mat->Shader = gBufferShader;
		Tree4Mat->Set("s_Diffuse", tree4);
		Tree4Mat->Set("s_Specular", noSpec);
		Tree4Mat->Set("u_Shininess", 8.0f);
		Tree4Mat->Set("u_TextureMix", 0.0f);

		ShaderMaterial::sptr PlayerMat = ShaderMaterial::Create();
		PlayerMat->Shader = gBufferShader;
		PlayerMat->Set("s_Diffuse", Player);
		PlayerMat->Set("s_Specular", stoneSpec);
		PlayerMat->Set("u_Shininess", 8.0f);
		PlayerMat->Set("u_TextureMix", 0.0f);

		GameObject player = scene->CreateEntity("Player");
		{
			VertexArrayObject::sptr vao = ObjLoader::LoadFromFile("models/Player/Idle/SkyBoundCharacter00.obj");
			player.emplace<RendererComponent>().SetMesh(vao).SetMaterial(PlayerMat);
			player.get<Transform>().SetLocalRotation(90.0f, 0.0f, 0.0f);
			player.get<Transform>().SetLocalPosition(0.0f, 4.0f, 3.4f);
			player.get<Transform>().SetLocalScale(0.3f, 0.3f, 0.3f);
		}


		GameObject obj1 = scene->CreateEntity("Ground");
		{
			VertexArrayObject::sptr vao = ObjLoader::LoadFromFile("models/SphereicalIsland.obj");
			obj1.emplace<RendererComponent>().SetMesh(vao).SetMaterial(grassMat);
			obj1.get<Transform>().SetLocalRotation(90.0f, 0.0f, 0.0f);
			obj1.get<Transform>().SetLocalScale(1.0f, 1.0f, 1.0f);
		}

		GameObject obj3 = scene->CreateEntity("Clouds");
		{
			VertexArrayObject::sptr vao = ObjLoader::LoadFromFile("models/SphereicalClouds.obj");
			obj3.emplace<RendererComponent>().SetMesh(vao).SetMaterial(cloudMat);
			obj3.get<Transform>().SetLocalRotation(90.0f, 0.0f, 0.0f);
			obj3.get<Transform>().SetLocalScale(1.0f, 1.0f, 1.0f);
			obj3.get<Transform>().SetLocalPosition(0.0f, 0.0f, 0.0f);

		}

		GameObject obj2 = scene->CreateEntity("rocket");
		{
			VertexArrayObject::sptr vao = ObjLoader::LoadFromFile("models/RocketShip.obj");
			obj2.emplace<RendererComponent>().SetMesh(vao).SetMaterial(rocketMat);
			obj2.get<Transform>().SetLocalPosition(0.0f, 0.0f, 4.0f);
			obj2.get<Transform>().SetLocalRotation(90.0f, 0.0f, -90.0f);
			obj2.get<Transform>().SetLocalScale(0.7f, 0.7f, 0.7f);
			BehaviourBinding::BindDisabled<SimpleMoveBehaviour>(obj2);
		}


		GameObject PineTree1 = scene->CreateEntity("PineTree");
		{
			VertexArrayObject::sptr vao = ObjLoader::LoadFromFile("models/PineTree0.obj");
			PineTree1.emplace<RendererComponent>().SetMesh(vao).SetMaterial(PineTreeMat);
			PineTree1.get<Transform>().SetLocalPosition(-3.0f, 10.0f, 1.0f);
			PineTree1.get<Transform>().SetLocalRotation(90.0f, 0.0f, -90.0f);
			PineTree1.get<Transform>().SetLocalScale(0.9f, 0.9f, 0.9f);
			BehaviourBinding::BindDisabled<SimpleMoveBehaviour>(PineTree1);
		}

		GameObject PineTree2 = scene->CreateEntity("PineTree");
		{
			VertexArrayObject::sptr vao = ObjLoader::LoadFromFile("models/PineTree0.obj");
			PineTree2.emplace<RendererComponent>().SetMesh(vao).SetMaterial(PineTreeMat);
			PineTree2.get<Transform>().SetLocalPosition(9.0f, 12.0f, 1.0f);
			PineTree2.get<Transform>().SetLocalRotation(90.0f, 0.0f, -90.0f);
			PineTree2.get<Transform>().SetLocalScale(0.9f, 0.9f, 0.9f);
			BehaviourBinding::BindDisabled<SimpleMoveBehaviour>(PineTree2);
		}

		GameObject PineTree3 = scene->CreateEntity("PineTree");
		{
			VertexArrayObject::sptr vao = ObjLoader::LoadFromFile("models/PineTree0.obj");
			PineTree3.emplace<RendererComponent>().SetMesh(vao).SetMaterial(PineTreeMat);
			PineTree3.get<Transform>().SetLocalPosition(4.0f, 6.0f, 1.0f);
			PineTree3.get<Transform>().SetLocalRotation(90.0f, 0.0f, -90.0f);
			PineTree3.get<Transform>().SetLocalScale(0.9f, 0.9f, 0.9f);
			BehaviourBinding::BindDisabled<SimpleMoveBehaviour>(PineTree2);
		}

		GameObject PineTree4 = scene->CreateEntity("PineTree");
		{
			VertexArrayObject::sptr vao = ObjLoader::LoadFromFile("models/PineTree0.obj");
			PineTree4.emplace<RendererComponent>().SetMesh(vao).SetMaterial(PineTreeMat);
			PineTree4.get<Transform>().SetLocalPosition(-8.0f, 2.0f, 1.0f);
			PineTree4.get<Transform>().SetLocalRotation(90.0f, 0.0f, -90.0f);
			PineTree4.get<Transform>().SetLocalScale(0.9f, 0.9f, 0.9f);
			BehaviourBinding::BindDisabled<SimpleMoveBehaviour>(PineTree4);
		}

		GameObject tree = scene->CreateEntity("tree4");
		{
			VertexArrayObject::sptr vao = ObjLoader::LoadFromFile("models/tree4.obj");
			tree.emplace<RendererComponent>().SetMesh(vao).SetMaterial(Tree4Mat);
			tree.get<Transform>().SetLocalPosition(-3.0f, -8.0f, 1.8f);
			tree.get<Transform>().SetLocalRotation(90.0f, 0.0f, -90.0f);
			tree.get<Transform>().SetLocalScale(1.0f, 1.0f, 1.0f);
			BehaviourBinding::BindDisabled<SimpleMoveBehaviour>(tree);
		}

		GameObject tree2 = scene->CreateEntity("tree4");
		{
			VertexArrayObject::sptr vao = ObjLoader::LoadFromFile("models/tree4.obj");
			tree2.emplace<RendererComponent>().SetMesh(vao).SetMaterial(Tree4Mat);
			tree2.get<Transform>().SetLocalPosition(7.0f, -4.0f, 1.8f);
			tree2.get<Transform>().SetLocalRotation(90.0f, 0.0f, -90.0f);
			tree2.get<Transform>().SetLocalScale(1.0f, 1.0f, 1.0f);
			BehaviourBinding::BindDisabled<SimpleMoveBehaviour>(tree2);
		}

		GameObject tree3 = scene->CreateEntity("tree4");
		{
			VertexArrayObject::sptr vao = ObjLoader::LoadFromFile("models/tree4.obj");
			tree3.emplace<RendererComponent>().SetMesh(vao).SetMaterial(Tree4Mat);
			tree3.get<Transform>().SetLocalPosition(-7.0f, 9.0f, 1.5f);
			tree3.get<Transform>().SetLocalRotation(90.0f, 0.0f, -90.0f);
			tree3.get<Transform>().SetLocalScale(0.8f, 0.8f, 0.8f);
			BehaviourBinding::BindDisabled<SimpleMoveBehaviour>(tree3);
		}

		GameObject tree5 = scene->CreateEntity("tree4");
		{
			VertexArrayObject::sptr vao = ObjLoader::LoadFromFile("models/tree4.obj");
			tree5.emplace<RendererComponent>().SetMesh(vao).SetMaterial(Tree4Mat);
			tree5.get<Transform>().SetLocalPosition(-10.0f, -4.0f, 1.5f);
			tree5.get<Transform>().SetLocalRotation(90.0f, 0.0f, -90.0f);
			tree5.get<Transform>().SetLocalScale(0.8f, 0.8f, 0.8f);
			BehaviourBinding::BindDisabled<SimpleMoveBehaviour>(tree5);
		}


		/*GameObject obj1 = scene->CreateEntity("Ground"); 
		{
			VertexArrayObject::sptr vao = ObjLoader::LoadFromFile("models/plane.obj");
			obj1.emplace<RendererComponent>().SetMesh(vao).SetMaterial(grassMat);
		}

		GameObject obj2 = scene->CreateEntity("monkey_quads");
		{
			VertexArrayObject::sptr vao = ObjLoader::LoadFromFile("models/monkey_quads.obj");
			obj2.emplace<RendererComponent>().SetMesh(vao).SetMaterial(stoneMat);
			obj2.get<Transform>().SetLocalPosition(0.0f, 0.0f, 2.0f);
			obj2.get<Transform>().SetLocalRotation(0.0f, 0.0f, -90.0f);
			BehaviourBinding::BindDisabled<SimpleMoveBehaviour>(obj2);
		}*/

		std::vector<glm::vec2> allAvoidAreasFrom = { glm::vec2(-4.0f, -4.0f) };
		std::vector<glm::vec2> allAvoidAreasTo = { glm::vec2(4.0f, 4.0f) };

		std::vector<glm::vec2> rockAvoidAreasFrom = { glm::vec2(-3.0f, -3.0f), glm::vec2(-19.0f, -19.0f), glm::vec2(5.0f, -19.0f),
														glm::vec2(-19.0f, 5.0f), glm::vec2(-19.0f, -19.0f) };
		std::vector<glm::vec2> rockAvoidAreasTo = { glm::vec2(3.0f, 3.0f), glm::vec2(19.0f, -5.0f), glm::vec2(19.0f, 19.0f),
														glm::vec2(19.0f, 19.0f), glm::vec2(-5.0f, 19.0f) };
		glm::vec2 spawnFromHere = glm::vec2(-19.0f, -19.0f);
		glm::vec2 spawnToHere = glm::vec2(19.0f, 19.0f);

		/*EnvironmentGenerator::AddObjectToGeneration("models/simplePine.obj", simpleFloraMat, 40,
			spawnFromHere, spawnToHere, allAvoidAreasFrom, allAvoidAreasTo);
		EnvironmentGenerator::AddObjectToGeneration("models/simpleTree.obj", simpleFloraMat, 40,
			spawnFromHere, spawnToHere, allAvoidAreasFrom, allAvoidAreasTo);
		EnvironmentGenerator::AddObjectToGeneration("models/simpleRock.obj", simpleFloraMat, 24,
			spawnFromHere, spawnToHere, rockAvoidAreasFrom, rockAvoidAreasTo);
		EnvironmentGenerator::GenerateEnvironment();*/

		// Create an object to be our camera
		//GameObject cameraObject = scene->CreateEntity("Camera");
		//{
		//	cameraObject.get<Transform>().SetLocalPosition(0, 3, 3).LookAt(glm::vec3(0, 0, 0));

		//	// We'll make our camera a component of the camera object
		//	Camera& camera = cameraObject.emplace<Camera>();// Camera::Create();
		//	camera.SetPosition(glm::vec3(0, 3, 3));
		//	camera.SetUp(glm::vec3(0, 0, 1));
		//	camera.LookAt(glm::vec3(0));
		//	camera.SetFovDegrees(90.0f); // Set an initial FOV
		//	camera.SetOrthoHeight(3.0f);
		//	BehaviourBinding::Bind<CameraControlBehaviour>(cameraObject);
		//}

		GameObject cameraObject = scene->CreateEntity("Camera");
		{ //0, 3, 7
			cameraObject.get<Transform>().SetLocalPosition(0, 3, 7).LookAt(glm::vec3(0.0f, 4.0f, 6.4f));

			// We'll make our camera a component of the camera object
			Camera& camera = cameraObject.emplace<Camera>();// Camera::Create();
			camera.SetPosition(glm::vec3(0, 0, 8));
			camera.SetUp(glm::vec3(0, 0, 1));
			camera.LookAt(player.get<Transform>().GetLocalPosition());
			camera.SetFovDegrees(90.0f); // Set an initial FOV
			camera.SetOrthoHeight(8.0f);
			//BehaviourBinding::Bind<CameraControlBehaviour>(cameraObject);
		}

		int width, height;
		glfwGetWindowSize(BackendHandler::window, &width, &height);

		GameObject gBufferObject= scene->CreateEntity("G Buffer");
		{
			gBuffer = &gBufferObject.emplace<GBuffer>();
			gBuffer->Init(width, height);
		}

		GameObject illuminationBufferObject = scene->CreateEntity("Illumination Buffer");
		{
			illuminationBuffer = &illuminationBufferObject.emplace<IlluminationBuffer>();
			illuminationBuffer->Init(width, height);
		}

		int shadowWidth = 4096;
		int shadowHeight = 4096;

		GameObject shadowBufferObject = scene->CreateEntity("Shadow Buffer");
		{
			shadowBuffer = &shadowBufferObject.emplace<Framebuffer>();
			shadowBuffer->AddDepthTarget();
			shadowBuffer->Init(shadowWidth, shadowHeight);
		}

		GameObject framebufferObject = scene->CreateEntity("Basic Effect");
		{
			basicEffect = &framebufferObject.emplace<PostEffect>();
			basicEffect->Init(width, height);
		}

		GameObject sepiaEffectObject = scene->CreateEntity("Sepia Effect");
		{
			sepiaEffect = &sepiaEffectObject.emplace<SepiaEffect>();
			sepiaEffect->Init(width, height);
			sepiaEffect->SetIntensity(0.0f);
		}
		effects.push_back(sepiaEffect);

		GameObject greyscaleEffectObject = scene->CreateEntity("Greyscale Effect");
		{
			greyscaleEffect = &greyscaleEffectObject.emplace<GreyscaleEffect>();
			greyscaleEffect->Init(width, height);
		}
		//effects.push_back(greyscaleEffect);
		
		GameObject colorCorrectEffectObject = scene->CreateEntity("Colour Correction Effect");
		{
			colorCorrectEffect = &colorCorrectEffectObject.emplace<ColorCorrectEffect>();
			colorCorrectEffect->Init(width, height);
		}
		//effects.push_back(colorCorrectEffect);

		GameObject bloomEffectObject = scene->CreateEntity("Bloom Effect");
		{
			bloomEffect = &bloomEffectObject.emplace<BloomEffect>();
			bloomEffect->Init(width, height);
		}
		effects.push_back(bloomEffect);

		GameObject pixelateEffectObject = scene->CreateEntity("Pixelate Effect");
		{
			pixelateEffect = &pixelateEffectObject.emplace<PixelateEffect>();
			pixelateEffect->Init(width, height);
		}
		effects.push_back(pixelateEffect);

		GameObject filmGrainEffectObject = scene->CreateEntity("Film Grain Effect");
		{
			filmGrainEffect = &filmGrainEffectObject.emplace<FilmGrainEffect>();
			filmGrainEffect->Init(width, height);
		}
		effects.push_back(filmGrainEffect);

		#pragma endregion 
		//////////////////////////////////////////////////////////////////////////////////////////

		/////////////////////////////////// SKYBOX ///////////////////////////////////////////////
		
			// Load our shaders
			Shader::sptr skybox = std::make_shared<Shader>();
			skybox->LoadShaderPartFromFile("shaders/skybox-shader.vert.glsl", GL_VERTEX_SHADER);
			skybox->LoadShaderPartFromFile("shaders/skybox-shader.frag.glsl", GL_FRAGMENT_SHADER);
			skybox->Link();

			ShaderMaterial::sptr skyboxMat = ShaderMaterial::Create();
			skyboxMat->Shader = skybox;  
			skyboxMat->Set("s_Environment", environmentMap);
			skyboxMat->Set("u_EnvironmentRotation", glm::mat3(glm::rotate(glm::mat4(1.0f), glm::radians(90.0f), glm::vec3(1, 0, 0))));
			skyboxMat->RenderLayer = 100;

			MeshBuilder<VertexPosNormTexCol> mesh;
			MeshFactory::AddIcoSphere(mesh, glm::vec3(0.0f), 1.0f);
			MeshFactory::InvertFaces(mesh);
			VertexArrayObject::sptr meshVao = mesh.Bake();
			
			GameObject skyboxObj = scene->CreateEntity("skybox");  
			skyboxObj.get<Transform>().SetLocalPosition(0.0f, 0.0f, 0.0f);
			//skyboxObj.get_or_emplace<RendererComponent>().SetMesh(meshVao).SetMaterial(skyboxMat).SetCastShadow(false);
		
		////////////////////////////////////////////////////////////////////////////////////////
		

		// We'll use a vector to store all our key press events for now (this should probably be a behaviour eventually)
		std::vector<KeyPressWatcher> keyToggles;
		{
			// This is an example of a key press handling helper. Look at InputHelpers.h an .cpp to see
			// how this is implemented. Note that the ampersand here is capturing the variables within
			// the scope. If you wanted to do some method on the class, your best bet would be to give it a method and
			// use std::bind
			keyToggles.emplace_back(GLFW_KEY_T, [&]() { cameraObject.get<Camera>().ToggleOrtho(); });

			//Toggles drawing specific buffers
			keyToggles.emplace_back(GLFW_KEY_F1, [&]() { drawGBuffer = !drawGBuffer; });
			keyToggles.emplace_back(GLFW_KEY_F2, [&]() { drawIllumBuffer = !drawIllumBuffer; });
			keyToggles.emplace_back(GLFW_KEY_F3, [&]() { drawColorBuffer = !drawColorBuffer; });

			controllables.push_back(obj2);

			keyToggles.emplace_back(GLFW_KEY_KP_ADD, [&]() {
				BehaviourBinding::Get<SimpleMoveBehaviour>(controllables[selectedVao])->Enabled = false;
				selectedVao++;
				if (selectedVao >= controllables.size())
					selectedVao = 0;
				BehaviourBinding::Get<SimpleMoveBehaviour>(controllables[selectedVao])->Enabled = true;
				});
			keyToggles.emplace_back(GLFW_KEY_KP_SUBTRACT, [&]() {
				BehaviourBinding::Get<SimpleMoveBehaviour>(controllables[selectedVao])->Enabled = false;
				selectedVao--;
				if (selectedVao < 0)
					selectedVao = controllables.size() - 1;
				BehaviourBinding::Get<SimpleMoveBehaviour>(controllables[selectedVao])->Enabled = true;
				});

			keyToggles.emplace_back(GLFW_KEY_Y, [&]() {
				auto behaviour = BehaviourBinding::Get<SimpleMoveBehaviour>(controllables[selectedVao]);
				behaviour->Relative = !behaviour->Relative;
				});
		}

		// Initialize our timing instance and grab a reference for our use
		Timing& time = Timing::Instance();
		time.LastFrame = glfwGetTime();

		glm::vec3 endPos = glm::vec3(0.0f, 0.0f, 1.0f);;
		glm::vec3 startPos = glm::vec3(0.0f, 0.0f, 0.0f);;

		float CloudTimer = 0.0f;
		float CloudTimeLimit = 3.0f;
		bool CloudMove = true;


		///// Game loop /////
		while (!glfwWindowShouldClose(BackendHandler::window)) {
			glfwPollEvents();

			// Update the timing
			time.CurrentFrame = glfwGetTime();
			time.DeltaTime = static_cast<float>(time.CurrentFrame - time.LastFrame);

			time.DeltaTime = time.DeltaTime > 1.0f ? 1.0f : time.DeltaTime;

			// Update our FPS tracker data
			fpsBuffer[frameIx] = 1.0f / time.DeltaTime;
			frameIx++;
			if (frameIx >= 128)
				frameIx = 0;

			PlayerInput(player, time.DeltaTime, 3.0f);
			cameraObject.get<Transform>().SetLocalPosition(player.get<Transform>().GetLocalPosition() + glm::vec3(0, -6, 3));

			// We'll make sure our UI isn't focused before we start handling input for our game
			if (!ImGui::IsAnyWindowFocused()) {
				// We need to poll our key watchers so they can do their logic with the GLFW state
				// Note that since we want to make sure we don't copy our key handlers, we need a const
				// reference!
				for (const KeyPressWatcher& watcher : keyToggles) {
					watcher.Poll(BackendHandler::window);
				}
			}

			// Iterate over all the behaviour binding components
			scene->Registry().view<BehaviourBinding>().each([&](entt::entity entity, BehaviourBinding& binding) {
				// Iterate over all the behaviour scripts attached to the entity, and update them in sequence (if enabled)
				for (const auto& behaviour : binding.Behaviours) {
					if (behaviour->Enabled) {
						behaviour->Update(entt::handle(scene->Registry(), entity));
					}
				}
			});


			//Cloud Lerping
			CloudTimer += time.DeltaTime;

			if (CloudTimer >= CloudTimeLimit)
			{
				CloudTimer = 0.0f;
				CloudMove = !CloudMove;
			}

			float TPos = CloudTimer / CloudTimeLimit;

			if (CloudMove == true)
			{
				obj3.get<Transform>().SetLocalPosition(LERP(startPos, endPos, TPos));
			}
			else if (CloudMove == false)
			{
				obj3.get<Transform>().SetLocalPosition(LERP(endPos, startPos, TPos));
			}
			

			// Clear the screen
			basicEffect->Clear();
			/*greyscaleEffect->Clear();
			sepiaEffect->Clear();*/
			for (int i = 0; i < effects.size(); i++)
			{
				effects[i]->Clear();
			}
			shadowBuffer->Clear();
			gBuffer->Clear();
			illuminationBuffer->Clear();
			

			glClearColor(1.0f, 1.0f, 1.0f, 0.3f);
			glEnable(GL_DEPTH_TEST);
			glClearDepth(1.0f);
			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

			// Update all world matrices for this frame
			scene->Registry().view<Transform>().each([](entt::entity entity, Transform& t) {
				t.UpdateWorldMatrix();
			});

			// Grab out camera info from the camera object
			Transform& camTransform = cameraObject.get<Transform>();
			glm::mat4 view = glm::inverse(camTransform.LocalTransform());
			glm::mat4 projection = cameraObject.get<Camera>().GetProjection();
			glm::mat4 viewProjection = projection * view;

			//Set up light space matrix
			glm::mat4 lightProjectionMatrix = glm::ortho(-20.0f, 20.0f, -20.0f, 20.0f, -30.0f, 30.0f);
			glm::mat4 lightViewMatrix = glm::lookAt(glm::vec3(-illuminationBuffer->GetSunRef()._lightDirection), glm::vec3(), glm::vec3(0.0f, 0.0f, 1.0f));
			glm::mat4 lightSpaceViewProj = lightProjectionMatrix * lightViewMatrix;

			//Set shadow stuff
			illuminationBuffer->SetLightSpaceViewProj(lightSpaceViewProj);
			glm::vec3 camPos = glm::inverse(view) * glm::vec4(0, 0, 0, 1);
			illuminationBuffer->SetCamPos(camPos);

			// Sort the renderers by shader and material, we will go for a minimizing context switches approach here,
			// but you could for instance sort front to back to optimize for fill rate if you have intensive fragment shaders
			renderGroup.sort<RendererComponent>([](const RendererComponent& l, const RendererComponent& r) {
				// Sort by render layer first, higher numbers get drawn last
				if (l.Material->RenderLayer < r.Material->RenderLayer) return true;
				if (l.Material->RenderLayer > r.Material->RenderLayer) return false;

				// Sort by shader pointer next (so materials using the same shader run sequentially where possible)
				if (l.Material->Shader < r.Material->Shader) return true;
				if (l.Material->Shader > r.Material->Shader) return false;

				// Sort by material pointer last (so we can minimize switching between materials)
				if (l.Material < r.Material) return true;
				if (l.Material > r.Material) return false;

				return false;
			});
			
			// Start by assuming no shader or material is applied
			Shader::sptr current = nullptr;
			ShaderMaterial::sptr currentMat = nullptr;

			glViewport(0, 0, shadowWidth, shadowHeight);
			shadowBuffer->Bind();

			renderGroup.each([&](entt::entity e, RendererComponent& renderer, Transform& transform) {
				// Render the mesh
				if (renderer.CastShadows)
				{
					BackendHandler::RenderVAO(simpleDepthShader, renderer.Mesh, viewProjection, transform, lightSpaceViewProj);
				}
			});

			shadowBuffer->Unbind();

			glfwGetWindowSize(BackendHandler::window, &width, &height);

			glViewport(0, 0, width, height);
			
			gBuffer->Bind();
			// Iterate over the render group components and draw them
			renderGroup.each([&](entt::entity e, RendererComponent& renderer, Transform& transform) {
				// If the shader has changed, set up it's uniforms
				if (current != renderer.Material->Shader) {
					current = renderer.Material->Shader;
					current->Bind();
					BackendHandler::SetupShaderForFrame(current, view, projection);
				}
				// If the material has changed, apply it
				if (currentMat != renderer.Material) {
					currentMat = renderer.Material;
					currentMat->Apply();
				}


				// Render the mesh
				BackendHandler::RenderVAO(renderer.Material->Shader, renderer.Mesh, viewProjection, transform, lightSpaceViewProj);
				
			});

			gBuffer->Unbind();
			
			illuminationBuffer->BindBuffer(0);

			skybox->Bind();
			BackendHandler::SetupShaderForFrame(skybox, view, projection);
			skyboxMat->Apply();
			BackendHandler::RenderVAO(skybox, meshVao, viewProjection, skyboxObj.get<Transform>(), lightSpaceViewProj);
			skybox->UnBind();
			
			illuminationBuffer->UnbindBuffer();

			shadowBuffer->BindDepthAsTexture(30);

			illuminationBuffer->ApplyEffect(gBuffer);

			shadowBuffer->UnbindTexture(30);
			
			if(drawGBuffer)
			{
				gBuffer->DrawBuffersToScreen();
			}
			else if (drawIllumBuffer)
			{
				illuminationBuffer->DrawIllumBuffer();
			}
			else if (drawColorBuffer)
			{
				gBuffer->DrawColorBufferToScreen();
			}
			else if (drawDepthBuffer)
			{
				gBuffer->DrawDepthBufferToScreen();
			}
			else if (drawNormalsBuffer)
			{
				gBuffer->DrawNormalBufferToScreen();
			}
			else
			{
				//illuminationBuffer->DrawToScreen();
				effects[activeEffect]->ApplyEffect(illuminationBuffer);

				if (drawCompositedScene)
				{
					effects[activeEffect]->DrawToScreen();
				}
				//effects[activeEffect]->DrawToScreen();
			}

			/*effects[activeEffect]->ApplyEffect(basicEffect);
			
			effects[activeEffect]->DrawToScreen();*/
			
			// Draw our ImGui content
			BackendHandler::RenderImGui();

			scene->Poll();
			glfwSwapBuffers(BackendHandler::window);
			time.LastFrame = time.CurrentFrame;
		}

		// Nullify scene so that we can release references
		Application::Instance().ActiveScene = nullptr;
		//Clean up the environment generator so we can release references
		EnvironmentGenerator::CleanUpPointers();
		BackendHandler::ShutdownImGui();
	}	



	// Clean up the toolkit logger so we don't leak memory
	Logger::Uninitialize();
	return 0;
}