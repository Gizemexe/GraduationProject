#include "icb_gui.h" 
#include "ic_media.h" 
#include "icbytes.h"
#include <windows.h>
#include <stdio.h>
#include <commdlg.h> // Save file dialog i�in
#include <iostream> 


// Global de�i�kenler
ICBYTES m;               // �izim alan�
int FRM1;                 // �izim alan� paneli
bool isDrawing = false;   // �izim durumunu takip etme
int prevX = -1, prevY = -1; // �nceki fare konumu

HANDLE drawThread = NULL;    // �izim i� par�ac���
DWORD drawThreadID;          // �� par�ac��� ID'si
bool threadStopFlag = false; // �� par�ac���n� durdurma bayra��



int SLE1;               // Kademe kutucu�unun ba�l� oldu�u metin kutusu
int RTrackBar; // Renk kademeleri i�in track barlar
int selectedColor = 0x000000; // Siyah
int ColorPreviewFrame; // Renk �nizleme �er�evesi
ICBYTES ColorPreview;  // Renk �nizleme alan� i�in bir matris

int MouseLogBox; // Fare hareketlerini g�stermek i�in metin kutusu

int frameOffsetX = 50; // �er�eve X ba�lang�� konumu
int frameOffsetY = 100; // �er�eve Y ba�lang�� konumu

int lineThickness = 1; // Varsay�lan olarak 1 piksel kal�nl�k

int startX = -1, startY = -1; // Fare s�r�kleme ba�lang�� pozisyonu
int currentX = -1, currentY = -1; // Fare s�r�kleme ge�erli pozisyonu

enum Mode { NORMAL, ELLIPSE, RECTANGLE, FILLED_RECTANGLE, LINE };
Mode activeMode = NORMAL; // Varsay�lan mod NORMAL


// �izim Alan�n� Temizle
void ClearCanvas() {
    // E�er zaten beyazsa tekrar temizlemeye gerek yok
    static bool isCleared = false;
    if (!isCleared) {
        FillRect(m, 0, 0, 800, 600, 0xFFFFFF);
        isCleared = true;
    }
    DisplayImage(FRM1, m);
}


// GUI Ba�latma Fonksiyonu
void ICGUI_Create() {
    ICG_MWSize(1000, 600);  
    ICG_MWTitle("Paint Uygulamasi - ICBYTES");  
}

void UpdateLineThickness(int value) {
    lineThickness = value; // Kal�nl�k de�erini g�ncelle
    ICG_printf(MouseLogBox, "Line thickness updated: %d\n", lineThickness);
}

void CreateThicknessTrackbar() {
    ICG_TrackBarH(700, 30, 200, 30, UpdateLineThickness); // Trackbar pozisyonu ve uzunlu�u
}

// �izgi �izme Fonksiyonu
void DrawLine(ICBYTES& canvas, int x1, int y1, int x2, int y2, int color) {
    int dx = abs(x2 - x1), dy = abs(y2 - y1);
    int sx = (x1 < x2) ? 1 : -1, sy = (y1 < y2) ? 1 : -1, err = dx - dy;

    while (true) {
        for (int i = -lineThickness / 2; i <= lineThickness / 2; i++) {
            for (int j = -lineThickness / 2; j <= lineThickness / 2; j++) {
                int px = x1 + i;
                int py = y1 + j;
                if (px >= 0 && py >= 0 && px < canvas.X() && py < canvas.Y()) {
                    canvas.B(px, py, 0) = color & 0xFF;
                    canvas.B(px, py, 1) = (color >> 8) & 0xFF;
                    canvas.B(px, py, 2) = (color >> 16) & 0xFF;
                }
            }
        }
        if (x1 == x2 && y1 == y2) break;
        int e2 = 2 * err;
        if (e2 > -dy) { err -= dy; x1 += sx; }
        if (e2 < dx) { err += dx; y1 += sy; }
    }
}




