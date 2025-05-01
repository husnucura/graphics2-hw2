#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <fstream>
#include <iostream>
#include <sstream>
#include <vector>
#define _USE_MATH_DEFINES
#include <math.h>
#include <GL/glew.h>
// #include <OpenGL/gl3.h>   // The GL Header File
#include <GLFW/glfw3.h> // The GLFW header
#include <glm/glm.hpp>	// GL Math library header
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtx/quaternion.hpp>

#include <map>
#include <ft2build.h>
#include <format>

#include FT_FREETYPE_H

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#define format_float(val) std::format("{:.2f}", val)

#define BUFFER_OFFSET(i) ((char *)NULL + (i))

using namespace std;

GLuint vao[3];
GLuint gProgram[3];
GLuint texture;
int gWidth = 640, gHeight = 480;

GLint modelingMatrixLoc[2];
GLint viewingMatrixLoc[2];
GLint projectionMatrixLoc[2];
GLint eyePosLoc[2];

glm::mat4 projectionMatrix;
glm::mat4 viewingMatrix;
glm::mat4 modelingMatrix;
glm::vec3 eyePos(0, 0, 0);
glm::vec3 eyeGaze(0, 0, -1);
glm::vec3 eyeUp(0, 1, 0);

GLuint cubemapTexture;
GLuint cubemapVAO, cubemapVBO;
GLuint cubemapProgram;

float exposure = 1.0f;
float gamma_val = 2.2f;

GLint exposureLoc;
GLint gammaLoc;

bool firstMouse = true;
float lastX = gWidth / 2.0f;
float lastY = gHeight / 2.0f;
float yaw = -90.0f;
float pitch = 0.0f;
float fov = 45.0f;
bool middleMousePressed = false;

/// Holds all state information relevant to a character as loaded using FreeType
struct Character
{
	GLuint TextureID;	// ID handle of the glyph texture
	glm::ivec2 Size;	// Size of glyph
	glm::ivec2 Bearing; // Offset from baseline to left/top of glyph
	GLuint Advance;		// Horizontal offset to advance to next glyph
};

std::map<GLchar, Character> Characters;

struct Vertex
{
	Vertex(GLfloat inX, GLfloat inY, GLfloat inZ) : x(inX), y(inY), z(inZ) {}
	GLfloat x, y, z;
};

struct Texture
{
	Texture(GLfloat inU, GLfloat inV) : u(inU), v(inV) {}
	GLfloat u, v;
};

struct Normal
{
	Normal(GLfloat inX, GLfloat inY, GLfloat inZ) : x(inX), y(inY), z(inZ) {}
	GLfloat x, y, z;
};

struct Face
{
	Face(int v[], int t[], int n[])
	{
		vIndex[0] = v[0];
		vIndex[1] = v[1];
		vIndex[2] = v[2];
		tIndex[0] = t[0];
		tIndex[1] = t[1];
		tIndex[2] = t[2];
		nIndex[0] = n[0];
		nIndex[1] = n[1];
		nIndex[2] = n[2];
	}
	GLuint vIndex[3], tIndex[3], nIndex[3];
};

vector<Vertex> gVertices[2];
vector<Texture> gTextures[2];
vector<Normal> gNormals[2];
vector<Face> gFaces[2];

GLuint gVertexAttribBuffer[2], gIndexBuffer[2], gTextVBO;
GLint gInVertexLoc[2], gInNormalLoc[2];
int gVertexDataSizeInBytes[2], gNormalDataSizeInBytes[2], gTextureDataSizeInBytes[2];

