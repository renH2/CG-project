#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <iostream>
#include <vector>
#include <string>
#include <math.h>
#include <fstream>
#include <sstream>
#include <random>
#include "camera.h"
#include "shader.h"
#include "model.h"
#include "filesystem.h"
#include "global.h"
#include "monster.h"
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
using namespace std;

//全局变量
float lastX = (float)SCR_WIDTH / 2.0f;
float lastY = (float)SCR_HEIGHT / 2.0f;
bool firstMouse = true;
float deltaTime = 0.0f;
float lastFrame = 0.0f;
bool dirlight = true;
bool haveParticle = true;
bool isParticle = false;
bool isMagic = false;
bool isNight = false;
bool isShadow = false;
bool ishdr = false;
bool isgamma = false;

//游戏视角，可在游戏中切换（FPV - 第一视角；GPV -上帝视角；TPV-第三视角）
unsigned int view_type = FPV;

unsigned int quadVAO = 0;
unsigned int quadVBO;
unsigned int ScreenVAO = 0;
unsigned int ScreenVBO;
unsigned int cVAO = 0;
unsigned int cVBO = 0;
unsigned int planeVAO;

int state = PIC1;
int monsterCount = 0;

Particle p;
Magic ma;

//相机位置
Camera camera(glm::vec3(initX, initY + 1.5f, initZ));
Monster* monsterVector[5];
//存放从map文件中读取的位置信息
std::vector<glm::vec3> boxPosition;
glm::vec3 cupPosition;
std::vector<glm::vec3>monsterPosition;

struct Magic initMagic(glm::vec3 position, GLuint lifespan, glm::vec3 front, glm::vec3 color, float eps);
void UpdateMagic(glm::vec3 time, Magic &p);
struct Particle initParticle(glm::vec3 position, GLuint lifespan, int num, glm::vec3 color, float eps);
void UpdateParticle(glm::vec3 time, Particle &p);
void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void mouse_callback(GLFWwindow* window, double xpos, double ypos);
void mouse_button_callback(GLFWwindow* window, int button, int action, int mod);
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset);
void processInput(GLFWwindow *window, Camera& mypos, Camera& camera);
bool CollisionDetection(glm::vec3& position, float width, float height);
bool CollisionDetection(glm::vec3& position, float radius);
void CreateMap(const std::string filename);
unsigned int CreateSkybox(int id);
unsigned int loadCubemap(std::vector<std::string> faces);
unsigned int loadTexture(char const* path);
float randFloat(float a, float b);
int randInt(int a, int b);
void renderQuad();
void renderScene(const Shader &shader);
void renderScreen();
void renderCube();


