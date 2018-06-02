#define WIN32_LEAN_AND_MEAN		// "odchudza" aplikacj� Windows

/*******************************************************************
*	Rozdzia� 8,  tekstury - przyk�ad trzeci
*	Autor: Kevin Hawkins
*	Pozwala obraca� oraz przybli�a� i oddala� teren.
*	Dane o ukszta�towaniu terenu �adowane s� z pliku BMP.
********************************************************************/

////// Definicje
#define BITMAP_ID 0x4D42		// identyfikator formatu BMP
#define MAP_X	32				// rozmiar mapy wzd�u� osi x
#define MAP_Z	32				// rozmiar mapy wzd�u� osi y
#define MAP_SCALE	20.0f		// skala mapy
#define PI		3.14159

////// Pliki nag��wkowe
#include <windows.h>			// standardowy plik nag��wkowy Windows
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <gl/gl.h>				// standardowy plik nag��wkowy OpenGL
#include <gl/glu.h>				// funkcje pomocnicze OpenGL
#include<freeglut.h>
////// Zmienne globalne
HDC g_HDC;					// globalny kontekst urz�dzenia
bool fullScreen = false;			// true = tryb pe�noekranowy; 
									//false = tryb okienkowy
bool keyPressed[256];			// tablica przyci�ni�� klawiszy

float angle = 0.0f;				// k�t kamery
float radians = 0.0f;			// k�t kamery w radianach
float waterHeight = 154.0f;		// poziom morza
bool waterDir = true;			// animacja falowania wody; 
								// true = poziom w g�r�, false = poziom w d�

								////// Zmienne myszy i kamery
int mouseX, mouseY;						// wsp�rz�dne myszy
float cameraX, cameraY, cameraZ;	// wsp�rz�dne kamery
float lookX, lookY, lookZ;			// punkt wycelowania kamery

									////// Opis tekstury
BITMAPINFOHEADER	bitmapInfoHeader;	// pomocniczy nag��wek obrazu
BITMAPINFOHEADER	landInfo;			// nag��wek obrazu tekstury l�du
BITMAPINFOHEADER	waterInfo;			// nag��wek obrazu tekstury wody

unsigned char		*imageData;		// dane mapy
unsigned char*      landTexture;	// dane tekstury l�du
unsigned char*		waterTexture;	// dane tekstury wody
unsigned int		land;			// obiekt tekstury l�du
unsigned int		water;			// obiekt tekstury wody

									////// Opis terenu
float terrain[MAP_X][MAP_Z][3];		// wysoko�� terenu w punktach siatki (0-255)

									// LoadBitmapFile
									// opis: �aduje map� bitow� z pliku i zwraca jej adres.
									//       Wype�nia struktur� nag��wka.
									//	 Nie obs�uguje map 8-bitowych.
