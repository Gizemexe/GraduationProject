#include "icb_gui.h" 
#include "ic_media.h" 
#include "icbytes.h"
#include <windows.h>
#include <stdio.h>
#include <commdlg.h> // Save file dialog için
#include <iostream> 


// Global değişkenler
ICBYTES m;               // Çizim alanı
int FRM1;                 // Çizim alanı paneli
bool isDrawing = false;   // Çizim durumunu takip etme
int prevX = -1, prevY = -1; // Önceki fare konumu

HANDLE hThread;      // Çizim iş parçacığı
DWORD threadID;      // İş parçacığı kimliği
bool stopDrawing = false; // İş parçacığının durumu

int SLE1;               // Kademe kutucuğunun bağlı olduğu metin kutusu
int RTrackBar; // Renk kademeleri için track barlar
int selectedColor = 0x000000; // Siyah
int ColorPreviewFrame; // Renk önizleme çerçevesi
ICBYTES ColorPreview;  // Renk önizleme alanı için bir matris

int MouseLogBox; // Fare hareketlerini göstermek için metin kutusu

int frameOffsetX = 50; // Çerçeve X başlangıç konumu
int frameOffsetY = 100; // Çerçeve Y başlangıç konumu

int lineThickness = 1; // Varsayılan olarak 1 piksel kalınlık

int startX = -1, startY = -1; // Fare sürükleme başlangıç pozisyonu
int currentX = -1, currentY = -1; // Fare sürükleme geçerli pozisyonu

enum Mode { NORMAL, ELLIPSE, RECTANGLE, FILLED_RECTANGLE, LINE, CIRCLE, FILLED_CIRCLE, TRIANGLE, PLUS_MARK, ERASER };

Mode activeMode = NORMAL; // Varsayılan mod NORMAL

struct DrawParams {
    ICBYTES* canvas;
    Mode mode;
    int startX, startY;
    int endX, endY;
    int color;
};

ICBYTES tempCanvas;

// Çizim Alanını Temizle
void ClearCanvas() {
    // Canvas'ı beyaz ile temizle
    FillRect(m, 0, 0, 800, 600, 0xFFFFFF);

    // Geçici canvas'ı temizle
    FillRect(tempCanvas, 0, 0, tempCanvas.X(), tempCanvas.Y(), 0xFFFFFF);

    DisplayImage(FRM1, m);
}

// GUI Başlatma Fonksiyonu
void ICGUI_Create() {
    ICG_MWSize(1000, 600);
    ICG_MWTitle("Paint Uygulamasi - ICBYTES");
}

void UpdateLineThickness(int value) {
    lineThickness = value; // Kalınlık değerini güncelle
    ICG_printf(MouseLogBox, "Line thickness updated: %d\n", lineThickness);
}

