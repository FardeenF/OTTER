#include "IlluminationBuffer.h"

void IlluminationBuffer::Init(unsigned width, unsigned height)
{
	//Composite Buffer
	int index = int(_buffers.size());
	_buffers.push_back(new Framebuffer());
	_buffers[index]->AddColorTarget(GL_RGBA8);
	_buffers[index]->AddDepthTarget();
	_buffers[index]->Init(width, height);

	//Illum Buffer
	index = int(_buffers.size());
	_buffers.push_back(new Framebuffer());
	_buffers[index]->AddColorTarget(GL_RGBA8);
	_buffers[index]->AddDepthTarget();
	_buffers[index]->Init(width, height);

	//Loads the directional gBuffer shader
	index = int(_shaders.size());
	_shaders.push_back(Shader::Create());
	_shaders[index]->LoadShaderPartFromFile("shaders/passthrough_vert.glsl", GL_VERTEX_SHADER);
	_shaders[index]->LoadShaderPartFromFile("shaders/gBuffer_directional_frag.glsl", GL_FRAGMENT_SHADER);
	_shaders[index]->Link();

	//Loads the ambient gBuffer shader
	index = int(_shaders.size());
	_shaders.push_back(Shader::Create());
	_shaders[index]->LoadShaderPartFromFile("shaders/passthrough_vert.glsl", GL_VERTEX_SHADER);
	_shaders[index]->LoadShaderPartFromFile("shaders/gBuffer_ambient_frag.glsl", GL_FRAGMENT_SHADER);
	_shaders[index]->Link();

	//Loads the ambient gBuffer shader
	index = int(_shaders.size());
	_shaders.push_back(Shader::Create());
	_shaders[index]->LoadShaderPartFromFile("shaders/passthrough_vert.glsl", GL_VERTEX_SHADER);
	_shaders[index]->LoadShaderPartFromFile("shaders/hdr_frag.glsl", GL_FRAGMENT_SHADER);
	_shaders[index]->Link();

	//Allocates sun buffer
	_sunBuffer.AllocateMemory(sizeof(DirectionalLight));
	_pointBuffer.AllocateMemory(sizeof(PointLight));

	
	_pointBuffer.SendData(reinterpret_cast<void*>(&_pointLight), sizeof(PointLight));
	

	//If sun enabled, send data
	if (_sunEnabled)
	{
		_sunBuffer.SendData(reinterpret_cast<void*>(&_sun), sizeof(DirectionalLight));
	}

	PostEffect::Init(width, height);
}

void IlluminationBuffer::ApplyEffect(GBuffer* gBuffer)
{
	_sunBuffer.SendData(reinterpret_cast<void*>(&_sun), sizeof(DirectionalLight));

	
	_pointBuffer.SendData(reinterpret_cast<void*>(&_pointLight), sizeof(DirectionalLight));


	if (_sunEnabled)
	{
		//Binds the directional shader
		_shaders[Lights::DIRECTIONAL]->Bind();
		_shaders[Lights::DIRECTIONAL]->SetUniformMatrix("u_LightSpaceMatrix", _lightSpaceViewProj);
		_shaders[Lights::DIRECTIONAL]->SetUniform("u_CamPos", _camPos);

		//Send the directional light data and bind it
		_sunBuffer.Bind(0);
		//Send the directional light data and bind it
		_pointBuffer.Bind(1);

		gBuffer->BindLighting();

		//Binds and draws to the illumination buffer
		_buffers[1]->RenderToFSQ();

		gBuffer->UnbindLighting();

		//Unbinds the uniform buffer
		_sunBuffer.Unbind(0);
		//Unbinds the uniform buffer
		_pointBuffer.Unbind(1);


		//Unbind shader
		_shaders[Lights::DIRECTIONAL]->UnBind();
	}




	//_pointBuffer.SendData(reinterpret_cast<void*>(&_pointLight), sizeof(DirectionalLight));


	////Send the directional light data and bind it
	//_pointBuffer.Bind(1);

	//gBuffer->BindLighting();

	////Binds and draws to the illumination buffer
	//_buffers[1]->RenderToFSQ();

	//gBuffer->UnbindLighting();

	////Unbinds the uniform buffer
	//_pointBuffer.Unbind(1);










	//Bind ambient shader
	_shaders[Lights::AMBIENT]->Bind();

	//Send the directional light data
	_sunBuffer.Bind(0);

	gBuffer->BindLighting();
	_buffers[1]->BindColorAsTexture(0, 4);
	_buffers[0]->BindColorAsTexture(0, 5);

	_buffers[0]->RenderToFSQ();

	_buffers[0]->UnbindTexture(5);
	_buffers[1]->UnbindTexture(4);
	gBuffer->UnbindLighting();

	//Unbind uniform buffer
	_sunBuffer.Unbind(0);

	_shaders[Lights::AMBIENT]->UnBind();

}

