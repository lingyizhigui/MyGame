#pragma once
#include <array>
#include <algorithm>
#include <format>
#include "elements.h"
#include "framework.h"
class GameLevel {
private:
	static constexpr char gameIndexW = 12;
	static constexpr char gameIndexH = 6;
	static constexpr int GameWidth = 64 * gameIndexW;
	static constexpr int GameHeight = 64 * (gameIndexH + 4);
	const Window& window;
	int gameTop;
	int gameLeft;
	std::array<std::array<Brick, 12>, 6> bricks;
	std::vector<Ball> balls;
	std::vector<Particle> particles;
	Paddle paddle;
	bool gameStart = false;
	bool gameOver = false;
	bool gameWin = false;
	TextCenter gameStartText;
	TextCenter gameOverText;
	TextCenter gameWinText;
	double startTime = 0.0;
	double gameTime = 0.0;
	double bestTime = 0.0;
	int currentWin = 0;
	Vector2 mousePos = { 0,0 };
	int hsv = 0;
	std::array<std::string, 3> currentWinStr = { "Current","Win","Streak" };
public:
	void resetBricks() {
		for (int i = 0; i < gameIndexH; i++) {
			for (int j = 0; j < gameIndexW; j++) {
				Brick& b = bricks[i][j];
				b.indexX = j;
				b.indexY = i;
				b.pos = Vector2(gameLeft + j * 64, gameTop + i * 64);
				b.resetStatus(getWeightedRandom(b.Weight));
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
		mousePos.y = std::max(float(gameTop + GameHeight - 64 * 3), mousePos.y);
		mousePos.y = std::min(float(gameTop + GameHeight), mousePos.y);
	}
	void readyStartGame() {
		if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
			gameStart = true;
			startTime = GetTime();
			Vector2 v{ GetRandomValue(10,10),GetRandomValue(-5,-3) };
			balls.push_back(Ball(Vector2(mousePos.x, mousePos.y - 22 - 24), v));
		}
	}
	void breakIncident(Brick& bri, std::vector<Vector2>& newBallPos) {
		if (!bri.status) return;
		for (int genPat = 0; genPat < bri.ParticleNum[bri.status]; genPat++) {
			particles.push_back(Particle(Vector2(bri.pos.x + 32, bri.pos.y + 32)));
		}
		char status = bri.status;
		bri.status = 0;
		window.breakSound->play();
		if (status == bri.kind::NewBall) {
			newBallPos.push_back({ bri.pos.x + 32,bri.pos.y + 32 });
		}
		else if (status == bri.kind::Bomb) {
			for (char i = bri.indexX - 1; i <= bri.indexX + 1; i++) {
				for (char j = bri.indexY - 1; j <= bri.indexY + 1; j++) {
					if (i < 0 || i >= gameIndexW) continue;
					if (j < 0 || j >= gameIndexH) continue;
					breakIncident(bricks[j][i], newBallPos);
				}
			}
		}
	}
	bool collideWithBricks(Ball& ball,std::vector<Vector2>& addBallPos) {
		for (auto& vec:bricks) {
			for (Brick& brick:vec) {
				if (brick.status) {
					Rectangle r(brick.pos.x, brick.pos.y, 64, 64);
					if (Vector2 t = ball.isCollideAsRect(r); t != Vector2{ 0,0 }) {
						ball.collide(t);
						brick.hp--;
						if (Vector2Length(ball.speed) > ball.FastVelocity) {
							if (brick.hp) brick.hp--;
						}
						if (brick.hp == 0) breakIncident(brick, addBallPos);
						return true;
					}
				}
			}
		}
		return false;
	}
	bool collideWithPaddle(Ball& b) {
		const int PICIES = 8;
		for (int piece = 0; piece < PICIES; piece++) {
			Vector2 thatPos = Vector2Subtract(paddle.pos, paddle.speed / PICIES * (PICIES - piece));
			Rectangle r(thatPos.x, thatPos.y, paddle.width, paddle.height);
			if (Vector2 t = b.isCollideRotated(r, paddle.angle); t != Vector2{ 0,0 }) {
				Vector2 normT = Vector2Normalize(t);
				b.collide(t, Vector2DotProduct(normT, paddle.speed));
				b.move(paddle.speed / PICIES * (PICIES - piece));
				return true;
			}
		}
		return false;
	}
	void update() {
		hsv = (hsv + 2) % 360;
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
			if(!gameWin) gameTime = GetTime() - startTime;
			for (int i=0;i<particles.size();) {
				if (!particles[i].life) {
					std::swap(particles[i], particles.back());
					particles.pop_back();
				}
				else{
					particles[i].go();
					i++;
				}
			}
			std::vector<Vector2> addBall;
			for (auto it = balls.begin(); it != balls.end();) {
				Ball& b = *it;
				b.go();
				if (b.pos.y > gameTop + GameHeight - 22 - 5) {
					it = balls.erase(it);
					if (balls.size() == 0 && addBall.size() == 0 && !gameWin) {
						gameOver = true;
						currentWin = 0;
					}
					continue;
				}
				bool collideHappen = collideWithBricks(b, addBall);
				int total = 0;
				for (const auto& a : bricks) {
					for (const auto& br : a) {
						if (br.status) total++;
					}
				}
				if (!total && !gameWin) {
					gameWin = true;
					window.winSound->play();
					currentWin++;
					if (!bestTime) bestTime = gameTime;
					else if (gameTime < bestTime) bestTime = gameTime;
					break;
				}
				if (!collideHappen) collideHappen = collideWithPaddle(b);
				if (!collideHappen) {
					Rectangle r(gameLeft - 64, gameTop - 64, GameWidth + 64 * 2, GameHeight + 64);
					if (Vector2 t = b.isCollideInside(r); t != Vector2{ 0,0 }) {
						b.collide(t);
						collideHappen = true;
					}
				}
				it++;
			}
			if (!addBall.empty()) {
				for (Vector2& newBallPoss : addBall) {
					Vector2 v{ GetRandomValue(3,7),GetRandomValue(-5,-3) };
					balls.push_back(Ball(newBallPoss, v));
				}
			}
		}
	}
	void basicDraw() const {
		ClearBackground(BLACK);
		DrawRectangleLines(gameLeft - 64, gameTop - 64, GameWidth + 64 * 2, GameHeight + 64, ORANGE);
		DrawRectangle(gameLeft - 64, gameTop + GameHeight - 5, GameWidth + 64 * 2, 5, RED);
		DrawFPS(0, 0);
	}
	void drawBricks() {
		Color color= ColorFromHSV(hsv, 0.9, 1);
		for (int i = 0; i < gameIndexH; i++) {
			for (int j = 0; j < gameIndexW; j++) {
				if (bricks[i][j].status) bricks[i][j].draw(window,color);
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
		for (auto& p: particles) {
			p.draw();
		}
	}
	std::string timeToString(double time) const{
		int _minute = int(time / 60);
		double _second = time - _minute * 60;
		std::string minute = std::format("{:02d}", _minute);
		std::string second = std::format("{:05.2f}", _second);
		return minute + ":" + second;
	}
	void drawTexts() const {
		if (!gameStart) gameStartText.draw();
		if (gameOver) gameOverText.draw();
		if (gameWin) gameWinText.draw();
		for (int i = 0; i <= currentWinStr.size(); i++) {
			if (i < currentWinStr.size()) {
				DrawText(currentWinStr[i].c_str(),
					MeasureText("Current", 30) / 2 - MeasureText(currentWinStr[i].c_str(), 30) / 2 + 35,
					(i + 1) * 30, 30, RAYWHITE);
			}
			else {
				Color co = currentWin ? GOLD : RAYWHITE;
				DrawText(std::to_string(currentWin).c_str(),
					MeasureText("Current", 30) / 2 - MeasureText(std::to_string(currentWin).c_str(), 60) / 2 + 35,
					(i + 1) * 30, 60, co);
			}
		}
		Color co = bestTime ? GOLD : RAYWHITE;
		DrawText("Best Time", 25, 200, 30, RAYWHITE);
		DrawText(timeToString(bestTime).c_str(),
			MeasureText("Best Time", 30) / 2 - MeasureText(timeToString(bestTime).c_str(), 40) / 2 + 25,
			230, 40, co);

		DrawText(timeToString(gameTime).c_str(),
			window.WINDOW_WIDTH / 2 - MeasureText("00:00.0", 60) / 2,
			gameTop + GameHeight + 5, 60, RAYWHITE);
	}
};