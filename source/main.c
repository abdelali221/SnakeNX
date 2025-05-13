#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <switch.h>
#include <SDL.h>
#include <SDL_mixer.h>
#include <time.h>

const uint8_t HOR_OFFSET = 0;
const uint8_t VER_OFFSET = 0;
const uint8_t COLS = (80 + HOR_OFFSET);
const uint8_t ROWS = (35 + VER_OFFSET);

bool Resume = false;
bool GenBall = true;
bool Start = false;
bool kDownButton = false;
bool BallEaten = false;
bool doPause = false;

int BallX, BallY, ANSBallX, ANSBallY;
uint SnakeX = (COLS/2);
uint SnakeY = ROWS/2;
int VSnakeX = 1;
int VSnakeY = 0;
uint Lives = 3;
uint Score = 0;
uint SnakeLength = 2;
uint counter = 0;
uint Speed;
uint SnakePOSbuffer[6000][2];

PadState pad;

void SystemInit() {
	consoleInit(NULL);

    // Configure our sUpported input layout: a single player with standard controller styles
    padConfigureInput(1, HidNpadStyleSet_NpadStandard);

    // Initialize the default gamepad (which reads handheld mode inputs as well as the first connected controller)
    padInitializeDefault(&pad);

    Result rc = romfsInit();
	if (R_FAILED(rc))
    	printf("romfsInit: %08X\n", rc);
	SDL_Init(SDL_INIT_AUDIO);
	Mix_Init(MIX_INIT_MP3);
	Mix_OpenAudio(44100, MIX_DEFAULT_FORMAT, MIX_DEFAULT_CHANNELS, 4096);
}

void CheckController() {
	padUpdate(&pad);
	u64 kDown = padGetButtonsDown(&pad);
	if (kDown & HidNpadButton_Minus) {
		consoleExit(NULL);
	} else if ((kDown & HidNpadButton_Up) && (VSnakeX != 0 || !Start) && !kDownButton) {
		kDownButton = true;
		if (!Start) {
			Start = true;
		}
		VSnakeY = -1;
		VSnakeX = 0;
	} else if ((kDown & HidNpadButton_Down) && (VSnakeX != 0 || !Start) && !kDownButton) {
		kDownButton = true;
		if (!Start) {
			Start = true;
		}
		VSnakeY = 1;
		VSnakeX = 0;
	} else if ((kDown & HidNpadButton_Left) && (VSnakeY != 0 || !Start) && !kDownButton) {
		kDownButton = true;
		if (!Start) {
			Start = true;
		}
		VSnakeX = -1;
		VSnakeY = 0;
	} else if ((kDown & HidNpadButton_Right) && (VSnakeY != 0 || !Start) && !kDownButton) {
		kDownButton = true;
		if (!Start) {
			Start = true;
		}
		VSnakeX = 1;
		VSnakeY = 0;
	} else if (kDown & HidNpadButton_Plus) {
		doPause = true; 
	}
}

void sleep(float delay) {
	float ticks = delay/20;
	float start = 0;
	while (start < ticks) {
		CheckController();
		start++;
		consoleUpdate(NULL);
	}
}

void POSCursor(uint8_t X, uint8_t Y) {
	printf("\x1b[%d;%dH", Y, X);
}

void RenderBorders(bool DELAY, bool PLAYSOUND) {
	Mix_Music *start = Mix_LoadMUS("romfs:/start.mp3");
	for (size_t Y = VER_OFFSET; Y <= ROWS; Y++) {
  
		for (size_t X = HOR_OFFSET; X <= COLS; X++) {
  
			if ( ( (X == HOR_OFFSET || X == COLS) && (Y >= VER_OFFSET && Y <= ROWS) )|| (Y == VER_OFFSET || Y == ROWS)) {
  				POSCursor(X, Y);
				if (DELAY) {
  					sleep(10);
				}
		  		printf("#");
  			}
		}
	}
	if (PLAYSOUND) {
		Mix_PlayMusic(start, 1);
	}	
}