bool ParseObj(const string &fileName, int objId)
{
	fstream myfile;

	// Open the input
	myfile.open(fileName.c_str(), std::ios::in);

	if (myfile.is_open())
	{
		string curLine;

		while (getline(myfile, curLine))
		{
			stringstream str(curLine);
			GLfloat c1, c2, c3;
			GLuint index[9];
			string tmp;

			if (curLine.length() >= 2)
			{
				if (curLine[0] == 'v')
				{
					if (curLine[1] == 't') // texture
					{
						str >> tmp; // consume "vt"
						str >> c1 >> c2;
						gTextures[objId].push_back(Texture(c1, c2));
					}
					else if (curLine[1] == 'n') // normal
					{
						str >> tmp; // consume "vn"
						str >> c1 >> c2 >> c3;
						gNormals[objId].push_back(Normal(c1, c2, c3));
					}
					else // vertex
					{
						str >> tmp; // consume "v"
						str >> c1 >> c2 >> c3;
						gVertices[objId].push_back(Vertex(c1, c2, c3));
					}
				}
				else if (curLine[0] == 'f') // face
				{
					str >> tmp; // consume "f"
					char c;
					int vIndex[3], nIndex[3], tIndex[3];
					str >> vIndex[0];
					str >> c >> c; // consume "//"
					str >> nIndex[0];
					str >> vIndex[1];
					str >> c >> c; // consume "//"
					str >> nIndex[1];
					str >> vIndex[2];
					str >> c >> c; // consume "//"
					str >> nIndex[2];

					assert(vIndex[0] == nIndex[0] &&
						   vIndex[1] == nIndex[1] &&
						   vIndex[2] == nIndex[2]); // a limitation for now

					// make indices start from 0
					for (int c = 0; c < 3; ++c)
					{
						vIndex[c] -= 1;
						nIndex[c] -= 1;
						tIndex[c] -= 1;
					}

					gFaces[objId].push_back(Face(vIndex, tIndex, nIndex));
				}
				else
				{
					cout << "Ignoring unidentified line in obj file: " << curLine << endl;
				}
			}

			// data += curLine;
			if (!myfile.eof())
			{
				// data += "\n";
			}
		}

		myfile.close();
	}
	else
	{
		return false;
	}

	assert(gVertices[objId].size() == gNormals[objId].size());

	return true;
}

bool ReadDataFromFile(
	const string &fileName, ///< [in]  Name of the shader file
	string &data)			///< [out] The contents of the file
{
	fstream myfile;

	// Open the input
	myfile.open(fileName.c_str(), std::ios::in);

	if (myfile.is_open())
	{
		string curLine;

		while (getline(myfile, curLine))
		{
			data += curLine;
			if (!myfile.eof())
			{
				data += "\n";
			}
		}

		myfile.close();
	}
	else
	{
		return false;
	}

	return true;
}

GLuint createVS(const char *shaderName)
{
	string shaderSource;

	string filename(shaderName);
	if (!ReadDataFromFile(filename, shaderSource))
	{
		cout << "Cannot find file name: " + filename << endl;
		exit(-1);
	}

	GLint length = shaderSource.length();
	const GLchar *shader = (const GLchar *)shaderSource.c_str();

	GLuint vs = glCreateShader(GL_VERTEX_SHADER);
	glShaderSource(vs, 1, &shader, &length);
	glCompileShader(vs);

	char output[1024] = {0};
	glGetShaderInfoLog(vs, 1024, &length, output);
	printf("VS compile log: %s\n", output);

	return vs;
}

GLuint createFS(const char *shaderName)
{
	string shaderSource;

	string filename(shaderName);
	if (!ReadDataFromFile(filename, shaderSource))
	{
		cout << "Cannot find file name: " + filename << endl;
		exit(-1);
	}

	GLint length = shaderSource.length();
	const GLchar *shader = (const GLchar *)shaderSource.c_str();

	GLuint fs = glCreateShader(GL_FRAGMENT_SHADER);
	glShaderSource(fs, 1, &shader, &length);
	glCompileShader(fs);

	char output[1024] = {0};
	glGetShaderInfoLog(fs, 1024, &length, output);
	printf("FS compile log: %s\n", output);

	return fs;
}

