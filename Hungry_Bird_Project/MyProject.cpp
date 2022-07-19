// This has been adapted from the Vulkan tutorial

#include "MyProject.hpp"

const std::string MODEL_PATH = "Assets/models";
const std::string TEXTURE_PATH = "Assets/textures";

bool cameraON = true;
const glm::vec3 CANNON_BOT_POS = glm::vec3(-0.45377f, 8.78275f, -3.0006f);
const glm::vec3 CANNON_TOP_POS = glm::vec3(-0.45377f, 9.50215f, -3.0006f);


// The global buffer object used for view and proj
struct GlobalUniformBufferObject {
	alignas(16) glm::mat4 view;
	alignas(16) glm::mat4 proj;
};

// The uniform buffer object used for models
struct UniformBufferObject {
	alignas(16) glm::mat4 model;
};

class GameTime
{

	/**
	 * The Singleton's constructor should always be private to prevent direct
	 * construction calls with the `new` operator.
	 */

protected:
	GameTime()
	{}

	static GameTime* singleton_;
	float deltaT;
	float time;

public:

	GameTime(GameTime& other) = delete;

	void operator=(const GameTime&) = delete;

	static GameTime* GetInstance();

	void setTime() {
		static auto startTime = std::chrono::high_resolution_clock::now();
		static float lastTime = 0.0f;
		auto currentTime = std::chrono::high_resolution_clock::now();
		time = std::chrono::duration<float, std::chrono::seconds::period>
			(currentTime - startTime).count();
		deltaT = time - lastTime;
		lastTime = time;
	}

	float getDelta() {
		return deltaT;
	}

	float getTime() {
		return time;
	}

};

GameTime* GameTime::singleton_ = nullptr;;

GameTime* GameTime::GetInstance()
{
	if (singleton_ == nullptr) {
		singleton_ = new GameTime();
	}
	return singleton_;
}

class SkyBox {
protected:
	Pipeline P_SkyBox;
	Model M_skyBox;
	Texture T_skyBox;
	DescriptorSet DS_skyBox;

public:
	// initialize all attributes
	void init(BaseProject* bp, DescriptorSetLayout DSLobj, DescriptorSetLayout DSLglobal) {
		P_SkyBox.init(bp, "shaders/skyBoxVert.spv", "shaders/skyBoxFrag.spv", { &DSLglobal, &DSLobj });
		M_skyBox.init(bp, MODEL_PATH + "/SkyBox/SkyBox.obj");
		T_skyBox.init(bp, TEXTURE_PATH + "/SkyBox/SkyBox.png");
		DS_skyBox.init(bp, &DSLobj, {
		{0, UNIFORM, sizeof(UniformBufferObject), nullptr},
		{1, TEXTURE, 0, &T_skyBox}
			});
	}

	// cleanup all the attributes
	void cleanup() {
		DS_skyBox.cleanup();
		T_skyBox.cleanup();
		M_skyBox.cleanup();
		P_SkyBox.cleanup();
	}


	// Populate command buffer ( bind pipeline, descriptorSet global and descriptorSet skyBox )
	void populateCommandBuffer(VkCommandBuffer commandBuffer, int currentImage, DescriptorSet DS_global) {
		vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
			P_SkyBox.graphicsPipeline);

		vkCmdBindDescriptorSets(commandBuffer,
			VK_PIPELINE_BIND_POINT_GRAPHICS,
			P_SkyBox.pipelineLayout, 0, 1, &DS_global.descriptorSets[currentImage],
			0, nullptr);