void IlluminationBuffer::ApplyPointEffect(GBuffer* gBuffer)
{

	_pointBuffer.SendData(reinterpret_cast<void*>(&_pointLight), sizeof(DirectionalLight));


	//Binds the directional shader
	_shaders[Lights::DIRECTIONAL]->Bind();
	_shaders[Lights::DIRECTIONAL]->SetUniformMatrix("u_LightSpaceMatrix", _lightSpaceViewProj);
	_shaders[Lights::DIRECTIONAL]->SetUniform("u_CamPos", _camPos);

	//Send the directional light data and bind it
	_pointBuffer.Bind(1);

	gBuffer->BindLighting();

	//Binds and draws to the illumination buffer
	_buffers[1]->RenderToFSQ();

	gBuffer->UnbindLighting();

	//Unbinds the uniform buffer
	_pointBuffer.Unbind(1);

	//Unbind shader
	_shaders[Lights::DIRECTIONAL]->UnBind();
	


	//Bind HDR Shader
	_shaders[Lights::HDR]->Bind();

	//Send the directional light data
	_pointBuffer.Bind(0);

	gBuffer->BindLighting();
	_buffers[1]->BindColorAsTexture(0, 4);
	_buffers[0]->BindColorAsTexture(0, 5);

	_buffers[0]->RenderToFSQ();

	_buffers[0]->UnbindTexture(5);
	_buffers[1]->UnbindTexture(4);
	gBuffer->UnbindLighting();

	//Unbind uniform buffer
	_pointBuffer.Unbind(0);

	_shaders[Lights::HDR]->UnBind();


	//Bind ambient shader
	_shaders[Lights::AMBIENT]->Bind();

	//Send the directional light data
	_sunBuffer.Bind(0);

	gBuffer->BindLighting();
	_buffers[1]->BindColorAsTexture(0, 4);
	_buffers[0]->BindColorAsTexture(0, 5);

	_buffers[0]->RenderToFSQ();

	_buffers[0]->UnbindTexture(5);
	_buffers[1]->UnbindTexture(4);
	gBuffer->UnbindLighting();

	//Unbind uniform buffer
	_sunBuffer.Unbind(0);

	_shaders[Lights::AMBIENT]->UnBind();

}

void IlluminationBuffer::DrawIllumBuffer()
{
	_shaders[_shaders.size() - 1]->Bind();

	_buffers[1]->BindColorAsTexture(0, 0);

	Framebuffer::DrawFullscreenQuad();

	BackendHandler::RenderVAO(_icoShader, _vao, _viewProj, _worldTransform, _normalMatrix, _lightSpaceViewProj);

	//_vao->Render();

	_buffers[1]->UnbindTexture(0);

	_shaders[_shaders.size() - 1]->UnBind();

}

	////////

	//_shaders[_shaders.size() - 1]->Bind();

	//_buffers[1]->BindColorAsTexture(0, 1);

	////vao->Render();

	//Framebuffer::DrawFullscreenQuad();

	//_buffers[1]->UnbindTexture(1);

	//_shaders[_shaders.size() - 1]->UnBind();
//}

void IlluminationBuffer::SetLightSpaceViewProj(glm::mat4 lightSpaceViewProj)
{
	_lightSpaceViewProj = lightSpaceViewProj;
}

void IlluminationBuffer::SetCamPos(glm::vec3 camPos)
{
	_camPos = camPos;
}

DirectionalLight& IlluminationBuffer::GetSunRef()
{
	return _sun;
}

PointLight& IlluminationBuffer::GetPointLightRef()
{
	return _pointLight;
}

void IlluminationBuffer::SetSun(DirectionalLight newSun)
{
	_sun = newSun;
}

void IlluminationBuffer::SetSun(glm::vec4 lightDir, glm::vec4 lightCol)
{
	_sun._lightDirection = lightDir;
	_sun._lightCol = lightCol;
}

void IlluminationBuffer::EnableSun(bool enabled)
{
	_sunEnabled = enabled;
}

void IlluminationBuffer::SetShader(Shader::sptr& shader)
{
	_icoShader = shader;
}

void IlluminationBuffer::SetViewProjection(glm::mat4& viewProj)
{
	_viewProj = viewProj;
}

void IlluminationBuffer::SetTransform(Transform& transform)
{
	_worldTransform = transform.WorldTransform();
	_normalMatrix = transform.WorldNormalMatrix();
}

void IlluminationBuffer::SetVAO(VertexArrayObject::sptr vao)
{
	_vao = vao;
}
