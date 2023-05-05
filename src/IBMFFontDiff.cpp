#include "IBMFFontDiff.hpp"

#include <algorithm>
#include <iomanip>
#include <iostream>

void IBMFFontDiff::clear() {
  initialized_ = false;
  for (auto &face : faces_) {
    for (auto bitmap : face->bitmaps) {
      bitmap->clear();
    }
    for (auto bitmap : face->compressedBitmaps) {
      bitmap->clear();
    }
    face->glyphs.clear();
    face->backupGlyphs.clear();
    face->bitmaps.clear();
    face->compressedBitmaps.clear();
    face->glyphsLigKern.clear();
    face->ligKernSteps.clear();
  }
  faces_.clear();
  faceOffsets_.clear();
  planes_.clear();
  codePointBundles_.clear();
}

bool IBMFFontDiff::load() {
  // Preamble retrieval
  memcpy(&preamble_, memory_, sizeof(Preamble));
  if (strncmp("IBMF", preamble_.marker, 4) != 0) return false;
  if (preamble_.bits.version != IBMF_VERSION) return false;

  int idx = ((sizeof(Preamble) + preamble_.faceCount + 3) & 0xFFFFFFFC);

  // Faces offset retrieval
  for (int i = 0; i < preamble_.faceCount; i++) {
    uint32_t offset = *((uint32_t *)&memory_[idx]);
    faceOffsets_.push_back(offset);
    idx += 4;
  }

  // Unicode CodePoint Table retrieval
  if (preamble_.bits.fontFormat == FontFormat::UTF32) {
    PlanesPtr planes      = reinterpret_cast<PlanesPtr>(&memory_[idx]);
    int       bundleCount = 0;
    for (int i = 0; i < 4; i++) {
      planes_.push_back((*planes)[i]);
      bundleCount += (*planes)[i].entriesCount;
    }
    idx += sizeof(Planes);

    CodePointBundlesPtr codePointBundles = reinterpret_cast<CodePointBundlesPtr>(&memory_[idx]);
    for (int i = 0; i < bundleCount; i++) {
      codePointBundles_.push_back((*codePointBundles)[i]);
    }
    idx +=
        (((*planes)[3].codePointBundlesIdx + (*planes)[3].entriesCount) * sizeof(CodePointBundle));
  } else {
    planes_.clear();
    codePointBundles_.clear();
  }

  // Faces retrieval
  for (int i = 0; i < preamble_.faceCount; i++) {
    if (idx != faceOffsets_[i]) return false;

    // Face Header
    FacePtr                       face   = FacePtr(new Face);
    FaceHeaderPtr                 header = FaceHeaderPtr(new FaceHeader);
    GlyphsPixelPoolIndexesTempPtr glyphsPixelPoolIndexes;
    PixelsPoolTempPtr             pixelsPool;

    memcpy(header.get(), &memory_[idx], sizeof(FaceHeader));
    idx += sizeof(FaceHeader);

    // Glyphs RLE bitmaps indexes in the bitmaps pool
    glyphsPixelPoolIndexes = reinterpret_cast<GlyphsPixelPoolIndexesTempPtr>(&memory_[idx]);
    idx += (sizeof(PixelPoolIndex) * header->glyphCount);

    // Glyphs info and bitmaps

    if (preamble_.bits.fontFormat == FontFormat::BACKUP) {
      pixelsPool = reinterpret_cast<PixelsPoolTempPtr>(
          &memory_[idx + (sizeof(BackupGlyphInfo) * header->glyphCount)]);

      face->backupGlyphs.reserve(header->glyphCount);

      for (int glyphCode = 0; glyphCode < header->glyphCount; glyphCode++) {
        BackupGlyphInfoPtr backupGlyphInfo = BackupGlyphInfoPtr(new BackupGlyphInfo);
        memcpy(backupGlyphInfo.get(), &memory_[idx], sizeof(BackupGlyphInfo));
        idx += sizeof(BackupGlyphInfo);

        int       bitmap_size = backupGlyphInfo->bitmapHeight * backupGlyphInfo->bitmapWidth;
        BitmapPtr bitmap      = BitmapPtr(new Bitmap);
        bitmap->pixels        = Pixels(bitmap_size, 0);
        bitmap->dim           = Dim(backupGlyphInfo->bitmapWidth, backupGlyphInfo->bitmapHeight);

        RLEBitmapPtr compressedBitmap = RLEBitmapPtr(new RLEBitmap);
        compressedBitmap->dim         = bitmap->dim;
        compressedBitmap->pixels.reserve(backupGlyphInfo->packetLength);
        compressedBitmap->length = backupGlyphInfo->packetLength;
        for (int pos = 0; pos < backupGlyphInfo->packetLength; pos++) {
          compressedBitmap->pixels.push_back(
              (*pixelsPool)[pos + (*glyphsPixelPoolIndexes)[glyphCode]]);
        }

        RLEExtractor rle;
        rle.retrieveBitmap(*compressedBitmap, *bitmap, Pos(0, 0), backupGlyphInfo->rleMetrics);
        // retrieveBitmap(idx, glyphInfo.get(), *bitmap, Pos(0,0));

        face->backupGlyphs.push_back(backupGlyphInfo);
        face->bitmaps.push_back(bitmap);
        face->compressedBitmaps.push_back(compressedBitmap);

        // idx += glyphInfo->packetLength;
      }
    } else {
      pixelsPool = reinterpret_cast<PixelsPoolTempPtr>(
          &memory_[idx + (sizeof(GlyphInfo) * header->glyphCount)]);

      face->glyphs.reserve(header->glyphCount);

      for (int glyphCode = 0; glyphCode < header->glyphCount; glyphCode++) {
        GlyphInfoPtr glyphInfo = GlyphInfoPtr(new GlyphInfo);
        memcpy(glyphInfo.get(), &memory_[idx], sizeof(GlyphInfo));
        idx += sizeof(GlyphInfo);

        int       bitmap_size         = glyphInfo->bitmapHeight * glyphInfo->bitmapWidth;
        BitmapPtr bitmap              = BitmapPtr(new Bitmap);
        bitmap->pixels                = Pixels(bitmap_size, 0);
        bitmap->dim                   = Dim(glyphInfo->bitmapWidth, glyphInfo->bitmapHeight);

        RLEBitmapPtr compressedBitmap = RLEBitmapPtr(new RLEBitmap);
        compressedBitmap->dim         = bitmap->dim;
        compressedBitmap->pixels.reserve(glyphInfo->packetLength);
        compressedBitmap->length = glyphInfo->packetLength;
        for (int pos = 0; pos < glyphInfo->packetLength; pos++) {
          compressedBitmap->pixels.push_back(
              (*pixelsPool)[pos + (*glyphsPixelPoolIndexes)[glyphCode]]);
        }

        RLEExtractor rle;
        rle.retrieveBitmap(*compressedBitmap, *bitmap, Pos(0, 0), glyphInfo->rleMetrics);
        // retrieveBitmap(idx, glyphInfo.get(), *bitmap, Pos(0,0));

        face->glyphs.push_back(glyphInfo);
        face->bitmaps.push_back(bitmap);
        face->compressedBitmaps.push_back(compressedBitmap);

        // idx += glyphInfo->packetLength;
      }
    }

    if (&memory_[idx] != (uint8_t *)pixelsPool) {
      return false;
    }

    idx += header->pixelsPoolSize;

    if (preamble_.bits.fontFormat == FontFormat::BACKUP) {
      for (GlyphCode glyphCode = 0; glyphCode < header->glyphCount; glyphCode++) {
        BackupGlyphLigKernPtr glk = BackupGlyphLigKernPtr(new BackupGlyphLigKern);

        for (int i = 0; i < face->backupGlyphs[glyphCode]->ligCount; i++) {
          BackupGlyphLigStep l;
          memcpy(&l, &memory_[idx], sizeof(BackupGlyphLigStep));
          idx += sizeof(BackupGlyphLigStep);
          glk->ligSteps.push_back(l);
        }
        for (int i = 0; i < face->backupGlyphs[glyphCode]->kernCount; i++) {
          BackupGlyphKernStep k;
          memcpy(&k, &memory_[idx], sizeof(BackupGlyphKernStep));
          idx += sizeof(BackupGlyphKernStep);
          glk->kernSteps.push_back(k);
        }
        face->backupGlyphsLigKern.push_back(glk);
      }

      face->header = header;
      faces_.push_back(std::move(face));
    } else {
      if (header->ligKernStepCount > 0) {
        face->ligKernSteps.reserve(header->ligKernStepCount);
        for (int j = 0; j < header->ligKernStepCount; j++) {
          LigKernStep step;
          memcpy(&step, &memory_[idx], sizeof(LigKernStep));

          face->ligKernSteps.push_back(step);

          idx += sizeof(LigKernStep);
        }
      }

      for (GlyphCode glyphCode = 0; glyphCode < header->glyphCount; glyphCode++) {
        GlyphLigKernPtr glk = GlyphLigKernPtr(new GlyphLigKern);

        if (face->glyphs[glyphCode]->ligKernPgmIndex != 255) {
          int lk_idx = face->glyphs[glyphCode]->ligKernPgmIndex;
          if (lk_idx < header->ligKernStepCount) {
            if ((face->ligKernSteps[lk_idx].b.goTo.isAGoTo) &&
                (face->ligKernSteps[lk_idx].b.kern.isAKern)) {
              lk_idx = face->ligKernSteps[lk_idx].b.goTo.displacement;
            }
            do {
              if (face->ligKernSteps[lk_idx].b.kern.isAKern) { // true = kern, false = ligature
                glk->kernSteps.push_back(
                    GlyphKernStep{.nextGlyphCode = face->ligKernSteps[lk_idx].a.data.nextGlyphCode,
                                  .kern          = face->ligKernSteps[lk_idx].b.kern.kerningValue});
              } else {
                glk->ligSteps.push_back(GlyphLigStep{
                    .nextGlyphCode        = face->ligKernSteps[lk_idx].a.data.nextGlyphCode,
                    .replacementGlyphCode = face->ligKernSteps[lk_idx].b.repl.replGlyphCode});
              }
            } while (!face->ligKernSteps[lk_idx++].a.data.stop);
          }
        }
        face->glyphsLigKern.push_back(glk);
      }

      face->header = header;
      faces_.push_back(std::move(face));
    }
  }

  return true;
}