unsigned char *LoadBitmapFile(const char *filename, BITMAPINFOHEADER *bitmapInfoHeader)
{
	FILE *filePtr;							// wska�nik pozycji pliku
	BITMAPFILEHEADER	bitmapFileHeader;		// nag��wek pliku
	unsigned char		*bitmapImage;			// dane obrazu
	int					imageIdx = 0;		// licznik pikseli
	unsigned char		tempRGB;				// zmienna zamiany sk�adowych
	errno_t eFile;

	eFile = fopen_s(&filePtr, filename, "rb");											// otwiera plik w trybie "read binary"

	if (filePtr == NULL)
		return NULL;

	// wczytuje nag��wek pliku
	fread(&bitmapFileHeader, sizeof(BITMAPFILEHEADER), 1, filePtr);

	// sprawdza, czy jest to plik formatu BMP
	if (bitmapFileHeader.bfType != BITMAP_ID)
	{
		fclose(filePtr);
		return NULL;
	}

	// wczytuje nag��wek obrazu
	fread(bitmapInfoHeader, sizeof(BITMAPINFOHEADER), 1, filePtr);

	// ustawia wska�nik pozycji pliku na pocz�tku danych obrazu
	fseek(filePtr, bitmapFileHeader.bfOffBits, SEEK_SET);

	// przydziela pami�� buforowi obrazu
	bitmapImage = (unsigned char*)malloc(bitmapInfoHeader->biSizeImage);

	// prawdza, czy uda�o si� przydzieli� pami��
	if (!bitmapImage)
	{
		free(bitmapImage);
		fclose(filePtr);
		return NULL;
	}

	// wczytuje dane obrazu
	fread(bitmapImage, 1, bitmapInfoHeader->biSizeImage, filePtr);

	// sprawdza, czy dane zosta�y wczytane
	if (bitmapImage == NULL)
	{
		fclose(filePtr);
		return NULL;
	}

	// zamienia miejscami sk�adowe R i B 
	for (imageIdx = 0; imageIdx < bitmapInfoHeader->biSizeImage; imageIdx += 3)
	{
		tempRGB = bitmapImage[imageIdx];
		bitmapImage[imageIdx] = bitmapImage[imageIdx + 2];
		bitmapImage[imageIdx + 2] = tempRGB;
	}

	// zamyka plik i zwraca wska�nik bufora zawieraj�cego wczytany obraz
	fclose(filePtr);
	return bitmapImage;
}

// InitializeTerrain()
// opis: inicjuje dane opisuj�ce teren
void InitializeTerrain()
{
	// oblicza wsp�rz�dne wierzcho�k�w
	// dla wszystkich punkt�w siatki
	for (int z = 0; z < MAP_Z; z++)
	{
		for (int x = 0; x < MAP_X; x++)
		{
			terrain[x][z][0] = float(x)*MAP_SCALE;
			terrain[x][z][1] = float(imageData[(z*MAP_Z + x) * 3]);
			terrain[x][z][2] = -float(z)*MAP_SCALE;
		}
	}
}

bool LoadTextures()
{
	// �aduje obraz tekstury l�du
	landTexture = LoadBitmapFile("green.bmp", &landInfo);
	if (!landTexture)
		return false;

	// �aduje obraz tekstury wody
	waterTexture = LoadBitmapFile("water.bmp", &waterInfo);
	if (!waterTexture)
		return false;

	// tworzy mipmapy l�du
	glGenTextures(1, &land);
	glBindTexture(GL_TEXTURE_2D, land);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	gluBuild2DMipmaps(GL_TEXTURE_2D, GL_RGB, landInfo.biWidth, landInfo.biHeight, GL_RGB, GL_UNSIGNED_BYTE, landTexture);

	// tworzy mipmapy wody
	glGenTextures(1, &water);
	glBindTexture(GL_TEXTURE_2D, water);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	gluBuild2DMipmaps(GL_TEXTURE_2D, GL_RGB, waterInfo.biWidth, waterInfo.biHeight, GL_RGB, GL_UNSIGNED_BYTE, waterTexture);

	return true;
}

void CleanUp()
{
	free(imageData);
	free(landTexture);
	free(waterTexture);
}

// Initialize
// opis: inicjuje OpenGL
void Initialize()
{
	glClearColor(0.0f, 0.0f, 0.0f, 0.0f);		// t�o w kolorze czarnym

	glShadeModel(GL_SMOOTH);					// cieniowanie g�adkie
	glEnable(GL_DEPTH_TEST);					// usuwanie ukrytych powierzchni
	glEnable(GL_CULL_FACE);						// brak oblicze� dla niewidocznych stron wielok�t�w
	glFrontFace(GL_CCW);						// niewidoczne strony posiadaj� porz�dek wierzcho�k�w 
												// przeciwny do kierunku ruchu wskaz�wek zegara

	glEnable(GL_TEXTURE_2D);					// w��cza tekstury dwuwymiarowe

	imageData = LoadBitmapFile("terrain2.bmp", &bitmapInfoHeader);

	// inicjuje dane terenu i �aduje tekstury
	InitializeTerrain();
	LoadTextures();

}