void initFonts(int windowWidth, int windowHeight)
{
	// Set OpenGL options
	// glEnable(GL_CULL_FACE);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	std::cout << "windowWidth = " << windowWidth << std::endl;
	std::cout << "windowHeight = " << windowHeight << std::endl;

	glm::mat4 projection = glm::ortho(0.0f, static_cast<GLfloat>(windowWidth), 0.0f, static_cast<GLfloat>(windowHeight));
	glUseProgram(gProgram[2]);
	glUniformMatrix4fv(glGetUniformLocation(gProgram[2], "projection"), 1, GL_FALSE, glm::value_ptr(projection));

	// FreeType
	FT_Library ft;
	// All functions return a value different than 0 whenever an error occurred
	if (FT_Init_FreeType(&ft))
	{
		std::cout << "ERROR::FREETYPE: Could not init FreeType Library" << std::endl;
	}

	// Load font as face
	FT_Face face;
	if (FT_New_Face(ft, "/usr/share/fonts/truetype/liberation/LiberationSans-Regular.ttf", 0, &face))
	// if (FT_New_Face(ft, "/usr/share/fonts/truetype/gentium-basic/GenBkBasR.ttf", 0, &face))
	{
		std::cout << "ERROR::FREETYPE: Failed to load font" << std::endl;
	}

	// Set size to load glyphs as
	FT_Set_Pixel_Sizes(face, 0, 48);

	// Disable byte-alignment restriction
	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

	// Load first 128 characters of ASCII set
	for (GLubyte c = 0; c < 128; c++)
	{
		// Load character glyph
		if (FT_Load_Char(face, c, FT_LOAD_RENDER))
		{
			std::cout << "ERROR::FREETYTPE: Failed to load Glyph" << std::endl;
			continue;
		}
		// Generate texture
		GLuint texture;
		glGenTextures(1, &texture);
		glBindTexture(GL_TEXTURE_2D, texture);
		glTexImage2D(
			GL_TEXTURE_2D,
			0,
			GL_RED,
			face->glyph->bitmap.width,
			face->glyph->bitmap.rows,
			0,
			GL_RED,
			GL_UNSIGNED_BYTE,
			face->glyph->bitmap.buffer);
		// Set texture options
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		// Now store character for later use
		Character character = {
			texture,
			glm::ivec2(face->glyph->bitmap.width, face->glyph->bitmap.rows),
			glm::ivec2(face->glyph->bitmap_left, face->glyph->bitmap_top),
			(GLuint)face->glyph->advance.x};
		Characters.insert(std::pair<GLchar, Character>(c, character));
	}

	glBindTexture(GL_TEXTURE_2D, 0);
	// Destroy FreeType once we're finished
	FT_Done_Face(face);
	FT_Done_FreeType(ft);

	//
	// Configure VBO for texture quads
	//
	GLuint vaoLocal, vbo;
	glGenVertexArrays(1, &vaoLocal);
	assert(vaoLocal > 0);
	vao[2] = vaoLocal;
	glBindVertexArray(vaoLocal);

	glGenBuffers(1, &gTextVBO);
	glBindBuffer(GL_ARRAY_BUFFER, gTextVBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(GLfloat) * 6 * 4, NULL, GL_DYNAMIC_DRAW);

	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 4 * sizeof(GLfloat), 0);

	glBindBuffer(GL_ARRAY_BUFFER, 0);
}
unsigned int loadCubemap()
{
	std::string folder = "cubemap3/";
	std::vector<std::string> faces = {
		"px.hdr",
		"nx.hdr",
		"py.hdr",
		"ny.hdr",
		"pz.hdr",
		"nz.hdr"};

	unsigned int textureID;
	glGenTextures(1, &textureID);
	glBindTexture(GL_TEXTURE_CUBE_MAP, textureID);

	int width, height, nrChannels;
	for (unsigned int i = 0; i < faces.size(); i++)
	{
		float *data = stbi_loadf((folder + faces[i]).c_str(), &width, &height, &nrChannels, 0);
		if (data)
		{
			glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i,
						 0, GL_RGB16F, width, height, 0, GL_RGB, GL_FLOAT, data);
			stbi_image_free(data);
			std::cout << "Loaded cubemap face: " << faces[i] << std::endl;
		}
		else
		{
			std::cout << "Cubemap texture failed to load at path: " << faces[i] << std::endl;
			stbi_image_free(data);
			return 1;
		}
	}

	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

	return textureID;
}