auto IBMFFontDiff::findFace(uint8_t pointSize) -> FacePtr {

  for (auto &face : faces_) {
    if (face->header->pointSize == pointSize) return face;
  }
  return nullptr;
}

auto IBMFFontDiff::findGlyphIndex(FacePtr face, char32_t codePoint) const -> int {

  int idx = 0;
  for (auto &glyph : face->backupGlyphs) {
    if (glyph->codePoint == codePoint) return idx;
    idx++;
  }
  return -1;
}

/// @brief Search Ligature and Kerning table
///
/// Using the LigKern program of **glyphCode1**, find the first entry in the
/// program for which **glyphCode2** is the next character. If a ligature is
/// found, sets **glyphCode2** with the new code and returns *true*. If a
/// kerning entry is found, it sets the kern parameter with the value
/// in the table and return *false*. If the LigKern pgm is empty or there
/// is no entry for **glyphCode2**, it returns *false*.
///
/// Note: character codes have to be translated to internal GlyphCode before
/// calling this method.
///
/// @param glyphCode1 In. The GlyhCode for which to find a LigKern entry in its program.
/// @param glyphCode2 InOut. The GlyphCode that must appear in the program as the next
///                   character in sequence. Will be replaced with the target
///                   ligature GlyphCode if found.
/// @param kern Out. When a kerning entry is found in the program, kern will receive the value.
/// @param kernFound Out. True if a kerning pair was found.
/// @return True if a ligature was found, false otherwise.
///
auto IBMFFontDiff::ligKern(int faceIndex, const GlyphCode glyphCode1, GlyphCode *glyphCode2,
                           FIX16 *kern, bool *kernPairPresent, GlyphLigKernPtr bypassLigKern) const
    -> bool {

  *kern            = 0;
  *kernPairPresent = false;

  if ((faceIndex >= preamble_.faceCount) || (glyphCode1 < 0) ||
      (glyphCode1 >= faces_[faceIndex]->header->glyphCount) || (*glyphCode2 < 0) ||
      (*glyphCode2 >= faces_[faceIndex]->header->glyphCount)) {
    return false;
  }

  //
  GlyphLigSteps  *ligSteps;
  GlyphKernSteps *kernSteps;

  if (bypassLigKern == nullptr) {
    ligSteps  = &faces_[faceIndex]->glyphsLigKern[glyphCode1]->ligSteps;
    kernSteps = &faces_[faceIndex]->glyphsLigKern[glyphCode1]->kernSteps;
  } else {
    ligSteps  = &bypassLigKern->ligSteps;
    kernSteps = &bypassLigKern->kernSteps;
  }

  if ((ligSteps->size() == 0) && (kernSteps->size() == 0)) {
    return false;
  }

  GlyphCode code = faces_[faceIndex]->glyphs[*glyphCode2]->mainCode;
  if (preamble_.bits.fontFormat == FontFormat::LATIN) {
    code &= LATIN_GLYPH_CODE_MASK;
  }
  bool first = true;

  for (auto &ligStep : *ligSteps) {
    if (ligStep.nextGlyphCode == *glyphCode2) {
      *glyphCode2 = ligStep.replacementGlyphCode;
      return true;
    }
  }

  for (auto &kernStep : *kernSteps) {
    if (kernStep.nextGlyphCode == code) {
      FIX16 k = kernStep.kern;
      if (k & 0x2000) k |= 0xC000;
      *kern            = k;
      *kernPairPresent = true;
      return false;
    }
  }
  return false;
}