void CreateThicknessTrackbar() {
    ICG_TrackBarH(700, 30, 200, 30, UpdateLineThickness); // Trackbar pozisyonu ve uzunluğu
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

    while (true) {
        // Pikseli çiz ve kalınlığa göre çevresini doldur
        for (int i = -lineThickness / 2; i <= lineThickness / 2; i++) {
            for (int j = -lineThickness / 2; j <= lineThickness / 2; j++) {
                int px = x1 + i;
                int py = y1 + j;
                if (px >= 0 && px < canvas.X() && py >= 0 && py < canvas.Y()) {
                    canvas.B(px, py, 0) = color & 0xFF;        // Mavi
                    canvas.B(px, py, 1) = (color >> 8) & 0xFF; // Yeşil
                    canvas.B(px, py, 2) = (color >> 16) & 0xFF; // Kırmızı
                }
            }
        }

        // Çizim tamamlandıysa döngüyü kır
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

DWORD WINAPI DrawShape(LPVOID lpParam) {
    DrawParams* params = (DrawParams*)lpParam;

    switch (params->mode) {
    case ELLIPSE: {
        int radiusX = abs(params->endX - params->startX) / 2;
        int radiusY = abs(params->endY - params->startY) / 2;
        int centerX = (params->startX + params->endX) / 2;
        int centerY = (params->startY + params->endY) / 2;

        Ellipse(*params->canvas, centerX, centerY, radiusX, radiusY, params->color);
        break;
    }
    case RECTANGLE:
        Rect(*params->canvas, params->startX, params->startY, params->endX, params->endY, params->color);
        break;
    case FILLED_RECTANGLE:
        FillRect(*params->canvas, params->startX, params->startY, params->endX, params->endY, params->color);
        break;
    case LINE:
        DrawLine(*params->canvas, params->startX, params->startY, params->endX, params->endY, params->color);
        break;
    default:
        break;
    }

    // İş parçacığı parametrelerini temizle
    delete params;
    return 0;
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
    int x = 0, y = radius;
    int d = 1 - radius;

    while (x <= y) {
        // 8 yönlü simetriyi uygula (Bresenham Circle Algorithm)
        for (int i = -1; i <= 1; i += 2) {
            for (int j = -1; j <= 1; j += 2) {
                if (cx + x * i >= 0 && cx + x * i < canvas.X() && cy + y * j >= 0 && cy + y * j < canvas.Y())
                    canvas.B(cx + x * i, cy + y * j, 0) = color & 0xFF;
                if (cx + y * i >= 0 && cx + y * i < canvas.X() && cy + x * j >= 0 && cy + x * j < canvas.Y())
                    canvas.B(cx + y * i, cy + x * j, 0) = color & 0xFF;
            }
        }

        if (d < 0) {
            d += 2 * x + 3;
        }
        else {
            d += 2 * (x - y) + 5;
            y--;
        }
        x++;
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

    // Bölge 1 (Daha yatay olan kısım)
    p = rySq - (rxSq * ry) + (0.25 * rxSq);
    while (px < py) {
        for (int i = -1; i <= 1; i += 2) {
            for (int j = -1; j <= 1; j += 2) {
                if (cx + x * i >= 0 && cx + x * i < canvas.X() && cy + y * j >= 0 && cy + y * j < canvas.Y())
                    canvas.B(cx + x * i, cy + y * j, 0) = color & 0xFF;
            }
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

    // Bölge 2 (Daha dikey olan kısım)
    p = rySq * (x + 0.5) * (x + 0.5) + rxSq * (y - 1) * (y - 1) - rxSq * rySq;
    while (y >= 0) {
        for (int i = -1; i <= 1; i += 2) {
            for (int j = -1; j <= 1; j += 2) {
                if (cx + x * i >= 0 && cx + x * i < canvas.X() && cy + y * j >= 0 && cy + y * j < canvas.Y())
                    canvas.B(cx + x * i, cy + y * j, 0) = color & 0xFF;
            }
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
        for (int x = -radius; x <= radius; x++) {
            if (x * x + y * y <= radius * radius) {
                int px = cx + x;
                int py = cy + y;
                if (px >= 0 && px < canvas.X() && py >= 0 && py < canvas.Y()) {
                    canvas.B(px, py, 0) = color & 0xFF;
                    canvas.B(px, py, 1) = (color >> 8) & 0xFF;
                    canvas.B(px, py, 2) = (color >> 16) & 0xFF;
                }
            }
        }
    }
}


void DrawPlusMark(ICBYTES& canvas, int xc, int yc, int size, int color) {
    DrawLine(canvas, xc - size, yc, xc + size, yc, color);
    DrawLine(canvas, xc, yc - size, xc, yc + size, color);
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

int ConvertToBGR(int color) {
    return ((color & 0xFF) << 16) | (color & 0xFF00) | ((color >> 16) & 0xFF);
}


void SelectColor1() { selectedColor = ConvertToBGR(0x000000); UpdatePreview(); } // Siyah
void SelectColor2() { selectedColor = ConvertToBGR(0x6d6d6d); UpdatePreview(); } // Gri
void SelectColor3() { selectedColor = ConvertToBGR(0xFF0000); UpdatePreview(); } // Kırmızı
void SelectColor4() { selectedColor = ConvertToBGR(0x500000); UpdatePreview(); } // Koyu Kırmızı
void SelectColor5() { selectedColor = ConvertToBGR(0xfc4e03); UpdatePreview(); } // Turuncu
void SelectColor6() { selectedColor = ConvertToBGR(0xfce803); UpdatePreview(); } // Sarı
void SelectColor7() { selectedColor = ConvertToBGR(0x00FF00); UpdatePreview(); } // Yeşil
void SelectColor8() { selectedColor = ConvertToBGR(0x00c4e2); UpdatePreview(); } // Açık Mavi
void SelectColor9() { selectedColor = ConvertToBGR(0x0919ca); UpdatePreview(); } // Mavi
void SelectColor10() { selectedColor = ConvertToBGR(0x8209c0); UpdatePreview(); } // Mor



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
    ColorPreview = 0xff00;
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
    ICBYTES combinedCanvas;
    CreateMatrix(combinedCanvas, m.X(), m.Y(), 3, ICB_UCHAR);

    // Ana tuvali manuel olarak birleştirilmiş tuvale kopyala
    ManualCopy(m, combinedCanvas);

    // Geçici canvas'ı ekle
    for (int y = 0; y < combinedCanvas.Y(); ++y) {
        for (int x = 0; x < combinedCanvas.X(); ++x) {
            // Eğer tempCanvas beyaz değilse (şeffaf değilse), onu ekle
            if (!(tempCanvas.B(x, y, 0) == 0xFF &&
                tempCanvas.B(x, y, 1) == 0xFF &&
                tempCanvas.B(x, y, 2) == 0xFF)) {
                combinedCanvas.B(x, y, 0) = tempCanvas.B(x, y, 0);
                combinedCanvas.B(x, y, 1) = tempCanvas.B(x, y, 1);
                combinedCanvas.B(x, y, 2) = tempCanvas.B(x, y, 2);
            }
        }
    }

    // Birleştirilen tuvali ekrana yansıt
    DisplayImage(FRM1, combinedCanvas);
    Free(combinedCanvas);
}



void ClearTemporaryCanvas() {
    for (int y = 0; y < tempCanvas.Y(); ++y) {
        for (int x = 0; x < tempCanvas.X(); ++x) {
            tempCanvas.B(x, y, 0) = 0xFF; // Mavi
            tempCanvas.B(x, y, 1) = 0xFF; // Yeşil
            tempCanvas.B(x, y, 2) = 0xFF; // Kırmızı
        }
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

    FillRect(tempCanvas, 0, 0, tempCanvas.X(), tempCanvas.Y(), 0xFFFFFF);

    int centerX = (startX + currentX) / 2;
    int centerY = (startY + currentY) / 2;
    int width = abs(currentX - startX);
    int height = abs(currentY - startY);
    int radius = width / 2;

    if (activeMode == ERASER) {
        if (prevX >= 0 && prevY >= 0) {
            for (int i = -lineThickness / 2; i <= lineThickness / 2; ++i) {
                for (int j = -lineThickness / 2; j <= lineThickness / 2; ++j) {
                    int px = currentX + i;
                    int py = currentY + j;

                    if (px >= 0 && px < m.X() && py >= 0 && py < m.Y()) {
                        m.B(px, py, 0) = 0xFF;
                        m.B(px, py, 1) = 0xFF;
                        m.B(px, py, 2) = 0xFF;
                    }

                    if (px >= 0 && px < tempCanvas.X() && py >= 0 && py < tempCanvas.Y()) {
                        tempCanvas.B(px, py, 0) = 0xFF;
                        tempCanvas.B(px, py, 1) = 0xFF;
                        tempCanvas.B(px, py, 2) = 0xFF;
                    }
                }
            }
            UpdateCombinedCanvas();
        }
        prevX = currentX;
        prevY = currentY;
        return;
    }

    if (activeMode == NORMAL) {
        if (prevX >= 0 && prevY >= 0) {
            DrawLine(m, prevX, prevY, currentX, currentY, selectedColor);
            DisplayImage(FRM1, m);
        }
        prevX = currentX;
        prevY = currentY;
    }
    else {
        FillRect(tempCanvas, 0, 0, tempCanvas.X(), tempCanvas.Y(), 0xFFFFFF);

        switch (activeMode) {
        case RECTANGLE:
            DrawRectangle(tempCanvas, startX, startY, currentX - startX, currentY - startY, selectedColor);
            break;
        case FILLED_RECTANGLE:
            DrawFilledRectangle(tempCanvas, startX, startY, currentX - startX, currentY - startY, selectedColor);
            break;
        case ELLIPSE: 
            DrawEllipse(tempCanvas, centerX, centerY, width / 2, height / 2, selectedColor);
            break;
        case LINE:
            DrawLine(tempCanvas, startX, startY, currentX, currentY, selectedColor);
            break;
        case CIRCLE:
            DrawCircle(tempCanvas, centerX, centerY, radius, selectedColor);
            break;
        case FILLED_CIRCLE:
            DrawFilledCircle(tempCanvas, (startX + currentX) / 2, (startY + currentY) / 2, abs(currentX - startX) / 2, selectedColor);
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
        switch (activeMode) {
        case RECTANGLE:
            DrawRectangle(m, startX, startY, currentX - startX, currentY - startY, selectedColor);
            break;
        case FILLED_RECTANGLE:
            DrawFilledRectangle(m, startX, startY, currentX - startX, currentY - startY, selectedColor);
            break;
        case ELLIPSE:
            DrawEllipse(tempCanvas, centerX, centerY, width / 2, height / 2, selectedColor);
            break;
        case LINE:
            DrawLine(m, startX, startY, currentX, currentY, selectedColor);
            break;
        case CIRCLE:
            DrawCircle(m, centerX, centerY, radius, selectedColor);
            break;
        case FILLED_CIRCLE:
            DrawFilledCircle(m, (startX + currentX) / 2, (startY + currentY) / 2, abs(currentX - startX) / 2, selectedColor);
            break;
        case TRIANGLE:
            DrawTriangle(m, startX, startY, currentX, startY, (startX + currentX) / 2, currentY, selectedColor);
            break;
        case PLUS_MARK:
            DrawPlusMark(m, (startX + currentX) / 2, (startY + currentY) / 2, abs(currentX - startX) / 4, selectedColor);
            break;
        default:
            break;
        }

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

    // Ana canvas
    CreateMatrix(m, canvasWidth, canvasHeight, 3, ICB_UCHAR);
    FillRect(m, 0, 0, canvasWidth, canvasHeight, 0xFFFFFF);

    // Geçici canvas
    CreateMatrix(tempCanvas, canvasWidth, canvasHeight, 3, ICB_UCHAR);
    FillRect(tempCanvas, 0, 0, canvasWidth, canvasHeight, 0xFFFFFF);

    // Geçici canvas'ın tamamını tam şeffaf beyaza ayarla
    for (int y = 0; y < tempCanvas.Y(); ++y) {
        for (int x = 0; x < tempCanvas.X(); ++x) {
            tempCanvas.B(x, y, 0) = 0xFF; // Mavi
            tempCanvas.B(x, y, 1) = 0xFF; // Yeşil
            tempCanvas.B(x, y, 2) = 0xFF; // Kırmızı
        }
    }

    // Çerçeve oluştur ve ekrana yansıt
    FRM1 = ICG_FrameMedium(frameOffsetX, frameOffsetY, canvasWidth, canvasHeight);
    DisplayImage(FRM1, m);
}


void NewFunc() {
    ICG_DestroyWidget(FRM1);

    // Yeni çerçeve oluştur (800x600 boyutunda)
    FRM1 = ICG_FrameMedium(frameOffsetX, frameOffsetY, 800, 400);

    // Canvas'ı beyaza sıfırla
    m = FRM1;
    FillRect(m, 0, 0, 800, 600, 0xFFFFFF);
    DisplayMatrix(FRM1, m);
}

void OpenFunc() {

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
            DeleteObject(bitmap);
        }
        else {
            MessageBox(ICG_GetMainWindow(), "Bitmap oluşturulamadı.", "Hata", MB_OK);
        }
    }
}

void ExitFunc() {

    PostQuitMessage(0);  // Programdan güvenli bir şekilde çıkış yapar
}
void CutFunc() {}
void CopyFunc() {}
void PasteFunc() {}
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
    ICG_Button(700, 70, 80, 30, "Normal", NormalMode);
    //ICG_Button(700, 110, 80, 30, "Ellipse", Elipsee);
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
    HMENU AnaMenu, DosyaMenu, DuzenleMenu, GorunumMenu;

    AnaMenu = CreateMenu();
    DosyaMenu = CreatePopupMenu();
    DuzenleMenu = CreatePopupMenu();
    GorunumMenu = CreatePopupMenu();

    // Dosya Menüsü
    ICG_AppendMenuItem(DosyaMenu, "Yeni", NewFunc);
    ICG_AppendMenuItem(DosyaMenu, "Aç", OpenFunc);
    ICG_AppendMenuItem(DosyaMenu, "Kaydet", SaveFunc);
    ICG_AppendMenuItem(DosyaMenu, "Çıkış", ExitFunc);

    // Düzenle Menüsü
    ICG_AppendMenuItem(DuzenleMenu, "Kes", CutFunc);
    ICG_AppendMenuItem(DuzenleMenu, "Kopyala", CopyFunc);
    ICG_AppendMenuItem(DuzenleMenu, "Yapıştır", PasteFunc);

    // Görünüm Menüsü
    ICG_AppendMenuItem(GorunumMenu, "Yakınlaştırma", PasteFunc);
    ICG_AppendMenuItem(GorunumMenu, "Cetveller", PasteFunc);
    ICG_AppendMenuItem(GorunumMenu, "Kılavuz Çizgileri", PasteFunc);
    ICG_AppendMenuItem(GorunumMenu, "Durum Çubuğu", PasteFunc);
    ICG_AppendMenuItem(GorunumMenu, "Tam Ekran", ToggleFullScreen);

    // Ana Menüye ekle
    AppendMenu(AnaMenu, MF_POPUP, (UINT_PTR)DosyaMenu, "Dosya");
    AppendMenu(AnaMenu, MF_POPUP, (UINT_PTR)DuzenleMenu, "Düzenle");
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
    butonresmi7 = 0xffffff;
    BTN8 = ICG_BitmapButton(600, 55, 40, 40, Triangle);
    DrawLine(butonresmi7, 10, 11, 40, 40, 0x0000000);
    DrawLine(butonresmi7, 11, 12, 40, 40, 0x0000000);
    DrawLine(butonresmi7, 12, 13, 40, 40, 0x0000000);
    SetButtonBitmap(BTN8, butonresmi7);

    //Silgi 
    CreateImage(eraserIcon, 50, 50, ICB_UINT);
    eraserIcon = 0xffffff;  // Beyaz zemin üzerinde bir silgi simgesi çizebilirsiniz
    BTNERASER = ICG_BitmapButton(650, 10, 40, 40, EraserMode);
    Rect(eraserIcon, 10, 10, 30, 30, 0x000000); // Simge olarak siyah çerçeve
    Line(eraserIcon, 10, 30, 30, 10, 0x000000); // Çapraz çizgi
    SetButtonBitmap(BTNERASER, eraserIcon);
}

// Ana GUI Fonksiyonu
void ICGUI_main() {
    ICGUI_Create();  // GUI başlat

    //Bitmap Buton Ekleme 
    int SaveButton;
    static ICBYTES saveIcon;
    ReadImage("save.bmp", saveIcon);
    SaveButton = ICG_BitmapButton(900, 10, 40, 40, SaveFunc);
    SetButtonBitmap(SaveButton, saveIcon);

    CreateMenuItems();          // Menüleri oluştur
    InitializeCanvas();    // Çizim alanını başlat
    InitializeColorPreview(); // Renk Paletini Ekle
    CreateColorButtons();       // Renk seçici butonları ekle
    CreateThicknessTrackbar();  // Çizgi kalınlığı için trackbar ekle
    CreateModeButtons();  // Mod butonlarını ekle
    SetupMouseHandlers(); // Fare olaylarını ayarla
    CreateDrawingButtons(); // Çizim butonlarını oluştur

    // Mouse hareketlerini izlemek için metin kutusu
    MouseLogBox = ICG_MLEditSunken(10, 700, 600, 80, "", SCROLLBAR_V); // 600x80 boyutunda metin kutusu

    // Clear butonu 
    ICG_Button(900, 60, 80, 30, "Clear", ClearCanvas);

}