// Kademe Kutucu�u :Renk Bile�eni G�ncelleme
void UpdateColor(int kademe) {
    // �rnek olarak basit bir RGB renk paleti
    int red = kademe * 10 % 256; // Renk tonlar�n� farkl� yap
    int green = (kademe * 20) % 256;
    int blue = (kademe * 30) % 256;

    selectedColor = (red << 16) | (green << 8) | blue;

    // G�ncellenen rengi g�ster
    ICG_SetWindowText(SLE1, "");
    ICG_printf(SLE1, "Selected Color: 0x%X\n", selectedColor);
    FillRect(ColorPreview, 0, 0, 50, 50, selectedColor);
    DisplayImage(ColorPreviewFrame, ColorPreview);
}

void UpdatePreview() {
    FillRect(ColorPreview, 0, 0, 50, 50, selectedColor);
    DisplayImage(ColorPreviewFrame, ColorPreview);
    ICG_printf(MouseLogBox, "Selected color: 0x%X\n", selectedColor);
}

void SelectColor1() { selectedColor = 0x000000; UpdatePreview(); }
void SelectColor2() { selectedColor = 0x6d6d6d; UpdatePreview(); }
void SelectColor3() { selectedColor = 0xFF0000; UpdatePreview(); }
void SelectColor4() { selectedColor = 0x500000; UpdatePreview(); }
void SelectColor5() { selectedColor = 0xfc4e03; UpdatePreview(); }
void SelectColor6() { selectedColor = 0xfce803; UpdatePreview(); }
void SelectColor7() { selectedColor = 0x00FF00; UpdatePreview(); }
void SelectColor8() { selectedColor = 0x00c4e2; UpdatePreview(); }
void SelectColor9() { selectedColor = 0x0919ca; UpdatePreview(); }
void SelectColor10() { selectedColor = 0x8209c0; UpdatePreview(); }
//void SelectColor2() { selectedColor = 0x8209c0; UpdatePreview(); } 


void CreateColorButtons() {

    int CBTN1, CBTN2, CBTN3, CBTN4, CBTN5, CBTN6, CBTN7, CBTN8, CBTN9, CBTN10;

    static ICBYTES Cbutonresmi, Cbutonresmi1, Cbutonresmi2, Cbutonresmi3, Cbutonresmi4, Cbutonresmi5, Cbutonresmi6, Cbutonresmi7, Cbutonresmi8, Cbutonresmi9;

    CreateImage(Cbutonresmi, 50, 50, ICB_UINT);
    Cbutonresmi = 0x000000;
    CBTN1 = ICG_BitmapButton(300, 10, 25, 25, SelectColor1);
    SetButtonBitmap(CBTN1, Cbutonresmi);

    CreateImage(Cbutonresmi1, 50, 50, ICB_UINT);
    Cbutonresmi1 = 0x6d6d6d;
    CBTN2 = ICG_BitmapButton(330, 10, 25, 25, SelectColor2);
    SetButtonBitmap(CBTN2, Cbutonresmi1);

    CreateImage(Cbutonresmi2, 50, 50, ICB_UINT);
    Cbutonresmi2 = 0xFF0000;
    CBTN3 = ICG_BitmapButton(360, 10, 25, 25, SelectColor3);
    SetButtonBitmap(CBTN3, Cbutonresmi2);

    CreateImage(Cbutonresmi3, 50, 50, ICB_UINT);
    Cbutonresmi3 = 0x500000;
    CBTN4 = ICG_BitmapButton(390, 10, 25, 25, SelectColor4);
    SetButtonBitmap(CBTN4, Cbutonresmi3);

    CreateImage(Cbutonresmi4, 50, 50, ICB_UINT);
    Cbutonresmi4 = 0xfc4e03;
    CBTN5 = ICG_BitmapButton(420, 10, 25, 25, SelectColor5);
    SetButtonBitmap(CBTN5, Cbutonresmi4);
// Alt sat�r 
    CreateImage(Cbutonresmi5, 50, 50, ICB_UINT);
    Cbutonresmi5 = 0xfce803;
    CBTN6 = ICG_BitmapButton(300, 40, 25, 25, SelectColor6);
    SetButtonBitmap(CBTN6, Cbutonresmi5);

    CreateImage(Cbutonresmi6, 50, 50, ICB_UINT);
    Cbutonresmi6 = 0x00FF00;
    CBTN7 = ICG_BitmapButton(330, 40, 25, 25, SelectColor7);
    SetButtonBitmap(CBTN7, Cbutonresmi6);

    CreateImage(Cbutonresmi7, 50, 50, ICB_UINT);
    Cbutonresmi7 = 0x00c4e2;
    CBTN8 = ICG_BitmapButton(360, 40, 25, 25, SelectColor8);
    SetButtonBitmap(CBTN8, Cbutonresmi7);

    CreateImage(Cbutonresmi8, 50, 50, ICB_UINT);
    Cbutonresmi8 = 0x0919ca;
    CBTN9 = ICG_BitmapButton(390, 40, 25, 25, SelectColor9);
    SetButtonBitmap(CBTN9, Cbutonresmi8);

    CreateImage(Cbutonresmi9, 50, 50, ICB_UINT);
    Cbutonresmi9 = 0x8209c0;
    CBTN10 = ICG_BitmapButton(420, 40, 25, 25, SelectColor10);
    SetButtonBitmap(CBTN10, Cbutonresmi9);
    
}


