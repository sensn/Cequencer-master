//#include "Header.h"
//#include <cstddef>
#include <stdlib.h>
#include "rlutil.h"
#include <time.h>
#include <windows.h>
#include <mmsystem.h>
#include <dsound.h>



#define MAX 35
#define X_COUNT 8
#define Y_COUNT 16
struct dat
{
	int posX;
	int posY;
	int triggerX;
	char playerChr;

	char name[MAX];

	struct date {
		int day;
		int month;
		int year;
	}date;
};

typedef struct {
	int trigger[16];
	int notenumber;
	int vel;
	int channel;
}Room;


typedef struct {
	int screenX;
	int screenY;
	Room room[3];

}House;

//***
typedef struct IDirectSound* LPDIRECTSOUND;

//***


int playSequence(struct dat lib[9][9]);
//int input_Buffer_Events_main();
int call_raster_main();
extern int posX;
extern int posY;
extern int myMouseB;
//asd
int xs = X_COUNT + 1;
int ys = Y_COUNT + 1;

int yoffset = 1;
char** fpath = { "C:\\Users\\ATN_70\\Desktop\\C2_Ausbildung - master\\C2_Ausbildung\\snare.wav", "C:\\Users\\ATN_70\\Desktop\\C2_Ausbildung-master\\C2_Ausbildung\\kick.wav" };

struct dat lib[X_COUNT + 1][Y_COUNT + 1] = { '\0' };

DWORD WINAPI ThreadFunc(void* data) {
	playSequence(lib);
	// Do stuff.  This will be the first function called on the new thread.
	// When this function returns, the thread goes away.  See MSDN for more details.
	return 0;
}
int s_main(char** fpath);

int main(int argc, char argv[]) {
	
	//HRESULT WINAPI DirectSoundCreate(LPGUID lpGuid, LPDIRECTSOUND * ppDS, LPUNKNOWN  pUnkOuter);

	//HANDLE thread = CreateThread(NULL, 0, ThreadFunc, NULL, 0, NULL);
	//call_raster_main();
	s_main(fpath);
	//input_Buffer_Events_main();
//	setColor(10);  //				reset to white;

}

int call_raster_main() {
	//CALL ME123456456yxcv
	int tx = 0;  //tmp for trigger print

	char player[2] = { 'X','O' };
	int p = 2;
	int eingabe = 1;
	int yoffset = 1;
	//lib[1][1].posY = 1;
	//lib[1][1].posX =1;
	//lib[1][1].playerChr ='x';
	//playSequence(lib);
	HANDLE    hIOMutex = CreateMutex(NULL, TRUE, NULL);

	
	do
	{
		WaitForSingleObject(hIOMutex, INFINITE);
		system("cls");  // Clean the screen!
						//Print the Rastaman!
		setColor(7);
		for (int i = 0; i < yoffset; i++) {
			puts("");

		}
		for (size_t x = 0; x < ys*2-2; x +=2) {
			SetPosition(x, yoffset);
			printf(" %c", '_');
			//printf(" ");
			/*SetPosition(x+1, yoffset);
			printf("%c ",'_');*/
		}
		puts("");
		for (size_t x = 1; x < xs; x++) {
			for (size_t y = 1; y < ys; y++) {
				printf("|");

				//printf("%c", ((lib[x][y].posX > 0 && lib[x][y].posY > 0) && (x == lib[x][y].posX && y == lib[x][y].posY)) ? lib[x][y].playerChr : '_');
				if ((lib[x][y].posX > 0 && lib[x][y].posY > 0) && (x == lib[x][y].posX && y == lib[x][y].posY)) {
					setColor(10);
					printf("%c", lib[x][y].playerChr);
					setColor(7);
				}
				else {
					printf("%c", '_');
				}

				//printf("%s", ((lib[x][y].posX >0 && lib[x][y].posY >0) && (x == lib[x][y].posX && y == lib[x][y].posY)) ? "T" :"_" );
				if (y == ys - 1) {
					printf("|");
				}
			}
		puts("");
		
		}
		/*printf("MOUSElib X: %d Y:%d\n", lib[posX][posY].posX, lib[posX][posY].posY);
		printf("MOUSEPOS X: %d Y:%d", posX, posY);*/
		do
		{
		} while (!input_Buffer_Events_main());
		//	setColor(10);
			//printf("MOUSE X: %d Y:%d",posX,posY);
		//if (input_Buffer_Events_main()) {
		//	if (posX < xs && posY < ys) {
		if (myMouseB != 0) {
			tx = posX;
			if (posX == 1) {

			}
			else if (posX > 1) {
				posX = posX - (posX / 2);
			}
			posY = posY - yoffset;
			lib[posY][posX].posY = posX;
			lib[posY][posX].posX = posY;
			if (myMouseB == 1) {
				lib[posY][posX].playerChr = '*';
			}
			else if (myMouseB == 2) {
				lib[posY][posX].playerChr = ' ';
			}

			lib[posY][posX].triggerX = tx;
		}
				//}
				ReleaseMutex(hIOMutex);

	//	}
		//printf("MOUSE");
	} while (eingabe != 0);
	return 1;
}



