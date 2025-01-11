#include "icb_gui.h" 
#include "ic_media.h" 
#include "icbytes.h"
#include <windows.h>
#include <stdio.h>
#include <commdlg.h> // Save file dialog için
#include <iostream> 


// Global deðiþkenler
ICBYTES m;               // Çizim alaný
int FRM1;                 // Çizim alaný paneli
bool isDrawing = false;   // Çizim durumunu takip etme
int prevX = -1, prevY = -1; // Önceki fare konumu

int SLE1;               // Kademe kutucuðunun baðlý olduðu metin kutusu
int RTrackBar; // Renk kademeleri için track barlar
int selectedColor = 0x000000;        // Seçilen renk (siyah)
int ColorPreviewFrame; // Renk önizleme çerçevesi
ICBYTES ColorPreview;  // Renk önizleme alaný için bir matris


// Çizim Alanýný Temizle
void ClearCanvas() {
    // Canvas'ý beyaz ile temizle
    FillRect(m, 0, 0, 800, 600, 0xFFFFFF);
    DisplayMatrix(FRM1, m); 
}

// GUI Baþlatma Fonksiyonu
void ICGUI_Create() {
    ICG_MWSize(1000, 600);  
    ICG_MWTitle("Paint Uygulamasi - ICBYTES");  
}

// Çizgi Çizme Fonksiyonu
void DrawLine(ICBYTES& canvas, int x1, int y1, int x2, int y2, int color) {
    int dx = abs(x2 - x1), dy = abs(y2 - y1);
    int sx = (x1 < x2) ? 1 : -1;
    int sy = (y1 < y2) ? 1 : -1;
    int err = dx - dy;
    
    while (true) {
        if (x1 >= 0 && x1 < canvas.X() && y1 >= 0 && y1 < canvas.Y()) {
            canvas.B(x1, y1, 0) = (color & 0xFF);         // Mavi
            canvas.B(x1, y1, 1) = ((color >> 8) & 0xFF);  // Yeþil
            canvas.B(x1, y1, 2) = ((color >> 16) & 0xFF); // Kýrmýzý
        }
        if (x1 == x2 && y1 == y2) break;
        int e2 = 2 * err;
        if (e2 > -dy) { err -= dy; x1 += sx; }
        if (e2 < dx) { err += dx; y1 += sy; }
    }
}

// Kademe Kutucuðu :Renk Bileþeni Güncelleme
void UpdateColor(int kademe) {
    int selectedColor = (kademe << 16) | (kademe << 8) | kademe;

    // Güncellenen rengi göster
    ICG_SetWindowText(SLE1, "");
    ICG_printf(SLE1, "%d", kademe);
    ColorPreview = 0xff00 + kademe * 2.55;
    DisplayImage(ColorPreviewFrame, ColorPreview);

    
}

void InitializeColorPreview() {
    CreateImage(ColorPreview, 50, 50, ICB_UINT);
    SLE1 = ICG_SLEditBorder(10, 10, 200, 25, "0");
    RTrackBar = ICG_TrackBarH(10, 50, 200, 30, UpdateColor);
    // Çerçeve oluþtur
    ColorPreviewFrame = ICG_FrameMedium(220, 10, 70, 70);
    ColorPreview = 0xff00;
    DisplayImage(ColorPreviewFrame, ColorPreview);
}


// Sol Fare Tuþuna Basýldýðýnda
void OnMouseLDown() {
    isDrawing = true;
    prevX = ICG_GetMouseX();
    prevY = ICG_GetMouseY();
}

// Fare Hareket Ettiðinde
void OnMouseMove(int x, int y) {
    if (isDrawing && prevX >= 0 && prevY >= 0) {
        int currentX = ICG_GetMouseX();
        int currentY = ICG_GetMouseY();
        DrawLine(m, prevX, prevY, currentX, currentY, selectedColor); // Seçilen renkle çiz
        prevX = currentX;
        prevY = currentY;
        DisplayMatrix(FRM1, m); // deðiþiklik olduðunda güncelle
    }
}

// Sol Fare Tuþunu Býraktýðýnýzda
void OnMouseLUp() {
    isDrawing = false;
    prevX = -1;
    prevY = -1;
}

// Fare Olaylarýný Ayarlama
void SetupMouseHandlers() {
    ICG_SetOnMouseLDown(OnMouseLDown);
    ICG_SetOnMouseMove(OnMouseMove);
    ICG_SetOnMouseLUp(OnMouseLUp);
}

