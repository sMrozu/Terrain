#define WIN32_LEAN_AND_MEAN		// "odchudza" aplikacjê Windows

/*******************************************************************
*	Rozdzia³ 8,  tekstury - przyk³ad trzeci
*	Autor: Kevin Hawkins
*	Pozwala obracaæ oraz przybli¿aæ i oddalaæ teren.
*	Dane o ukszta³towaniu terenu ³adowane s¹ z pliku BMP.
********************************************************************/

////// Definicje
#define BITMAP_ID 0x4D42		// identyfikator formatu BMP
#define MAP_X	32				// rozmiar mapy wzd³u¿ osi x
#define MAP_Z	32				// rozmiar mapy wzd³u¿ osi y
#define MAP_SCALE	20.0f		// skala mapy
#define PI		3.14159

////// Pliki nag³ówkowe
#include <windows.h>			// standardowy plik nag³ówkowy Windows
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <gl/gl.h>				// standardowy plik nag³ówkowy OpenGL
#include <gl/glu.h>				// funkcje pomocnicze OpenGL
#include<freeglut.h>
////// Zmienne globalne
HDC g_HDC;					// globalny kontekst urz¹dzenia
bool fullScreen = false;			// true = tryb pe³noekranowy; 
									//false = tryb okienkowy
bool keyPressed[256];			// tablica przyciœniêæ klawiszy

float angle = 0.0f;				// k¹t kamery
float radians = 0.0f;			// k¹t kamery w radianach
float waterHeight = 154.0f;		// poziom morza
bool waterDir = true;			// animacja falowania wody; 
								// true = poziom w górê, false = poziom w dó³

								////// Zmienne myszy i kamery
int mouseX, mouseY;						// wspó³rzêdne myszy
float cameraX, cameraY, cameraZ;	// wspó³rzêdne kamery
float lookX, lookY, lookZ;			// punkt wycelowania kamery

									////// Opis tekstury
BITMAPINFOHEADER	bitmapInfoHeader;	// pomocniczy nag³ówek obrazu
BITMAPINFOHEADER	landInfo;			// nag³ówek obrazu tekstury l¹du
BITMAPINFOHEADER	waterInfo;			// nag³ówek obrazu tekstury wody

unsigned char		*imageData;		// dane mapy
unsigned char*      landTexture;	// dane tekstury l¹du
unsigned char*		waterTexture;	// dane tekstury wody
unsigned int		land;			// obiekt tekstury l¹du
unsigned int		water;			// obiekt tekstury wody

									////// Opis terenu
float terrain[MAP_X][MAP_Z][3];		// wysokoœæ terenu w punktach siatki (0-255)

									// LoadBitmapFile
									// opis: ³aduje mapê bitow¹ z pliku i zwraca jej adres.
									//       Wype³nia strukturê nag³ówka.
									//	 Nie obs³uguje map 8-bitowych.
