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
std::string currentRenderModeStr = "Default";
GLuint vao[3];
GLuint gProgram[3];
GLuint texture;
int gWidth = 640, gHeight = 480;
double lastTime = glfwGetTime();
int frameCount = 0;
int fps = 0;
bool vsync = true;
std::string currentKeyPressed = "";
double keyPressTimer = 0.0;

void calculateFPS()
{
	double currentTime = glfwGetTime();
	frameCount++;

	if (currentTime - lastTime >= 1.0)
	{
		fps = double(frameCount) / (currentTime - lastTime);
		frameCount = 0;
		lastTime = currentTime;
	}
}

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

GLuint cubemapFBO, cubemapColorTex, cubemapDepthRBO;

GLuint quadVAO = 0, quadVBO = 0;
GLuint fullscreenShaderProgram = 0;

float exposure = 1.0f;
float gamma_val = 2.2f;

GLint exposureLoc;
GLint gammaLoc;

bool firstMouse = true;
glm::quat cameraOrientation = glm::quat(1, 0, 0, 0); // Identity quaternion
float lastX = gWidth / 2.0f;
float lastY = gHeight / 2.0f;
float yaw = 0.0f;
float pitch = 0.0f;
float fov = 45.0f;
bool middleMousePressed = false;
GLuint gBufferShaderProgram;
GLuint depthRBO;

GLuint compositeShaderProgram;
GLuint lightingTex;

glm::vec3 lightPos = glm::vec3(2.0f, 4.0f, -2.0f);
glm::vec3 lightColor = glm::vec3(1.0f, 0.85f, 0.6f);
glm::vec3 cameraPos = glm::vec3(0.0f, 0.0f, 0.0f);
float minX = 1e6, maxX = -1e6;
float minY = 1e6, maxY = -1e6;
float minZ = 1e6, maxZ = -1e6;

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

GLuint deferredLightingShaderProgram;

GLuint motionBlurFBO, toneMapFBO;
GLuint motionBlurTexture, toneMapTexture;
GLuint motionBlurShader, toneMapShader;
float blurAmount = 0.0f;
float blurEnabled = true;
float angularVelocity = 0.0f;
glm::vec2 lastMouseOffset = glm::vec2(0.0f);
double lastFrameTime = 0.0;
float keyValue = 0.18f;
bool gammaEnabled = true;
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
vector<float> tmp;

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
}

GLuint createShaderProgram(const char *vertexPath, const char *fragmentPath)
{
	GLuint program = glCreateProgram();

	if (vertexPath && std::strlen(vertexPath) > 0)
	{
		GLuint vs = createVS(vertexPath);
		glAttachShader(program, vs);
		glDeleteShader(vs);
	}

	if (fragmentPath && std::strlen(fragmentPath) > 0)
	{
		GLuint fs = createFS(fragmentPath);
		glAttachShader(program, fs);
		glDeleteShader(fs);
	}

	glLinkProgram(program);

	GLint status;
	glGetProgramiv(program, GL_LINK_STATUS, &status);
	if (status != GL_TRUE)
	{
		char buffer[512];
		glGetProgramInfoLog(program, 512, NULL, buffer);
		std::cerr << "Shader program linking failed ("
				  << (vertexPath ? vertexPath : "nullptr") << ", "
				  << (fragmentPath ? fragmentPath : "nullptr") << "):\n"
				  << buffer << std::endl;
		exit(-1);
	}

	return program;
}

