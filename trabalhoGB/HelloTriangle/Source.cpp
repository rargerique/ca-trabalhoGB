#pragma warning(disable : 4996)

using namespace std;

#include <iostream>
#include <string>
#include <assert.h>
#include <vector>
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include "Shader.h"
#include "stb_image.h"

struct Material {
	glm::vec3 ka;
	glm::vec3 kd;
	glm::vec3 ks;
	std::string map;
};


struct Obj {
	std::string path;
	glm::vec3 position;
	GLfloat rotate;
	GLfloat escale;
	std::vector< glm::vec3 > vertices;
	std::vector< glm::vec2 > uvs;
	std::vector< glm::vec3 > normals;
	GLuint VBOs[3];
	GLuint VAO;
	Material materials[5];
	int* textureBind = new int[5];
};

// Function prototypes
void readConfigFile(const char * path);
GLFWwindow* initializeEnvironment();
void loadObject(GLuint VBOs[3], GLuint VAO, std::vector< glm::vec3 > vertices, std::vector< glm::vec2 > uvs, std::vector< glm::vec3 > normals);
void loadLight(GLuint VBO, GLuint VAO);
void drawObject(Shader shader, Obj object);
void key_callback(GLFWwindow* window, int key, int scancode, int action, int mode);
void mouse_callback(GLFWwindow* window, double xpos, double ypos);
bool loadOBJ(const char * path, std::vector<glm::vec3>& out_vertices, std::vector<glm::vec2>& out_uvs, std::vector<glm::vec3>& out_normals);
void loadMaterials(const char * path, Obj *object);

// Window dimensions
const GLuint WIDTH = 800, HEIGHT = 600;

// Camera variables
glm::vec3 cameraPos = glm::vec3(0.0f, 0.0f, 0.0f);
glm::vec3 cameraFront = glm::vec3(0.0f, 0.0f, -1.0f);
glm::vec3 cameraUp = glm::vec3(0.0f, 1.0f, 0.0f);

// Object position variables
//glm::vec3 objectPositions = glm::vec3(0.0f, 0.0f, 0.0f);
//GLfloat objectRotate = 0.0f;
//GLfloat escaleObject = 1.0f;

// Light attributes
glm::vec3 lightPos(-1.2f, 2.0f, 5.0f);

// Mouse attributes
bool firstMouse = true;
float yaw = -90.0f;	// yaw is initialized to -90.0 degrees since a yaw of 0.0 results in a direction vector pointing to the right so we initially rotate a bit to the left.
float pitch = 0.0f;
float lastX = 800.0f / 2.0;
float lastY = 600.0 / 2.0;
float fov = 45.0f;

//Config file parameters
// std::string objPath;
glm::vec3 lightColor(0.0f, 0.0f, 0.0f);

//Multiple objects 
int numberOfObjs = 0;

Obj objects[20];