int main()
{
	//初始化并配置版本信息
	glfwInit();
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
	//创建并初始化窗口
	GLFWwindow* window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "Maze", NULL, NULL);
	if (window == NULL)
	{
		std::cout << "Failed to create GLFW window" << std::endl;
		glfwTerminate();
		return -1;
	}
	glfwMakeContextCurrent(window);
	//窗口、鼠标、键盘等事件响应
	glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
	glfwSetCursorPosCallback(window, mouse_callback);
	glfwSetScrollCallback(window, scroll_callback);
	glfwSetMouseButtonCallback(window, mouse_button_callback);
	//设置光标模式为 只能在窗口内且被隐藏
	glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
	//初始化glad管理指针
	if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
	{
		std::cout << "Failed to initialize GLAD" << std::endl;
		return -1;
	}

	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LEQUAL);

	//主角模型的位置
	Camera myPos = (glm::vec3(initX, initY, initZ));

	//读取地图数据并创建
	CreateMap("resource/map/diff_map.txt");

	//读取shader文件
	Shader BoxShader("resource/vs/box.vs", "resource/fs/box.fs");
	Shader  newBoxShader("resource/vs/newbox.vs", "resource/fs/newbox.fs");
	Shader LightShader("resource/vs/lightcube.vs", "resource/fs/lightcube.fs");
	Shader SkyboxShader("resource/vs/skybox.vs", "resource/fs/skybox.fs");
	Shader PlaneShader("resource/vs/plane.vs", "resource/fs/plane.fs");
	Shader hdrShader("resource/vs/hdr.vs", "resource/fs/hdr.fs");
	Shader InterfaceShader("resource/vs/interface.vs", "resource/fs/interface.fs");
	Shader DepthShader("resource/vs/depth.vs", "resource/fs/depth.fs");

	//读入模型obj文件
	Model TrophyCup(FileSystem::getPath("resource/obj/cup/cup.obj"));
	Model HarryPotter(FileSystem::getPath("resource/obj/characters/harry.obj"));
	Model StoneMan(FileSystem::getPath("resource/obj/monsters/stoneman/Stone.obj"));
	Model Ogre(FileSystem::getPath("resource/obj/monsters/ogre/OgreOBJ.obj"));

	//生成纹理
	unsigned int  SkyboxTexture = CreateSkybox(0);
	unsigned int  OtherTexture = CreateSkybox(1);
	unsigned int planeTexture = loadTexture("resource/material/ground2.jpg");
	unsigned int cupTexture = loadTexture("resource/material/gold2.png");
	unsigned int plantTexture = loadTexture("resource/material/plant5.png");
	unsigned int diffuse = loadTexture("resource/planet3/diffuse.jpg");
	unsigned int specular = loadTexture("resource/planet3/specular.png");
	unsigned int interface1 = loadTexture("resource/interface/interface1.bmp");
	unsigned int interface2 = loadTexture("resource/interface/interface2.bmp");

	//绑定cube数组的坐标、法向量、纹理坐标
	unsigned int cubeVBO, cubeVAO;
	glGenVertexArrays(1, &cubeVAO);
	glGenBuffers(1, &cubeVBO);
	glBindBuffer(GL_ARRAY_BUFFER, cubeVBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(BoxVertices), BoxVertices, GL_STATIC_DRAW);
	glBindVertexArray(cubeVAO);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));
	glEnableVertexAttribArray(1);
	glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(6 * sizeof(float)));
	glEnableVertexAttribArray(2);

	//绑定单个light cube坐标信息
	unsigned int lightCubeVAO;
	glGenVertexArrays(1, &lightCubeVAO);
	glBindVertexArray(lightCubeVAO);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
	glEnableVertexAttribArray(0);

	//绑定地板plane数组的坐标、法向量、纹理坐标
	unsigned int planeVBO;
	glGenVertexArrays(1, &planeVAO);
	glGenBuffers(1, &planeVBO);
	glBindBuffer(GL_ARRAY_BUFFER, planeVBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(planeVertices), planeVertices, GL_STATIC_DRAW);
	glBindVertexArray(planeVAO);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));
	glEnableVertexAttribArray(1);
	glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(6 * sizeof(float)));
	glEnableVertexAttribArray(2);

	//绑定天空盒数组的坐标
	unsigned int skyboxVAO, skyboxVBO;
	glGenVertexArrays(1, &skyboxVAO);
	glGenBuffers(1, &skyboxVBO);
	glBindBuffer(GL_ARRAY_BUFFER, skyboxVBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(SkyBoxVertices), &SkyBoxVertices, GL_STATIC_DRAW);
	glBindVertexArray(skyboxVAO);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);

	unsigned int hdrFBO, colorBuffer;
	glGenFramebuffers(1, &hdrFBO);
	glGenTextures(1, &colorBuffer);
	glBindTexture(GL_TEXTURE_2D, colorBuffer);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, SCR_WIDTH, SCR_HEIGHT, 0, GL_RGBA, GL_FLOAT, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	unsigned int rboDepth;
	glGenRenderbuffers(1, &rboDepth);
	glBindRenderbuffer(GL_RENDERBUFFER, rboDepth);
	glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT, SCR_WIDTH, SCR_HEIGHT);
	glBindFramebuffer(GL_FRAMEBUFFER, hdrFBO);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, colorBuffer, 0);
	glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, rboDepth);
	if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
		std::cout << "Framebuffer not complete!" << std::endl;
	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	//阴影shader
	unsigned int depthMapFBO, depthMap;
	glGenFramebuffers(1, &depthMapFBO);
	glGenTextures(1, &depthMap);
	glBindTexture(GL_TEXTURE_2D, depthMap);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, SHADOW_WIDTH, SHADOW_HEIGHT, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
	float borderColor[] = { 1.0, 1.0, 1.0, 1.0 };
	glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, borderColor);
	glBindFramebuffer(GL_FRAMEBUFFER, depthMapFBO);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, depthMap, 0);
	glDrawBuffer(GL_NONE);
	glReadBuffer(GL_NONE);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);


	//周期循环检测、绘制
	while (!glfwWindowShouldClose(window))
	{
		glm::mat4 projection = glm::perspective(glm::radians(camera.Zoom), (float)SCR_WIDTH / (float)SCR_HEIGHT, 0.1f, 100.0f);
		glm::mat4 view = camera.GetViewMatrix();
		glm::mat4 model = glm::mat4(1.0f);
		glm::mat4 lightProjection, lightView;
		glm::mat4 lightSpaceMatrix;

		//响应设备输入信号
		processInput(window, myPos, camera);
		//初始化位深和色深信息
		glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		if (state == PIC1 || state == PIC2)
		{
			InterfaceShader.use();
			InterfaceShader.setInt("pic", 0);
			glActiveTexture(GL_TEXTURE0);
			switch (state)
			{
			case PIC1:
				glBindTexture(GL_TEXTURE_2D, interface1);
				break;
			case PIC2:
				glBindTexture(GL_TEXTURE_2D, interface2);
				break;
			}
			renderScreen();

			glfwSwapBuffers(window);
			glfwPollEvents();
			continue;
		}


		if (isShadow == true)
		{
			glm::vec3 lightPos(-3.0f, -1.0f, -3.0f);
			float near_plane = 1.0f, far_plane = 17.5f;
			lightProjection = glm::ortho(-10.0f, 10.0f, -10.0f, 10.0f, near_plane, far_plane);
		//	lightProjection = glm::perspective(glm::radians(45.0f), (GLfloat)SHADOW_WIDTH / (GLfloat)SHADOW_HEIGHT, near_plane, far_plane);
			lightView = glm::lookAt(camera.Position, camera.Position + camera.Front, camera.Up);
			lightSpaceMatrix = lightProjection * lightView;
			DepthShader.use();
			DepthShader.setMat4("lightSpaceMatrix", lightSpaceMatrix);
			glViewport(0, 0, SHADOW_WIDTH, SHADOW_HEIGHT);
			glBindFramebuffer(GL_FRAMEBUFFER, depthMapFBO);
			glClear(GL_DEPTH_BUFFER_BIT);
			glActiveTexture(GL_TEXTURE0);
			glBindTexture(GL_TEXTURE_2D, diffuse);
			renderScene(DepthShader);
			DepthShader.use();
			for (unsigned int i = 0; i < monsterCount; i++)
			{
				if (monsterVector[i]->type != 1) continue;
				glm::mat4 StoneManModel = glm::mat4(1.0f);
				StoneManModel = glm::translate(StoneManModel, monsterVector[i]->position);
				StoneManModel = glm::scale(StoneManModel, glm::vec3(monsterVector[i]->zoomRate, monsterVector[i]->zoomRate, monsterVector[i]->zoomRate));
				StoneManModel = glm::rotate(StoneManModel, monsterVector[i]->rotateAngle, glm::vec3(0.0f, 1.0f, 0.0f));
				DepthShader.setMat4("model", StoneManModel);
				StoneMan.Draw(DepthShader);
			}

			for (unsigned int i = 0; i < monsterCount; i++)
			{
				if (monsterVector[i]->type != 2) continue;
				glm::mat4 OgreModel = glm::mat4(1.0f);
				OgreModel = glm::translate(OgreModel, monsterVector[i]->position);
				OgreModel = glm::scale(OgreModel, glm::vec3(monsterVector[i]->zoomRate, monsterVector[i]->zoomRate, monsterVector[i]->zoomRate));
				OgreModel = glm::rotate(OgreModel, monsterVector[i]->rotateAngle, glm::vec3(0.0f, 1.0f, 0.0f));
				DepthShader.setMat4("model", OgreModel);
				Ogre.Draw(DepthShader);
			}
			glm::mat4 CupModel = glm::mat4(1.0f);
			CupModel = glm::translate(CupModel, cupPosition);
			CupModel = glm::scale(CupModel, glm::vec3(0.01f, 0.01f, 0.01f));
			DepthShader.setMat4("model", CupModel);
			TrophyCup.Draw(DepthShader);

			glm::mat4 HarryModel = glm::mat4(1.0f);
			HarryModel = glm::translate(HarryModel, myPos.Position);
			HarryModel = glm::scale(HarryModel, glm::vec3(0.025f, 0.025f, 0.025f));
			HarryModel = glm::rotate(HarryModel, glm::radians(90.0f), glm::vec3(0.0f, 1.0f, 0.0f));
			HarryModel = glm::rotate(HarryModel, glm::radians(90.0f), glm::vec3(-1.0f, 0.0f, 0.0f));
			HarryModel = glm::rotate(HarryModel, glm::radians(180.0f), glm::vec3(0.0f, 0.0f, 1.0f));
			HarryModel = glm::rotate(HarryModel, myPos.Yaw / 57.0f, glm::vec3(0.0f, 0.0f, -1.0f));
			DepthShader.setMat4("model", HarryModel);
			HarryPotter.Draw(DepthShader);

			glBindFramebuffer(GL_FRAMEBUFFER, 0);
			glViewport(0, 0, SCR_WIDTH, SCR_HEIGHT);
			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		}

		glBindFramebuffer(GL_FRAMEBUFFER, hdrFBO);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		//火焰杯模型绘制
		BoxShader.use();
		//平行光相关参数
		BoxShader.setInt("dir", dirlight);
		BoxShader.setVec3("dirLight.direction", -0.2f, -1.0f, -0.3f);
		BoxShader.setVec3("dirLight.ambient", 0.75f, 0.75f, 0.75f);
		BoxShader.setVec3("dirLight.diffuse", 0.8f, 0.8f, 0.8f);
		BoxShader.setVec3("dirLight.specular", 0.8f, 0.8f, 0.8f);
		//材质相关参数
		BoxShader.setInt("material.diffuse", 0);
		BoxShader.setInt("material.specular", 1);
		BoxShader.setInt("material.shadowMap", 2);
		BoxShader.setFloat("material.shininess", 32.0f);
		//点光源相关参数
		BoxShader.setVec3("pointLights[0].position", myPos.Position);
		BoxShader.setVec3("pointLights[0].ambient", 0.9f, 0.90f, 0.5f);
		BoxShader.setVec3("pointLights[0].diffuse", 0.85f, 0.850f, 0.35f);
		BoxShader.setVec3("pointLights[0].specular", 0.80f, 0.80f, 0.40f);
		BoxShader.setFloat("pointLights[0].constant", 1.0f);
		BoxShader.setFloat("pointLights[0].linear", 0.09);
		BoxShader.setFloat("pointLights[0].quadratic", 0.032);
		//矩阵变换相关参数
		BoxShader.setMat4("projection", projection);
		BoxShader.setMat4("view", view);
		BoxShader.setVec3("viewPos", camera.Position);
		BoxShader.setVec3("lightPos", camera.Position);
		BoxShader.setMat4("lightSpaceMatrix", lightSpaceMatrix);
		glm::mat4 CupModel = glm::mat4(1.0f);
		CupModel = glm::translate(CupModel, cupPosition);
		CupModel = glm::scale(CupModel, glm::vec3(0.01f, 0.01f, 0.01f));
		BoxShader.setMat4("model", CupModel);
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, cupTexture);
		TrophyCup.Draw(BoxShader);

		//第一人称视角时不显示模型
		if (view_type == GPV || view_type == TPV)
		{
			//哈利波特模型绘制
			BoxShader.use();
			//平行光相关参数
			BoxShader.setInt("dir", dirlight);
			BoxShader.setVec3("dirLight.direction", -0.2f, -1.0f, -0.3f);
			BoxShader.setVec3("dirLight.ambient", 0.75f, 0.75f, 0.75f);
			BoxShader.setVec3("dirLight.diffuse", 0.8f, 0.8f, 0.8f);
			BoxShader.setVec3("dirLight.specular", 0.8f, 0.8f, 0.8f);
			//材质相关参数
			BoxShader.setInt("material.diffuse", 0);
			BoxShader.setInt("material.specular", 1);
			BoxShader.setInt("material.shadowMap", 2);
			BoxShader.setFloat("material.shininess", 32.0f);
			//点光源相关参数
			BoxShader.setVec3("pointLights[0].position", myPos.Position);
			BoxShader.setVec3("pointLights[0].ambient", 0.6f, 0.60f, 0.6f);
			BoxShader.setVec3("pointLights[0].diffuse", 0.55f, 0.55f, 0.55f);
			BoxShader.setVec3("pointLights[0].specular", 0.80f, 0.80f, 0.80f);
			BoxShader.setFloat("pointLights[0].constant", 1.0f);
			BoxShader.setFloat("pointLights[0].linear", 0.09);
			BoxShader.setFloat("pointLights[0].quadratic", 0.032);
			//矩阵变换相关参数
			BoxShader.setMat4("projection", projection);
			BoxShader.setMat4("view", view);
			BoxShader.setVec3("viewPos", camera.Position);
			BoxShader.setVec3("lightPos", camera.Position);
			BoxShader.setMat4("lightSpaceMatrix", lightSpaceMatrix);
			glm::mat4 HarryModel = glm::mat4(1.0f);
			//相机放置在模型眼睛高度附近
			HarryModel = glm::translate(HarryModel, myPos.Position);
			HarryModel = glm::scale(HarryModel, glm::vec3(0.025f, 0.025f, 0.025f));
			HarryModel = glm::rotate(HarryModel, glm::radians(90.0f), glm::vec3(0.0f, 1.0f, 0.0f));
			HarryModel = glm::rotate(HarryModel, glm::radians(90.0f), glm::vec3(-1.0f, 0.0f, 0.0f));
			HarryModel = glm::rotate(HarryModel, glm::radians(180.0f), glm::vec3(0.0f, 0.0f, 1.0f));
			//模型跟随视角旋转
			HarryModel = glm::rotate(HarryModel, myPos.Yaw / 57.0f, glm::vec3(0.0f, 0.0f, -1.0f));
			BoxShader.setMat4("model", HarryModel);
			HarryPotter.Draw(BoxShader);
		}

		if (view_type == FPV)
		{
			camera.Position = myPos.Position + glm::vec3(0.0f, 1.5f, 0.0f);
			myPos.Yaw = camera.Yaw;
			myPos.Front = camera.Front;
			myPos.Right = camera.Right;
			myPos.Up = camera.Up;
		}
		else if (view_type == TPV)
		{
			camera.Position = myPos.Position + glm::vec3(0.0f, 1.8f, 0.0f) - glm::normalize(glm::vec3(myPos.Front.x, 0.0f, myPos.Front.z)) * 1.2f;
			myPos.Yaw = camera.Yaw;
			myPos.Front = camera.Front;
			myPos.Right = camera.Right;
			myPos.Up = camera.Up;
		}

		//更新怪物方向
		for (unsigned int i = 0; i < monsterCount; i++)
		{
			monsterVector[i]->UpdateAngle(myPos.Position);
		}

		//石头怪模型绘制
		BoxShader.use();
		//平行光相关参数
		BoxShader.setInt("dir", dirlight);
		BoxShader.setVec3("dirLight.direction", -0.2f, -1.0f, -0.3f);
		BoxShader.setVec3("dirLight.ambient", 0.55f, 0.55f, 0.55f);
		BoxShader.setVec3("dirLight.diffuse", 0.4f, 0.4f, 0.4f);
		BoxShader.setVec3("dirLight.specular", 0.5f, 0.5f, 0.5f);
		//材质相关参数
		BoxShader.setInt("material.diffuse", 0);
		BoxShader.setInt("material.specular", 1);
		BoxShader.setInt("material.shadowMap", 2);
		BoxShader.setFloat("material.shininess", 32.0f);
		//点光源相关参数
		BoxShader.setVec3("pointLights[0].position", myPos.Position);
		BoxShader.setVec3("pointLights[0].ambient", 0.5f, 0.50f, 0.5f);
		BoxShader.setVec3("pointLights[0].diffuse", 0.35f, 0.350f, 0.35f);
		BoxShader.setVec3("pointLights[0].specular", 0.40f, 0.40f, 0.40f);
		BoxShader.setFloat("pointLights[0].constant", 1.0f);
		BoxShader.setFloat("pointLights[0].linear", 0.09);
		BoxShader.setFloat("pointLights[0].quadratic", 0.032);
		//矩阵变换相关参数
		BoxShader.setMat4("projection", projection);
		BoxShader.setMat4("view", view);
		BoxShader.setVec3("viewPos", camera.Position);
		BoxShader.setVec3("lightPos", camera.Position);
		BoxShader.setMat4("lightSpaceMatrix", lightSpaceMatrix);

		for (unsigned int i = 0; i < monsterCount; i++)
		{
			if (monsterVector[i]->type != 1) continue;
			glm::mat4 StoneManModel = glm::mat4(1.0f);
			StoneManModel = glm::translate(StoneManModel, monsterVector[i]->position);
			StoneManModel = glm::scale(StoneManModel, glm::vec3(monsterVector[i]->zoomRate, monsterVector[i]->zoomRate, monsterVector[i]->zoomRate));
			StoneManModel = glm::rotate(StoneManModel, monsterVector[i]->rotateAngle, glm::vec3(0.0f, 1.0f, 0.0f));
			BoxShader.setMat4("model", StoneManModel);
			StoneMan.Draw(BoxShader);
		}

		//Ogre模型绘制
		for (unsigned int i = 1; i < monsterCount; i++)
		{
			if (monsterVector[i]->type != 2) continue;
			glm::mat4 OgreModel = glm::mat4(1.0f);
			OgreModel = glm::translate(OgreModel, monsterVector[i]->position);
			OgreModel = glm::scale(OgreModel, glm::vec3(monsterVector[i]->zoomRate, monsterVector[i]->zoomRate, monsterVector[i]->zoomRate));
			OgreModel = glm::rotate(OgreModel, monsterVector[i]->rotateAngle, glm::vec3(0.0f, 1.0f, 0.0f));
			BoxShader.setMat4("model", OgreModel);
			Ogre.Draw(BoxShader);
		}

		if (isShadow == true)
		{
			//box 绘制
			BoxShader.use();
			//平行光相关参数
			BoxShader.setInt("dir", dirlight);
			BoxShader.setVec3("dirLight.direction", -0.2f, -1.0f, -0.3f);
			BoxShader.setVec3("dirLight.ambient", 0.55f, 0.55f, 0.55f);
			BoxShader.setVec3("dirLight.diffuse", 0.4f, 0.4f, 0.4f);
			BoxShader.setVec3("dirLight.specular", 0.5f, 0.5f, 0.5f);
			//材质相关参数
			BoxShader.setInt("material.diffuse", 0);
			BoxShader.setInt("material.specular", 1);
			BoxShader.setInt("material.shadowMap", 2);
			BoxShader.setFloat("material.shininess", 32.0f);
			//点光源相关参数
			BoxShader.setVec3("pointLights[0].position", myPos.Position);
			BoxShader.setVec3("pointLights[0].ambient", 0.5f, 0.80f, 0.5f);
			BoxShader.setVec3("pointLights[0].diffuse", 0.35f, 0.80f, 0.35f);
			BoxShader.setVec3("pointLights[0].specular", 0.40f, 0.80f, 0.40f);
			BoxShader.setFloat("pointLights[0].constant", 1.0f);
			BoxShader.setFloat("pointLights[0].linear", 0.09);
			BoxShader.setFloat("pointLights[0].quadratic", 0.032);
			//矩阵变换相关参数
			BoxShader.setMat4("projection", projection);
			BoxShader.setMat4("view", view);
			BoxShader.setVec3("viewPos", camera.Position);
			BoxShader.setVec3("lightPos", camera.Position);
			BoxShader.setMat4("lightSpaceMatrix", lightSpaceMatrix);

			//绑定cube纹理
			glActiveTexture(GL_TEXTURE0);
			glBindTexture(GL_TEXTURE_2D, plantTexture);
			//增加纹理层数
			glActiveTexture(GL_TEXTURE1);
			glBindTexture(GL_TEXTURE_2D, specular);
			glActiveTexture(GL_TEXTURE2);
			glBindTexture(GL_TEXTURE_2D, depthMap);
			glBindVertexArray(cubeVAO);
			//绘制两层cube
			for (unsigned int i = 0; i < boxPosition.size(); i++)
			{
				//计算cube块位移和放缩后的矩阵并绘制
				glm::mat4 CubeModel = glm::mat4(1.0f);
				CubeModel = glm::translate(CubeModel, boxPosition[i]);
				CubeModel = glm::scale(CubeModel, glm::vec3(2.0, 2.0, 2.0));
				BoxShader.setMat4("model", CubeModel);
				glDrawArrays(GL_TRIANGLES, 0, 36);
			}
			for (unsigned int i = 0; i < boxPosition.size(); i++)
			{
				//计算cube块位移和放缩后的矩阵并绘制
				glm::mat4 CubeModel = glm::mat4(1.0f);
				CubeModel = glm::translate(CubeModel, boxPosition[i]);
				CubeModel = glm::translate(CubeModel, glm::vec3(0.0, 2.0, 0.0));
				CubeModel = glm::scale(CubeModel, glm::vec3(2.0, 2.0, 2.0));
				BoxShader.setMat4("model", CubeModel);
				glDrawArrays(GL_TRIANGLES, 0, 36);
			}
		}
		else
		{
			newBoxShader.use();

			newBoxShader.setInt("dir", dirlight);
			newBoxShader.setVec3("dirLight.direction", -0.2f, -1.0f, -0.3f);
			newBoxShader.setVec3("dirLight.ambient", 0.55f, 0.55f, 0.55f);
			newBoxShader.setVec3("dirLight.diffuse", 0.4f, 0.4f, 0.4f);
			newBoxShader.setVec3("dirLight.specular", 0.5f, 0.5f, 0.5f);

			newBoxShader.setInt("material.diffuse", 0);
			newBoxShader.setInt("material.specular", 1);
			newBoxShader.setFloat("material.shininess", 32.0f);

			newBoxShader.setVec3("pointLights[0].position", myPos.Position);
			newBoxShader.setVec3("pointLights[0].ambient", 0.5f, 0.80f, 0.5f);
			newBoxShader.setVec3("pointLights[0].diffuse", 0.35f, 0.80f, 0.35f);
			newBoxShader.setVec3("pointLights[0].specular", 0.40f, 0.80f, 0.40f);
			newBoxShader.setFloat("pointLights[0].constant", 1.0f);
			newBoxShader.setFloat("pointLights[0].linear", 0.09);
			newBoxShader.setFloat("pointLights[0].quadratic", 0.032);

			newBoxShader.setMat4("projection", projection);
			newBoxShader.setMat4("view", view);
			newBoxShader.setVec3("viewPos", camera.Position);

			glActiveTexture(GL_TEXTURE0);
			glBindTexture(GL_TEXTURE_2D, plantTexture);
			glBindVertexArray(cubeVAO);
			for (unsigned int i = 0; i < boxPosition.size(); i++)
			{
				glm::mat4 CubeModel = glm::mat4(1.0f);
				CubeModel = glm::translate(CubeModel, boxPosition[i]);
				CubeModel = glm::scale(CubeModel, glm::vec3(2.0, 2.0, 2.0));
				newBoxShader.setMat4("model", CubeModel);
				glDrawArrays(GL_TRIANGLES, 0, 36);
			}
			for (unsigned int i = 0; i < boxPosition.size(); i++)
			{
				glm::mat4 CubeModel = glm::mat4(1.0f);
				CubeModel = glm::translate(CubeModel, boxPosition[i]);
				CubeModel = glm::translate(CubeModel, glm::vec3(0.0, 2.0, 0.0));
				CubeModel = glm::scale(CubeModel, glm::vec3(2.0, 2.0, 2.0));
				newBoxShader.setMat4("model", CubeModel);
				glDrawArrays(GL_TRIANGLES, 0, 36);
			}
		}

		//绘制light cube
		LightShader.use();
		LightShader.setMat4("projection", projection);
		LightShader.setMat4("view", view);
		glBindVertexArray(lightCubeVAO);
		if (isParticle == true)
		{
			for (unsigned int i = 0; i < p.pos.size(); i++)
			{
				glm::mat4 ParticleModel = glm::mat4(1.0f);
				ParticleModel = glm::translate(ParticleModel, p.pos[i]);
				ParticleModel = glm::scale(ParticleModel, glm::vec3(0.05f));
				LightShader.setMat4("model", ParticleModel);
				LightShader.setVec4("pcolor", glm::vec4(p.color[i].x, p.color[i].y, p.color[i].z, 0.0f));
				glDrawArrays(GL_TRIANGLES, 0, 36);
			}
			glm::vec3 time = glm::vec3(10);
			UpdateParticle(time, p);
			if (p.lifespan < 0)
			{
				haveParticle = true;
				isParticle = false;
			}
		}
		if (isMagic == true)
		{
			for (unsigned int i = 0; i < ma.pos.size(); i++)
			{
				glm::mat4 MagicModel = glm::mat4(1.0f);
				MagicModel = glm::translate(MagicModel, ma.pos[i]);
				MagicModel = glm::scale(MagicModel, glm::vec3(0.05f));
				LightShader.setMat4("model", MagicModel);
				LightShader.setVec4("pcolor", glm::vec4(ma.color.x, ma.color.y, ma.color.z, 0.0f));
				glDrawArrays(GL_TRIANGLES, 0, 36);
			}
			glm::vec3 time = glm::vec3(0.2);
			UpdateMagic(time, ma);
		}

		//绘制 plane
		BoxShader.use();
		//平行光相关参数
		BoxShader.setInt("dir", dirlight);
		BoxShader.setVec3("dirLight.direction", -0.2f, -1.0f, -0.3f);
		BoxShader.setVec3("dirLight.ambient", 0.55f, 0.55f, 0.55f);
		BoxShader.setVec3("dirLight.diffuse", 0.4f, 0.4f, 0.4f);
		BoxShader.setVec3("dirLight.specular", 0.2f, 0.2f, 0.2f);
		//材质相关参数
		BoxShader.setInt("material.diffuse", 0);
		BoxShader.setInt("material.specular", 1);
		BoxShader.setFloat("material.shininess", 32.0f);
		//点光源相关参数
		BoxShader.setVec3("pointLights[0].position", myPos.Position);
		BoxShader.setVec3("pointLights[0].ambient", 0.5f, 0.5f, 0.5f);
		BoxShader.setVec3("pointLights[0].diffuse", 0.3f, 0.3f, 0.3f);
		BoxShader.setVec3("pointLights[0].specular", 0.5f, 0.5f, 0.5f);
		BoxShader.setFloat("pointLights[0].constant", 1.0f);
		BoxShader.setFloat("pointLights[0].linear", 0.09);
		BoxShader.setFloat("pointLights[0].quadratic", 0.032);
		//矩阵变化参数
		BoxShader.setMat4("projection", projection);
		BoxShader.setMat4("model", model);
		BoxShader.setMat4("view", view);
		BoxShader.setVec3("viewPos", camera.Position);
		BoxShader.setVec3("lightPos", camera.Position);
		BoxShader.setMat4("lightSpaceMatrix", lightSpaceMatrix);

		//绘制二维平面
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, planeTexture);
		glActiveTexture(GL_TEXTURE1);
		glBindTexture(GL_TEXTURE_2D, specular);
		glBindVertexArray(planeVAO);
		glDrawArrays(GL_TRIANGLES, 0, 6);

		//绘制skybox
		SkyboxShader.use();
		glm::mat4 SkyBoxView = glm::mat4(glm::mat3(camera.GetViewMatrix()));
		SkyboxShader.setMat4("view", SkyBoxView);
		SkyboxShader.setMat4("projection", projection);

		glActiveTexture(GL_TEXTURE0);
		if (isNight == false)
			glBindTexture(GL_TEXTURE_CUBE_MAP, SkyboxTexture);
		else
			glBindTexture(GL_TEXTURE_CUBE_MAP, OtherTexture);
		glBindVertexArray(skyboxVAO);
		glDrawArrays(GL_TRIANGLES, 0, 36);

		glBindFramebuffer(GL_FRAMEBUFFER, 0);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		hdrShader.use();
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, colorBuffer);
		hdrShader.setInt("hdrBuffer", 0);
		hdrShader.setInt("hdr", ishdr);
		hdrShader.setInt("gm", isgamma);
		hdrShader.setFloat("exposure", 1.0f);
		renderQuad();

		//双缓冲交换
		glfwSwapBuffers(window);
		//立即处理到位的事件
		glfwPollEvents();
	}

	//释放内存
	glDeleteVertexArrays(1, &cubeVAO);
	glDeleteVertexArrays(1, &lightCubeVAO);
	glDeleteVertexArrays(1, &skyboxVAO);
	glDeleteVertexArrays(1, &planeVAO);
	glDeleteBuffers(1, &skyboxVAO);
	glDeleteBuffers(1, &cubeVBO);

	//结束glfw
	glfwTerminate();
	return 0;
}

