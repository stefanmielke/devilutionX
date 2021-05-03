/**
 * @file palette.cpp
 *
 * Implementation of functions for handling the engines color palette.
 */

#include "dx.h"
#include "options.h"
#include "render.h"
#include "storm/storm.h"
#include "utils/display.h"
#include "utils/sdl_compat.h"

namespace devilution {

SDL_Color logical_palette[256];
SDL_Color system_palette[256];
SDL_Color orig_palette[256];
Uint8 paletteTransparencyLookup[256][256]; //Lookup table for transparency

/* data */

/** Specifies whether the palette has max brightness. */
bool sgbFadedIn = true;

void palette_update()
{
	assert(palette);
	if (SDLC_SetSurfaceAndPaletteColors(pal_surface, palette, system_palette, 0, 256) < 0) {
		ErrSdl();
	}
	pal_surface_palette_version++;
}

void ApplyGamma(SDL_Color *dst, const SDL_Color *src, int n)
{
	int i;
	double g;

	g = sgOptions.Graphics.nGammaCorrection / 100.0;

	for (i = 0; i < n; i++) {
		dst[i].r = pow(src[i].r / 256.0, g) * 256.0;
		dst[i].g = pow(src[i].g / 256.0, g) * 256.0;
		dst[i].b = pow(src[i].b / 256.0, g) * 256.0;
	}
	force_redraw = 255;
}

static void LoadGamma()
{
	int gamma_value = sgOptions.Graphics.nGammaCorrection;

	if (gamma_value < 30) {
		gamma_value = 30;
	} else if (gamma_value > 100) {
		gamma_value = 100;
	}
	sgOptions.Graphics.nGammaCorrection = gamma_value - gamma_value % 5;
}

void palette_init()
{
	LoadGamma();
	memcpy(system_palette, orig_palette, sizeof(orig_palette));
	InitPalette();
}

/**
 * @brief Generate lookup table for transparency
 *
 * This is based of the same technique found in Quake2.
 *
 * To mimic 50% transparency we figure out what colors in the existing palette are the best match for the combination of any 2 colors.
 * We save this into a lookup table for use during rendering.
 *
 * @param palette The colors to operate on
 * @param skipFrom Do not use colors between this index and skipTo
 * @param skipTo Do not use colors between skipFrom and this index
 * @param toUpdate Only update the first n colors
 */
static void GenerateBlendedLookupTable(SDL_Color *palette, int skipFrom, int skipTo, int toUpdate = 256)
{
	for (int i = 0; i < 256; i++) {
		for (int j = 0; j < 256; j++) {
			if (i == j) { // No need to calculate transparency between 2 identical colors
				paletteTransparencyLookup[i][j] = j;
				continue;
			}
			if (i > j) { // Half the blends will be mirror identical ([i][j] is the same as [j][i]), so simply copy the existing combination.
				paletteTransparencyLookup[i][j] = paletteTransparencyLookup[j][i];
				continue;
			}
			if (i > toUpdate && j > toUpdate) {
				continue;
			}

			Uint8 r = ((int)palette[i].r + (int)palette[j].r) / 2;
			Uint8 g = ((int)palette[i].g + (int)palette[j].g) / 2;
			Uint8 b = ((int)palette[i].b + (int)palette[j].b) / 2;
			Uint8 best;
			Uint32 bestDiff = SDL_MAX_UINT32;
			for (int k = 0; k < 256; k++) {
				if (k >= skipFrom && k <= skipTo)
					continue;
				int diffr = palette[k].r - r;
				int diffg = palette[k].g - g;
				int diffb = palette[k].b - b;
				Uint32 diff = diffr * diffr + diffg * diffg + diffb * diffb;

				if (bestDiff > diff) {
					best = k;
					bestDiff = diff;
				}
			}
			paletteTransparencyLookup[i][j] = best;
		}
	}
}

void LoadPalette(const char *pszFileName)
{
	assert(pszFileName);

	struct Color {
		uint8_t r;
		uint8_t g;
		uint8_t b;
	};

	std::array<Color, 256> PalData;

	LoadFileInMem(pszFileName, PalData);

	for (int i = 0; i < PalData.size(); i++) {
		orig_palette[i].r = PalData[i].r;
		orig_palette[i].g = PalData[i].g;
		orig_palette[i].b = PalData[i].b;
#ifndef USE_SDL1
		orig_palette[i].a = SDL_ALPHA_OPAQUE;
#endif
	}

	if (sgOptions.Graphics.bBlendedTransparancy) {
		if (leveltype == DTYPE_CAVES || leveltype == DTYPE_CRYPT) {
			GenerateBlendedLookupTable(orig_palette, 1, 31);
		} else if (leveltype == DTYPE_NEST) {
			GenerateBlendedLookupTable(orig_palette, 1, 15);
		} else {
			GenerateBlendedLookupTable(orig_palette, -1, -1);
		}
	}
}

void LoadRndLvlPal(int l)
{
	if (l == DTYPE_TOWN) {
		LoadPalette("Levels\\TownData\\Town.pal");
		return;
	}

	int rv = GenerateRnd(4) + 1;
	if (l == 5) {
		LoadPalette("NLevels\\L5Data\\L5Base.PAL");
		return;
	}

	char szFileName[27];
	if (l == 6) {
		if (!gbNestArt) {
			rv++;
		}
		sprintf(szFileName, "NLevels\\L%iData\\L%iBase%i.PAL", 6, 6, rv);
	} else {
		sprintf(szFileName, "Levels\\L%iData\\L%i_%i.PAL", l, l, rv);
	}
	LoadPalette(szFileName);
}

void ResetPal()
{
}

void IncreaseGamma()
{
	if (sgOptions.Graphics.nGammaCorrection < 100) {
		sgOptions.Graphics.nGammaCorrection += 5;
		if (sgOptions.Graphics.nGammaCorrection > 100)
			sgOptions.Graphics.nGammaCorrection = 100;
		ApplyGamma(system_palette, logical_palette, 256);
		palette_update();
	}
}

void DecreaseGamma()
{
	if (sgOptions.Graphics.nGammaCorrection > 30) {
		sgOptions.Graphics.nGammaCorrection -= 5;
		if (sgOptions.Graphics.nGammaCorrection < 30)
			sgOptions.Graphics.nGammaCorrection = 30;
		ApplyGamma(system_palette, logical_palette, 256);
		palette_update();
	}
}

int UpdateGamma(int gamma)
{
	if (gamma) {
		sgOptions.Graphics.nGammaCorrection = 130 - gamma;
		ApplyGamma(system_palette, logical_palette, 256);
		palette_update();
	}
	return 130 - sgOptions.Graphics.nGammaCorrection;
}

void SetFadeLevel(int fadeval)
{
	for (int i = 0; i < 256; i++) {
		system_palette[i].r = (fadeval * logical_palette[i].r) / 256;
		system_palette[i].g = (fadeval * logical_palette[i].g) / 256;
		system_palette[i].b = (fadeval * logical_palette[i].b) / 256;
	}
	palette_update();
}

void BlackPalette()
{
	SetFadeLevel(0);
}

void PaletteFadeIn(int fr)
{
	ApplyGamma(logical_palette, orig_palette, 256);

	SDL_Rect SrcRect { BUFFER_BORDER_LEFT, BUFFER_BORDER_TOP, gnScreenWidth, gnScreenHeight };
	const uint32_t tc = SDL_GetTicks();
	fr *= 3;

	for (uint32_t i = 0; i < 256; i = fr * (SDL_GetTicks() - tc) / 50) {
		SetFadeLevel(i);
		BltFast(&SrcRect, nullptr);
		RenderPresent();
	}
	SetFadeLevel(256);

	memcpy(logical_palette, orig_palette, sizeof(orig_palette));

	sgbFadedIn = true;
}

void PaletteFadeOut(int fr)
{
	if (!sgbFadedIn)
		return;

	SDL_Rect SrcRect { BUFFER_BORDER_LEFT, BUFFER_BORDER_TOP, gnScreenWidth, gnScreenHeight };
	const uint32_t tc = SDL_GetTicks();
	fr *= 3;

	for (uint32_t i = 0; i < 256; i = fr * (SDL_GetTicks() - tc) / 50) {
		SetFadeLevel(256 - i);
		BltFast(&SrcRect, nullptr);
		RenderPresent();
	}
	SetFadeLevel(0);

	sgbFadedIn = false;
}

/**
 * @brief Cycle the given range of colors in the palette
 * @param from First color index of the range
 * @param to First color index of the range
 */
static void CycleColors(int from, int to)
{
	SDL_Color col = system_palette[from];
	for (int i = from; i < to; i++) {
		system_palette[i] = system_palette[i + 1];
	}
	system_palette[to] = col;

	if (!sgOptions.Graphics.bBlendedTransparancy)
		return;

	for (auto &palette : paletteTransparencyLookup) {
		Uint8 col = palette[from];
		for (int j = from; j < to; j++) {
			palette[j] = palette[j + 1];
		}
		palette[to] = col;
	}

	Uint8 colRow[256];
	memcpy(colRow, &paletteTransparencyLookup[from], sizeof(*paletteTransparencyLookup));
	for (int i = from; i < to; i++) {
		memcpy(&paletteTransparencyLookup[i], &paletteTransparencyLookup[i + 1], sizeof(*paletteTransparencyLookup));
	}
	memcpy(&paletteTransparencyLookup[to], colRow, sizeof(colRow));
}

/**
 * @brief Cycle the given range of colors in the palette in reverse direction
 * @param from First color index of the range
 * @param to First color index of the range
 */
static void CycleColorsReverse(int from, int to)
{
	SDL_Color col = system_palette[to];
	for (int i = to; i > from; i--) {
		system_palette[i] = system_palette[i - 1];
	}
	system_palette[from] = col;

	if (!sgOptions.Graphics.bBlendedTransparancy)
		return;

	for (auto &palette : paletteTransparencyLookup) {
		Uint8 col = palette[to];
		for (int j = to; j > from; j--) {
			palette[j] = palette[j - 1];
		}
		palette[from] = col;
	}

	Uint8 colRow[256];
	memcpy(colRow, &paletteTransparencyLookup[to], sizeof(*paletteTransparencyLookup));
	for (int i = to; i > from; i--) {
		memcpy(&paletteTransparencyLookup[i], &paletteTransparencyLookup[i - 1], sizeof(*paletteTransparencyLookup));
	}
	memcpy(&paletteTransparencyLookup[from], colRow, sizeof(colRow));
}

void palette_update_caves()
{
	CycleColors(1, 31);
	palette_update();
}

int dword_6E2D58;
int dword_6E2D54;
void palette_update_crypt()
{
	if (dword_6E2D58 > 1) {
		CycleColorsReverse(1, 15);
		dword_6E2D58 = 0;
	} else {
		dword_6E2D58++;
	}
	if (dword_6E2D54 > 0) {
		CycleColorsReverse(16, 31);
		palette_update();
		dword_6E2D54++;
	} else {
		dword_6E2D54 = 1;
	}
}

int dword_6E2D5C;
int dword_6E2D60;
void palette_update_hive()
{
	if (dword_6E2D60 == 2) {
		CycleColorsReverse(1, 8);
		dword_6E2D60 = 0;
	} else {
		dword_6E2D60++;
	}
	if (dword_6E2D5C == 2) {
		CycleColorsReverse(9, 15);
		palette_update();
		dword_6E2D5C = 0;
	} else {
		dword_6E2D5C++;
	}
}

void palette_update_quest_palette(int n)
{
	int i;

	for (i = 32 - n; i >= 0; i--) {
		logical_palette[i] = orig_palette[i];
	}
	ApplyGamma(system_palette, logical_palette, 32);
	palette_update();
	GenerateBlendedLookupTable(logical_palette, 1, 31, 32 - n); // Possible optimization would be to only update color 0 as only the UI can overlap with transparency in this quest
}

} // namespace devilution