int main()
{
	GLFWwindow* window = initializeEnvironment();

	readConfigFile("config.txt");

	Shader lightingShader("../shaders/lighting.vs", "../shaders/lighting.frag");
	Shader lampShader("../shaders/lamp.vs", "../shaders/lamp.frag");

	for (int k = 0; k < numberOfObjs; k++) {
		// Load object
		bool res = loadOBJ(objects[k].path.c_str(), objects[k].vertices, objects[k].uvs, objects[k].normals);
		loadMaterials("Pikachu.mtl", &objects[k]);
		glGenVertexArrays(1, &objects[k].VAO);
		glGenBuffers(3, objects[k].VBOs);
		loadObject(objects[k].VBOs, objects[k].VAO, objects[k].vertices, objects[k].uvs, objects[k].normals);

		//--------------------------------------BEGIN-TEXTURE
		int i = 0;
		for each (Material mat in objects[k].materials) {
			//Texture index
			unsigned int texture;

			glGenTextures(1, &texture);
			glBindTexture(GL_TEXTURE_2D, texture); // all upcoming GL_TEXTURE_2D operations now have effect on this texture object
												   // set the texture wrapping parameters
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);	// set texture wrapping to GL_REPEAT (default wrapping method)
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
			// set texture filtering parameters
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

			// load image, create texture and generate mipmaps
			int width, height, nrChannels;
			//unsigned char *data = SOIL_load_image("../textures/wall.jpg", &width, &height, 0, SOIL_LOAD_RGB);
			unsigned char *data = stbi_load(objects[k].materials[i].map.c_str(), &width, &height, &nrChannels, 0);

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

			glBindTexture(GL_TEXTURE_2D, 0); // Unbind texture when done, so we won't accidentily mess up our texture.
			objects[k].textureBind[i] = texture;
			i++;
		}

	}

	//glActiveTexture(GL_TEXTURE0);

	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	//-------------------------------------------END-OF-TEXTURE

	// Load light
	GLuint lightVAO, lightVBO;
	glGenVertexArrays(1, &lightVAO);
	glGenBuffers(1, &lightVBO);
	loadLight(lightVBO, lightVAO);

	glBindVertexArray(0);

	while (!glfwWindowShouldClose(window))
	{
		glfwPollEvents();

		glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		for (int k = 0; k < numberOfObjs; k++) {
			// Draw object
			drawObject(lightingShader, objects[k]);

			glBindVertexArray(objects[k].VAO);
			glBindTexture(GL_TEXTURE_2D, objects[k].textureBind[k]);
			glDrawArrays(GL_TRIANGLES, 0, objects[k].uvs.size());
			glBindVertexArray(0);
		}

		// Swap the screen buffers
		glfwSwapBuffers(window);
	}

	// Properly de-allocate all resources once they've outlived their purpose
	// glDeleteVertexArrays(1, &VAO);
	//glDeleteBuffers(3, VBOs);
	glDeleteVertexArrays(1, &lightVAO);
	glDeleteBuffers(1, &lightVBO);
	glfwTerminate();
	return 0;
}

GLFWwindow* initializeEnvironment() {
	glfwInit();
	GLFWwindow* window = glfwCreateWindow(WIDTH, HEIGHT, "LearnOpenGL", nullptr, nullptr);
	glfwMakeContextCurrent(window);
	glfwSetKeyCallback(window, key_callback);
	glewExperimental = GL_TRUE;
	glewInit();

	const GLubyte* renderer = glGetString(GL_RENDERER); /* get renderer string */
	const GLubyte* version = glGetString(GL_VERSION); /* version as a string */
	cout << "Renderer: " << renderer << endl;
	cout << "OpenGL version supported " << version << endl;

	int width, height;
	glfwGetFramebufferSize(window, &width, &height);
	glViewport(0, 0, width, height);

	glEnable(GL_DEPTH_TEST);

	glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
	glfwSetCursorPosCallback(window, mouse_callback);

	return window;
}

void loadObject(GLuint VBOs[3], GLuint VAO, std::vector< glm::vec3 > vertices, std::vector< glm::vec2 > uvs, std::vector< glm::vec3 > normals) {
	glBindVertexArray(VAO);

	// Position attribute
	glBindBuffer(GL_ARRAY_BUFFER, VBOs[0]);
	glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(glm::vec3), &vertices[0], GL_STATIC_DRAW);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, (GLvoid*)0);
	glEnableVertexAttribArray(0);

	// Normals attribute
	glBindBuffer(GL_ARRAY_BUFFER, VBOs[2]);
	glBufferData(GL_ARRAY_BUFFER, normals.size() * sizeof(glm::vec3), &normals[0], GL_STATIC_DRAW);
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 0, (GLvoid*)0);
	glEnableVertexAttribArray(1);

	// Texture attribute
	glBindBuffer(GL_ARRAY_BUFFER, VBOs[1]);
	glBufferData(GL_ARRAY_BUFFER, uvs.size() * sizeof(glm::vec2), &uvs[0], GL_STATIC_DRAW);
	glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 0, (GLvoid*)0);
	glEnableVertexAttribArray(2);
}