void CreateMap(const std::string filename)
{
	std::ifstream in(filename);
	std::string s, temp;
	int i = -1, length = 0, width = 0;
	int count = 0;
	while (getline(in, s))
	{
		if (i == -1)
		{
			std::istringstream instr(s);
			instr >> length >> width;
		}
		else
		{
			int temp;
			std::istringstream instr(s);
			for (int k = 0; k < width; k++)
			{
				instr >> temp;
				if (temp == 1)
				{
					boxPosition.push_back(glm::vec3(2 * i - 1, 0.0f, 2 * k - 1));
				}
				else if (temp == 2)
				{
					cupPosition = (glm::vec3(2 * i - 1, -1.0f, 2 * k - 1));
				}
				else if (temp == 3)
				{
					monsterVector[monsterCount++] = new Monster(1, 0.2f, glm::vec3(4.5f, 9.6f, 1.3f), glm::vec3(2 * i - 1, -1.0f, 2 * k - 1), 100);
				}
				else if (temp == 4)
				{
					monsterVector[monsterCount++] = new Monster(2, 0.3f, glm::vec3(3.0f, 5.0f, 1.0f), glm::vec3(2 * i - 1, -1.0f, 2 * k - 1), 100);
				}
			}
		}
		i++;
		if (i == length) break;
	}
}