void initTexture()
{
	glGenTextures(1, &texture);
	glBindTexture(GL_TEXTURE_2D, texture);

	// set the texture wrapping/filtering options (on the currently bound texture object)
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	// load the texture
	int width, height, nrChannels;
	unsigned char *data = stbi_load("haunted_library.jpg", &width, &height, &nrChannels, 0);
	if (data)
	{
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
		glGenerateMipmap(GL_TEXTURE_2D);
	}
	else
	{
		std::cout << "Failed to load texture" << std::endl;
	}
	stbi_image_free(data);
}

void initShaders()
{
	// Create the programs

	gProgram[0] = glCreateProgram(); // for armadillo
	gProgram[1] = glCreateProgram(); // for background quad
	gProgram[2] = glCreateProgram(); // for text rendering

	// Create the shaders for both programs

	// for armadillo
	GLuint vs1 = createVS("vert.glsl"); // or vert2.glsl
	GLuint fs1 = createFS("frag.glsl"); // or frag2.glsl

	// for background quad
	GLuint vs2 = createVS("vert_quad.glsl");
	GLuint fs2 = createFS("frag_quad.glsl");

	// for background quad
	GLuint vs3 = createVS("vert_text.glsl");
	GLuint fs3 = createFS("frag_text.glsl");

	// Attach the shaders to the programs

	glAttachShader(gProgram[0], vs1);
	glAttachShader(gProgram[0], fs1);

	glAttachShader(gProgram[1], vs2);
	glAttachShader(gProgram[1], fs2);

	glAttachShader(gProgram[2], vs3);
	glAttachShader(gProgram[2], fs3);

	// Link the programs

	glLinkProgram(gProgram[0]);
	GLint status;
	glGetProgramiv(gProgram[0], GL_LINK_STATUS, &status);

	if (status != GL_TRUE)
	{
		cout << "Program link failed" << endl;
		exit(-1);
	}

	glLinkProgram(gProgram[1]);
	glGetProgramiv(gProgram[1], GL_LINK_STATUS, &status);

	if (status != GL_TRUE)
	{
		cout << "Program link failed" << endl;
		exit(-1);
	}

	glLinkProgram(gProgram[2]);
	glGetProgramiv(gProgram[2], GL_LINK_STATUS, &status);

	if (status != GL_TRUE)
	{
		cout << "Program link failed" << endl;
		exit(-1);
	}

	// Get the locations of the uniform variables from both programs

	for (int i = 0; i < 2; ++i)
	{
		glUseProgram(gProgram[i]);

		modelingMatrixLoc[i] = glGetUniformLocation(gProgram[i], "modelingMatrix");
		viewingMatrixLoc[i] = glGetUniformLocation(gProgram[i], "viewingMatrix");
		projectionMatrixLoc[i] = glGetUniformLocation(gProgram[i], "projectionMatrix");
		eyePosLoc[i] = glGetUniformLocation(gProgram[i], "eyePos");
	}
}

