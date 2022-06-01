#define STB_IMAGE_IMPLEMENTATION
#define _CRT_SECURE_NO_WARNINGS

#include <iostream>
#include <GL/glew.h>
#include <SFML/Window.hpp>
#include <stdio.h>
#include <math.h>
#include <string>
#include <conio.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <SFML/System/Time.hpp>
#include "stb_image.h"
#include <cstdio>

#define M_PI       3.14159265358979323846

// Kody shaderów
const GLchar* vertexSource = R"glsl(
#version 150 core
in vec3 position;
in vec3 color;
in vec2 aTexCoord;
out vec2 TexCoord;
out vec3 Color;
in vec3 aNormal;
out vec3 Normal;
out vec3 FragPos;
uniform mat4 model;
uniform mat4 view;
uniform mat4 proj; 
void main(){
Color = color;

gl_Position = proj * view * model * vec4(position, 1.0);

Normal = aNormal;

TexCoord = aTexCoord;
}
)glsl";

const GLchar* fragmentSource = R"glsl(
#version 150 core
in vec3 Color;
in vec2 TexCoord;
out vec4 outColor;
out vec3 color;
in vec3 Normal;
in vec3 FragPos;
uniform vec3 lightPos;  
uniform sampler2D texture1;
uniform sampler2D texture2;
void main()
{
//outColor = vec4(Color, 1.0);
//outColor=texture(texture1, TexCoord); //bez swiatła moze zostac

float ambientStrength = 0.1; // ambient
vec3 ambientlightColor = vec3(1.0,1.0,1.0);
vec4 ambient = ambientStrength * vec4(ambientlightColor,1.0);
vec3 difflightColor = vec3(1.0,1.0,1.0);

vec3 norm = normalize(Normal);
vec3 lightDir = normalize(lightPos - FragPos);

float diff = max(dot(norm, lightDir), 0.0);
vec3 diffuse = diff * difflightColor;

outColor = (ambient+vec4(diffuse,1.0)) *  texture(texture2, TexCoord);
}
)glsl";

using namespace std;

double yaw = -90; //obrót względem osi Y
double pitch = 0; //obrót względem osi X
double lastX = 400;
double lastY = 300;
const float sensitivity = 0.1f;