// Render
// opis: rysuje scen�
void Render()
{
	radians = float(PI*(angle - 90.0f) / 180.0f);

	// wyznacza po�o�enie kamery
	cameraX = lookX + sin(radians)*mouseY;	// mno�enie przez mouseY 
	cameraZ = lookZ + cos(radians)*mouseY;  // powoduje zbli�enie b�d� oddalenie kamery
	cameraY = lookY + mouseY / 2.0f;

	// wycelowuje kamer� w �rodek terenu
	lookX = (MAP_X*MAP_SCALE) / 2.0f;
	lookY = 150.0f;
	lookZ = -(MAP_Z*MAP_SCALE) / 2.0f;

	// opr�nia bufory ekranu i g��bi
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glLoadIdentity();

	// umieszcza kamer�
	gluLookAt(cameraX, cameraY, cameraZ, lookX, lookY, lookZ, 0.0, 1.0, 0.0);

	// wybiera tekstur� l�du
	glBindTexture(GL_TEXTURE_2D, land);

	// przegl�da rz�dami wszystkie punkty siatki,
	// rysuj�c dla ka�dego pojedynczy �a�cuch tr�jk�t�w wzd�u� osi x.
	for (int z = 0; z < MAP_Z - 1; z++)
	{
		glBegin(GL_TRIANGLE_STRIP);
		for (int x = 0; x < MAP_X - 1; x++)
		{
			// dla ka�dego wierzcho�ka: obliczamy sk�adowe odcienia szaro�ci
			// wyznaczamy wsp�rz�dne tekstury i rysujemy go.
			/*
			wierzcho�ki rysowane s� w nast�puj�cej kolejno�ci:

			0  ---> 1
			/
			/
			|/
			2  ---> 3
			*/

			// wierzcho�ek 0
			glColor3f(terrain[x][z][1] / 255.0f, terrain[x][z][1] / 255.0f, terrain[x][z][1] / 255.0f);
			glTexCoord2f(0.0f, 0.0f);
			glVertex3f(terrain[x][z][0], terrain[x][z][1], terrain[x][z][2]);

			// wierzcho�ek 1
			glTexCoord2f(1.0f, 0.0f);
			glColor3f(terrain[x + 1][z][1] / 255.0f, terrain[x + 1][z][1] / 255.0f, terrain[x + 1][z][1] / 255.0f);
			glVertex3f(terrain[x + 1][z][0], terrain[x + 1][z][1], terrain[x + 1][z][2]);

			// wierzcho�ek 2
			glTexCoord2f(0.0f, 1.0f);
			glColor3f(terrain[x][z + 1][1] / 255.0f, terrain[x][z + 1][1] / 255.0f, terrain[x][z + 1][1] / 255.0f);
			glVertex3f(terrain[x][z + 1][0], terrain[x][z + 1][1], terrain[x][z + 1][2]);

			//wierzcho�ek 3
			glColor3f(terrain[x + 1][z + 1][1] / 255.0f, terrain[x + 1][z + 1][1] / 255.0f, terrain[x + 1][z + 1][1] / 255.0f);
			glTexCoord2f(1.0f, 1.0f);
			glVertex3f(terrain[x + 1][z + 1][0], terrain[x + 1][z + 1][1], terrain[x + 1][z + 1][2]);
		}
		glEnd();
	}


	// w��cza ��czenie kolor�w
	glEnable(GL_BLEND);

	// prze��cza bufor g��bi w tryb "tylko-do-odczytu"
	glDepthMask(GL_FALSE);

	// okre�la funkcje ��czenia dla efektu przejrzysto�ci
	glBlendFunc(GL_SRC_ALPHA, GL_ONE);

	glColor4f(0.5f, 0.5f, 1.0f, 0.7f);			// kolor niebieski, przezroczysty
	glBindTexture(GL_TEXTURE_2D, water);		// wybiera tekstur� wody

												// rysuje powierzchni� wody jako jeden du�y czworok�t
	glBegin(GL_QUADS);
	glTexCoord2f(0.0f, 0.0f);				// lewy, dolny wierzcho�ek
	glVertex3f(terrain[0][0][0], waterHeight, terrain[0][0][2]);

	glTexCoord2f(10.0f, 0.0f);				// prawy, dolny wierzcho�ek
	glVertex3f(terrain[MAP_X - 1][0][0], waterHeight, terrain[MAP_X - 1][0][2]);

	glTexCoord2f(10.0f, 10.0f);				// prawy, g�rny wierzcho�ek
	glVertex3f(terrain[MAP_X - 1][MAP_Z - 1][0], waterHeight, terrain[MAP_X - 1][MAP_Z - 1][2]);

	glTexCoord2f(0.0f, 10.0f);				// lewy, g�rny wierzcho�ek
	glVertex3f(terrain[0][MAP_Z - 1][0], waterHeight, terrain[0][MAP_Z - 1][2]);
	glEnd();

	// przywraca zwyk�y tryb pracy bufora g��bi
	glDepthMask(GL_TRUE);

	// wy��cza ��czenie kolor�w
	glDisable(GL_BLEND);

	// tworzy animacj� powierzchni wody
	if (waterHeight > 155.0f)
		waterDir = false;
	else if (waterHeight < 154.0f)
		waterDir = true;

	if (waterDir)
		waterHeight += 0.001f;
	else
		waterHeight -= 0.001f;

	glFlush();
	SwapBuffers(g_HDC);			// prze��cza bufory
}