void loadLight(GLuint VBO, GLuint VAO) {
	glBindVertexArray(VAO);
	glBindBuffer(GL_ARRAY_BUFFER, VBO);
	glEnableVertexAttribArray(0);
}

void drawObject(Shader lightingShader, Obj object) {
	// Use cooresponding shader when setting uniforms/drawing objects
	lightingShader.Use();
	// GLint objectColorLoc = glGetUniformLocation(lightingShader.Program, "objectColor");
	GLint lightColorLoc = glGetUniformLocation(lightingShader.Program, "lightColor");
	GLint lightPosLoc = glGetUniformLocation(lightingShader.Program, "lightPos");
	GLint viewPosLoc = glGetUniformLocation(lightingShader.Program, "viewPos");
	GLint kaLoc = glGetUniformLocation(lightingShader.Program, "ka");
	GLint kdLoc = glGetUniformLocation(lightingShader.Program, "kd");
	GLint ksLoc = glGetUniformLocation(lightingShader.Program, "ks");
	//glUniform3f(objectColorLoc, 1.0f, 0.5f, 0.31f);
	glUniform3f(lightColorLoc, lightColor.x, lightColor.y, lightColor.z);
	glUniform3f(lightPosLoc, lightPos.x, lightPos.y, lightPos.z);
	glUniform3f(viewPosLoc, cameraPos[0], cameraPos[1], cameraPos[2]);

	for (int j = 0; j < 5; j++) {
		glUniform1f(kaLoc, object.materials[j].ka.x);
		glUniform1f(kdLoc, object.materials[j].kd.x);
		glUniform1f(ksLoc, object.materials[j].ks.x);
	}

	// Create camera transformations
	glm::mat4 view = glm::lookAt(cameraPos, cameraPos + cameraFront, cameraUp);
	glm::mat4 projection = glm::perspective(45.0f, (GLfloat)WIDTH / (GLfloat)HEIGHT, 0.1f, 100.0f);
	glm::mat4 model;
	model = glm::translate(model, object.position);
	model = glm::rotate(model, glm::radians(object.rotate), glm::vec3(1.0f, 1.0f, 1.0f));
	model = glm::scale(model, glm::vec3(object.escale));

	// Get the uniform locations
	GLint modelLoc = glGetUniformLocation(lightingShader.Program, "model");
	GLint viewLoc = glGetUniformLocation(lightingShader.Program, "view");
	GLint projLoc = glGetUniformLocation(lightingShader.Program, "projection");

	// Pass the matrices to the shader
	glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
	glUniformMatrix4fv(projLoc, 1, GL_FALSE, glm::value_ptr(projection));
	glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));

	//glBindTexture(GL_TEXTURE_2D, texture);
	// glUniform1i(glGetUniformLocation(lightingShader.Program, "ourTexture"), 0);
}

void key_callback(GLFWwindow* window, int key, int scancode, int action, int mode)
{
	//Close window
	if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
		glfwSetWindowShouldClose(window, GL_TRUE);

	//Handle camera movimentation
	float cameraSpeed = 0.05f;
	if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
		cameraPos += cameraSpeed * cameraFront;
	if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
		cameraPos -= cameraSpeed * cameraFront;
	if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
		cameraPos -= glm::normalize(glm::cross(cameraFront, cameraUp)) * cameraSpeed;
	if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
		cameraPos += glm::normalize(glm::cross(cameraFront, cameraUp)) * cameraSpeed;

	//Handle object movimentation
	if (glfwGetKey(window, GLFW_KEY_J) == GLFW_PRESS)
		objects[0].position -= glm::normalize(glm::cross(cameraFront, cameraUp)) * cameraSpeed;
	if (glfwGetKey(window, GLFW_KEY_L) == GLFW_PRESS)
		objects[0].position += glm::normalize(glm::cross(cameraFront, cameraUp)) * cameraSpeed;
	if (glfwGetKey(window, GLFW_KEY_I) == GLFW_PRESS)
		objects[0].position += cameraSpeed * cameraFront;
	if (glfwGetKey(window, GLFW_KEY_K) == GLFW_PRESS)
		objects[0].position -= cameraSpeed * cameraFront;

	if (glfwGetKey(window, GLFW_KEY_U) == GLFW_PRESS)
		objects[0].position += 1.0f;

	if (glfwGetKey(window, GLFW_KEY_N) == GLFW_PRESS)
		objects[0].position += 0.1f;
	if (glfwGetKey(window, GLFW_KEY_M) == GLFW_PRESS)
		objects[0].position -= 0.1f;

}