auto IBMFFontDiff::getGlyph(int faceIndex, int glyphCode, GlyphInfoPtr &glyphInfo,
                            BitmapPtr &bitmap, GlyphLigKernPtr &glyphLigKern) const -> bool {

  if ((faceIndex >= preamble_.faceCount) || (glyphCode < 0) ||
      (glyphCode >= faces_[faceIndex]->header->glyphCount)) {
    return false;
  }

  int glyphIndex = glyphCode;

  glyphInfo      = std::make_shared<GlyphInfo>(*faces_[faceIndex]->glyphs[glyphIndex]);
  bitmap         = std::make_shared<Bitmap>(*faces_[faceIndex]->bitmaps[glyphIndex]);
  glyphLigKern   = std::make_shared<GlyphLigKern>(*faces_[faceIndex]->glyphsLigKern[glyphCode]);

  return true;
}

auto IBMFFontDiff::convertToOneBit(const Bitmap &bitmapHeightBits, BitmapPtr *bitmapOneBit)
    -> bool {
  *bitmapOneBit        = BitmapPtr(new Bitmap);
  (*bitmapOneBit)->dim = bitmapHeightBits.dim;
  auto pix             = &(*bitmapOneBit)->pixels;
  for (int row = 0, idx = 0; row < bitmapHeightBits.dim.height; row++) {
    uint8_t data = 0;
    uint8_t mask = 0x80;
    for (int col = 0; col < bitmapHeightBits.dim.width; col++, idx++) {
      data |= bitmapHeightBits.pixels[idx] == 0 ? 0 : mask;
      mask >>= 1;
      if (mask == 0) {
        pix->push_back(data);
        data = 0;
        mask = 0x80;
      }
    }
    if (mask != 0) pix->push_back(data);
  }
  return pix->size() == (bitmapHeightBits.dim.height * ((bitmapHeightBits.dim.width + 7) >> 3));
}

