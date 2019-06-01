// BitBlt color aimbot
// for games with hp bars, outlines or colored bright enemy models
// color aimbots require 120 fps or more to work properly (turn off vsync)
// will not work in fullscreen mode
//
// 1. find the right colors, take screenshots of colored models or hp bars at multiple distances and use a color picker to get the right colors you want to aim at
// 2. convert RGB to hex (1100FF = FF0011 because we use BGR)
// 3. use the right findwindow for your game, see main()
// 4. rename compiled exe

#include <Windows.h>
#include <stdio.h>
#pragma comment(lib, "winmm.lib") //timeGetTime

/*
green color		RGB			BGR
58,255,58		3AFF3A		3AFF3A
46,222,46		2EDE2E		2EDE2E
50,238,50		32EE32		32EE32

orange hp bar	RGB			BGR
235,123,56		EB7B38		387BEB

red hp bar		RGB			BGR
241,72,71		F14847		4748F1
*/

//BGR
COLORREF targetColors[3] = { 0x3AFF3A, 0x2EDE2E, 0x32EE32 }; //aim at green enemy colors in quake live (does not aim at enemies with blue quad aura)
//COLORREF targetColors[7] = { 0x00FF00, 0x34FF34, 0x4EFF4E, 0x66FF66, 0x6AFF6A, 0x49FF49, 0x4AFF4A }; //aim at green color example
//COLORREF targetColors[7] = { 0xFF0000, 0xFF3434, 0xFF4E4E, 0xFF6666, 0xFF6A6A, 0xFF4949, 0xFF4A4A }; //aim at blue color example
//COLORREF targetColors[7] = { 0x0000FF, 0x3434FF, 0x4E4EFF, 0x6666FF, 0x6A6AFF, 0x4949FF, 0x4A4AFF }; //aim at red color example

// settings
DWORD Daimkey = VK_SHIFT;	//aimkey (VK_SHIFT, VK_RBUTTON etc.)
int tolerance = 20;			//0-50 color tolerance
int aimsens = 7;			//aim sensitivity (higher = smoother), setting depends on your mouse sensitivity 
int aimheight = 30;			//+ to aim higher, - to aim lower 
int aimfov = 10;			//low value = high aimfov, higher value = lower aimfov

DWORD astime = timeGetTime();	//autoshoot timer
unsigned int asdelay = 50;		//wait 50ms
bool IsPressed = false;

BYTE colorDeviation = 0x10;
BYTE* bitData = NULL;


BOOL ScanPixel(HWND hwnd, PLONG pixelX, PLONG pixelY, RECT scanArea, COLORREF* targetColors, BYTE deviation, COLORREF* foundColor)
{
	deviation = tolerance;

	HDC hdc = GetWindowDC(hwnd);
	HDC _hdcMem = CreateCompatibleDC(hdc);

	LONG scanWidth = scanArea.right - scanArea.left;
	LONG scanHeight = scanArea.bottom - scanArea.top;

	HBITMAP bmp = CreateCompatibleBitmap(hdc, scanWidth, scanHeight);
	BITMAPINFOHEADER bmi = { 0 };
	bmi.biSize = sizeof(BITMAPINFOHEADER);
	bmi.biPlanes = 1;
	bmi.biBitCount = 24;
	bmi.biWidth = scanWidth;
	bmi.biHeight = -scanHeight;
	bmi.biCompression = BI_RGB;

	SelectObject(_hdcMem, bmp);

	BitBlt(_hdcMem, 0, 0, scanWidth, scanHeight, hdc, scanArea.left, scanArea.top, SRCCOPY);
	
	GetDIBits(hdc, bmp, 0, scanHeight, bitData, (BITMAPINFO*)&bmi, DIB_RGB_COLORS);

	// avoid memory leak (todo: verify if all this is correct)
	DeleteObject(bmp);
	DeleteDC(_hdcMem);
	ReleaseDC(NULL, hdc);
	//ReleaseDC(hwnd, hdc);

	// Scan from bottom to top
	for (int y = scanHeight; y >= 0; y--) 
	{
		//for (int x = 0; x < scanWidth; x+= 4) //step 4
		for (int x = 0; x < scanWidth; x++) 
		{
			BYTE r = bitData[3 * ((y * scanWidth) + x) + 2];
			BYTE g = bitData[3 * ((y * scanWidth) + x) + 1];
			BYTE b = bitData[3 * ((y * scanWidth) + x) + 0];

			//for (int i = 0; i < targetColors[i]; i++)
			for (int i = 0; i < 3; i++) //3 if targetColors[3]
			{ 
				UINT targetR = GetRValue(targetColors[i]);
				UINT targetG = GetGValue(targetColors[i]);
				UINT targetB = GetBValue(targetColors[i]);

				if (r >= targetR - deviation && r <= targetR + deviation &&
					g >= targetG - deviation && g <= targetG + deviation &&
					b >= targetB - deviation && b <= targetB + deviation)
				{
					*pixelX = scanArea.left + x;
					*pixelY = scanArea.top + y - aimheight;
					*foundColor = targetColors[i];
					return TRUE;
				}
			}
		}
	}

	return FALSE;
}