unsigned char *LoadBitmapFile(const char *filename, BITMAPINFOHEADER *bitmapInfoHeader)
{
	FILE *filePtr;							// wskaŸnik pozycji pliku
	BITMAPFILEHEADER	bitmapFileHeader;		// nag³ówek pliku
	unsigned char		*bitmapImage;			// dane obrazu
	int					imageIdx = 0;		// licznik pikseli
	unsigned char		tempRGB;				// zmienna zamiany sk³adowych
	errno_t eFile;

	eFile = fopen_s(&filePtr, filename, "rb");											// otwiera plik w trybie "read binary"

	if (filePtr == NULL)
		return NULL;

	// wczytuje nag³ówek pliku
	fread(&bitmapFileHeader, sizeof(BITMAPFILEHEADER), 1, filePtr);

	// sprawdza, czy jest to plik formatu BMP
	if (bitmapFileHeader.bfType != BITMAP_ID)
	{
		fclose(filePtr);
		return NULL;
	}

	// wczytuje nag³ówek obrazu
	fread(bitmapInfoHeader, sizeof(BITMAPINFOHEADER), 1, filePtr);

	// ustawia wskaŸnik pozycji pliku na pocz¹tku danych obrazu
	fseek(filePtr, bitmapFileHeader.bfOffBits, SEEK_SET);

	// przydziela pamiêæ buforowi obrazu
	bitmapImage = (unsigned char*)malloc(bitmapInfoHeader->biSizeImage);

	// prawdza, czy uda³o siê przydzieliæ pamiêæ
	if (!bitmapImage)
	{
		free(bitmapImage);
		fclose(filePtr);
		return NULL;
	}

	// wczytuje dane obrazu
	fread(bitmapImage, 1, bitmapInfoHeader->biSizeImage, filePtr);

	// sprawdza, czy dane zosta³y wczytane
	if (bitmapImage == NULL)
	{
		fclose(filePtr);
		return NULL;
	}

	// zamienia miejscami sk³adowe R i B 
	for (imageIdx = 0; imageIdx < bitmapInfoHeader->biSizeImage; imageIdx += 3)
	{
		tempRGB = bitmapImage[imageIdx];
		bitmapImage[imageIdx] = bitmapImage[imageIdx + 2];
		bitmapImage[imageIdx + 2] = tempRGB;
	}

	// zamyka plik i zwraca wskaŸnik bufora zawieraj¹cego wczytany obraz
	fclose(filePtr);
	return bitmapImage;
}

// InitializeTerrain()
// opis: inicjuje dane opisuj¹ce teren
void InitializeTerrain()
{
	// oblicza wspó³rzêdne wierzcho³ków
	// dla wszystkich punktów siatki
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
	// ³aduje obraz tekstury l¹du
	landTexture = LoadBitmapFile("green.bmp", &landInfo);
	if (!landTexture)
		return false;

	// ³aduje obraz tekstury wody
	waterTexture = LoadBitmapFile("water.bmp", &waterInfo);
	if (!waterTexture)
		return false;

	// tworzy mipmapy l¹du
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
	glClearColor(0.0f, 0.0f, 0.0f, 0.0f);		// t³o w kolorze czarnym

	glShadeModel(GL_SMOOTH);					// cieniowanie g³adkie
	glEnable(GL_DEPTH_TEST);					// usuwanie ukrytych powierzchni
	glEnable(GL_CULL_FACE);						// brak obliczeñ dla niewidocznych stron wielok¹tów
	glFrontFace(GL_CCW);						// niewidoczne strony posiadaj¹ porz¹dek wierzcho³ków 
												// przeciwny do kierunku ruchu wskazówek zegara

	glEnable(GL_TEXTURE_2D);					// w³¹cza tekstury dwuwymiarowe

	imageData = LoadBitmapFile("terrain2.bmp", &bitmapInfoHeader);

	// inicjuje dane terenu i ³aduje tekstury
	InitializeTerrain();
	LoadTextures();

}

