#include "icb_gui.h" 
#include "ic_media.h" 
#include "icbytes.h"
#include <windows.h>
#include <stdio.h>
#include <commdlg.h> // Save file dialog için
#include <iostream> 
#include <mutex> // Mutex için

// Global değişkenler
ICBYTES m;               // Çizim alanı
int FRM1;                 // Çizim alanı paneli
bool isDrawing = false;   // Çizim durumunu takip etme
int prevX = -1, prevY = -1; // Önceki fare konumu

HANDLE hThread;      // Çizim iş parçacığı
DWORD threadID;      // İş parçacığı kimliği
bool stopDrawing = false; // İş parçacığının durumu

std::mutex drawMutex;  // Çizim işlemi için mutex
std::mutex threadMutex; // **Tüm thread işlemlerini koruyan mutex**

int SLE1;               // Kademe kutucuğunun bağlı olduğu metin kutusu
int RTrackBar; // Renk kademeleri için track barlar
int selectedColor = 0x000000; // Siyah
int ColorPreviewFrame; // Renk önizleme çerçevesi
ICBYTES ColorPreview;  // Renk önizleme alanı için bir matris
ICBYTES saveIcon;
int MouseLogBox; // Fare hareketlerini göstermek için metin kutusu

int frameOffsetX = 50; // Çerçeve X başlangıç konumu
int frameOffsetY = 100; // Çerçeve Y başlangıç konumu

int lineThickness = 1; // Varsayılan olarak 1 piksel kalınlık

int startX = -1, startY = -1; // Fare sürükleme başlangıç pozisyonu
int currentX = -1, currentY = -1; // Fare sürükleme geçerli pozisyonu

enum Mode { NORMAL, ELLIPSE, RECTANGLE, FILLED_RECTANGLE, LINE, CIRCLE, FILLED_CIRCLE, TRIANGLE, PLUS_MARK, ERASER };

Mode activeMode = NORMAL; // Varsayılan mod NORMAL

#define MAX_THREADS 16  // Maksimum 16 thread çalıştır
HANDLE threadPool[MAX_THREADS] = { 0 };  // Thread havuzu
bool threadActive[MAX_THREADS] = { false };  // Hangi thread'lerin çalıştığını takip et

struct DrawParams {
    ICBYTES* canvas;
    Mode mode;
    int startX, startY;
    int endX, endY;
    int color;
};

ICBYTES tempCanvas;
ICBYTES backBuffer;

void FreeThreadSlot(HANDLE hThread) {
    for (int i = 0; i < MAX_THREADS; i++) {
        if (threadPool[i] == hThread) {
            WaitForSingleObject(threadPool[i], INFINITE);  // **Thread'in tamamlanmasını bekle**
            CloseHandle(threadPool[i]);  // **Thread'i kapat**
            threadPool[i] = NULL;
            threadActive[i] = false;
            break;
        }
    }
}

// Çizim Alanını Temizle
void ClearCanvas() {
    // Ana canvas ve tamponları tamamen beyaz yap
    FillRect(m, 0, 0, m.X(), m.Y(), 0xFFFFFF);
    FillRect(backBuffer, 0, 0, backBuffer.X(), backBuffer.Y(), 0xFFFFFF);
    FillRect(tempCanvas, 0, 0, tempCanvas.X(), tempCanvas.Y(), 0xFFFFFF);

    // **Tüm pikselleri beyaz yap ve alpha değerini sıfırla**
    for (int y = 0; y < m.Y(); y++) {
        for (int x = 0; x < m.X(); x++) {
            // **RGB formatında beyaz ayarla**
            m.B(x, y, 0) = 0xFF;  // Mavi
            m.B(x, y, 1) = 0xFF;  // Yeşil
            m.B(x, y, 2) = 0xFF;  // Kırmızı
            m.B(x, y, 3) = 0xFF;  // Alpha (tam opak)

            // **Geçici canvas'ı şeffaf yap**
            tempCanvas.B(x, y, 3) = 0x00;  // Tamamen şeffaf
        }
    }

    // **Ekranı güncelle**
    DisplayImage(FRM1, m);
    ICG_printf(MouseLogBox, "ClearCanvas(): Tuval sıfırlandı ve beyaza ayarlandı.\n");
}



// GUI Başlatma Fonksiyonu
void ICGUI_Create() {
    ICG_MWSize(1000, 600);
    ICG_MWTitle("Paint Uygulaması - ICBYTES");
}

void UpdateLineThickness(int value) {
    lineThickness = value; // Kalınlık değerini güncelle
    ICG_printf(MouseLogBox, "Line thickness updated: %d\n", lineThickness);
}

void CreateThicknessTrackbar() {
    ICG_Static(705, 0, 200, 15, "Line Thickness");
    ICG_TrackBarH(700, 15, 200, 30, UpdateLineThickness); // Trackbar pozisyonu ve uzunluğu
}

void EraserMode() {
    activeMode = ERASER;  // Silgi modunu etkinleştir
    ICG_printf(MouseLogBox, "Eraser mode activated.\n");
}

// Çizgi Çizme Fonksiyonu
void DrawLine(ICBYTES& canvas, int x1, int y1, int x2, int y2, int color) {
    int dx = abs(x2 - x1);
    int dy = abs(y2 - y1);
    int sx = (x1 < x2) ? 1 : -1;
    int sy = (y1 < y2) ? 1 : -1;
    int err = dx - dy;

    float length = sqrt(dx * dx + dy * dy);
    float normX = (length == 0) ? 0 : -dy / length;
    float normY = (length == 0) ? 0 : dx / length;

    int halfThickness = max(1, (lineThickness - 1) / 2);

    while (true) {
        // **İlk önce sadece ana çizgiyi çizer**
        Line(canvas, x1, y1, x2, y2, color);

        // **Daha sonra genişlik ekler (ilk noktada direkt kalınlaşmayı önler)**
        if (lineThickness > 1) {
            for (float i = 1; i <= halfThickness; i += 0.5) {
                int offsetX = round(i * normX);
                int offsetY = round(i * normY);
                Line(canvas, x1 + offsetX, y1 + offsetY, x2 + offsetX, y2 + offsetY, color);
                Line(canvas, x1 - offsetX, y1 - offsetY, x2 - offsetX, y2 - offsetY, color);
            }
        }

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
    }
}