int playSequence(struct dat lib[X_COUNT + 1][Y_COUNT + 1]) {
	HANDLE    hIOMutex = CreateMutex(NULL, TRUE, NULL);

	WaitForSingleObject(hIOMutex, INFINITE);
	

	
	
	
	_Bool isplaying = 1;

	double time_spent = 0.0;
	
	long begin = clock();

	// do some stuff here
	//Sleep(3);

	long end = clock();
	double theend = clock();

	// calculate elapsed time by finding difference (end - begin) and
	// divide by CLOCKS_PER_SEC to convert to seconds
	time_spent += (double)(end - begin) / CLOCKS_PER_SEC;

	//	printf("Time elpased is %f seconds", time_spent);

		//
	int index = 1;

	double ms;
	double dur;
	double swing = 0;
	double theswing = 0.0;
	int thebpm = 120;

	ms = ((60000.0 / (double)thebpm) / (double)4);  //Millisecond per quarternote
	//ms = 125;  //Millisecond per quarternote
	dur = (ms / 1000) * CLOCKS_PER_SEC;
	printf("MILLIS PER QUATER:%f\n\n", ms);
	printf("ms/Clocks :%f\n\n", dur);

	//

	House myHouse = { 640,400,.room[0] = {{1,0,0,0,1,0,0,0,1,0,0,0,1,0,0,0},64,127,1},
							   .room[1] = {{1,0,1,0,1,0,1,0,1,0,1,0,1,0,1,0},64,127,1},
							   .room[2] = {{1,1,0,1,1,1,0,1,1,1,0,1,1,1,0,1},64,127,1} };
	//printf("THE ROOM: %d\n", myHouse.room[1].notenumber);

	while (isplaying) {
		begin = clock();
		if ((double)begin > theend) {

			theend = (double)clock() + dur + (dur * (swing));
			swing = (index % 2) ? 0 : theswing;
			//printf("%f", swing);
			//printf("*");    //Do things...

			for (int i = 1; i < 9; i++) {
				//if (myHouse.room[i].trigger[index]) {
				if (lib[i][index].playerChr == '*') {
					
				 SetPosition(lib[i][index].triggerX, X_COUNT+1+ yoffset + i);
				 printf("T");
				// printf("X: %d Y: %d",i, index);
					
					PlaySound("C:\\Users\\ATN_70\\Desktop\\C2_Ausbildung-master\\C2_Ausbildung\\snare.wav", NULL, SND_ASYNC);  //   PLAY THE SOUND using win api
					PlaySound("C:\\Users\\ATN_70\\Desktop\\C2_Ausbildung-master\\C2_Ausbildung\\kick.wav", NULL, SND_ASYNC);
				
				}
				else {
					//	printf("-");
				}
			}//for



			index++;
		} //if beginn > end


		if (index == 17) { //reset things
			//tickindex = 0;
			index = 1;
			//system("cls");
			//puts("");
			ReleaseMutex(hIOMutex);
		}

	}

}