unsigned int CreateSkybox(int id)
{
	std::vector<std::string> faces;
	if (id == 0)
	{
		faces.push_back(std::string("resource/skybox/skybox3/left.jpg"));
		faces.push_back(std::string("resource/skybox/skybox3/right.jpg"));
		faces.push_back(std::string("resource/skybox/skybox3/top.jpg"));
		faces.push_back(std::string("resource/skybox/skybox3/bottom.jpg"));
		faces.push_back(std::string("resource/skybox/skybox3/front.jpg"));
		faces.push_back(std::string("resource/skybox/skybox3/back.jpg"));
	}
	else if (id == 1)
	{
		faces.push_back(std::string("resource/skybox/skybox1/right.jpg"));
		faces.push_back(std::string("resource/skybox/skybox1/left.jpg"));
		faces.push_back(std::string("resource/skybox/skybox1/top.jpg"));
		faces.push_back(std::string("resource/skybox/skybox1/bottom.jpg"));
		faces.push_back(std::string("resource/skybox/skybox1/front.jpg"));
		faces.push_back(std::string("resource/skybox/skybox1/back.jpg"));
	}
	unsigned int textureID;
	glGenTextures(1, &textureID);
	glBindTexture(GL_TEXTURE_CUBE_MAP, textureID);

	int width, height, nrChannels;
	for (unsigned int i = 0; i < faces.size(); i++)
	{
		unsigned char* data = stbi_load(faces[i].c_str(), &width, &height, &nrChannels, 0);
		if (data)
		{
			glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
			stbi_image_free(data);
		}
		else
		{
			std::cout << "Cubemap texture failed to load at path: " << faces[i] << std::endl;
			stbi_image_free(data);
		}
	}
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

	return textureID;
}