void DrawTriangle(ICBYTES& canvas, int x1, int y1, int x2, int y2, int x3, int y3, int color) {
    DrawLine(canvas, x1, y1, x2, y2, color);
    DrawLine(canvas, x2, y2, x3, y3, color);
    DrawLine(canvas, x3, y3, x1, y1, color);
}

void DrawRectangle(ICBYTES& canvas, int x, int y, int width, int height, int color) {
    DrawLine(canvas, x, y, x + width, y, color);
    DrawLine(canvas, x, y, x, y + height, color);
    DrawLine(canvas, x + width, y, x + width, y + height, color);
    DrawLine(canvas, x, y + height, x + width, y + height, color);
}

void DrawFilledRectangle(ICBYTES& canvas, int x, int y, int width, int height, int color) {
    for (int i = 0; i < width; i++) {
        DrawLine(canvas, x + i, y, x + i, y + height, color);
    }
}

void DrawCircle(ICBYTES& canvas, int cx, int cy, int radius, int color) {
    int x = radius, y = 0;
    int decisionOver2 = 1 - x;

    while (y <= x) {
        // **Küçük çizgiler ile çember konturunu oluştur**
        DrawLine(canvas, cx + x, cy + y, cx + x + 2, cy + y, color);
        DrawLine(canvas, cx - x, cy + y, cx - x - 2, cy + y, color);
        DrawLine(canvas, cx + x, cy - y, cx + x + 2, cy - y, color);
        DrawLine(canvas, cx - x, cy - y, cx - x - 2, cy - y, color);

        DrawLine(canvas, cx + y, cy + x, cx + y + 2, cy + x, color);
        DrawLine(canvas, cx - y, cy + x, cx - y - 2, cy + x, color);
        DrawLine(canvas, cx + y, cy - x, cx + y + 2, cy - x, color);
        DrawLine(canvas, cx - y, cy - x, cx - y - 2, cy - x, color);

        y++;
        if (decisionOver2 <= 0) {
            decisionOver2 +=  y + 1;
        }
        else {
            x--;
            decisionOver2 += (y - x) + 1;
        }
    }
}

void DrawEllipse(ICBYTES& canvas, int cx, int cy, int rx, int ry, int color) {
    int x = 0, y = ry;
    long long rxSq = (long long)rx * rx;
    long long rySq = (long long)ry * ry;
    long long twoRxSq = 2 * rxSq;
    long long twoRySq = 2 * rySq;
    long long p;
    long long px = 0, py = twoRxSq * y;

    // **Bölge 1 (Hafif yatay kısım)**
    p = rySq - (rxSq * ry) + (0.25 * rxSq);
    while (px < py) {
        // **4 bölgeye simetrik çizim yapıyoruz**
        if (cx + x >= 0 && cx + x < canvas.X() && cy + y >= 0 && cy + y < canvas.Y()) {
            canvas.B(cx + x, cy + y, 0) = color & 0xFF;
            canvas.B(cx + x, cy + y, 1) = (color >> 8) & 0xFF;
            canvas.B(cx + x, cy + y, 2) = (color >> 16) & 0xFF;
        }
        if (cx - x >= 0 && cx - x < canvas.X() && cy + y >= 0 && cy + y < canvas.Y()) {
            canvas.B(cx - x, cy + y, 0) = color & 0xFF;
            canvas.B(cx - x, cy + y, 1) = (color >> 8) & 0xFF;
            canvas.B(cx - x, cy + y, 2) = (color >> 16) & 0xFF;
        }
        if (cx + x >= 0 && cx + x < canvas.X() && cy - y >= 0 && cy - y < canvas.Y()) {
            canvas.B(cx + x, cy - y, 0) = color & 0xFF;
            canvas.B(cx + x, cy - y, 1) = (color >> 8) & 0xFF;
            canvas.B(cx + x, cy - y, 2) = (color >> 16) & 0xFF;
        }
        if (cx - x >= 0 && cx - x < canvas.X() && cy - y >= 0 && cy - y < canvas.Y()) {
            canvas.B(cx - x, cy - y, 0) = color & 0xFF;
            canvas.B(cx - x, cy - y, 1) = (color >> 8) & 0xFF;
            canvas.B(cx - x, cy - y, 2) = (color >> 16) & 0xFF;
        }

        x++;
        px += twoRySq;
        if (p < 0) {
            p += rySq + px;
        }
        else {
            y--;
            py -= twoRxSq;
            p += rySq + px - py;
        }
    }

    // **Bölge 2 (Daha dik kısım)**
    p = rySq * (x + 0.5) * (x + 0.5) + rxSq * (y - 1) * (y - 1) - rxSq * rySq;
    while (y >= 0) {
        if (cx + x >= 0 && cx + x < canvas.X() && cy + y >= 0 && cy + y < canvas.Y()) {
            canvas.B(cx + x, cy + y, 0) = color & 0xFF;
            canvas.B(cx + x, cy + y, 1) = (color >> 8) & 0xFF;
            canvas.B(cx + x, cy + y, 2) = (color >> 16) & 0xFF;
        }
        if (cx - x >= 0 && cx - x < canvas.X() && cy + y >= 0 && cy + y < canvas.Y()) {
            canvas.B(cx - x, cy + y, 0) = color & 0xFF;
            canvas.B(cx - x, cy + y, 1) = (color >> 8) & 0xFF;
            canvas.B(cx - x, cy + y, 2) = (color >> 16) & 0xFF;
        }
        if (cx + x >= 0 && cx + x < canvas.X() && cy - y >= 0 && cy - y < canvas.Y()) {
            canvas.B(cx + x, cy - y, 0) = color & 0xFF;
            canvas.B(cx + x, cy - y, 1) = (color >> 8) & 0xFF;
            canvas.B(cx + x, cy - y, 2) = (color >> 16) & 0xFF;
        }
        if (cx - x >= 0 && cx - x < canvas.X() && cy - y >= 0 && cy - y < canvas.Y()) {
            canvas.B(cx - x, cy - y, 0) = color & 0xFF;
            canvas.B(cx - x, cy - y, 1) = (color >> 8) & 0xFF;
            canvas.B(cx - x, cy - y, 2) = (color >> 16) & 0xFF;
        }

        y--;
        py -= twoRxSq;
        if (p > 0) {
            p += rxSq - py;
        }
        else {
            x++;
            px += twoRySq;
            p += rxSq - py + px;
        }
    }
}