/*
//create random window title
#include <iostream>
#include <time.h>

static const char consoleName[] = "1234567890ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz";
int consoleNameLength = sizeof(consoleName) - 1;
char RandomConsoleName()
{
	return consoleName[rand() % consoleNameLength];
}
*/

HWND hwnd = NULL;
int main()
{
	/*
	//set random window title
	srand(time(0));
	std::wstring ConsoleNameStr;
	for (unsigned int i = 0; i < 20; ++i)
	{
		ConsoleNameStr += RandomConsoleName();

	}
	SetConsoleTitle(ConsoleNameStr.c_str());
	*/

	/*
	//or set fake window title
	SetConsoleTitleA("calc.exe");
	*/

	SetPriorityClass(GetCurrentProcess(), HIGH_PRIORITY_CLASS);
	printf("Waiting for game window"); //if nothing happens, click on game window. If still nothing, your findwindow is wrong

	while (!hwnd)
	{
		//use the right findwindow for your game
		hwnd = FindWindowA(0, "Quake Live");
		//hwnd = FindWindowA("LaunchUnrealUWindowsClient", NULL); //ut3
		//hwnd = FindWindowA("UnrealWindow", 0); //ut4
		Sleep(500);
	}

	//try to get screen size of hwnd
	RECT rc{ 0 };
	GetClientRect(hwnd, &rc);
	LONG width = rc.right;
	LONG height = rc.bottom;

	//wait while the game window is not fully initialised
	while (rc.right == 0 || rc.bottom == 0)
	{
		Sleep(1000);
		printf(".");
		GetClientRect(hwnd, &rc);
		if (rc.right != 0 || rc.bottom != 0)
			break;
	}

	//get correct screen size of hwnd
	GetClientRect(hwnd, &rc);
	width = rc.right;
	height = rc.bottom;
	printf("\n");
	printf("Screen size %dx%d\n", width, height);

	//calculate scan area
	LONG centerX = width / 2;
	LONG centerY = height / 2;
	LONG fovX = centerX / aimfov; //width / aimfov;
	LONG fovY = centerY / aimfov; //height / aimfov;
	RECT scanArea = { centerX - fovX, centerY - fovY, centerX + fovX, centerY + fovX };
	printf("Scan area %d,%d,%d,%d\n", scanArea.left, scanArea.top, scanArea.right, scanArea.bottom);

	//calculate scan area size
	LONG scanWidth = scanArea.right - scanArea.left;
	LONG scanHeight = scanArea.bottom - scanArea.top;
	printf("Scan area size %d,%d\n", scanWidth, scanHeight);

	bitData = new BYTE[3 * scanWidth * scanHeight];
	LONG pixelX = 0;
	LONG pixelY = 0;
	COLORREF foundColor = 0;

	while (1)
	{
		if (GetAsyncKeyState(Daimkey) & 0x8000) //only scan anything if aimkey is pressed
		if (GetForegroundWindow() == hwnd && ScanPixel(hwnd, &pixelX, &pixelY, scanArea, targetColors, colorDeviation, &foundColor))
		{
			LONG aimX = pixelX - centerX;
			LONG aimY = pixelY - centerY;

			//aim sensitivity
			aimX /= aimsens;
			aimY /= aimsens;

			//aim
			//aim at the correct targetcolors[i] amount
			if (foundColor == targetColors[0] || foundColor == targetColors[1] || foundColor == targetColors[2])
				mouse_event(MOUSEEVENTF_MOVE, aimX-1, aimY, 0, 0); //you may have to adjust aimpoint to +1 or -1 pixel to left or right, depends on your game and settings

			/*
			//trigger radius (autoshoot will not work good without aimbot)
			int radiusx = 3 * (centerX / 100); //3+ looks more legit
			int radiusy = 3 * (centerY / 100);

			//if not fireing manually (do not interrupt manual fireing)
			if (!GetAsyncKeyState(VK_LBUTTON))
			//if in screenmiddle on target
			if (pixelX >= centerX - radiusx && pixelX <= centerX + radiusx && pixelY >= centerY - radiusy && pixelY <= centerY + radiusy) //sucks
			//if target color found
			if (foundColor == targetColors[0] || foundColor == targetColors[1] || foundColor == targetColors[2])
			{
				//autoshoot on
				if (!IsPressed)
				{
					IsPressed = true;
					mouse_event(MOUSEEVENTF_LEFTDOWN, 0, 0, 0, 0);
				}
			}
			*/
		}

		/*
		//autoshoot off
		if (IsPressed)
		{
			if (timeGetTime() - astime >= asdelay)
			{
				IsPressed = false;
				mouse_event(MOUSEEVENTF_LEFTUP, 0, 0, 0, 0);
				astime = timeGetTime();
			}
		}
		*/
		Sleep(1); //1 best for 120+ fps, 2 ok at 60 fps, 3+ bad
	}

	system("pause");
}