		VkBuffer vertexBuffers_skyBox[] = { M_skyBox.vertexBuffer };
		VkDeviceSize offsets_skyBox[] = { 0 };
		vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertexBuffers_skyBox, offsets_skyBox);
		vkCmdBindIndexBuffer(commandBuffer, M_skyBox.indexBuffer, 0,
			VK_INDEX_TYPE_UINT32);
		vkCmdBindDescriptorSets(commandBuffer,
			VK_PIPELINE_BIND_POINT_GRAPHICS,
			P_SkyBox.pipelineLayout, 1, 1, &DS_skyBox.descriptorSets[currentImage],
			0, nullptr);
		vkCmdDrawIndexed(commandBuffer,
			static_cast<uint32_t>(M_skyBox.indices.size()), 1, 0, 0, 0);
	}

	// update before rendering
	UniformBufferObject update(UniformBufferObject ubo) {
		ubo.model = glm::scale(glm::mat4(1.0f), glm::vec3(50.0f, 50.0f, 50.0f));
		return ubo;
	}

	// update ubo and render
	void updateUniformBuffer(VkDevice device, int currentImage, void* data, UniformBufferObject ubo) {
		ubo = update(ubo);
		vkMapMemory(device, DS_skyBox.uniformBuffersMemory[0][currentImage], 0,
			sizeof(ubo), 0, &data);
		memcpy(data, &ubo, sizeof(ubo));
		vkUnmapMemory(device, DS_skyBox.uniformBuffersMemory[0][currentImage]);
	}
};

class Camera {
protected:
	glm::vec3 CamPos = glm::vec3(0.0f, 0.0f, 0.0f);
	glm::vec3 CamAng = glm::vec3(0.0f, 0.0f, 0.0f);
	glm::mat4 CamDir;

	const float ROT_SPEED = glm::radians(60.0f);
	const float MOVE_SPEED = 1.75f;

public:
	// update the camera position
	glm::mat4 update(GLFWwindow* window) {
		float deltaT = GameTime::GetInstance()->getDelta();
		// Camera movements
		if (glfwGetKey(window, GLFW_KEY_LEFT)) {
			CamAng.y += deltaT * ROT_SPEED;
		}
		if (glfwGetKey(window, GLFW_KEY_RIGHT)) {
			CamAng.y -= deltaT * ROT_SPEED;
		}
		if (glfwGetKey(window, GLFW_KEY_UP)) {
			CamAng.x += deltaT * ROT_SPEED;
		}
		if (glfwGetKey(window, GLFW_KEY_DOWN)) {
			CamAng.x -= deltaT * ROT_SPEED;
		}

		glm::mat3 CamEye = glm::mat3(glm::rotate(glm::mat4(1.0f), CamAng.y, glm::vec3(0.0f, 1.0f, 0.0f))) *
			glm::mat3(glm::rotate(glm::mat4(1.0f), CamAng.x, glm::vec3(1.0f, 0.0f, 0.0f))) *
			glm::mat3(glm::rotate(glm::mat4(1.0f), CamAng.z, glm::vec3(0.0f, 0.0f, 1.0f)));

		if (cameraON) {
			if (glfwGetKey(window, GLFW_KEY_A)) {
				CamPos -= MOVE_SPEED * glm::vec3(glm::rotate(glm::mat4(1.0f), CamAng.y,
					glm::vec3(0.0f, 1.0f, 0.0f)) * glm::vec4(1, 0, 0, 1)) * deltaT;
			}
			if (glfwGetKey(window, GLFW_KEY_D)) {
				CamPos += MOVE_SPEED * glm::vec3(glm::rotate(glm::mat4(1.0f), CamAng.y,
					glm::vec3(0.0f, 1.0f, 0.0f)) * glm::vec4(1, 0, 0, 1)) * deltaT;
			}
			if (glfwGetKey(window, GLFW_KEY_S)) {
				CamPos += MOVE_SPEED * glm::vec3(glm::rotate(glm::mat4(1.0f), CamAng.y,
					glm::vec3(0.0f, 1.0f, 0.0f)) * glm::vec4(0, 0, 1, 1)) * deltaT;
			}
			if (glfwGetKey(window, GLFW_KEY_W)) {
				CamPos -= MOVE_SPEED * glm::vec3(glm::rotate(glm::mat4(1.0f), CamAng.y,
					glm::vec3(0.0f, 1.0f, 0.0f)) * glm::vec4(0, 0, 1, 1)) * deltaT;
			}
			if (glfwGetKey(window, GLFW_KEY_F)) {
				CamPos -= MOVE_SPEED * glm::vec3(0, 1, 0) * deltaT;
			}
			if (glfwGetKey(window, GLFW_KEY_R)) {
				CamPos += MOVE_SPEED * glm::vec3(0, 1, 0) * deltaT;
			}
		}

		return CamDir = glm::translate(glm::transpose(glm::mat4(CamEye)), -CamPos);
	}

};