void DrawFilledCircle(ICBYTES& canvas, int cx, int cy, int radius, int color) {
    for (int y = -radius; y <= radius; y++) {
        int dx = sqrt(radius * radius - y * y);
        DrawLine(canvas, cx - dx, cy + y, cx + dx, cy + y, color);
    }
}

void DrawPlusMark(ICBYTES& canvas, int xc, int yc, int size, int color) {
    DrawLine(canvas, xc - size, yc, xc + size, yc, color);
    DrawLine(canvas, xc, yc - size, xc, yc + size, color);
}

void FillTriangle(ICBYTES& canvas, int x1, int y1, int x2, int y2, int x3, int y3, int color) {
    auto Swap = [](int& a, int& b) { int temp = a; a = b; b = temp; };

    // **Y'ye göre sıralama (Küçükten büyüğe)**
    if (y1 > y2) { Swap(x1, x2); Swap(y1, y2); }
    if (y1 > y3) { Swap(x1, x3); Swap(y1, y3); }
    if (y2 > y3) { Swap(x2, x3); Swap(y2, y3); }

    // Üçgenin yüksekliği
    int totalHeight = y3 - y1;

    // Alt yarıyı doldur
    for (int i = 0; i <= y2 - y1; i++) {
        int segmentHeight = y2 - y1 + 1;
        float alpha = (float)i / totalHeight;
        float beta = (float)i / segmentHeight;

        int startX = x1 + (x3 - x1) * alpha;
        int endX = x1 + (x2 - x1) * beta;

        if (startX > endX) Swap(startX, endX);

        // Yatay çizgi çiz
        for (int x = startX; x <= endX; x++) {
            if (x >= 0 && x < canvas.X() && (y1 + i) >= 0 && (y1 + i) < canvas.Y()) {
                canvas.B(x, y1 + i, 0) = color & 0xFF;
                canvas.B(x, y1 + i, 1) = (color >> 8) & 0xFF;
                canvas.B(x, y1 + i, 2) = (color >> 16) & 0xFF;
            }
        }
    }

    // Üst yarıyı doldur
    for (int i = 0; i <= y3 - y2; i++) {
        int segmentHeight = y3 - y2 + 1;
        float alpha = (float)(y2 - y1 + i) / totalHeight;
        float beta = (float)i / segmentHeight;

        int startX = x1 + (x3 - x1) * alpha;
        int endX = x2 + (x3 - x2) * beta;

        if (startX > endX) Swap(startX, endX);

        // Yatay çizgi çiz
        for (int x = startX; x <= endX; x++) {
            if (x >= 0 && x < canvas.X() && (y2 + i) >= 0 && (y2 + i) < canvas.Y()) {
                canvas.B(x, y2 + i, 0) = color & 0xFF;
                canvas.B(x, y2 + i, 1) = (color >> 8) & 0xFF;
                canvas.B(x, y2 + i, 2) = (color >> 16) & 0xFF;
            }
        }
    }
}

// Kademe Kutucuğu :Renk Bileşeni Güncelleme
void UpdateColor(int kademe) {
    // Örnek olarak basit bir RGB renk paleti
    int red = kademe * 10 % 256; // Renk tonlarını farklı yap
    int green = (kademe * 20) % 256;
    int blue = (kademe * 30) % 256;

    selectedColor = (red << 16) | (green << 8) | blue;

    // Güncellenen rengi göster
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

void SelectColor1() { selectedColor = 0x000000; UpdatePreview(); } // Siyah
void SelectColor2() { selectedColor = 0x6d6d6d; UpdatePreview(); } // Gri
void SelectColor3() { selectedColor = 0xFF0000; UpdatePreview(); } // Kırmızı
void SelectColor4() { selectedColor = 0x500000; UpdatePreview(); } // Koyu Kırmızı
void SelectColor5() { selectedColor = 0xfc4e03; UpdatePreview(); } // Turuncu
void SelectColor6() { selectedColor = 0xfce803; UpdatePreview(); } // Sarı
void SelectColor7() { selectedColor = 0x00FF00; UpdatePreview(); } // Yeşil
void SelectColor8() { selectedColor = 0x00c4e2; UpdatePreview(); } // Açık Mavi
void SelectColor9() { selectedColor = 0x0919ca; UpdatePreview(); } // Mavi
void SelectColor10() { selectedColor = 0x8209c0; UpdatePreview(); } // Mor

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
    // Alt satır 
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
    // Çerçeve oluştur
    ColorPreviewFrame = ICG_FrameMedium(220, 10, 70, 70);
    ColorPreview = 0x000000;
    DisplayImage(ColorPreviewFrame, ColorPreview);
}


void ManualCopy(ICBYTES& source, ICBYTES& destination) {
    // Boyutları kontrol et
    if (source.X() != destination.X() || source.Y() != destination.Y()) {
        ICG_printf(MouseLogBox, "Copy failed: Dimension mismatch.\n");
        return;
    }

    // Piksel bazında kopyalama işlemi
    for (int y = 0; y < source.Y(); ++y) {
        for (int x = 0; x < source.X(); ++x) {
            destination.B(x, y, 0) = source.B(x, y, 2); // Mavi
            destination.B(x, y, 1) = source.B(x, y, 1); // Yeşil
            destination.B(x, y, 2) = source.B(x, y, 0); // Kırmızı
        }
    }
}

void UpdateCombinedCanvas() {
    // **Arka tamponu beyaz yap**
    FillRect(backBuffer, 0, 0, backBuffer.X(), backBuffer.Y(), 0xFFFFFF);

    // **Ana tuvali birleştirilen tuvale kopyala**
    ManualCopy(m, backBuffer);

    // **Geçici canvas'ı ekle (sadece şeffaf olmayan pikselleri kopyala)**
    for (int y = 0; y < backBuffer.Y(); y++) {
        for (int x = 0; x < backBuffer.X(); x++) {
            if (tempCanvas.B(x, y, 3) != 0x00) {  // **Alpha 0 değilse çiz**
                backBuffer.B(x, y, 0) = tempCanvas.B(x, y, 0);
                backBuffer.B(x, y, 1) = tempCanvas.B(x, y, 1);
                backBuffer.B(x, y, 2) = tempCanvas.B(x, y, 2);
            }
        }
    }

    // **Birleştirilen tuvali ekrana yansıt**
    DisplayImage(FRM1, backBuffer);
}