void initVBO()
{
	for (size_t t = 0; t < 2; t++) // 2 objects. t=0 is armadillo, t=1 is background quad.
	{
		glGenVertexArrays(1, &vao[t]);
		assert(vao[t] > 0);

		glBindVertexArray(vao[t]);
		cout << "vao = " << vao[t] << endl;

		glEnableVertexAttribArray(0);
		glEnableVertexAttribArray(1);
		glEnableVertexAttribArray(2);
		assert(glGetError() == GL_NONE);

		glGenBuffers(1, &gVertexAttribBuffer[t]);
		glGenBuffers(1, &gIndexBuffer[t]);

		assert(gVertexAttribBuffer[t] > 0 && gIndexBuffer[t] > 0);

		glBindBuffer(GL_ARRAY_BUFFER, gVertexAttribBuffer[t]);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, gIndexBuffer[t]);

		gVertexDataSizeInBytes[t] = gVertices[t].size() * 3 * sizeof(GLfloat);
		gNormalDataSizeInBytes[t] = gNormals[t].size() * 3 * sizeof(GLfloat);
		gTextureDataSizeInBytes[t] = gTextures[t].size() * 2 * sizeof(GLfloat);
		int indexDataSizeInBytes = gFaces[t].size() * 3 * sizeof(GLuint);

		GLfloat *vertexData = new GLfloat[gVertices[t].size() * 3];
		GLfloat *normalData = new GLfloat[gNormals[t].size() * 3];
		GLfloat *textureData = new GLfloat[gTextures[t].size() * 2];
		GLuint *indexData = new GLuint[gFaces[t].size() * 3];

		float minX = 1e6, maxX = -1e6;
		float minY = 1e6, maxY = -1e6;
		float minZ = 1e6, maxZ = -1e6;

		for (int i = 0; i < gVertices[t].size(); ++i)
		{
			vertexData[3 * i] = gVertices[t][i].x;
			vertexData[3 * i + 1] = gVertices[t][i].y;
			vertexData[3 * i + 2] = gVertices[t][i].z;

			minX = std::min(minX, gVertices[t][i].x);
			maxX = std::max(maxX, gVertices[t][i].x);
			minY = std::min(minY, gVertices[t][i].y);
			maxY = std::max(maxY, gVertices[t][i].y);
			minZ = std::min(minZ, gVertices[t][i].z);
			maxZ = std::max(maxZ, gVertices[t][i].z);
		}

		std::cout << "minX = " << minX << std::endl;
		std::cout << "maxX = " << maxX << std::endl;
		std::cout << "minY = " << minY << std::endl;
		std::cout << "maxY = " << maxY << std::endl;
		std::cout << "minZ = " << minZ << std::endl;
		std::cout << "maxZ = " << maxZ << std::endl;

		for (int i = 0; i < gNormals[t].size(); ++i)
		{
			normalData[3 * i] = gNormals[t][i].x;
			normalData[3 * i + 1] = gNormals[t][i].y;
			normalData[3 * i + 2] = gNormals[t][i].z;
		}

		for (int i = 0; i < gTextures[t].size(); ++i)
		{
			textureData[2 * i] = gTextures[t][i].u;
			textureData[2 * i + 1] = gTextures[t][i].v;
		}

		for (int i = 0; i < gFaces[t].size(); ++i)
		{
			indexData[3 * i] = gFaces[t][i].vIndex[0];
			indexData[3 * i + 1] = gFaces[t][i].vIndex[1];
			indexData[3 * i + 2] = gFaces[t][i].vIndex[2];
		}

		glBufferData(GL_ARRAY_BUFFER, gVertexDataSizeInBytes[t] + gNormalDataSizeInBytes[t] + gTextureDataSizeInBytes[t], 0, GL_STATIC_DRAW);
		glBufferSubData(GL_ARRAY_BUFFER, 0, gVertexDataSizeInBytes[t], vertexData);
		glBufferSubData(GL_ARRAY_BUFFER, gVertexDataSizeInBytes[t], gNormalDataSizeInBytes[t], normalData);
		glBufferSubData(GL_ARRAY_BUFFER, gVertexDataSizeInBytes[t] + gNormalDataSizeInBytes[t], gTextureDataSizeInBytes[t], textureData);
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, indexDataSizeInBytes, indexData, GL_STATIC_DRAW);

		// done copying; can free now
		delete[] vertexData;
		delete[] normalData;
		delete[] textureData;
		delete[] indexData;

		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);
		glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 0, BUFFER_OFFSET(gVertexDataSizeInBytes[t]));
		glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 0, BUFFER_OFFSET(gVertexDataSizeInBytes[t] + gNormalDataSizeInBytes[t]));
	}
}
void initCubemap()
{
	float cubeVertices[] = {
		-1.0f, 1.0f, -1.0f,
		-1.0f, -1.0f, -1.0f,
		1.0f, -1.0f, -1.0f,
		1.0f, -1.0f, -1.0f,
		1.0f, 1.0f, -1.0f,
		-1.0f, 1.0f, -1.0f,

		-1.0f, -1.0f, 1.0f,
		-1.0f, -1.0f, -1.0f,
		-1.0f, 1.0f, -1.0f,
		-1.0f, 1.0f, -1.0f,
		-1.0f, 1.0f, 1.0f,
		-1.0f, -1.0f, 1.0f,

		1.0f, -1.0f, -1.0f,
		1.0f, -1.0f, 1.0f,
		1.0f, 1.0f, 1.0f,
		1.0f, 1.0f, 1.0f,
		1.0f, 1.0f, -1.0f,
		1.0f, -1.0f, -1.0f,

		-1.0f, -1.0f, 1.0f,
		-1.0f, 1.0f, 1.0f,
		1.0f, 1.0f, 1.0f,
		1.0f, 1.0f, 1.0f,
		1.0f, -1.0f, 1.0f,
		-1.0f, -1.0f, 1.0f,

		-1.0f, 1.0f, -1.0f,
		1.0f, 1.0f, -1.0f,
		1.0f, 1.0f, 1.0f,
		1.0f, 1.0f, 1.0f,
		-1.0f, 1.0f, 1.0f,
		-1.0f, 1.0f, -1.0f,

		-1.0f, -1.0f, -1.0f,
		-1.0f, -1.0f, 1.0f,
		1.0f, -1.0f, -1.0f,
		1.0f, -1.0f, -1.0f,
		-1.0f, -1.0f, 1.0f,
		1.0f, -1.0f, 1.0f};

	glGenVertexArrays(1, &cubemapVAO);
	glGenBuffers(1, &cubemapVBO);
	glBindVertexArray(cubemapVAO);
	glBindBuffer(GL_ARRAY_BUFFER, cubemapVBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(cubeVertices), &cubeVertices, GL_STATIC_DRAW);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void *)0);
	cubemapTexture = loadCubemap();
	GLuint vs_cubemap = createVS("vert_cubemap.glsl");
	GLuint fs_cubemap = createFS("frag_cubemap.glsl");
	cubemapProgram = glCreateProgram();
	glAttachShader(cubemapProgram, vs_cubemap);
	glAttachShader(cubemapProgram, fs_cubemap);
	glLinkProgram(cubemapProgram);

	// Get uniform locations
	glUseProgram(cubemapProgram);
	glUniform1i(glGetUniformLocation(cubemapProgram, "cubeSampler"), 0);
	exposureLoc = glGetUniformLocation(cubemapProgram, "exposure");
	gammaLoc = glGetUniformLocation(cubemapProgram, "gamma");
}
void init()
{
	ParseObj("armadillo.obj", 0);
	glEnable(GL_DEPTH_TEST);
	initTexture();
	initShaders();
	initVBO();
	initFonts(gWidth, gHeight);
	initCubemap();
}

