/*

	Homework #2 - Pong
	jc7699@nyu.edu

*/

#ifdef _WINDOWS
#include <GL/glew.h>
#endif
#include <SDL.h>
#define GL_GLEXT_PROTOTYPES 1
#include <SDL_opengl.h>
#include <SDL_image.h>
#include "ShaderProgram.h"
#include "glm/mat4x4.hpp"
#include "glm/gtc/matrix_transform.hpp"
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#include <string>

#ifdef _WINDOWS
#define RESOURCE_FOLDER ""
#else
#define RESOURCE_FOLDER "NYUCodebase.app/Contents/Resources/"
#endif

SDL_Window* displayWindow;
ShaderProgram program;
glm::mat4 projectionMatrix;
glm::mat4 modelMatrix;
glm::mat4 viewMatrix;

const float leftBorder = -1.777f;
const float rightBorder = 1.777f;
const float topBorder = 1.0f;
const float bottomBorder = -1.0f;
const float borderHeight = 0.1f;

float p1_pos[] = { -1.6f, 0.0f, 1.0f };
float p2_pos[] = { 1.6f, 0.0f, 1.0f };
float p1_color[] = { 0.0f, 1.0f, 0.0f, 1.0f };
float p2_color[] = { 0.0f, 0.0f, 1.0f, 1.0f };
float COLOR_WHITE[4] = { 1.0f, 1.0f, 1.0f, 1.0f };

GLuint LoadTexture(const char *filePath) {
	int w, h, comp;
	unsigned char* image = stbi_load(filePath, &w, &h, &comp, STBI_rgb_alpha);

	if (image == NULL) {
		std::cout << "Unable to load image. Make sure the path is correct\n";
		assert(false);
	}

	GLuint retTexture;
	glGenTextures(1, &retTexture);
	glBindTexture(GL_TEXTURE_2D, retTexture);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, image);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	stbi_image_free(image);
	return retTexture;
}

// Paddle entity with its own member values
struct paddle {
	float height = 0.3f;
	float width = 0.1f;
	float vertices[12] = { -width / 2, height / 2, -width / 2, -height / 2, width / 2, height / 2, width / 2, -height / 2, -width / 2, -height / 2, width / 2, height / 2 };
	float speed = 0.0f;
	float position[3];
	float color[4];

	paddle(float pos[3], float color_values[4]) {
		for (int i = 0; i < 3; ++i)
			position[i] = pos[i];
		for (int i = 0; i < 4; ++i)
			color[i] = color_values[i];
	}
};

// Ball entity with its own member values
struct ball {
	float length = 0.1f;
	float vertices[12] = { -length / 2, length / 2, -length / 2, -length / 2, length / 2, length / 2, length / 2, -length / 2, - length / 2, -length / 2, length / 2, length / 2 };
	float position[3] = { 0.0f, 0.0f, 1.0f };
	float velocity_x;
	float velocity_y;

	ball() {
		int r = rand() % 2;
		if (r == 0)
			velocity_x = -0.75f;
		else
			velocity_x = 0.75f;

		r = rand() % 2;
		if (r == 0)
			velocity_y = -0.75f;
		else
			velocity_y = 0.75f;
	}
};

// Draws the paddle at its current position
void drawPaddle(glm::mat4& modelMatrix, paddle& paddle) {
	modelMatrix = glm::mat4(1.0f);
	modelMatrix = glm::translate(modelMatrix, glm::vec3(paddle.position[0], paddle.position[1], paddle.position[2]));
	program.SetModelMatrix(modelMatrix);
	program.SetColor(paddle.color[0], paddle.color[1], paddle.color[2], paddle.color[3]);

	glVertexAttribPointer(program.positionAttribute, 2, GL_FLOAT, false, 0, paddle.vertices);
	glEnableVertexAttribArray(program.positionAttribute);
	glDrawArrays(GL_TRIANGLES, 0, 6);
	glDisableVertexAttribArray(program.positionAttribute);
}

// Draws the ball at its current position
void drawBall(glm::mat4& modelMatrix, ball& ball) {
	modelMatrix = glm::mat4(1.0f);
	modelMatrix = glm::translate(modelMatrix, glm::vec3(ball.position[0], ball.position[1], ball.position[2]));
	program.SetModelMatrix(modelMatrix);
	program.SetColor(1.0f, 1.0f, 1.0f, 1.0f);

	glVertexAttribPointer(program.positionAttribute, 2, GL_FLOAT, false, 0, ball.vertices);
	glEnableVertexAttribArray(program.positionAttribute);
	glDrawArrays(GL_TRIANGLES, 0, 6);
	glDisableVertexAttribArray(program.positionAttribute);
}