class Asset {
protected:
	Model model;
	Texture texture;
	std::vector<DescriptorSet*> dSetVector;

public:
	// initialize model and texture
	void init(BaseProject* bp, std::string modelPath, std::string texturePath, DescriptorSetLayout* DSLobj) {
		model.init(bp, MODEL_PATH + modelPath);
		texture.init(bp, TEXTURE_PATH + texturePath);
	}

	// Add a descriptorSet which means a new gameObject of the asset to render
	void addDSet(BaseProject* bp, DescriptorSetLayout* DSLobj, DescriptorSet* dSet) {
		dSetVector.push_back(dSet);
		(*dSet).init(bp, DSLobj, {
		{0, UNIFORM, sizeof(UniformBufferObject), nullptr},
		{1, TEXTURE, 0, &texture}
			});
	}

	// cleanup all the attributes
	void cleanup() {
		for (DescriptorSet* dSet : dSetVector)
		{
			(*dSet).cleanup();
		}
		texture.cleanup();
		model.cleanup();
	}

	// Populate command buffer (vertex, descriptor set, indices)
	void populateCommandBuffer(VkCommandBuffer commandBuffer, int currentImage, DescriptorSet DS_global, Pipeline* P1) {
		VkBuffer vertexBuffers[] = { model.vertexBuffer };
		VkDeviceSize offsets[] = { 0 };
		vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertexBuffers, offsets);
		vkCmdBindIndexBuffer(commandBuffer, model.indexBuffer, 0,
			VK_INDEX_TYPE_UINT32);
		for (DescriptorSet* dSet : dSetVector)
		{
			vkCmdBindDescriptorSets(commandBuffer,
				VK_PIPELINE_BIND_POINT_GRAPHICS,
				(*P1).pipelineLayout, 1, 1, &(*dSet).descriptorSets[currentImage],
				0, nullptr);
			vkCmdDrawIndexed(commandBuffer,
				static_cast<uint32_t>(model.indices.size()), 1, 0, 0, 0);
		}
	}
};

class GameObject {
public:
	DescriptorSet dSet;

	virtual UniformBufferObject update(GLFWwindow* window, UniformBufferObject ubo) = 0; //must return ubo

	void updateUniformBuffer(GLFWwindow* window, VkDevice device, int currentImage, void* data, UniformBufferObject ubo) {
		ubo = update(window, ubo);
		vkMapMemory(device, dSet.uniformBuffersMemory[0][currentImage], 0, sizeof(ubo), 0, &data);
		memcpy(data, &ubo, sizeof(ubo));
		vkUnmapMemory(device, dSet.uniformBuffersMemory[0][currentImage]);
	}
};

class Decoration: public GameObject {
	virtual UniformBufferObject update(GLFWwindow* window, UniformBufferObject ubo) override {
		//tutto fermo
		ubo.model = glm::translate(glm::mat4(1.0f), glm::vec3(1.0f, 0.0f, 0.0f));
		return ubo;
	}
};

class Bird :public GameObject {
protected:
	glm::vec3 startPos = CANNON_TOP_POS;
	glm::vec3 birdPos = CANNON_TOP_POS;
	glm::vec3 birdAng = glm::vec3(0.0f);

	const float ROT_SPEED = 60.0f;

	bool isActive = false;

	bool isJumping = false;
	float startJump = 0.0f;
	float deltaT = 0.0f;

	virtual UniformBufferObject update(GLFWwindow* window, UniformBufferObject ubo) override {

		if (glfwGetKey(window, GLFW_KEY_J) && !isJumping) {
			isJumping = true;
			startJump = glfwGetTime();
		}

		if (isJumping) {
			jump(10.0f, glm::radians(45.0f), glm::radians(birdAng.x));
		}
		if (birdPos == glm::vec3(0.0f)) {
			std::cout << "0";
			ubo.model = glm::translate(glm::mat4(1.0f), birdPos); //aggiunto per essere siucro che andasse all'origine
		}
		else
			ubo.model = glm::translate(glm::mat4(1.0f), birdPos) * glm::rotate(glm::mat4(1.0f), glm::radians(birdAng.x), glm::vec3(0.0f, 1.0f, 0.0f));
		return ubo;
	}