void initShaders()
{
	// Create and link shader programs using the helper
	gProgram[0] = createShaderProgram("vert.glsl", "frag.glsl");		   // Armadillo
	gProgram[1] = createShaderProgram("vert_quad.glsl", "frag_quad.glsl"); // Background quad
	gProgram[2] = createShaderProgram("vert_text.glsl", "frag_text.glsl"); // Text rendering

	gBufferShaderProgram = createShaderProgram("gBuffer.vert", "gBuffer.frag");
	cubemapProgram = createShaderProgram("cubemap.vert", "cubemap.frag");
	fullscreenShaderProgram = createShaderProgram("quad.vert", "quad.frag");

	deferredLightingShaderProgram = createShaderProgram("quad.vert", "lighting.frag");
	compositeShaderProgram = createShaderProgram("quad.vert", "composite.frag");

	motionBlurShader = createShaderProgram("quad.vert", "motion_blur.frag");
	toneMapShader = createShaderProgram("quad.vert", "tone_mapping.frag");

	glUseProgram(cubemapProgram);

	glUniform1i(glGetUniformLocation(cubemapProgram, "cubeSampler"), 0);
	exposureLoc = glGetUniformLocation(cubemapProgram, "exposure");
	gammaLoc = glGetUniformLocation(cubemapProgram, "gamma");

	for (int i = 0; i < 1; ++i)
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
	for (size_t t = 0; t < 1; t++) // 2 objects. t=0 is armadillo, t=1 is background quad.
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

		for (int i = 0; i < gVertices[t].size(); ++i)
		{
			vertexData[3 * i] = gVertices[t][i].x;
			vertexData[3 * i + 1] = gVertices[t][i].y;
			vertexData[3 * i + 2] = gVertices[t][i].z;
			tmp.push_back(gVertices[t][i].x);
			tmp.push_back(gVertices[t][i].y);
			tmp.push_back(gVertices[t][i].z);

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
void initFullscreenQuad()
{
	// Only initialize once
	if (quadVAO != 0)
		return;

	// Quad vertices: positions (x, y) and texture coords (u, v)
	float quadVertices[] = {
		// positions   // texCoords
		-1.0f, 1.0f, 0.0f, 1.0f,
		-1.0f, -1.0f, 0.0f, 0.0f,
		1.0f, -1.0f, 1.0f, 0.0f,

		-1.0f, 1.0f, 0.0f, 1.0f,
		1.0f, -1.0f, 1.0f, 0.0f,
		1.0f, 1.0f, 1.0f, 1.0f};

	// Create VAO and VBO
	glGenVertexArrays(1, &quadVAO);
	glGenBuffers(1, &quadVBO);
	glBindVertexArray(quadVAO);
	glBindBuffer(GL_ARRAY_BUFFER, quadVBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), quadVertices, GL_STATIC_DRAW);

	// Position attribute
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void *)0);

	// TexCoord attribute
	glEnableVertexAttribArray(1);
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void *)(2 * sizeof(float)));

	glBindVertexArray(0);
}

GLuint gBuffer, gPosition, gNormal, gAlbedoSpec, rboDepth;

void initGBuffer()
{

	glGenFramebuffers(1, &gBuffer);
	glBindFramebuffer(GL_FRAMEBUFFER, gBuffer);

	glGenTextures(1, &gPosition);
	glBindTexture(GL_TEXTURE_2D, gPosition);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB16F, gWidth, gHeight, 0, GL_RGB, GL_FLOAT, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, gPosition, 0);

	glGenTextures(1, &gNormal);
	glBindTexture(GL_TEXTURE_2D, gNormal);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB16F, gWidth, gHeight, 0, GL_RGB, GL_FLOAT, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, GL_TEXTURE_2D, gNormal, 0);

	glGenTextures(1, &gAlbedoSpec);
	glBindTexture(GL_TEXTURE_2D, gAlbedoSpec);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, gWidth, gHeight, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT2, GL_TEXTURE_2D, gAlbedoSpec, 0);

	unsigned int attachments[3] = {GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1, GL_COLOR_ATTACHMENT2};
	glDrawBuffers(3, attachments);

	glGenRenderbuffers(1, &rboDepth);
	glBindRenderbuffer(GL_RENDERBUFFER, rboDepth);
	glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT, gWidth, gHeight);
	glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, rboDepth);

	if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
		std::cout << "Framebuffer not complete!" << std::endl;
	glBindTexture(GL_TEXTURE_2D, 0);
	glBindRenderbuffer(GL_RENDERBUFFER, 0);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void initGBufferShaders()
{
	GLuint vs = createVS("gBuffer.vert");
	GLuint fs = createFS("gBuffer.frag");

	gBufferShaderProgram = glCreateProgram();
	glAttachShader(gBufferShaderProgram, vs);
	glAttachShader(gBufferShaderProgram, fs);
	glLinkProgram(gBufferShaderProgram);

	GLint status;
	glGetProgramiv(gBufferShaderProgram, GL_LINK_STATUS, &status);
	if (status != GL_TRUE)
	{
		char buffer[512];
		glGetProgramInfoLog(gBufferShaderProgram, 512, NULL, buffer);
		std::cerr << "G-buffer shader program linking failed:\n"
				  << buffer << std::endl;
		exit(-1);
	}

	glDeleteShader(vs);
	glDeleteShader(fs);
}
GLuint lightFBO, lightColorTex, lightDepthRBO;