// Draws the background of the game
void drawBackground(glm::mat4 modelMatrix, float color[]) {
	float vh = borderHeight / 2;
	float solid_line_vertices[12] = { -1.777f, vh, -1.777f, -vh, 1.777f, vh, 1.777f, -vh, -1.777f, -vh, 1.777f, vh };
	float dotted_line_vertices[12] = { -vh, vh, -vh, -vh, vh, vh, vh, -vh, -vh, -vh, vh, vh };
	program.SetColor(color[0], color[1], color[2], color[3]);

	// Drawing top border
	modelMatrix = glm::mat4(1.0f);
	modelMatrix = glm::translate(modelMatrix, glm::vec3(0.0f, 0.95f, 1.0f));
	program.SetModelMatrix(modelMatrix);
	glVertexAttribPointer(program.positionAttribute, 2, GL_FLOAT, false, 0, solid_line_vertices);
	glEnableVertexAttribArray(program.positionAttribute);
	glDrawArrays(GL_TRIANGLES, 0, 6);
	glDisableVertexAttribArray(program.positionAttribute);
	
	// Drawing bottom border
	modelMatrix = glm::mat4(1.0f);
	modelMatrix = glm::translate(modelMatrix, glm::vec3(0.0f, -0.95f, 1.0f));
	program.SetModelMatrix(modelMatrix);
	glVertexAttribPointer(program.positionAttribute, 2, GL_FLOAT, false, 0, solid_line_vertices);
	glEnableVertexAttribArray(program.positionAttribute);
	glDrawArrays(GL_TRIANGLES, 0, 6);
	glDisableVertexAttribArray(program.positionAttribute);

	// Drawing center line from center of window
	for (int i = 0; i < 5; ++i) {
		modelMatrix = glm::mat4(1.0f);
		modelMatrix = glm::translate(modelMatrix, glm::vec3(0.0f, i * 0.2f, 1.0f));
		program.SetModelMatrix(modelMatrix);
		glVertexAttribPointer(program.positionAttribute, 2, GL_FLOAT, false, 0, dotted_line_vertices);
		glEnableVertexAttribArray(program.positionAttribute);
		glDrawArrays(GL_TRIANGLES, 0, 6);
		glDisableVertexAttribArray(program.positionAttribute);
		
		modelMatrix = glm::mat4(1.0f);
		modelMatrix = glm::translate(modelMatrix, glm::vec3(0.0f, i * -0.2f, 1.0f));
		program.SetModelMatrix(modelMatrix);
		glVertexAttribPointer(program.positionAttribute, 2, GL_FLOAT, false, 0, dotted_line_vertices);
		glEnableVertexAttribArray(program.positionAttribute);
		glDrawArrays(GL_TRIANGLES, 0, 6);
		glDisableVertexAttribArray(program.positionAttribute);
	}
}

// Helper functin for pdadle and ball collisions
bool collidesWithEdge(paddle& p, ball& b) {
	return abs(b.position[0] - p.position[0]) <= (b.length / 2 + p.width / 2) && abs(b.position[1] - p.position[1]) <= (b.length / 2 + p.height / 2);
}

// Helper function for checking collisions between the ball and top/bottom border
bool collidesWithBorder(ball& b) {
	return b.position[1] + b.length / 2 >= topBorder - 0.1f || b.position[1] - b.length / 2 <= bottomBorder + 0.1f;
}

// Updates the velocity of the ball on collisions between the ball and paddles/borders
void checkBallCollision(paddle& p1, paddle& p2, ball& b) {	
	if (collidesWithEdge(p1, b) && b.velocity_x < 0)
		b.velocity_x = 0.75f;
	else if (collidesWithEdge(p2, b) && b.velocity_x > 0)
		b.velocity_x = -0.75f;
	
	if ((collidesWithBorder(b) || collidesWithEdge(p1, b) && b.position[1] < p1.position[1] || collidesWithEdge(p2, b) && b.position[1] < p2.position[1]) && b.velocity_y > 0)
		b.velocity_y = -0.75f;
	else if ((collidesWithBorder(b) || collidesWithEdge(p1, b) && b.position[1] > p1.position[1] || collidesWithEdge(p2, b) && b.position[1] < p2.position[1]) && b.velocity_y < 0)
		b.velocity_y = 0.75f;
}

// Updates the positions of all entities using their current positions and their velocities
void updatePositions(paddle& p1, paddle& p2, ball& b, float elapsed) {
	p1.position[1] += p1.speed * elapsed;
	p2.position[1] += p2.speed * elapsed;
	b.position[0] += b.velocity_x * elapsed;
	b.position[1] += b.velocity_y * elapsed;
}