// In the process of optimizing the size of the ligKern table, this method
// search to find if a part of the already prepared list contains the same
// steps as per the pgm received as a parameter. If so, the index of the
// similar list of steps is returned, else -1
auto IBMFFontDiff::findList(std::vector<LigKernStep> &pgm, std::vector<LigKernStep> &list) const
    -> int {

  auto pred = [](const LigKernStep &e1, const LigKernStep &e2) -> bool {
    return (e1.a.whole.val == e2.a.whole.val) && (e1.b.whole.val == e2.b.whole.val);
  };

  auto it = std::search(list.begin(), list.end(), pgm.begin(), pgm.end(), pred);
  return (it == list.end()) ? -1 : std::distance(list.begin(), it);
}

auto IBMFFontDiff::toGlyphCode(char32_t codePoint) const -> GlyphCode {

  GlyphCode glyphCode = NO_GLYPH_CODE;

  uint16_t planeIdx   = static_cast<uint16_t>(codePoint >> 16);

  if (planeIdx <= 3) {
    char16_t u16                = static_cast<char16_t>(codePoint);

    uint16_t codePointBundleIdx = planes_[planeIdx].codePointBundlesIdx;
    uint16_t entriesCount       = planes_[planeIdx].entriesCount;
    int      gCode              = planes_[planeIdx].firstGlyphCode;
    int      i                  = 0;
    while (i < entriesCount) {
      if (u16 <= codePointBundles_[codePointBundleIdx].lastCodePoint) {
        break;
      }
      gCode += (codePointBundles_[codePointBundleIdx].lastCodePoint -
                codePointBundles_[codePointBundleIdx].firstCodePoint + 1);
      i++;
      codePointBundleIdx++;
    }
    if ((i < entriesCount) && (u16 >= codePointBundles_[codePointBundleIdx].firstCodePoint)) {
      glyphCode = gCode + u16 - codePointBundles_[codePointBundleIdx].firstCodePoint;
    }
  }

  return glyphCode;
}

