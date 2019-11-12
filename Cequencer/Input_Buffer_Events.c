//#include "Header.h"
#include <stdio.h>
#include <windows.h>



HANDLE hStdin;
DWORD fdwSaveOldMode;

VOID ErrorExit(LPSTR);
VOID KeyEventProc(KEY_EVENT_RECORD);
int MouseEventProc(MOUSE_EVENT_RECORD);
VOID ResizeEventProc(WINDOW_BUFFER_SIZE_RECORD);
_Bool ispressed = 0;
void SetPosition(int X, int Y);
int input_Buffer_Events_main();

extern int posX;
extern int posY;
extern int myMouseB;
posX = 0;
posY = 0;
myMouseB = 0;
int input_Buffer_Events_main()
{
	////TEXTCOLOR
	//HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
	//CONSOLE_SCREEN_BUFFER_INFO consoleInfo;
	//WORD saved_attributes;

	///* Save current attributes */
	//GetConsoleScreenBufferInfo(hConsole, &consoleInfo);
	//saved_attributes = consoleInfo.wAttributes;

	//SetConsoleTextAttribute(hConsole, FOREGROUND_RED);

	////***************



	_Bool leftispressed = 0;
	//printf("[]");
	DWORD cNumRead, fdwMode, i;
	INPUT_RECORD irInBuf[128];
	int counter = 0;

	// Get the standard input handle. 

	hStdin = GetStdHandle(STD_INPUT_HANDLE);
	if (hStdin == INVALID_HANDLE_VALUE)
		ErrorExit("GetStdHandle");

	// Save the current input mode, to be restored on exit. 

	if (!GetConsoleMode(hStdin, &fdwSaveOldMode))
		ErrorExit("GetConsoleMode");


	/*
		 Step-1:
		 Disable 'Quick Edit Mode' option programmatically
	  */
	fdwMode = ENABLE_EXTENDED_FLAGS;
	if (!SetConsoleMode(hStdin, fdwMode))
		ErrorExit("SetConsoleMode");
	/*
	   Step-2:
	   Enable the window and mouse input events,
	   after you have already applied that 'ENABLE_EXTENDED_FLAGS'
	   to disable 'Quick Edit Mode'
	*/


	// Enable the window and mouse input events. 

	fdwMode = ENABLE_WINDOW_INPUT | ENABLE_MOUSE_INPUT;
	//SetConsoleMode(hStdin, fdwMode);
	if (!SetConsoleMode(hStdin, fdwMode))
		ErrorExit("SetConsoleMode");

	// Loop to read and handle the next 100 input events. 

	//while (1)
	//{
		// Wait for the events. 

		if (!ReadConsoleInput(
			hStdin,      // input buffer handle 
			irInBuf,     // buffer to read into 
			128,         // size of read buffer 
			&cNumRead)) // number of records read 
			ErrorExit("ReadConsoleInput");

		// Dispatch the events to the appropriate handler. 

		for (i = 0; i < cNumRead; i++)

		{
			switch (irInBuf[i].EventType)
			{
			case KEY_EVENT: // keyboard input 
				KeyEventProc(irInBuf[i].Event.KeyEvent);
				break;

			case MOUSE_EVENT: // mouse input 
				return (MouseEventProc(irInBuf[i].Event.MouseEvent));
				
				break;

			case WINDOW_BUFFER_SIZE_EVENT: // scrn buf. resizing 
				ResizeEventProc(irInBuf[i].Event.WindowBufferSizeEvent);
				break;

			case FOCUS_EVENT:  // disregard focus events 

			case MENU_EVENT:   // disregard menu events 
				break;

			default:
				ErrorExit("Unknown event type");
				break;
			}
		}
	//}

	// Restore input mode on exit.

	SetConsoleMode(hStdin, fdwSaveOldMode);

	//RESTORE TEXTCOLOR
	  /* Restore original attributes */
	//SetConsoleTextAttribute(hConsole, saved_attributes);
	//printf("Back to normal");




	return 0;
}