void InitializeColorPreview() {
    CreateImage(ColorPreview, 50, 50, ICB_UINT);
    SLE1 = ICG_SLEditBorder(10, 10, 200, 25, "0");
    RTrackBar = ICG_TrackBarH(10, 50, 200, 30, UpdateColor);
    // �er�eve olu�tur
    ColorPreviewFrame = ICG_FrameMedium(220, 10, 70, 70);
    ColorPreview = 0xff00;
    DisplayImage(ColorPreviewFrame, ColorPreview);
}

void LogMouseAction(const char* action, int x, int y) {
    if (x < 0 || x >= m.X() || y < 0 || y >= m.Y()) {
        ICG_printf(MouseLogBox, "%s - Out of bounds - X: %d, Y: %d\n", action, x, y);
    }
    else {
        ICG_printf(MouseLogBox, "%s - X: %d, Y: %d\n", action, x, y);
    }
}

HANDLE drawEvent = CreateEvent(NULL, FALSE, FALSE, NULL);

DWORD WINAPI DrawingThread(LPVOID param) {
    while (!threadStopFlag) {
        WaitForSingleObject(drawEvent, INFINITE); // Olay tetiklenene kadar bekle
        if (threadStopFlag) break;

        // �izim i�lemleri
        if (isDrawing && prevX >= 0 && prevY >= 0) {
            int currentX = ICG_GetMouseX() - frameOffsetX;
            int currentY = ICG_GetMouseY() - frameOffsetY;
            DrawLine(m, prevX, prevY, currentX, currentY, selectedColor);
            DisplayImage(FRM1, m);
            prevX = currentX;
            prevY = currentY;
        }
    }
    return 0;
}


void StartDrawingThread() {
    if (!drawThread || threadStopFlag) {
        threadStopFlag = false;
        drawThread = CreateThread(NULL, 0, DrawingThread, NULL, 0, &drawThreadID);
    }
}

void SignalDrawingThread() {
    SetEvent(drawEvent); // �� par�ac���n� tetikle
}

void StopDrawingThread() {
    if (drawThread) {
        threadStopFlag = true;
        SignalDrawingThread(); // �� par�ac���n� uyar
        WaitForSingleObject(drawThread, INFINITE);
        CloseHandle(drawThread);
        drawThread = NULL;
    }
}


// Sol Fare Tu�una Bas�ld���nda
void OnMouseLDown() {
    isDrawing = true;
    prevX = ICG_GetMouseX() - frameOffsetX;
    prevY = ICG_GetMouseY() - frameOffsetY;

    if (activeMode == NORMAL) {
        StartDrawingThread(); // Serbest �izim i�in thread ba�lat
    }
    else {
        startX = prevX;
        startY = prevY;
    }
}