/**
 * @brief Translate UTF32 codePoint to it's internal representation
 *
 * All supported character sets must use this method to get the internal
 * glyph code correponding to the CodePoint. For the LATIN font format,
 * the glyph code contains both the glyph index and accented information. For
 * UTF32 font format, it contains the index of the glyph in all faces that are
 * part of the font.
 *
 * FontFormat 0 - Latin Char Set Translation
 * -----------------------------------------
 *
 * The class allow for latin1+ CodePoints to be plotted into a bitmap. As the
 * font doesn't contains all accented glyphs, a combination of glyphs must be
 * used to draw a single bitmap. This method translate some of the supported
 * CodePoint values to that combination. The least significant byte will contains
 * the main glyph code index and the next byte will contain the accent code.
 *
 * The supported UTF16 CodePoints are the following:
 *
 *      U+0020 to U+007F
 *      U+00A1 to U+017f
 *
 *   and the following:
 *
 * |  UTF16  | Description          |
 * |:-------:|----------------------|
 * | U+02BB  | reverse apostrophe
 * | U+02BC  | apostrophe
 * | U+02C6  | circumflex
 * | U+02DA  | ring
 * | U+02DC  | tilde ~
 * | U+2013  | endash (Not available with CM Typewriter)
 * | U+2014  | emdash (Not available with CM Typewriter)
 * | U+2018  | quote left
 * | U+2019  | quote right
 * | U+201A  | comma like ,
 * | U+201C  | quoted left "
 * | U+201D  | quoted right
 * | U+2032  | minute '
 * | U+2033  | second "
 * | U+2044  | fraction /
 * | U+20AC  | euro
 *
 * Font Format 1 - UTF32 translation
 * ---------------------------------
 *
 * Using the Planes and CodePointBundles structure, retrieves the
 * glyph code corresponding to the CodePoint.
 *
 * @param codePoint The UTF32 character code
 * @return The internal representation of CodePoint
 */
auto IBMFFontDiff::translate(char32_t codePoint) const -> GlyphCode {
  GlyphCode glyphCode = SPACE_CODE;

  if (preamble_.bits.fontFormat == FontFormat::LATIN) {
    if ((codePoint > 0x20) && (codePoint < 0x7F)) {
      glyphCode = codePoint; // ASCII codes No accent
    } else if ((codePoint >= 0xA1) && (codePoint <= 0x1FF)) {
      glyphCode = latinTranslationSet[codePoint - 0xA1];
    } else {
      switch (codePoint) {
        case 0x2013: // endash
          glyphCode = 0x0015;
          break;
        case 0x2014: // emdash
          glyphCode = 0x0016;
          break;
        case 0x2018: // quote left
        case 0x02BB: // reverse apostrophe
          glyphCode = 0x0060;
          break;
        case 0x2019: // quote right
        case 0x02BC: // apostrophe
          glyphCode = 0x0027;
          break;
        case 0x201C: // quoted left "
          glyphCode = 0x0010;
          break;
        case 0x201D: // quoted right
          glyphCode = 0x0011;
          break;
        case 0x02C6: // circumflex
          glyphCode = 0x005E;
          break;
        case 0x02DA: // ring
          glyphCode = 0x0006;
          break;
        case 0x02DC: // tilde ~
          glyphCode = 0x007E;
          break;
        case 0x201A: // comma like ,
          glyphCode = 0x000D;
          break;
        case 0x2032: // minute '
          glyphCode = 0x0027;
          break;
        case 0x2033: // second "
          glyphCode = 0x0022;
          break;
        case 0x2044: // fraction /
          glyphCode = 0x002F;
          break;
        case 0x20AC: // euro
          glyphCode = 0x00AD;
          break;
      }
    }
  } else if (preamble_.bits.fontFormat == FontFormat::UTF32) {
    uint16_t planeIdx = static_cast<uint16_t>(codePoint >> 16);

    if (planeIdx <= 3) {
      char16_t u16                = static_cast<char16_t>(codePoint);

      uint16_t codePointBundleIdx = planes_[planeIdx].codePointBundlesIdx;
      uint16_t entriesCount       = planes_[planeIdx].entriesCount;
      int      gCode              = planes_[planeIdx].firstGlyphCode;
      int      i                  = 0;
      while (i < entriesCount) {
        if (u16 <= codePointBundles_[codePointBundleIdx].lastCodePoint) {
          break;
        }
        gCode += (codePointBundles_[codePointBundleIdx].lastCodePoint -
                  codePointBundles_[codePointBundleIdx].firstCodePoint + 1);
        i++;
        codePointBundleIdx++;
      }
      if ((i < entriesCount) && (u16 >= codePointBundles_[codePointBundleIdx].firstCodePoint)) {
        glyphCode = gCode + u16 - codePointBundles_[codePointBundleIdx].firstCodePoint;
      }
    }
  }

  return glyphCode;
}

