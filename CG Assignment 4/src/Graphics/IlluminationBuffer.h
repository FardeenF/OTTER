#pragma once

#include "Post/PostEffect.h"
#include "UniformBuffer.h"
#include "GBuffer.h"
#include "PointLight.h"
#include "DirectionalLight.h"
#include "VertexArrayObject.h"
#include "ObjLoader.h"
#include "MeshFactory.h"
#include "Transform.h"
#include "Utilities/BackendHandler.h"

enum Lights
{
	DIRECTIONAL,
	AMBIENT,
	HDR
};

//This is a post effect to make our job easier
class IlluminationBuffer : public PostEffect
{
public:
	//Initializes framebuffer
	//Overrides post effect Init
	void Init(unsigned width, unsigned height) override;
	
	//Makes it so apply effect with a PostEffect does nothing for this object
	void ApplyEffect(PostEffect* buffer) override { };
	//Can only apply effect using GBuffer object
	void ApplyEffect(GBuffer* gBuffer);

	void ApplyPointEffect(GBuffer* gBuffer);

	void DrawIllumBuffer();

	void SetLightSpaceViewProj(glm::mat4 lightSpaceViewProj);
	void SetCamPos(glm::vec3 camPos);

	DirectionalLight& GetSunRef();
	PointLight& GetPointLightRef();
	
	//Sets the sun in the scene
	void SetSun(DirectionalLight newSun);
	void SetSun(glm::vec4 lightDir, glm::vec4 lightCol);

	void EnableSun(bool enabled);

	void SetShader(Shader::sptr& shader);

	void SetViewProjection(glm::mat4& viewProj);

	void SetTransform(Transform& transform);

	void SetVAO(VertexArrayObject::sptr vao);


private:
	glm::mat4 _lightSpaceViewProj;
	glm::vec3 _camPos;

	Shader::sptr _icoShader;
	glm::mat4 _viewProj;

	glm::mat4 _worldTransform;
	glm::mat3 _normalMatrix;

	UniformBuffer _sunBuffer;
	UniformBuffer _pointBuffer;

	bool _sunEnabled = true;
	
	DirectionalLight _sun;
	PointLight _pointLight;

	MeshBuilder<VertexPosNormTexCol> builder = MeshBuilder<VertexPosNormTexCol>();

	VertexArrayObject::sptr vao3 = builder.Bake();

	VertexArrayObject::sptr _vao = VertexArrayObject::Create();
};


