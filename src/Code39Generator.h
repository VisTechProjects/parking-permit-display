#ifndef CODE39GENERATOR_H
#define CODE39GENERATOR_H

#include <Arduino.h>
#include "heltec-eink-modules.h"

class Code39Generator {
private:
    // Code 39 patterns (0 = thin bar/space, 1 = thick bar/space)
    // Format: BSBSBSBSB (B = bar, S = space)
    const char* PATTERNS[44] = {
        "000110100", // 0
        "100100001", // 1
        "001100001", // 2
        "101100000", // 3
        "000110001", // 4
        "100110000", // 5
        "001110000", // 6
        "000100101", // 7
        "100100100", // 8
        "001100100", // 9
        "100001001", // A
        "001001001", // B
        "101001000", // C
        "000011001", // D
        "100011000", // E
        "001011000", // F
        "000001101", // G
        "100001100", // H
        "001001100", // I
        "000011100", // J
        "100000011", // K
        "001000011", // L
        "101000010", // M
        "000010011", // N
        "100010010", // O
        "001010010", // P
        "000000111", // Q
        "100000110", // R
        "001000110", // S
        "000010110", // T
        "110000001", // U
        "011000001", // V
        "111000000", // W
        "010010001", // X
        "110010000", // Y
        "011010000", // Z
        "010000101", // -
        "110000100", // .
        "011000100", // SPACE
        "010101000", // $
        "010100010", // /
        "010001010", // +
        "000101010", // %
        "010010100"  // *
    };

    const char* CHARS = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ-. $/+%*";
    
    EInkDisplay_VisionMasterE290* display;
    
    int findCharIndex(char c) {
        for(int i = 0; i < 44; i++) {
            if(CHARS[i] == c) return i;
        }
        return -1;
    }

public:
    Code39Generator(EInkDisplay_VisionMasterE290* disp) : display(disp) {}
    
    // Compute the pixel width of the rendered barcode for a given text and narrow bar width
    int getBarcodeWidth(const char* text, int narrowWidth = 2) {
        int wideWidth = narrowWidth * 3;
        int total = 0;
        // start pattern
        const char* startPattern = PATTERNS[43];
        for (int i = 0; i < 9; i++) total += (startPattern[i] == '0') ? narrowWidth : wideWidth;
        // inter-character narrow space after start
        total += narrowWidth;

        // content
        for (int t = 0; text[t] != '\0'; t++) {
            char c = toupper(text[t]);
            int idx = findCharIndex(c);
            if (idx != -1) {
                const char* p = PATTERNS[idx];
                for (int i = 0; i < 9; i++) total += (p[i] == '0') ? narrowWidth : wideWidth;
                total += narrowWidth; // inter-character space
            }
        }

        // stop pattern (same as start)
        for (int i = 0; i < 9; i++) total += (startPattern[i] == '0') ? narrowWidth : wideWidth;

        return total;
    }
    
    void drawBarcode(const char* text, int x, int y, int height, int narrowWidth = 2) {
        int wideWidth = narrowWidth * 3;  // Wide bars are 3x narrow bars
        int currentX = x;
        
        // Draw start character *
        const char* startPattern = PATTERNS[43];  // * character
        drawPattern(startPattern, currentX, y, height, narrowWidth, wideWidth);
        currentX += (6 * narrowWidth + 3 * wideWidth) + narrowWidth;  // Add space after pattern
        
        // Draw content
        for(int i = 0; text[i] != '\0'; i++) {
            char c = toupper(text[i]);
            int idx = findCharIndex(c);
            if(idx != -1) {
                drawPattern(PATTERNS[idx], currentX, y, height, narrowWidth, wideWidth);
                currentX += (6 * narrowWidth + 3 * wideWidth) + narrowWidth;  // Add space after pattern
            }
        }
        
        // Draw stop character *
        drawPattern(startPattern, currentX, y, height, narrowWidth, wideWidth);
    }

private:
    void drawPattern(const char* pattern, int x, int y, int height, int narrowWidth, int wideWidth) {
        int currentX = x;
        
        for(int i = 0; i < 9; i++) {
            int width = (pattern[i] == '0') ? narrowWidth : wideWidth;
            if(i % 2 == 0) {  // Draw bar
                display->fillRect(currentX, y, width, height, 0x0000);
            }
            currentX += width;
        }
    }
};

#endif