void GenerateBall() {
	for (size_t i = 1; i <= SnakeLength; i++) {
		while (BallX < HOR_OFFSET + 1 || BallX > COLS || BallY < VER_OFFSET + 1 || BallY > ROWS || (BallX == SnakePOSbuffer[i][0] && BallY == SnakePOSbuffer[i][1]) || (BallX == ANSBallX && BallY == ANSBallY)) {
			BallX = HOR_OFFSET + 1 + rand() % (COLS - HOR_OFFSET - 1);
			BallY = VER_OFFSET + 1 + rand() % (ROWS - VER_OFFSET - 1);
		}
	}
	BallEaten = false;
	ANSBallX = BallX;
	ANSBallY = BallY;
	POSCursor(BallX, BallY);
	printf("O");
}

void RenderSnake() {
	POSCursor(SnakePOSbuffer[SnakeLength][0], SnakePOSbuffer[SnakeLength][1]);
	printf(" ");
	POSCursor(SnakeX, SnakeY);
	printf("#");	
	SnakePOSbuffer[0][0] = SnakeX;
	SnakePOSbuffer[0][1] = SnakeY;
	for (size_t i = SnakeLength; i > 0; i--) {
		SnakePOSbuffer[i][0] = SnakePOSbuffer[i - 1][0];
		SnakePOSbuffer[i][1] = SnakePOSbuffer[i - 1][1];
	}

}

void DifficultySelect() {
	Mix_Music *Select = Mix_LoadMUS("romfs:/select.mp3");
	Mix_Music *easy = Mix_LoadMUS("romfs:/easy.mp3");
	Mix_Music *medium = Mix_LoadMUS("romfs:/medium.mp3");
	Mix_Music *hard = Mix_LoadMUS("romfs:/hard.mp3");
	int Selection = 0;
	printf("\x1b[2J");
	POSCursor((COLS/4) - 1, (ROWS/2) - 2);
	printf("Choose the difficulty :");
	POSCursor(COLS/4, ROWS/2);
	printf("Easy");
	POSCursor(COLS/4, (ROWS/2) + 2);
	printf("Medium");
	POSCursor(COLS/4, (ROWS/2) + 4);
	printf("Hard");
	POSCursor((COLS/4) - 1, (ROWS/2) + Selection);
	printf(">");
	while(1) {
		padUpdate(&pad);
		u64 kDown = padGetButtonsDown(&pad);

		if ((kDown & HidNpadButton_Down) && Selection < 4) {
			POSCursor((COLS/4) - 1, (ROWS/2) + Selection);
			printf(" ");
			Selection = Selection + 2;
			POSCursor((COLS/4) - 1, (ROWS/2) + Selection);
			printf(">");
			Mix_PlayMusic(Select, 1);
		} else if ((kDown & HidNpadButton_Up) && Selection > 0) {
			POSCursor((COLS/4) - 1, (ROWS/2) + Selection);
			printf(" ");
			Selection = Selection - 2;
			POSCursor((COLS/4) - 1, (ROWS/2) + Selection);
			printf(">");
			Mix_PlayMusic(Select, 1);
		} else if (kDown & HidNpadButton_A) {
			printf("\x1b[2J");
			switch (Selection)
			{
				case 0:
					Speed = 500;
					Mix_PlayMusic(easy, 1);
				break;
				
				case 2:
					Speed = 250;
					Mix_PlayMusic(medium, 1);
				break;

				case 4:
					Speed = 100;
					Mix_PlayMusic(hard, 1);
				break;

				default:
				break;
			}
			return;
		} else if (kDown & HidNpadButton_Minus) {
			consoleExit(NULL);
		}
		consoleUpdate(NULL);
	}
}

void GameOver() {
	POSCursor(30, 13);
	printf("Game Over!");
	POSCursor(27, 15);
	printf("Your Score : %d", Score);
	POSCursor(14, 17);
	printf("Press Minus to exit or A to restart the game");
	while (true) {
		padUpdate(&pad);
		u64 kDown = padGetButtonsDown(&pad);
		if (kDown & HidNpadButton_Minus) {
			consoleExit(NULL);
		} else if (kDown & HidNpadButton_A) {
			printf("\x1b[2J");
			DifficultySelect();
			RenderBorders(true, true);
			return;
		}
		consoleUpdate(NULL);
	}

}