void drawScene()
{
	glDepthFunc(GL_LEQUAL);
	glUseProgram(cubemapProgram);

	glm::mat4 view = glm::mat4(glm::mat3(viewingMatrix));
	glUniformMatrix4fv(glGetUniformLocation(cubemapProgram, "view"), 1, GL_FALSE, glm::value_ptr(view));
	glUniformMatrix4fv(glGetUniformLocation(cubemapProgram, "projection"), 1, GL_FALSE, glm::value_ptr(projectionMatrix));
	glUniform1f(exposureLoc, exposure);
	glUniform1f(gammaLoc, gamma_val);

	glBindVertexArray(cubemapVAO);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_CUBE_MAP, cubemapTexture);
	glDrawArrays(GL_TRIANGLES, 0, 36);
	glBindVertexArray(0);
	glDepthFunc(GL_LESS); // Reset depth function to default

	// Then draw the textured objects (armadillo)
	glUseProgram(gProgram[0]);
	glBindTexture(GL_TEXTURE_2D, texture);

	glUniformMatrix4fv(projectionMatrixLoc[0], 1, GL_FALSE, glm::value_ptr(projectionMatrix));
	glUniformMatrix4fv(viewingMatrixLoc[0], 1, GL_FALSE, glm::value_ptr(viewingMatrix));
	glUniformMatrix4fv(modelingMatrixLoc[0], 1, GL_FALSE, glm::value_ptr(modelingMatrix));
	glUniform3fv(eyePosLoc[0], 1, glm::value_ptr(eyePos));

	glBindVertexArray(vao[0]);
	glDrawElements(GL_TRIANGLES, gFaces[0].size() * 3, GL_UNSIGNED_INT, 0);
}
void renderText(const std::string &text, GLfloat x, GLfloat y, GLfloat scale, glm::vec3 color, bool right_aligned = false, bool bottom_aligned = false)
{
	// Calculate total width and height if alignment is enabled
	GLfloat textWidth = 0.0f;
	GLfloat maxHeight = 0.0f;

	if (right_aligned || bottom_aligned)
	{
		for (char c : text)
		{
			Character ch = Characters[c];
			textWidth += (ch.Advance >> 6) * scale; // advance in pixels
			GLfloat h = ch.Size.y * scale;
			if (h > maxHeight)
				maxHeight = h;
		}

		if (right_aligned)
			x -= textWidth;
		if (bottom_aligned)
			y += maxHeight;
	}

	// Activate corresponding render state
	glUseProgram(gProgram[2]);
	glUniform3f(glGetUniformLocation(gProgram[2], "textColor"), color.x, color.y, color.z);
	glActiveTexture(GL_TEXTURE0);

	for (char c : text)
	{
		Character ch = Characters[c];

		GLfloat xpos = x + ch.Bearing.x * scale;
		GLfloat ypos = y - (ch.Size.y - ch.Bearing.y) * scale;

		GLfloat w = ch.Size.x * scale;
		GLfloat h = ch.Size.y * scale;

		GLfloat vertices[6][4] = {
			{xpos, ypos + h, 0.0, 0.0},
			{xpos, ypos, 0.0, 1.0},
			{xpos + w, ypos, 1.0, 1.0},

			{xpos, ypos + h, 0.0, 0.0},
			{xpos + w, ypos, 1.0, 1.0},
			{xpos + w, ypos + h, 1.0, 0.0}};

		glBindTexture(GL_TEXTURE_2D, ch.TextureID);
		glBindVertexArray(vao[2]);

		glBindBuffer(GL_ARRAY_BUFFER, gTextVBO);
		glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(vertices), vertices);

		glDrawArrays(GL_TRIANGLES, 0, 6);

		x += (ch.Advance >> 6) * scale;
	}

	glBindTexture(GL_TEXTURE_2D, 0);
}