// Render
// opis: rysuje scenê
void Render()
{
	radians = float(PI*(angle - 90.0f) / 180.0f);

	// wyznacza po³o¿enie kamery
	cameraX = lookX + sin(radians)*mouseY;	// mno¿enie przez mouseY 
	cameraZ = lookZ + cos(radians)*mouseY;  // powoduje zbli¿enie b¹dŸ oddalenie kamery
	cameraY = lookY + mouseY / 2.0f;

	// wycelowuje kamerê w œrodek terenu
	lookX = (MAP_X*MAP_SCALE) / 2.0f;
	lookY = 150.0f;
	lookZ = -(MAP_Z*MAP_SCALE) / 2.0f;

	// opró¿nia bufory ekranu i g³êbi
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glLoadIdentity();

	// umieszcza kamerê
	gluLookAt(cameraX, cameraY, cameraZ, lookX, lookY, lookZ, 0.0, 1.0, 0.0);

	// wybiera teksturê l¹du
	glBindTexture(GL_TEXTURE_2D, land);

	// przegl¹da rzêdami wszystkie punkty siatki,
	// rysuj¹c dla ka¿dego pojedynczy ³añcuch trójk¹tów wzd³u¿ osi x.
	for (int z = 0; z < MAP_Z - 1; z++)
	{
		glBegin(GL_TRIANGLE_STRIP);
		for (int x = 0; x < MAP_X - 1; x++)
		{
			// dla ka¿dego wierzcho³ka: obliczamy sk³adowe odcienia szaroœci
			// wyznaczamy wspó³rzêdne tekstury i rysujemy go.
			/*
			wierzcho³ki rysowane s¹ w nastêpuj¹cej kolejnoœci:

			0  ---> 1
			/
			/
			|/
			2  ---> 3
			*/

			// wierzcho³ek 0
			glColor3f(terrain[x][z][1] / 255.0f, terrain[x][z][1] / 255.0f, terrain[x][z][1] / 255.0f);
			glTexCoord2f(0.0f, 0.0f);
			glVertex3f(terrain[x][z][0], terrain[x][z][1], terrain[x][z][2]);

			// wierzcho³ek 1
			glTexCoord2f(1.0f, 0.0f);
			glColor3f(terrain[x + 1][z][1] / 255.0f, terrain[x + 1][z][1] / 255.0f, terrain[x + 1][z][1] / 255.0f);
			glVertex3f(terrain[x + 1][z][0], terrain[x + 1][z][1], terrain[x + 1][z][2]);

			// wierzcho³ek 2
			glTexCoord2f(0.0f, 1.0f);
			glColor3f(terrain[x][z + 1][1] / 255.0f, terrain[x][z + 1][1] / 255.0f, terrain[x][z + 1][1] / 255.0f);
			glVertex3f(terrain[x][z + 1][0], terrain[x][z + 1][1], terrain[x][z + 1][2]);

			//wierzcho³ek 3
			glColor3f(terrain[x + 1][z + 1][1] / 255.0f, terrain[x + 1][z + 1][1] / 255.0f, terrain[x + 1][z + 1][1] / 255.0f);
			glTexCoord2f(1.0f, 1.0f);
			glVertex3f(terrain[x + 1][z + 1][0], terrain[x + 1][z + 1][1], terrain[x + 1][z + 1][2]);
		}
		glEnd();
	}


	// w³¹cza ³¹czenie kolorów
	glEnable(GL_BLEND);

	// prze³¹cza bufor g³êbi w tryb "tylko-do-odczytu"
	glDepthMask(GL_FALSE);

	// okreœla funkcje ³¹czenia dla efektu przejrzystoœci
	glBlendFunc(GL_SRC_ALPHA, GL_ONE);

	glColor4f(0.5f, 0.5f, 1.0f, 0.7f);			// kolor niebieski, przezroczysty
	glBindTexture(GL_TEXTURE_2D, water);		// wybiera teksturê wody

												// rysuje powierzchniê wody jako jeden du¿y czworok¹t
	glBegin(GL_QUADS);
	glTexCoord2f(0.0f, 0.0f);				// lewy, dolny wierzcho³ek
	glVertex3f(terrain[0][0][0], waterHeight, terrain[0][0][2]);

	glTexCoord2f(10.0f, 0.0f);				// prawy, dolny wierzcho³ek
	glVertex3f(terrain[MAP_X - 1][0][0], waterHeight, terrain[MAP_X - 1][0][2]);

	glTexCoord2f(10.0f, 10.0f);				// prawy, górny wierzcho³ek
	glVertex3f(terrain[MAP_X - 1][MAP_Z - 1][0], waterHeight, terrain[MAP_X - 1][MAP_Z - 1][2]);

	glTexCoord2f(0.0f, 10.0f);				// lewy, górny wierzcho³ek
	glVertex3f(terrain[0][MAP_Z - 1][0], waterHeight, terrain[0][MAP_Z - 1][2]);
	glEnd();

	// przywraca zwyk³y tryb pracy bufora g³êbi
	glDepthMask(GL_TRUE);

	// wy³¹cza ³¹czenie kolorów
	glDisable(GL_BLEND);

	// tworzy animacjê powierzchni wody
	if (waterHeight > 155.0f)
		waterDir = false;
	else if (waterHeight < 154.0f)
		waterDir = true;

	if (waterDir)
		waterHeight += 0.001f;
	else
		waterHeight -= 0.001f;

	glFlush();
	SwapBuffers(g_HDC);			// prze³¹cza bufory
}