// Canvas Baþlat
void InitializeCanvas() {
    FRM1 = ICG_FrameMedium(50, 100, 800, 400); // Çizim alaný çerçevesi
    CreateMatrix(m, 800, 400, 3, ICB_UCHAR);  // Matris oluþtur
    FillRect(m, 0, 0, 800, 400, 0xFFFFFF);    // Beyaz arka plan
    DisplayMatrix(FRM1, m);                   // Çerçeveyi güncelle
}

void NewFunc(){
    ICG_DestroyWidget(FRM1);

    // Yeni çerçeve oluþtur (800x600 boyutunda)
    FRM1 = ICG_FrameMedium(0, 0, 800, 600);

    // Canvas'ý beyaza sýfýrla
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
    bi.biHeight = -bmp.bmHeight; // Pozitif deðer ters görüntü kaydeder
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
    OPENFILENAME ofn;       // Dosya seçme iletiþim kutusu
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
                MessageBox(ICG_GetMainWindow(), "Resim baþarýyla kaydedildi.", "Bilgi", MB_OK);
            }
            else {
                MessageBox(ICG_GetMainWindow(), "Resim kaydedilemedi.", "Hata", MB_OK);
            }
            DeleteObject(bitmap);
        }
        else {
            MessageBox(ICG_GetMainWindow(), "Bitmap oluþturulamadý.", "Hata", MB_OK);
        }
    }
}

void ExitFunc(){

    PostQuitMessage(0);  // Programdan güvenli bir þekilde çýkýþ yapar
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
    static WINDOWPLACEMENT wpPrev = { sizeof(wpPrev) }; // Önceki pencere boyutu
    HWND hwnd = GetActiveWindow(); // Aktif pencereyi al

    if (!isFullScreen) {
        // Tam ekran moduna geç
        MONITORINFO mi = { sizeof(mi) };
        if (GetWindowPlacement(hwnd, &wpPrev) && GetMonitorInfo(MonitorFromWindow(hwnd, MONITOR_DEFAULTTOPRIMARY), &mi)) {
            // WS_OVERLAPPEDWINDOW stilini koruyarak pencereyi büyüt
            SetWindowPos(hwnd, HWND_TOP, mi.rcMonitor.left, mi.rcMonitor.top,
                mi.rcMonitor.right - mi.rcMonitor.left,
                mi.rcMonitor.bottom - mi.rcMonitor.top,
                SWP_NOZORDER | SWP_FRAMECHANGED);
            isFullScreen = true;
        }
    }
    else {
        // Önceki pencere boyutuna dön
        SetWindowPlacement(hwnd, &wpPrev);
        isFullScreen = false;
    }
}