void ClearTemporaryCanvas() {
    for (int y = 0; y < tempCanvas.Y(); y++) {
        for (int x = 0; x < tempCanvas.X(); x++) {
            tempCanvas.B(x, y, 0) = 0xFF; // **Mavi (Tam Beyaz)**
            tempCanvas.B(x, y, 1) = 0xFF; // **Yeşil (Tam Beyaz)**
            tempCanvas.B(x, y, 2) = 0xFF; // **Kırmızı (Tam Beyaz)**
            tempCanvas.B(x, y, 3) = 0x00; // **Alpha tamamen şeffaf yap**
        }
    }
}


HANDLE* GetAvailableThreadSlot() {
    int attempts = 100;  // En fazla 100 kez dene
    while (attempts--) {
        for (int i = 0; i < MAX_THREADS; i++) {
            if (!threadActive[i] || WaitForSingleObject(threadPool[i], 0) == WAIT_OBJECT_0) {
                if (threadPool[i]) {
                    CloseHandle(threadPool[i]);
                }
                threadActive[i] = true;
                return &threadPool[i];
            }
        }
        Sleep(10);
    }
    return nullptr;  // Eğer thread bulunamazsa NULL döndür
}


// Serbest çizim için thread fonksiyonu
DWORD WINAPI DrawFreehandThread(LPVOID lpParam) {
    DrawParams* params = (DrawParams*)lpParam;

    threadMutex.lock(); // **Kritik bölgeye giriş**
    DrawLine(m, params->startX, params->startY, params->endX, params->endY, params->color);
    DisplayImage(FRM1, m);
    threadMutex.unlock(); // **Kritik bölgeden çıkış**

    delete params;
    return 0;
}

void StartFreehandDrawingThread(ICBYTES& canvas, int x1, int y1, int x2, int y2, int color) {
    HANDLE* hSlot = GetAvailableThreadSlot();
    if (!hSlot) return;

    DrawParams* params = new DrawParams{ &canvas, NORMAL, x1, y1, x2, y2, color };

    *hSlot = CreateThread(NULL, 0, DrawFreehandThread, params, 0, NULL);
    if (*hSlot) {
        SetThreadPriority(*hSlot, THREAD_PRIORITY_LOWEST);
    }
    else {
        delete params;
    }
}

DWORD WINAPI EraseThread(LPVOID lpParam) {
    DrawParams* params = (DrawParams*)lpParam;

    threadMutex.lock(); // **Thread güvenliği sağla**

    // **Silme işlemi (Beyaz renge döndür)**
    for (int i = -lineThickness / 2; i <= lineThickness / 2; ++i) {
        for (int j = -lineThickness / 2; j <= lineThickness / 2; ++j) {
            int px = params->endX + i;
            int py = params->endY + j;
            if (px >= 0 && px < m.X() && py >= 0 && py < m.Y()) {
                m.B(px, py, 0) = 0xFF; // **Mavi (Tam Beyaz)**
                m.B(px, py, 1) = 0xFF; // **Yeşil (Tam Beyaz)**
                m.B(px, py, 2) = 0xFF; // **Kırmızı (Tam Beyaz)**
                m.B(px, py, 3) = 0xFF; // **Alpha 255 (tam görünür)**
            }
        }
    }

    DisplayImage(FRM1, m);
    threadMutex.unlock(); // **Thread’i serbest bırak**

    delete params; // **Bellek sızıntısını önlemek için serbest bırak**
    return 0;
}


void StartEraseThread(ICBYTES& canvas, int x, int y) {
    HANDLE* hSlot = GetAvailableThreadSlot();
    if (!hSlot) return;

    DrawParams* params = new DrawParams{ &canvas, ERASER, 0, 0, x, y, 0xFFFFFF };

    *hSlot = CreateThread(NULL, 0, EraseThread, params, 0, NULL);
    if (*hSlot) {
        SetThreadPriority(*hSlot, THREAD_PRIORITY_LOWEST);
    }
    else {
        delete params;
    }
}

// Thread fonksiyonu - Her şekli farklı bir thread içinde çizecek
DWORD WINAPI DrawThread(LPVOID lpParam) {
    DrawParams* params = (DrawParams*)lpParam;

    threadMutex.lock();

    switch (params->mode) {
    case LINE:
        DrawLine(*params->canvas, params->startX, params->startY, params->endX, params->endY, params->color);
        break;
    case RECTANGLE:
        DrawRectangle(*params->canvas, params->startX, params->startY, params->endX - params->startX, params->endY - params->startY, params->color);
        break;
    case FILLED_RECTANGLE:
        DrawFilledRectangle(*params->canvas, params->startX, params->startY, params->endX - params->startX, params->endY - params->startY, params->color);
        break;
    case CIRCLE:
        DrawCircle(*params->canvas, (params->startX + params->endX) / 2, (params->startY + params->endY) / 2, abs(params->endX - params->startX) / 2, params->color);
        break;
    case FILLED_CIRCLE:
        DrawFilledCircle(*params->canvas, (params->startX + params->endX) / 2, (params->startY + params->endY) / 2, abs(params->endX - params->startX) / 2, params->color);
        break;
    case ELLIPSE:
        DrawEllipse(*params->canvas, (params->startX + params->endX) / 2, (params->startY + params->endY) / 2, abs(params->endX - params->startX) / 2, abs(params->endY - params->startY) / 2, params->color);
        break;
    case TRIANGLE:
        DrawTriangle(*params->canvas, params->startX, params->startY, params->endX, params->startY, (params->startX + params->endX) / 2, params->endY, params->color);
        break;
    case PLUS_MARK:
        DrawPlusMark(*params->canvas, (params->startX + params->endX) / 2, (params->startY + params->endY) / 2, abs(params->endX - params->startX) / 4, params->color);
        break;
    default:
        break;
    }

    threadMutex.unlock();
    delete params;
    return 0;
}

