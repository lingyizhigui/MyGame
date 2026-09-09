#pragma once
#include <array>
#include <algorithm>
#include "elements.h"
#include "framework.h"
class GameLevel {
private:
	static constexpr int gameIndexW = 12;
	static constexpr int gameIndexH = 6;
	static constexpr int GameWidth = 64 * gameIndexW;
	static constexpr int GameHeight = 64 * (gameIndexH + 4);
	const Window& window;
	int gameTop;
	int gameLeft;
	std::array<std::array<Brick, 12>, 6> bricks;
	std::vector<Ball> balls;
	std::vector<std::array<Particle,16>> particles;
	Paddle paddle;
	bool gameStart = false;
	bool gameOver = false;
	bool gameWin = false;
	TextCenter gameStartText;
	TextCenter gameOverText;
	TextCenter gameWinText;
	int currentWin = 0;
	Vector2 mousePos = { 0,0 };
	std::array<std::string, 3> currentWinStr = { "Current","Win","Streak" };
public:
	void resetBricks() {
		for (int i = 0; i < gameIndexH; i++) {
			for (int j = 0; j < gameIndexW; j++) {
				bricks[i][j].pos = Vector2(gameLeft + j * 64, gameTop + i * 64);
				bricks[i][j].status = GetRandomValue(0, 1);
				bricks[i][j].hp = 4;
			}
		}
	}
	GameLevel(const Window& _window) :
		window(_window),
		gameStartText("Welcome! Press left button to start", _window, RAYWHITE, 60),
		gameOverText("NO WAY ! YOU LOSE!\n Press R for another", _window, RAYWHITE, 60),
		gameWinText("CONGRATULATION! YOU WIN!\n      Press R for another", _window, RAYWHITE, 60)
	{
		gameTop = (window.height - GameHeight) / 2;
		gameLeft = (window.width - GameWidth) / 2;
		balls.reserve(8);
		particles.reserve((size_t)20 * 8);
		resetBricks();
	}
	void mousePosRedirect() {
		mousePos.x = std::max(float(gameLeft), mousePos.x);
		mousePos.x = std::min(float(gameLeft + GameWidth), mousePos.x);
		mousePos.y = std::max(float(gameTop + GameHeight - 64 * 3.5f), mousePos.y);
		mousePos.y = std::min(float(gameTop + GameHeight), mousePos.y);
	}
	void readyStartGame() {
		if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
			gameStart = true;
			Vector2 v{ GetRandomValue(10,10),GetRandomValue(-5,-3) };
			balls.push_back(Ball(Vector2(mousePos.x, mousePos.y - 22 - 24), v));
		}
	}
	void update() {
		mousePos = GetMousePosition();
		mousePosRedirect();
		paddle.update(mousePos);
		if (IsKeyPressed(KEY_B)) {
			Vector2 v{ GetRandomValue(1,7),GetRandomValue(-5,-3) };
			balls.push_back(Ball(Vector2(mousePos.x, mousePos.y - 22 - 24), v));
		}
		if (!gameStart) readyStartGame();
		else if (IsKeyPressed(KEY_R) && (gameOver || gameWin)) {
			gameStart = false;
			gameOver = false;
			gameWin = false;
			balls.clear();
			resetBricks();
		}
		else {
			auto timeOutParticles = particles.end();
			for (auto it = particles.begin();it!=particles.end();it++) {
				for (auto& p : *it) {
					if (!p.life) {
						timeOutParticles = it;
						break;
					}
					p.go();
				}
			}
			if (timeOutParticles != particles.end()) {
				particles.erase(timeOutParticles);
			}
			Vector2 addBall = { 0,0 };
			for (auto it = balls.begin(); it != balls.end();) {
				Ball& b = *it;
				b.go();
				if (b.pos.y > gameTop + GameHeight - 22 - 5) {
					it = balls.erase(it);
					if (balls.size() == 0 && addBall == Vector2(0, 0) && !gameWin) {
						gameOver = true;
						currentWin = 0;
					}
					continue;
				}
				bool collideHappen = false;
				int total = 0;
				for (int i = 0; i < gameIndexH; i++) {
					for (int j = 0; j < gameIndexW; j++) {
						if (bricks[i][j].status) {
							total++;
							if (!collideHappen) {
								Rectangle r(gameLeft + j * 64, gameTop + i * 64, 64, 64);
								if (Vector2 t = b.isCollide(r); t != Vector2{ 0,0 }) {
									b.collide(t);
									bricks[i][j].hp--;
									if (bricks[i][j].status == 6) {
										bricks[i][j].status = 0;
										addBall = Vector2(gameLeft + j * 64 + 32, gameTop + i * 64 + 32);
									}
									if (bricks[i][j].hp == 0) {
										bricks[i][j].status = 0;
										total--;
										window.breakSound->play();
										particles.push_back({
											Particle(Vector2(gameLeft + j * 64 + 32, gameTop + i * 64 + 32)),
											Particle(Vector2(gameLeft + j * 64 + 32, gameTop + i * 64 + 32)),
											Particle(Vector2(gameLeft + j * 64 + 32, gameTop + i * 64 + 32)),
											Particle(Vector2(gameLeft + j * 64 + 32, gameTop + i * 64 + 32)),
											Particle(Vector2(gameLeft + j * 64 + 32, gameTop + i * 64 + 32)),
											Particle(Vector2(gameLeft + j * 64 + 32, gameTop + i * 64 + 32)),
											Particle(Vector2(gameLeft + j * 64 + 32, gameTop + i * 64 + 32)),
											Particle(Vector2(gameLeft + j * 64 + 32, gameTop + i * 64 + 32)),
											Particle(Vector2(gameLeft + j * 64 + 32, gameTop + i * 64 + 32)),
											Particle(Vector2(gameLeft + j * 64 + 32, gameTop + i * 64 + 32)),
											Particle(Vector2(gameLeft + j * 64 + 32, gameTop + i * 64 + 32)),
											Particle(Vector2(gameLeft + j * 64 + 32, gameTop + i * 64 + 32)),
											Particle(Vector2(gameLeft + j * 64 + 32, gameTop + i * 64 + 32)),
											Particle(Vector2(gameLeft + j * 64 + 32, gameTop + i * 64 + 32)),
											Particle(Vector2(gameLeft + j * 64 + 32, gameTop + i * 64 + 32)),
											Particle(Vector2(gameLeft + j * 64 + 32, gameTop + i * 64 + 32)),
											});
									}
									collideHappen = true;
								}
							}
						}
					}
				}
				if (total == 0 && !gameWin) {
					gameWin = true;
					window.winSound->play();
					currentWin++;
					break;
				}
				if (!collideHappen) {
					const int PICIES = 8;
					for (int piece = 0; piece < PICIES; piece++) {
						Vector2 thatPos = Vector2Subtract(paddle.pos, paddle.speed / PICIES * (PICIES - piece));
						Rectangle r(thatPos.x, thatPos.y, paddle.width, paddle.height);
						if (Vector2 t = b.isCollideRotated(r, paddle.angle); t != Vector2{ 0,0 }) {
							Vector2 normT = Vector2Normalize(t);
							b.collide(t, Vector2DotProduct(normT, paddle.speed));
							b.move(paddle.speed / PICIES * (PICIES - piece));
							collideHappen = true;
							break;
						}
					}
					
				}
				if (!collideHappen) {
					Rectangle r(gameLeft - 64, gameTop - 64, GameWidth + 64 * 2, GameHeight + 64);
					if (Vector2 t = b.isCollideInside(r); t != Vector2{ 0,0 }) {
						b.collide(t);
						collideHappen = true;
					}
				}
				it++;
			}
			if (addBall != Vector2(0, 0)) {
				Vector2 v{ GetRandomValue(1,7),GetRandomValue(-5,-3) };
				balls.push_back(Ball(addBall, v));
			}
		}
	}
	void basicDraw() const {
		//DrawRectangle(0, 0, GetScreenWidth(), GetScreenHeight(), ColorAlpha(BLACK, 0.3f));
		ClearBackground(BLACK);
		DrawRectangleLines(gameLeft - 64, gameTop - 64, GameWidth + 64 * 2, GameHeight + 64, ORANGE);
		DrawRectangle(gameLeft - 64, gameTop + GameHeight - 5, GameWidth + 64 * 2, 5, RED);
		DrawFPS(0, 0);
	}
	void drawBricks() const {
		for (int i = 0; i < gameIndexH; i++) {
			for (int j = 0; j < gameIndexW; j++) {
				if (bricks[i][j].status) bricks[i][j].draw(window);
			}
		}
	}
	void drawBalls() const {
		Rectangle ballPath = window.posMap.find("ballGrey.png")->second;
		for (const Ball& b : balls) {
			if (Vector2Length(b.speed) > b.FastVelocity) {
				int index = b.index;
				for (int i = 0; i < b.tails.size(); i++) {
					DrawCircle(b.tails[index].x,
						b.tails[index].y,
						b.radius / b.tails.size() * i,
						{ 255,255,255,unsigned char(255 / b.tails.size() * i) });
					index = (index + 1) % b.tails.size();
				}
			}
			DrawTextureRec(window.allImg, ballPath,
				{ b.pos.x - b.width / 2,b.pos.y - b.height / 2 }, WHITE);
		}
	}
	void drawPaddle() const {
		Rectangle paddlePath = window.posMap.find("paddleBlu.png")->second;
		DrawTexturePro(window.allImg, paddlePath,
			{ paddle.pos.x + paddle.width / 2,paddle.pos.y + paddle.height / 2,(float)paddle.width,(float)paddle.height },
			Vector2(paddle.width / 2, paddle.height / 2), paddle.angle, WHITE);
	}
	void drawParticles() const {
		for (auto& arr: particles) {
			for (auto& p : arr) {
				p.draw();
			}
		}
	}
	void drawTexts() const {
		if (!gameStart) gameStartText.draw();
		if (gameOver) gameOverText.draw();
		if (gameWin) gameWinText.draw();
		for (int i = 0; i <= currentWinStr.size(); i++) {
			if (i < currentWinStr.size()) {
				DrawText(currentWinStr[i].c_str(),
					MeasureText("Current", 30) / 2 - MeasureText(currentWinStr[i].c_str(), 30) / 2 + 30,
					(i + 1) * 30, 30, RAYWHITE);
			}
			else {
				Color co = currentWin ? GOLD : RAYWHITE;
				DrawText(std::to_string(currentWin).c_str(),
					MeasureText("Current", 30) / 2 - MeasureText(std::to_string(currentWin).c_str(), 60) / 2 + 30,
					(i + 1) * 30, 60, co);
			}
		}
	}
};