	void jump(float v0, float angY, float angX) {
		deltaT = glfwGetTime() - startJump;

		birdPos = startPos.x + (v0 * cos(angY)) * deltaT * glm::vec3(glm::rotate(glm::mat4(1.0f), angX,
			glm::vec3(0.0f, 1.0f, 0.0f)) * glm::vec4(0, 0, 1, 1));

		birdPos += glm::vec3(0.0f, -(0.5 * 9.8f * pow(deltaT, 2)) + (v0 * sin(angY)) * deltaT + startPos.y, 0.0f);
		if (birdPos.y <= 0.0f) {
			birdPos.y = 0.0f;
			isJumping = false;
		}
	}

	public:
		void ShowStat(int i) {
			std::cout << "----- Bird in " << i << "----- " << std::endl;
			std::cout << "Active: " << isActive << std::endl;
			std::cout << "Position: " << birdPos.x <<" " << birdPos.y << " " << birdPos.z << std::endl;
			std::cout << "-----------------------------" << std::endl;
		}
	void setActive() {
		this->isActive = true;
		this->startPos = glm::vec3(0.0f);
		this->birdPos = glm::vec3(0.0f);
	}
};

class BirdBlue :public Bird {

};

class BirdYellow : public Bird {
public:
	BirdYellow() : Bird() {
		startPos.x += 2.0f ;
		birdPos.x += 2.0f;
	}
};

class Pig :public GameObject {
	virtual UniformBufferObject update(GLFWwindow* window, UniformBufferObject ubo) override {
		ubo.model = glm::translate(glm::mat4(1.0f), glm::vec3(1.0f, 0.0f, 0.0f));
		return ubo;
	}
};

class PigStd :public Pig {

};


class CannonBot : public GameObject {
	protected:
	glm::vec3 cannonPos = CANNON_BOT_POS;
	glm::vec3 cannonAng = glm::vec3(0.0f);

	const float ROT_SPEED = 60.0f;

	public: 
	virtual UniformBufferObject update(GLFWwindow* window, UniformBufferObject ubo) override {
		float deltaT = GameTime::GetInstance()->getDelta();
		if (!cameraON) {
			if (glfwGetKey(window, GLFW_KEY_A)) {
				cannonAng.x += ROT_SPEED * deltaT;
			}
			if (glfwGetKey(window, GLFW_KEY_D)) {
				cannonAng.x -= ROT_SPEED * deltaT;
			}
		}
		ubo.model = glm::rotate(glm::translate(glm::mat4(1.0f), cannonPos), glm::radians(cannonAng.x), glm::vec3(0.0f, 1.0f, 0.0f));
		return ubo;
	}
};

class CannonTop : public GameObject {
	protected:
	glm::vec3 cannonPos = CANNON_TOP_POS;
	glm::vec3 cannonAng = glm::vec3(0.0f);

	const float ROT_SPEED = 60.0f;

	public: 
	virtual UniformBufferObject update(GLFWwindow* window, UniformBufferObject ubo) override {
		float deltaT = GameTime::GetInstance()->getDelta();
		if (!cameraON) {
			if (glfwGetKey(window, GLFW_KEY_A)) {
				cannonAng.x += ROT_SPEED * deltaT;
			}
			if (glfwGetKey(window, GLFW_KEY_D)) {
				cannonAng.x -= ROT_SPEED * deltaT;
			}
			if (glfwGetKey(window, GLFW_KEY_S)) {
				if (cannonAng.y < 25.5746f) {
					cannonAng.y += ROT_SPEED * deltaT;
				}
				else {
					cannonAng.y = 25.5746;
				}
			}
			if (glfwGetKey(window, GLFW_KEY_W)) {
				if (cannonAng.y > -90.0f) {
					cannonAng.y -= ROT_SPEED * deltaT;
				}
				else {
					cannonAng.y = -90.0f;
				}
			}
		}
		
		ubo.model = glm::rotate(glm::rotate(glm::translate(glm::mat4(1.0f), cannonPos), glm::radians(cannonAng.x), glm::vec3(0.0f, 1.0f, 0.0f)), glm::radians(cannonAng.y), glm::vec3(1.0f, 0.0f, 0.0f));
		return ubo;
	}
};


