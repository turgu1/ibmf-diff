## IBMF Diff Tool

Usage: ibmf-diff <ibmf-file1> <ibmf-file2>

This tool compares ibmf files for differences. The differences will be shown in the standard output.

Here is an example of running the tool:

```
$ .pio/build/stable/program test_fonts/test.ibmf test_fonts/test2.ibmf 
IBMF Differences:
< test_fonts/test.ibmf
> test_fonts/test2.ibmf

----- Face headers with pointSize 14 differ:
< DPI: 75, point siz: 14, linHght: 20, xHght: 8, emSiz: 15, spcSiz: 4, glyphCnt: 94, LKCnt: 0, PixPoolSiz: 696, slantCorr: 0, descHght: 5
> DPI: 75, point siz: 14, linHght: 20, xHght: 8, emSiz: 15, spcSiz: 4, glyphCnt: 95, LKCnt: 1, PixPoolSiz: 700, slantCorr: 0, descHght: 5

----- Glyph Metrics differ for codePoint U+00021 of pointSize 14
< [0]: codePoint: U+00021, pixWdth: 2, pixHght: 10, hOff: -1, vOff: 10, pixSiz: 3, adv: 4, dynF: 13, 1stBlack: 1, beforeOptKrn: 0, afterOptKrn: 0, ligKrnPgmIdx: 255
> [0]: codePoint: U+00021, pixWdth: 2, pixHght: 10, hOff: -1, vOff: 10, pixSiz: 2, adv: 4, dynF: 12, 1stBlack: 1, beforeOptKrn: 0, afterOptKrn: 0, ligKrnPgmIdx: 255

----- Glyph Pixels differ for codePoint U+00021 of pointSize 14
< +--+
< |XX|
< |XX|
< |XX|
< |XX|
< |X |
< |X |
< |X |
< |  |
< |XX|
< |XX|
< +--+

> +--+
> |XX|
> |XX|
> |XX|
> |XX|
> |XX|
> |XX|
> |XX|
> |  |
> |XX|
> |XX|
> +--+

----- Glyph Metrics differ for codePoint U+00022 of pointSize 14
< [1]: codePoint: U+00022, pixWdth: 4, pixHght: 3, hOff: -1, vOff: 10, pixSiz: 2, adv: 6, dynF: 14, 1stBlack: 1, beforeOptKrn: 0, afterOptKrn: 0, ligKrnPgmIdx: 255
> [1]: codePoint: U+00022, pixWdth: 5, pixHght: 3, hOff: -1, vOff: 10, pixSiz: 2, adv: 7, dynF: 14, 1stBlack: 1, beforeOptKrn: 0, afterOptKrn: 0, ligKrnPgmIdx: 255

----- Glyph Pixels differ for codePoint U+00022 of pointSize 14
< +----+
< |X  X|
< |X  X|
< |X  X|
< +----+

> +-----+
> |X  XX|
> |X  XX|
> |X  XX|
> +-----+

----- Glyph Metrics differ for codePoint U+00023 of pointSize 14
< [2]: codePoint: U+00023, pixWdth: 9, pixHght: 10, hOff: 0, vOff: 10, pixSiz: 12, adv: 9, dynF: 14, 1stBlack: 0, beforeOptKrn: 0, afterOptKrn: 0, ligKrnPgmIdx: 255
> [2]: codePoint: U+00023, pixWdth: 9, pixHght: 10, hOff: 0, vOff: 10, pixSiz: 12, adv: 9, dynF: 14, 1stBlack: 0, beforeOptKrn: 2, afterOptKrn: 0, ligKrnPgmIdx: 0

----- Glyph Ligature/Kerning differ for codePoint U+00023 of pointSize 14
< None

> [0]: NxtGlyphCode: 40(U+00049), Kern: 2

----- Face with pointSize 14
< CodePoint not found: U+0e000

-----
Completed. Number of differences found: 8.
```