void Loose() {
	Mix_Music *lost = Mix_LoadMUS("romfs:/lost.mp3");
	Mix_Music *died = Mix_LoadMUS("romfs:/died.mp3");
	printf("\x1b[2J");
	GenBall = true;
	Start = false;
	for (size_t i = 1; i < 599; i++) {
		SnakePOSbuffer[i][0] = 0;
		SnakePOSbuffer[i][1] = 0;
	}
	SnakeX = (COLS/2);
	SnakeY = ROWS/2;
	VSnakeX = 0;
	VSnakeY = 0;
	SnakeLength = 2;
	if (Lives > 0) {
		Mix_PlayMusic(lost, 1);
		sleep(2000);
		RenderBorders(false, true);
		Lives--;
	} else {
		Mix_PlayMusic(died, 1);
		sleep(4000);
		GameOver();
		Lives = 3;
		Score = 0;
	}
}

void ManageSnakePos() {
	CheckController();
	if (SnakeX < HOR_OFFSET + 1 || SnakeX > COLS - 1 || SnakeY < VER_OFFSET + 1 || SnakeY > ROWS - 1) {
		Loose();
	}
	if (SnakeLength > 4) {
		for (size_t i = 4; i < SnakeLength + 1; i++) {
			if (SnakeX == SnakePOSbuffer[i][0] && SnakeY == SnakePOSbuffer[i][1]) {
				Loose();
			}
		}
	}
		
	if (SnakeX == BallX && SnakeY == BallY && !BallEaten) {
		Mix_Music *audio = Mix_LoadMUS("romfs:/increase.mp3");
		Mix_PlayMusic(audio, 1);
		Score++;
		SnakeLength++;
		BallEaten = true;
		GenBall = true;
	}
	SnakeX = SnakeX + VSnakeX;
	SnakeY = SnakeY + VSnakeY;
}

void PrintGameStats() {
	POSCursor(0, ROWS + 2);
	printf(" Score : %d \n", Score);
	printf(" Lives : %d ", Lives);
}

void Pause() {
	Mix_Music *pause = Mix_LoadMUS("romfs:/pause.mp3");
	Mix_Music *resume = Mix_LoadMUS("romfs:/resume.mp3");
	printf("\x1b[2J");
	POSCursor(30, 10);
	printf("Paused!");
	POSCursor(25, 12);
	printf("Press + to resume...");
	Mix_PlayMusic(pause, 1);
	while (1) {
		padUpdate(&pad);
		u64 kDown = padGetButtonsDown(&pad);
		if (kDown & HidNpadButton_Plus) {
			Mix_PlayMusic(resume, 1);
			printf("\x1b[2J");
			RenderBorders(false, false);
			for (size_t i = 1; i < SnakeLength; i++) {
				POSCursor(SnakePOSbuffer[i][0], SnakePOSbuffer[i][1]);
				printf("#");
			}
			if (!BallEaten && !GenBall) {
				POSCursor(BallX, BallY);
				printf("O");
			}			
			PrintGameStats();
			doPause = false;
			return;
		} else if (kDown & HidNpadButton_Minus) {
			consoleExit(NULL);
		}
		consoleUpdate(NULL);
	}
}

void RunGame() {
	sleep(Speed);
	if (doPause) {
		Pause();
	}
	PrintGameStats();
	if (counter < 4*(1000/Speed)) {
		counter++;
	} else {
		counter = 0;
	}
	if (counter == 3*(1000/Speed) && GenBall) {
		GenerateBall();
		GenBall = false;
	}
	ManageSnakePos();
	RenderSnake();
	kDownButton = false;
	consoleUpdate(NULL);
}

int main(int argc, char* argv[]) {
	srand(time(NULL));
	SystemInit();
	printf("\x1b[2J");
	printf("SnakeNX\nMade By Abdelali221\nGithub : https://github.com/abdelali221/\n");
	printf("\nPress A to start...");

	while(!Resume) {
		padUpdate(&pad);
		u64 kDown = padGetButtonsDown(&pad);
		if ( kDown & HidNpadButton_A ) {
			Resume = true;
		}
		consoleUpdate(NULL);
	}

	DifficultySelect();
	RenderBorders(true, true);

	while (1) {
		RunGame();
	}
}