class Terrain : public GameObject {
	virtual UniformBufferObject update(GLFWwindow* window, UniformBufferObject ubo) override {
		ubo.model = glm::mat4(1.0f);
		return ubo;
	}
};

// MAIN ! 
class MyProject : public BaseProject {
protected:
	// Here you list all the Vulkan objects you need:

	// Descriptor Layouts [what will be passed to the shaders]
	DescriptorSetLayout DSLglobal;
	DescriptorSetLayout DSLobj;

	Camera camera;
	SkyBox skyBox;

	// Pipelines [Shader couples]
	Pipeline P1;

	// Models, textures and Descriptors (values assigned to the uniforms)

	Asset A_BlueBird;
	BirdBlue bird1;
	BirdYellow bird2;
	BirdBlue bird3;

	std::vector<Bird *> birds;
	int birdInCannon = 0;

	Asset A_PigStd;
	PigStd pigStd;

	Asset A_Terrain;
	Terrain terrain;

	Asset A_CannonBot;
	CannonBot cannonBot;

	Asset A_CannonTop;
	CannonTop cannonTop;

	//Decorations Assets and GO
	Asset A_TowerSiege;
	Decoration towerSiege;

	Asset A_Baloon;
	Decoration baloon;

	Asset A_SeaCity25;
	Decoration seaCity25;

	Asset A_SeaCity37;
	Decoration seaCity37;

	Asset A_ShipSmall;
	Decoration shipSmall;

	Asset A_ShipVikings;
	Decoration shipVikings;

	Asset A_SkyCity;
	Decoration skyCity;

	DescriptorSet DS_global;



	// Here you set the main application parameters
	void setWindowParameters() {
		// window size, titile and initial background
		windowWidth = 1600;
		windowHeight = 1200;
		windowTitle = "Hungry_Bird";
		initialBackgroundColor = { 0.0f, 0.0f, 0.0f, 1.0f };

		// Descriptor pool sizes
		uniformBlocksInPool = 20;	//10
		texturesInPool = 20;		//9
		setsInPool = 20;			//10
	}

	void setGameState() {
		birds.push_back(&bird1);
		birds.push_back(&bird2);
		//birds.push_back(&bird3);
	}