unsigned int loadTexture(char const* path)
{
	// Get textureID
	unsigned int textureID;
	glGenTextures(1, &textureID);

	// use stbi_load to load texture pic
	int width, height, nrComponents;
	unsigned char* data = stbi_load(path, &width, &height, &nrComponents, 0);
	if (data)
	{
		GLenum format;
		if (nrComponents == 1)
			format = GL_RED;
		else if (nrComponents == 3)
			format = GL_RGB;
		else if (nrComponents == 4)
			format = GL_RGBA;

		// Bind texture to the buffer and set data
		glBindTexture(GL_TEXTURE_2D, textureID);
		glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
		// Generate mipmap
		glGenerateMipmap(GL_TEXTURE_2D);

		//Set the behavior when scaling
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

		// Free the data
		stbi_image_free(data);
	}
	else
	{
		std::cout << "Texture failed to load at path: " << path << std::endl;
		stbi_image_free(data);
	}

	return textureID;
}

unsigned int loadCubemap(std::vector<std::string> faces)
{
	unsigned int textureID;
	glGenTextures(1, &textureID);
	glBindTexture(GL_TEXTURE_CUBE_MAP, textureID);

	int width, height, nrChannels;
	for (unsigned int i = 0; i < faces.size(); i++)
	{
		unsigned char* data = stbi_load(faces[i].c_str(), &width, &height, &nrChannels, 0);
		if (data)
		{
			glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
			stbi_image_free(data);
		}
		else
		{
			std::cout << "Cubemap texture failed to load at path: " << faces[i] << std::endl;
			stbi_image_free(data);
		}
	}
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

	return textureID;
}

void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
	glViewport(0, 0, width, height);
}

void processInput(GLFWwindow *window, Camera& mypos, Camera& camera)
{
	//计算间隔时间
	float currentFrame = glfwGetTime();
	deltaTime = currentFrame - lastFrame;
	lastFrame = currentFrame;
	//退出游戏
	if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
	{
		glfwSetWindowShouldClose(window, true);
	}
	//冲刺
	if (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS)
	{
		mypos.MovementSpeed = ACC_VELOCITY;
		camera.MovementSpeed = ACC_VELOCITY;
	}
	if (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_RELEASE)
	{
		mypos.MovementSpeed = DEFAULT_VELOCITY;
		camera.MovementSpeed = DEFAULT_VELOCITY;
	}

	//前后左右移动
	if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
	{
		if (view_type == GPV)
			camera.ProcessKeyboard(FORWARD, deltaTime, view_type);
		else if ((view_type == FPV || view_type == TPV) && CollisionDetection(mypos.Position, HarryWidth / 2))
		{
			mypos.ProcessKeyboard(FORWARD, deltaTime, view_type);
		}
	}
	if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
	{
		if (view_type == GPV)
			camera.ProcessKeyboard(BACKWARD, deltaTime, view_type);
		else if ((view_type == FPV || view_type == TPV) && CollisionDetection(mypos.Position, HarryWidth / 2))
		{
			mypos.ProcessKeyboard(BACKWARD, deltaTime, view_type);
		}
	}
	if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
	{
		if (view_type == GPV)
			camera.ProcessKeyboard(LEFT, deltaTime, view_type);
		else if ((view_type == FPV || view_type == TPV) && CollisionDetection(mypos.Position, HarryWidth / 2))
		{
			mypos.ProcessKeyboard(LEFT, deltaTime, view_type);
		}
	}
	if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
	{
		if (view_type == GPV)
			camera.ProcessKeyboard(RIGHT, deltaTime, view_type);
		else if ((view_type == FPV || view_type == TPV) && CollisionDetection(mypos.Position, HarryWidth / 2))
		{
			mypos.ProcessKeyboard(RIGHT, deltaTime, view_type);
		}
	}

	//数字键123切换视角
	if (glfwGetKey(window, GLFW_KEY_1) == GLFW_PRESS)
	{
		view_type = FPV;
	}
	if (glfwGetKey(window, GLFW_KEY_2) == GLFW_PRESS)
	{
		view_type = GPV;
	}
	if (glfwGetKey(window, GLFW_KEY_3) == GLFW_PRESS)
	{
		view_type = TPV;
	}

	//平行光开关
	if (glfwGetKey(window, GLFW_KEY_Q) == GLFW_PRESS)
		dirlight = true;
	if (glfwGetKey(window, GLFW_KEY_E) == GLFW_PRESS)
		dirlight = false;
	//天空盒切换
	if (glfwGetKey(window, GLFW_KEY_N) == GLFW_PRESS)
		isNight = true;
	if (glfwGetKey(window, GLFW_KEY_M) == GLFW_PRESS)
		isNight = false;
	//gamma修正开关
	if (glfwGetKey(window, GLFW_KEY_O) == GLFW_PRESS)
		isgamma = true;
	if (glfwGetKey(window, GLFW_KEY_P) == GLFW_PRESS)
		isgamma = false;
	//hdr修正开关
	if (glfwGetKey(window, GLFW_KEY_K) == GLFW_PRESS)
		ishdr = true;
	if (glfwGetKey(window, GLFW_KEY_L) == GLFW_PRESS)
		ishdr = false;
	//阴影开关
	if (glfwGetKey(window, GLFW_KEY_U) == GLFW_PRESS)
		isShadow = true;
	if (glfwGetKey(window, GLFW_KEY_I) == GLFW_PRESS)
		isShadow = false;
	//暂停
	if (glfwGetKey(window, GLFW_KEY_ENTER) == GLFW_PRESS)
		state = state + 1;
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
	float yoffset = lastY - ypos;
	lastX = xpos;
	lastY = ypos;
	camera.ProcessMouseMovement(xoffset, yoffset);
}

void scroll_callback(GLFWwindow* window, double xoffset, double yoffset)
{
	camera.ProcessMouseScroll(yoffset);
}