void StartDrawingThread(ICBYTES& canvas, Mode mode, int x1, int y1, int x2, int y2, int color) {
    HANDLE* hSlot = GetAvailableThreadSlot();
    if (!hSlot) return;

    DrawParams* params = new DrawParams{ &canvas, mode, x1, y1, x2, y2, color };

    *hSlot = CreateThread(NULL, 0, DrawThread, params, 0, NULL);
    if (*hSlot) {
        SetThreadPriority(*hSlot, THREAD_PRIORITY_LOWEST);
    }
    else {
        delete params;
    }
}


void LogMouseAction(const char* action, int x, int y) {
    if (x < 0 || x >= m.X() || y < 0 || y >= m.Y()) {
        ICG_printf(MouseLogBox, "%s - Out of bounds - X: %d, Y: %d\n", action, x, y);
    }
    else {
        ICG_printf(MouseLogBox, "%s - X: %d, Y: %d\n", action, x, y);
    }
}


// Sol Fare Tuşuna Basıldığında
void OnMouseLDown() {
    isDrawing = true;
    prevX = ICG_GetMouseX() - frameOffsetX;
    prevY = ICG_GetMouseY() - frameOffsetY;

    if (activeMode != NORMAL) {
        startX = prevX;
        startY = prevY;
    }
}

// Fare Hareket Ettiğinde
void OnMouseMove(int x, int y) {
    if (!isDrawing) return;

    currentX = ICG_GetMouseX() - frameOffsetX;
    currentY = ICG_GetMouseY() - frameOffsetY;

    ClearTemporaryCanvas();

    if (activeMode == ERASER) {
        if (prevX >= 0 && prevY >= 0) {
            StartEraseThread(m, currentX, currentY);
        }
        prevX = currentX;
        prevY = currentY;
        return;
    }

    if (activeMode == NORMAL) {
        if (prevX >= 0 && prevY >= 0) {
            StartFreehandDrawingThread(m, prevX, prevY, currentX, currentY, selectedColor);
        }
        prevX = currentX;
        prevY = currentY;
    }
    else {
        // Yeni çizim başlamadan önce çalışan thread’leri temizle
        for (int i = 0; i < MAX_THREADS; i++) {
            if (threadPool[i]) {
                FreeThreadSlot(threadPool[i]);
            }
        }

        int centerX = (startX + currentX) / 2;
        int centerY = (startY + currentY) / 2;
        int width = abs(currentX - startX);
        int height = abs(currentY - startY);
        int radius = max(2, width / 2);

        switch (activeMode) {
        case RECTANGLE:
            DrawRectangle(tempCanvas, startX, startY, currentX - startX, currentY - startY, selectedColor);
            break;
        case FILLED_RECTANGLE:
            DrawFilledRectangle(tempCanvas, startX, startY, currentX - startX, currentY - startY, selectedColor);
            break;
        case ELLIPSE:
            DrawEllipse(tempCanvas, centerX, centerY, max(2, width / 2), max(2, height / 2), selectedColor);
            break;
        case LINE:
            DrawLine(tempCanvas, startX, startY, currentX, currentY, selectedColor);
            break;
        case CIRCLE:
            DrawCircle(tempCanvas, centerX, centerY, radius, selectedColor);
            break;
        case FILLED_CIRCLE:
            DrawFilledCircle(tempCanvas, centerX, centerY, radius, selectedColor);
            break;
        case TRIANGLE:
            DrawTriangle(tempCanvas, startX, startY, currentX, startY, (startX + currentX) / 2, currentY, selectedColor);
            break;
        case PLUS_MARK:
            DrawPlusMark(tempCanvas, (startX + currentX) / 2, (startY + currentY) / 2, abs(currentX - startX) / 4, selectedColor);
            break;
        default:
            break;
        }

        // Alpha kanalını güncelle
        for (int y = 0; y < tempCanvas.Y(); y++) {
            for (int x = 0; x < tempCanvas.X(); x++) {
                if (!(tempCanvas.B(x, y, 0) == 0xFF &&
                    tempCanvas.B(x, y, 1) == 0xFF &&
                    tempCanvas.B(x, y, 2) == 0xFF)) {
                    tempCanvas.B(x, y, 3) = 0xFF;  // Şekillerin çizilirken görünmesi için
                }
            }
        }

        UpdateCombinedCanvas();
    }
}


// Sol Fare Tuşunu Bıraktığınızda
void OnMouseLUp() {
    if (!isDrawing) return;

    isDrawing = false;

    int centerX = (startX + currentX) / 2;
    int centerY = (startY + currentY) / 2;
    int width = abs(currentX - startX);
    int height = abs(currentY - startY);
    int radius = width / 2;

    if (activeMode != NORMAL) {
        StartDrawingThread(m, activeMode, startX, startY, currentX, currentY, selectedColor);
        ICG_printf(MouseLogBox, "Thread started for drawing: %d\n", activeMode);
        ClearTemporaryCanvas();
        DisplayImage(FRM1, m);
    }

    prevX = -1;
    prevY = -1;
}

// Fare Olaylarını Ayarlama
void SetupMouseHandlers() {
    ICG_SetOnMouseLDown(OnMouseLDown);
    ICG_SetOnMouseMove(OnMouseMove);
    ICG_SetOnMouseLUp(OnMouseLUp);
}

// Canvas Başlat
void InitializeCanvas() {
    int canvasWidth = 800;
    int canvasHeight = 600;

    // Ana canvas (4 kanal: RGBA)
    CreateMatrix(m, canvasWidth, canvasHeight, 4, ICB_UCHAR);
    CreateMatrix(backBuffer, canvasWidth, canvasHeight, 3, ICB_UCHAR);
    CreateMatrix(tempCanvas, canvasWidth, canvasHeight, 4, ICB_UCHAR);

    // **Canvas'ı tamamen beyaza ayarla**
    for (int y = 0; y < canvasHeight; y++) {
        for (int x = 0; x < canvasWidth; x++) {
            m.B(x, y, 0) = 0xFF;  // **Mavi**
            m.B(x, y, 1) = 0xFF;  // **Yeşil**
            m.B(x, y, 2) = 0xFF;  // **Kırmızı**
            m.B(x, y, 3) = 0xFF;  // **Alpha (tam opak)**
        }
    }

    FillRect(backBuffer, 0, 0, canvasWidth, canvasHeight, 0xFFFFFF);

    // **TempCanvas temizle**
    ClearTemporaryCanvas();

    // **Çerçeve oluştur ve ekrana yansıt**
    FRM1 = ICG_FrameMedium(frameOffsetX, frameOffsetY, canvasWidth, canvasHeight);
    DisplayImage(FRM1, m);
}

