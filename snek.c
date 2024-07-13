#include <stdio.h> // printf, sprintf
#include <string.h> // memset, strcpy
#include <time.h> // time, timespec, nanosleep
#include <stdlib.h> // srand, rand, malloc, free
#include <signal.h> // signal, SIGINT
#include <sys/time.h> // gettimeofday

/* --- CUSTOMIZABLE --- */

// default 16
#define FPS 30
// default 64
#define WIDTH 16
// default 20
#define HEIGHT 9
// initial number of snake segments
#define SNEK_LEN_INIT 4
// function that the CPU uses to decide the direction of the next move.
#define SNEK_STRATEGY STRAT_PERF

/* --- SCREEN --- */

char screen[HEIGHT+2][WIDTH+2];
// extra rows for stats, extra col for null terminators

void drawScreen(){
	memset(screen[0], '-', WIDTH);
	screen[0][WIDTH] = 0;
	for(int i = 1; i < HEIGHT-1; i++){
		screen[i][0] = '|';
		screen[i][WIDTH-1] = '|';
		screen[i][WIDTH] = 0;
		memset(screen[i]+1, ' ', WIDTH-2);	
	}
	memset(screen[HEIGHT-1], '-', WIDTH);
	screen[HEIGHT-1][WIDTH] = 0;
}

void renderScreen(){
	for(int i = 0; i < HEIGHT+2; i++){
		printf("%s\n", screen[i]);
	}
}

struct Obj {
	int x; // x pos; 0 < x < WIDTH
	int y; // y pos; 0 < x < HEIGHT
};

/* --- SNAKE --- */

struct Segment {
	struct Segment * prev;
	struct Segment * next;	
	struct Obj obj;
};
// doubly linked list node of snake segs
struct Segment * snakeHEAD;
struct Segment * snakeTAIL;
// directions: 'u', 'l', 'd', 'r'
char snakeHEADdir;
char prev_snakeHEADdir;

/* --- SNEK --- */

void addSegmentHead(){
	if(snakeHEAD == NULL){
		snakeHEAD = (struct Segment*)malloc(sizeof(struct Segment));
		snakeHEAD->prev = NULL;
		snakeHEAD->next = NULL;
		snakeTAIL = snakeHEAD;
		
		snakeHEAD->obj.x = WIDTH/2;
		snakeHEAD->obj.y = HEIGHT/2 - 2;
		return;
	}
	struct Segment *newSeg = (struct Segment*)malloc(sizeof(struct Segment));
	newSeg->next = snakeHEAD;
	snakeHEAD->prev = newSeg;
	snakeHEAD = newSeg;

	snakeHEAD->obj.x = snakeHEAD->next->obj.x;
	snakeHEAD->obj.y = snakeHEAD->next->obj.y;
	switch(snakeHEADdir){ // cancel backtracking
		case 'u':
			if(prev_snakeHEADdir == 'd'){
				snakeHEADdir = 'd';
				snakeHEAD->obj.y++;
			} else {
				snakeHEAD->obj.y--;
			}
			break;
		case 'd':
			if(prev_snakeHEADdir == 'u'){
				snakeHEADdir = 'u';
				snakeHEAD->obj.y--;
			} else {
				snakeHEAD->obj.y++;
			}
			break;
		case 'l':
			if(prev_snakeHEADdir == 'r'){
				snakeHEADdir = 'r';
				snakeHEAD->obj.x++;
			} else {
				snakeHEAD->obj.x--;
			}
			break;
		case 'r':
			if(prev_snakeHEADdir == 'l'){
				snakeHEADdir = 'l';
				snakeHEAD->obj.x--;
			} else {
				snakeHEAD->obj.x++;
			}
			break;
	}
}

void remSegmentTail(){
	if(snakeHEAD == NULL) return;
	if(snakeHEAD == snakeTAIL){
		free(snakeTAIL);
		snakeHEAD = NULL;
		snakeTAIL = NULL;
		return;
	}
	struct Segment * oldSeg = snakeTAIL->prev;
	oldSeg->next = NULL;
	free(snakeTAIL);
	snakeTAIL = oldSeg;
}

void initSnake(){
	snakeHEAD = NULL;
	snakeTAIL = NULL;
	snakeHEADdir = 'd';
	prev_snakeHEADdir = '\0';
	for(int i = 0; i < SNEK_LEN_INIT; i++){
		addSegmentHead();
	}
}

void moveSnake(){
	// move head
	addSegmentHead();
	// move tail
	remSegmentTail();
}

// map snake to screen matrix
// also check for collisions
int drawSnake(){
	struct Segment * currSeg = snakeHEAD;
	if(snakeHEAD == NULL) return -2;
	if(screen[currSeg->obj.y][currSeg->obj.x] != ' '){
		return -1;
	}
	switch(snakeHEADdir){
		case 'u':
			screen[currSeg->obj.y][currSeg->obj.x] = '^';
			break;
		case 'd':
			screen[currSeg->obj.y][currSeg->obj.x] = 'v';
			break;
		case 'l':
			screen[currSeg->obj.y][currSeg->obj.x] = '<';
			break;
		case 'r':
			screen[currSeg->obj.y][currSeg->obj.x] = '>';
			break;
	}
	currSeg = currSeg->next;	
	while(currSeg != NULL){
		if(screen[currSeg->obj.y][currSeg->obj.x] != ' ') return -1;
		screen[currSeg->obj.y][currSeg->obj.x] = 's';
		currSeg = currSeg->next;
	}
	return 0;
}

