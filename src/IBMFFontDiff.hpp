#pragma once

#include <cstdlib>
#include <cstring>
#include <iostream>
#include <set>
#include <vector>

#include "IBMFDefs.hpp"

using namespace IBMFDefs;

#include "RLEExtractor.hpp"

#define DEBUG 0

#if DEBUG
  #include <iomanip>
  #include <iostream>
#endif

class IBMFFontDiff;

typedef std::shared_ptr<IBMFFontDiff> IBMFFontDiffPtr;

/**
 * @brief Access to a IBMF font.
 *
 * This is a class to allow for the modification of a IBMF font generated from
 * METAFONT
 *
 */
class IBMFFontDiff {
public:
  struct Face {
    FaceHeaderPtr                header;
    std::vector<GlyphInfoPtr>    glyphs; // Not used with BAKCUP format
    std::vector<BitmapPtr>       bitmaps;
    std::vector<GlyphLigKernPtr> glyphsLigKern; // Specific to each glyph
    // used ontly at save and load time
    std::vector<RLEBitmapPtr> compressedBitmaps; // Todo: maybe unused at the end

    // used only at save time
    std::vector<LigKernStep> ligKernSteps; // The complete list of lig/kerns

    // Only used with BACKUP format
    std::vector<BackupGlyphInfoPtr>    backupGlyphs;
    std::vector<BackupGlyphLigKernPtr> backupGlyphsLigKern;
  };

  typedef std::shared_ptr<Face> FacePtr;

  IBMFFontDiff(uint8_t *memoryFont, uint32_t size) : memory_(memoryFont), memoryLength_(size) {
    initialized_ = load();
    lastError_   = 0;
  }

  ~IBMFFontDiff() { clear(); }

  auto clear() -> void;

  inline auto getPreamble() const -> Preamble { return preamble_; }
  inline auto getFace(int faceIdx) const -> FacePtr { return faces_[faceIdx]; }
  inline auto getFontFormat() const -> FontFormat { return preamble_.bits.fontFormat; }
  inline auto isInitialized() const -> bool { return initialized_; }
  inline auto getLastError() const -> int { return lastError_; }
  inline auto getLineHeight(int faceIdx) const -> int {
    return ((faceIdx >= 0) && (faceIdx < preamble_.faceCount)) ? faces_[faceIdx]->header->lineHeight
                                                               : 0;
  }

  inline auto getFaceHeader(int faceIdx) const -> const FaceHeaderPtr {
    return ((faceIdx >= 0) && (faceIdx < preamble_.faceCount)) ? faces_[faceIdx]->header : nullptr;
  }

  inline auto characterCodes() const -> const CharCodes * {
    CharCodes *chCodes = new CharCodes;
    for (GlyphCode i = 0; i < faces_[0]->header->glyphCount; i++) {
      char32_t ch = getUTF32(i);
      chCodes->push_back(ch);
    }
    return chCodes;
  }

  auto findFace(uint8_t pointSize) -> FacePtr;
  auto findGlyphIndex(FacePtr face, char32_t codePoint) const -> int;
  auto ligKern(int faceIndex, const GlyphCode glyphCode1, GlyphCode *glyphCode2, FIX16 *kern,
               bool *kernPairPresent, GlyphLigKernPtr bypassLigKern = nullptr) const -> bool;
  auto getGlyph(int faceIndex, int glyphCode, GlyphInfoPtr &glyphInfo, BitmapPtr &bitmap,
                GlyphLigKernPtr &glyphLigKern) const -> bool;

  auto convertToOneBit(const Bitmap &bitmapHeightBits, BitmapPtr *bitmapOneBit) -> bool;
  auto translate(char32_t codePoint) const -> GlyphCode;
  auto getUTF32(GlyphCode glyphCode) const -> char32_t;
  auto toGlyphCode(char32_t codePoint) const -> GlyphCode;

  auto showBitmap(std::ostream &stream, char first, const BitmapPtr bitmap) const -> void;
  auto showLigKerns(std::ostream &stream, char first, GlyphLigKernPtr lk) const -> void;
  auto showGlyphInfo(std::ostream &stream, char first, GlyphCode i, const GlyphInfoPtr g) const
      -> void;
  auto showFaceHeader(std::ostream &stream, char first, FacePtr face) const -> void;
  auto showCodePointBundles(std::ostream &stream, char first, int firstIdx, int count) const
      -> void;
  auto showPlanes(std::ostream &stream, char first) const -> void;

  auto glyphIsModified(int faceIdx, GlyphCode glyphCode, BitmapPtr &bitmap, GlyphInfoPtr &glyphInfo,
                       GlyphLigKernPtr &ligKern) const -> bool;

protected:
  static constexpr uint8_t IBMF_VERSION = 4;

  Preamble preamble_;

  std::vector<Plane>           planes_;
  std::vector<CodePointBundle> codePointBundles_;
  std::vector<FacePtr>         faces_;

private:
  bool initialized_;

  std::vector<uint32_t> faceOffsets_;

  uint8_t *memory_;
  uint32_t memoryLength_;

  int lastError_;

  auto findList(std::vector<LigKernStep> &pgm, std::vector<LigKernStep> &list) const -> int;
  auto prepareLigKernVectors() -> bool;
  auto load() -> bool;
};