void initLightingFBO()
{
	// Create framebuffer if it doesn't exist
	glGenFramebuffers(1, &lightFBO);

	// Create texture to store the lighting result

	glGenTextures(1, &lightColorTex);
	glBindTexture(GL_TEXTURE_2D, lightColorTex);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

	// Allocate storage for the texture
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, gWidth, gHeight, 0, GL_RGBA, GL_FLOAT, nullptr);

	// Create a depth renderbuffer if it doesn't exist

	glGenRenderbuffers(1, &lightDepthRBO);
	glBindRenderbuffer(GL_RENDERBUFFER, lightDepthRBO);
	glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, gWidth, gHeight);

	// Bind framebuffer and attach textures
	glBindFramebuffer(GL_FRAMEBUFFER, lightFBO);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, lightColorTex, 0);
	glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, lightDepthRBO);

	// Check if the framebuffer is complete
	if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
		std::cerr << "ERROR::FRAMEBUFFER:: Lighting framebuffer is not complete!" << std::endl;

	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void initCubemapFBO()
{
	// Create framebuffer if it doesn't exist
	glGenFramebuffers(1, &cubemapFBO);

	// Create texture if it doesn't exist

	glGenTextures(1, &cubemapColorTex);
	glBindTexture(GL_TEXTURE_2D, cubemapColorTex);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	// Create renderbuffer if it doesn't exist

	glGenRenderbuffers(1, &cubemapDepthRBO);
	glBindRenderbuffer(GL_RENDERBUFFER, cubemapDepthRBO);

	// Resize the texture and renderbuffer with the new dimensions
	glBindTexture(GL_TEXTURE_2D, cubemapColorTex);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB16F, gWidth, gHeight, 0, GL_RGB, GL_FLOAT, nullptr);

	glBindRenderbuffer(GL_RENDERBUFFER, cubemapDepthRBO);
	glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, gWidth, gHeight);

	// Attach the texture and renderbuffer to the framebuffer
	glBindFramebuffer(GL_FRAMEBUFFER, cubemapFBO);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, cubemapColorTex, 0);
	glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, cubemapDepthRBO);

	// Check if the framebuffer is complete
	if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
		std::cerr << "ERROR::FRAMEBUFFER:: Cubemap framebuffer is not complete!" << std::endl;

	glBindFramebuffer(GL_FRAMEBUFFER, 0);
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
	initCubemapFBO();
}
unsigned int compositeFBO;
unsigned int compositeTexture;