void ResizeImage(ICBYTES& source, ICBYTES& destination, int newWidth, int newHeight) {
    CreateMatrix(destination, newWidth, newHeight, 3, ICB_UCHAR);

    for (int y = 0; y < newHeight; ++y) {
        for (int x = 0; x < newWidth; ++x) {
            int srcX = x * source.X() / newWidth;
            int srcY = y * source.Y() / newHeight;

            destination.B(x, y, 0) = source.B(srcX, srcY, 0);
            destination.B(x, y, 1) = source.B(srcX, srcY, 1);
            destination.B(x, y, 2) = source.B(srcX, srcY, 2);
        }
    }
}

void NewFunc() {
    // Önceki çerçeveyi yok et
    ICG_DestroyWidget(FRM1);

    // Yeni çerçeve oluştur (800x600 boyutunda)
    FRM1 = ICG_FrameMedium(frameOffsetX, frameOffsetY, 800, 600);

    // **Yeni bir canvas oluştur**
    CreateMatrix(m, 800, 600, 4, ICB_UCHAR);

    // **Tüm pikselleri beyaz yap ve alpha değerini sıfırla**
    for (int y = 0; y < m.Y(); y++) {
        for (int x = 0; x < m.X(); x++) {
            m.B(x, y, 0) = 0xFF;  // Mavi (tam beyaz)
            m.B(x, y, 1) = 0xFF;  // Yeşil (tam beyaz)
            m.B(x, y, 2) = 0xFF;  // Kırmızı (tam beyaz)
            m.B(x, y, 3) = 0xFF;  // Alpha (tam opak)
        }
    }

    // **Ekranı güncelle**
    DisplayImage(FRM1, m);
    ICG_printf(MouseLogBox, "New canvas created and set to white.\n");
}

void OpenFunc() {

    ICBYTES dosyol, resim;
    ReadImage(OpenFileMenu(dosyol, "JPEG\0*.JPG\0"), resim);
    
    if (resim.X() == 0 || resim.Y() == 0) {
        ICG_printf(MouseLogBox, "Error: Image could not be loaded!\n");
        return;
    }

    ResizeImage(resim, m, m.X(), m.Y());  // Resmi tuval boyutuna ayarla

    // **Alpha kanalını düzeltelim!**
    for (int y = 0; y < m.Y(); y++) {
        for (int x = 0; x < m.X(); x++) {
            m.B(x, y, 3) = 0x00; // **Alpha tamamen şeffaf olacak!**
        }
    }

    // **Tuvali güncelle ve ekrana çiz**
    ManualCopy(m, backBuffer);
    DisplayImage(FRM1, backBuffer);

    // Hafızayı temizle
    Free(resim);
}

// Bitmap'i dosyaya kaydetme fonksiyonu
bool SaveBitmapToFile(HBITMAP hBitmap, const char* filePath) {
    BITMAP bmp;
    GetObject(hBitmap, sizeof(BITMAP), &bmp);

    BITMAPFILEHEADER bmfHeader;
    BITMAPINFOHEADER bi;

    bi.biSize = sizeof(BITMAPINFOHEADER);
    bi.biWidth = bmp.bmWidth;
    bi.biHeight = -bmp.bmHeight; // Pozitif değer ters görüntü kaydeder
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

void SaveFunc() {
    OPENFILENAME ofn;       // Dosya seçme iletişim kutusu
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
                MessageBox(ICG_GetMainWindow(), "Resim başarıyla kaydedildi.", "Bilgi", MB_OK);
            }
            else {
                MessageBox(ICG_GetMainWindow(), "Resim kaydedilemedi.", "Hata", MB_OK);
            }
            DeleteObject(bitmap); // Bitmap'i serbest bırak
        }
        else {
            MessageBox(ICG_GetMainWindow(), "Bitmap oluşturulamadı.", "Hata", MB_OK);
        }
    }
}
void Cleanup() {
    for (int i = 0; i < MAX_THREADS; i++) {
        if (threadPool[i]) {
            FreeThreadSlot(threadPool[i]);
        }
    }
    Free(m);
    Free(tempCanvas);
    Free(backBuffer);
}

void ExitFunc() {

    PostQuitMessage(0);  // Programdan güvenli bir şekilde çıkış yapar
    Cleanup();
}

void Elipsee() {
    activeMode = ELLIPSE;  // Elips modunu etkinleştir
    ICG_printf(MouseLogBox, "Ellipse mode activated.\n");
}

void NormalMode() {
    activeMode = NORMAL;  // Normal modu etkinleştir
    ICG_printf(MouseLogBox, "Normal mode activated.\n");
}
void Rectt() {
    activeMode = RECTANGLE;  // Çerçeveli dikdörtgen modunu etkinleştir
    ICG_printf(MouseLogBox, "Rectangle mode activated.\n");
}
void Rectfil() {
    activeMode = FILLED_RECTANGLE;  // Dolu dikdörtgen modunu etkinleştir
    ICG_printf(MouseLogBox, "Filled Rectangle mode activated.\n");
}
void Linee() {
    activeMode = LINE;  // Çizgi modunu etkinleştir
    ICG_printf(MouseLogBox, "Line mode activated.\n");
}
void Circle() {
    activeMode = CIRCLE;  // Çizgi modunu etkinleştir
    ICG_printf(MouseLogBox, "Circle mode activated.\n");
}
void FilledCircle() {
    activeMode = FILLED_CIRCLE;  // Çizgi modunu etkinleştir
    ICG_printf(MouseLogBox, "Filled Circle mode activated.\n");
}
void Triangle() {
    activeMode = TRIANGLE;  // Çizgi modunu etkinleştir
    ICG_printf(MouseLogBox, "Triangle mode activated.\n");
}
void MarkPlus() {
    activeMode = PLUS_MARK;  // Çizgi modunu etkinleştir
    ICG_printf(MouseLogBox, "Plus Mark mode activated.\n");
}