VOID ErrorExit(LPSTR lpszMessage)
{
	fprintf(stderr, "%s\n", lpszMessage);

	// Restore input mode on exit.

	SetConsoleMode(hStdin, fdwSaveOldMode);

	ExitProcess(0);
}

VOID KeyEventProc(KEY_EVENT_RECORD ker)
{
	//printf("Key event: ");
	static count = 0;
	if (ker.bKeyDown) {
		//printf("key pressed  %c \n", ker.uChar.AsciiChar);
		if (ker.uChar.AsciiChar == 'a')
			ispressed = ++count % 2;
		else if (ker.uChar.AsciiChar == 'c')
			system("cls");
		DWORD fdwMode = ENABLE_WINDOW_INPUT | ENABLE_MOUSE_INPUT;
		//SetConsoleMode(hStdin, fdwMode);
		if (!SetConsoleMode(hStdin, fdwMode))
			ErrorExit("SetConsoleMode");
	}

	else {
		//printf("key released %c\n", ker.uChar.AsciiChar);

	}


	//if (ker.dwControlKeyState == CAPSLOCK_ON)
	//{
	//	//printf("key released %c\n", ker.uChar.AsciiChar);
	//	ispressed = 1;
	//}
	//else {
	//	ispressed = 0;
	//}


}

int MouseEventProc(MOUSE_EVENT_RECORD mer)
{
#ifndef MOUSE_HWHEELED
#define MOUSE_HWHEELED 0x0008
#endif
	//printf("Mouse event: ");

	switch (mer.dwEventFlags)

	{
	case 0:
		//printf("BUTTON");
		if (mer.dwButtonState == FROM_LEFT_1ST_BUTTON_PRESSED)
		{

			SetPosition(mer.dwMousePosition.X, mer.dwMousePosition.Y);
			
			//setColor(10);
			//ispressed = 1;
			posX = mer.dwMousePosition.X;
			posY = mer.dwMousePosition.Y;
			myMouseB = 1;
			//print_colors();
			

			//printf("*");
			return 1;
			//printf("left button press \n");
			
			
			//printf("left clicked at x=%d, y=%d\n", mer.dwMousePosition.X, mer.dwMousePosition.Y);


			/*int myint = 0;
			puts("inputnumber");
			scanf_s("%d", &myint);*/
		}
		else if (mer.dwButtonState == RIGHTMOST_BUTTON_PRESSED)
		{
			myMouseB = 2;
			posX = mer.dwMousePosition.X;
			posY = mer.dwMousePosition.Y;
			return 1;
			//printf("right button press \n");
		}
		else
		{
			myMouseB = 0;
			ispressed = 0;
			//printf("button press\n");
		}
		break;
	case -1: printf("right button released \n"); break;
	case DOUBLE_CLICK:
		//printf("double click\n");
		break;
	case MOUSE_HWHEELED:
		//printf("horizontal mouse wheel\n");
		break;
	case MOUSE_MOVED:
		//printf("mouse moved\n");
		if (ispressed) {
			SetPosition(mer.dwMousePosition.X, mer.dwMousePosition.Y);
			//
			printf("*");
		}

		break;
	case MOUSE_WHEELED:
		//printf("vertical mouse wheel\n");
		break;
	default:
		//printf("unknown\n");
		break;
	}
	return 0;
}

VOID ResizeEventProc(WINDOW_BUFFER_SIZE_RECORD wbsr)
{
//	printf("Resize event\n");
//	printf("Console screen buffer is %d columns by %d rows.\n", wbsr.dwSize.X, wbsr.dwSize.Y);
}



//*****************************

//*** Prototypes ***



//*****
void SetPosition(int X, int Y)
{
	HANDLE Screen;
	Screen = GetStdHandle(STD_OUTPUT_HANDLE);
	COORD Position = { X, Y };

	SetConsoleCursorPosition(Screen, Position);
}