void initComposite()
{

	glGenFramebuffers(1, &compositeFBO);
	glBindFramebuffer(GL_FRAMEBUFFER, compositeFBO);

	glGenTextures(1, &compositeTexture);
	glBindTexture(GL_TEXTURE_2D, compositeTexture);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, gWidth, gHeight, 0, GL_RGB, GL_UNSIGNED_BYTE, nullptr);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, compositeTexture, 0);

	if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
		std::cerr << "Composite framebuffer not complete!" << std::endl;

	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void initMotionBlurAndToneMapping()
{
	glGenFramebuffers(1, &motionBlurFBO);
	glBindFramebuffer(GL_FRAMEBUFFER, motionBlurFBO);

	glGenTextures(1, &motionBlurTexture);
	glBindTexture(GL_TEXTURE_2D, motionBlurTexture);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, gWidth, gHeight, 0, GL_RGBA, GL_FLOAT, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, motionBlurTexture, 0);

	GLuint luminanceTexture;
	glGenTextures(1, &luminanceTexture);
	glBindTexture(GL_TEXTURE_2D, luminanceTexture);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_R32F, gWidth, gHeight, 0, GL_RED, GL_FLOAT, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, GL_TEXTURE_2D, luminanceTexture, 0);

	GLenum drawBuffers[2] = {GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1};
	glDrawBuffers(2, drawBuffers);

	if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
	{
		std::cerr << "Framebuffer not complete!" << std::endl;
	}

	glGenFramebuffers(1, &toneMapFBO);
	glGenTextures(1, &toneMapTexture);
	glBindTexture(GL_TEXTURE_2D, toneMapTexture);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, gWidth, gHeight, 0, GL_RGBA, GL_FLOAT, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glBindFramebuffer(GL_FRAMEBUFFER, toneMapFBO);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, toneMapTexture, 0);
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
	initGBuffer();
	initFullscreenQuad();
	initLightingFBO();
	initMotionBlurAndToneMapping();
	initComposite();
}
void geometryPass()
{
	glEnable(GL_DEPTH_TEST);

	glBindFramebuffer(GL_FRAMEBUFFER, gBuffer);

	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	glUseProgram(gBufferShaderProgram);

	glUniformMatrix4fv(glGetUniformLocation(gBufferShaderProgram, "model"), 1, GL_FALSE, glm::value_ptr(modelingMatrix));
	glUniformMatrix4fv(glGetUniformLocation(gBufferShaderProgram, "view"), 1, GL_FALSE, glm::value_ptr(viewingMatrix));
	glUniformMatrix4fv(glGetUniformLocation(gBufferShaderProgram, "projection"), 1, GL_FALSE, glm::value_ptr(projectionMatrix));
	// glUniformMatrix4fv(glGetUniformLocation(gBufferShaderProgram, "eyePos"), 1, GL_FALSE, glm::value_ptr(eyePos));

	/* glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, diffuseTexture);
	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, specularTexture); */

	glBindVertexArray(vao[0]);
	glDrawElements(GL_TRIANGLES, gFaces[0].size() * 3, GL_UNSIGNED_INT, 0);
	glBindVertexArray(0);

	glBindVertexArray(0);
	glUseProgram(0);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

int renderMode = 1;
void drawCubemapTonemappedWithMotionBlur() {
};
void drawCubemapWithMotionBlur() {};
void drawLitArmadillo() {};
void renderTexture(GLuint shaderProgram, GLuint textureID, int visualizeMode, GLuint outputFBO = 0)
{
	glBindFramebuffer(GL_FRAMEBUFFER, outputFBO);
	glViewport(0, 0, gWidth, gHeight);

	glUseProgram(shaderProgram);
	glBindVertexArray(quadVAO);

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, textureID);
	glUniform1i(glGetUniformLocation(shaderProgram, "screenTexture"), 0);
	glUniform1i(glGetUniformLocation(shaderProgram, "visualizeMode"), visualizeMode);

	if (visualizeMode == 2)
	{
		glm::vec3 minModel(1e6f);
		glm::vec3 maxModel(-1e6f);

		int t = 0;
		bool azd = false;
		for (int i = 0; i < gVertices[t].size(); ++i)
		{
			minModel.x = std::min(minModel.x, tmp[3 * i]);
			maxModel.x = std::max(maxModel.x, tmp[3 * i]);
			minModel.y = std::min(minModel.y, tmp[3 * i + 1]);
			maxModel.y = std::max(maxModel.y, tmp[3 * i + 1]);
			minModel.z = std::min(minModel.z, tmp[3 * i + 2]);
			maxModel.z = std::max(maxModel.z, tmp[3 * i + 2]);
		}

		glm::vec4 transformed = modelingMatrix * glm::vec4(minModel, 1.0f);
		minModel = glm::vec3(transformed);
		transformed = modelingMatrix * glm::vec4(maxModel, 1.0f);
		maxModel = glm::vec3(transformed);

		if (azd == true)
			return;

		glUniform3fv(glGetUniformLocation(shaderProgram, "minPos"), 1, glm::value_ptr(minModel));
		glUniform3fv(glGetUniformLocation(shaderProgram, "maxPos"), 1, glm::value_ptr(maxModel));
		std::cout << "min" << minModel.x << "," << minModel.y << "," << minModel.z << std::endl;
		std::cout << "max" << maxModel.x << "," << maxModel.y << "," << maxModel.z << std::endl;

		azd = true;
	}

	glDrawArrays(GL_TRIANGLES, 0, 6);

	glBindVertexArray(0);
	glUseProgram(0);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void drawWorldPositions()
{
	geometryPass();
	renderTexture(fullscreenShaderProgram, gPosition, 2);
}

void drawNormals()
{
	geometryPass();
	renderTexture(fullscreenShaderProgram, gNormal, 1);
}
void blitTo(GLuint sourceFBO, GLuint outputFBO = 0)
{
	// Save currently bound framebuffers
	GLint prevReadFBO = 0, prevDrawFBO = 0;
	glGetIntegerv(GL_READ_FRAMEBUFFER_BINDING, &prevReadFBO);
	glGetIntegerv(GL_DRAW_FRAMEBUFFER_BINDING, &prevDrawFBO);

	// Bind desired FBOs for blitting
	glBindFramebuffer(GL_READ_FRAMEBUFFER, sourceFBO);
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, outputFBO);

	// Perform the blit
	glBlitFramebuffer(
		0, 0, gWidth, gHeight,
		0, 0, gWidth, gHeight,
		GL_COLOR_BUFFER_BIT,
		GL_NEAREST);

	// Restore previous framebuffer bindings
	glBindFramebuffer(GL_READ_FRAMEBUFFER, prevReadFBO);
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, prevDrawFBO);
}