	// Here you load and setup all your Vulkan objects
	void localInit() {

		setGameState();

		// Descriptor Layouts [what will be passed to the shaders]
		DSLobj.init(this, {
			// this array contains the binding:
			// first  element : the binding number
			// second element : the time of element (buffer or texture)
			// third  element : the pipeline stage where it will be used
			{0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT},
			{1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT}
			});
		DSLglobal.init(this, {
		{0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_ALL_GRAPHICS}
			});

		// Pipelines [Shader couples]
		// The last array, is a vector of pointer to the layouts of the sets that will
		// be used in this pipeline. The first element will be set 0, and so on..
		P1.init(this, "shaders/materialVert.spv", "shaders/materialFrag.spv", { &DSLglobal, &DSLobj });


		// Models, textures and Descriptors (values assigned to the uniforms)
		A_BlueBird.init(this, "/Birds/blues.obj", "/texture.png", &DSLobj);
		for (Bird *bird : birds) {
			A_BlueBird.addDSet(this, &DSLobj, &(*bird).dSet);
		}

		A_PigStd.init(this, "/Pigs/pig.obj", "/texture.png", &DSLobj);
		A_PigStd.addDSet(this, &DSLobj, &pigStd.dSet);

		A_Terrain.init(this, "/Terrain/Terrain.obj", "/Terrain/terrain.png", &DSLobj);
		A_Terrain.addDSet(this, &DSLobj, &terrain.dSet);

		A_CannonBot.init(this, "/Cannon/BotCannon.obj", "/Cannon/map_CP_001.001_BaseColorRedBird.png", &DSLobj);
		A_CannonBot.addDSet(this, &DSLobj, &cannonBot.dSet);

		A_CannonTop.init(this, "/Cannon/TopCannon.obj", "/Cannon/map_CP_001.001_BaseColorRedBird.png", &DSLobj);
		A_CannonTop.addDSet(this, &DSLobj, &cannonTop.dSet);

		A_TowerSiege.init(this, "/Decorations/TowerSiege.obj", "/Decorations/TowerSiege.png", &DSLobj);
		A_TowerSiege.addDSet(this, &DSLobj, &towerSiege.dSet);

		A_Baloon.init(this, "/Decorations/Baloon.obj", "/Decorations/Baloon.png", &DSLobj);
		A_Baloon.addDSet(this, &DSLobj, &baloon.dSet);

		A_SeaCity25.init(this, "/Decorations/SeaCity25.obj", "/Decorations/SeaCity25.png", &DSLobj);
		A_SeaCity25.addDSet(this, &DSLobj, &seaCity25.dSet);

		A_SeaCity37.init(this, "/Decorations/SeaCity37.obj", "/Decorations/SeaCity37.png", &DSLobj);
		A_SeaCity37.addDSet(this, &DSLobj, &seaCity37.dSet);

		A_ShipSmall.init(this, "/Decorations/ShipSmall.obj", "/Decorations/ShipSmall.png", &DSLobj);
		A_ShipSmall.addDSet(this, &DSLobj, &shipSmall.dSet);

		A_ShipVikings.init(this, "/Decorations/ShipVikings.obj", "/Decorations/ShipVikings.png", &DSLobj);
		A_ShipVikings.addDSet(this, &DSLobj, &shipVikings.dSet);

		A_SkyCity.init(this, "/Decorations/SkyCity.obj", "/Decorations/SkyCity.png", &DSLobj);
		A_SkyCity.addDSet(this, &DSLobj, &skyCity.dSet);

		skyBox.init(this, DSLobj, DSLglobal);


		DS_global.init(this, &DSLglobal, {
		{0, UNIFORM, sizeof(GlobalUniformBufferObject), nullptr},
			});
	}

	// Here you destroy all the objects you created!		
	void localCleanup() {

		A_BlueBird.cleanup();

		A_PigStd.cleanup();

		A_Terrain.cleanup();

		A_CannonBot.cleanup();

		A_CannonTop.cleanup();

		//Decorations
		A_Baloon.cleanup();
		A_SeaCity25.cleanup();
		A_SeaCity37.cleanup();
		A_ShipSmall.cleanup();
		A_ShipVikings.cleanup();
		A_TowerSiege.cleanup();
		A_SkyCity.cleanup();

		skyBox.cleanup();

		P1.cleanup();


		DS_global.cleanup();

		DSLglobal.cleanup();
		DSLobj.cleanup();
	}

	// Here it is the creation of the command buffer:
	// You send to the GPU all the objects you want to draw,
	// with their buffers and textures
	void populateCommandBuffer(VkCommandBuffer commandBuffer, int currentImage) {

		// --------------------- SKYBOX -------------------------

		skyBox.populateCommandBuffer(commandBuffer, currentImage, DS_global);


		// -------------------- Pipeline 1 -----------------------------


		vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
			P1.graphicsPipeline);

		vkCmdBindDescriptorSets(commandBuffer,
			VK_PIPELINE_BIND_POINT_GRAPHICS,
			P1.pipelineLayout, 0, 1, &DS_global.descriptorSets[currentImage],
			0, nullptr);


		// ---------------------- BIRD BLUES ------------

		A_BlueBird.populateCommandBuffer(commandBuffer, currentImage, DS_global, &P1);

		// ------------------------ PIG --------------------

		A_PigStd.populateCommandBuffer(commandBuffer, currentImage, DS_global, &P1);

		// ------------------------ Terrain -----------------

		 A_Terrain.populateCommandBuffer(commandBuffer, currentImage, DS_global, &P1);