// Returns the corresponding UTF32 character for the glyphCode.
auto IBMFFontDiff::getUTF32(GlyphCode glyphCode) const -> char32_t {
  char32_t codePoint = 0;
  if (preamble_.bits.fontFormat == FontFormat::UTF32) {
    int i = 0;
    while (i < 3) {
      if (planes_[i + 1].firstGlyphCode > glyphCode) break;
      i += 1;
    }
    if (i < 4) {
      int planeMask    = i << 16;
      int bundleIdx    = planes_[i].codePointBundlesIdx;
      int idx          = planes_[i].firstGlyphCode;
      int entriesCount = planes_[i].entriesCount;
      int j            = 0;
      int bundleSize   = codePointBundles_[bundleIdx].lastCodePoint -
                       codePointBundles_[bundleIdx].firstCodePoint + 1;

      while (j < entriesCount) {
        if ((idx + bundleSize) > glyphCode) break;
        idx += bundleSize;
        bundleIdx += 1;
        bundleSize = codePointBundles_[bundleIdx].lastCodePoint -
                     codePointBundles_[bundleIdx].firstCodePoint + 1;
        j += 1;
      }
      if (j < entriesCount) {
        codePoint = (codePointBundles_[bundleIdx].firstCodePoint + (glyphCode - idx)) | planeMask;
      }
    }
  } else {
    if (glyphCode < fontFormat0CodePoints.size()) {
      codePoint = fontFormat0CodePoints[glyphCode];
    }
  }
  return codePoint;
}

auto IBMFFontDiff::showBitmap(std::ostream &stream, char first, const BitmapPtr bitmap) const
    -> void {

  uint32_t  row, col;
  MemoryPtr rowPtr;

  uint32_t maxWidth = 50;
  if (bitmap->dim.width < maxWidth) {
    maxWidth = bitmap->dim.width;
  }

  stream << first << " "
         << "+";
  for (col = 0; col < maxWidth; col++) {
    stream << '-';
  }
  stream << '+' << std::endl;

  uint32_t rowSize = bitmap->dim.width;
  for (row = 0, rowPtr = bitmap->pixels.data(); row < bitmap->dim.height;
       row++, rowPtr += rowSize) {
    stream << first << " "
           << "|";
    for (col = 0; col < maxWidth; col++) {
      if constexpr (BLACK_EIGHT_BITS) {
        stream << ((rowPtr[col] == BLACK_EIGHT_BITS) ? 'X' : ' ');
      } else {
        stream << ((rowPtr[col] == BLACK_EIGHT_BITS) ? ' ' : 'X');
      }
    }
    stream << '|';
    stream << std::endl;
  }

  stream << first << " "
         << "+";
  for (col = 0; col < maxWidth; col++) {
    stream << '-';
  }
  stream << '+' << std::endl;
}