void drawCubemap(GLuint targetFBO = 0)
{
	glBindFramebuffer(GL_FRAMEBUFFER, targetFBO);
	glViewport(0, 0, gWidth, gHeight);

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

	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glDrawArrays(GL_TRIANGLES, 0, 36);

	glBindVertexArray(0);
	glUseProgram(0);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glDepthFunc(GL_LESS);
}

void drawDeferredLighting(GLuint lightFBO = 0)
{
	geometryPass();
	glBindFramebuffer(GL_FRAMEBUFFER, lightFBO);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	glUseProgram(deferredLightingShaderProgram);
	glBindVertexArray(quadVAO);

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, gPosition);
	GLint gPosLoc = glGetUniformLocation(deferredLightingShaderProgram, "gPosition");
	glUniform1i(gPosLoc, 0);

	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, gNormal);
	GLint gNormLoc = glGetUniformLocation(deferredLightingShaderProgram, "gNormal");
	glUniform1i(gNormLoc, 1);

	GLint viewPosLoc = glGetUniformLocation(deferredLightingShaderProgram, "viewPos");
	GLint exposureLoc = glGetUniformLocation(deferredLightingShaderProgram, "exposure");

	glUniform3fv(viewPosLoc, 1, glm::value_ptr(eyePos));
	glUniform1f(exposureLoc, exposure);

	glDrawArrays(GL_TRIANGLES, 0, 6);

	glBindVertexArray(0);
	glUseProgram(0);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}
