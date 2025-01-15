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

int SLE1;               // Kademe kutucu�unun ba�l� oldu�u metin kutusu
int RTrackBar; // Renk kademeleri i�in track barlar
int selectedColor = 0xFF0000; // Siyah
int ColorPreviewFrame; // Renk �nizleme �er�evesi
ICBYTES ColorPreview;  // Renk �nizleme alan� i�in bir matris

int MouseLogBox; // Fare hareketlerini g�stermek i�in metin kutusu

int frameOffsetX = 50; // �er�eve X ba�lang�� konumu
int frameOffsetY = 100; // �er�eve Y ba�lang�� konumu

// �izim Alan�n� Temizle
void ClearCanvas() {
    // Canvas'� beyaz ile temizle
    FillRect(m, 0, 0, 800, 600, 0xFFFFFF);
    DisplayImage(FRM1, m); 
}

// GUI Ba�latma Fonksiyonu
void ICGUI_Create() {
    ICG_MWSize(1000, 600);  
    ICG_MWTitle("Paint Uygulamasi - ICBYTES");  
}

// �izgi �izme Fonksiyonu
void DrawLine(ICBYTES& canvas, int x1, int y1, int x2, int y2, int color) {
    int dx = abs(x2 - x1);
    int dy = abs(y2 - y1);
    int sx = (x1 < x2) ? 1 : -1;
    int sy = (y1 < y2) ? 1 : -1;
    int err = dx - dy;

    while (true) {
        if (x1 >= 0 && x1 < canvas.X() && y1 >= 0 && y1 < canvas.Y()) {
            canvas.B(x1, y1, 0) = color & 0xFF;        // Mavi
            canvas.B(x1, y1, 1) = (color >> 8) & 0xFF; // Ye�il
            canvas.B(x1, y1, 2) = (color >> 16) & 0xFF; // K�rm�z�

        }
        else {
            ICG_printf(MouseLogBox, "Pixel out of bounds - X: %d, Y: %d\n", x1, y1);
        }


        // �izim tamamland�ysa d�ng�y� k�r
        if (x1 == x2 && y1 == y2) break;

        int e2 = 2 * err;
        if (e2 > -dy) {
            err -= dy;
            x1 += sx;
        }
        if (e2 < dx) {
            err += dx;
            y1 += sy;
        }
        ICG_printf(MouseLogBox, "Drawing Pixel - X: %d, Y: %d, Color: 0x%X\n", x1, y1, color);


    }
}