auto IBMFFontDiff::showGlyphInfo(std::ostream &stream, char first, GlyphCode i,
                                 const GlyphInfoPtr g) const -> void {
  stream << first << " "
         << "[" << i << "]: "
         << "codePoint: "
         << "U+" << std::hex << std::setw(5) << std::setfill('0') << getUTF32(i) << std::dec
         << ", w: " << +g->bitmapWidth << ", h: " << +g->bitmapHeight
         << ", hoff: " << +g->horizontalOffset << ", voff: " << +g->verticalOffset
         << ", pktLen: " << +g->packetLength << ", adv: " << +((float)g->advance / 64.0)
         << ", dynF: " << +g->rleMetrics.dynF << ", 1stBlack: " << +g->rleMetrics.firstIsBlack
         << ", lKPgmIdx: " << +g->ligKernPgmIndex;

  if (g->mainCode != i) {
    stream << ", mainCode: " << g->mainCode;
  }
  stream << std::endl;
}

auto IBMFFontDiff::showLigKerns(std::ostream &stream, char first, GlyphLigKernPtr lk) const
    -> void {

  if ((lk != nullptr) && ((lk->ligSteps.size() > 0) || (lk->kernSteps.size() > 0))) {
    uint16_t i = 0;
    for (auto &lig : lk->ligSteps) {
      stream << first << " "
             << "[" << i << "]: "
             << "NxtGlyphCode: " << +lig.nextGlyphCode << ", "
             << "LigCode: " << +lig.replacementGlyphCode << std::endl;
      i += 1;
    }

    for (auto &kern : lk->kernSteps) {
      stream << first << " "
             << "[" << i << "]: "
             << "NxtGlyphCode: " << +kern.nextGlyphCode << ", "
             << "Kern: " << (float)(kern.kern / 64.0) << std::endl;
      i += 1;
    }
  }
}

auto IBMFFontDiff::showFaceHeader(std::ostream &stream, char first, FacePtr face) const -> void {

  stream << first << " "
         << "DPI: " << face->header->dpi << ", point siz: " << +face->header->pointSize
         << ", linHght: " << +face->header->lineHeight
         << ", xHght: " << +((float)face->header->xHeight / 64.0)
         << ", emSiz: " << +((float)face->header->emSize / 64.0)
         << ", spcSiz: " << +face->header->spaceSize << ", glyphCnt: " << +face->header->glyphCount
         << ", LKCnt: " << +face->header->ligKernStepCount
         << ", PixPoolSiz: " << +face->header->pixelsPoolSize
         << ", slantCorr: " << +((float)face->header->slantCorrection / 64.0)
         << ", descHght: " << +face->header->descenderHeight << std::endl;
}

auto IBMFFontDiff::showCodePointBundles(std::ostream &stream, char first, int firstIdx,
                                        int count) const -> void {
  for (int idx = firstIdx; count > 0; idx++, count--) {
    stream << first << " "
           << "    [" << idx << "] "
           << "First CodePoint: " << +codePointBundles_[idx].firstCodePoint
           << ", Last CodePoint: " << +codePointBundles_[idx].lastCodePoint << std::endl;
  }
}

auto IBMFFontDiff::showPlanes(std::ostream &stream, char first) const -> void {
  stream << "----------- Planes -----------" << std::endl;
  for (int i = 0; i < 4; i++) {
    stream << first << " "
           << "[" << i << "] CodePoint Bundle Index: " << planes_[i].codePointBundlesIdx
           << ", Entries Count: " << planes_[i].entriesCount
           << ", First glyph code: " << planes_[i].firstGlyphCode << std::endl;
    if (planes_[i].entriesCount > 0) {
      stream << "    CodePoint Bundles:" << std::endl;
      showCodePointBundles(stream, first, planes_[i].codePointBundlesIdx, planes_[i].entriesCount);
    }
  }
}

auto IBMFFontDiff::glyphIsModified(int faceIdx, GlyphCode glyphCode, BitmapPtr &bitmap,
                                   GlyphInfoPtr &glyphInfo, GlyphLigKernPtr &ligKern) const
    -> bool {
  FacePtr face = faces_[faceIdx];

  return !((*face->glyphs[glyphCode] == *glyphInfo) && (*face->bitmaps[glyphCode] == *bitmap) &&
           (*face->glyphsLigKern[glyphCode] == *ligKern));
}