/* --- APPLE --- */

struct Apple {
	struct Obj obj;
} apple;

int appleEaten = 0;

void initApple(){
	appleEaten = 0;
	struct Obj tempObj;
	struct Segment * currSeg;
	int valid; // if apple collides with snake
	do {
		valid = 1;
		tempObj.x = rand() % (WIDTH - 2) + 1;
		tempObj.y = rand() % (HEIGHT - 2) + 1;
		currSeg = snakeHEAD;
		while(currSeg != NULL){
			if(tempObj.x == currSeg->obj.x
			&& tempObj.y == currSeg->obj.y){
				valid = 0;
				break;
			}
			currSeg = currSeg->next;
		}
	} while(!valid);
	apple.obj = tempObj;
}

void drawApple(){
	screen[apple.obj.y][apple.obj.x] = 'a';
}

// called after new head, before cutting tail
void snakeEatsApple(){	
	if(snakeHEAD->obj.x == apple.obj.x
	&& snakeHEAD->obj.y == apple.obj.y){
		appleEaten = 1;
	}
}

/* --- SCORE + TIMER --- */

// maximum score; result from previous macros
#define SNEK_SCORE_MAX ((((WIDTH)-2) * ((HEIGHT)-2)) - (SNEK_LEN_INIT))
// compute elapsed time between 2 timevals
#define TIME(t1, t2) ((t2).tv_sec-(t1).tv_sec)+(((t2).tv_usec-(t1).tv_usec)/1e6)

struct timespec ts; // arg for nanosleep

void initTimespec(){
	if(FPS > 1){
		ts.tv_sec = 0;
		ts.tv_nsec = (1000000000/FPS);
	} else {
		// just set effective FPS to 1
		ts.tv_sec = 1;
		ts.tv_nsec = 0;
	}
}

int score;
char scoreBuf[16];

void drawScore(){
	sprintf(scoreBuf, "%d", score);
	strcpy(&screen[HEIGHT][0], "SCORE: ");
	strcpy(&screen[HEIGHT][7], scoreBuf); 
}

struct timeval tStart;
struct timeval tCurr;
char tBuf[16]; // seconds

// requires tCurr to be accurate
void drawTime(){
	sprintf(tBuf, "%0.2f", TIME(tStart, tCurr));
	strcpy(&screen[HEIGHT+1][0], "TIME: ");
	strcpy(&screen[HEIGHT+1][6], tBuf);
}

// draws score and time
void drawStats(){
	drawScore();
	gettimeofday(&tCurr, NULL);
	drawTime();
}

enum ExitCode { SNEK_ERR = 999, SNEK_LOSE, SNEK_WIN };

// upon receiving SIGINT (Ctrl+C)
// free all malloc'd memory and exit
void endGame(int sig){
	if(sig == SIGINT) printf("\n"); // because of ^C printout
	switch(sig){
		case SNEK_ERR:
			printf("ERROR: FPS value is not a positive integer!\n");
			break;
		case SNEK_LOSE:
			printf("Crashed going %c!\n", snakeHEADdir);
			break;
		case SNEK_WIN:
			// draw and render rest of frame
			drawSnake(); // !collision unchecked!
			drawStats();
			renderScreen();
			printf("PERFECT!!! You Win!\n");
			break;
	}
	// delete snake
	struct Segment * currSeg = snakeHEAD;
	while(currSeg->next != NULL){
		currSeg = currSeg->next;
		free(currSeg->prev);
	}
	if(currSeg != NULL) free(currSeg);
	// exit and pass signal along
	exit(sig);
}

/* --- STRATEGY --- */

char possibleDirs[4][3] = {{'u', 'd', 'l'},
		   {'u', 'd', 'r'},
		   {'u', 'l', 'r'},
		   {'d', 'l', 'r'}};

int backtracks(){
	switch(snakeHEADdir){
		case 'u':
			return prev_snakeHEADdir == 'd';
		case 'd':
			return prev_snakeHEADdir == 'u';
		case 'l':
			return prev_snakeHEADdir == 'r';
		case 'r':
			return prev_snakeHEADdir == 'l';
	}
	return -1;
}

// picks a random direction for the snake to go
void decideMove_random(){
	switch(snakeHEADdir){
		case 'l':
			snakeHEADdir = possibleDirs[0][rand() % 3];
			break;
		case 'r':
			snakeHEADdir = possibleDirs[1][rand() % 3];
			break;
		case 'u':
			snakeHEADdir = possibleDirs[2][rand() % 3];
			break;
		case 'd':
			snakeHEADdir = possibleDirs[3][rand() % 3];
			break;
			
	}
}