void CreateModeButtons() {
    static ICBYTES pencilIcon;
    CreateImage(pencilIcon, 75, 30, ICB_UINT);
    FillRect(pencilIcon, 0, 0, 75, 30, 0xFFFFFF);  // Arka plan beyaz

    // **Kalem Gövdesi (Sarı)**
    int x1 = 15, y1 = 5;
    int x2 = 50, y2 = 5;
    int x3 = 50, y3 = 25;
    int x4 = 15, y4 = 25;
    FillRect(pencilIcon, x1, y1, x2 - x1, y3 - y1, 0xFFD700);  // Sarı gövde

    // **Pembe Silgi (Sol Tarafta)**
    int sx1 = 5, sy1 = 5;
    int sx2 = 15, sy2 = 5;
    int sx3 = 15, sy3 = 25;
    int sx4 = 5, sy4 = 25;
    FillRect(pencilIcon, sx1, sy1, sx2 - sx1, sy3 - sy1, 0xFFC0CB);  // Pembe alan

    // **Sarı Gövde Üzerine Siyah Çizgiler**
    for (int i = 0; i < 3; i++) {
        Line(pencilIcon, x1 + 5, y1 + 5 + (i * 5), x2 - 5, y1 + 5 + (i * 5), 0x000000);
    }

    // **Kalem Ucu (Dolu Üçgen - Siyah)**
    int ux1 = 50, uy1 = 5;
    int ux2 = 70, uy2 = 15;
    int ux3 = 50, uy3 = 25;

    // **Üçgeni Doldur - Scan-line algoritması**
    for (int y = uy1; y <= uy3; y++) {
        int startX = ux1 + ((y - uy1) * (ux2 - ux1)) / (uy2 - uy1);
        int endX = ux1 + ((y - uy1) * (ux3 - ux1)) / (uy3 - uy1);

        if (startX > endX) std::swap(startX, endX); // Koordinatları doğru sıraya koy

        for (int x = startX; x <= endX; x++) {
            if (x >= 0 && x < 75 && y >= 0 && y < 30) {  // Buton sınırları içinde kal
                pencilIcon.B(x, y, 0) = 0x00;  // Mavi (RGB) - Siyah
                pencilIcon.B(x, y, 1) = 0x00;  // Yeşil (RGB) - Siyah
                pencilIcon.B(x, y, 2) = 0x00;  // Kırmızı (RGB) - Siyah
            }
        }
    }

    // **Çerçeve Çiz (Gri Kenarlık)**
    Line(pencilIcon, ux1, uy1, ux2, uy2, 0x808080);
    Line(pencilIcon, ux2, uy2, ux3, uy3, 0x808080);
    Line(pencilIcon, ux3, uy3, ux1, uy1, 0x808080);


    // **Butona ekleme**
    
    int pencilButton = ICG_BitmapButton(700, 55, 75, 30, NormalMode);
    SetButtonBitmap(pencilButton, pencilIcon);
    ICG_Static(705, 85, 200, 15, "Pen");
}

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
void CreateMenuItems() {
    // Menüleri tanımla
    HMENU AnaMenu, DosyaMenu, GorunumMenu;

    AnaMenu = CreateMenu();
    DosyaMenu = CreatePopupMenu();
    GorunumMenu = CreatePopupMenu();

    // Dosya Menüsü
    ICG_AppendMenuItem(DosyaMenu, "Yeni", NewFunc);
    ICG_AppendMenuItem(DosyaMenu, "Aç", OpenFunc);
    ICG_AppendMenuItem(DosyaMenu, "Kaydet", SaveFunc);
    ICG_AppendMenuItem(DosyaMenu, "Çıkış", ExitFunc);

    // Görünüm Menüsü
    ICG_AppendMenuItem(GorunumMenu, "Tam Ekran", ToggleFullScreen);

    // Ana Menüye ekle
    AppendMenu(AnaMenu, MF_POPUP, (UINT_PTR)DosyaMenu, "Dosya");
    AppendMenu(AnaMenu, MF_POPUP, (UINT_PTR)GorunumMenu, "Görünüm");

    // Ana Menü'yü GUI'ye ata
    ICG_SetMenu(AnaMenu);
}
void CreateDrawingButtons() {
    int BTN1, BTN2, BTN3, BTN4, BTN5, BTN6, BTN7, BTN8, BTNERASER;

    static ICBYTES butonresmi, butonresmi1, butonresmi2, butonresmi3, butonresmi4, butonresmi5, butonresmi6, butonresmi7, eraserIcon;

    // BTN1: Elips
    CreateImage(butonresmi, 50, 50, ICB_UINT);
    butonresmi = 0xffffff;
    BTN1 = ICG_BitmapButton(450, 10, 40, 40, Elipsee);
    Ellipse(butonresmi, 9, 8, 17, 17, 0x0000000);
    Ellipse(butonresmi, 10, 9, 16, 16, 0x0000000);
    Ellipse(butonresmi, 11, 10, 15, 15, 0x0000000);

    SetButtonBitmap(BTN1, butonresmi);

    // BTN2: Dolu Dikdörtgen
    CreateImage(butonresmi1, 50, 50, ICB_UINT);
    butonresmi1 = 0xffffff;
    BTN2 = ICG_BitmapButton(500, 10, 40, 40, Rectfil);
    FillRect(butonresmi1, 10, 9, 30, 30, 0x0000000);
    SetButtonBitmap(BTN2, butonresmi1);

    // BTN3: Çerçeve Dikdörtgen
    CreateImage(butonresmi2, 50, 50, ICB_UINT);
    butonresmi2 = 0xffffff;
    BTN3 = ICG_BitmapButton(550, 10, 40, 40, Rectt);
    Rect(butonresmi2, 8, 9, 30, 30, 0x0000000);
    Rect(butonresmi2, 9, 10, 30, 30, 0x0000000);
    Rect(butonresmi2, 10, 11, 30, 30, 0x0000000);
    SetButtonBitmap(BTN3, butonresmi2);

    // BTN4: Çizgi
    CreateImage(butonresmi3, 50, 50, ICB_UINT);
    butonresmi3 = 0xffffff;
    BTN4 = ICG_BitmapButton(600, 10, 40, 40, Linee);
    Line(butonresmi3, 10, 11, 40, 40, 0x0000000);
    Line(butonresmi3, 11, 12, 40, 40, 0x0000000);
    Line(butonresmi3, 12, 13, 40, 40, 0x0000000);
    SetButtonBitmap(BTN4, butonresmi3);

    // BTN5: Artı İşareti
    CreateImage(butonresmi4, 50, 50, ICB_UINT);
    butonresmi4 = 0xffffff;
    BTN5 = ICG_BitmapButton(450, 55, 40, 40, MarkPlus);
    MarkPlus(butonresmi4, 25, 25, 10, 0);
    SetButtonBitmap(BTN5, butonresmi4);

    // BTN6: Daire İşareti
    CreateImage(butonresmi5, 50, 50, ICB_UINT);
    butonresmi5 = 0xffffff;
    BTN6 = ICG_BitmapButton(500, 55, 40, 40, Circle);
    Circle(butonresmi5, 25, 25, 10, 0);
    SetButtonBitmap(BTN6, butonresmi5);

    // BTN7: Dolu Daire İşareti
    CreateImage(butonresmi6, 50, 50, ICB_UINT);
    butonresmi6 = 0xffffff;
    BTN7 = ICG_BitmapButton(550, 55, 40, 40, FilledCircle);
    FillCircle(butonresmi6, 25, 25, 10, 0);
    SetButtonBitmap(BTN7, butonresmi6);

    // BTN8: Üçgen İşareti
    CreateImage(butonresmi7, 50, 50, ICB_UINT);
    butonresmi7 = 0xFFFFFF; // Arka plan beyaz

    // Üçgenin noktalarını belirle (buton ortasına göre)
    int x1 = 25, y1 = 10;   // Üst nokta
    int x2 = 10, y2 = 40;   // Sol alt
    int x3 = 40, y3 = 40;  // Sağ alt

    BTN8 = ICG_BitmapButton(600, 55, 40, 40, Triangle);
    // Üçgeni butona çiz
    Line(butonresmi7, x1, y1, x2, y2, 0x000000); // Sol kenar
    Line(butonresmi7, x2, y2, x3, y3, 0x000000); // Alt kenar
    Line(butonresmi7, x3, y3, x1, y1, 0x000000); // Sağ kenar
    SetButtonBitmap(BTN8, butonresmi7);

    //Silgi 
    // Silgi Butonu (50x50 boyutunda)
    // Silgi Butonu (50x50 boyutunda)
    CreateImage(eraserIcon, 50, 50, ICB_UINT);
    eraserIcon = 0xFFFFFF;  // Arka plan beyaz

    // **Silgi Gövdesi (Yatay Dikdörtgen)**
    int x4 = 10, y4 = 20;  // Sol üst köşe
    int x5 = 40, y5 = 20;  // Sağ üst köşe
    int x6 = 40, y6 = 35;  // Sağ alt köşe
    int x7 = 10, y7 = 35;  // Sol alt köşe

    // **Sol Kısım (Pembe)**
    FillRect(eraserIcon, x4, y4, (x5 - x4) / 2, y6 - y4, 0xFFC0CB);  // Pembe sol taraf

    // **Sağ Kısım (Siyah)**
    FillRect(eraserIcon, x4 + (x5 - x4) / 2, y4, (x5 - x4) / 2, y6 - y4, 0x000000);  // Siyah sağ taraf

    // **Orta Çizgi (Gri)**
    DrawLine(eraserIcon, x4 + (x5 - x4) / 2, y4, x4 + (x5 - x4) / 2, y6, 0x808080); // Pembe ve siyahı ayıran gri çizgi

    // **Silgi Çerçevesi**
    DrawLine(eraserIcon, x4, y4, x5, y5, 0x000000); // Üst çizgi
    DrawLine(eraserIcon, x5, y5, x6, y6, 0x000000); // Sağ çizgi
    DrawLine(eraserIcon, x6, y6, x7, y7, 0x000000); // Alt çizgi
    DrawLine(eraserIcon, x7, y7, x4, y4, 0x000000); // Sol çizgi

    // **Silgi Butonu**
    BTNERASER = ICG_BitmapButton(650, 10, 40, 40, EraserMode);
    SetButtonBitmap(BTNERASER, eraserIcon);
    ICG_Static(650, 50, 200, 15, "Eraser");

    //Bitmap Buton Ekleme 
    int SaveButton;
    SaveButton = ICG_BitmapButton(900, 10, 45, 45, SaveFunc);
    SetButtonBitmap(SaveButton, saveIcon);
   
}

