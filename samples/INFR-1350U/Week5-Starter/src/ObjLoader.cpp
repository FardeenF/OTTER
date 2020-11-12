#include "ObjLoader.h"

#include <string>
#include <sstream>
#include <fstream>
#include <iostream>
#include <algorithm>

// Borrowed from https://stackoverflow.com/questions/216823/whats-the-best-way-to-trim-stdstring
#pragma region String Trimming

// trim from start (in place)
static inline void ltrim(std::string& s) {
	s.erase(s.begin(), std::find_if(s.begin(), s.end(), [](int ch) {
		return !std::isspace(ch);
		}));
}

// trim from end (in place)
static inline void rtrim(std::string& s) {
	s.erase(std::find_if(s.rbegin(), s.rend(), [](int ch) {
		return !std::isspace(ch);
		}).base(), s.end());
}

// trim from both ends (in place)
static inline void trim(std::string& s) {
	ltrim(s);
	rtrim(s);
}

#pragma endregion 

bool ObjLoader::LoadFromFile(const std::string& filename, std::vector<glm::vec3>& outVec, std::vector<glm::vec2>& outUV, std::vector<glm::vec3>& outNorm)
{
	// Open our file in binary mode
	std::ifstream file;
	file.open(filename, std::ios::binary);

	// If our file fails to open, we will throw an error
	if (!file) {
		throw std::runtime_error("Failed to open file");
	}

	MeshBuilder<VertexPosNormTexCol> mesh;
	std::string line;


	std::vector<GLint> vertexIndices, vtIndices, normalIndices;
	std::vector< glm::vec3 > vertices;
	std::vector< glm::vec2 > uvs;
	std::vector< glm::vec3 > normals;

	// TODO: Load from file
	// Iterate as long as there is content to read
	while (std::getline(file, line)) {
		trim(line);
		if (line.substr(0, 1) == "#")
		{
			// Comment, no-op
		}
		else if (line.substr(0, 5) == "cube ") // We can do equality check this way since the left side is a string and not a char*
		{
			std::istringstream ss = std::istringstream(line.substr(5));
			glm::vec3 pos;
			ss >> pos.x >> pos.y >> pos.z;

			glm::vec3 scale;
			ss >> scale.x >> scale.y >> scale.z;

			glm::vec3 eulerDeg;
			ss >> eulerDeg.x >> eulerDeg.y >> eulerDeg.z;

			glm::vec4 color = glm::vec4(1.0f);
			if (ss.rdbuf()->in_avail() > 0) {
				ss >> color.r >> color.g >> color.b;
			}
			if (ss.rdbuf()->in_avail() > 0) {
				ss >> color.a;
			}
			MeshFactory::AddCube(mesh, pos, scale, eulerDeg, color);
		}
		else if (line.substr(0, 6) == "plane ")
		{
			std::istringstream ss = std::istringstream(line.substr(6));
			glm::vec3 pos;
			ss >> pos.x >> pos.y >> pos.z;

			glm::vec3 normal;
			ss >> normal.x >> normal.y >> normal.z;

			glm::vec3 tangent;
			ss >> tangent.x >> tangent.y >> tangent.z;

			glm::vec2 size;
			ss >> size.x >> size.y;

			glm::vec4 color = glm::vec4(1.0f);

			if (ss.rdbuf()->in_avail() > 0) {
				ss >> color.r >> color.g >> color.b;
			}
			if (ss.rdbuf()->in_avail() > 0) {
				ss >> color.a;
			}

			MeshFactory::AddPlane(mesh, pos, normal, tangent, size, color);
		}
		else if (line.substr(0, 7) == "sphere ")
		{
			std::istringstream ss = std::istringstream(line.substr(7));

			std::string mode;
			ss >> mode;

			int tesselation = 0;
			ss >> tesselation;

			glm::vec3 pos;
			ss >> pos.x >> pos.y >> pos.z;

			glm::vec3 radii;
			ss >> radii.x >> radii.y >> radii.z;

			glm::vec4 color = glm::vec4(1.0f);
			if (ss.rdbuf()->in_avail() > 0) {
				ss >> color.r >> color.g >> color.b;
			}
			if (ss.rdbuf()->in_avail() > 0) {
				ss >> color.a;
			}

			if (mode == "ico") {
				MeshFactory::AddIcoSphere(mesh, pos, radii, tesselation, color);
			}
			else if (mode == "uv") {
				MeshFactory::AddUvSphere(mesh, pos, radii, tesselation, color);
			}
		}
		else if (line.substr(0, 2) == "v ")
		{
			std::istringstream ss = std::istringstream(line.substr(2));

			glm::vec3 v;
			ss >> v.x >> v.y >> v.z;

			vertices.push_back(v);
		}
		else if (line.substr(0, 3) == "vt ")
		{
			std::istringstream ss = std::istringstream(line.substr(3));

			glm::vec2 vt;
			ss >> vt.x >> vt.y;

			uvs.push_back(vt);
		}
		else if (line.substr(0, 3) == "vn ")
		{
			std::istringstream ss = std::istringstream(line.substr(3));

			glm::vec3 vn;
			ss >> vn.x >> vn.y >> vn.z;

			normals.push_back(vn);
		}
		else if (line.substr(0, 2) == "f ")
		{
			std::istringstream ss = std::istringstream(line.substr(2));

			char slash;
			int pos, vt, vn;

			for (int x = 0; x < 3; x++)
			{
				ss >> pos >> slash >> vt >> slash >> vn;

				vertexIndices.push_back(pos);
				vtIndices.push_back(vt);
				normalIndices.push_back(vn);
			}
		}
		
		
	}

	// Note: with actual OBJ files you're going to run into the issue where faces are composited of different indices
	// You'll need to keep track of these and create vertex entries for each vertex in the face
	// If you want to get fancy, you can track which vertices you've already added

	//Load in position, uv, and normals for the indices
	for (size_t i = 0; i < vertexIndices.size(); i++)
	{
		unsigned int vertexIndex = vertexIndices[i] - 1;
		unsigned int vtIndex = vtIndices[i] - 1;
		unsigned int normalIndex = normalIndices[i] - 1;

		glm::vec3 Vertex = vertices[vertexIndex];
		glm::vec2 UV = uvs[vtIndex];
		glm::vec3 Normal = normals[normalIndex];

		outVec.push_back(Vertex);
		outUV.push_back(UV);
		outNorm.push_back(Normal);
	}

	return true;
}