void mouse_callback(GLFWwindow* window, double xpos, double ypos)
{
	if (firstMouse)
	{
		lastX = xpos;
		lastY = ypos;
		firstMouse = false;
	}

	float xoffset = xpos - lastX;
	float yoffset = lastY - ypos; // reversed since y-coordinates go from bottom to top
	lastX = xpos;
	lastY = ypos;

	float sensitivity = 0.1f; // change this value to your liking
	xoffset *= sensitivity;
	yoffset *= sensitivity;

	yaw += xoffset;
	pitch += yoffset;

	// make sure that when pitch is out of bounds, screen doesn't get flipped
	if (pitch > 89.0f)
		pitch = 89.0f;
	if (pitch < -89.0f)
		pitch = -89.0f;

	glm::vec3 front;
	front.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
	front.y = sin(glm::radians(pitch));
	front.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));
	cameraFront = glm::normalize(front);
}

bool loadOBJ(const char * path, std::vector<glm::vec3>& out_vertices, std::vector<glm::vec2>& out_uvs, std::vector<glm::vec3>& out_normals)
{
	std::vector< unsigned int > vertexIndices, uvIndices, normalIndices;
	std::vector< glm::vec3 > temp_vertices;
	std::vector< glm::vec2 > temp_uvs;
	std::vector< glm::vec3 > temp_normals;

	//opening file
	FILE * file = fopen(path, "r");
	if (file == NULL) {
		printf("Impossible to open the file !\n");
		return false;
	}

	while (1) {

		char lineHeader[128];
		// read the first word of the line
		int res = fscanf(file, "%s", lineHeader);
		if (res == EOF)
			break;

		//read the vertices
		if (strcmp(lineHeader, "v") == 0) {
			glm::vec3 vertex;
			fscanf(file, "%f %f %f\n", &vertex.x, &vertex.y, &vertex.z);
			temp_vertices.push_back(vertex);
		}
		else if (strcmp(lineHeader, "vt") == 0) { //read the texture coordinates
			glm::vec3 uv;
			fscanf(file, "%f %f\n", &uv.x, &uv.y);
			temp_uvs.push_back(uv);
		}
		else if (strcmp(lineHeader, "vn") == 0) { //read the normals
			glm::vec3 normal;
			fscanf(file, "%f %f %f\n", &normal.x, &normal.y, &normal.z);
			temp_normals.push_back(normal);
		}
		else if (strcmp(lineHeader, "f") == 0) { //read the faces
			std::string vertex1, vertex2, vertex3;
			unsigned int vertexIndex[3], uvIndex[3], normalIndex[3];
			int matches = fscanf(file, "%d/%d/%d %d/%d/%d %d/%d/%d\n", &vertexIndex[0], &uvIndex[0], &normalIndex[0], &vertexIndex[1], &uvIndex[1], &normalIndex[1], &vertexIndex[2], &uvIndex[2], &normalIndex[2]);
			if (matches != 9) {
				printf("File can't be read by our simple parser : ( Try exporting with other options\n");
				return false;
			}
			vertexIndices.push_back(vertexIndex[0]);
			vertexIndices.push_back(vertexIndex[1]);
			vertexIndices.push_back(vertexIndex[2]);
			uvIndices.push_back(uvIndex[0]);
			uvIndices.push_back(uvIndex[1]);
			uvIndices.push_back(uvIndex[2]);
			normalIndices.push_back(normalIndex[0]);
			normalIndices.push_back(normalIndex[1]);
			normalIndices.push_back(normalIndex[2]);
		}
	}

	for (unsigned int i = 0; i < vertexIndices.size(); i++) {
		unsigned int vertexIndex = vertexIndices[i];
		glm::vec3 vertex = temp_vertices[vertexIndex - 1];
		out_vertices.push_back(vertex);
	}

	for (unsigned int i = 0; i < uvIndices.size(); i++) {
		unsigned int uvIndex = uvIndices[i];
		glm::vec2 uv = temp_uvs[uvIndex - 1];
		out_uvs.push_back(uv);
	}

	for (unsigned int i = 0; i < normalIndices.size(); i++) {
		unsigned int normalIndex = normalIndices[i];
		glm::vec3 normal = temp_normals[normalIndex - 1];
		out_normals.push_back(normal);
	}
}