bool CollisionDetection(glm::vec3& position, float width, float height)
{
	float curx = position.x, cury = position.y, curz = position.z;
	float leftEdge = curx - width / 2;
	float rightEdge = curx + width / 2;
	float frontEdge = curz - width / 2;
	float backEdge = curz + width / 2;
	float bottomEdge = cury - height / 2;
	float topEdge = cury + height / 2;
	for (int i = 0; i < boxPosition.size(); i++)
	{
		if (leftEdge > boxPosition[i].x - BoxWidth / 2 && leftEdge < boxPosition[i].x + BoxWidth / 2
			&& frontEdge > boxPosition[i].z - BoxWidth / 2 && frontEdge < boxPosition[i].z + BoxWidth / 2
			&& topEdge > boxPosition[i].y - BoxHeight / 2 && topEdge < boxPosition[i].y + BoxHeight / 2)
		{
			return false;
		}

		if (leftEdge > boxPosition[i].x - BoxWidth / 2 && leftEdge < boxPosition[i].x + BoxWidth / 2
			&& frontEdge > boxPosition[i].z - BoxWidth / 2 && frontEdge < boxPosition[i].z + BoxWidth / 2
			&& bottomEdge > boxPosition[i].y - BoxHeight / 2 && bottomEdge < boxPosition[i].y + BoxHeight / 2)
		{
			return false;
		}

		if (leftEdge > boxPosition[i].x - BoxWidth / 2 && leftEdge < boxPosition[i].x + BoxWidth / 2
			&& backEdge > boxPosition[i].z - BoxWidth / 2 && backEdge < boxPosition[i].z + BoxWidth / 2
			&& topEdge > boxPosition[i].y - BoxHeight / 2 && topEdge < boxPosition[i].y + BoxHeight / 2)
		{
			return false;
		}

		if (leftEdge > boxPosition[i].x - BoxWidth / 2 && leftEdge < boxPosition[i].x + BoxWidth / 2
			&& backEdge > boxPosition[i].z - BoxWidth / 2 && backEdge < boxPosition[i].z + BoxWidth / 2
			&& bottomEdge > boxPosition[i].y - BoxHeight / 2 && bottomEdge < boxPosition[i].y + BoxHeight / 2)
		{
			return false;
		}

		if (rightEdge > boxPosition[i].x - BoxWidth / 2 && rightEdge < boxPosition[i].x + BoxWidth / 2
			&& backEdge > boxPosition[i].z - BoxWidth / 2 && backEdge < boxPosition[i].z + BoxWidth / 2
			&& topEdge > boxPosition[i].y - BoxHeight / 2 && topEdge < boxPosition[i].y + BoxHeight / 2)
		{
			return false;
		}

		if (rightEdge > boxPosition[i].x - BoxWidth / 2 && rightEdge < boxPosition[i].x + BoxWidth / 2
			&& backEdge > boxPosition[i].z - BoxWidth / 2 && backEdge < boxPosition[i].z + BoxWidth / 2
			&& bottomEdge > boxPosition[i].y - BoxHeight / 2 && bottomEdge < boxPosition[i].y + BoxHeight / 2)
		{
			return false;
		}

		if (rightEdge > boxPosition[i].x - BoxWidth / 2 && rightEdge < boxPosition[i].x + BoxWidth / 2
			&& frontEdge > boxPosition[i].z - BoxWidth / 2 && frontEdge < boxPosition[i].z + BoxWidth / 2
			&& topEdge > boxPosition[i].y - BoxHeight / 2 && topEdge < boxPosition[i].y + BoxHeight / 2)
		{
			return false;
		}

		if (rightEdge > boxPosition[i].x - BoxWidth / 2 && rightEdge < boxPosition[i].x + BoxWidth / 2
			&& frontEdge > boxPosition[i].z - BoxWidth / 2 && frontEdge < boxPosition[i].z + BoxWidth / 2
			&& bottomEdge > boxPosition[i].y - BoxHeight / 2 && bottomEdge < boxPosition[i].y + BoxHeight / 2)
		{
			return false;
		}
	}

	for (int i = 0; i < monsterCount; i++)
	{
		if (leftEdge > monsterVector[i]->position.x - monsterVector[i]->volume.x *  monsterVector[i]->zoomRate
			&& leftEdge < monsterVector[i]->position.x + monsterVector[i]->volume.x *  monsterVector[i]->zoomRate
			&& frontEdge > monsterVector[i]->position.z - monsterVector[i]->volume.z *  monsterVector[i]->zoomRate
			&& frontEdge < monsterVector[i]->position.z + monsterVector[i]->volume.z *  monsterVector[i]->zoomRate
			&& topEdge > monsterVector[i]->position.y - monsterVector[i]->volume.y *  monsterVector[i]->zoomRate
			&& topEdge < monsterVector[i]->position.y + monsterVector[i]->volume.y *  monsterVector[i]->zoomRate)
		{
			monsterVector[i]->UpdateHP(25);
			return false;
		}

		if (rightEdge > monsterVector[i]->position.x - monsterVector[i]->volume.x *  monsterVector[i]->zoomRate
			&& rightEdge < monsterVector[i]->position.x + monsterVector[i]->volume.x *  monsterVector[i]->zoomRate
			&& frontEdge > monsterVector[i]->position.z - monsterVector[i]->volume.z *  monsterVector[i]->zoomRate
			&& frontEdge < monsterVector[i]->position.z + monsterVector[i]->volume.z *  monsterVector[i]->zoomRate
			&& bottomEdge > monsterVector[i]->position.y - monsterVector[i]->volume.y *  monsterVector[i]->zoomRate
			&& bottomEdge < monsterVector[i]->position.y + monsterVector[i]->volume.y *  monsterVector[i]->zoomRate)
		{
			monsterVector[i]->UpdateHP(25);
			return false;
		}

		if (rightEdge > monsterVector[i]->position.x - monsterVector[i]->volume.x *  monsterVector[i]->zoomRate
			&& rightEdge < monsterVector[i]->position.x + monsterVector[i]->volume.x *  monsterVector[i]->zoomRate
			&& frontEdge > monsterVector[i]->position.z - monsterVector[i]->volume.z *  monsterVector[i]->zoomRate
			&& frontEdge < monsterVector[i]->position.z + monsterVector[i]->volume.z *  monsterVector[i]->zoomRate
			&& topEdge > monsterVector[i]->position.y - monsterVector[i]->volume.y *  monsterVector[i]->zoomRate
			&& topEdge < monsterVector[i]->position.y + monsterVector[i]->volume.y *  monsterVector[i]->zoomRate)
		{
			monsterVector[i]->UpdateHP(25);
			return false;
		}

		if (leftEdge > monsterVector[i]->position.x - monsterVector[i]->volume.x *  monsterVector[i]->zoomRate
			&& leftEdge < monsterVector[i]->position.x + monsterVector[i]->volume.x *  monsterVector[i]->zoomRate
			&& frontEdge > monsterVector[i]->position.z - monsterVector[i]->volume.z *  monsterVector[i]->zoomRate
			&& frontEdge < monsterVector[i]->position.z + monsterVector[i]->volume.z *  monsterVector[i]->zoomRate
			&& bottomEdge > monsterVector[i]->position.y - monsterVector[i]->volume.y *  monsterVector[i]->zoomRate
			&& bottomEdge < monsterVector[i]->position.y + monsterVector[i]->volume.y *  monsterVector[i]->zoomRate)
		{
			monsterVector[i]->UpdateHP(25);
			return false;
		}

		if (leftEdge > monsterVector[i]->position.x - monsterVector[i]->volume.x *  monsterVector[i]->zoomRate
			&& leftEdge < monsterVector[i]->position.x + monsterVector[i]->volume.x *  monsterVector[i]->zoomRate
			&& backEdge > monsterVector[i]->position.z - monsterVector[i]->volume.z *  monsterVector[i]->zoomRate
			&& backEdge < monsterVector[i]->position.z + monsterVector[i]->volume.z *  monsterVector[i]->zoomRate
			&& topEdge > monsterVector[i]->position.y - monsterVector[i]->volume.y *  monsterVector[i]->zoomRate
			&& topEdge < monsterVector[i]->position.y + monsterVector[i]->volume.y *  monsterVector[i]->zoomRate)
		{
			monsterVector[i]->UpdateHP(25);
			return false;
		}

		if (leftEdge > monsterVector[i]->position.x - monsterVector[i]->volume.x *  monsterVector[i]->zoomRate
			&& leftEdge < monsterVector[i]->position.x + monsterVector[i]->volume.x *  monsterVector[i]->zoomRate
			&& backEdge > monsterVector[i]->position.z - monsterVector[i]->volume.z *  monsterVector[i]->zoomRate
			&& backEdge < monsterVector[i]->position.z + monsterVector[i]->volume.z *  monsterVector[i]->zoomRate
			&& bottomEdge > monsterVector[i]->position.y - monsterVector[i]->volume.y *  monsterVector[i]->zoomRate
			&& bottomEdge < monsterVector[i]->position.y + monsterVector[i]->volume.y *  monsterVector[i]->zoomRate)
		{
			monsterVector[i]->UpdateHP(25);
			return false;
		}

		if (rightEdge > monsterVector[i]->position.x - monsterVector[i]->volume.x *  monsterVector[i]->zoomRate
			&& rightEdge < monsterVector[i]->position.x + monsterVector[i]->volume.x *  monsterVector[i]->zoomRate
			&& backEdge > monsterVector[i]->position.z - monsterVector[i]->volume.z *  monsterVector[i]->zoomRate
			&& backEdge < monsterVector[i]->position.z + monsterVector[i]->volume.z *  monsterVector[i]->zoomRate
			&& topEdge > monsterVector[i]->position.y - monsterVector[i]->volume.y *  monsterVector[i]->zoomRate
			&& topEdge < monsterVector[i]->position.y + monsterVector[i]->volume.y *  monsterVector[i]->zoomRate)
		{
			monsterVector[i]->UpdateHP(25);
			return false;
		}

		if (rightEdge > monsterVector[i]->position.x - monsterVector[i]->volume.x *  monsterVector[i]->zoomRate
			&& rightEdge < monsterVector[i]->position.x + monsterVector[i]->volume.x *  monsterVector[i]->zoomRate
			&& backEdge > monsterVector[i]->position.z - monsterVector[i]->volume.z *  monsterVector[i]->zoomRate
			&& backEdge < monsterVector[i]->position.z + monsterVector[i]->volume.z *  monsterVector[i]->zoomRate
			&& bottomEdge > monsterVector[i]->position.y - monsterVector[i]->volume.y *  monsterVector[i]->zoomRate
			&& bottomEdge < monsterVector[i]->position.y + monsterVector[i]->volume.y *  monsterVector[i]->zoomRate)
		{
			monsterVector[i]->UpdateHP(25);
			return false;
		}
	}
	return true;
}

float randFloat(float a, float b) {
	return a + (b - a)*(1.0*rand() / RAND_MAX);
}

int randInt(int a, int b) {
	return a + rand() % (b - a);
}

struct Magic initMagic(glm::vec3 position, GLuint lifespan, glm::vec3 front, glm::vec3 color, float eps)
{
	Magic p;
	float temp = randFloat(-0.001, 0.001);
	p.pos.push_back(position + glm::vec3(eps, eps, randFloat(-0.001, 0.001)));
	p.pos.push_back(position + glm::vec3(-eps, -eps, randFloat(-0.001, 0.001)));
	p.pos.push_back(position + glm::vec3(eps, randFloat(-0.001, 0.001), eps));
	p.pos.push_back(position + glm::vec3(-eps, randFloat(-0.001, 0.001), -eps));
	p.pos.push_back(position + glm::vec3(randFloat(-0.001, 0.001), eps, eps));
	p.pos.push_back(position + glm::vec3(randFloat(-0.001, 0.001), -eps, -eps));
	p.pos.push_back(position + glm::vec3(eps, -eps, randFloat(-0.001, 0.001)));
	p.pos.push_back(position + glm::vec3(-eps, eps, randFloat(-0.001, 0.001)));
	p.pos.push_back(position + glm::vec3(eps, randFloat(-0.001, 0.001), -eps));
	p.pos.push_back(position + glm::vec3(-eps, randFloat(-0.001, 0.001), eps));
	p.pos.push_back(position + glm::vec3(randFloat(-0.001, 0.001), eps, -eps));
	p.pos.push_back(position + glm::vec3(randFloat(-0.001, 0.001), -eps, eps));
	p.speed = glm::normalize(front);
	p.color = color;
	p.lifespan = lifespan;
	p.disappear = false;
	return p;
}