// Ana GUI Fonksiyonu
void ICGUI_main() {
    ICGUI_Create();  // GUI baþlat

    HMENU AnaMenu, DosyaMenu, DuzenleMenu, GorunumMenu;
    AnaMenu = CreateMenu();
    DosyaMenu = CreatePopupMenu();
    DuzenleMenu = CreatePopupMenu();
    GorunumMenu = CreatePopupMenu();

    ICG_AppendMenuItem(DosyaMenu, "Yeni", NewFunc);
    ICG_AppendMenuItem(DosyaMenu, "Aç", OpenFunc);
    ICG_AppendMenuItem(DosyaMenu, "Kaydet", SaveFunc);
    //ICG_AppendMenuItem(DosyaMenu, "Paylaþ", ShareFunc);
    ICG_AppendMenuItem(DosyaMenu, "Çýkýþ", ExitFunc);

    ICG_AppendMenuItem(DuzenleMenu, "Kes", CutFunc);
    ICG_AppendMenuItem(DuzenleMenu, "Kopyala", CopyFunc);
    ICG_AppendMenuItem(DuzenleMenu, "Yapýþtýr", PasteFunc);

    ICG_AppendMenuItem(GorunumMenu, "Yakýnlaþtýrma", PasteFunc);
    ICG_AppendMenuItem(GorunumMenu, "Cetveller", PasteFunc);
    ICG_AppendMenuItem(GorunumMenu, "Kýlavuz Çizgileri", PasteFunc);
    ICG_AppendMenuItem(GorunumMenu, "Durum Çubuðu", PasteFunc);
    ICG_AppendMenuItem(GorunumMenu, "Tam Ekran", ToggleFullScreen);
    //ICG_AppendMenuItem(DuzenleMenu, "Küçük Resim", PasteFunc);

    AppendMenu(AnaMenu, MF_POPUP, (UINT_PTR)DosyaMenu, "Dosya");
    AppendMenu(AnaMenu, MF_POPUP, (UINT_PTR)DuzenleMenu, "Düzenle");
    AppendMenu(AnaMenu, MF_POPUP, (UINT_PTR)GorunumMenu, "Görünüm");

    ICG_SetMenu(AnaMenu);

    //Bitmap Buton Ekleme 
    int SaveButton;
    static ICBYTES saveIcon;
    ReadImage("save.bmp", saveIcon);
    SaveButton = ICG_BitmapButton(900, 10, 40, 40, SaveFunc);
    SetButtonBitmap(SaveButton, saveIcon);

    int BTN1;
    static ICBYTES butonresmi;
    CreateImage(butonresmi, 50, 50, ICB_UINT);
    butonresmi = 0xffffff;
    BTN1 = ICG_BitmapButton(300, 10, 40, 40, Elipsee);
    Ellipse(butonresmi, 9, 8, 17, 17, 0x0000000);
    Ellipse(butonresmi, 10, 9, 16, 16, 0x0000000);
    Ellipse(butonresmi, 11, 10, 15, 15, 0x0000000);
    for (int y = 1;y <= butonresmi.Y();y++)
        for (int x = 1; x <= butonresmi.X(); x++)
        {
            butonresmi.U(x, y) -= 0x030000 * y;
        }
    SetButtonBitmap(BTN1, butonresmi);

    int BTN2;
    static ICBYTES butonresmi1;
    CreateImage(butonresmi1, 50, 50, ICB_UINT);
    butonresmi1 = 0xffffff;
    BTN2 = ICG_BitmapButton(350, 10, 40, 40, Rectt);
    FillRect(butonresmi1, 10, 9, 30, 30, 0x0000000);
    for (int y = 1;y <= butonresmi1.Y();y++)
        for (int x = 1; x <= butonresmi1.X(); x++)
        {
            butonresmi1.U(x, y) -= 0x030000 * y;
        }
    SetButtonBitmap(BTN2, butonresmi1);

    int BTN3;
    static ICBYTES butonresmi2;
    CreateImage(butonresmi2, 50, 50, ICB_UINT);
    butonresmi2 = 0xffffff;
    BTN3 = ICG_BitmapButton(400, 10, 40, 40, Rectfil);
    Rect(butonresmi2, 8, 9, 30, 30, 0x0000000);
    Rect(butonresmi2, 9, 10, 30, 30, 0x0000000);
    Rect(butonresmi2, 10, 11, 30, 30, 0x0000000);
    for (int y = 1;y <= butonresmi2.Y();y++)
        for (int x = 1; x <= butonresmi2.X(); x++)
        {
            butonresmi2.U(x, y) -= 0x030000 * y;
        }
    SetButtonBitmap(BTN3, butonresmi2);

    int BTN4;
    static ICBYTES butonresmi3;
    CreateImage(butonresmi3, 50, 50, ICB_UINT);
    butonresmi3 = 0xffffff;
    BTN4 = ICG_BitmapButton(450, 10, 40, 40, Linee);
    Line(butonresmi3, 10, 11, 40, 40, 0x0000000);
    Line(butonresmi3, 11, 12, 40, 40, 0x0000000);
    Line(butonresmi3, 12, 13, 40, 40, 0x0000000);
    for (int y = 1;y <= butonresmi3.Y();y++)
        for (int x = 1; x <= butonresmi3.X(); x++)
        {
            butonresmi3.U(x, y) -= 0x030000 * y;
        }
    SetButtonBitmap(BTN4, butonresmi3);

    int BTN5;
    static ICBYTES butonresmi4;
    CreateImage(butonresmi4, 50, 50, ICB_UINT);
    butonresmi4 = 0xffffff;
    BTN5 = ICG_BitmapButton(500, 10, 40, 40, Linee);
    MarkPlus(butonresmi4, 25, 25, 10, 0);
    for (int y = 1;y <= butonresmi4.Y();y++)
        for (int x = 1; x <= butonresmi4.X(); x++)
        {
            butonresmi4.U(x, y) -= 0x030000 * y;
        }
    SetButtonBitmap(BTN5, butonresmi4);

    InitializeCanvas();    // Çizim alanýný baþlat
    InitializeColorPreview(); // Renk Paletini Ekle
    SetupMouseHandlers(); // Fare olaylarýný ayarla

    // Clear butonu 
    ICG_Button(810, 20, 80, 30, "Clear", ClearCanvas);

}