void loadMaterials(const char * path, Obj *object) {
	//opening file
	FILE * file = fopen(path, "r");
	int matIndex = 0;

	while (1) {

		char lineHeader[128];
		// read the first word of the line
		int res = fscanf(file, "%s", lineHeader);
		if (res == EOF)
			break;

		//read the ka
		if (strcmp(lineHeader, "Ka") == 0) {
			glm::vec3 ka;
			fscanf(file, "%f %f %f\n", &ka.x, &ka.y, &ka.z);
			object->materials[matIndex].ka = ka;
		}
		else if (strcmp(lineHeader, "Kd") == 0) { //read the kd
			glm::vec3 kd;
			fscanf(file, "%f %f %f\n", &kd.x, &kd.y, &kd.z);
			object->materials[matIndex].kd = kd;
		}
		else if (strcmp(lineHeader, "Ks") == 0) { //read the ks
			glm::vec3 ks;
			fscanf(file, "%f %f %f\n", &ks.x, &ks.y, &ks.z);
			object->materials[matIndex].ks = ks;
		}
		else if (strcmp(lineHeader, "map_Kd") == 0) { //read the map 
			char map[128];
			fscanf(file, "%s", &map);
			object->materials[matIndex].map = map;
			matIndex++;
		}
	}
}

void readConfigFile(const char * path) {
	//opening file
	FILE * file = fopen(path, "r");

	while (1) {

		char lineHeader[128];
		// read the first word of the line
		int res = fscanf(file, "%s", lineHeader);
		if (res == EOF)
			break;

		//read the location of the obj file
		if (strcmp(lineHeader, "filename") == 0) {
			char map[128];
			fscanf(file, "%s", &map);
			objects[numberOfObjs].path = map;
		}
		else if (strcmp(lineHeader, "positionx") == 0) {
			fscanf(file, "%f", &objects[numberOfObjs].position.x);
		}
		else if (strcmp(lineHeader, "positiony") == 0) {
			fscanf(file, "%f", &objects[numberOfObjs].position.y);
		}
		else if (strcmp(lineHeader, "positionz") == 0) {
			fscanf(file, "%f", &objects[numberOfObjs].position.z);
		}
		else if (strcmp(lineHeader, "rotation") == 0) {
			fscanf(file, "%f", &objects[numberOfObjs].rotate);
		}
		else if (strcmp(lineHeader, "escale") == 0) {
			fscanf(file, "%f", &objects[numberOfObjs].escale);
			numberOfObjs++;
		}
		else if (strcmp(lineHeader, "lightcolorx") == 0) {
			fscanf(file, "%f", &lightColor.x);
		}
		else if (strcmp(lineHeader, "lightcolory") == 0) {
			fscanf(file, "%f", &lightColor.y);
		}
		else if (strcmp(lineHeader, "lightcolorz") == 0) {
			fscanf(file, "%f", &lightColor.z);
		}
		else if (strcmp(lineHeader, "cameraposx") == 0) {
			fscanf(file, "%f", &cameraPos.x);
		}
		else if (strcmp(lineHeader, "cameraposy") == 0) {
			fscanf(file, "%f", &cameraPos.y);
		}
		else if (strcmp(lineHeader, "cameraposz") == 0) {
			fscanf(file, "%f", &cameraPos.z);
		}
	}
}