// funkcja okreœlaj¹ca format pikseli
void SetupPixelFormat(HDC hDC)
{
	int nPixelFormat;					// indeks formatu pikseli

	static PIXELFORMATDESCRIPTOR pfd = {
		sizeof(PIXELFORMATDESCRIPTOR),	// rozmiar struktury
		1,								// domyœlna wersja
		PFD_DRAW_TO_WINDOW |			// grafika w oknie
		PFD_SUPPORT_OPENGL |			// grafika OpenGL 
		PFD_DOUBLEBUFFER,				// podwójne buforowanie
		PFD_TYPE_RGBA,					// tryb kolorów RGBA 
		32,								// 32-bitowy opis kolorów
		0, 0, 0, 0, 0, 0,				// nie specyfikuje bitów kolorów
		0,								// bez buforu alfa
		0,								// nie specyfikuje bitu przesuniêcia
		0,								// bez bufora akumulacji
		0, 0, 0, 0,						// ignoruje bity akumulacji
		16,								// 16-bit bufor z
		0,								// bez bufora powielania
		0,								// bez buforów pomocniczych
		PFD_MAIN_PLANE,					// g³ówna p³aszczyzna rysowania
		0,								// zarezerwowane
		0, 0, 0 };						// ingoruje maski warstw

	nPixelFormat = ChoosePixelFormat(hDC, &pfd);	// wybiera najbardziej zgodny format pikseli 

	SetPixelFormat(hDC, nPixelFormat, &pfd);		// okreœla format pikseli dla danego kontekstu urz¹dzenia
}

// procedura okienkowa
LRESULT CALLBACK WndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	static HGLRC hRC;					// kontekst tworzenia grafiki
	static HDC hDC;						// kontekst urz¹dzenia
	int width, height;					// szerokoœæ i wysokoœæ okna
	int oldMouseX, oldMouseY; // wspó³rzêdne poprzedniego po³o¿enia myszy

	switch (message)
	{
	case WM_CREATE:					// okno jest tworzone

		hDC = GetDC(hwnd);			// pobiera kontekst urz¹dzenia dla okna
		g_HDC = hDC;
		SetupPixelFormat(hDC);		// wywo³uje funkcjê okreœlaj¹c¹ format pikseli

									// tworzy kontekst tworzenia grafiki i czyni go bie¿¹cym
		hRC = wglCreateContext(hDC);
		wglMakeCurrent(hDC, hRC);

		return 0;
		break;

	case WM_CLOSE:					// okno jest zamykane

									// deaktywuje bie¿¹cy kontekst tworzenia grafiki i usuwa go
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

	case WM_KEYDOWN:					// u¿ytkownik nacisn¹³ klawisz?
		keyPressed[wParam] = true;
		return 0;
		break;

	case WM_KEYUP:
		keyPressed[wParam] = false;
		return 0;
		break;

	case WM_MOUSEMOVE:
		// zapamiêtuje wspó³rzêdne myszy
		oldMouseX = mouseX;
		oldMouseY = mouseY;

		// pobiera nowe wspó³rzêdne myszy
		mouseX = LOWORD(lParam);
		mouseY = HIWORD(lParam);

		// ograniczenia ruchu kamery
		if (mouseY < 200)
			mouseY = 200;
		if (mouseY > 450)
			mouseY = 450;

		if ((mouseX - oldMouseX) > 0)		// mysz przesuniêta w prawo
			angle += 3.0f;
		else if ((mouseX - oldMouseX) < 0)	// mysz przesuniêta w lewo
			angle -= 3.0f;

		return 0;
		break;
	default:
		break;
	}

	return (DefWindowProc(hwnd, message, wParam, lParam));
}