bool loader(const char* path, std::vector<glm::vec3>& out_vertices, std::vector<glm::vec2>& out_uvs, std::vector<glm::vec3>& out_normals) {
	std::vector<unsigned int> vertexIndices, uvIndices, normalIndices;
	std::vector<glm::vec3> temp_vertices;
	std::vector<glm::vec2> temp_uvs;
	std::vector<glm::vec3> temp_normals;

	FILE* file = fopen(path, "r");
	if (file == NULL) {
		printf("Nie znaleziono pliku\n");
		return false;
	}

	while (1) {
		char lineHeader[128];
		int res = fscanf(file, "%s", lineHeader);
		if (res == EOF)
			break;

		if (strcmp(lineHeader, "v") == 0) {
			glm::vec3 vertex;
			fscanf(file, "%f %f %f\n", &vertex.x, &vertex.y, &vertex.z);
			temp_vertices.push_back(vertex);
		}
		else if (strcmp(lineHeader, "vt") == 0) {
			glm::vec2 uv;
			fscanf(file, "%f %f\n", &uv.x, &uv.y);
			//uv.y = -uv.y;
			temp_uvs.push_back(uv);
		}
		else if (strcmp(lineHeader, "vn") == 0) {
			glm::vec3 normal;
			fscanf(file, "%f %f %f\n", &normal.x, &normal.y, &normal.z);
			temp_normals.push_back(normal);
		}
		else if (strcmp(lineHeader, "f") == 0) {
			std::string vertex1, vertex2, vertex3;
			unsigned int vertexIndex[3], uvIndex[3], normalIndex[3];
			int matches = fscanf(file, "%d/%d/%d %d/%d/%d %d/%d/%d\n", &vertexIndex[0], &uvIndex[0], &normalIndex[0], &vertexIndex[1], &uvIndex[1], &normalIndex[1], &vertexIndex[2], &uvIndex[2], &normalIndex[2]);
			if (matches != 9) {
				printf("Plik nie pasuje do parsera\n");
				fclose(file);
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
		else {
			char stupidBuffer[1000];
			fgets(stupidBuffer, 1000, file);
		}
	}

	for (unsigned int i = 0; i < vertexIndices.size(); i++) {
		unsigned int vertexIndex = vertexIndices[i];
		unsigned int uvIndex = uvIndices[i];
		unsigned int normalIndex = normalIndices[i];

		glm::vec3 vertex = temp_vertices[vertexIndex - 1];
		glm::vec2 uv = temp_uvs[uvIndex - 1];
		glm::vec3 normal = temp_normals[normalIndex - 1];

		out_vertices.push_back(vertex);
		out_uvs.push_back(uv);
		out_normals.push_back(normal);
	}
	fclose(file);
	return true;
}

int main()
{
	//pozycje kamery
	glm::vec3 cameraPos = glm::vec3(10.0f, 10.0f, 15.0f);
	glm::vec3 cameraFront = glm::vec3(1.0f, 0.0f, -1.0f);
	glm::vec3 cameraUp = glm::vec3(0.0f, 1.0f, 0.0f);

	//model
	std::vector< glm::vec3 > vertices;
	std::vector< glm::vec2 > uvs;
	std::vector< glm::vec3 > normals;
	bool res = loader("C:\\Users\\Piotrek\\source\\repos\\Grafika\\Grafika\\rp_dennis_posed_004_100k_native1.obj", vertices, uvs, normals);

	//swiatlo
	glm::vec3 lightPos(1.0f, 1.0f, 1.2f);

	sf::ContextSettings settings;

	settings.depthBits = 24;
	settings.stencilBits = 8;
	// Okno renderingu

	sf::Window window(sf::VideoMode(1280, 960, 32), "OpenGL", sf::Style::Titlebar | sf::Style::Close, settings);

	//przechwycenie kursora myszy
	window.setMouseCursorGrabbed(true);
	window.setMouseCursorVisible(false);

	//limit fps
	window.setFramerateLimit(20);

	GLuint texture1;
	GLuint texture2;

	//glGenTextures(1, &texture1); // Generuje tekstury
	//glBindTexture(GL_TEXTURE_2D, texture1); //Ustawienie tekstury jako bieżącej (powiązanie)
	//// set the texture wrapping parameters
	//glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	//glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	//glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	//glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);

	glGenTextures(1, &texture2); // Generuje tekstury
	glBindTexture(GL_TEXTURE_2D, texture2); //Ustawienie tekstury jako bieżącej (powiązanie)
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);


	GLenum prymitywy[10];
	prymitywy[0] = GL_POINTS;
	prymitywy[1] = GL_LINES;
	prymitywy[2] = GL_LINE_STRIP;
	prymitywy[3] = GL_LINE_LOOP;
	prymitywy[4] = GL_TRIANGLES;
	prymitywy[5] = GL_TRIANGLE_STRIP;
	prymitywy[6] = GL_TRIANGLE_FAN;
	prymitywy[7] = GL_QUADS;
	prymitywy[8] = GL_QUAD_STRIP;
	prymitywy[9] = GL_POLYGON;

	GLenum zmien = prymitywy[4];

	//macierz
	glm::mat4 model = glm::mat4(1.0f);
	model = glm::rotate(model, glm::radians(0.0f), glm::vec3(1.0f, 0.0f, 1.0f));

	// Inicjalizacja GLEW
	glewExperimental = GL_TRUE;
	glewInit();
	// Utworzenie VAO (Vertex Array Object)
	GLuint vao;
	glGenVertexArrays(1, &vao);
	glBindVertexArray(vao);

	// Utworzenie VBO (Vertex Buffer Object)
	// i skopiowanie do niego danych wierzchołkowych
	GLuint vbo;
	glGenBuffers(1, &vbo);
	glBindBuffer(GL_ARRAY_BUFFER, vbo);
	glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(glm::vec3), &vertices[0], GL_STATIC_DRAW);
	
	GLuint uvbuffer;
	glGenBuffers(1, &uvbuffer);
	glBindBuffer(GL_ARRAY_BUFFER, uvbuffer);
	glBufferData(GL_ARRAY_BUFFER, uvs.size() * sizeof(glm::vec2), &uvs[0], GL_STATIC_DRAW);

	// Utworzenie i skompilowanie shadera wierzchołków
	GLuint vertexShader =
		glCreateShader(GL_VERTEX_SHADER);
	glShaderSource(vertexShader, 1, &vertexSource, NULL);
	glCompileShader(vertexShader);

	// Utworzenie i skompilowanie shadera fragmentów
	GLuint fragmentShader =
		glCreateShader(GL_FRAGMENT_SHADER);
	glShaderSource(fragmentShader, 1, &fragmentSource, NULL);
	glCompileShader(fragmentShader);

	// Zlinkowanie obu shaderów w jeden wspólny program
	GLuint shaderProgram = glCreateProgram();
	glAttachShader(shaderProgram, vertexShader);
	glAttachShader(shaderProgram, fragmentShader);
	glBindFragDataLocation(shaderProgram, 0, "outColor");
	glLinkProgram(shaderProgram);
	glUseProgram(shaderProgram);

	GLint NorAttrib = glGetAttribLocation(shaderProgram, "aNormal");
	glEnableVertexAttribArray(NorAttrib);
	glVertexAttribPointer(NorAttrib, 3, GL_FLOAT, GL_FALSE, 0, (void*)0);    //swiatlo poki co na OFFie
	
	GLint uniLightPos = glGetUniformLocation(shaderProgram, "lightPos");
	glUniform3fv(uniLightPos, 1, &lightPos[0]);

	GLint vertexCheck, fragmentCheck;
	glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &vertexCheck);
	glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &fragmentCheck);

	if (vertexCheck != GL_FALSE)
	{
		std::cout << "vertexShader poprawnie skompilowane" << std::endl;
	}
	else
	{
		GLsizei log_length = 0;
		GLchar message[1024];
		glGetShaderInfoLog(vertexShader, 1024, &log_length, message);
		std::cout << message << std::endl;
	}

	if (fragmentCheck != GL_FALSE)
	{
		std::cout << "fragmentShader poprawnie skompilowane" << std::endl;
	}
	else
	{
		GLsizei log_length = 0;
		GLchar message[1024];
		glGetShaderInfoLog(fragmentShader, 1024, &log_length, message);
		std::cout << message << std::endl;
	}

	////textura
	//int width, height, nrChannels;
	//stbi_set_flip_vertically_on_load(true); // tell stb_image.h to flip loaded texture's on the y-axis.
	//unsigned char* data = stbi_load("ball.jpg", &width, &height, &nrChannels, 0);
	//if (data)
	//{
	//	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
	//	glGenerateMipmap(GL_TEXTURE_2D);
	//}
	//else
	//{
	//	std::cout << "Failed to load texture" << std::endl;
	//}
	//stbi_image_free(data);
	//glBindTexture(GL_TEXTURE_2D, texture1);

	int width2, height2, nrChannels2;
	stbi_set_flip_vertically_on_load(true); // tell stb_image.h to flip loaded texture's on the y-axis.
	unsigned char* data2 = stbi_load("rp_dennis_posed_004_dif.jpg", &width2, &height2, &nrChannels2, 0);
	if (data2)
	{
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width2, height2, 0, GL_RGB, GL_UNSIGNED_BYTE, data2);
		glGenerateMipmap(GL_TEXTURE_2D);
	}
	else
	{
		std::cout << "Failed to load texture" << std::endl;
	}
	stbi_image_free(data2);
	glBindTexture(GL_TEXTURE_2D, texture2);

	// Specifikacja formatu danych wierzchołkowych
	//glEnableVertexAttribArray(0);
	//glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, (void*)0);

	GLint posAttrib = glGetAttribLocation(shaderProgram, "position");
	glEnableVertexAttribArray(posAttrib);
	glBindBuffer(GL_ARRAY_BUFFER, vbo);
	glVertexAttribPointer(posAttrib, 3, GL_FLOAT, GL_FALSE, 0, 0);

	GLint TexCoord = glGetAttribLocation(shaderProgram, "aTexCoord");
	glEnableVertexAttribArray(TexCoord);
	glBindBuffer(GL_ARRAY_BUFFER, uvbuffer);
	glVertexAttribPointer(TexCoord, 2, GL_FLOAT, GL_FALSE, 0, 0);

	//wysłanie do shadera
	GLint uniTrans = glGetUniformLocation(shaderProgram, "model");
	glUniformMatrix4fv(uniTrans, 1, GL_FALSE, glm::value_ptr(model));

	//macierz LookAt - macierz widoku
	glm::mat4 view;
	view = glm::lookAt(glm::vec3(0.0f, 0.0f, 3.0f),
		glm::vec3(0.0f, 0.0f, 0.0f),
		glm::vec3(0.0f, 1.0f, 0.0f));

	GLint uniView = glGetUniformLocation(shaderProgram, "view");

	//macierz projekcji - perspektywa
	glm::mat4 proj = glm::perspective(glm::radians(90.0f), 800.0f / 800.0f, 0.06f, 100.0f);
	GLint uniProj = glGetUniformLocation(shaderProgram, "proj");
	glUniformMatrix4fv(uniProj, 1, GL_FALSE, glm::value_ptr(proj));

	sf::Clock clock;
	sf::Time time;

	sf::Clock clock2;

	float obrot = 0.1f;
	// Rozpoczęcie pętli zdarzeń

	bool running = true;
	while (running) {
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		float cameraSpeed = 0.00002f * time.asMicroseconds();

		time = clock.getElapsedTime();
		clock.restart();

		sf::Time time2 = clock2.getElapsedTime();
		float fps = 1.0f / time2.asSeconds();
		window.setTitle("FPS: " + to_string(fps));
		clock2.restart().asSeconds();

		sf::Event windowEvent;
		while (window.pollEvent(windowEvent)) {
			switch (windowEvent.type) {
			case sf::Event::KeyPressed:
				switch (windowEvent.key.code)
				{
				case sf::Keyboard::Escape:
					running = false;
					break;
				case sf::Keyboard::Num0:
					zmien = prymitywy[0];
					break;
				case sf::Keyboard::Num1:
					zmien = prymitywy[1];
					break;
				case sf::Keyboard::Num2:
					zmien = prymitywy[2];
					break;
				case sf::Keyboard::Num3:
					zmien = prymitywy[3];
					break;
				case sf::Keyboard::Num4:
					zmien = prymitywy[4];
					break;
				case sf::Keyboard::Num5:
					zmien = prymitywy[5];
					break;
				case sf::Keyboard::Num6:
					zmien = prymitywy[6];
					break;
				case sf::Keyboard::Num7:
					zmien = prymitywy[7];
					break;
				case sf::Keyboard::Num8:
					zmien = prymitywy[8];
					break;
				case sf::Keyboard::Num9:
					zmien = prymitywy[9];
					break;
				case sf::Keyboard::Up:
					cameraPos += cameraSpeed * cameraFront;
					break;
				case sf::Keyboard::Left:
					//obrot -= cameraSpeed;
					//cameraFront.x = sin(obrot);  ---> z poprzednich zajęć
					//cameraFront.z = -cos(obrot);
					cameraPos -= glm::normalize(glm::cross(cameraFront, cameraUp)) * cameraSpeed;
					break;
				case sf::Keyboard::Right:
					//obrot += cameraSpeed;
					//cameraFront.x = sin(obrot);  ---> z poprzednich zajęć
					//cameraFront.z = -cos(obrot);
					cameraPos += glm::normalize(glm::cross(cameraFront, cameraUp)) * cameraSpeed;
					break;
				case sf::Keyboard::Down:
					cameraPos -= cameraSpeed * cameraFront;
					break;
				}
			case sf::Event::MouseMoved:
				sf::Vector2i localPosition = sf::Mouse::getPosition(window);

				double xoffset = localPosition.x - lastX;
				double yoffset = localPosition.y - lastY;
				lastX = localPosition.x;
				lastY = localPosition.y;

				xoffset *= sensitivity;
				yoffset *= sensitivity;
				yaw += xoffset;
				pitch -= yoffset;

				if (pitch > 89.0f)
					pitch = 89.0f;
				if (pitch < -89.0f)
					pitch = -89.0f;

				glm::vec3 front;
				front.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
				front.y = sin(glm::radians(pitch));
				front.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));
				cameraFront = glm::normalize(front);

				break;
			}
			//case sf::Event::Closed:
			//    running = false;
			//    break;

		}

		//funkcja tworząca macierz widoku - w czasie renderu
		view = glm::lookAt(cameraPos, cameraPos + cameraFront, cameraUp);
		glUniformMatrix4fv(uniView, 1, GL_FALSE, glm::value_ptr(view));

		glUseProgram(shaderProgram);

		glEnable(GL_DEPTH_TEST);
		glDepthFunc(GL_DEPTH);
		glEnable(GL_CULL_FACE);

		// Nadanie scenie koloru nb
		glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
		glClear(GL_COLOR_BUFFER_BIT);

		// Narysowanie elementu
		glDrawArrays(zmien, 0, vertices.size());

		// Wymiana buforów tylni/przedni
		window.display();
	}
	// Kasowanie programu i czyszczenie buforów
	glDeleteProgram(shaderProgram);
	glDeleteShader(fragmentShader);
	glDeleteShader(vertexShader);
	glDeleteBuffers(1, &vbo);
	glDeleteBuffers(1, &uvbuffer);
	glDeleteVertexArrays(1, &vao);
	glDeleteTextures(1, &texture1);
	glDeleteTextures(1, &texture2);
	// Zamknięcie okna renderingu
	window.close();
	return 0;
 }