// funkcja okre�laj�ca format pikseli
void SetupPixelFormat(HDC hDC)
{
	int nPixelFormat;					// indeks formatu pikseli

	static PIXELFORMATDESCRIPTOR pfd = {
		sizeof(PIXELFORMATDESCRIPTOR),	// rozmiar struktury
		1,								// domy�lna wersja
		PFD_DRAW_TO_WINDOW |			// grafika w oknie
		PFD_SUPPORT_OPENGL |			// grafika OpenGL 
		PFD_DOUBLEBUFFER,				// podw�jne buforowanie
		PFD_TYPE_RGBA,					// tryb kolor�w RGBA 
		32,								// 32-bitowy opis kolor�w
		0, 0, 0, 0, 0, 0,				// nie specyfikuje bit�w kolor�w
		0,								// bez buforu alfa
		0,								// nie specyfikuje bitu przesuni�cia
		0,								// bez bufora akumulacji
		0, 0, 0, 0,						// ignoruje bity akumulacji
		16,								// 16-bit bufor z
		0,								// bez bufora powielania
		0,								// bez bufor�w pomocniczych
		PFD_MAIN_PLANE,					// g��wna p�aszczyzna rysowania
		0,								// zarezerwowane
		0, 0, 0 };						// ingoruje maski warstw

	nPixelFormat = ChoosePixelFormat(hDC, &pfd);	// wybiera najbardziej zgodny format pikseli 

	SetPixelFormat(hDC, nPixelFormat, &pfd);		// okre�la format pikseli dla danego kontekstu urz�dzenia
}

