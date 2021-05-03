/**
 * @file engine.cpp
 *
 * Implementation of basic engine helper functions:
 * - Sprite blitting
 * - Drawing
 * - Angle calculation
 * - RNG
 * - Memory allocation
 * - File loading
 * - Video playback
 */

#include "lighting.h"
#include "movie.h"
#include "options.h"
#include "storm/storm.h"

namespace devilution {

/** Seed value before the most recent call to SetRndSeed() */
int32_t orgseed;
/** Current game seed */
int32_t sglGameSeed;

/**
 * Specifies the increment used in the Borland C/C++ pseudo-random.
 */
const uint32_t RndInc = 1;

/**
 * Specifies the multiplier used in the Borland C/C++ pseudo-random number generator algorithm.
 */
const uint32_t RndMult = 0x015A4E35;

namespace {

/**
 * Cel frame data encoding bytes.
 * See https://github.com/savagesteel/d1-file-formats/blob/master/PC-Mac/CEL.md#42-cel-frame-data
 */

/** [0, 0x7E]: followed by this many pixels. Ends the line. */
constexpr std::uint8_t CelPixelsEolMax = 0x7E;

/** 0x7F: followed by 128 (0x7F) pixels. Does not end the line. */
constexpr std::uint8_t CelPixelsContinue = 0x7F;

/** 0x80: followed by 128 (256 - 0x80) pixels. Does not end the line. */
constexpr std::uint8_t CelTransparentContinue = 0x80;

/** [0x81, 0xFF]: followed by 256 - this many pixels. Ends the line. */
constexpr std::uint8_t CelTransparentEolMin = 0x81;

constexpr bool IsCelPixelsEol(std::uint8_t control)
{
	return control <= CelPixelsEolMax;
}

constexpr bool IsCelTransparent(std::uint8_t control)
{
	constexpr std::uint8_t CelTransparentMask = 0x80;
	return (control & CelTransparentMask) != 0;
}

constexpr bool IsCelTransparentEol(std::uint8_t control)
{
	return control != CelTransparentContinue;
}

constexpr std::uint8_t GetCelTransparentWidth(std::uint8_t control)
{
	return -static_cast<std::int8_t>(control);
}

constexpr std::uint8_t MaxCl2Width = 65;

BYTE *GetLightTable(char light)
{
	int idx = light4flag ? 1024 : 4096;
	if (light == 2)
		idx += 256; // gray colors
	if (light >= 4)
		idx += (light - 1) << 8;
	return &pLightTbl[idx];
}

} // namespace

CelSprite LoadCel(const char *pszName, int width)
{
	return CelSprite(LoadFileInMem<BYTE>(pszName), width);
}

CelSprite LoadCel(const char *pszName, const int *widths)
{
	return CelSprite(LoadFileInMem<BYTE>(pszName), widths);
}

std::pair<int, int> MeasureSolidHorizontalBounds(const CelSprite &cel, int frame)
{
	int nDataSize;
	const BYTE *src = CelGetFrame(cel.Data(), frame, &nDataSize);
	const BYTE *end = src + nDataSize;
	const int celWidth = cel.Width(frame);

	int xBegin = celWidth;
	int xEnd = 0;

	int transparentRun = 0;
	int xCur = 0;
	bool firstTransparentRun = true;
	while (src < end) {
		const auto val = static_cast<std::uint8_t>(*src++);
		if (IsCelTransparent(val)) {
			const int width = GetCelTransparentWidth(val);
			transparentRun += width;
			xCur += width;
			if (IsCelTransparentEol(val)) {
				xEnd = std::max(xEnd, celWidth - transparentRun);
				xCur = 0;
				firstTransparentRun = true;
				transparentRun = 0;
			}
		} else {
			if (firstTransparentRun) {
				xBegin = std::min(xBegin, transparentRun);
				firstTransparentRun = false;
				if (xBegin == 0 && xEnd == celWidth)
					break;
			}
			transparentRun = 0;
			xCur += val;
			src += val;
			if (IsCelPixelsEol(val)) {
				xEnd = celWidth;
				if (xBegin == 0)
					break;
				xCur = 0;
				firstTransparentRun = true;
			}
		}
	}
	return { xBegin, xEnd };
}

void CelDrawTo(const CelOutputBuffer &out, int sx, int sy, const CelSprite &cel, int frame)
{
	int nDataSize;
	const BYTE *pRLEBytes = CelGetFrame(cel.Data(), frame, &nDataSize);
	CelBlitSafeTo(out, sx, sy, pRLEBytes, nDataSize, cel.Width(frame));
}

void CelClippedDrawTo(const CelOutputBuffer &out, int sx, int sy, const CelSprite &cel, int frame)
{
	int nDataSize;
	const BYTE *pRLEBytes = CelGetFrameClipped(cel.Data(), frame, &nDataSize);

	CelBlitSafeTo(out, sx, sy, pRLEBytes, nDataSize, cel.Width(frame));
}

void CelDrawLightTo(const CelOutputBuffer &out, int sx, int sy, const CelSprite &cel, int frame, BYTE *tbl)
{
	int nDataSize;
	const BYTE *pRLEBytes = CelGetFrame(cel.Data(), frame, &nDataSize);

	if (light_table_index != 0 || tbl != nullptr)
		CelBlitLightSafeTo(out, sx, sy, pRLEBytes, nDataSize, cel.Width(frame), tbl);
	else
		CelBlitSafeTo(out, sx, sy, pRLEBytes, nDataSize, cel.Width(frame));
}

void CelClippedDrawLightTo(const CelOutputBuffer &out, int sx, int sy, const CelSprite &cel, int frame)
{
	int nDataSize;
	const BYTE *pRLEBytes = CelGetFrameClipped(cel.Data(), frame, &nDataSize);

	if (light_table_index != 0)
		CelBlitLightSafeTo(out, sx, sy, pRLEBytes, nDataSize, cel.Width(frame), nullptr);
	else
		CelBlitSafeTo(out, sx, sy, pRLEBytes, nDataSize, cel.Width(frame));
}

void CelDrawLightRedTo(const CelOutputBuffer &out, int sx, int sy, const CelSprite &cel, int frame, char light)
{
	int nDataSize;
	const BYTE *pRLEBytes = CelGetFrameClipped(cel.Data(), frame, &nDataSize);
	BYTE *dst = out.at(sx, sy);
	BYTE *tbl = GetLightTable(light);
	const auto celWidth = cel.Width(frame);

	for (const BYTE *end = &pRLEBytes[nDataSize]; pRLEBytes != end; dst -= out.pitch() + celWidth) {
		for (int w = celWidth; w > 0;) {
			BYTE width = *pRLEBytes++;
			if (!IsCelTransparent(width)) {
				w -= width;
				while (width > 0) {
					*dst = tbl[*pRLEBytes];
					pRLEBytes++;
					dst++;
					width--;
				}
			} else {
				width = -(char)width;
				dst += width;
				w -= width;
			}
		}
	}
}

void CelBlitSafeTo(const CelOutputBuffer &out, int sx, int sy, const BYTE *pRLEBytes, int nDataSize, int nWidth)
{
	assert(pRLEBytes != nullptr);

	const BYTE *src = pRLEBytes;
	BYTE *dst = out.at(sx, sy);

	for (; src != &pRLEBytes[nDataSize]; dst -= out.pitch() + nWidth) {
		for (int w = nWidth; w > 0;) {
			BYTE width = *src++;
			if (!IsCelTransparent(width)) {
				w -= width;
				if (dst < out.end() && dst >= out.begin()) {
					memcpy(dst, src, std::min(static_cast<ptrdiff_t>(width), out.end() - dst));
				}
				src += width;
				dst += width;
			} else {
				width = -(char)width;
				dst += width;
				w -= width;
			}
		}
	}
}

void CelClippedDrawSafeTo(const CelOutputBuffer &out, int sx, int sy, const CelSprite &cel, int frame)
{
	int nDataSize;
	const BYTE *pRLEBytes = CelGetFrameClipped(cel.Data(), frame, &nDataSize);
	CelBlitSafeTo(out, sx, sy, pRLEBytes, nDataSize, cel.Width(frame));
}

void CelBlitLightSafeTo(const CelOutputBuffer &out, int sx, int sy, const BYTE *pRLEBytes, int nDataSize, int nWidth, BYTE *tbl)
{
	assert(pRLEBytes != nullptr);

	const BYTE *src = pRLEBytes;
	BYTE *dst = out.at(sx, sy);

	if (tbl == nullptr)
		tbl = &pLightTbl[light_table_index * 256];

	for (; src != &pRLEBytes[nDataSize]; dst -= out.pitch() + nWidth) {
		for (int w = nWidth; w > 0;) {
			BYTE width = *src++;
			if (!IsCelTransparent(width)) {
				w -= width;
				if (dst < out.end() && dst > out.begin()) {
					if ((width & 1) != 0) {
						dst[0] = tbl[src[0]];
						src++;
						dst++;
					}
					width /= 2;
					if ((width & 1) != 0) {
						dst[0] = tbl[src[0]];
						dst[1] = tbl[src[1]];
						src += 2;
						dst += 2;
					}
					width /= 2;
					for (; width > 0; width--) {
						dst[0] = tbl[src[0]];
						dst[1] = tbl[src[1]];
						dst[2] = tbl[src[2]];
						dst[3] = tbl[src[3]];
						src += 4;
						dst += 4;
					}
				} else {
					src += width;
					dst += width;
				}
			} else {
				width = -(char)width;
				dst += width;
				w -= width;
			}
		}
	}
}

void CelBlitLightTransSafeTo(const CelOutputBuffer &out, int sx, int sy, const BYTE *pRLEBytes, int nDataSize, int nWidth)
{
	assert(pRLEBytes != nullptr);

	const BYTE *src = pRLEBytes;
	BYTE *dst = out.at(sx, sy);
	BYTE *tbl = &pLightTbl[light_table_index * 256];
	bool shift = ((size_t)dst % 2) != 0;

	for (; src != &pRLEBytes[nDataSize]; dst -= out.pitch() + nWidth, shift = !shift) {
		for (int w = nWidth; w > 0;) {
			BYTE width = *src++;
			if (!IsCelTransparent(width)) {
				w -= width;
				if (dst < out.end() && dst > out.begin()) {
					if (((size_t)dst % 2) == shift) {
						if ((width & 1) == 0) {
							goto L_ODD;
						} else {
							src++;
							dst++;
						L_EVEN:
							width /= 2;
							if ((width & 1) != 0) {
								dst[0] = tbl[src[0]];
								src += 2;
								dst += 2;
							}
							width /= 2;
							for (; width > 0; width--) {
								dst[0] = tbl[src[0]];
								dst[2] = tbl[src[2]];
								src += 4;
								dst += 4;
							}
						}
					} else {
						if ((width & 1) == 0) {
							goto L_EVEN;
						} else {
							dst[0] = tbl[src[0]];
							src++;
							dst++;
						L_ODD:
							width /= 2;
							if ((width & 1) != 0) {
								dst[1] = tbl[src[1]];
								src += 2;
								dst += 2;
							}
							width /= 2;
							for (; width > 0; width--) {
								dst[1] = tbl[src[1]];
								dst[3] = tbl[src[3]];
								src += 4;
								dst += 4;
							}
						}
					}
				} else {
					src += width;
					dst += width;
				}
			} else {
				width = -(char)width;
				dst += width;
				w -= width;
			}
		}
	}
}

/**
 * @brief Same as CelBlitLightSafe, with blended transparancy applied
 * @param out The output buffer
 * @param pRLEBytes CEL pixel stream (run-length encoded)
 * @param nDataSize Size of CEL in bytes
 * @param nWidth Width of sprite
 * @param tbl Palette translation table
 */
static void CelBlitLightBlendedSafeTo(const CelOutputBuffer &out, int sx, int sy, const BYTE *pRLEBytes, int nDataSize, int nWidth, BYTE *tbl)
{
	assert(pRLEBytes != nullptr);

	const BYTE *src = pRLEBytes;
	BYTE *dst = out.at(sx, sy);
	if (tbl == nullptr)
		tbl = &pLightTbl[light_table_index * 256];

	for (; src != &pRLEBytes[nDataSize]; dst -= out.pitch() + nWidth) {
		for (int w = nWidth; w > 0;) {
			BYTE width = *src++;
			if (!IsCelTransparent(width)) {
				w -= width;
				if (dst < out.end() && dst > out.begin()) {
					if ((width & 1) != 0) {
						dst[0] = paletteTransparencyLookup[dst[0]][tbl[src[0]]];
						src++;
						dst++;
					}
					width /= 2;
					if ((width & 1) != 0) {
						dst[0] = paletteTransparencyLookup[dst[0]][tbl[src[0]]];
						dst[1] = paletteTransparencyLookup[dst[1]][tbl[src[1]]];
						src += 2;
						dst += 2;
					}
					width /= 2;
					for (; width > 0; width--) {
						dst[0] = paletteTransparencyLookup[dst[0]][tbl[src[0]]];
						dst[1] = paletteTransparencyLookup[dst[1]][tbl[src[1]]];
						dst[2] = paletteTransparencyLookup[dst[2]][tbl[src[2]]];
						dst[3] = paletteTransparencyLookup[dst[3]][tbl[src[3]]];
						src += 4;
						dst += 4;
					}
				} else {
					src += width;
					dst += width;
				}
			} else {
				width = -(char)width;
				dst += width;
				w -= width;
			}
		}
	}
}

void CelClippedBlitLightTransTo(const CelOutputBuffer &out, int sx, int sy, const CelSprite &cel, int frame)
{
	int nDataSize;
	const BYTE *pRLEBytes = CelGetFrameClipped(cel.Data(), frame, &nDataSize);

	if (cel_transparency_active) {
		if (sgOptions.Graphics.bBlendedTransparancy)
			CelBlitLightBlendedSafeTo(out, sx, sy, pRLEBytes, nDataSize, cel.Width(frame), nullptr);
		else
			CelBlitLightTransSafeTo(out, sx, sy, pRLEBytes, nDataSize, cel.Width(frame));
	} else if (light_table_index != 0)
		CelBlitLightSafeTo(out, sx, sy, pRLEBytes, nDataSize, cel.Width(frame), nullptr);
	else
		CelBlitSafeTo(out, sx, sy, pRLEBytes, nDataSize, cel.Width(frame));
}

void CelDrawLightRedSafeTo(const CelOutputBuffer &out, int sx, int sy, const CelSprite &cel, int frame, char light)
{
	int nDataSize;
	const BYTE *pRLEBytes = CelGetFrameClipped(cel.Data(), frame, &nDataSize);
	BYTE *dst = out.at(sx, sy);

	BYTE *tbl = GetLightTable(light);
	const BYTE *end = &pRLEBytes[nDataSize];
	const int celWidth = static_cast<int>(cel.Width(frame));

	for (; pRLEBytes != end; dst -= out.pitch() + celWidth) {
		for (int w = celWidth; w > 0;) {
			BYTE width = *pRLEBytes++;
			if (!IsCelTransparent(width)) {
				w -= width;
				if (dst < out.end() && dst > out.begin()) {
					while (width > 0) {
						*dst = tbl[*pRLEBytes];
						pRLEBytes++;
						dst++;
						width--;
					}
				} else {
					pRLEBytes += width;
					dst += width;
				}
			} else {
				width = -(char)width;
				dst += width;
				w -= width;
			}
		}
	}
}

void CelDrawUnsafeTo(const CelOutputBuffer &out, int x, int y, const CelSprite &cel, int frame)
{
	int nDataSize;
	const BYTE *pRLEBytes = CelGetFrame(cel.Data(), frame, &nDataSize);
	const BYTE *end = &pRLEBytes[nDataSize];
	BYTE *dst = out.at(x, y);
	const int celWidth = static_cast<int>(cel.Width(frame));

	for (; pRLEBytes != end; dst -= out.pitch() + celWidth) {
		for (int w = celWidth; w > 0;) {
			BYTE width = *pRLEBytes++;
			if (!IsCelTransparent(width)) {
				w -= width;
				memcpy(dst, pRLEBytes, width);
				dst += width;
				pRLEBytes += width;
			} else {
				width = -(char)width;
				dst += width;
				w -= width;
			}
		}
	}
}

void CelBlitOutlineTo(const CelOutputBuffer &out, BYTE col, int sx, int sy, const CelSprite &cel, int frame, bool skipColorIndexZero)
{
	int nDataSize;

	const BYTE *src = CelGetFrameClipped(cel.Data(), frame, &nDataSize);
	const BYTE *end = &src[nDataSize];
	BYTE *dst = out.at(sx, sy);
	const int celWidth = static_cast<int>(cel.Width(frame));

	for (; src != end; dst -= out.pitch() + celWidth) {
		for (int w = celWidth; w > 0;) {
			BYTE width = *src++;
			if (!IsCelTransparent(width)) {
				w -= width;
				if (dst < out.end() && dst > out.begin()) {
					if (dst >= out.end() - out.pitch()) {
						while (width > 0) {
							if (!skipColorIndexZero || *src > 0) {
								dst[-out.pitch()] = col;
								dst[-1] = col;
								dst[1] = col;
							}
							src++;
							dst++;
							width--;
						}
					} else {
						while (width > 0) {
							if (!skipColorIndexZero || *src > 0) {
								dst[-out.pitch()] = col;
								dst[-1] = col;
								dst[1] = col;
								dst[out.pitch()] = col;
							}
							src++;
							dst++;
							width--;
						}
					}
				} else {
					src += width;
					dst += width;
				}
			} else {
				width = -(char)width;
				dst += width;
				w -= width;
			}
		}
	}
}

void SetPixel(const CelOutputBuffer &out, Point position, BYTE col)
{
	if (!out.in_bounds(position))
		return;

	*out.at(position.x, position.y) = col;
}

void DrawLineTo(const CelOutputBuffer &out, Point a, Point b, BYTE color_index)
{
	int dx = b.x - a.x;
	int dy = b.y - a.y;
	int steps = abs(dx) > abs(dy) ? abs(dx) : abs(dy);
	float ix = dx / (float)steps;
	float iy = dy / (float)steps;
	float sx = a.x;
	float sy = a.y;

	for (int i = 0; i <= steps; i++, sx += ix, sy += iy) {
		SetPixel(out, { static_cast<int>(sx), static_cast<int>(sy) }, color_index);
	}
}

static void DrawHalfTransparentBlendedRectTo(const CelOutputBuffer &out, int sx, int sy, int width, int height)
{
	BYTE *pix = out.at(sx, sy);

	for (int row = 0; row < height; row++) {
		for (int col = 0; col < width; col++) {
			*pix = paletteTransparencyLookup[0][*pix];
			pix++;
		}
		pix += out.pitch() - width;
	}
}

static void DrawHalfTransparentStippledRectTo(const CelOutputBuffer &out, int sx, int sy, int width, int height)
{
	BYTE *pix = out.at(sx, sy);

	for (int row = 0; row < height; row++) {
		for (int col = 0; col < width; col++) {
			if (((row & 1) != 0 && (col & 1) != 0) || ((row & 1) == 0 && (col & 1) == 0))
				*pix = 0;
			pix++;
		}
		pix += out.pitch() - width;
	}
}

void DrawHalfTransparentRectTo(const CelOutputBuffer &out, int sx, int sy, int width, int height)
{
	if (sgOptions.Graphics.bBlendedTransparancy) {
		DrawHalfTransparentBlendedRectTo(out, sx, sy, width, height);
	} else {
		DrawHalfTransparentStippledRectTo(out, sx, sy, width, height);
	}
}

/**
 * @brief Returns the direction a vector from p1(x1, y1) to p2(x2, y2) is pointing to.
 *
 *      W    SW     S
 *            ^
 *            |
 *     NW ----+---> SE
 *            |
 *            |
 *      N    NE     E
 *
 * @param x1 the x coordinate of p1
 * @param y1 the y coordinate of p1
 * @param x2 the x coordinate of p2
 * @param y2 the y coordinate of p2
 * @return the direction of the p1->p2 vector
*/
direction GetDirection(Point start, Point destination)
{
	direction md = DIR_S;

	int mx = destination.x - start.x;
	int my = destination.y - start.y;
	if (mx >= 0) {
		if (my >= 0) {
			if (5 * mx <= (my * 2)) // mx/my <= 0.4, approximation of tan(22.5)
				return DIR_SW;
			md = DIR_S;
		} else {
			my = -my;
			if (5 * mx <= (my * 2))
				return DIR_NE;
			md = DIR_E;
		}
		if (5 * my <= (mx * 2)) // my/mx <= 0.4
			md = DIR_SE;
	} else {
		mx = -mx;
		if (my >= 0) {
			if (5 * mx <= (my * 2))
				return DIR_SW;
			md = DIR_W;
		} else {
			my = -my;
			if (5 * mx <= (my * 2))
				return DIR_NE;
			md = DIR_N;
		}
		if (5 * my <= (mx * 2))
			md = DIR_NW;
	}
	return md;
}

int CalculateWidth2(int width)
{
	return (width - 64) / 2;
}

/**
 * @brief Set the RNG seed
 * @param s RNG seed
 */
void SetRndSeed(int32_t s)
{
	sglGameSeed = s;
	orgseed = s;
}

/**
 * @brief Advance the internal RNG seed and return the new value
 * @return RNG seed
 */
int32_t AdvanceRndSeed()
{
	sglGameSeed = (RndMult * static_cast<uint32_t>(sglGameSeed)) + RndInc;
	return abs(sglGameSeed);
}

/**
 * @brief Get the current RNG seed
 * @return RNG seed
 */
int32_t GetRndSeed()
{
	return abs(sglGameSeed);
}

/**
 * @brief Main RNG function
 * @param v The upper limit for the return value
 * @return A random number from 0 to (v-1)
 */
int32_t GenerateRnd(int32_t v)
{
	if (v <= 0)
		return 0;
	if (v < 0xFFFF)
		return (AdvanceRndSeed() >> 16) % v;
	return AdvanceRndSeed() % v;
}

size_t GetFileSize(const char *pszName)
{
	HANDLE file;
	SFileOpenFile(pszName, &file);
	const size_t fileLen = SFileGetFileSize(file, nullptr);
	SFileCloseFile(file);

	return fileLen;
}

void LoadFileData(const char *pszName, byte *buffer, size_t fileLen)
{
	HANDLE file;
	SFileOpenFile(pszName, &file);

	if (fileLen == 0)
		app_fatal("Zero length SFILE:\n%s", pszName);

	SFileReadFileThreadSafe(file, buffer, fileLen);
	SFileCloseFile(file);
}

/**
 * @brief Load a file in to the given buffer
 * @param pszName Path of file
 * @param p Target buffer
 * @return Size of file
 */
DWORD LoadFileWithMem(const char *pszName, BYTE *p)
{
	assert(pszName);
	if (p == nullptr) {
		app_fatal("LoadFileWithMem(NULL):\n%s", pszName);
	}

	HANDLE hsFile;
	SFileOpenFile(pszName, &hsFile);

	DWORD dwFileLen = SFileGetFileSize(hsFile, nullptr);
	if (dwFileLen == 0) {
		app_fatal("Zero length SFILE:\n%s", pszName);
	}

	SFileReadFileThreadSafe(hsFile, p, dwFileLen);
	SFileCloseFile(hsFile);

	return dwFileLen;
}

/**
 * @brief Apply the color swaps to a CL2 sprite
 * @param p CL2 buffer
 * @param ttbl Palette translation table
 * @param nCel Frame number in CL2 file
 */
void Cl2ApplyTrans(BYTE *p, BYTE *ttbl, int nCel)
{
	assert(p != nullptr);
	assert(ttbl != nullptr);

	for (int i = 1; i <= nCel; i++) {
		int nDataSize;
		BYTE *dst = CelGetFrame(p, i, &nDataSize) + 10;
		nDataSize -= 10;
		while (nDataSize > 0) {
			char width = *dst++;
			nDataSize--;
			assert(nDataSize >= 0);
			if (width < 0) {
				width = -width;
				if (width > MaxCl2Width) {
					nDataSize--;
					assert(nDataSize >= 0);
					*dst = ttbl[*dst];
					dst++;
				} else {
					nDataSize -= width;
					assert(nDataSize >= 0);
					for (; width > 0; width--) {
						*dst = ttbl[*dst];
						dst++;
					}
				}
			}
		}
	}
}

/**
 * @brief Blit CL2 sprite to the given buffer
 * @param out Target buffer
 * @param sx Target buffer coordinate
 * @param sy Target buffer coordinate
 * @param pRLEBytes CL2 pixel stream (run-length encoded)
 * @param nDataSize Size of CL2 in bytes
 * @param nWidth Width of sprite
 */
static void Cl2BlitSafe(const CelOutputBuffer &out, int sx, int sy, const BYTE *pRLEBytes, int nDataSize, int nWidth)
{
	const BYTE *src = pRLEBytes;
	BYTE *dst = out.at(sx, sy);
	int w = nWidth;

	while (nDataSize > 0) {
		char width = *src++;
		nDataSize--;
		if (width < 0) {
			width = -width;
			if (width > MaxCl2Width) {
				width -= MaxCl2Width;
				nDataSize--;
				BYTE fill = *src++;
				if (dst < out.end() && dst > out.begin()) {
					w -= width;
					while (width > 0) {
						*dst = fill;
						dst++;
						width--;
					}
					if (w == 0) {
						w = nWidth;
						dst -= out.pitch() + w;
					}
					continue;
				}
			} else {
				nDataSize -= width;
				if (dst < out.end() && dst > out.begin()) {
					w -= width;
					while (width > 0) {
						*dst = *src;
						src++;
						dst++;
						width--;
					}
					if (w == 0) {
						w = nWidth;
						dst -= out.pitch() + w;
					}
					continue;
				}
				src += width;
			}
		}
		while (width > 0) {
			if (width > w) {
				dst += w;
				width -= w;
				w = 0;
			} else {
				dst += width;
				w -= width;
				width = 0;
			}
			if (w == 0) {
				w = nWidth;
				dst -= out.pitch() + w;
			}
		}
	}
}

/**
 * @brief Blit a solid colder shape one pixel larger then the given sprite shape, to the given buffer
 * @param out Target buffer
 * @param sx Target buffer coordinate
 * @param sy Target buffer coordinate
 * @param pRLEBytes CL2 pixel stream (run-length encoded)
 * @param nDataSize Size of CL2 in bytes
 * @param nWidth Width of sprite
 * @param col Color index from current palette
 */
static void Cl2BlitOutlineSafe(const CelOutputBuffer &out, int sx, int sy, const BYTE *pRLEBytes, int nDataSize, int nWidth, BYTE col)
{
	const BYTE *src = pRLEBytes;
	BYTE *dst = out.at(sx, sy);
	int w = nWidth;

	while (nDataSize > 0) {
		int8_t width = *src++;
		nDataSize--;
		if (width < 0) {
			width = -width;
			if (width > MaxCl2Width) {
				width -= MaxCl2Width;
				nDataSize--;
				if (*src++ != 0 && dst < out.end() && dst > out.begin()) {
					w -= width;
					dst[-1] = col;
					dst[width] = col;
					while (width > 0) {
						dst[-out.pitch()] = col;
						dst[out.pitch()] = col;
						dst++;
						width--;
					}
					if (w == 0) {
						w = nWidth;
						dst -= out.pitch() + w;
					}
					continue;
				}
			} else {
				nDataSize -= width;
				if (dst < out.end() && dst > out.begin()) {
					w -= width;
					while (width > 0) {
						if (*src != 0) {
							dst[-1] = col;
							dst[1] = col;
							dst[-out.pitch()] = col;
							// BUGFIX: only set `if (dst+out.pitch() < out.end())`
							dst[out.pitch()] = col;
						}
						src++;
						dst++;
						width--;
					}
					if (w == 0) {
						w = nWidth;
						dst -= out.pitch() + w;
					}
					continue;
				}
				src += width;
			}
		}
		while (width > 0) {
			if (width > w) {
				dst += w;
				width -= w;
				w = 0;
			} else {
				dst += width;
				w -= width;
				width = 0;
			}
			if (w == 0) {
				w = nWidth;
				dst -= out.pitch() + w;
			}
		}
	}
}

/**
 * @brief Blit CL2 sprite, and apply lighting, to the given buffer
 * @param out Target buffer
 * @param sx Target buffer coordinate
 * @param sy Target buffer coordinate
 * @param pRLEBytes CL2 pixel stream (run-length encoded)
 * @param nDataSize Size of CL2 in bytes
 * @param nWidth With of CL2 sprite
 * @param pTable Light color table
 */
static void Cl2BlitLightSafe(const CelOutputBuffer &out, int sx, int sy, const BYTE *pRLEBytes, int nDataSize, int nWidth, BYTE *pTable)
{
	const BYTE *src = pRLEBytes;
	BYTE *dst = out.at(sx, sy);
	int w = nWidth;

	while (nDataSize > 0) {
		char width = *src++;
		nDataSize--;
		if (width < 0) {
			width = -width;
			if (width > MaxCl2Width) {
				width -= MaxCl2Width;
				nDataSize--;
				BYTE fill = pTable[*src++];
				if (dst < out.end() && dst > out.begin()) {
					w -= width;
					while (width > 0) {
						*dst = fill;
						dst++;
						width--;
					}
					if (w == 0) {
						w = nWidth;
						dst -= out.pitch() + w;
					}
					continue;
				}
			} else {
				nDataSize -= width;
				if (dst < out.end() && dst > out.begin()) {
					w -= width;
					while (width > 0) {
						*dst = pTable[*src];
						src++;
						dst++;
						width--;
					}
					if (w == 0) {
						w = nWidth;
						dst -= out.pitch() + w;
					}
					continue;
				}
				src += width;
			}
		}
		while (width > 0) {
			if (width > w) {
				dst += w;
				width -= w;
				w = 0;
			} else {
				dst += width;
				w -= width;
				width = 0;
			}
			if (w == 0) {
				w = nWidth;
				dst -= out.pitch() + w;
			}
		}
	}
}

void Cl2Draw(const CelOutputBuffer &out, int sx, int sy, const CelSprite &cel, int frame)
{
	assert(frame > 0);

	int nDataSize;
	const BYTE *pRLEBytes = CelGetFrameClipped(cel.Data(), frame, &nDataSize);

	Cl2BlitSafe(out, sx, sy, pRLEBytes, nDataSize, cel.Width(frame));
}

void Cl2DrawOutline(const CelOutputBuffer &out, BYTE col, int sx, int sy, const CelSprite &cel, int frame)
{
	assert(frame > 0);

	int nDataSize;
	const BYTE *pRLEBytes = CelGetFrameClipped(cel.Data(), frame, &nDataSize);

	const CelOutputBuffer &sub = out.subregionY(0, out.h() - 1);
	Cl2BlitOutlineSafe(sub, sx, sy, pRLEBytes, nDataSize, cel.Width(frame), col);
}

void Cl2DrawLightTbl(const CelOutputBuffer &out, int sx, int sy, const CelSprite &cel, int frame, char light)
{
	assert(frame > 0);

	int nDataSize;
	const BYTE *pRLEBytes = CelGetFrameClipped(cel.Data(), frame, &nDataSize);
	Cl2BlitLightSafe(out, sx, sy, pRLEBytes, nDataSize, cel.Width(frame), GetLightTable(light));
}

void Cl2DrawLight(const CelOutputBuffer &out, int sx, int sy, const CelSprite &cel, int frame)
{
	assert(frame > 0);

	int nDataSize;
	const BYTE *pRLEBytes = CelGetFrameClipped(cel.Data(), frame, &nDataSize);

	if (light_table_index != 0)
		Cl2BlitLightSafe(out, sx, sy, pRLEBytes, nDataSize, cel.Width(frame), &pLightTbl[light_table_index * 256]);
	else
		Cl2BlitSafe(out, sx, sy, pRLEBytes, nDataSize, cel.Width(frame));
}

/**
 * @brief Fade to black and play a video
 * @param pszMovie file path of movie
 */
void PlayInGameMovie(const char *pszMovie)
{
	PaletteFadeOut(8);
	play_movie(pszMovie, false);
	ClearScreenBuffer();
	force_redraw = 255;
	scrollrt_draw_game_screen(true);
	PaletteFadeIn(8);
	force_redraw = 255;
}

} // namespace devilution