void UpdateMagic(glm::vec3 time, Magic &myma)
{
	for (int i = 0; i < myma.pos.size(); i++)
	{
		if (myma.lifespan == 0 || !CollisionDetection(myma.pos[i], 0.01f, 0.01f) || myma.pos[i].y <= -1.0f)
		{
			isMagic = false;
			isParticle = true;
			p = initParticle(myma.pos[0], 3000, 100, glm::vec3(0.8, 0, 0), 0.5);
			break;
		}
		myma.pos[i] = myma.pos[i] + myma.speed * time;
	}
	myma.lifespan--;
}

struct Particle initParticle(glm::vec3 position, GLuint lifespan, int num, glm::vec3 color, float eps)
{
	int ParticleNumber = num;
	Particle p;
	for (int i = 0; i < ParticleNumber; i++)
	{
		p.pos.push_back(position);
		p.speed.push_back(glm::vec3(0.00f, 0.0f, 0.00f));
		float a, b, c;
		a = randFloat(-eps, eps);
		b = randFloat(-eps, eps);
		c = randFloat(-eps, eps);
		glm::vec3 temp = glm::vec3(a, b, c);
		p.color.push_back(temp + color);
		p.lifespan = lifespan;
	}
	return p;
}

void UpdateParticle(glm::vec3 time, Particle &p)
{
	for (int i = 0; i < p.pos.size(); i++)
	{
		if (p.lifespan == 0)
			break;
		p.pos[i] = p.pos[i] + p.speed[i] * time;
		float a, b, c;
		a = randFloat(-0.0001, 0.0001);
		b = randFloat(-0.0001, 0.0001);
		c = randFloat(-0.0001, 0.0001);
		glm::vec3 temp = glm::vec3(a, b, c);
		p.speed[i] = p.speed[i] + temp;
	}
	p.lifespan--;
}

void mouse_button_callback(GLFWwindow* window, int button, int action, int mods)
{
	//左键攻击
	if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS)
		if (isMagic == false)
		{
			isMagic = true;
			ma = initMagic(camera.Position, 100, camera.Front, glm::vec3(0.8, 0.2, 0.2), 0.05);
		}
}

void renderQuad()
{
	if (quadVAO == 0)
	{
		float quadVertices[] = {
			// positions        // texture Coords
			-1.0f,  1.0f, 0.0f, 0.0f, 1.0f,
			-1.0f, -1.0f, 0.0f, 0.0f, 0.0f,
			 1.0f,  1.0f, 0.0f, 1.0f, 1.0f,
			 1.0f, -1.0f, 0.0f, 1.0f, 0.0f,
		};
		glGenVertexArrays(1, &quadVAO);
		glGenBuffers(1, &quadVBO);
		glBindVertexArray(quadVAO);
		glBindBuffer(GL_ARRAY_BUFFER, quadVBO);
		glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), &quadVertices, GL_STATIC_DRAW);
		glEnableVertexAttribArray(0);
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
		glEnableVertexAttribArray(1);
		glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
	}
	glBindVertexArray(quadVAO);
	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
	glBindVertexArray(0);
}

void renderScreen()
{
	if (ScreenVAO == 0)
	{
		float quadVertices[] = {
			// positions        // texture Coords
			-1.0f,  1.0f, 0.0f, 0.0f, 0.0f,
			-1.0f, -1.0f, 0.0f, 0.0f, 1.0f,
			 1.0f,  1.0f, 0.0f, 1.0f, 0.0f,
			 1.0f, -1.0f, 0.0f, 1.0f, 1.0f,
		};
		glGenVertexArrays(1, &ScreenVAO);
		glGenBuffers(1, &ScreenVBO);
		glBindVertexArray(ScreenVAO);
		glBindBuffer(GL_ARRAY_BUFFER, ScreenVBO);
		glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), &quadVertices, GL_STATIC_DRAW);
		glEnableVertexAttribArray(0);
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
		glEnableVertexAttribArray(1);
		glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
	}
	glBindVertexArray(ScreenVAO);
	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
	glBindVertexArray(0);
}

void renderScene(const Shader &shader)
{
	glm::mat4 model = glm::mat4(1.0f);
	shader.setMat4("model", model);
	glBindVertexArray(planeVAO);
	glDrawArrays(GL_TRIANGLES, 0, 6);

	for (unsigned int i = 0; i < boxPosition.size(); i++)
	{
		model = glm::mat4(1.0f);
		model = glm::translate(model, boxPosition[i]);
		model = glm::scale(model, glm::vec3(2.0));
		shader.setMat4("model", model);
		renderCube();
	}
	for (unsigned int i = 0; i < boxPosition.size(); i++)
	{
		model = glm::mat4(1.0f);
		model = glm::translate(model, boxPosition[i]);
		model = glm::translate(model, glm::vec3(0.0, 2.0, 0.0));
		model = glm::scale(model, glm::vec3(2.0));
		shader.setMat4("model", model);
		renderCube();
	}
}

void renderCube()
{
	if (cVAO == 0)
	{
		glGenVertexArrays(1, &cVAO);
		glGenBuffers(1, &cVBO);
		glBindBuffer(GL_ARRAY_BUFFER, cVBO);
		glBufferData(GL_ARRAY_BUFFER, sizeof(BoxVertices), BoxVertices, GL_STATIC_DRAW);
		glBindVertexArray(cVAO);
		glEnableVertexAttribArray(0);
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
		glEnableVertexAttribArray(1);
		glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));
		glEnableVertexAttribArray(2);
		glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(6 * sizeof(float)));
		glBindBuffer(GL_ARRAY_BUFFER, 0);
		glBindVertexArray(0);
	}
	glBindVertexArray(cVAO);
	glDrawArrays(GL_TRIANGLES, 0, 36);
	glBindVertexArray(0);
}