// punkt, w którym rozpoczyna siê wykonywanie aplikacji Windows
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nShowCmd)
{
	WNDCLASSEX windowClass;		// klasa okna
	HWND	   hwnd;			// uchwyt okna
	MSG		   msg;				// komunikat
	bool	   done;			// znacznik zakoñczenia aplikacji
	DWORD	   dwExStyle;						// rozszerzony styl okna
	DWORD	   dwStyle;						// styl okna
	RECT	   windowRect;

	// zmienne pomocnicze
	int width = 1300;
	int height = 900;
	int bits = 32;

	//fullScreen = TRUE;

	windowRect.left = (long)0;						// struktura okreœlaj¹ca rozmiary okna
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
	windowClass.hIcon = LoadIcon(NULL, IDI_APPLICATION);	// domyœlna ikona
	windowClass.hCursor = LoadCursor(NULL, IDC_ARROW);		// domyœlny kursor
	windowClass.hbrBackground = NULL;								// bez t³a
	windowClass.lpszMenuName = NULL;								// bez menu
	windowClass.lpszClassName = "MojaKlasa";
	windowClass.hIconSm = LoadIcon(NULL, IDI_WINLOGO);		// logo Windows

															// rejestruje klasê okna
	if (!RegisterClassEx(&windowClass))
		return 0;

	if (fullScreen)								// tryb pe³noekranowy?
	{
		DEVMODE dmScreenSettings;					// tryb urz¹dzenia
		memset(&dmScreenSettings, 0, sizeof(dmScreenSettings));
		dmScreenSettings.dmSize = sizeof(dmScreenSettings);
		dmScreenSettings.dmPelsWidth = width;			// szerokoœæ ekranu
		dmScreenSettings.dmPelsHeight = height;			// wysokoœæ ekranu
		dmScreenSettings.dmBitsPerPel = bits;				// bitów na piksel
		dmScreenSettings.dmFields = DM_BITSPERPEL | DM_PELSWIDTH | DM_PELSHEIGHT;

		// 
		if (ChangeDisplaySettings(&dmScreenSettings, CDS_FULLSCREEN) != DISP_CHANGE_SUCCESSFUL)
		{
			// prze³¹czenie trybu nie powiod³o siê, z powrotem tryb okienkowy
			MessageBox(NULL, "Prze³¹czenie trybu wyœwietlania nie powiod³o siê", NULL, MB_OK);
			fullScreen = FALSE;
		}

	}

	if (fullScreen)								// tryb pe³noekranowy?
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
		"Tekstury, przyk³ad trzeci: ukszta³towanie terenu",		// app name
		dwStyle | WS_CLIPCHILDREN |
		WS_CLIPSIBLINGS,
		0, 0,								// wspó³rzêdne x,y
		windowRect.right - windowRect.left,
		windowRect.bottom - windowRect.top, // szerokoœæ, wysokoœæ
		NULL,									// uchwyt okna nadrzêdnego
		NULL,									// uchwyt menu
		hInstance,							// instancja aplikacji
		NULL);								// bez dodatkowych parametrów

											// sprawdza, czy utworzenie okna nie powiod³o siê (wtedy wartoœæ hwnd równa NULL)
	if (!hwnd)
		return 0;

	ShowWindow(hwnd, SW_SHOW);			// wyœwietla okno
	UpdateWindow(hwnd);					// aktualizuje okno

	done = false;						// inicjuje zmienn¹ warunku pêtli
	Initialize();						// inicjuje OpenGL

										// pêtla przetwarzania komunikatów
	while (!done)
	{
		PeekMessage(&msg, hwnd, NULL, NULL, PM_REMOVE);

		if (msg.message == WM_QUIT)		// aplikacja otrzyma³a komunikat WM_QUIT?
		{
			done = true;				// jeœli tak, to koñczy dzia³anie
		}
		else
		{
			if (keyPressed[VK_ESCAPE])
				done = true;
			else
			{
				Render();

				TranslateMessage(&msg);		// t³umaczy komunikat i wysy³a do systemu
				DispatchMessage(&msg);
			}
		}
	}

	CleanUp();

	if (fullScreen)
	{
		ChangeDisplaySettings(NULL, 0);					// przywraca pulpit
		ShowCursor(TRUE);						// i wskaŸnik myszy
	}
	return msg.wParam;
}
