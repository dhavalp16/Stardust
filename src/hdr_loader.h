// hdr_loader.h - Custom Radiance HDR (.hdr) file loader for Raylib
// Handles RLE-compressed RGBE format with inline tone mapping.
// Outputs RGBA8 directly, avoiding the massive float intermediate buffer
// that causes stb_image to fail on large HDR files.
#pragma once
#include "raylib.h"
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cmath>

static Image LoadHDRManual(const char *filename) {
    Image image = {0};

    FILE *f = fopen(filename, "rb");
    if (!f) return image;

    // Skip header lines until empty line
    char line[1024];
    while (fgets(line, sizeof(line), f)) {
        size_t len = strlen(line);
        while (len > 0 && (line[len - 1] == '\n' || line[len - 1] == '\r'))
            line[--len] = '\0';
        if (len == 0) break;
    }

    // Read resolution line
    int width = 0, height = 0;
    if (!fgets(line, sizeof(line), f)) { fclose(f); return image; }
    if (sscanf(line, "-Y %d +X %d", &height, &width) != 2 &&
        sscanf(line, "+Y %d +X %d", &height, &width) != 2 &&
        sscanf(line, "+X %d -Y %d", &width, &height) != 2 &&
        sscanf(line, "+X %d +Y %d", &width, &height) != 2) {
        fclose(f);
        return image;
    }

    if (width <= 0 || height <= 0 || width > 65536 || height > 65536) {
        fclose(f);
        return image;
    }

    // Allocate output RGBA8 buffer
    unsigned char *pixels = (unsigned char *)malloc((size_t)width * height * 4);
    if (!pixels) { fclose(f); return image; }

    // Temporary scanline buffer for RGBE data (one row only)
    unsigned char *scanline = (unsigned char *)malloc((size_t)width * 4);
    if (!scanline) { free(pixels); fclose(f); return image; }

    for (int y = 0; y < height; y++) {
        unsigned char header[4];
        if (fread(header, 1, 4, f) != 4) { free(scanline); free(pixels); fclose(f); return image; }

        if (width >= 8 && width <= 0x7fff && header[0] == 2 && header[1] == 2) {
            // New-style adaptive RLE
            int lineWidth = ((int)header[2] << 8) | header[3];
            if (lineWidth != width) { free(scanline); free(pixels); fclose(f); return image; }

            for (int ch = 0; ch < 4; ch++) {
                int pos = 0;
                while (pos < width) {
                    unsigned char code;
                    if (fread(&code, 1, 1, f) != 1) { free(scanline); free(pixels); fclose(f); return image; }

                    if (code > 128) {
                        int count = code - 128;
                        unsigned char val;
                        if (fread(&val, 1, 1, f) != 1) { free(scanline); free(pixels); fclose(f); return image; }
                        if (pos + count > width) count = width - pos;
                        for (int i = 0; i < count; i++)
                            scanline[(pos + i) * 4 + ch] = val;
                        pos += count;
                    } else {
                        int count = code;
                        if (count == 0) { free(scanline); free(pixels); fclose(f); return image; }
                        if (pos + count > width) count = width - pos;
                        for (int i = 0; i < count; i++) {
                            if (fread(&scanline[(pos + i) * 4 + ch], 1, 1, f) != 1) {
                                free(scanline); free(pixels); fclose(f); return image;
                            }
                        }
                        pos += count;
                    }
                }
            }
        } else {
            // Flat (uncompressed) RGBE
            scanline[0] = header[0];
            scanline[1] = header[1];
            scanline[2] = header[2];
            scanline[3] = header[3];
            if (width > 1) {
                size_t remaining = (size_t)(width - 1) * 4;
                if (fread(scanline + 4, 1, remaining, f) != remaining) {
                    free(scanline); free(pixels); fclose(f); return image;
                }
            }
        }

        // Convert RGBE scanline to tone-mapped RGBA8
        for (int x = 0; x < width; x++) {
            size_t idx = (size_t)y * width + x;
            unsigned char re = scanline[x * 4 + 0];
            unsigned char ge = scanline[x * 4 + 1];
            unsigned char be = scanline[x * 4 + 2];
            unsigned char e  = scanline[x * 4 + 3];

            float rf, gf, bf;
            if (e == 0) {
                rf = gf = bf = 0.0f;
            } else {
                float scale = ldexpf(1.0f, (int)e - 128 - 8);
                rf = re * scale;
                gf = ge * scale;
                bf = be * scale;
            }

            // Exposure + Reinhard tone mapping + gamma + contrast crush
            float exposure = 10.0f;
            rf *= exposure; gf *= exposure; bf *= exposure;
            rf = rf / (rf + 1.0f);
            gf = gf / (gf + 1.0f);
            bf = bf / (bf + 1.0f);
            // Combined gamma correction and contrast curve: (v ^ (1/2.2)) ^ 2.0 = v ^ (2.0/2.2)
            // Cuts powf calls in half (saves 150 million operations on 10000x5000 HDR)
            float gammaContrast = 2.0f / 2.2f;
            rf = powf(rf, gammaContrast);
            gf = powf(gf, gammaContrast);
            bf = powf(bf, gammaContrast);

            pixels[idx * 4 + 0] = (unsigned char)(rf * 255.0f);
            pixels[idx * 4 + 1] = (unsigned char)(gf * 255.0f);
            pixels[idx * 4 + 2] = (unsigned char)(bf * 255.0f);
            pixels[idx * 4 + 3] = 255;
        }
    }

    free(scanline);
    fclose(f);

    image.data = pixels;
    image.width = width;
    image.height = height;
    image.mipmaps = 1;
    image.format = PIXELFORMAT_UNCOMPRESSED_R8G8B8A8;
    return image;
}
