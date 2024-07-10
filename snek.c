#include <stdio.h> // printf, sprintf
#include <string.h> // memset, strcpy
#include <time.h> // time, timespec, nanosleep, clock
#include <stdlib.h> // srand, rand, malloc, free
#include <signal.h> // signal, SIGINT
#include <sys/time.h> // gettimeofday

// default 15 autoplay
#define FPS 15

// compute elapsed time between 2 timevals
#define TIME(t1, t2) ((t2).tv_sec-(t1).tv_sec)+(((t2).tv_usec-(t1).tv_usec)/1e6)
// TIME_DIV may need adjusting across systems
//#define TIME_DIV CLOCKS_PER_SEC
#define TIME_DIV 1000

// default 65
#define LENGTH 65
// default 19
#define WIDTH 19

struct timespec ts; // arg for nanosleep
struct timeval tv_0, tv_f; // for gettimeofday

char screen[WIDTH+1][LENGTH+2];

void drawScreen(){
	memset(screen[0], '-', LENGTH);
	screen[0][LENGTH] = 0;
	for(int i = 1; i < WIDTH-1; i++){
		screen[i][0] = '|';
		screen[i][LENGTH-1] = '|';
		screen[i][LENGTH] = 0;
		memset(screen[i]+1, ' ', LENGTH-2);	
	}
	memset(screen[WIDTH-1], '-', LENGTH);
	screen[WIDTH-1][LENGTH] = 0;
}

void renderScreen(){
	for(int i = 0; i < WIDTH; i++){
		printf("%s\n", screen[i]);
	}
}

struct Obj {
	int x; // x pos; 0 < x < LENGTH
	int y; // y pos; 0 < x < WIDTH
};


// doubly linked list node of snake segs
struct Segment {
	struct Segment * prev;
	struct Segment * next;	
	struct Obj obj;
};

struct Segment * snakeHEAD;
struct Segment * snakeTAIL;
char snakeHEADdir;
char prev_snakeHEADdir;

struct Apple {
	struct Obj obj;
} apple;

// upon receiving SIGINT (Ctrl+C)
// free all malloc'd memory and exit
void endGame(int sig){
	if(sig == SIGINT) printf("\n"); // because of ^C printout
	struct Segment * currSeg = snakeHEAD;
	while(currSeg->next != NULL){
		currSeg = currSeg->next;
		free(currSeg->prev);
	}
	if(currSeg != NULL) free(currSeg);
	exit(sig);
}

void initTimespec(){
	if(FPS > 1){
		ts.tv_sec = 0;
		ts.tv_nsec = (1000000000/FPS);
	} else if(FPS == 1){
		ts.tv_sec = 1;
		ts.tv_nsec = 0;
	} else {
		endGame(1001);
	}
}

/* --- SNEK --- */

int appleEaten;

void addSegmentHead(){
	if(snakeHEAD == NULL){
		snakeHEAD = (struct Segment*)malloc(sizeof(struct Segment));
		snakeHEAD->prev = NULL;
		snakeHEAD->next = NULL;
		snakeTAIL = snakeHEAD;
		
		snakeHEAD->obj.x = LENGTH/2;
		snakeHEAD->obj.y = WIDTH/2 - 2;
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
	addSegmentHead();
	addSegmentHead();
	addSegmentHead();
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

void initApple(){
	appleEaten = 0;
	struct Obj tempObj;
	struct Segment * currSeg;
	int valid; // if apple collides with snake
	do {
		valid = 1;
		tempObj.x = rand() % (LENGTH - 2) + 1;
		tempObj.y = rand() % (WIDTH - 2) + 1;
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

/* --- CALCULATE SNEK'S NEXT MOVE --- */

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

// tries to reduce the distance from the apple to zero.
// preference for going U/D first, then L/R.
// if apple is behind snake, will pick a random move.
void decideMove_simple(){
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

int phase = 0;

void decideMove_perfect(){
	if(phase){
		
	}
}


/* --- SCORE + TIMER --- */

int score;
char scoreBuf[4];

void drawScore(){
	strcpy(&screen[WIDTH-1][5], "SCORE:");
	screen[WIDTH-1][10] = ' '; // cut '\0'
	strcpy(&screen[WIDTH-1][11], scoreBuf); 
	screen[WIDTH-1][11+strlen(scoreBuf)] = '-'; // cut '\0'
}

struct timeval tStart;
struct timeval tCurr;
char tBuf[5]; // seconds

// requires tCurr to be accurate
void drawTime(){
	sprintf(tBuf, "%0.2f", TIME(tStart, tCurr));
	strcpy(&screen[WIDTH-1][LENGTH-16], "TIME:");
	screen[WIDTH-1][LENGTH-11] = ' '; // cut '\0'
	strcpy(&screen[WIDTH-1][LENGTH-10], tBuf);
	screen[WIDTH-1][LENGTH-10 + strlen(tBuf)] = '-'; // cut '\0'	
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
	 // score
	score = 0;
	sprintf(scoreBuf, "%d", score);
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
		decideMove_simple();
		//move snake head
		addSegmentHead();
		snakeEatsApple(); // sets appleEaten
		if(appleEaten){
			initApple(); // unsets appleEaten
			score++;
			sprintf(scoreBuf, "%d", score);
		} else {
			remSegmentTail();
		}
		drawApple();
		ret = drawSnake();
		if(ret < 0){ // if snake collided with wall or itself
			printf("Crashed going %c!\n", snakeHEADdir);
			endGame(1000);
		}
		drawScore();
		// time calculation
		gettimeofday(&tCurr, NULL);
		drawTime();
		renderScreen();
		nanosleep(&ts, &ts);
	}
	return 0;
}