// Ana GUI Fonksiyonu
void ICGUI_main() {
    ICGUI_Create();  // GUI başlat

    ReadImage("savee.bmp", saveIcon);

    CreateMenuItems();          // Menüleri oluştur
    InitializeCanvas();    // Çizim alanını başlat
    //ClearCanvas();
    InitializeColorPreview(); // Renk Paletini Ekle
    CreateColorButtons();       // Renk seçici butonları ekle
    CreateThicknessTrackbar();  // Çizgi kalınlığı için trackbar ekle
    CreateModeButtons();  // Mod butonlarını ekle
    SetupMouseHandlers(); // Fare olaylarını ayarla
    CreateDrawingButtons(); // Çizim butonlarını oluştur

    // Mouse hareketlerini izlemek için metin kutusu
    MouseLogBox = ICG_MLEditSunken(10, 700, 600, 80, "", SCROLLBAR_V); // 600x80 boyutunda metin kutusu

    FillRect(m, 10, 10, 100, 100, 0xFF0000);  // **Kırmızı Olmalı**
    FillRect(m, 120, 10, 100, 100, 0x00FF00); // **Yeşil Olmalı**
    FillRect(m, 230, 10, 100, 100, 0x0000FF); // **Mavi Olmalı**
    DisplayImage(FRM1, m);


    ICG_Button(870, 60, 80, 30, "Clear", ClearCanvas);
    



}