// Kademe Kutucu�u :Renk Bile�eni G�ncelleme
void UpdateColor(int kademe) {
    selectedColor = (kademe << 16) | (kademe << 8) | kademe;

    // G�ncellenen rengi g�ster
    ICG_SetWindowText(SLE1, "");
    ICG_printf(SLE1, "Selected Color: 0x%X\n", selectedColor);
    FillRect(ColorPreview, 0, 0, 50, 50, selectedColor);
    DisplayImage(ColorPreviewFrame, ColorPreview);

    
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



// Sol Fare Tu�una Bas�ld���nda
void OnMouseLDown() {
    isDrawing = true;
    prevX = ICG_GetMouseX() - frameOffsetX;
    prevY = ICG_GetMouseY() - frameOffsetY;

    LogMouseAction("Mouse Down", prevX, prevY);
}


// Fare Hareket Etti�inde
void OnMouseMove(int x, int y) {
    if (isDrawing) {
        // Gelen x ve y de�erlerini kullanarak pozisyonu normalize edin
        int currentX = ICG_GetMouseX() - frameOffsetX;
        int currentY = ICG_GetMouseY() - frameOffsetY;


        // Matris s�n�rlar�n� kontrol edin
        if (currentX >= 0 && currentX < m.X() && currentY >= 0 && currentY < m.Y()) {
            if (prevX >= 0 && prevY >= 0) {
                // �izgi �izin
                DrawLine(m, prevX, prevY, currentX, currentY, selectedColor);
            }

            // �nceki pozisyonu g�ncelleyin
            prevX = currentX;
            prevY = currentY;

           
        }
        ICG_printf(MouseLogBox, "MouseMove - X: %d, Y: %d, FrameX: %d, FrameY: %d\n",
            x, y, frameOffsetX, frameOffsetY);

    }
}


// Sol Fare Tu�unu B�rakt���n�zda
void OnMouseLUp() {
    isDrawing = false;
    prevX = -1;
    prevY = -1;
    LogMouseAction("Mouse Up", prevX, prevY);

    // Matrisi ekrana yans�t�n
    DisplayImage(FRM1, m);
    ICG_printf(MouseLogBox, "Matrix size: %d x %d, Frame ID: %d\n", m.X(), m.Y(), FRM1);

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
    FRM1 = ICG_FrameMedium(0, 0, 800, 600);

    // Canvas'� beyaza s�f�rla
    m = FRM1;
    FillRect(m, 0, 0, 800, 600, 0xFFFFFF);
    DisplayMatrix(FRM1, m);
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

    PostQuitMessage(0);  // Programdan g�venli bir �ekilde ��k�� yapar
}
void CutFunc(){}
void CopyFunc(){}
void PasteFunc(){}
void Elipsee(){}
void Rectt(){}
void Rectfil(){}
void Linee(){}

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
    BTN1 = ICG_BitmapButton(300, 10, 40, 40, Elipsee);
    Ellipse(butonresmi, 9, 8, 17, 17, 0x0000000);
    Ellipse(butonresmi, 10, 9, 16, 16, 0x0000000);
    Ellipse(butonresmi, 11, 10, 15, 15, 0x0000000);
    for (int y = 1; y <= butonresmi.Y(); y++) {
        for (int x = 1; x <= butonresmi.X(); x++) {
            butonresmi.U(x, y) -= 0x030000 * y;
        }
    }
    SetButtonBitmap(BTN1, butonresmi);

    // BTN2: Dolu Dikd�rtgen
    CreateImage(butonresmi1, 50, 50, ICB_UINT);
    butonresmi1 = 0xffffff;
    BTN2 = ICG_BitmapButton(350, 10, 40, 40, Rectt);
    FillRect(butonresmi1, 10, 9, 30, 30, 0x0000000);
    for (int y = 1; y <= butonresmi1.Y(); y++) {
        for (int x = 1; x <= butonresmi1.X(); x++) {
            butonresmi1.U(x, y) -= 0x030000 * y;
        }
    }
    SetButtonBitmap(BTN2, butonresmi1);

    // BTN3: �er�eve Dikd�rtgen
    CreateImage(butonresmi2, 50, 50, ICB_UINT);
    butonresmi2 = 0xffffff;
    BTN3 = ICG_BitmapButton(400, 10, 40, 40, Rectfil);
    Rect(butonresmi2, 8, 9, 30, 30, 0x0000000);
    Rect(butonresmi2, 9, 10, 30, 30, 0x0000000);
    Rect(butonresmi2, 10, 11, 30, 30, 0x0000000);
    for (int y = 1; y <= butonresmi2.Y(); y++) {
        for (int x = 1; x <= butonresmi2.X(); x++) {
            butonresmi2.U(x, y) -= 0x030000 * y;
        }
    }
    SetButtonBitmap(BTN3, butonresmi2);

    // BTN4: �izgi
    CreateImage(butonresmi3, 50, 50, ICB_UINT);
    butonresmi3 = 0xffffff;
    BTN4 = ICG_BitmapButton(450, 10, 40, 40, Linee);
    Line(butonresmi3, 10, 11, 40, 40, 0x0000000);
    Line(butonresmi3, 11, 12, 40, 40, 0x0000000);
    Line(butonresmi3, 12, 13, 40, 40, 0x0000000);
    for (int y = 1; y <= butonresmi3.Y(); y++) {
        for (int x = 1; x <= butonresmi3.X(); x++) {
            butonresmi3.U(x, y) -= 0x030000 * y;
        }
    }
    SetButtonBitmap(BTN4, butonresmi3);

    // BTN5: Art� ��areti
    CreateImage(butonresmi4, 50, 50, ICB_UINT);
    butonresmi4 = 0xffffff;
    BTN5 = ICG_BitmapButton(500, 10, 40, 40, Linee);
    MarkPlus(butonresmi4, 25, 25, 10, 0);
    for (int y = 1; y <= butonresmi4.Y(); y++) {
        for (int x = 1; x <= butonresmi4.X(); x++) {
            butonresmi4.U(x, y) -= 0x030000 * y;
        }
    }
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
    SetupMouseHandlers(); // Fare olaylar�n� ayarla
    CreateDrawingButtons(); // �izim butonlar�n� olu�tur

    // Mouse hareketlerini izlemek i�in metin kutusu
    MouseLogBox = ICG_MLEditSunken(10, 500, 600, 80, "", SCROLLBAR_V); // 600x80 boyutunda metin kutusu

    // Clear butonu 
    ICG_Button(810, 20, 80, 30, "Clear", ClearCanvas);

}