// Fare Hareket Etti�inde
void OnMouseMove(int x, int y) {
    int currentX = ICG_GetMouseX() - frameOffsetX;
    int currentY = ICG_GetMouseY() - frameOffsetY;

    if (isDrawing) {
        if (activeMode == NORMAL) {
            // Serbest �izim modu
            if (prevX >= 0 && prevY >= 0) {
                DrawLine(m, prevX, prevY, currentX, currentY, selectedColor);
                DisplayImage(FRM1, m); // �izimi kal�c� olarak ana tuvale uygula
            }
            prevX = currentX;
            prevY = currentY;
        }
        else {
            // Dinamik �ekil �izim modu
            ClearCanvas(); // Tuvali temizle
            DisplayImage(FRM1, m); // �nceki �izimleri yeniden y�kle

            if (activeMode == RECTANGLE) {
                Rect(m, startX, startY, currentX, currentY, selectedColor);
            }
            else if (activeMode == FILLED_RECTANGLE) {
                FillRect(m, startX, startY, currentX, currentY, selectedColor);
            }
            else if (activeMode == ELLIPSE) {
                int radiusX = abs(currentX - startX) / 2;
                int radiusY = abs(currentY - startY) / 2;
                Ellipse(m, (startX + currentX) / 2, (startY + currentY) / 2, radiusX, radiusY, selectedColor);
            }
            else if (activeMode == LINE) {
                DrawLine(m, startX, startY, currentX, currentY, selectedColor);
            }

            DisplayImage(FRM1, m); // Ge�ici �izimi ekrana g�ster
        }
    }
}

// Sol Fare Tu�unu B�rakt���n�zda
void OnMouseLUp() {
    isDrawing = false;

    if (activeMode != NORMAL) {
        int currentX = ICG_GetMouseX() - frameOffsetX;
        int currentY = ICG_GetMouseY() - frameOffsetY;

        if (activeMode == RECTANGLE) {
            Rect(m, startX, startY, currentX, currentY, selectedColor);
        }
        else if (activeMode == FILLED_RECTANGLE) {
            FillRect(m, startX, startY, currentX, currentY, selectedColor);
        }
        else if (activeMode == ELLIPSE) {
            int radiusX = abs(currentX - startX) / 2;
            int radiusY = abs(currentY - startY) / 2;
            Ellipse(m, (startX + currentX) / 2, (startY + currentY) / 2, radiusX, radiusY, selectedColor);
        }
        else if (activeMode == LINE) {
            DrawLine(m, startX, startY, currentX, currentY, selectedColor);
        }
        DisplayImage(FRM1, m); // �izimi kal�c� hale getir
    }

    StopDrawingThread(); // Thread'i durdur
    prevX = prevY = -1;
}



// Fare Olaylar�n� Ayarlama
void SetupMouseHandlers() {
    ICG_SetOnMouseLDown(OnMouseLDown);
    ICG_SetOnMouseMove(OnMouseMove);
    ICG_SetOnMouseLUp(OnMouseLUp);
}

// Canvas Ba�lat
void InitializeCanvas() {
    int canvasWidth = 800;
    int canvasHeight = 400;

    // �er�eve olu�tur
    FRM1 = ICG_FrameMedium(frameOffsetX, frameOffsetY, canvasWidth, canvasHeight);

    // Matris olu�tur ve beyaza ayarla
    CreateMatrix(m, canvasWidth, canvasHeight, 3, ICB_UCHAR);
    FillRect(m, 0, 0, canvasWidth, canvasHeight, 0xFFFFFF);

    // Matris �er�eveye yans�t�l�yor
    DisplayMatrix(FRM1, m);  // Burada �er�eve ID'si kullan�lmal�
    ICG_printf(MouseLogBox, "Matrix updated on frame %d\n", FRM1);
    ICG_printf(MouseLogBox, "Matrix size: %d x %d, Frame ID: %d\n", m.X(), m.Y(), FRM1);


}

void NewFunc(){
    ICG_DestroyWidget(FRM1);

    // Yeni �er�eve olu�tur (800x600 boyutunda)
    FRM1 = ICG_FrameMedium(frameOffsetX, frameOffsetY, 800, 400);

    // Canvas'� beyaza s�f�rla
    m = FRM1;
    FillRect(m, 0, 0, 800, 600, 0xFFFFFF);
    DisplayMatrix(FRM1, m);
}

void CleanupResources() {
    StopDrawingThread(); // Thread'i durdur
    Free(m); // Belle�i serbest b�rak
    Free(ColorPreview); // Renk �nizleme matrisi belle�ini serbest b�rak
}


