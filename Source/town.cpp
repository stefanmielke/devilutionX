/**
 * @file town.h
 *
 * Implementation of functionality for rendering the town, towners and calling other render routines.
 */
#include "town.h"

#include "drlg_l1.h"
#include "init.h"
#include "player.h"
#include "quests.h"
#include "trigs.h"

namespace devilution {

namespace {

/**
 * @brief Load level data into dPiece
 * @param P3Tiles Tile set
 * @param path Path of dun file
 * @param xi upper left destination
 * @param yi upper left destination
 */
void T_FillSector(uint16_t *P3Tiles, const char *path, int xi, int yy)
{
	auto dunData = LoadFileInMem<uint16_t>(path);

	int width = SDL_SwapLE16(dunData[0]);
	int height = SDL_SwapLE16(dunData[1]);

	uint16_t *tileLayer = &dunData[2];

	for (int j = 0; j < height; j++) {
		int xx = xi;
		for (int i = 0; i < width; i++) {
			int nMap = SDL_SwapLE16(tileLayer[j * width + i]);
			uint16_t *Sector = &P3Tiles[(nMap - 1) * 4];
			dPiece[xx + 0][yy + 0] = nMap != 0 ? (SDL_SwapLE16(*(Sector + 0)) + 1) : 0;
			dPiece[xx + 1][yy + 0] = nMap != 0 ? (SDL_SwapLE16(*(Sector + 0)) + 2) : 0;
			dPiece[xx + 0][yy + 1] = nMap != 0 ? (SDL_SwapLE16(*(Sector + 0)) + 3) : 0;
			dPiece[xx + 1][yy + 1] = nMap != 0 ? (SDL_SwapLE16(*(Sector + 0)) + 4) : 0;
			xx += 2;
		}
		yy += 2;
	}
}

/**
 * @brief Load a tile in to dPiece
 * @param Tiles Tile set
 * @param xx upper left destination
 * @param yy upper left destination
 * @param t tile id
 */
void T_FillTile(uint16_t *Tiles, int xx, int yy, int t)
{
	dPiece[xx + 0][yy + 0] = SDL_SwapLE16(Tiles[(t - 1) * 4 + 0]) + 1;
	dPiece[xx + 1][yy + 0] = SDL_SwapLE16(Tiles[(t - 1) * 4 + 1]) + 1;
	dPiece[xx + 0][yy + 1] = SDL_SwapLE16(Tiles[(t - 1) * 4 + 2]) + 1;
	dPiece[xx + 1][yy + 1] = SDL_SwapLE16(Tiles[(t - 1) * 4 + 3]) + 1;
}

/**
 * @brief Update the map to show the closed hive
 */
void TownCloseHive()
{
	dPiece[78][60] = 0x48a;
	dPiece[79][60] = 0x4eb;
	dPiece[78][61] = 0x4ec;
	dPiece[79][61] = 0x4ed;
	dPiece[78][62] = 0x4ee;
	dPiece[79][62] = 0x4ef;
	dPiece[78][63] = 0x4f0;
	dPiece[79][63] = 0x4f1;
	dPiece[78][64] = 0x4f2;
	dPiece[79][64] = 0x4f3;
	dPiece[78][65] = 0x4f4;
	dPiece[80][60] = 0x4f5;
	dPiece[81][60] = 0x4f6;
	dPiece[80][61] = 0x4f7;
	dPiece[81][61] = 0x4f8;
	dPiece[82][60] = 0x4f9;
	dPiece[83][60] = 0x4fa;
	dPiece[82][61] = 0x4fb;
	dPiece[83][61] = 0x4fc;
	dPiece[80][62] = 0x4fd;
	dPiece[81][62] = 0x4fe;
	dPiece[80][63] = 0x4ff;
	dPiece[81][63] = 0x500;
	dPiece[80][64] = 0x501;
	dPiece[81][64] = 0x502;
	dPiece[80][65] = 0x503;
	dPiece[81][65] = 0x504;
	dPiece[82][64] = 0x509;
	dPiece[83][64] = 0x50a;
	dPiece[82][65] = 0x50b;
	dPiece[83][65] = 0x50c;
	dPiece[82][62] = 0x505;
	dPiece[83][62] = 0x506;
	dPiece[82][63] = 0x507;
	dPiece[83][63] = 0x508;
	dPiece[84][61] = 0x118;
	dPiece[84][62] = 0x118;
	dPiece[84][63] = 0x118;
	dPiece[85][60] = 0x118;
	dPiece[85][61] = 0x118;
	dPiece[85][63] = 8;
	dPiece[85][64] = 8;
	dPiece[86][60] = 0xd9;
	dPiece[86][61] = 0x18;
	dPiece[85][62] = 0x13;
	dPiece[84][64] = 0x118;
	SetDungeonMicros();
}

/**
 * @brief Update the map to show the closed grave
 */
void TownCloseGrave()
{
	dPiece[36][21] = 0x52b;
	dPiece[37][21] = 0x52c;
	dPiece[36][22] = 0x52d;
	dPiece[37][22] = 0x52e;
	dPiece[36][23] = 0x52f;
	dPiece[37][23] = 0x530;
	dPiece[36][24] = 0x531;
	dPiece[37][24] = 0x532;
	dPiece[35][21] = 0x53b;
	dPiece[34][21] = 0x53c;
	SetDungeonMicros();
}

/**
 * @brief Initialize all of the levels data
 */
void T_Pass3()
{
	int xx, yy, x;

	for (yy = 0; yy < MAXDUNY; yy += 2) {
		for (xx = 0; xx < MAXDUNX; xx += 2) {
			dPiece[xx][yy] = 0;
			dPiece[xx + 1][yy] = 0;
			dPiece[xx][yy + 1] = 0;
			dPiece[xx + 1][yy + 1] = 0;
		}
	}

	auto P3Tiles = LoadFileInMem<uint16_t>("Levels\\TownData\\Town.TIL");

	T_FillSector(P3Tiles.get(), "Levels\\TownData\\Sector1s.DUN", 46, 46);
	T_FillSector(P3Tiles.get(), "Levels\\TownData\\Sector2s.DUN", 46, 0);
	T_FillSector(P3Tiles.get(), "Levels\\TownData\\Sector3s.DUN", 0, 46);
	T_FillSector(P3Tiles.get(), "Levels\\TownData\\Sector4s.DUN", 0, 0);

	if (gbIsSpawn || !gbIsMultiplayer) {
		if (gbIsSpawn || (!(plr[myplr].pTownWarps & 1) && (!gbIsHellfire || plr[myplr]._pLevel < 10))) {
			T_FillTile(P3Tiles.get(), 48, 20, 320);
		}
		if (gbIsSpawn || (!(plr[myplr].pTownWarps & 2) && (!gbIsHellfire || plr[myplr]._pLevel < 15))) {
			T_FillTile(P3Tiles.get(), 16, 68, 332);
			T_FillTile(P3Tiles.get(), 16, 70, 331);
		}
		if (gbIsSpawn || (!(plr[myplr].pTownWarps & 4) && (!gbIsHellfire || plr[myplr]._pLevel < 20))) {
			for (x = 36; x < 46; x++) {
				T_FillTile(P3Tiles.get(), x, 78, GenerateRnd(4) + 1);
			}
		}
	}
	if (gbIsHellfire) {
		if (quests[Q_FARMER]._qactive == QUEST_DONE || quests[Q_FARMER]._qactive == QUEST_HIVE_DONE
		    || quests[Q_JERSEY]._qactive == QUEST_DONE || quests[Q_JERSEY]._qactive == QUEST_HIVE_DONE) {
			TownOpenHive();
		} else {
			TownCloseHive();
		}
		if (quests[Q_GRAVE]._qactive == QUEST_DONE || plr[myplr]._pLvlVisited[21])
			TownOpenGrave();
		else
			TownCloseGrave();
	}

	if (quests[Q_PWATER]._qactive != QUEST_DONE && quests[Q_PWATER]._qactive != QUEST_NOTAVAIL) {
		T_FillTile(P3Tiles.get(), 60, 70, 342);
	} else {
		T_FillTile(P3Tiles.get(), 60, 70, 71);
	}
}

} // namespace

/**
 * @brief Update the map to show the open hive
 */
void TownOpenHive()
{
	dPiece[78][60] = 0x48a;
	dPiece[79][60] = 0x48b;
	dPiece[78][61] = 0x48c;
	dPiece[79][61] = 0x50e;
	dPiece[78][62] = 0x4ee;
	dPiece[78][63] = 0x4f0;
	dPiece[79][62] = 0x510;
	dPiece[79][63] = 0x511;
	dPiece[79][64] = 0x512;
	dPiece[78][64] = 0x11a;
	dPiece[78][65] = 0x11c;
	dPiece[79][65] = 0x11d;
	dPiece[80][60] = 0x513;
	dPiece[80][61] = 0x515;
	dPiece[81][61] = 0x516;
	dPiece[82][60] = 0x517;
	dPiece[83][60] = 0x518;
	dPiece[82][61] = 0x519;
	dPiece[83][61] = 0x51a;
	dPiece[80][62] = 0x51b;
	dPiece[81][62] = 0x51c;
	dPiece[80][63] = 0x51d;
	dPiece[81][63] = 0x51e;
	dPiece[80][64] = 0x51f;
	dPiece[81][64] = 0x520;
	dPiece[80][65] = 0x521;
	dPiece[81][65] = 0x522;
	dPiece[82][64] = 0x527;
	dPiece[83][64] = 0x528;
	dPiece[82][65] = 0x529;
	dPiece[83][65] = 0x52a;
	dPiece[82][62] = 0x523;
	dPiece[83][62] = 0x524;
	dPiece[82][63] = 0x525;
	dPiece[83][63] = 0x526;
	dPiece[84][61] = 0x118;
	dPiece[84][62] = 0x118;
	dPiece[84][63] = 0x118;
	dPiece[85][60] = 0x118;
	dPiece[85][61] = 0x118;
	dPiece[85][63] = 8;
	dPiece[85][64] = 8;
	dPiece[86][60] = 0xd9;
	dPiece[86][61] = 0x18;
	dPiece[85][62] = 0x13;
	dPiece[84][64] = 0x118;
	SetDungeonMicros();
}

/**
 * @brief Update the map to show the open grave
 */
void TownOpenGrave()
{
	dPiece[36][21] = 0x533;
	dPiece[37][21] = 0x534;
	dPiece[36][22] = 0x535;
	dPiece[37][22] = 0x536;
	dPiece[36][23] = 0x537;
	dPiece[37][23] = 0x538;
	dPiece[36][24] = 0x539;
	dPiece[37][24] = 0x53a;
	dPiece[35][21] = 0x53b;
	dPiece[34][21] = 0x53c;
	SetDungeonMicros();
}

/**
 * @brief Initialize town level
 * @param entry Methode of entry
 */
void CreateTown(lvl_entry entry)
{
	int x, y;

	dminx = 10;
	dminy = 10;
	dmaxx = 84;
	dmaxy = 84;
	DRLG_InitTrans();
	DRLG_Init_Globals();

	if (entry == ENTRY_MAIN) { // New game
		ViewX = 75;
		ViewY = 68;
	} else if (entry == ENTRY_PREV) { // Cathedral
		ViewX = 25;
		ViewY = 31;
	} else if (entry == ENTRY_TWARPUP) {
		if (TWarpFrom == 5) {
			ViewX = 49;
			ViewY = 22;
		}
		if (TWarpFrom == 9) {
			ViewX = 18;
			ViewY = 69;
		}
		if (TWarpFrom == 13) {
			ViewX = 41;
			ViewY = 81;
		}
		if (TWarpFrom == 21) {
			ViewX = 36;
			ViewY = 25;
		}
		if (TWarpFrom == 17) {
			ViewX = 79;
			ViewY = 62;
		}
	}

	T_Pass3();
	memset(dFlags, 0, sizeof(dFlags));
	memset(dLight, 0, sizeof(dLight));
	memset(dFlags, 0, sizeof(dFlags));
	memset(dPlayer, 0, sizeof(dPlayer));
	memset(dMonster, 0, sizeof(dMonster));
	memset(dObject, 0, sizeof(dObject));
	memset(dItem, 0, sizeof(dItem));
	memset(dSpecial, 0, sizeof(dSpecial));

	for (y = 0; y < MAXDUNY; y++) {
		for (x = 0; x < MAXDUNX; x++) {
			if (dPiece[x][y] == 360) {
				dSpecial[x][y] = 1;
			} else if (dPiece[x][y] == 358) {
				dSpecial[x][y] = 2;
			} else if (dPiece[x][y] == 129) {
				dSpecial[x][y] = 6;
			} else if (dPiece[x][y] == 130) {
				dSpecial[x][y] = 7;
			} else if (dPiece[x][y] == 128) {
				dSpecial[x][y] = 8;
			} else if (dPiece[x][y] == 117) {
				dSpecial[x][y] = 9;
			} else if (dPiece[x][y] == 157) {
				dSpecial[x][y] = 10;
			} else if (dPiece[x][y] == 158) {
				dSpecial[x][y] = 11;
			} else if (dPiece[x][y] == 156) {
				dSpecial[x][y] = 12;
			} else if (dPiece[x][y] == 162) {
				dSpecial[x][y] = 13;
			} else if (dPiece[x][y] == 160) {
				dSpecial[x][y] = 14;
			} else if (dPiece[x][y] == 214) {
				dSpecial[x][y] = 15;
			} else if (dPiece[x][y] == 212) {
				dSpecial[x][y] = 16;
			} else if (dPiece[x][y] == 217) {
				dSpecial[x][y] = 17;
			} else if (dPiece[x][y] == 216) {
				dSpecial[x][y] = 18;
			}
		}
	}
}

} // namespace devilution