void drawComposite(unsigned int outputFBO = 0)
{

	drawCubemap(cubemapFBO);
	drawDeferredLighting(lightFBO);
	blitTo(cubemapFBO, outputFBO);
	glBindFramebuffer(GL_FRAMEBUFFER, outputFBO);

	glUseProgram(compositeShaderProgram);
	glBindVertexArray(quadVAO);

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, lightColorTex);
	glUniform1i(glGetUniformLocation(compositeShaderProgram, "lightingTexture"), 0);

	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, gPosition);
	glUniform1i(glGetUniformLocation(compositeShaderProgram, "gPosition"), 1);

	glActiveTexture(GL_TEXTURE2);
	glBindTexture(GL_TEXTURE_2D, gNormal);
	glUniform1i(glGetUniformLocation(compositeShaderProgram, "gNormal"), 2);

	glActiveTexture(GL_TEXTURE3);
	glBindTexture(GL_TEXTURE_CUBE_MAP, cubemapTexture);
	glUniform1i(glGetUniformLocation(compositeShaderProgram, "cubemapTexture"), 3);

	glUniform3fv(glGetUniformLocation(compositeShaderProgram, "viewPos"), 1, glm::value_ptr(eyePos));
	glUniform3fv(glGetUniformLocation(compositeShaderProgram, "cameraFront"), 1, glm::value_ptr(eyeGaze));

	glUniform1f(glGetUniformLocation(compositeShaderProgram, "exposure"), exposure);
	glUniform1f(glGetUniformLocation(compositeShaderProgram, "reflectionStrength"), 0.5f);

	glDrawArrays(GL_TRIANGLES, 0, 6);

	glBindVertexArray(0);
	glUseProgram(0);

	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void drawCompositeAndMotionBlur(GLuint outputFBO = 0)
{
	static double lastTime = glfwGetTime();
	double currentTime = glfwGetTime();
	float deltaTime = float(currentTime - lastTime);
	lastTime = currentTime;

	blurAmount = std::max(0.0f, blurAmount * exp(-deltaTime * 2.0f));

	drawComposite(compositeFBO);

	glUseProgram(motionBlurShader);

	glUniform1f(glGetUniformLocation(motionBlurShader, "blurAmount"), blurEnabled ? blurAmount : 0.0f);
	glm::vec2 texelSize = glm::vec2(1.0f / gWidth, 1.0f / gHeight);
	glUniform2fv(glGetUniformLocation(motionBlurShader, "texelSize"), 1, glm::value_ptr(texelSize));

	glBindFramebuffer(GL_FRAMEBUFFER, motionBlurFBO);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	renderTexture(motionBlurShader, compositeTexture, 0, motionBlurFBO);

	blitTo(motionBlurFBO, outputFBO);
}