// procedura okienkowa
LRESULT CALLBACK WndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	static HGLRC hRC;					// kontekst tworzenia grafiki
	static HDC hDC;						// kontekst urz�dzenia
	int width, height;					// szeroko�� i wysoko�� okna
	int oldMouseX, oldMouseY; // wsp�rz�dne poprzedniego po�o�enia myszy

	switch (message)
	{
	case WM_CREATE:					// okno jest tworzone

		hDC = GetDC(hwnd);			// pobiera kontekst urz�dzenia dla okna
		g_HDC = hDC;
		SetupPixelFormat(hDC);		// wywo�uje funkcj� okre�laj�c� format pikseli

									// tworzy kontekst tworzenia grafiki i czyni go bie��cym
		hRC = wglCreateContext(hDC);
		wglMakeCurrent(hDC, hRC);

		return 0;
		break;

	case WM_CLOSE:					// okno jest zamykane

									// deaktywuje bie��cy kontekst tworzenia grafiki i usuwa go
		wglMakeCurrent(hDC, NULL);
		wglDeleteContext(hRC);

		// wstawia komunikat WM_QUIT do kolejki
		PostQuitMessage(0);

		return 0;
		break;

	case WM_SIZE:
		height = HIWORD(lParam);		// pobiera nowe rozmiary okna
		width = LOWORD(lParam);

		if (height == 0)					// unika dzielenie przez 0
		{
			height = 1;
		}

		glViewport(0, 0, width, height);		// nadaje nowe wymairy oknu OpenGL
		glMatrixMode(GL_PROJECTION);			// wybiera macierz rzutowania
		glLoadIdentity();						// resetuje macierz rzutowania

												// wyznacza proporcje obrazu
		gluPerspective(54.0f, (GLfloat)width / (GLfloat)height, 1.0f, 1000.0f);

		glMatrixMode(GL_MODELVIEW);				// wybiera macierz modelowania
		glLoadIdentity();						// resetuje macierz modelowania

		return 0;
		break;

	case WM_KEYDOWN:					// u�ytkownik nacisn�� klawisz?
		keyPressed[wParam] = true;
		return 0;
		break;

	case WM_KEYUP:
		keyPressed[wParam] = false;
		return 0;
		break;

	case WM_MOUSEMOVE:
		// zapami�tuje wsp�rz�dne myszy
		oldMouseX = mouseX;
		oldMouseY = mouseY;

		// pobiera nowe wsp�rz�dne myszy
		mouseX = LOWORD(lParam);
		mouseY = HIWORD(lParam);

		// ograniczenia ruchu kamery
		if (mouseY < 200)
			mouseY = 200;
		if (mouseY > 450)
			mouseY = 450;

		if ((mouseX - oldMouseX) > 0)		// mysz przesuni�ta w prawo
			angle += 3.0f;
		else if ((mouseX - oldMouseX) < 0)	// mysz przesuni�ta w lewo
			angle -= 3.0f;

		return 0;
		break;
	default:
		break;
	}

	return (DefWindowProc(hwnd, message, wParam, lParam));
}