void mouse_callback(GLFWwindow *window, double xpos, double ypos)
{
	if (!middleMousePressed)
		return;

	if (firstMouse)
	{

		firstMouse = false;
		lastX = xpos;
		lastY = ypos;
	}
	float xoffset = xpos - lastX;
	float yoffset = lastY - ypos;
	lastX = xpos;
	lastY = ypos;

	float sensitivity = 0.1f;
	xoffset *= sensitivity;
	yoffset *= sensitivity;

	yaw += xoffset;
	pitch += yoffset;

	// Constrain pitch to avoid screen flipping
	if (pitch > 89.0f)
		pitch = 89.0f;
	if (pitch < -89.0f)
		pitch = -89.0f;

	// Update camera vectors
	glm::vec3 front;
	front.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
	front.y = sin(glm::radians(pitch));
	front.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));
	eyeGaze = glm::normalize(front);
	eyeUp = glm::vec3(0.0f, 1.0f, 0.0f); // Keep up vector fixed to world up
}
void renderInfo()
{
	renderText("Test", 0, gHeight - 25, 0.6, glm::vec3(1, 1, 0));
	renderText("Exposure:" + format_float(exposure), gWidth - 5, 1.0, 0.6, glm::vec3(1, 1, 0), true, true);
}
void display()
{
	glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	viewingMatrix = glm::lookAt(eyePos, eyePos + eyeGaze, eyeUp);

	static float angle = 0;
	float angleRad = (float)(angle / 180.0) * M_PI;

	// Compute the modeling matrix for the armadillo
	glm::mat4 matT = glm::translate(glm::mat4(1.0), glm::vec3(0.0f, 0.0f, -7.0f));
	glm::mat4 matRx = glm::rotate<float>(glm::mat4(1.0), (-90. / 180.) * M_PI, glm::vec3(1.0, 0.0, 0.0));
	glm::mat4 matRy = glm::rotate<float>(glm::mat4(1.0), (-90. / 180.) * M_PI, glm::vec3(0.0, 1.0, 0.0));
	// modelingMatrix = matT * matRy * matRx;
	modelingMatrix = matT;

	// Let's make some alternating roll rotation
	static float rollDeg = 0;
	static float changeRoll = 2.5;
	float rollRad = (float)(rollDeg / 180.f) * M_PI;
	rollDeg += changeRoll;

	glm::mat4 matRoll = glm::rotate<float>(glm::mat4(1.0), rollRad, glm::vec3(0.0, 1.0, 0.0));

	modelingMatrix = modelingMatrix * matRoll;

	drawScene();

	// Draw text
	glDisable(GL_DEPTH_TEST);
	renderInfo();
	glEnable(GL_DEPTH_TEST);

	angle += 0.5;
}

