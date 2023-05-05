#include <cstdarg>
#include <cstdio>
#include <iomanip>
#include <iostream>

#include "IBMFFontDiff.hpp"

using namespace IBMFDefs;

typedef std::shared_ptr<uint8_t[]> BufferPtr;

IBMFFontDiffPtr font1, font2;
int             diffCount;

auto usage(char *name) -> void {
  std::cout << "Usage: " << name << " <ibmf-file1> <ibmf-file2>" << std::endl;
  exit(1);
}

auto header(char *name1, char *name2) -> void {
  std::cout << "IBMF Differences:" << std::endl
            << "< " << name1 << std::endl
            << "> " << name2 << std::endl;
}

auto prepareFont(char *filename) -> IBMFFontDiffPtr {

  BufferPtr buffer;
  FILE     *f;

  if ((f = fopen(filename, "rb")) == nullptr) {
    std::cerr << "Unable to open file " << filename << std::endl;
    exit(1);
  }

  fseek(f, 0, SEEK_END);
  uint32_t len = ftell(f);
  fseek(f, 0, SEEK_SET);

  buffer = BufferPtr(new uint8_t[len]);
  if (fread(buffer.get(), 1, len, f) != len) {
    std::cerr << "Unable to read file content: " << filename << std::endl;
    exit(1);
  }

  fclose(f);

  auto font = IBMFFontDiffPtr(new IBMFFontDiff(buffer.get(), len));
  if ((font.get() == nullptr) || !font->isInitialized() ||
      (font->getPreamble().bits.fontFormat != FontFormat::UTF32)) {
    std::cerr << "File " << filename << " is not of an appropriate IBMF format." << std::endl;
    exit(1);
  }

  return font;
}

void checkPreamble() {
  if (font1->getPreamble().faceCount != font2->getPreamble().faceCount) {
    std::cout << std::endl
              << "FaceCount differ:" << std::endl
              << "< " << font1->getPreamble().faceCount << "> " << font2->getPreamble().faceCount
              << std::endl;
    diffCount += 1;
  }
}

void checkFaceHeaders() {
  int faceIdx1, faceIdx2;

  for (faceIdx1 = 0; faceIdx1 < font1->getPreamble().faceCount; faceIdx1++) {
    IBMFFontDiff::FacePtr face1 = font1->getFace(faceIdx1);
    IBMFFontDiff::FacePtr face2 = font2->findFace(face1->header->pointSize);
    if (face2 == nullptr) {
      std::cout << std::endl << "----- Face not found:" << std::endl;
      std::cout << "> Face with pointSize " << +face1->header->pointSize << std::endl;
      diffCount += 1;
    } else {
      if (!(*face1->header == *face2->header)) {
        std::cout << std::endl
                  << "----- Face headers with pointSize " << +face1->header->pointSize
                  << " differ:" << std::endl;
        font1->showFaceHeader(std::cout, '<', face1);
        font2->showFaceHeader(std::cout, '>', face2);
        diffCount += 1;
      }
    }
  }

  for (faceIdx2 = 0; faceIdx2 < font1->getPreamble().faceCount; faceIdx2++) {
    IBMFFontDiff::FacePtr face2 = font2->getFace(faceIdx2);
    IBMFFontDiff::FacePtr face1 = font1->findFace(face2->header->pointSize);
    if (face1 == nullptr) {
      std::cout << std::endl << "----- Face not found:" << std::endl;
      std::cout << "< Face with pointSize " << +face2->header->pointSize << std::endl;
      diffCount += 1;
    }
  }
}

#define CODEPOINT(c) "U+" << std::hex << std::setw(5) << std::setfill('0') << +c << std::dec

auto checkGlyphs() {

  int faceIdx1, faceIdx2;

  for (faceIdx1 = 0; faceIdx1 < font1->getPreamble().faceCount; faceIdx1++) {

    IBMFFontDiff::FacePtr face1 = font1->getFace(faceIdx1);
    IBMFFontDiff::FacePtr face2 = font2->findFace(face1->header->pointSize);

    if (face2 != nullptr) {
      GlyphCode code1, code2;
      for (code1 = 0; code1 < face1->header->glyphCount; code1++) {
        char16_t codePoint = font1->getUTF32(code1);
        code2              = font2->translate(codePoint);
        if ((code2 != NO_GLYPH_CODE) && (code2 != SPACE_CODE)) {
          if (!(*face1->glyphs[code1] == *face2->glyphs[code2])) {
            std::cout << std::endl
                      << "----- Glyph Metrics differ for codePoint " << CODEPOINT(codePoint)
                      << " of pointSize " << +face1->header->pointSize << std::endl;
            font1->showGlyphInfo(std::cout, '<', code1, face1->glyphs[code1]);
            font2->showGlyphInfo(std::cout, '>', code2, face2->glyphs[code2]);
            diffCount += 1;
          }
          if (!(*face1->bitmaps[code1] == *face2->bitmaps[code2])) {
            std::cout << std::endl
                      << "----- Glyph Pixels differ for codePoint " << CODEPOINT(codePoint)
                      << " of pointSize " << +face1->header->pointSize << std::endl;
            font1->showBitmap(std::cout, '<', face1->bitmaps[code1]);
            std::cout << std::endl;
            font2->showBitmap(std::cout, '>', face2->bitmaps[code2]);
            diffCount += 1;
          }
          if (!(*face1->glyphsLigKern[code1] == *face2->glyphsLigKern[code2])) {
            std::cout << std::endl
                      << "----- Glyph Ligature/Kerning differ for codePoint "
                      << CODEPOINT(codePoint) << " of pointSize " << +face1->header->pointSize
                      << std::endl;
            font1->showLigKerns(std::cout, '<', face1->glyphsLigKern[code1]);
            std::cout << std::endl;
            font2->showLigKerns(std::cout, '>', face2->glyphsLigKern[code2]);
            diffCount += 1;
          }
        } else {
          std::cout << std::endl
                    << "----- Face with pointSize " << +face1->header->pointSize << std::endl;
          std::cout << "> CodePoint not found: " << CODEPOINT(codePoint) << std::endl;
          diffCount += 1;
        }
      }

      for (code2 = 0; code2 < face2->header->glyphCount; code2++) {
        char16_t codePoint = font2->getUTF32(code2);
        code1              = font1->translate(codePoint);
        if ((code1 == NO_GLYPH_CODE) || (code1 == SPACE_CODE)) {
          std::cout << std::endl
                    << "----- Face with pointSize " << +face1->header->pointSize << std::endl;
          std::cout << "< CodePoint not found: " << CODEPOINT(codePoint) << std::endl;
          diffCount += 1;
        }
      }
    }
  }
}

auto main(int argc, char **argv) -> int {

  if (argc != 3) {
    usage(argv[0]);
  }

  font1     = prepareFont(argv[1]);
  font2     = prepareFont(argv[2]);

  diffCount = 0;

  header(argv[1], argv[2]);
  checkPreamble();
  checkFaceHeaders();
  checkGlyphs();

  std::cout << std::endl
            << "-----" << std::endl
            << "Completed. Number of differences found: " << diffCount << "." << std::endl;
}

auto formatStr(const std::string &format, ...) -> char * {

  static char buffer[256];
  va_list     args;
  va_start(args, format);

  if (std::vsnprintf(buffer, 256, format.c_str(), args) > 256) {
    std::cout << "Internal error!!" << std::endl;
  }

  va_end(args);
  return buffer;
}