void drawFinal()
{
}
void drawScene2()
{
	switch (renderMode)
	{
	case 0:
		drawFinal();
		break;
	case 1: // Only cubemap (no tone mapping)
		drawCubemap();
		break;
	case 2: // World positions of armadillo
		drawWorldPositions();
		break;
	case 3: // Normals of armadillo
		drawNormals();
		break;
	case 4: // Deferred lighting of armadillo
		drawDeferredLighting();
		break;
	case 5: // Composite: cubemap (no tone mapping) + lit armadillo
		drawComposite();
		break;
	case 6: // Composite + motion blur (no tone mapping)
		drawCompositeAndMotionBlur();
		break;
	default:
		break;
	}
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
	float xoffset = lastX - xpos;
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

	float yaw_rad = glm::radians(yaw);
	float pitch_rad = glm::radians(pitch);

	glm::quat yawQuat = glm::angleAxis(yaw_rad, glm::vec3(0.0f, 1.0f, 0.0f));
	glm::quat pitchQuat = glm::angleAxis(pitch_rad, glm::vec3(1.0f, 0.0f, 0.0f));

	glm::quat finalOrientation = yawQuat * pitchQuat;

	// Calculate front vector from quaternion
	eyeGaze = finalOrientation * glm::vec3(0.0f, 0.0f, -1.0f);

	// Calculate up vector from quaternion
	eyeUp = finalOrientation * glm::vec3(0.0f, 1.0f, 0.0f);

	float currentTime = glfwGetTime();
	float deltaTime = currentTime - lastFrameTime;
	lastFrameTime = currentTime;

	glm::vec2 currentOffset = glm::vec2(xoffset, yoffset);
	float currentVelocity = length(currentOffset);
	angularVelocity = glm::mix(angularVelocity, currentVelocity, 0.1f);
	lastMouseOffset = currentOffset;

	blurAmount = glm::clamp(pow(angularVelocity, 1.0f), 0.0f, 1.0f);
	std::
			cout
		<< angularVelocity << ",,,,,,,";
}
void renderInfo()
{
	renderText("Fps:" + to_string(fps), 0, gHeight - 25, 0.6, glm::vec3(1, 1, 0));
	renderText("Exposure:" + format_float(exposure), gWidth - 5, 1.0, 0.6, glm::vec3(1, 1, 0), true, true);
	renderText("Gamma:" + format_float(gamma_val), gWidth - 5, 25.0, 0.6, glm::vec3(1, 1, 0), true, true);
	renderText("Render mode: " + currentRenderModeStr, gWidth - 5, gHeight - 30, 0.6, glm::vec3(1, 1, 0), true, false);
	if (!currentKeyPressed.empty() && glfwGetTime() - keyPressTimer <= 0.25)
	{
		renderText(currentKeyPressed, 0, 0, 0.6, glm::vec3(1, 1, 0), false, true);
	}
	else
	{
		currentKeyPressed = "";
	}
}
void animateModel()
{
	viewingMatrix = glm::lookAt(eyePos, eyePos + eyeGaze, eyeUp);

	// Base translation
	glm::mat4 matT = glm::translate(glm::mat4(1.0f), glm::vec3(0.1, 0.1f, -6.0f));

	// Static rotations (if needed)
	glm::mat4 matRx = glm::rotate(glm::mat4(1.0f), glm::radians(-90.0f), glm::vec3(1.0f, 0.0f, 0.0f));
	glm::mat4 matRy = glm::rotate(glm::mat4(1.0f), glm::radians(-90.0f), glm::vec3(0.0f, 1.0f, 0.0f));

	// Static variables to maintain state across frames
	static float yawDeg = 0.0f;
	static float changeYaw = 1.0;

	// Define yaw limits
	const float minYaw = -180.0f;
	const float maxYaw = 180.0f;

	// Update yaw angle
	yawDeg += changeYaw;

	// Clamp yaw angle within limits
	if (yawDeg > maxYaw)
	{
		yawDeg = minYaw;
	}
	// Convert yaw angle to radians
	float yawRad = glm::radians(yawDeg);

	// Create quaternion for yaw rotation
	glm::quat yawQuat = glm::angleAxis(yawRad, glm::vec3(0.0f, 1.0f, 0.0f));
	glm::mat4 matYaw = glm::toMat4(yawQuat);

	// Combine transformations
	modelingMatrix = matT * matYaw;
}