void reshape(GLFWwindow *window, int w, int h)
{
	w = w < 1 ? 1 : w;
	h = h < 1 ? 1 : h;

	gWidth = w;
	gHeight = h;

	glViewport(0, 0, w, h);

	float fovyRad = (float)(45.0 / 180.0) * M_PI;
	projectionMatrix = glm::perspective(fovyRad, (float)w / (float)h, 0.1f, 100.0f);

	// Update viewing matrix with current eyeGaze
	viewingMatrix = glm::lookAt(eyePos, eyePos + eyeGaze, eyeUp);
	glm::mat4 projection = glm::ortho(0.0f, static_cast<GLfloat>(gWidth), 0.0f, static_cast<GLfloat>(gHeight));
	glUseProgram(gProgram[2]);
	glUniformMatrix4fv(glGetUniformLocation(gProgram[2], "projection"), 1, GL_FALSE, glm::value_ptr(projection));
}
void keyboard(GLFWwindow *window, int key, int scancode, int action, int mods)
{
	if (key == GLFW_KEY_Q && action == GLFW_PRESS)
	{
		glfwSetWindowShouldClose(window, GLFW_TRUE);
	}
	else if (key == GLFW_KEY_F && action == GLFW_PRESS)
	{
		// glShadeModel(GL_FLAT);
	}
	else if (key == GLFW_KEY_K)
	{
		if (action == GLFW_PRESS)
		{
			middleMousePressed = !middleMousePressed;
			firstMouse = true;
		}
	}
}

void mainLoop(GLFWwindow *window)
{
	while (!glfwWindowShouldClose(window))
	{
		display();
		glfwSwapBuffers(window);
		glfwPollEvents();
	}
}

int main(int argc, char **argv) // Create Main Function For Bringing It All Together
{
	GLFWwindow *window;
	if (!glfwInit())
	{
		exit(-1);
	}

	// glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 2);
	// glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 1);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
	// glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_COMPAT_PROFILE);
	// glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);

	window = glfwCreateWindow(gWidth, gHeight, "Simple Example", NULL, NULL);

	if (!window)
	{
		glfwTerminate();
		exit(-1);
	}

	glfwMakeContextCurrent(window);
	glfwSwapInterval(1);

	// Initialize GLEW to setup the OpenGL Function pointers
	if (GLEW_OK != glewInit())
	{
		std::cout << "Failed to initialize GLEW" << std::endl;
		return EXIT_FAILURE;
	}

	char rendererInfo[512] = {0};
	strcpy(rendererInfo, (const char *)glGetString(GL_RENDERER));
	strcat(rendererInfo, " - ");
	strcat(rendererInfo, (const char *)glGetString(GL_VERSION));
	glfwSetWindowTitle(window, rendererInfo);

	init();

	glfwSetKeyCallback(window, keyboard);
	glfwSetWindowSizeCallback(window, reshape);
	glfwSetCursorPosCallback(window, mouse_callback);

	reshape(window, gWidth, gHeight); // need to call this once ourselves
	mainLoop(window);				  // this does not return unless the window is closed

	glfwDestroyWindow(window);
	glfwTerminate();

	return 0;
}