		 // ----------------------- Cannon -------------------

		 A_CannonBot.populateCommandBuffer(commandBuffer, currentImage, DS_global, &P1);
		 A_CannonTop.populateCommandBuffer(commandBuffer, currentImage, DS_global, &P1);

		 // ----------------------- DECORATIONS --------------------
		 A_Baloon.populateCommandBuffer(commandBuffer, currentImage, DS_global, &P1);
		 A_SeaCity25.populateCommandBuffer(commandBuffer, currentImage, DS_global, &P1);
		 A_SeaCity37.populateCommandBuffer(commandBuffer, currentImage, DS_global, &P1);
		 A_ShipSmall.populateCommandBuffer(commandBuffer, currentImage, DS_global, &P1);
		 A_ShipVikings.populateCommandBuffer(commandBuffer, currentImage, DS_global, &P1);
		 A_TowerSiege.populateCommandBuffer(commandBuffer, currentImage, DS_global, &P1);
		 A_SkyCity.populateCommandBuffer(commandBuffer, currentImage, DS_global, &P1);
		 
	}

	// Here is where you update the uniforms.
	// Very likely this will be where you will be writing the logic of your application.
	void updateUniformBuffer(uint32_t currentImage) {

		GameTime::GetInstance()->setTime();

		if (glfwGetKey(window, GLFW_KEY_X)) {
			cameraON = !cameraON;
		}
		
		if (glfwGetKey(window, GLFW_KEY_SPACE)) {
			(*birds.at(1)).setActive();
			(*birds.at(1)).ShowStat(1);
			(*birds.at(0)).ShowStat(0);
		}

		if (glfwGetKey(window, GLFW_KEY_L)) {
			(*birds[1]).ShowStat(1);
			(*birds.at(0)).ShowStat(0);
		}

		UniformBufferObject ubo{};
		GlobalUniformBufferObject gubo{};

		

		void* data;

		gubo.view = camera.update(window);
		gubo.proj = glm::perspective(glm::radians(45.0f),
			swapChainExtent.width / (float)swapChainExtent.height,
			0.1f, 200.0f);
		gubo.proj[1][1] *= -1;


		// global
		vkMapMemory(device, DS_global.uniformBuffersMemory[0][currentImage], 0,
			sizeof(gubo), 0, &data);
		memcpy(data, &gubo, sizeof(gubo));
		vkUnmapMemory(device, DS_global.uniformBuffersMemory[0][currentImage]);


		// SkyBox

		skyBox.updateUniformBuffer(device, currentImage, data, ubo);


		// Here is where you actually update your uniforms

		// ------------------------ BIRDS ---------------------------


		for (Bird *bird : birds) {
			(*bird).updateUniformBuffer(window, device, currentImage, data, ubo);
		}

		// ------------------------ PIGS ---------------------------

		pigStd.updateUniformBuffer(window, device, currentImage, data, ubo);

		//----------------------- TERRAIN --------------------------

		terrain.updateUniformBuffer(window, device, currentImage, data, ubo);

		// --------------------- Cannon ----------------------------

		cannonBot.updateUniformBuffer(window, device, currentImage, data, ubo);
		cannonTop.updateUniformBuffer(window, device, currentImage, data, ubo);

		// -------------------- DECORATION ---------------------------
		baloon.updateUniformBuffer(window, device, currentImage, data, ubo);
		seaCity25.updateUniformBuffer(window, device, currentImage, data, ubo);
		seaCity37.updateUniformBuffer(window, device, currentImage, data, ubo);
		shipSmall.updateUniformBuffer(window, device, currentImage, data, ubo);
		shipVikings.updateUniformBuffer(window, device, currentImage, data, ubo);
		towerSiege.updateUniformBuffer(window, device, currentImage, data, ubo);
		skyCity.updateUniformBuffer(window, device, currentImage, data, ubo);

	}
};

// This is the main: probably you do not need to touch this!
int main() {
	MyProject app;

	try {
		app.run();
	}
	catch (const std::exception& e) {
		std::cerr << e.what() << std::endl;
		return EXIT_FAILURE;
	}

	return EXIT_SUCCESS;
}