void OpenFunc(){

    ICBYTES dosyol, resim;
    ReadImage(OpenFileMenu(dosyol, "JPEG\0*.JPG\0"), resim);
    DisplayImage(FRM1, resim);
}

// Bitmap'i dosyaya kaydetme fonksiyonu
bool SaveBitmapToFile(HBITMAP hBitmap, const char* filePath) {
    BITMAP bmp;
    GetObject(hBitmap, sizeof(BITMAP), &bmp);

    BITMAPFILEHEADER bmfHeader;
    BITMAPINFOHEADER bi;

    bi.biSize = sizeof(BITMAPINFOHEADER);
    bi.biWidth = bmp.bmWidth;
    bi.biHeight = -bmp.bmHeight; // Pozitif de�er ters g�r�nt� kaydeder
    bi.biPlanes = 1;
    bi.biBitCount = 32; // 32 bit renk
    bi.biCompression = BI_RGB;
    bi.biSizeImage = 0;
    bi.biXPelsPerMeter = 0;
    bi.biYPelsPerMeter = 0;
    bi.biClrUsed = 0;
    bi.biClrImportant = 0;

    DWORD dwBmpSize = ((bmp.bmWidth * bi.biBitCount + 31) / 32) * 4 * bmp.bmHeight;
    HANDLE hDIB = GlobalAlloc(GHND, dwBmpSize);
    char* lpbitmap = (char*)GlobalLock(hDIB);

    GetDIBits(GetDC(0), hBitmap, 0, (UINT)bmp.bmHeight, lpbitmap, (BITMAPINFO*)&bi, DIB_RGB_COLORS);

    HANDLE hFile = CreateFile(filePath, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    if (hFile == INVALID_HANDLE_VALUE) {
        GlobalUnlock(hDIB);
        GlobalFree(hDIB);
        return false;
    }

    DWORD dwSizeofDIB = dwBmpSize + sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER);
    bmfHeader.bfOffBits = sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER);
    bmfHeader.bfSize = dwSizeofDIB;
    bmfHeader.bfType = 0x4D42; // BM

    DWORD dwBytesWritten;
    WriteFile(hFile, (LPSTR)&bmfHeader, sizeof(BITMAPFILEHEADER), &dwBytesWritten, NULL);
    WriteFile(hFile, (LPSTR)&bi, sizeof(BITMAPINFOHEADER), &dwBytesWritten, NULL);
    WriteFile(hFile, (LPSTR)lpbitmap, dwBmpSize, &dwBytesWritten, NULL);

    CloseHandle(hFile);
    GlobalUnlock(hDIB);
    GlobalFree(hDIB);

    return true;
}

void SaveFunc(){
    OPENFILENAME ofn;       // Dosya se�me ileti�im kutusu
    char file_name[260] = { 0 };
    ZeroMemory(&ofn, sizeof(ofn));

    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = ICG_GetMainWindow(); // Ana pencere
    ofn.lpstrFilter = "Bitmap Files\0*.bmp\0";
    ofn.lpstrFile = file_name;
    ofn.nMaxFile = sizeof(file_name);
    ofn.Flags = OFN_OVERWRITEPROMPT;
    ofn.lpstrDefExt = "bmp";

    if (GetSaveFileName(&ofn)) {
        // HBITMAP al
        HBITMAP bitmap = m.GetHBitmap();
        if (bitmap) {
            // Bitmap'i dosyaya kaydet
            if (SaveBitmapToFile(bitmap, ofn.lpstrFile)) {
                MessageBox(ICG_GetMainWindow(), "Resim ba�ar�yla kaydedildi.", "Bilgi", MB_OK);
            }
            else {
                MessageBox(ICG_GetMainWindow(), "Resim kaydedilemedi.", "Hata", MB_OK);
            }
            DeleteObject(bitmap);
        }
        else {
            MessageBox(ICG_GetMainWindow(), "Bitmap olu�turulamad�.", "Hata", MB_OK);
        }
    }
}

void ExitFunc(){
    CleanupResources();
    PostQuitMessage(0);  // Programdan g�venli bir �ekilde ��k�� yapar
}
void CutFunc(){}
void CopyFunc(){}
void PasteFunc(){}
void Elipsee() {
    activeMode = ELLIPSE;  // Elips modunu etkinle�tir
    ICG_printf(MouseLogBox, "Ellipse mode activated.\n");
}

