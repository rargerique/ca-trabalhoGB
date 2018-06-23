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

// Function prototypes
GLFWwindow* initializeEnvironment();
void loadObject(GLuint VBOs[3], GLuint VAO, std::vector< glm::vec3 > vertices, std::vector< glm::vec3 > uvs, std::vector< glm::vec3 > normals);
void loadLight(GLuint VBO, GLuint VAO);
void drawObject(Shader shader);
void key_callback(GLFWwindow* window, int key, int scancode, int action, int mode);
void mouse_callback(GLFWwindow* window, double xpos, double ypos);
bool loadOBJ(const char * path, std::vector<glm::vec3>& out_vertices, std::vector<glm::vec3>& out_uvs, std::vector<glm::vec3>& out_normals);

// Window dimensions
const GLuint WIDTH = 800, HEIGHT = 600;

// Camera variables
glm::vec3 cameraPos = glm::vec3(0.0f, 3.0f, 10.0f);
glm::vec3 cameraFront = glm::vec3(0.0f, 0.0f, -1.0f);
glm::vec3 cameraUp = glm::vec3(0.0f, 1.0f, 0.0f);

// Object position variables
glm::vec3 objectPositions = glm::vec3(0.0f, 0.0f, 0.0f);
GLfloat objectRotate = 0.0f;
GLfloat escaleObject = 1.0f;

// Light attributes
glm::vec3 lightPos(-1.2f, 2.0f, 5.0f);

// Mouse attributes
bool firstMouse = true;
float yaw = -90.0f;	// yaw is initialized to -90.0 degrees since a yaw of 0.0 results in a direction vector pointing to the right so we initially rotate a bit to the left.
float pitch = 0.0f;
float lastX = 800.0f / 2.0;
float lastY = 600.0 / 2.0;
float fov = 45.0f;

//Texture index
unsigned int texture;

int main()
{
	GLFWwindow* window = initializeEnvironment();

	Shader lightingShader("../shaders/lighting.vs", "../shaders/lighting.frag");
	Shader lampShader("../shaders/lamp.vs", "../shaders/lamp.frag");

	// Load first object
	std::vector< glm::vec3 > vertices, uvs, normals;
	bool res = loadOBJ("Charmander.obj", vertices, uvs, normals);
	GLuint VBOs[3], VAO;
	glGenVertexArrays(1, &VAO);
	glGenBuffers(3, VBOs);
	loadObject(VBOs, VAO, vertices, uvs, normals);

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

		// Draw object
		drawObject(lightingShader);
		glBindVertexArray(VAO);
		glDrawArrays(GL_TRIANGLES, 0, vertices.size());
		glBindVertexArray(0);

		// Swap the screen buffers
		glfwSwapBuffers(window);
	}

	// Properly de-allocate all resources once they've outlived their purpose
	glDeleteVertexArrays(1, &VAO);
	glDeleteBuffers(3, VBOs);
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

void loadObject(GLuint VBOs[3], GLuint VAO, std::vector< glm::vec3 > vertices, std::vector< glm::vec3 > uvs, std::vector< glm::vec3 > normals) {
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
	glBufferData(GL_ARRAY_BUFFER, uvs.size() * sizeof(glm::vec3), &uvs[0], GL_STATIC_DRAW);
	glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, 0, (GLvoid*)0);
	glEnableVertexAttribArray(2);

	// Texture
	// load and create a texture 
	// -------------------------
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
	unsigned char *data = stbi_load("../textures/Charmander.png", &width, &height, &nrChannels, 0);

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

	glActiveTexture(GL_TEXTURE0);

	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
}

void loadLight(GLuint VBO, GLuint VAO) {
	glBindVertexArray(VAO);
	glBindBuffer(GL_ARRAY_BUFFER, VBO);
	glEnableVertexAttribArray(0);
}

void drawObject(Shader lightingShader) {
	// Use cooresponding shader when setting uniforms/drawing objects
	lightingShader.Use();
	GLint objectColorLoc = glGetUniformLocation(lightingShader.Program, "objectColor");
	GLint lightColorLoc = glGetUniformLocation(lightingShader.Program, "lightColor");
	GLint lightPosLoc = glGetUniformLocation(lightingShader.Program, "lightPos");
	GLint viewPosLoc = glGetUniformLocation(lightingShader.Program, "viewPos");
	glUniform3f(objectColorLoc, 1.0f, 0.5f, 0.31f);
	glUniform3f(lightColorLoc, 5.0f, 1.0f, 1.0f);
	glUniform3f(lightPosLoc, lightPos.x, lightPos.y, lightPos.z);
	glUniform3f(viewPosLoc, cameraPos[0], cameraPos[1], cameraPos[2]);

	// Create camera transformations
	glm::mat4 view = glm::lookAt(cameraPos, cameraPos + cameraFront, cameraUp);
	glm::mat4 projection = glm::perspective(45.0f, (GLfloat)WIDTH / (GLfloat)HEIGHT, 0.1f, 100.0f);
	glm::mat4 model;
	model = glm::translate(model, objectPositions);
	model = glm::rotate(model, glm::radians(objectRotate), glm::vec3(1.0f, 1.0f, 1.0f));
	model = glm::scale(model, glm::vec3(escaleObject));

	// Get the uniform locations
	GLint modelLoc = glGetUniformLocation(lightingShader.Program, "model");
	GLint viewLoc = glGetUniformLocation(lightingShader.Program, "view");
	GLint projLoc = glGetUniformLocation(lightingShader.Program, "projection");

	// Pass the matrices to the shader
	glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
	glUniformMatrix4fv(projLoc, 1, GL_FALSE, glm::value_ptr(projection));
	glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));

	glBindTexture(GL_TEXTURE_2D, texture);
	glUniform1i(glGetUniformLocation(lightingShader.Program, "ourTexture"), 0);
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
		objectPositions -= glm::normalize(glm::cross(cameraFront, cameraUp)) * cameraSpeed;
	if (glfwGetKey(window, GLFW_KEY_L) == GLFW_PRESS)
		objectPositions += glm::normalize(glm::cross(cameraFront, cameraUp)) * cameraSpeed;
	if (glfwGetKey(window, GLFW_KEY_I) == GLFW_PRESS)
		objectPositions += cameraSpeed * cameraFront;
	if (glfwGetKey(window, GLFW_KEY_K) == GLFW_PRESS)
		objectPositions -= cameraSpeed * cameraFront;

	if (glfwGetKey(window, GLFW_KEY_U) == GLFW_PRESS)
		 objectRotate += 1.0f;

	if (glfwGetKey(window, GLFW_KEY_N) == GLFW_PRESS)
		escaleObject += 0.1f;
	if (glfwGetKey(window, GLFW_KEY_M) == GLFW_PRESS)
		escaleObject -= 0.1f;

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

bool loadOBJ(const char * path, std::vector<glm::vec3>& out_vertices, std::vector<glm::vec3>& out_uvs, std::vector<glm::vec3>& out_normals)
{
	std::vector< unsigned int > vertexIndices, uvIndices, normalIndices;
	std::vector< glm::vec3 > temp_vertices;
	std::vector< glm::vec3 > temp_uvs;
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
		glm::vec3 uv = temp_uvs[uvIndex - 1];
		out_uvs.push_back(uv);
	}

	for (unsigned int i = 0; i < normalIndices.size(); i++) {
		unsigned int normalIndex = normalIndices[i];
		glm::vec3 normal = temp_normals[normalIndex - 1];
		out_normals.push_back(normal);
	}
}