/* 	Tries to reduce the distance from the apple to zero.
	 	Preference for going U/D first, then L/R.
 		If apple is behind snake, will pick a random move.
*/
void decideMove_greedy(){
	int x_diff = snakeHEAD->obj.x - apple.obj.x; // + = left
	int y_diff = snakeHEAD->obj.y - apple.obj.y; // + = up
	struct Segment * currSeg = snakeHEAD;	
	
	if(y_diff < 0){
		snakeHEADdir = 'd';
	} else if(y_diff > 0){
		snakeHEADdir = 'u';
	} else { // matched y level
		if(x_diff < 0){
			snakeHEADdir = 'r';
		} else if(x_diff > 0){
			snakeHEADdir = 'l';
		}
	}
	// prevent backtracking
	if(backtracks()){
		decideMove_random();
	}
}

// ONLY WORKS with EVEN HEIGHT
int perfect_phase = 0;
void decideMove_perfectEasy(){
	switch(perfect_phase){
		case 0: // move down (initially)
			if(snakeHEAD->obj.y == HEIGHT-2){ // transition
				perfect_phase = 1;
				snakeHEADdir = 'r';
				return;
			} else {
				snakeHEADdir = 'd';
			}
			break;
		case 1: // snaking right
			if(snakeHEAD->obj.x == WIDTH-2){ // switch to snaking left
				perfect_phase = 2;
				snakeHEADdir = 'u';
				return;
			} else { // keep snaking right
				snakeHEADdir = 'r';
			}
			break;
		case 2: // snaking left
			if(snakeHEAD->obj.x == 2){
				if(snakeHEAD->obj.y == 1){ // move downwards
					perfect_phase = 0;
					snakeHEADdir = 'l';
				} else { // switch to snaking right
					perfect_phase = 1;
					snakeHEADdir = 'u';
				}
			} else { // keep snaking left
				snakeHEADdir = 'l';
			}
			break;	
		default:
			decideMove_random(); // shouldn't be here
	}
}

// ONLY WORKS with EVEN WIDTH
void decideMove_perfectMed(){
	switch(perfect_phase){
		case 0: // move right (initially)
			if(snakeHEAD->obj.x == WIDTH-2){ // transition
				perfect_phase = 1;
				snakeHEADdir = 'u';
				return;
			} else {
				snakeHEADdir = 'r';
			}
			break;
		case 1: // snaking up
			if(snakeHEAD->obj.y == 1){ // switch to snaking down
				perfect_phase = 2;
				snakeHEADdir = 'l';
				return;
			} else { // keep snaking up
				snakeHEADdir = 'u';
			}
			break;
		case 2: // snaking down
			if(snakeHEAD->obj.y == HEIGHT-3){
				if(snakeHEAD->obj.x == 1){ // move leftward
					perfect_phase = 0;
					snakeHEADdir = 'd';
				} else { // switch to snaking up
					perfect_phase = 1;
					snakeHEADdir = 'l';
				}
			} else { // keep snaking down
				snakeHEADdir = 'd';
			}
			break;	
		default:
			decideMove_random(); // shouldn't be here
	}
}

// TODO: implement hard (more phases)
void decideMove_perfectHard(){
	
}

void (*activeStrategy)(); // set by initStrategy
enum Strategy {STRAT_RAND, STRAT_GREED, STRAT_PERF};

// determines which decideMove_... to use
void initStrategy(){
	switch(SNEK_STRATEGY){
		case STRAT_RAND:
			activeStrategy = decideMove_random;
			break;
		case STRAT_GREED:
			activeStrategy = decideMove_greedy;
			break;
		case STRAT_PERF:
			// examine board dimensions
			if(HEIGHT % 2 == 0){ // easy strat with even HEIGHT
				activeStrategy = decideMove_perfectEasy;
			} else {
				if(WIDTH % 2 == 0){ // easy strat rotated
					activeStrategy = decideMove_perfectMed;
				} else { // more complex strat if both dims odd
					activeStrategy = decideMove_perfectHard;
				}
			}
			break;
	}
}

/* --- DRAW LOOP (MAIN) --- */

int main(){
	int ret;
	initTimespec();
	srand(time(NULL));
	signal(SIGINT, endGame);
	drawScreen();
	initSnake();
	drawSnake();
	initApple(); // after drawing snake
	drawApple();
	initStrategy(); // initialize activeStrategy
	 // score
	score = 0;
	drawScore();
	// set starting time;
	gettimeofday(&tStart, NULL);
	tCurr = tStart;
	drawTime();
	renderScreen();
	nanosleep(&ts, &ts);
	while(1){
		drawScreen();
		prev_snakeHEADdir = snakeHEADdir;
		activeStrategy();
		//move snake head
		addSegmentHead();
		snakeEatsApple(); // sets appleEaten
		if(appleEaten){
			score++;
			if(score == SNEK_SCORE_MAX){ // perfect game!!!
				endGame(SNEK_WIN);
			}
			initApple(); // unsets appleEaten
		} else {
			remSegmentTail();
		}
		drawApple();
		ret = drawSnake();
		if(ret < 0){ // if snake collided with wall or itself
			endGame(SNEK_LOSE);
		}
		drawStats();
		renderScreen();
		nanosleep(&ts, &ts);
	}
	return 0;
}