bool CollisionDetection(glm::vec3& position, float radius)
{
	float curx = position.x, cury = position.y, curz = position.z;
	float leftEdge = curx - radius;
	float rightEdge = curx + radius;
	float frontEdge = curz - radius;
	float backEdge = curz + radius;
	for (int i = 0; i < boxPosition.size(); i++)
	{
		if (leftEdge > boxPosition[i].x - BoxWidth / 2 && leftEdge < boxPosition[i].x + BoxWidth / 2 && frontEdge > boxPosition[i].z - BoxWidth / 2 && frontEdge < boxPosition[i].z + BoxWidth / 2)
		{
			float a = leftEdge - (boxPosition[i].x - BoxWidth / 2);
			float b = (boxPosition[i].x + BoxWidth / 2) - leftEdge;
			float c = frontEdge - (boxPosition[i].z - BoxWidth / 2);
			float d = (boxPosition[i].z + BoxWidth / 2) - frontEdge;
			float min = (a < b) ? (a < c ? a : c) : (b < c ? b : c);
			min = (min < d) ? min : d;
			if (min == a)
				position.x = boxPosition[i].x - BoxWidth / 2 - eps + radius;
			if (min == b)
				position.x = boxPosition[i].x + BoxWidth / 2 + eps + radius;
			if (min == c)
				position.z = boxPosition[i].z - BoxWidth / 2 - eps + radius;
			if (min == d)
				position.z = boxPosition[i].z + BoxWidth / 2 + eps + radius;
			return false;
		}
		if (leftEdge > boxPosition[i].x - BoxWidth / 2 && leftEdge < boxPosition[i].x + BoxWidth / 2 && backEdge > boxPosition[i].z - BoxWidth / 2 && backEdge < boxPosition[i].z + BoxWidth / 2)
		{
			float a = leftEdge - (boxPosition[i].x - BoxWidth / 2);
			float b = (boxPosition[i].x + BoxWidth / 2) - leftEdge;
			float c = backEdge - (boxPosition[i].z - BoxWidth / 2);
			float d = (boxPosition[i].z + BoxWidth / 2) - backEdge;
			float min = (a < b) ? (a < c ? a : c) : (b < c ? b : c);
			min = (min < d) ? min : d;
			if (min == a)
				position.x = boxPosition[i].x - BoxWidth / 2 - eps + radius;
			if (min == b)
				position.x = boxPosition[i].x + BoxWidth / 2 + eps + radius;
			if (min == c)
				position.z = boxPosition[i].z - BoxWidth / 2 - eps - radius;
			if (min == d)
				position.z = boxPosition[i].z + BoxWidth / 2 + eps - radius;
			return false;
		}
		if (rightEdge > boxPosition[i].x - BoxWidth / 2 && rightEdge < boxPosition[i].x + BoxWidth / 2 && backEdge > boxPosition[i].z - BoxWidth / 2 && backEdge < boxPosition[i].z + BoxWidth / 2)
		{
			float a = rightEdge - (boxPosition[i].x - BoxWidth / 2);
			float b = (boxPosition[i].x + BoxWidth / 2) - rightEdge;
			float c = backEdge - (boxPosition[i].z - BoxWidth / 2);
			float d = (boxPosition[i].z + BoxWidth / 2) - backEdge;
			float min = (a < b) ? (a < c ? a : c) : (b < c ? b : c);
			min = (min < d) ? min : d;
			if (min == a)
				position.x = boxPosition[i].x - BoxWidth / 2 - eps - radius;
			if (min == b)
				position.x = boxPosition[i].x + BoxWidth / 2 + eps - radius;
			if (min == c)
				position.z = boxPosition[i].z - BoxWidth / 2 - eps - radius;
			if (min == d)
				position.z = boxPosition[i].z + BoxWidth / 2 + eps - radius;
			return false;
		}
		if (rightEdge > boxPosition[i].x - BoxWidth / 2 && rightEdge < boxPosition[i].x + BoxWidth / 2 && frontEdge > boxPosition[i].z - BoxWidth / 2 && frontEdge < boxPosition[i].z + BoxWidth / 2)
		{
			float a = rightEdge - (boxPosition[i].x - BoxWidth / 2);
			float b = (boxPosition[i].x + BoxWidth / 2) - rightEdge;
			float c = frontEdge - (boxPosition[i].z - BoxWidth / 2);
			float d = (boxPosition[i].z + BoxWidth / 2) - frontEdge;
			float min = (a < b) ? (a < c ? a : c) : (b < c ? b : c);
			min = (min < d) ? min : d;
			if (min == a)
				position.x = boxPosition[i].x - BoxWidth / 2 - eps - radius;
			if (min == b)
				position.x = boxPosition[i].x + BoxWidth / 2 + eps - radius;
			if (min == c)
				position.z = boxPosition[i].z - BoxWidth / 2 - eps + radius;
			if (min == d)
				position.z = boxPosition[i].z + BoxWidth / 2 + eps + radius;
			return false;
		}
	}

	for (int i = 0; i < monsterCount; i++)
	{
		if (leftEdge > monsterVector[i]->position.x - monsterVector[i]->volume.x *  monsterVector[i]->zoomRate
			&& leftEdge < monsterVector[i]->position.x + monsterVector[i]->volume.x *  monsterVector[i]->zoomRate
			&& frontEdge > monsterVector[i]->position.z - monsterVector[i]->volume.z *  monsterVector[i]->zoomRate
			&& frontEdge < monsterVector[i]->position.z + monsterVector[i]->volume.z *  monsterVector[i]->zoomRate)
		{
			float a = leftEdge - (monsterVector[i]->position.x - monsterVector[i]->volume.x *  monsterVector[i]->zoomRate);
			float b = (monsterVector[i]->position.x + monsterVector[i]->volume.x *  monsterVector[i]->zoomRate) - leftEdge;
			float c = frontEdge - (monsterVector[i]->position.z - monsterVector[i]->volume.z *  monsterVector[i]->zoomRate);
			float d = (monsterVector[i]->position.z + monsterVector[i]->volume.z *  monsterVector[i]->zoomRate) - frontEdge;
			float min = (a < b) ? (a < c ? a : c) : (b < c ? b : c);
			min = (min < d) ? min : d;
			if (min == a)
				position.x = monsterVector[i]->position.x - monsterVector[i]->volume.x *  monsterVector[i]->zoomRate - eps + radius;
			if (min == b)
				position.x = monsterVector[i]->position.x + monsterVector[i]->volume.x *  monsterVector[i]->zoomRate + eps + radius;
			if (min == c)
				position.z = monsterVector[i]->position.z - monsterVector[i]->volume.z *  monsterVector[i]->zoomRate - eps + radius;
			if (min == d)
				position.z = monsterVector[i]->position.z + monsterVector[i]->volume.z *  monsterVector[i]->zoomRate + eps + radius;
			return false;
		}
		if (leftEdge > monsterVector[i]->position.x - monsterVector[i]->volume.x *  monsterVector[i]->zoomRate
			&& leftEdge < monsterVector[i]->position.x + monsterVector[i]->volume.x *  monsterVector[i]->zoomRate
			&& backEdge > monsterVector[i]->position.z - monsterVector[i]->volume.z *  monsterVector[i]->zoomRate
			&& backEdge < monsterVector[i]->position.z + monsterVector[i]->volume.z *  monsterVector[i]->zoomRate)
		{
			float a = leftEdge - (monsterVector[i]->position.x - monsterVector[i]->volume.x *  monsterVector[i]->zoomRate);
			float b = (monsterVector[i]->position.x + monsterVector[i]->volume.x *  monsterVector[i]->zoomRate) - leftEdge;
			float c = backEdge - (monsterVector[i]->position.z - monsterVector[i]->volume.z *  monsterVector[i]->zoomRate);
			float d = (monsterVector[i]->position.z + monsterVector[i]->volume.z *  monsterVector[i]->zoomRate) - backEdge;
			float min = (a < b) ? (a < c ? a : c) : (b < c ? b : c);
			min = (min < d) ? min : d;
			if (min == a)
				position.x = monsterVector[i]->position.x - monsterVector[i]->volume.x *  monsterVector[i]->zoomRate - eps + radius;
			if (min == b)
				position.x = monsterVector[i]->position.x + monsterVector[i]->volume.x *  monsterVector[i]->zoomRate + eps + radius;
			if (min == c)
				position.z = monsterVector[i]->position.z - monsterVector[i]->volume.z *  monsterVector[i]->zoomRate - eps - radius;
			if (min == d)
				position.z = monsterVector[i]->position.z + monsterVector[i]->volume.z *  monsterVector[i]->zoomRate + eps - radius;
			return false;
		}
		if (rightEdge > monsterVector[i]->position.x - monsterVector[i]->volume.x *  monsterVector[i]->zoomRate
			&& rightEdge < monsterVector[i]->position.x + monsterVector[i]->volume.x *  monsterVector[i]->zoomRate
			&& backEdge > monsterVector[i]->position.z - monsterVector[i]->volume.z *  monsterVector[i]->zoomRate
			&& backEdge < monsterVector[i]->position.z + monsterVector[i]->volume.z *  monsterVector[i]->zoomRate)
		{
			float a = rightEdge - (monsterVector[i]->position.x - monsterVector[i]->volume.x *  monsterVector[i]->zoomRate);
			float b = (monsterVector[i]->position.x + monsterVector[i]->volume.x *  monsterVector[i]->zoomRate) - rightEdge;
			float c = backEdge - (monsterVector[i]->position.z - monsterVector[i]->volume.z *  monsterVector[i]->zoomRate);
			float d = (monsterVector[i]->position.z + monsterVector[i]->volume.z *  monsterVector[i]->zoomRate) - backEdge;
			float min = (a < b) ? (a < c ? a : c) : (b < c ? b : c);
			min = (min < d) ? min : d;
			if (min == a)
				position.x = monsterVector[i]->position.x - monsterVector[i]->volume.x *  monsterVector[i]->zoomRate - eps - radius;
			if (min == b)
				position.x = monsterVector[i]->position.x + monsterVector[i]->volume.x *  monsterVector[i]->zoomRate + eps - radius;
			if (min == c)
				position.z = monsterVector[i]->position.z - monsterVector[i]->volume.z *  monsterVector[i]->zoomRate - eps - radius;
			if (min == d)
				position.z = monsterVector[i]->position.z + monsterVector[i]->volume.z *  monsterVector[i]->zoomRate + eps - radius;
			return false;
		}
		if (rightEdge > monsterVector[i]->position.x - monsterVector[i]->volume.x *  monsterVector[i]->zoomRate
			&& rightEdge < monsterVector[i]->position.x + monsterVector[i]->volume.x *  monsterVector[i]->zoomRate
			&& frontEdge > monsterVector[i]->position.z - monsterVector[i]->volume.z *  monsterVector[i]->zoomRate
			&& frontEdge < monsterVector[i]->position.z + monsterVector[i]->volume.z *  monsterVector[i]->zoomRate)
		{
			float a = rightEdge - (monsterVector[i]->position.x - monsterVector[i]->volume.x *  monsterVector[i]->zoomRate);
			float b = (monsterVector[i]->position.x + monsterVector[i]->volume.x *  monsterVector[i]->zoomRate) - rightEdge;
			float c = frontEdge - (monsterVector[i]->position.z - monsterVector[i]->volume.z *  monsterVector[i]->zoomRate);
			float d = (monsterVector[i]->position.z + monsterVector[i]->volume.z *  monsterVector[i]->zoomRate) - frontEdge;
			float min = (a < b) ? (a < c ? a : c) : (b < c ? b : c);
			min = (min < d) ? min : d;
			if (min == a)
				position.x = monsterVector[i]->position.x - monsterVector[i]->volume.x *  monsterVector[i]->zoomRate - eps - radius;
			if (min == b)
				position.x = monsterVector[i]->position.x + monsterVector[i]->volume.x *  monsterVector[i]->zoomRate + eps - radius;
			if (min == c)
				position.z = monsterVector[i]->position.z - monsterVector[i]->volume.z *  monsterVector[i]->zoomRate - eps + radius;
			if (min == d)
				position.z = monsterVector[i]->position.z + monsterVector[i]->volume.z *  monsterVector[i]->zoomRate + eps + radius;
			return false;
		}
	}
	return true;
}

