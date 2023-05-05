## IBMF Diff Tool

Usage: ibmf-diff <ibmf-file1> <ibmf-file2>

This tool compares ibmf files for differences. The differences will be shown in the standard output.

Here is an example of running the tool:

```
$ .pio/build/stable-linux/program test_fonts/test.ibmf test_fonts/test2.ibmf 
IBMF Differences:
< test_fonts/test.ibmf
> test_fonts/test2.ibmf

----- Face headers with pointSize 14 differ:
< DPI: 75, point siz: 14, linHght: 20, xHght: 8, emSiz: 15, spcSiz: 4, glyphCnt: 94, LKCnt: 0, PixPoolSiz: 696, slantCorr: 0, descHght: 5
> DPI: 75, point siz: 14, linHght: 20, xHght: 8, emSiz: 15, spcSiz: 4, glyphCnt: 95, LKCnt: 1, PixPoolSiz: 700, slantCorr: 0, descHght: 5

----- Glyph Metrics differ for codePoint U+00021
< [0]: codePoint: U+00021, w: 2, h: 10, hoff: -1, voff: 10, pktLen: 3, adv: 4, dynF: 13, 1stBlack: 1, lKPgmIdx: 255
> [0]: codePoint: U+00021, w: 2, h: 10, hoff: -1, voff: 10, pktLen: 2, adv: 4, dynF: 12, 1stBlack: 1, lKPgmIdx: 255

----- Glyph Pixels differ for codePoint U+00021
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

----- Glyph Metrics differ for codePoint U+00022
< [1]: codePoint: U+00022, w: 4, h: 3, hoff: -1, voff: 10, pktLen: 2, adv: 6, dynF: 14, 1stBlack: 1, lKPgmIdx: 255
> [1]: codePoint: U+00022, w: 5, h: 3, hoff: -1, voff: 10, pktLen: 2, adv: 7, dynF: 14, 1stBlack: 1, lKPgmIdx: 255

----- Glyph Pixels differ for codePoint U+00022
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

----- Glyph Metrics differ for codePoint U+00023
< [2]: codePoint: U+00023, w: 9, h: 10, hoff: 0, voff: 10, pktLen: 12, adv: 9, dynF: 14, 1stBlack: 0, lKPgmIdx: 255
> [2]: codePoint: U+00023, w: 9, h: 10, hoff: 0, voff: 10, pktLen: 12, adv: 9, dynF: 14, 1stBlack: 0, lKPgmIdx: 0

----- Glyph Ligature/Kerning differ for codePoint U+00023

> [0]: NxtGlyphCode: 40, Kern: 2

----- Face with pointSize 
< CodePoint not found: U+0e000

-----
Completed. Number of differences found: 8.
```