void NormalMode() {
    activeMode = NORMAL;  // Normal modu etkinle�tir
    ICG_printf(MouseLogBox, "Normal mode activated.\n");
}
void Rectt(){
    activeMode = RECTANGLE;  // �er�eveli dikd�rtgen modunu etkinle�tir
    ICG_printf(MouseLogBox, "Rectangle mode activated.\n");
}
void Rectfil(){
    activeMode = FILLED_RECTANGLE;  // Dolu dikd�rtgen modunu etkinle�tir
    ICG_printf(MouseLogBox, "Filled Rectangle mode activated.\n");
}
void Linee(){
    activeMode = LINE;  // �izgi modunu etkinle�tir
    ICG_printf(MouseLogBox, "Line mode activated.\n");
}

void CreateModeButtons() {
    ICG_Button(700, 70, 80, 30, "Normal", NormalMode);
    //ICG_Button(700, 110, 80, 30, "Ellipse", Elipsee);
}

void ToggleFullScreen() {
    static bool isFullScreen = false; // Tam ekran modunu takip eder
    static WINDOWPLACEMENT wpPrev = { sizeof(wpPrev) }; // �nceki pencere boyutu
    HWND hwnd = GetActiveWindow(); // Aktif pencereyi al

    if (!isFullScreen) {
        // Tam ekran moduna ge�
        MONITORINFO mi = { sizeof(mi) };
        if (GetWindowPlacement(hwnd, &wpPrev) && GetMonitorInfo(MonitorFromWindow(hwnd, MONITOR_DEFAULTTOPRIMARY), &mi)) {
            // WS_OVERLAPPEDWINDOW stilini koruyarak pencereyi b�y�t
            SetWindowPos(hwnd, HWND_TOP, mi.rcMonitor.left, mi.rcMonitor.top,
                mi.rcMonitor.right - mi.rcMonitor.left,
                mi.rcMonitor.bottom - mi.rcMonitor.top,
                SWP_NOZORDER | SWP_FRAMECHANGED);
            isFullScreen = true;
        }
    }
    else {
        // �nceki pencere boyutuna d�n
        SetWindowPlacement(hwnd, &wpPrev);
        isFullScreen = false;
    }
}
void CreateMenuItems() {
    // Men�leri tan�mla
    HMENU AnaMenu, DosyaMenu, DuzenleMenu, GorunumMenu;

    AnaMenu = CreateMenu();
    DosyaMenu = CreatePopupMenu();
    DuzenleMenu = CreatePopupMenu();
    GorunumMenu = CreatePopupMenu();

    // Dosya Men�s�
    ICG_AppendMenuItem(DosyaMenu, "Yeni", NewFunc);
    ICG_AppendMenuItem(DosyaMenu, "A�", OpenFunc);
    ICG_AppendMenuItem(DosyaMenu, "Kaydet", SaveFunc);
    ICG_AppendMenuItem(DosyaMenu, "��k��", ExitFunc);

    // D�zenle Men�s�
    ICG_AppendMenuItem(DuzenleMenu, "Kes", CutFunc);
    ICG_AppendMenuItem(DuzenleMenu, "Kopyala", CopyFunc);
    ICG_AppendMenuItem(DuzenleMenu, "Yap��t�r", PasteFunc);

    // G�r�n�m Men�s�
    ICG_AppendMenuItem(GorunumMenu, "Yak�nla�t�rma", PasteFunc);
    ICG_AppendMenuItem(GorunumMenu, "Cetveller", PasteFunc);
    ICG_AppendMenuItem(GorunumMenu, "K�lavuz �izgileri", PasteFunc);
    ICG_AppendMenuItem(GorunumMenu, "Durum �ubu�u", PasteFunc);
    ICG_AppendMenuItem(GorunumMenu, "Tam Ekran", ToggleFullScreen);

    // Ana Men�ye ekle
    AppendMenu(AnaMenu, MF_POPUP, (UINT_PTR)DosyaMenu, "Dosya");
    AppendMenu(AnaMenu, MF_POPUP, (UINT_PTR)DuzenleMenu, "D�zenle");
    AppendMenu(AnaMenu, MF_POPUP, (UINT_PTR)GorunumMenu, "G�r�n�m");

    // Ana Men�'y� GUI'ye ata
    ICG_SetMenu(AnaMenu);
}
void CreateDrawingButtons() {
    int BTN1, BTN2, BTN3, BTN4, BTN5;

    static ICBYTES butonresmi, butonresmi1, butonresmi2, butonresmi3, butonresmi4;

    // BTN1: Elips
    CreateImage(butonresmi, 50, 50, ICB_UINT);
    butonresmi = 0xffffff;
    BTN1 = ICG_BitmapButton(450, 10, 40, 40, Elipsee);
    Ellipse(butonresmi, 9, 8, 17, 17, 0x0000000);
    Ellipse(butonresmi, 10, 9, 16, 16, 0x0000000);
    Ellipse(butonresmi, 11, 10, 15, 15, 0x0000000);
    
    SetButtonBitmap(BTN1, butonresmi);

    // BTN2: Dolu Dikd�rtgen
    CreateImage(butonresmi1, 50, 50, ICB_UINT);
    butonresmi1 = 0xffffff;
    BTN2 = ICG_BitmapButton(500, 10, 40, 40, Rectfil);
    FillRect(butonresmi1, 10, 9, 30, 30, 0x0000000);
    SetButtonBitmap(BTN2, butonresmi1);

    // BTN3: �er�eve Dikd�rtgen
    CreateImage(butonresmi2, 50, 50, ICB_UINT);
    butonresmi2 = 0xffffff;
    BTN3 = ICG_BitmapButton(550, 10, 40, 40, Rectt);
    Rect(butonresmi2, 8, 9, 30, 30, 0x0000000);
    Rect(butonresmi2, 9, 10, 30, 30, 0x0000000);
    Rect(butonresmi2, 10, 11, 30, 30, 0x0000000);
    SetButtonBitmap(BTN3, butonresmi2);

    // BTN4: �izgi
    CreateImage(butonresmi3, 50, 50, ICB_UINT);
    butonresmi3 = 0xffffff;
    BTN4 = ICG_BitmapButton(600, 10, 40, 40, Linee);
    Line(butonresmi3, 10, 11, 40, 40, 0x0000000);
    Line(butonresmi3, 11, 12, 40, 40, 0x0000000);
    Line(butonresmi3, 12, 13, 40, 40, 0x0000000);
    SetButtonBitmap(BTN4, butonresmi3);

    // BTN5: Art� ��areti
    CreateImage(butonresmi4, 50, 50, ICB_UINT);
    butonresmi4 = 0xffffff;
    BTN5 = ICG_BitmapButton(650, 10, 40, 40, Linee);
    MarkPlus(butonresmi4, 25, 25, 10, 0);
    SetButtonBitmap(BTN5, butonresmi4);
}