// Resets all game entities to their initial positions
void resetGame(paddle& p1, paddle& p2, ball& b) {
	p1.position[1] = 0.0f;
	p2.position[1] = 0.0f;
	b = ball();
}

// Setup function to create the window
void setup() {
	SDL_Init(SDL_INIT_VIDEO);
	displayWindow = SDL_CreateWindow("Pong", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 1280, 720, SDL_WINDOW_OPENGL);
	SDL_GLContext context = SDL_GL_CreateContext(displayWindow);
	SDL_GL_MakeCurrent(displayWindow, context);

#ifdef _WINDOWS
	glewInit();
#endif

	glViewport(0, 0, 1280, 720);
	program.Load(RESOURCE_FOLDER"vertex.glsl", RESOURCE_FOLDER"fragment.glsl");
	srand(SDL_GetTicks() % 100);

	glm::mat4 modelMatrix = glm::mat4(1.0f);
	glm::mat4 viewMatrix = glm::mat4(1.0f);

	projectionMatrix = glm::ortho(-1.777f, 1.777f, -1.0f, 1.0f, -1.0f, 1.0f);
	program.SetProjectionMatrix(projectionMatrix);
	program.SetViewMatrix(viewMatrix);
}

// Renders the entities and background of the game
void render(glm::mat4 mm, paddle& p1, paddle& p2, ball& b, float p1Won, float p2Won) {
	if (p1Won) {
		drawBackground(mm, p1.color);
	}
	else if (p2Won) {
		drawBackground(mm, p2.color);
	}
	else {
		drawBackground(mm, COLOR_WHITE);
	}
	drawPaddle(mm, p1);
	drawPaddle(mm, p2);
	drawBall(mm, b);
}

int main(int argc, char *argv[])
{
	setup();
	paddle playerOne(p1_pos, p1_color);
	paddle playerTwo(p2_pos, p2_color);
	bool p1Win = false;
	bool p2Win = false;
	ball theBall = ball();

	float lastFrameTicks = 0.0f;

    SDL_Event event;
    bool done = false;
    while (!done) {
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT || event.type == SDL_WINDOWEVENT_CLOSE) {
                done = true;
            }
        }

		float ticks = (float)SDL_GetTicks() / 1000.0f;
		float elapsed = ticks - lastFrameTicks;
		lastFrameTicks = ticks;

		checkBallCollision(playerOne, playerTwo, theBall);
		
		if (theBall.position[0] <= leftBorder + borderHeight / 2)
			p2Win = true;
		else if (theBall.position[0] >= rightBorder - (borderHeight / 2))
			p1Win = true;

		if (!p1Win && !p2Win) {
			updatePositions(playerOne, playerTwo, theBall, elapsed);
		}
			
		const Uint8 *keys = SDL_GetKeyboardState(NULL);	
	
		// Press W and S to move left paddle
		if (keys[SDL_SCANCODE_W] && playerOne.position[1] < topBorder - (playerOne.height / 2 + borderHeight) && !p1Win && !p2Win)
			playerOne.speed = 1.0f;
		else if (keys[SDL_SCANCODE_S] && playerOne.position[1] > bottomBorder + (playerTwo.height / 2 + borderHeight) && !p1Win && !p2Win)
			playerOne.speed = -1.0f;
		else {
			playerOne.speed = 0.0f;
		}

		// Press UP and DOWN to move right paddle
		if (keys[SDL_SCANCODE_UP] && playerTwo.position[1] < topBorder - (playerOne.height / 2 + borderHeight) && !p1Win && !p2Win)
			playerTwo.speed = 1.0f;
		else if (keys[SDL_SCANCODE_DOWN] && playerTwo.position[1] > bottomBorder + (playerTwo.height / 2 + borderHeight) && !p1Win && !p2Win)
			playerTwo.speed = -1.0f;
		else {
			playerTwo.speed = 0.0f;
		}
		
		// Press SPACE to reset the game when a winner has been determined
		if (keys[SDL_SCANCODE_SPACE]) {
			if (p1Win || p2Win) {
				resetGame(playerOne, playerTwo, theBall);
				p1Win = false;
				p2Win = false;
			}
		}

		glClear(GL_COLOR_BUFFER_BIT);
		render(modelMatrix, playerOne, playerTwo, theBall, p1Win, p2Win);
		SDL_GL_SwapWindow(displayWindow);
    }

    SDL_Quit();
    return 0;
}