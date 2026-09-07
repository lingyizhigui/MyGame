#include "raylib.h"
#include "raymath.h"
#include "tinyxml2.h"
#include <array>
#include <queue>
#include <map>
#include <iostream>
#include <string>
using std::array;
using std::string;
const int WINDOW_HEIGHT = 720;
const int WINDOW_WIDTH = 1280;
const int gameIndexW = 12;
const int gameIndexH = 6;
const int GameWidth = 64 * gameIndexW;
const int GameHeight = 64 * (gameIndexH + 3);
const int MaxVelocity = 10;
struct Ball {
	Vector2 pos;
	Vector2 speed;
	float width = 44;
	float height = 44;
	Ball(Vector2 _positon, Vector2 _speed) :pos(_positon), speed(_speed) {};
	void changeSpeed(float n) {
		speed = Vector2Scale(speed, n);
		if (Vector2Length(speed) > MaxVelocity) {
			speed = Vector2Normalize(speed);
			speed = Vector2Scale(speed, MaxVelocity);
		}
	}
	void go() {
		pos = Vector2Add(pos, speed);
	}
	Vector2 isCollide(Rectangle rect) const{
		Rectangle ballRect(pos.x - width / 2, pos.y - height / 2, width, height);
		float overlapX = std::min(rect.x + rect.width, ballRect.x + ballRect.width)- std::max(rect.x, ballRect.x);
		float overlapY = std::min(rect.y + rect.height, ballRect.y + ballRect.height) - std::max(rect.y, ballRect.y);
		if (overlapX <= 0 || overlapY <= 0) return{ 0.0f,0.0f };
		else if (overlapX < overlapY) return pos.x > rect.x + rect.width / 2 ? Vector2{ overlapX, 0.0f } : Vector2{ overlapX * -1, 0.0f };
		else return pos.y > rect.y + rect.height / 2 ? Vector2{ 0.0f,overlapY } : Vector2{ 0.0f,overlapY * -1 };
	}
	Vector2 isCollideInside(Rectangle rect) const {
		Rectangle ballRect(pos.x - width / 2, pos.y - height / 2, width, height);
		if (!(ballRect.x > rect.x && ballRect.x + ballRect.width < rect.x + rect.width &&
			ballRect.y > rect.y && ballRect.y + ballRect.height < rect.y + rect.height)) {
			float overX = pos.x > rect.x + rect.width / 2
				? std::min(rect.x + rect.width - (ballRect.x + ballRect.width), 0.0f)
				: std::max(rect.x - ballRect.x, 0.0f);
			float overY = pos.y > rect.y + rect.height / 2
				? std::min(rect.y + rect.height - (ballRect.y + ballRect.height), 0.0f)
				: std::max(rect.y - ballRect.y, 0.0f);
			if (std::abs(overX) > std::abs(overY)) return{ overX,0.0f };
			else return{ 0.0f,overY };
		}
		return { 0,0 };
	}
	void collide(Vector2 plane) {
		plane = Vector2Normalize(plane);
		float temp = Vector2DotProduct(plane, speed);
		if (temp > 0) {
			pos = Vector2Add(pos, Vector2Scale(plane, Vector2Length(speed)));
		}
		else {
			speed = Vector2Subtract(speed, Vector2Scale(plane, 2 * temp));
			if (Vector2Length(speed) > MaxVelocity) {
				speed = Vector2Normalize(speed);
				speed = Vector2Scale(speed, MaxVelocity);
			}
		}
	}
};
int main() {
	tinyxml2::XMLDocument doc;
	if (doc.LoadFile("./resources/Spritesheet/spritesheet_double.xml") != tinyxml2::XML_SUCCESS) {
		std::cerr << "¼ÓÔØÊ§°Ü" << doc.ErrorStr() << std::endl;
		return -1;
	}
	std::map<string, Rectangle> posMap;
	for (tinyxml2::XMLElement* sub = (doc.RootElement()->FirstChildElement("SubTexture"));
		sub != nullptr;
		sub = sub->NextSiblingElement("SubTexture")) {
		const char* name = sub->Attribute("name");
		float x = sub->FloatAttribute("x");
		float y = sub->FloatAttribute("y");
		float w = sub->FloatAttribute("width");
		float h = sub->FloatAttribute("height");
		posMap[name] = { x,y,w,h };
	}

	InitWindow(WINDOW_WIDTH, WINDOW_HEIGHT, "My game");
	InitAudioDevice();
	//SetTraceLogLevel(LOG_WARNING);
	int fps = 60;
	SetTargetFPS(fps);
	int gameTop = (WINDOW_HEIGHT - GameHeight) / 2;
	int gameLeft = (WINDOW_WIDTH - GameWidth) / 2;
	struct Brick {
		char status = 0;
		string color() const{
			if (status == 1) return "grey";
			else if (status == 2) return "green";
			else if (status == 3) return "blue";
			else if (status == 4) return "yellow";
			else if (status == 5) return "red";
			else if (status == 6) return "purple";
			else if (status == 7) return "grey";
		}
	};
	array<array<Brick, gameIndexW>, gameIndexH> statusBricks;
	for (auto& i : statusBricks) {
		for (auto& j : i) j.status = static_cast<char>(GetRandomValue(0, 7));
	}

	Music music = LoadMusicStream("./resources/music/The Last Encounter Medium Loop.wav");
	Sound winSound = LoadSound("./resources/music/Won!.wav");
	std::vector<Ball> balls;
	array<Sound, 8> breakSounds;
	int breakSoundIndex = 0;
	for (; breakSoundIndex < breakSounds.size(); breakSoundIndex++) {
		breakSounds[breakSoundIndex] = LoadSound("./resources/music/break_block_3.wav");
	}
	Texture2D allImg = LoadTexture("./resources/Spritesheet/spritesheet_double.png");
	Rectangle mousePath = posMap.find("pointer_glossy.png")->second;
	Rectangle paddlePath= posMap.find("paddleBlu.png")->second;
	Rectangle ballPath = posMap.find("ballGrey.png")->second;
	PlayMusicStream(music);
	bool gameStart = false;
	bool gameOver = false;
	bool gameWin = false;
	int hsv = 0;
	int currentWin = 0;
	array<string,3> currentWinStr = { "Current","Win","Streak"};
	while (!WindowShouldClose()) {
		hsv = (hsv + 2) % 360;
		UpdateMusicStream(music);
		Vector2 mousePos = GetMousePosition();
		mousePos.x = std::max(float(gameLeft), mousePos.x);
		mousePos.x = std::min(float(gameLeft + GameWidth), mousePos.x);
		mousePos.y = std::max(float(gameTop + GameHeight - 64*2), mousePos.y);
		mousePos.y = std::min(float(gameTop + GameHeight), mousePos.y);
		if (IsKeyPressed(KEY_F)) {
			fps = fps == 60 ? 120 : 60;
			SetTargetFPS(fps);
		}
		if (IsKeyPressed(KEY_B)) {
			Vector2 v{ GetRandomValue(1,7),GetRandomValue(-5,-3) };
			balls.push_back(Ball(Vector2(mousePos.x, mousePos.y - 22 - 24), v));
		}
		if (!gameStart) {
			if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
				gameStart = true;
				Vector2 v{ GetRandomValue(1,7),GetRandomValue(-5,-3) };
				balls.push_back(Ball(Vector2(mousePos.x, mousePos.y-22-24), v));
			}
		}
		else if (IsKeyPressed(KEY_R)) {
			if (gameWin || gameOver) {
				gameStart = false;
				gameOver = false;
				gameWin = false;
				balls.clear();
				for (auto& i : statusBricks) {
					for (auto& j : i) j.status = static_cast<char>(GetRandomValue(0, 7));
				}
			}
		}
		else {
			Vector2 addBall = { 0,0 };
			for (auto it = balls.begin();it!=balls.end();) {
				Ball& b = *it;
				b.go();
				if (b.pos.y > gameTop + GameHeight - 22 - 5) {
					it = balls.erase(it);
					if (balls.size() == 0 && addBall == Vector2(0, 0)) {
						gameOver = true;
						currentWin = 0;
					}
					continue;
				}
				bool collideHappen = false;
				int total = 0;
				for (int i = 0; i < gameIndexH; i++) {
					for (int j = 0; j < gameIndexW; j++) {
						if (statusBricks[i][j].status) {
							total++;
							if(!collideHappen){
								Rectangle r(gameLeft + j * 64, gameTop + i * 64, 64, 64);
								if (Vector2 t = b.isCollide(r); t != Vector2{ 0,0 }) {
									b.collide(t);
									statusBricks[i][j].status--;
									if (statusBricks[i][j].status == 6) {
										statusBricks[i][j].status = 0;
										addBall = Vector2(gameLeft + j * 64 + 32, gameTop + i * 64 + 32);
									}
									if (statusBricks[i][j].status == 0) {
										total--;
										breakSoundIndex = (breakSoundIndex + 1) % breakSounds.size();
										PlaySound(breakSounds[breakSoundIndex]);
									}
									collideHappen = true;
								}
							}
						}
					}
				}
				if (total == 0 && !gameWin) {
					gameWin = true;
					PlaySound(winSound);
					currentWin++;
					break;
				}
				if (!collideHappen) {
					Rectangle r(mousePos.x - 104, mousePos.y - 24, 208, 48);
					if (Vector2 t = b.isCollide(r); t != Vector2{ 0,0 }) {
						b.collide(t);
						collideHappen = true;
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

		BeginDrawing();
		//ClearBackground(RAYWHITE);
		DrawRectangle(0, 0, GetScreenWidth(), GetScreenHeight(), ColorAlpha(BLACK, 0.3f));
		DrawRectangleLines(gameLeft - 64, gameTop -64, GameWidth + 64 * 2, GameHeight +64, ORANGE);
		DrawRectangle(gameLeft - 64, gameTop + GameHeight - 5, GameWidth + 64 * 2, 5, RED);
		DrawFPS(0, 0);
		for (int i = 0; i < gameIndexH; i++) {
			for (int j = 0; j < gameIndexW; j++) {
				if (statusBricks[i][j].status) {
					string path = string("element_") + statusBricks[i][j].color() + "_square.png";
					const auto& r = posMap.find(path)->second;
					Vector2 pos = { gameLeft + j * 64,gameTop + i * 64 };
					if (statusBricks[i][j].status == 7) {
						Color co = ColorFromHSV(hsv, 0.9, 1);
						DrawTextureRec(allImg, r, pos, co);
					}
					else DrawTextureRec(allImg, r, pos, WHITE);
				}
			}
		}
		for (Ball& b : balls) {
			DrawTextureRec(allImg, ballPath,
				{ b.pos.x - b.width / 2,b.pos.y - b.height / 2 }, WHITE);
		}
		DrawTextureRec(allImg, paddlePath, { mousePos.x - 104,mousePos.y - 24 }, WHITE);
		if (!gameStart) {
			const char* welcome = "Welcome! Press left button to start";
			int width = MeasureText(welcome, 40);
			DrawText(welcome, WINDOW_WIDTH / 2 - width / 2, WINDOW_HEIGHT - 60, 40, RAYWHITE);
		}
		else if (gameWin) {
			const char* over = "CONGRATULATION! YOU WIN! Press R for another";
			int width = MeasureText(over, 40);
			DrawText(over, WINDOW_WIDTH / 2 - width / 2, WINDOW_HEIGHT - 60, 40, GOLD);
		}
		else if (gameOver) {
			const char* over = "NO WAY ! YOU LOSE! Press R for another";
			int width = MeasureText(over, 40);
			DrawText(over, WINDOW_WIDTH / 2 - width / 2, WINDOW_HEIGHT - 60, 40, PINK);
		}
		for (int i = 0;i<=currentWinStr.size();i++) {
			if (i < currentWinStr.size() ) {
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
		EndDrawing();
	}
	UnloadMusicStream(music);
	UnloadTexture(allImg);
	return 0;
}