// Ana GUI Fonksiyonu
void ICGUI_main() {
    ICGUI_Create();  // GUI ba�lat

    //Bitmap Buton Ekleme 
    int SaveButton;
    static ICBYTES saveIcon;
    ReadImage("save.bmp", saveIcon);
    SaveButton = ICG_BitmapButton(900, 10, 40, 40, SaveFunc);
    SetButtonBitmap(SaveButton, saveIcon);

    CreateMenuItems();          // Men�leri olu�tur
    InitializeCanvas();    // �izim alan�n� ba�lat
    InitializeColorPreview(); // Renk Paletini Ekle
    CreateColorButtons();       // Renk se�ici butonlar� ekle
    CreateThicknessTrackbar();  // �izgi kal�nl��� i�in trackbar ekle
    CreateModeButtons();  // Mod butonlar�n� ekle
    SetupMouseHandlers(); // Fare olaylar�n� ayarla
    CreateDrawingButtons(); // �izim butonlar�n� olu�tur

    // Mouse hareketlerini izlemek i�in metin kutusu
    MouseLogBox = ICG_MLEditSunken(10, 500, 600, 80, "", SCROLLBAR_V); // 600x80 boyutunda metin kutusu

    // Clear butonu 
    ICG_Button(900, 60, 80, 30, "Clear", ClearCanvas);

}