void display()
{
	glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	animateModel();
	drawScene2();

	// Draw text
	glDisable(GL_DEPTH_TEST);
	renderInfo();
	glEnable(GL_DEPTH_TEST);
}
void handleReshape()
{
	std::vector<GLuint> textures = {gPosition, gNormal, cubemapColorTex, lightColorTex, motionBlurTexture, toneMapTexture, compositeTexture};

	std::vector<GLuint> renderbuffers = {depthRBO, cubemapDepthRBO, lightDepthRBO};

	std::vector<GLuint> framebuffers = {gBuffer, cubemapFBO, lightFBO, motionBlurFBO, toneMapFBO, compositeFBO};

	for (GLuint tex : textures)
		if (glIsTexture(tex))
			glDeleteTextures(1, &tex);

	for (GLuint rbo : renderbuffers)
		if (glIsRenderbuffer(rbo))
			glDeleteRenderbuffers(1, &rbo);

	for (GLuint fbo : framebuffers)
		if (glIsFramebuffer(fbo))
			glDeleteFramebuffers(1, &fbo);

	// Reinitialize all necessary resources after resizing
	initCubemapFBO();
	initGBuffer();
	initLightingFBO();
	initMotionBlurAndToneMapping();
	initComposite();
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
	bool keypress = true;
	if (key == GLFW_KEY_Q && action == GLFW_PRESS)
	{
		glfwSetWindowShouldClose(window, GLFW_TRUE);
	}
	else if (key == GLFW_KEY_K)
	{
		if (action == GLFW_PRESS)
		{
			middleMousePressed = !middleMousePressed;
			firstMouse = true;
		}
	}
	else if (key == GLFW_KEY_UP && (action == GLFW_PRESS || action == GLFW_REPEAT))
	{
		exposure = min(128.0f, exposure * 2.0f);
	}
	else if (key == GLFW_KEY_DOWN && (action == GLFW_PRESS || action == GLFW_REPEAT))
	{
		exposure = max(1.0f, exposure / 2.0f);
	}
	else if (key == GLFW_KEY_V && action == GLFW_PRESS)
	{
		vsync = !vsync;
	}
	else if (key == GLFW_KEY_B && action == GLFW_PRESS)
	{
		blurEnabled = !blurEnabled;
		if (blurEnabled)
			blurAmount = 0.0f;
	}

	else if (action == GLFW_PRESS)
	{
		switch (key)
		{
		case GLFW_KEY_0:
			renderMode = 0;
			currentRenderModeStr = "Tonemapped";
			break;
		case GLFW_KEY_1:
			renderMode = 1;
			currentRenderModeStr = "Cubemap only";
			break;
		case GLFW_KEY_2:
			renderMode = 2;
			currentRenderModeStr = "World positions";
			break;
		case GLFW_KEY_3:
			renderMode = 3;
			currentRenderModeStr = "Normals";
			break;
		case GLFW_KEY_4:
			renderMode = 4;
			currentRenderModeStr = "Deferred lighting";
			break;
		case GLFW_KEY_5:
			renderMode = 5;
			currentRenderModeStr = "Composite (cubemap + model)";
			break;
		case GLFW_KEY_6:
			renderMode = 6;
			currentRenderModeStr = "Composite + motion blur";
			blurAmount = 0.0f;
			break;
		default:
			keypress = false;
			break;
		}
	}
	else
	{
		keypress = false;
	}

	if (keypress)
	{
		const char *name = glfwGetKeyName(key, 0);
		if (name == nullptr)
			currentKeyPressed = "azd";
		else
			currentKeyPressed = std::string(name);
		keyPressTimer = glfwGetTime();
	}
}

void mainLoop(GLFWwindow *window)
{
	int currentWidth = gWidth;
	int currentHeight = gHeight;

	while (!glfwWindowShouldClose(window))
	{
		// Check if window was resized
		int newWidth, newHeight;
		glfwGetWindowSize(window, &newWidth, &newHeight);

		// Render your scene
		glfwSwapInterval(vsync ? 1 : 0);
		display();
		calculateFPS();

		// Swap buffers
		glfwSwapBuffers(window);
		if (newWidth != currentWidth || newHeight != currentHeight)
		{
			currentWidth = newWidth;
			currentHeight = newHeight;
			gWidth = currentWidth;
			gHeight = currentHeight;

			// Handle the resize
			handleReshape();
		}

		// Poll events (this includes resize events)
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