// punkt, w kt�rym rozpoczyna si� wykonywanie aplikacji Windows
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nShowCmd)
{
	WNDCLASSEX windowClass;		// klasa okna
	HWND	   hwnd;			// uchwyt okna
	MSG		   msg;				// komunikat
	bool	   done;			// znacznik zako�czenia aplikacji
	DWORD	   dwExStyle;						// rozszerzony styl okna
	DWORD	   dwStyle;						// styl okna
	RECT	   windowRect;

	// zmienne pomocnicze
	int width = 1300;
	int height = 900;
	int bits = 32;

	//fullScreen = TRUE;

	windowRect.left = (long)0;						// struktura okre�laj�ca rozmiary okna
	windowRect.right = (long)width;
	windowRect.top = (long)0;
	windowRect.bottom = (long)height;

	// definicja klasy okna
	windowClass.cbSize = sizeof(WNDCLASSEX);
	windowClass.style = CS_HREDRAW | CS_VREDRAW;
	windowClass.lpfnWndProc = WndProc;
	windowClass.cbClsExtra = 0;
	windowClass.cbWndExtra = 0;
	windowClass.hInstance = hInstance;
	windowClass.hIcon = LoadIcon(NULL, IDI_APPLICATION);	// domy�lna ikona
	windowClass.hCursor = LoadCursor(NULL, IDC_ARROW);		// domy�lny kursor
	windowClass.hbrBackground = NULL;								// bez t�a
	windowClass.lpszMenuName = NULL;								// bez menu
	windowClass.lpszClassName = "MojaKlasa";
	windowClass.hIconSm = LoadIcon(NULL, IDI_WINLOGO);		// logo Windows

															// rejestruje klas� okna
	if (!RegisterClassEx(&windowClass))
		return 0;

	if (fullScreen)								// tryb pe�noekranowy?
	{
		DEVMODE dmScreenSettings;					// tryb urz�dzenia
		memset(&dmScreenSettings, 0, sizeof(dmScreenSettings));
		dmScreenSettings.dmSize = sizeof(dmScreenSettings);
		dmScreenSettings.dmPelsWidth = width;			// szeroko�� ekranu
		dmScreenSettings.dmPelsHeight = height;			// wysoko�� ekranu
		dmScreenSettings.dmBitsPerPel = bits;				// bit�w na piksel
		dmScreenSettings.dmFields = DM_BITSPERPEL | DM_PELSWIDTH | DM_PELSHEIGHT;

		// 
		if (ChangeDisplaySettings(&dmScreenSettings, CDS_FULLSCREEN) != DISP_CHANGE_SUCCESSFUL)
		{
			// prze��czenie trybu nie powiod�o si�, z powrotem tryb okienkowy
			MessageBox(NULL, "Prze��czenie trybu wy�wietlania nie powiod�o si�", NULL, MB_OK);
			fullScreen = FALSE;
		}

	}

	if (fullScreen)								// tryb pe�noekranowy?
	{
		dwExStyle = WS_EX_APPWINDOW;					// rozszerzony styl okna
		dwStyle = WS_POPUP;						// styl okna
		ShowCursor(FALSE);						// ukrywa kursor myszy
	}
	else
	{
		dwExStyle = WS_EX_APPWINDOW | WS_EX_WINDOWEDGE;			// definicja klasy okna
		dwStyle = WS_OVERLAPPEDWINDOW;					// styl okna
	}

	AdjustWindowRectEx(&windowRect, dwStyle, FALSE, dwExStyle);		// koryguje rozmiar okna

																	// tworzy okno
	hwnd = CreateWindowEx(NULL,									// rozszerzony styl okna
		"MojaKlasa",							// nazwa klasy
		"Tekstury, przyk�ad trzeci: ukszta�towanie terenu",		// app name
		dwStyle | WS_CLIPCHILDREN |
		WS_CLIPSIBLINGS,
		0, 0,								// wsp�rz�dne x,y
		windowRect.right - windowRect.left,
		windowRect.bottom - windowRect.top, // szeroko��, wysoko��
		NULL,									// uchwyt okna nadrz�dnego
		NULL,									// uchwyt menu
		hInstance,							// instancja aplikacji
		NULL);								// bez dodatkowych parametr�w

											// sprawdza, czy utworzenie okna nie powiod�o si� (wtedy warto�� hwnd r�wna NULL)
	if (!hwnd)
		return 0;

	ShowWindow(hwnd, SW_SHOW);			// wy�wietla okno
	UpdateWindow(hwnd);					// aktualizuje okno

	done = false;						// inicjuje zmienn� warunku p�tli
	Initialize();						// inicjuje OpenGL

										// p�tla przetwarzania komunikat�w
	while (!done)
	{
		PeekMessage(&msg, hwnd, NULL, NULL, PM_REMOVE);

		if (msg.message == WM_QUIT)		// aplikacja otrzyma�a komunikat WM_QUIT?
		{
			done = true;				// je�li tak, to ko�czy dzia�anie
		}
		else
		{
			if (keyPressed[VK_ESCAPE])
				done = true;
			else
			{
				Render();

				TranslateMessage(&msg);		// t�umaczy komunikat i wysy�a do systemu
				DispatchMessage(&msg);
			}
		}
	}

	CleanUp();

	if (fullScreen)
	{
		ChangeDisplaySettings(NULL, 0);					// przywraca pulpit
		ShowCursor(TRUE);						// i wska�nik myszy
	}
	return msg.wParam;
}
