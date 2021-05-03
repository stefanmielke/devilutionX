/**
 * @file objects.cpp
 *
 * Implementation of object functionality, interaction, spawning, loading, etc.
 */
#include <algorithm>
#include <climits>
#include <cstdint>

#include "automap.h"
#include "control.h"
#include "cursor.h"
#include "drlg_l1.h"
#include "drlg_l4.h"
#include "error.h"
#include "init.h"
#include "lighting.h"
#include "minitext.h"
#include "missiles.h"
#include "options.h"
#include "setmaps.h"
#include "stores.h"
#include "themes.h"
#include "towners.h"
#include "track.h"
#include "utils/language.h"
#include "utils/log.hpp"

namespace devilution {

enum shrine_type : uint8_t {
	SHRINE_MYSTERIOUS,
	SHRINE_HIDDEN,
	SHRINE_GLOOMY,
	SHRINE_WEIRD,
	SHRINE_MAGICAL,
	SHRINE_STONE,
	SHRINE_RELIGIOUS,
	SHRINE_ENCHANTED,
	SHRINE_THAUMATURGIC,
	SHRINE_FASCINATING,
	SHRINE_CRYPTIC,
	SHRINE_MAGICAL2,
	SHRINE_ELDRITCH,
	SHRINE_EERIE,
	SHRINE_DIVINE,
	SHRINE_HOLY,
	SHRINE_SACRED,
	SHRINE_SPIRITUAL,
	SHRINE_SPOOKY,
	SHRINE_ABANDONED,
	SHRINE_CREEPY,
	SHRINE_QUIET,
	SHRINE_SECLUDED,
	SHRINE_ORNATE,
	SHRINE_GLIMMERING,
	SHRINE_TAINTED,
	SHRINE_OILY,
	SHRINE_GLOWING,
	SHRINE_MENDICANT,
	SHRINE_SPARKLING,
	SHRINE_TOWN,
	SHRINE_SHIMMERING,
	SHRINE_SOLAR,
	SHRINE_MURPHYS,
	NUM_SHRINETYPE
};

int trapid;
int trapdir;
std::unique_ptr<BYTE[]> pObjCels[40];
object_graphic_id ObjFileList[40];
int objectactive[MAXOBJECTS];
/** Specifies the number of active objects. */
int nobjects;
int leverid;
int objectavail[MAXOBJECTS];
ObjectStruct object[MAXOBJECTS];
bool InitObjFlag;
bool LoadMapObjsFlag;
int numobjfiles;
int dword_6DE0E0;

/** Specifies the X-coordinate delta between barrels. */
int bxadd[8] = { -1, 0, 1, -1, 1, -1, 0, 1 };
/** Specifies the Y-coordinate delta between barrels. */
int byadd[8] = { -1, -1, -1, 0, 0, 1, 1, 1 };
/** Maps from shrine_id to shrine name. */
const char *const shrinestrs[] = {
	N_("Mysterious"),
	N_("Hidden"),
	N_("Gloomy"),
	N_("Weird"),
	N_("Magical"),
	N_("Stone"),
	N_("Religious"),
	N_("Enchanted"),
	N_("Thaumaturgic"),
	N_("Fascinating"),
	N_("Cryptic"),
	N_("Magical"),
	N_("Eldritch"),
	N_("Eerie"),
	N_("Divine"),
	N_("Holy"),
	N_("Sacred"),
	N_("Spiritual"),
	N_("Spooky"),
	N_("Abandoned"),
	N_("Creepy"),
	N_("Quiet"),
	N_("Secluded"),
	N_("Ornate"),
	N_("Glimmering"),
	N_("Tainted"),
	N_("Oily"),
	N_("Glowing"),
	N_("Mendicant's"),
	N_("Sparkling"),
	N_("Town"),
	N_("Shimmering"),
	N_("Solar"),
	N_("Murphy's"),
};
/** Specifies the minimum dungeon level on which each shrine will appear. */
char shrinemin[] = {
	1, // Mysterious
	1, // Hidden
	1, // Gloomy
	1, // Weird
	1, // Magical
	1, // Stone
	1, // Religious
	1, // Enchanted
	1, // Thaumaturgic
	1, // Fascinating
	1, // Cryptic
	1, // Magical
	1, // Eldritch
	1, // Eerie
	1, // Divine
	1, // Holy
	1, // Sacred
	1, // Spiritual
	1, // Spooky
	1, // Abandoned
	1, // Creepy
	1, // Quiet
	1, // Secluded
	1, // Ornate
	1, // Glimmering
	1, // Tainted
	1, // Oily
	1, // Glowing
	1, // Mendicant's
	1, // Sparkling
	1, // Town
	1, // Shimmering
	1, // Solar,
	1, // Murphy's
};

#define MAX_LVLS 24

/** Specifies the maximum dungeon level on which each shrine will appear. */
char shrinemax[] = {
	MAX_LVLS, // Mysterious
	MAX_LVLS, // Hidden
	MAX_LVLS, // Gloomy
	MAX_LVLS, // Weird
	MAX_LVLS, // Magical
	MAX_LVLS, // Stone
	MAX_LVLS, // Religious
	8,        // Enchanted
	MAX_LVLS, // Thaumaturgic
	MAX_LVLS, // Fascinating
	MAX_LVLS, // Cryptic
	MAX_LVLS, // Magical
	MAX_LVLS, // Eldritch
	MAX_LVLS, // Eerie
	MAX_LVLS, // Divine
	MAX_LVLS, // Holy
	MAX_LVLS, // Sacred
	MAX_LVLS, // Spiritual
	MAX_LVLS, // Spooky
	MAX_LVLS, // Abandoned
	MAX_LVLS, // Creepy
	MAX_LVLS, // Quiet
	MAX_LVLS, // Secluded
	MAX_LVLS, // Ornate
	MAX_LVLS, // Glimmering
	MAX_LVLS, // Tainted
	MAX_LVLS, // Oily
	MAX_LVLS, // Glowing
	MAX_LVLS, // Mendicant's
	MAX_LVLS, // Sparkling
	MAX_LVLS, // Town
	MAX_LVLS, // Shimmering
	MAX_LVLS, // Solar,
	MAX_LVLS, // Murphy's
};

/**
 * Specifies the game type for which each shrine may appear.
 * SHRINETYPE_ANY - sp & mp
 * SHRINETYPE_SINGLE - sp only
 * SHRINETYPE_MULTI - mp only
 */
enum shrine_gametype : uint8_t {
	SHRINETYPE_ANY,
	SHRINETYPE_SINGLE,
	SHRINETYPE_MULTI,
};

shrine_gametype shrineavail[] = {
	SHRINETYPE_ANY,    // SHRINE_MYSTERIOUS
	SHRINETYPE_ANY,    // SHRINE_HIDDEN
	SHRINETYPE_SINGLE, // SHRINE_GLOOMY
	SHRINETYPE_SINGLE, // SHRINE_WEIRD
	SHRINETYPE_ANY,    // SHRINE_MAGICAL
	SHRINETYPE_ANY,    // SHRINE_STONE
	SHRINETYPE_ANY,    // SHRINE_RELIGIOUS
	SHRINETYPE_ANY,    // SHRINE_ENCHANTED
	SHRINETYPE_SINGLE, // SHRINE_THAUMATURGIC
	SHRINETYPE_ANY,    // SHRINE_FASCINATING
	SHRINETYPE_ANY,    // SHRINE_CRYPTIC
	SHRINETYPE_ANY,    // SHRINE_MAGICAL2
	SHRINETYPE_ANY,    // SHRINE_ELDRITCH
	SHRINETYPE_ANY,    // SHRINE_EERIE
	SHRINETYPE_ANY,    // SHRINE_DIVINE
	SHRINETYPE_ANY,    // SHRINE_HOLY
	SHRINETYPE_ANY,    // SHRINE_SACRED
	SHRINETYPE_ANY,    // SHRINE_SPIRITUAL
	SHRINETYPE_MULTI,  // SHRINE_SPOOKY
	SHRINETYPE_ANY,    // SHRINE_ABANDONED
	SHRINETYPE_ANY,    // SHRINE_CREEPY
	SHRINETYPE_ANY,    // SHRINE_QUIET
	SHRINETYPE_ANY,    // SHRINE_SECLUDED
	SHRINETYPE_ANY,    // SHRINE_ORNATE
	SHRINETYPE_ANY,    // SHRINE_GLIMMERING
	SHRINETYPE_MULTI,  // SHRINE_TAINTED
	SHRINETYPE_ANY,    // SHRINE_OILY
	SHRINETYPE_ANY,    // SHRINE_GLOWING
	SHRINETYPE_ANY,    // SHRINE_MENDICANT
	SHRINETYPE_ANY,    // SHRINE_SPARKLING
	SHRINETYPE_ANY,    // SHRINE_TOWN
	SHRINETYPE_ANY,    // SHRINE_SHIMMERING
	SHRINETYPE_SINGLE, // SHRINE_SOLAR
	SHRINETYPE_ANY,    // SHRINE_MURPHYS
};
/** Maps from book_id to book name. */
const char *const StoryBookName[] = {
	N_("The Great Conflict"),
	N_("The Wages of Sin are War"),
	N_("The Tale of the Horadrim"),
	N_("The Dark Exile"),
	N_("The Sin War"),
	N_("The Binding of the Three"),
	N_("The Realms Beyond"),
	N_("Tale of the Three"),
	N_("The Black King"),
	N_("Journal: The Ensorcellment"),
	N_("Journal: The Meeting"),
	N_("Journal: The Tirade"),
	N_("Journal: His Power Grows"),
	N_("Journal: NA-KRUL"),
	N_("Journal: The End"),
	N_("A Spellbook"),
};
/** Specifies the speech IDs of each dungeon type narrator book, for each player class. */
_speech_id StoryText[3][3] = {
	{ TEXT_BOOK11, TEXT_BOOK12, TEXT_BOOK13 },
	{ TEXT_BOOK21, TEXT_BOOK22, TEXT_BOOK23 },
	{ TEXT_BOOK31, TEXT_BOOK32, TEXT_BOOK33 }
};

void InitObjectGFX()
{
	bool fileload[56];
	char filestr[32];
	int i, j;

	memset(fileload, false, sizeof(fileload));

	int lvl = currlevel;
	if (currlevel >= 21 && currlevel <= 24)
		lvl -= 20;
	else if (currlevel >= 17 && currlevel <= 20)
		lvl -= 8;
	for (i = 0; AllObjects[i].oload != -1; i++) {
		if (AllObjects[i].oload == 1
		    && (int)lvl >= AllObjects[i].ominlvl
		    && (int)lvl <= AllObjects[i].omaxlvl) {
			fileload[AllObjects[i].ofindex] = true;
		}
		if (AllObjects[i].otheme != THEME_NONE) {
			for (j = 0; j < numthemes; j++) {
				if (themes[j].ttype == AllObjects[i].otheme)
					fileload[AllObjects[i].ofindex] = true;
			}
		}

		if (AllObjects[i].oquest != -1) {
			if (QuestStatus(AllObjects[i].oquest))
				fileload[AllObjects[i].ofindex] = true;
		}
	}

	for (int i = OFILE_L1BRAZ; i <= OFILE_LZSTAND; i++) {
		if (fileload[i]) {
			ObjFileList[numobjfiles] = (object_graphic_id)i;
			sprintf(filestr, "Objects\\%s.CEL", ObjMasterLoadList[i]);
			if (currlevel >= 17 && currlevel < 21)
				sprintf(filestr, "Objects\\%s.CEL", ObjHiveLoadList[i]);
			else if (currlevel >= 21)
				sprintf(filestr, "Objects\\%s.CEL", ObjCryptLoadList[i]);
			pObjCels[numobjfiles] = LoadFileInMem<BYTE>(filestr);
			numobjfiles++;
		}
	}
}

void FreeObjectGFX()
{
	int i;

	for (i = 0; i < numobjfiles; i++) {
		pObjCels[i] = nullptr;
	}
	numobjfiles = 0;
}

bool RndLocOk(int xp, int yp)
{
	if (dMonster[xp][yp] != 0)
		return false;
	if (dPlayer[xp][yp] != 0)
		return false;
	if (dObject[xp][yp] != 0)
		return false;
	if ((dFlags[xp][yp] & BFLAG_POPULATED) != 0)
		return false;
	if (nSolidTable[dPiece[xp][yp]])
		return false;
	if (leveltype != DTYPE_CATHEDRAL || dPiece[xp][yp] <= 126 || dPiece[xp][yp] >= 144)
		return true;
	return false;
}

static bool WallTrapLocOkK(int xp, int yp)
{
	if ((dFlags[xp][yp] & BFLAG_POPULATED) != 0)
		return false;

	if (nTrapTable[dPiece[xp][yp]])
		return true;

	return false;
}

void InitRndLocObj(int min, int max, _object_id objtype)
{
	int i, xp, yp, numobjs;

	numobjs = GenerateRnd(max - min) + min;

	for (i = 0; i < numobjs; i++) {
		while (true) {
			xp = GenerateRnd(80) + 16;
			yp = GenerateRnd(80) + 16;
			if (RndLocOk(xp - 1, yp - 1)
			    && RndLocOk(xp, yp - 1)
			    && RndLocOk(xp + 1, yp - 1)
			    && RndLocOk(xp - 1, yp)
			    && RndLocOk(xp, yp)
			    && RndLocOk(xp + 1, yp)
			    && RndLocOk(xp - 1, yp + 1)
			    && RndLocOk(xp, yp + 1)
			    && RndLocOk(xp + 1, yp + 1)) {
				AddObject(objtype, xp, yp);
				break;
			}
		}
	}
}

void InitRndLocBigObj(int min, int max, _object_id objtype)
{
	int i, xp, yp, numobjs;

	numobjs = GenerateRnd(max - min) + min;
	for (i = 0; i < numobjs; i++) {
		while (true) {
			xp = GenerateRnd(80) + 16;
			yp = GenerateRnd(80) + 16;
			if (RndLocOk(xp - 1, yp - 2)
			    && RndLocOk(xp, yp - 2)
			    && RndLocOk(xp + 1, yp - 2)
			    && RndLocOk(xp - 1, yp - 1)
			    && RndLocOk(xp, yp - 1)
			    && RndLocOk(xp + 1, yp - 1)
			    && RndLocOk(xp - 1, yp)
			    && RndLocOk(xp, yp)
			    && RndLocOk(xp + 1, yp)
			    && RndLocOk(xp - 1, yp + 1)
			    && RndLocOk(xp, yp + 1)
			    && RndLocOk(xp + 1, yp + 1)) {
				AddObject(objtype, xp, yp);
				break;
			}
		}
	}
}

void InitRndLocObj5x5(int min, int max, _object_id objtype)
{
	bool exit;
	int xp, yp, numobjs, i, cnt, m, n;

	numobjs = min + GenerateRnd(max - min);
	for (i = 0; i < numobjs; i++) {
		cnt = 0;
		exit = false;
		while (!exit) {
			exit = true;
			xp = GenerateRnd(80) + 16;
			yp = GenerateRnd(80) + 16;
			for (n = -2; n <= 2; n++) {
				for (m = -2; m <= 2; m++) {
					if (!RndLocOk(xp + m, yp + n))
						exit = false;
				}
			}
			if (!exit) {
				cnt++;
				if (cnt > 20000)
					return;
			}
		}
		AddObject(objtype, xp, yp);
	}
}

void ClrAllObjects()
{
	int i;

	memset(object, 0, sizeof(object));
	nobjects = 0;
	for (i = 0; i < MAXOBJECTS; i++) {
		objectavail[i] = i;
	}
	memset(objectactive, 0, sizeof(objectactive));
	trapdir = 0;
	trapid = 1;
	leverid = 1;
}

void AddTortures()
{
	int ox, oy;

	for (oy = 0; oy < MAXDUNY; oy++) {
		for (ox = 0; ox < MAXDUNX; ox++) {
			if (dPiece[ox][oy] == 367) {
				AddObject(OBJ_TORTURE1, ox, oy + 1);
				AddObject(OBJ_TORTURE3, ox + 2, oy - 1);
				AddObject(OBJ_TORTURE2, ox, oy + 3);
				AddObject(OBJ_TORTURE4, ox + 4, oy - 1);
				AddObject(OBJ_TORTURE5, ox, oy + 5);
				AddObject(OBJ_TNUDEM1, ox + 1, oy + 3);
				AddObject(OBJ_TNUDEM2, ox + 4, oy + 5);
				AddObject(OBJ_TNUDEM3, ox + 2, oy);
				AddObject(OBJ_TNUDEM4, ox + 3, oy + 2);
				AddObject(OBJ_TNUDEW1, ox + 2, oy + 4);
				AddObject(OBJ_TNUDEW2, ox + 2, oy + 1);
				AddObject(OBJ_TNUDEW3, ox + 4, oy + 2);
			}
		}
	}
}
void AddCandles()
{
	int tx, ty;

	tx = quests[Q_PWATER].position.x;
	ty = quests[Q_PWATER].position.y;
	AddObject(OBJ_STORYCANDLE, tx - 2, ty + 1);
	AddObject(OBJ_STORYCANDLE, tx + 3, ty + 1);
	AddObject(OBJ_STORYCANDLE, tx - 1, ty + 2);
	AddObject(OBJ_STORYCANDLE, tx + 2, ty + 2);
}

void AddBookLever(int x1, int y1, int x2, int y2, _speech_id msg)
{
	bool exit;
	int xp, yp, ob, cnt, m, n;

	cnt = 0;
	exit = false;
	while (!exit) {
		exit = true;
		xp = GenerateRnd(80) + 16;
		yp = GenerateRnd(80) + 16;
		for (n = -2; n <= 2; n++) {
			for (m = -2; m <= 2; m++) {
				if (!RndLocOk(xp + m, yp + n))
					exit = false;
			}
		}
		if (!exit) {
			cnt++;
			if (cnt > 20000)
				return;
		}
	}

	if (QuestStatus(Q_BLIND))
		AddObject(OBJ_BLINDBOOK, xp, yp);
	if (QuestStatus(Q_WARLORD))
		AddObject(OBJ_STEELTOME, xp, yp);
	if (QuestStatus(Q_BLOOD)) {
		xp = 2 * setpc_x + 25;
		yp = 2 * setpc_y + 40;
		AddObject(OBJ_BLOODBOOK, xp, yp);
	}
	ob = dObject[xp][yp] - 1;
	SetObjMapRange(ob, x1, y1, x2, y2, leverid);
	SetBookMsg(ob, msg);
	leverid++;
	object[ob]._oVar6 = object[ob]._oAnimFrame + 1;
}

void InitRndBarrels()
{
	int numobjs; // number of groups of barrels to generate
	int xp, yp;
	_object_id o;
	bool found;
	int p; // regulates chance to stop placing barrels in current group
	int dir;
	int t; // number of tries of placing next barrel in current group
	int c; // number of barrels in current group
	int i;

	numobjs = GenerateRnd(5) + 3;
	for (i = 0; i < numobjs; i++) {
		do {
			xp = GenerateRnd(80) + 16;
			yp = GenerateRnd(80) + 16;
		} while (!RndLocOk(xp, yp));
		o = (GenerateRnd(4) != 0) ? OBJ_BARREL : OBJ_BARRELEX;
		AddObject(o, xp, yp);
		found = true;
		p = 0;
		c = 1;
		while (GenerateRnd(p) == 0 && found) {
			t = 0;
			found = false;
			while (true) {
				if (t >= 3)
					break;
				dir = GenerateRnd(8);
				xp += bxadd[dir];
				yp += byadd[dir];
				found = RndLocOk(xp, yp);
				t++;
				if (found)
					break;
			}
			if (found) {
				o = (GenerateRnd(5) != 0) ? OBJ_BARREL : OBJ_BARRELEX;
				AddObject(o, xp, yp);
				c++;
			}
			p = c / 2;
		}
	}
}

void AddL1Objs(int x1, int y1, int x2, int y2)
{
	int i, j, pn;

	for (j = y1; j < y2; j++) {
		for (i = x1; i < x2; i++) {
			pn = dPiece[i][j];
			if (pn == 270)
				AddObject(OBJ_L1LIGHT, i, j);
			if (pn == 44 || pn == 51 || pn == 214)
				AddObject(OBJ_L1LDOOR, i, j);
			if (pn == 46 || pn == 56)
				AddObject(OBJ_L1RDOOR, i, j);
		}
	}
}

void add_crypt_objs(int x1, int y1, int x2, int y2)
{
	int i, j, pn;

	for (j = y1; j < y2; j++) {
		for (i = x1; i < x2; i++) {
			pn = dPiece[i][j];
			if (pn == 77)
				AddObject(OBJ_L1LDOOR, i, j);
			if (pn == 80)
				AddObject(OBJ_L1RDOOR, i, j);
		}
	}
}

void AddL2Objs(int x1, int y1, int x2, int y2)
{
	int i, j, pn;

	for (j = y1; j < y2; j++) {
		for (i = x1; i < x2; i++) {
			pn = dPiece[i][j];
			if (pn == 13 || pn == 541)
				AddObject(OBJ_L2LDOOR, i, j);
			if (pn == 17 || pn == 542)
				AddObject(OBJ_L2RDOOR, i, j);
		}
	}
}

void AddL3Objs(int x1, int y1, int x2, int y2)
{
	int i, j, pn;

	for (j = y1; j < y2; j++) {
		for (i = x1; i < x2; i++) {
			pn = dPiece[i][j];
			if (pn == 531)
				AddObject(OBJ_L3LDOOR, i, j);
			if (pn == 534)
				AddObject(OBJ_L3RDOOR, i, j);
		}
	}
}

bool TorchLocOK(int xp, int yp)
{
	if ((dFlags[xp][yp] & BFLAG_POPULATED) != 0)
		return false;
	return true;
}

void AddL2Torches()
{
	int i, j, pn;

	for (j = 0; j < MAXDUNY; j++) {
		for (i = 0; i < MAXDUNX; i++) {
			if (!TorchLocOK(i, j))
				continue;

			pn = dPiece[i][j];
			if (pn == 1 && GenerateRnd(3) == 0)
				AddObject(OBJ_TORCHL2, i, j);

			if (pn == 5 && GenerateRnd(3) == 0)
				AddObject(OBJ_TORCHR2, i, j);

			if (pn == 37 && GenerateRnd(10) == 0 && dObject[i - 1][j] == 0)
				AddObject(OBJ_TORCHL, i - 1, j);

			if (pn == 41 && GenerateRnd(10) == 0 && dObject[i][j - 1] == 0)
				AddObject(OBJ_TORCHR, i, j - 1);
		}
	}
}

void AddObjTraps()
{
	int8_t oi_trap, oi;
	int i, j;
	int xp, yp;
	int rndv;

	if (currlevel == 1)
		rndv = 10;
	if (currlevel >= 2)
		rndv = 15;
	if (currlevel >= 5)
		rndv = 20;
	if (currlevel >= 7)
		rndv = 25;
	for (j = 0; j < MAXDUNY; j++) {
		for (i = 0; i < MAXDUNX; i++) {
			if (dObject[i][j] <= 0 || GenerateRnd(100) >= rndv)
				continue;

			oi = dObject[i][j] - 1;
			if (!AllObjects[object[oi]._otype].oTrapFlag)
				continue;

			if (GenerateRnd(2) == 0) {
				xp = i - 1;
				while (!nSolidTable[dPiece[xp][j]])
					xp--;

				if (!WallTrapLocOkK(xp, j) || i - xp <= 1)
					continue;

				AddObject(OBJ_TRAPL, xp, j);
				oi_trap = dObject[xp][j] - 1;
				object[oi_trap]._oVar1 = i;
				object[oi_trap]._oVar2 = j;
				object[oi]._oTrapFlag = true;
			} else {
				yp = j - 1;
				while (!nSolidTable[dPiece[i][yp]])
					yp--;

				if (!WallTrapLocOkK(i, yp) || j - yp <= 1)
					continue;

				AddObject(OBJ_TRAPR, i, yp);
				oi_trap = dObject[i][yp] - 1;
				object[oi_trap]._oVar1 = i;
				object[oi_trap]._oVar2 = j;
				object[oi]._oTrapFlag = true;
			}
		}
	}
}

void AddChestTraps()
{
	int i, j;
	int8_t oi;

	for (j = 0; j < MAXDUNY; j++) {
		for (i = 0; i < MAXDUNX; i++) {
			if (dObject[i][j] > 0) {
				oi = dObject[i][j] - 1;
				if (object[oi]._otype >= OBJ_CHEST1 && object[oi]._otype <= OBJ_CHEST3 && !object[oi]._oTrapFlag && GenerateRnd(100) < 10) {
					switch (object[oi]._otype) {
					case OBJ_CHEST1:
						object[oi]._otype = OBJ_TCHEST1;
						break;
					case OBJ_CHEST2:
						object[oi]._otype = OBJ_TCHEST2;
						break;
					case OBJ_CHEST3:
						object[oi]._otype = OBJ_TCHEST3;
						break;
					default:
						break;
					}
					object[oi]._oTrapFlag = true;
					if (leveltype == DTYPE_CATACOMBS) {
						object[oi]._oVar4 = GenerateRnd(2);
					} else {
						object[oi]._oVar4 = GenerateRnd(gbIsHellfire ? 6 : 3);
					}
				}
			}
		}
	}
}

void LoadMapObjects(const char *path, int startx, int starty, int x1, int y1, int w, int h, int leveridx)
{
	auto dunData = LoadFileInMem<uint16_t>(path);

	int width = SDL_SwapLE16(dunData[0]);
	int height = SDL_SwapLE16(dunData[1]);

	LoadMapObjsFlag = true;
	InitObjFlag = true;

	int layer2Offset = 2 + width * height;

	// The rest of the layers are at dPiece scale
	width *= 2;
	height *= 2;

	uint16_t *objectLayer = &dunData[layer2Offset + width * height * 2];

	for (int j = 0; j < height; j++) {
		for (int i = 0; i < width; i++) {
			uint8_t objectId = SDL_SwapLE16(objectLayer[j * width + i]);
			if (objectId != 0) {
				AddObject(ObjTypeConv[objectId], startx + 16 + i, starty + 16 + j);
				int oi = ObjIndex(startx + 16 + i, starty + 16 + j);
				SetObjMapRange(oi, x1, y1, x1 + w, y1 + h, leveridx);
			}
		}
	}

	InitObjFlag = false;
	LoadMapObjsFlag = false;
}

void LoadMapObjs(const char *path, int startx, int starty)
{
	auto dunData = LoadFileInMem<uint16_t>(path);

	int width = SDL_SwapLE16(dunData[0]);
	int height = SDL_SwapLE16(dunData[1]);

	LoadMapObjsFlag = true;
	InitObjFlag = true;

	int layer2Offset = 2 + width * height;

	// The rest of the layers are at dPiece scale
	width *= 2;
	height *= 2;

	uint16_t *objectLayer = &dunData[layer2Offset + width * height * 2];

	for (int j = 0; j < height; j++) {
		for (int i = 0; i < width; i++) {
			uint8_t objectId = SDL_SwapLE16(objectLayer[j * width + i]);
			if (objectId != 0) {
				AddObject(ObjTypeConv[objectId], startx + 16 + i, starty + 16 + j);
			}
		}
	}

	InitObjFlag = false;
	LoadMapObjsFlag = false;
}

void AddDiabObjs()
{
	LoadMapObjects("Levels\\L4Data\\diab1.DUN", 2 * diabquad1x, 2 * diabquad1y, diabquad2x, diabquad2y, 11, 12, 1);
	LoadMapObjects("Levels\\L4Data\\diab2a.DUN", 2 * diabquad2x, 2 * diabquad2y, diabquad3x, diabquad3y, 11, 11, 2);
	LoadMapObjects("Levels\\L4Data\\diab3a.DUN", 2 * diabquad3x, 2 * diabquad3y, diabquad4x, diabquad4y, 9, 9, 3);
}

void objects_add_lv22(int s)
{
	bool exit;
	int xp, yp, cnt, m, n;

	cnt = 0;
	exit = false;
	while (!exit) {
		exit = true;
		xp = GenerateRnd(80) + 16;
		yp = GenerateRnd(80) + 16;
		for (n = -2; n <= 2; n++) {
			for (m = -3; m <= 3; m++) {
				if (!RndLocOk(xp + m, yp + n))
					exit = false;
			}
		}
		if (!exit) {
			cnt++;
			if (cnt > 20000)
				return;
		}
	}
	objects_44D8C5(OBJ_STORYBOOK, s, xp, yp);
	AddObject(OBJ_STORYCANDLE, xp - 2, yp + 1);
	AddObject(OBJ_STORYCANDLE, xp - 2, yp);
	AddObject(OBJ_STORYCANDLE, xp - 1, yp - 1);
	AddObject(OBJ_STORYCANDLE, xp + 1, yp - 1);
	AddObject(OBJ_STORYCANDLE, xp + 2, yp);
	AddObject(OBJ_STORYCANDLE, xp + 2, yp + 1);
}

void objects_add_lv24()
{
	objects_rnd_454BEA();
	switch (GenerateRnd(6)) {
	case 0:
		objects_454AF0(6, UberRow + 3, UberCol);
		objects_454AF0(7, UberRow + 2, UberCol - 3);
		objects_454AF0(8, UberRow + 2, UberCol + 2);
		break;
	case 1:
		objects_454AF0(6, UberRow + 3, UberCol);
		objects_454AF0(8, UberRow + 2, UberCol - 3);
		objects_454AF0(7, UberRow + 2, UberCol + 2);
		break;
	case 2:
		objects_454AF0(7, UberRow + 3, UberCol);
		objects_454AF0(6, UberRow + 2, UberCol - 3);
		objects_454AF0(8, UberRow + 2, UberCol + 2);
		break;
	case 3:
		objects_454AF0(7, UberRow + 3, UberCol);
		objects_454AF0(8, UberRow + 2, UberCol - 3);
		objects_454AF0(6, UberRow + 2, UberCol + 2);
		break;
	case 4:
		objects_454AF0(8, UberRow + 3, UberCol);
		objects_454AF0(7, UberRow + 2, UberCol - 3);
		objects_454AF0(6, UberRow + 2, UberCol + 2);
		break;
	case 5:
		objects_454AF0(8, UberRow + 3, UberCol);
		objects_454AF0(6, UberRow + 2, UberCol - 3);
		objects_454AF0(7, UberRow + 2, UberCol + 2);
		break;
	}
}

void objects_454AF0(int a1, int a2, int a3)
{
	objects_44D8C5(OBJ_STORYBOOK, a1, a2, a3);
}

void AddStoryBooks()
{
	int xp, yp, xx, yy;
	int cnt;
	bool done;

	cnt = 0;
	done = false;
	while (!done) {
		done = true;
		xp = GenerateRnd(80) + 16;
		yp = GenerateRnd(80) + 16;
		for (yy = -2; yy <= 2; yy++) {
			for (xx = -3; xx <= 3; xx++) {
				if (!RndLocOk(xx + xp, yy + yp))
					done = false;
			}
		}
		if (!done) {
			cnt++;
			if (cnt > 20000)
				return;
		}
	}
	AddObject(OBJ_STORYBOOK, xp, yp);
	AddObject(OBJ_STORYCANDLE, xp - 2, yp + 1);
	AddObject(OBJ_STORYCANDLE, xp - 2, yp);
	AddObject(OBJ_STORYCANDLE, xp - 1, yp - 1);
	AddObject(OBJ_STORYCANDLE, xp + 1, yp - 1);
	AddObject(OBJ_STORYCANDLE, xp + 2, yp);
	AddObject(OBJ_STORYCANDLE, xp + 2, yp + 1);
}

void AddHookedBodies(int freq)
{
	int i, j, ii, jj;

	for (j = 0; j < DMAXY; j++) {
		jj = 16 + j * 2;
		for (i = 0; i < DMAXX; i++) {
			ii = 16 + i * 2;
			if (dungeon[i][j] != 1 && dungeon[i][j] != 2)
				continue;
			if (GenerateRnd(freq) != 0)
				continue;
			if (!SkipThemeRoom(i, j))
				continue;
			if (dungeon[i][j] == 1 && dungeon[i + 1][j] == 6) {
				switch (GenerateRnd(3)) {
				case 0:
					AddObject(OBJ_TORTURE1, ii + 1, jj);
					break;
				case 1:
					AddObject(OBJ_TORTURE2, ii + 1, jj);
					break;
				case 2:
					AddObject(OBJ_TORTURE5, ii + 1, jj);
					break;
				}
				continue;
			}
			if (dungeon[i][j] == 2 && dungeon[i][j + 1] == 6) {
				switch (GenerateRnd(2)) {
				case 0:
					AddObject(OBJ_TORTURE3, ii, jj);
					break;
				case 1:
					AddObject(OBJ_TORTURE4, ii, jj);
					break;
				}
			}
		}
	}
}

void AddL4Goodies()
{
	AddHookedBodies(6);
	InitRndLocObj(2, 6, OBJ_TNUDEM1);
	InitRndLocObj(2, 6, OBJ_TNUDEM2);
	InitRndLocObj(2, 6, OBJ_TNUDEM3);
	InitRndLocObj(2, 6, OBJ_TNUDEM4);
	InitRndLocObj(2, 6, OBJ_TNUDEW1);
	InitRndLocObj(2, 6, OBJ_TNUDEW2);
	InitRndLocObj(2, 6, OBJ_TNUDEW3);
	InitRndLocObj(2, 6, OBJ_DECAP);
	InitRndLocObj(1, 3, OBJ_CAULDRON);
}

void AddLazStand()
{
	int xp, yp, xx, yy;
	int cnt;
	bool found;

	cnt = 0;
	found = false;
	while (!found) {
		found = true;
		xp = GenerateRnd(80) + 16;
		yp = GenerateRnd(80) + 16;
		for (yy = -3; yy <= 3; yy++) {
			for (xx = -2; xx <= 3; xx++) {
				if (!RndLocOk(xp + xx, yp + yy))
					found = false;
			}
		}
		if (!found) {
			cnt++;
			if (cnt > 10000) {
				InitRndLocObj(1, 1, OBJ_LAZSTAND);
				return;
			}
		}
	}
	AddObject(OBJ_LAZSTAND, xp, yp);
	AddObject(OBJ_TNUDEM2, xp, yp + 2);
	AddObject(OBJ_STORYCANDLE, xp + 1, yp + 2);
	AddObject(OBJ_TNUDEM3, xp + 2, yp + 2);
	AddObject(OBJ_TNUDEW1, xp, yp - 2);
	AddObject(OBJ_STORYCANDLE, xp + 1, yp - 2);
	AddObject(OBJ_TNUDEW2, xp + 2, yp - 2);
	AddObject(OBJ_STORYCANDLE, xp - 1, yp - 1);
	AddObject(OBJ_TNUDEW3, xp - 1, yp);
	AddObject(OBJ_STORYCANDLE, xp - 1, yp + 1);
}

void InitObjects()
{
	ClrAllObjects();
	dword_6DE0E0 = 0;
	if (currlevel == 16) {
		AddDiabObjs();
	} else {
		InitObjFlag = true;
		AdvanceRndSeed();
		if (currlevel == 9 && !gbIsMultiplayer)
			AddSlainHero();
		if (currlevel == quests[Q_MUSHROOM]._qlevel && quests[Q_MUSHROOM]._qactive == QUEST_INIT)
			AddMushPatch();

		if (currlevel == 4 || currlevel == 8 || currlevel == 12)
			AddStoryBooks();
		if (currlevel == 21) {
			objects_add_lv22(1);
		} else if (currlevel == 22) {
			objects_add_lv22(2);
			objects_add_lv22(3);
		} else if (currlevel == 23) {
			objects_add_lv22(4);
			objects_add_lv22(5);
		}
		if (currlevel == 24) {
			objects_add_lv24();
		}
		if (leveltype == DTYPE_CATHEDRAL) {
			if (QuestStatus(Q_BUTCHER))
				AddTortures();
			if (QuestStatus(Q_PWATER))
				AddCandles();
			if (QuestStatus(Q_LTBANNER))
				AddObject(OBJ_SIGNCHEST, 2 * setpc_x + 26, 2 * setpc_y + 19);
			InitRndLocBigObj(10, 15, OBJ_SARC);
			if (currlevel >= 21)
				add_crypt_objs(0, 0, MAXDUNX, MAXDUNY);
			else
				AddL1Objs(0, 0, MAXDUNX, MAXDUNY);
			InitRndBarrels();
		}
		if (leveltype == DTYPE_CATACOMBS) {
			if (QuestStatus(Q_ROCK))
				InitRndLocObj5x5(1, 1, OBJ_STAND);
			if (QuestStatus(Q_SCHAMB))
				InitRndLocObj5x5(1, 1, OBJ_BOOK2R);
			AddL2Objs(0, 0, MAXDUNX, MAXDUNY);
			AddL2Torches();
			if (QuestStatus(Q_BLIND)) {
				_speech_id sp_id;
				switch (plr[myplr]._pClass) {
				case HeroClass::Warrior:
					sp_id = TEXT_BLINDING;
					break;
				case HeroClass::Rogue:
					sp_id = TEXT_RBLINDING;
					break;
				case HeroClass::Sorcerer:
					sp_id = TEXT_MBLINDING;
					break;
				case HeroClass::Monk:
					sp_id = TEXT_HBLINDING;
					break;
				case HeroClass::Bard:
					sp_id = TEXT_BBLINDING;
					break;
				case HeroClass::Barbarian:
					sp_id = TEXT_BLINDING;
					break;
				}
				quests[Q_BLIND]._qmsg = sp_id;
				AddBookLever(setpc_x, setpc_y, setpc_w + setpc_x + 1, setpc_h + setpc_y + 1, sp_id);
				LoadMapObjs("Levels\\L2Data\\Blind2.DUN", 2 * setpc_x, 2 * setpc_y);
			}
			if (QuestStatus(Q_BLOOD)) {
				_speech_id sp_id;
				switch (plr[myplr]._pClass) {
				case HeroClass::Warrior:
					sp_id = TEXT_BLOODY;
					break;
				case HeroClass::Rogue:
					sp_id = TEXT_RBLOODY;
					break;
				case HeroClass::Sorcerer:
					sp_id = TEXT_MBLOODY;
					break;
				case HeroClass::Monk:
					sp_id = TEXT_HBLOODY;
					break;
				case HeroClass::Bard:
					sp_id = TEXT_BBLOODY;
					break;
				case HeroClass::Barbarian:
					sp_id = TEXT_BLOODY;
					break;
				}
				quests[Q_BLOOD]._qmsg = sp_id;
				AddBookLever(setpc_x, setpc_y + 3, setpc_x + 2, setpc_y + 7, sp_id);
				AddObject(OBJ_PEDISTAL, 2 * setpc_x + 25, 2 * setpc_y + 32);
			}
			InitRndBarrels();
		}
		if (leveltype == DTYPE_CAVES) {
			AddL3Objs(0, 0, MAXDUNX, MAXDUNY);
			InitRndBarrels();
		}
		if (leveltype == DTYPE_HELL) {
			if (QuestStatus(Q_WARLORD)) {
				_speech_id sp_id;
				switch (plr[myplr]._pClass) {
				case HeroClass::Warrior:
					sp_id = TEXT_BLOODWAR;
					break;
				case HeroClass::Rogue:
					sp_id = TEXT_RBLOODWAR;
					break;
				case HeroClass::Sorcerer:
					sp_id = TEXT_MBLOODWAR;
					break;
				case HeroClass::Monk:
					sp_id = TEXT_HBLOODWAR;
					break;
				case HeroClass::Bard:
					sp_id = TEXT_BBLOODWAR;
					break;
				case HeroClass::Barbarian:
					sp_id = TEXT_BLOODWAR;
					break;
				}
				quests[Q_WARLORD]._qmsg = sp_id;
				AddBookLever(setpc_x, setpc_y, setpc_x + setpc_w, setpc_y + setpc_h, sp_id);
				LoadMapObjs("Levels\\L4Data\\Warlord.DUN", 2 * setpc_x, 2 * setpc_y);
			}
			if (QuestStatus(Q_BETRAYER) && !gbIsMultiplayer)
				AddLazStand();
			InitRndBarrels();
			AddL4Goodies();
		}
		InitRndLocObj(5, 10, OBJ_CHEST1);
		InitRndLocObj(3, 6, OBJ_CHEST2);
		InitRndLocObj(1, 5, OBJ_CHEST3);
		if (leveltype != DTYPE_HELL)
			AddObjTraps();
		if (leveltype > DTYPE_CATHEDRAL)
			AddChestTraps();
		InitObjFlag = false;
	}
}

void SetMapObjects(const uint16_t *dunData, int startx, int starty)
{
	bool filesLoaded[56];
	char filestr[32];

	ClrAllObjects();
	for (auto &fileLoaded : filesLoaded)
		fileLoaded = false;
	InitObjFlag = true;

	for (int i = 0; AllObjects[i].oload != -1; i++) {
		if (AllObjects[i].oload == 1 && leveltype == AllObjects[i].olvltype)
			filesLoaded[AllObjects[i].ofindex] = true;
	}

	int width = SDL_SwapLE16(dunData[0]);
	int height = SDL_SwapLE16(dunData[1]);

	int layer2Offset = 2 + width * height;

	// The rest of the layers are at dPiece scale
	width *= 2;
	height *= 2;

	const uint16_t *objectLayer = &dunData[layer2Offset + width * height * 2];

	for (int j = 0; j < height; j++) {
		for (int i = 0; i < width; i++) {
			uint8_t objectId = SDL_SwapLE16(objectLayer[j * width + i]);
			if (objectId != 0) {
				filesLoaded[AllObjects[ObjTypeConv[objectId]].ofindex] = true;
			}
		}
	}

	for (int i = OFILE_L1BRAZ; i <= OFILE_LZSTAND; i++) {
		if (!filesLoaded[i])
			continue;

		ObjFileList[numobjfiles] = (object_graphic_id)i;
		sprintf(filestr, "Objects\\%s.CEL", ObjMasterLoadList[i]);
		pObjCels[numobjfiles] = LoadFileInMem<BYTE>(filestr);
		numobjfiles++;
	}

	for (int j = 0; j < height; j++) {
		for (int i = 0; i < width; i++) {
			uint8_t objectId = SDL_SwapLE16(objectLayer[j * width + i]);
			if (objectId != 0) {
				AddObject(ObjTypeConv[objectId], startx + 16 + i, starty + 16 + j);
			}
		}
	}

	InitObjFlag = false;
}

void DeleteObject_(int oi, int i)
{
	int ox, oy;

	ox = object[oi].position.x;
	oy = object[oi].position.y;
	dObject[ox][oy] = 0;
	objectavail[-nobjects + MAXOBJECTS] = oi;
	nobjects--;
	if (nobjects > 0 && i != nobjects)
		objectactive[i] = objectactive[nobjects];
}

void SetupObject(int i, int x, int y, _object_id ot)
{
	object[i]._otype = ot;
	object_graphic_id ofi = AllObjects[ot].ofindex;
	object[i].position = { x, y };

	const auto &found = std::find(std::begin(ObjFileList), std::end(ObjFileList), ofi);
	if (found == std::end(ObjFileList)) {
		LogCritical("Unable to find object_graphic_id {} in list of objects to load, level generation error.", ofi);
		return;
	}

	const int j = std::distance(std::begin(ObjFileList), found);

	object[i]._oAnimData = pObjCels[j].get();
	object[i]._oAnimFlag = AllObjects[ot].oAnimFlag;
	if (AllObjects[ot].oAnimFlag != 0) {
		object[i]._oAnimDelay = AllObjects[ot].oAnimDelay;
		object[i]._oAnimCnt = GenerateRnd(AllObjects[ot].oAnimDelay);
		object[i]._oAnimLen = AllObjects[ot].oAnimLen;
		object[i]._oAnimFrame = GenerateRnd(AllObjects[ot].oAnimLen - 1) + 1;
	} else {
		object[i]._oAnimDelay = 1000;
		object[i]._oAnimCnt = 0;
		object[i]._oAnimLen = AllObjects[ot].oAnimLen;
		object[i]._oAnimFrame = AllObjects[ot].oAnimDelay;
	}
	object[i]._oAnimWidth = AllObjects[ot].oAnimWidth;
	object[i]._oSolidFlag = AllObjects[ot].oSolidFlag;
	object[i]._oMissFlag = AllObjects[ot].oMissFlag;
	object[i]._oLight = AllObjects[ot].oLightFlag;
	object[i]._oDelFlag = false;
	object[i]._oBreak = AllObjects[ot].oBreak;
	object[i]._oSelFlag = AllObjects[ot].oSelFlag;
	object[i]._oPreFlag = false;
	object[i]._oTrapFlag = false;
	object[i]._oDoorFlag = false;
}

void SetObjMapRange(int i, int x1, int y1, int x2, int y2, int v)
{
	object[i]._oVar1 = x1;
	object[i]._oVar2 = y1;
	object[i]._oVar3 = x2;
	object[i]._oVar4 = y2;
	object[i]._oVar8 = v;
}

void SetBookMsg(int i, _speech_id msg)
{
	object[i]._oVar7 = msg;
}

void AddL1Door(int i, int x, int y, int ot)
{
	object[i]._oDoorFlag = true;
	if (ot == 1) {
		object[i]._oVar1 = dPiece[x][y];
		object[i]._oVar2 = dPiece[x][y - 1];
	} else {
		object[i]._oVar1 = dPiece[x][y];
		object[i]._oVar2 = dPiece[x - 1][y];
	}
	object[i]._oVar4 = 0;
}

void AddSCambBook(int i)
{
	object[i]._oVar1 = setpc_x;
	object[i]._oVar2 = setpc_y;
	object[i]._oVar3 = setpc_w + setpc_x + 1;
	object[i]._oVar4 = setpc_h + setpc_y + 1;
	object[i]._oVar6 = object[i]._oAnimFrame + 1;
}

void AddChest(int i, int t)
{
	if (GenerateRnd(2) == 0)
		object[i]._oAnimFrame += 3;
	object[i]._oRndSeed = AdvanceRndSeed();
	switch (t) {
	case OBJ_CHEST1:
	case OBJ_TCHEST1:
		if (setlevel) {
			object[i]._oVar1 = 1;
			break;
		}
		object[i]._oVar1 = GenerateRnd(2);
		break;
	case OBJ_TCHEST2:
	case OBJ_CHEST2:
		if (setlevel) {
			object[i]._oVar1 = 2;
			break;
		}
		object[i]._oVar1 = GenerateRnd(3);
		break;
	case OBJ_TCHEST3:
	case OBJ_CHEST3:
		if (setlevel) {
			object[i]._oVar1 = 3;
			break;
		}
		object[i]._oVar1 = GenerateRnd(4);
		break;
	}
	object[i]._oVar2 = GenerateRnd(8);
}

void AddL2Door(int i, int x, int y, int ot)
{
	object[i]._oDoorFlag = true;
	if (ot == OBJ_L2LDOOR)
		ObjSetMicro(x, y, 538);
	else
		ObjSetMicro(x, y, 540);
	dSpecial[x][y] = 0;
	object[i]._oVar4 = 0;
}

void AddL3Door(int i, int x, int y, int ot)
{
	object[i]._oDoorFlag = true;
	if (ot == OBJ_L3LDOOR)
		ObjSetMicro(x, y, 531);
	else
		ObjSetMicro(x, y, 534);
	object[i]._oVar4 = 0;
}

void AddSarc(int i)
{
	dObject[object[i].position.x][object[i].position.y - 1] = -(i + 1);
	object[i]._oVar1 = GenerateRnd(10);
	object[i]._oRndSeed = AdvanceRndSeed();
	if (object[i]._oVar1 >= 8)
		object[i]._oVar2 = PreSpawnSkeleton();
}

void AddFlameTrap(int i)
{
	object[i]._oVar1 = trapid;
	object[i]._oVar2 = 0;
	object[i]._oVar3 = trapdir;
	object[i]._oVar4 = 0;
}

void AddFlameLvr(int i)
{
	object[i]._oVar1 = trapid;
	object[i]._oVar2 = MIS_FLAMEC;
}

void AddTrap(int i)
{
	int mt;

	mt = currlevel / 3 + 1;
	if (currlevel > 16) {
		mt = (currlevel - 4) / 3 + 1;
	}
	if (currlevel > 20) {
		mt = (currlevel - 8) / 3 + 1;
	}
	mt = GenerateRnd(mt);
	if (mt == 0)
		object[i]._oVar3 = MIS_ARROW;
	if (mt == 1)
		object[i]._oVar3 = MIS_FIREBOLT;
	if (mt == 2)
		object[i]._oVar3 = MIS_LIGHTCTRL;
	object[i]._oVar4 = 0;
}

void AddObjLight(int i, int r)
{
	if (InitObjFlag) {
		DoLighting(object[i].position.x, object[i].position.y, r, -1);
		object[i]._oVar1 = -1;
	} else {
		object[i]._oVar1 = 0;
	}
}

void AddBarrel(int i, int t)
{
	object[i]._oVar1 = 0;
	object[i]._oRndSeed = AdvanceRndSeed();
	object[i]._oVar2 = (t == OBJ_BARRELEX) ? 0 : GenerateRnd(10);
	object[i]._oVar3 = GenerateRnd(3);

	if (object[i]._oVar2 >= 8)
		object[i]._oVar4 = PreSpawnSkeleton();
}

void AddShrine(int i)
{
	int val;
	bool slist[NUM_SHRINETYPE];
	int j;
	object[i]._oPreFlag = true;

	int shrines = gbIsHellfire ? NUM_SHRINETYPE : 26;

	for (j = 0; j < shrines; j++) {
		slist[j] = currlevel >= shrinemin[j] && currlevel <= shrinemax[j];
		if (gbIsMultiplayer && shrineavail[j] == SHRINETYPE_SINGLE) {
			slist[j] = false;
		} else if (!gbIsMultiplayer && shrineavail[j] == SHRINETYPE_MULTI) {
			slist[j] = false;
		}
	}
	do {
		val = GenerateRnd(shrines);
	} while (!slist[val]);

	object[i]._oVar1 = val;
	if (GenerateRnd(2) != 0) {
		object[i]._oAnimFrame = 12;
		object[i]._oAnimLen = 22;
	}
}

void AddBookcase(int i)
{
	object[i]._oRndSeed = AdvanceRndSeed();
	object[i]._oPreFlag = true;
}

void AddBookstand(int i)
{
	object[i]._oRndSeed = AdvanceRndSeed();
}

void AddBloodFtn(int i)
{
	object[i]._oRndSeed = AdvanceRndSeed();
}

void AddPurifyingFountain(int i)
{
	int ox, oy;

	ox = object[i].position.x;
	oy = object[i].position.y;
	dObject[ox][oy - 1] = -(i + 1);
	dObject[ox - 1][oy] = -(i + 1);
	dObject[ox - 1][oy - 1] = -(i + 1);
	object[i]._oRndSeed = AdvanceRndSeed();
}

void AddArmorStand(int i)
{
	if (!armorFlag) {
		object[i]._oAnimFlag = 2;
		object[i]._oSelFlag = 0;
	}

	object[i]._oRndSeed = AdvanceRndSeed();
}

void AddGoatShrine(int i)
{
	object[i]._oRndSeed = AdvanceRndSeed();
}

void AddCauldron(int i)
{
	object[i]._oRndSeed = AdvanceRndSeed();
}

void AddMurkyFountain(int i)
{
	int ox, oy;

	ox = object[i].position.x;
	oy = object[i].position.y;
	dObject[ox][oy - 1] = -(i + 1);
	dObject[ox - 1][oy] = -(i + 1);
	dObject[ox - 1][oy - 1] = -(i + 1);
	object[i]._oRndSeed = AdvanceRndSeed();
}

void AddTearFountain(int i)
{
	object[i]._oRndSeed = AdvanceRndSeed();
}

void AddDecap(int i)
{
	object[i]._oRndSeed = AdvanceRndSeed();
	object[i]._oAnimFrame = GenerateRnd(8) + 1;
	object[i]._oPreFlag = true;
}

void AddVilebook(int i)
{
	if (setlevel && setlvlnum == SL_VILEBETRAYER) {
		object[i]._oAnimFrame = 4;
	}
}

void AddMagicCircle(int i)
{
	object[i]._oRndSeed = AdvanceRndSeed();
	object[i]._oPreFlag = true;
	object[i]._oVar6 = 0;
	object[i]._oVar5 = 1;
}

void AddBrnCross(int i)
{
	object[i]._oRndSeed = AdvanceRndSeed();
}

void AddPedistal(int i)
{
	object[i]._oVar1 = setpc_x;
	object[i]._oVar2 = setpc_y;
	object[i]._oVar3 = setpc_x + setpc_w;
	object[i]._oVar4 = setpc_y + setpc_h;
	object[i]._oVar6 = 0;
}

void AddStoryBook(int i)
{
	SetRndSeed(glSeedTbl[16]);

	object[i]._oVar1 = GenerateRnd(3);
	if (currlevel == 4)
		object[i]._oVar2 = StoryText[object[i]._oVar1][0];
	else if (currlevel == 8)
		object[i]._oVar2 = StoryText[object[i]._oVar1][1];
	else if (currlevel == 12)
		object[i]._oVar2 = StoryText[object[i]._oVar1][2];
	object[i]._oVar3 = (currlevel / 4) + 3 * object[i]._oVar1 - 1;
	object[i]._oAnimFrame = 5 - 2 * object[i]._oVar1;
	object[i]._oVar4 = object[i]._oAnimFrame + 1;
}

void AddWeaponRack(int i)
{
	if (!weaponFlag) {
		object[i]._oAnimFlag = 2;
		object[i]._oSelFlag = 0;
	}
	object[i]._oRndSeed = AdvanceRndSeed();
}

void AddTorturedBody(int i)
{
	object[i]._oRndSeed = AdvanceRndSeed();
	object[i]._oAnimFrame = GenerateRnd(4) + 1;
	object[i]._oPreFlag = true;
}

void GetRndObjLoc(int randarea, int *xx, int *yy)
{
	bool failed;
	int i, j, tries;

	if (randarea == 0)
		return;

	tries = 0;
	while (true) {
		tries++;
		if (tries > 1000 && randarea > 1)
			randarea--;
		*xx = GenerateRnd(MAXDUNX);
		*yy = GenerateRnd(MAXDUNY);
		failed = false;
		for (i = 0; i < randarea && !failed; i++) {
			for (j = 0; j < randarea && !failed; j++) {
				failed = !RndLocOk(i + *xx, j + *yy);
			}
		}
		if (!failed)
			break;
	}
}

void AddMushPatch()
{
	int i;
	int y, x;

	if (nobjects < MAXOBJECTS) {
		i = objectavail[0];
		GetRndObjLoc(5, &x, &y);
		dObject[x + 1][y + 1] = -(i + 1);
		dObject[x + 2][y + 1] = -(i + 1);
		dObject[x + 1][y + 2] = -(i + 1);
		AddObject(OBJ_MUSHPATCH, x + 2, y + 2);
	}
}

void AddSlainHero()
{
	int x, y;

	GetRndObjLoc(5, &x, &y);
	AddObject(OBJ_SLAINHERO, x + 2, y + 2);
}

void objects_44D8C5(_object_id ot, int v2, int ox, int oy)
{
	int oi;

	if (nobjects >= MAXOBJECTS)
		return;

	oi = objectavail[0];
	objectavail[0] = objectavail[MAXOBJECTS - 1 - nobjects];
	objectactive[nobjects] = oi;
	dObject[ox][oy] = oi + 1;
	SetupObject(oi, ox, oy, ot);
	objects_44DA68(oi, v2);
	nobjects++;
}

void objects_44DA68(int i, int a2)
{
	int v8, v9;
	if (a2 > 5) {
		object[i]._oVar8 = a2;
		switch (a2) {
		case 6:
			if (plr[myplr]._pClass == HeroClass::Warrior) {
				object[i]._oVar2 = TEXT_BOOKA;
			} else if (plr[myplr]._pClass == HeroClass::Rogue) {
				object[i]._oVar2 = TEXT_RBOOKA;
			} else if (plr[myplr]._pClass == HeroClass::Sorcerer) {
				object[i]._oVar2 = TEXT_MBOOKA;
			} else if (plr[myplr]._pClass == HeroClass::Monk) {
				object[i]._oVar2 = TEXT_OBOOKA;
			} else if (plr[myplr]._pClass == HeroClass::Bard) {
				object[i]._oVar2 = TEXT_BBOOKA;
			} else if (plr[myplr]._pClass == HeroClass::Barbarian) {
				object[i]._oVar2 = TEXT_BOOKA;
			}
			break;
		case 7:
			if (plr[myplr]._pClass == HeroClass::Warrior) {
				object[i]._oVar2 = TEXT_BOOKB;
			} else if (plr[myplr]._pClass == HeroClass::Rogue) {
				object[i]._oVar2 = TEXT_RBOOKB;
			} else if (plr[myplr]._pClass == HeroClass::Sorcerer) {
				object[i]._oVar2 = TEXT_MBOOKB;
			} else if (plr[myplr]._pClass == HeroClass::Monk) {
				object[i]._oVar2 = TEXT_OBOOKB;
			} else if (plr[myplr]._pClass == HeroClass::Bard) {
				object[i]._oVar2 = TEXT_BBOOKB;
			} else if (plr[myplr]._pClass == HeroClass::Barbarian) {
				object[i]._oVar2 = TEXT_BOOKB;
			}
			break;
		case 8:
			if (plr[myplr]._pClass == HeroClass::Warrior) {
				object[i]._oVar2 = TEXT_BOOKC;
			} else if (plr[myplr]._pClass == HeroClass::Rogue) {
				object[i]._oVar2 = TEXT_RBOOKC;
			} else if (plr[myplr]._pClass == HeroClass::Sorcerer) {
				object[i]._oVar2 = TEXT_MBOOKC;
			} else if (plr[myplr]._pClass == HeroClass::Monk) {
				object[i]._oVar2 = TEXT_OBOOKC;
			} else if (plr[myplr]._pClass == HeroClass::Bard) {
				object[i]._oVar2 = TEXT_BBOOKC;
			} else if (plr[myplr]._pClass == HeroClass::Barbarian) {
				object[i]._oVar2 = TEXT_BOOKC;
			}
			break;
		}
		object[i]._oVar1 = 1;
		object[i]._oVar3 = 15;
		v8 = 2 * object[i]._oVar1;
		object[i]._oAnimFrame = 5 - v8;
		object[i]._oVar4 = object[i]._oAnimFrame + 1;
	} else {

		object[i]._oVar1 = 1;
		object[i]._oVar2 = a2 + TEXT_SKLJRN;
		object[i]._oVar3 = a2 + 9;
		v9 = 2 * object[i]._oVar1;
		object[i]._oAnimFrame = 5 - v9;
		object[i]._oVar4 = object[i]._oAnimFrame + 1;
		object[i]._oVar8 = 0;
	}
}

void AddObject(_object_id ot, int ox, int oy)
{
	int oi;

	if (nobjects >= MAXOBJECTS)
		return;

	oi = objectavail[0];
	objectavail[0] = objectavail[MAXOBJECTS - 1 - nobjects];
	objectactive[nobjects] = oi;
	dObject[ox][oy] = oi + 1;
	SetupObject(oi, ox, oy, ot);
	switch (ot) {
	case OBJ_L1LIGHT:
		AddObjLight(oi, 5);
		break;
	case OBJ_SKFIRE:
	case OBJ_CANDLE1:
	case OBJ_CANDLE2:
	case OBJ_BOOKCANDLE:
		AddObjLight(oi, 5);
		break;
	case OBJ_STORYCANDLE:
		AddObjLight(oi, 3);
		break;
	case OBJ_TORCHL:
	case OBJ_TORCHR:
	case OBJ_TORCHL2:
	case OBJ_TORCHR2:
		AddObjLight(oi, 8);
		break;
	case OBJ_L1LDOOR:
	case OBJ_L1RDOOR:
		AddL1Door(oi, ox, oy, ot);
		break;
	case OBJ_L2LDOOR:
	case OBJ_L2RDOOR:
		AddL2Door(oi, ox, oy, ot);
		break;
	case OBJ_L3LDOOR:
	case OBJ_L3RDOOR:
		AddL3Door(oi, ox, oy, ot);
		break;
	case OBJ_BOOK2R:
		AddSCambBook(oi);
		break;
	case OBJ_CHEST1:
	case OBJ_CHEST2:
	case OBJ_CHEST3:
		AddChest(oi, ot);
		break;
	case OBJ_TCHEST1:
	case OBJ_TCHEST2:
	case OBJ_TCHEST3:
		AddChest(oi, ot);
		object[oi]._oTrapFlag = true;
		if (leveltype == DTYPE_CATACOMBS) {
			object[oi]._oVar4 = GenerateRnd(2);
		} else {
			object[oi]._oVar4 = GenerateRnd(3);
		}
		break;
	case OBJ_SARC:
		AddSarc(oi);
		break;
	case OBJ_FLAMEHOLE:
		AddFlameTrap(oi);
		break;
	case OBJ_FLAMELVR:
		AddFlameLvr(oi);
		break;
	case OBJ_WATER:
		object[oi]._oAnimFrame = 1;
		break;
	case OBJ_TRAPL:
	case OBJ_TRAPR:
		AddTrap(oi);
		break;
	case OBJ_BARREL:
	case OBJ_BARRELEX:
		AddBarrel(oi, ot);
		break;
	case OBJ_SHRINEL:
	case OBJ_SHRINER:
		AddShrine(oi);
		break;
	case OBJ_BOOKCASEL:
	case OBJ_BOOKCASER:
		AddBookcase(oi);
		break;
	case OBJ_SKELBOOK:
	case OBJ_BOOKSTAND:
		AddBookstand(oi);
		break;
	case OBJ_BLOODFTN:
		AddBloodFtn(oi);
		break;
	case OBJ_DECAP:
		AddDecap(oi);
		break;
	case OBJ_PURIFYINGFTN:
		AddPurifyingFountain(oi);
		break;
	case OBJ_ARMORSTAND:
	case OBJ_WARARMOR:
		AddArmorStand(oi);
		break;
	case OBJ_GOATSHRINE:
		AddGoatShrine(oi);
		break;
	case OBJ_CAULDRON:
		AddCauldron(oi);
		break;
	case OBJ_MURKYFTN:
		AddMurkyFountain(oi);
		break;
	case OBJ_TEARFTN:
		AddTearFountain(oi);
		break;
	case OBJ_BOOK2L:
		AddVilebook(oi);
		break;
	case OBJ_MCIRCLE1:
	case OBJ_MCIRCLE2:
		AddMagicCircle(oi);
		break;
	case OBJ_STORYBOOK:
		AddStoryBook(oi);
		break;
	case OBJ_BCROSS:
	case OBJ_TBCROSS:
		AddBrnCross(oi);
		AddObjLight(oi, 5);
		break;
	case OBJ_PEDISTAL:
		AddPedistal(oi);
		break;
	case OBJ_WARWEAP:
	case OBJ_WEAPONRACK:
		AddWeaponRack(oi);
		break;
	case OBJ_TNUDEM2:
		AddTorturedBody(oi);
		break;
	default:
		break;
	}
	nobjects++;
}

void Obj_Light(int i, int lr)
{
	int ox, oy, dx, dy, p, tr;
	bool turnon;

	turnon = false;
	if (object[i]._oVar1 != -1) {
		ox = object[i].position.x;
		oy = object[i].position.y;
		tr = lr + 10;
		if (!lightflag) {
			for (p = 0; p < MAX_PLRS && !turnon; p++) {
				if (plr[p].plractive) {
					if (currlevel == plr[p].plrlevel) {
						dx = abs(plr[p].position.tile.x - ox);
						dy = abs(plr[p].position.tile.y - oy);
						if (dx < tr && dy < tr)
							turnon = true;
					}
				}
			}
		}
		if (turnon) {
			if (object[i]._oVar1 == 0)
				object[i]._olid = AddLight(ox, oy, lr);
			object[i]._oVar1 = 1;
		} else {
			if (object[i]._oVar1 == 1)
				AddUnLight(object[i]._olid);
			object[i]._oVar1 = 0;
		}
	}
}

void Obj_Circle(int i)
{
	int ox, oy, wx, wy;

	ox = object[i].position.x;
	oy = object[i].position.y;
	wx = plr[myplr].position.tile.x;
	wy = plr[myplr].position.tile.y;
	if (wx == ox && wy == oy) {
		if (object[i]._otype == OBJ_MCIRCLE1)
			object[i]._oAnimFrame = 2;
		if (object[i]._otype == OBJ_MCIRCLE2)
			object[i]._oAnimFrame = 4;
		if (ox == 45 && oy == 47) {
			object[i]._oVar6 = 2;
		} else if (ox == 26 && oy == 46) {
			object[i]._oVar6 = 1;
		} else {
			object[i]._oVar6 = 0;
		}
		if (ox == 35 && oy == 36 && object[i]._oVar5 == 3) {
			object[i]._oVar6 = 4;
			ObjChangeMapResync(object[i]._oVar1, object[i]._oVar2, object[i]._oVar3, object[i]._oVar4);
			if (quests[Q_BETRAYER]._qactive == QUEST_ACTIVE && quests[Q_BETRAYER]._qvar1 <= 4) // BUGFIX stepping on the circle again will break the quest state (fixed)
				quests[Q_BETRAYER]._qvar1 = 4;
			AddMissile(plr[myplr].position.tile.x, plr[myplr].position.tile.y, 35, 46, plr[myplr]._pdir, MIS_RNDTELEPORT, TARGET_MONSTERS, myplr, 0, 0);
			track_repeat_walk(false);
			sgbMouseDown = CLICK_NONE;
			ClrPlrPath(myplr);
			StartStand(myplr, DIR_S);
		}
	} else {
		if (object[i]._otype == OBJ_MCIRCLE1)
			object[i]._oAnimFrame = 1;
		if (object[i]._otype == OBJ_MCIRCLE2)
			object[i]._oAnimFrame = 3;
		object[i]._oVar6 = 0;
	}
}

void Obj_StopAnim(int i)
{
	if (object[i]._oAnimFrame == object[i]._oAnimLen) {
		object[i]._oAnimCnt = 0;
		object[i]._oAnimDelay = 1000;
	}
}

void Obj_Door(int i)
{
	int dx, dy;
	bool dok;

	if (object[i]._oVar4 == 0) {
		object[i]._oSelFlag = 3;
		object[i]._oMissFlag = false;
	} else {
		dx = object[i].position.x;
		dy = object[i].position.y;
		dok = dMonster[dx][dy] == 0;
		dok = dok && dItem[dx][dy] == 0;
		dok = dok && dDead[dx][dy] == 0;
		dok = dok && dPlayer[dx][dy] == 0;
		object[i]._oSelFlag = 2;
		object[i]._oVar4 = dok ? 1 : 2;
		object[i]._oMissFlag = true;
	}
}

void Obj_Sarc(int i)
{
	if (object[i]._oAnimFrame == object[i]._oAnimLen)
		object[i]._oAnimFlag = 0;
}

void ActivateTrapLine(int ttype, int tid)
{
	int i, oi;

	for (i = 0; i < nobjects; i++) {
		oi = objectactive[i];
		if (object[oi]._otype == ttype && object[oi]._oVar1 == tid) {
			object[oi]._oVar4 = 1;
			object[oi]._oAnimFlag = 1;
			object[oi]._oAnimDelay = 1;
			object[oi]._olid = AddLight(object[oi].position.x, object[oi].position.y, 1);
		}
	}
}

void Obj_FlameTrap(int i)
{
	int x, y;
	int j, k;

	if (object[i]._oVar2 != 0) {
		if (object[i]._oVar4 != 0) {
			object[i]._oAnimFrame--;
			if (object[i]._oAnimFrame == 1) {
				object[i]._oVar4 = 0;
				AddUnLight(object[i]._olid);
			} else if (object[i]._oAnimFrame <= 4) {
				ChangeLightRadius(object[i]._olid, object[i]._oAnimFrame);
			}
		}
	} else if (object[i]._oVar4 == 0) {
		if (object[i]._oVar3 == 2) {
			x = object[i].position.x - 2;
			y = object[i].position.y;
			for (j = 0; j < 5; j++) {
				if (dPlayer[x][y] != 0 || dMonster[x][y] != 0)
					object[i]._oVar4 = 1;
				x++;
			}
		} else {
			x = object[i].position.x;
			y = object[i].position.y - 2;
			for (k = 0; k < 5; k++) {
				if (dPlayer[x][y] != 0 || dMonster[x][y] != 0)
					object[i]._oVar4 = 1;
				y++;
			}
		}
		if (object[i]._oVar4 != 0)
			ActivateTrapLine(object[i]._otype, object[i]._oVar1);
	} else {
		int damage[4] = { 6, 8, 10, 12 };

		int mindam = damage[leveltype - 1];
		int maxdam = mindam * 2;

		x = object[i].position.x;
		y = object[i].position.y;
		if (dMonster[x][y] > 0)
			MonsterTrapHit(dMonster[x][y] - 1, mindam / 2, maxdam / 2, 0, MIS_FIREWALLC, false);
		if (dPlayer[x][y] > 0) {
			bool unused;
			PlayerMHit(dPlayer[x][y] - 1, -1, 0, mindam, maxdam, MIS_FIREWALLC, false, 0, &unused);
		}

		if (object[i]._oAnimFrame == object[i]._oAnimLen)
			object[i]._oAnimFrame = 11;
		if (object[i]._oAnimFrame <= 5)
			ChangeLightRadius(object[i]._olid, object[i]._oAnimFrame);
	}
}

void Obj_Trap(int i)
{
	if (object[i]._oVar4 != 0)
		return;

	int oti = dObject[object[i]._oVar1][object[i]._oVar2] - 1;
	switch (object[oti]._otype) {
	case OBJ_L1LDOOR:
	case OBJ_L1RDOOR:
	case OBJ_L2LDOOR:
	case OBJ_L2RDOOR:
	case OBJ_L3LDOOR:
	case OBJ_L3RDOOR:
		if (object[oti]._oVar4 == 0)
			return;
		break;
	case OBJ_LEVER:
	case OBJ_CHEST1:
	case OBJ_CHEST2:
	case OBJ_CHEST3:
	case OBJ_SWITCHSKL:
	case OBJ_SARC:
		if (object[oti]._oSelFlag != 0)
			return;
		break;
	default:
		return;
	}

	object[i]._oVar4 = 1;
	int dx = object[oti].position.x;
	int dy = object[oti].position.y;
	for (int y = dy - 1; y <= object[oti].position.y + 1; y++) {
		for (int x = object[oti].position.x - 1; x <= object[oti].position.x + 1; x++) {
			if (dPlayer[x][y] != 0) {
				dx = x;
				dy = y;
			}
		}
	}
	if (!deltaload) {
		direction dir = GetDirection(object[i].position, object[oti].position);
		int sx = object[i].position.x;
		int sy = object[i].position.y;
		AddMissile(sx, sy, dx, dy, dir, object[i]._oVar3, TARGET_PLAYERS, -1, 0, 0);
		PlaySfxLoc(IS_TRAP, object[oti].position.x, object[oti].position.y);
	}
	object[oti]._oTrapFlag = false;
}

void Obj_BCrossDamage(int i)
{
	int fire_resist;
	int damage[4] = { 6, 8, 10, 12 };

	if (plr[myplr]._pmode == PM_DEATH)
		return;

	fire_resist = plr[myplr]._pFireResist;
	if (fire_resist > 0)
		damage[leveltype - 1] -= fire_resist * damage[leveltype - 1] / 100;

	if (plr[myplr].position.tile.x != object[i].position.x || plr[myplr].position.tile.y != object[i].position.y - 1)
		return;

	ApplyPlrDamage(myplr, 0, 0, damage[leveltype - 1]);
	if (plr[myplr]._pHitPoints >> 6 > 0) {
		plr[myplr].PlaySpeach(68);
	}
}

void ProcessObjects()
{
	int oi;
	int i;

	for (i = 0; i < nobjects; ++i) {
		oi = objectactive[i];
		switch (object[oi]._otype) {
		case OBJ_L1LIGHT:
			Obj_Light(oi, 10);
			break;
		case OBJ_SKFIRE:
		case OBJ_CANDLE2:
		case OBJ_BOOKCANDLE:
			Obj_Light(oi, 5);
			break;
		case OBJ_STORYCANDLE:
			Obj_Light(oi, 3);
			break;
		case OBJ_CRUX1:
		case OBJ_CRUX2:
		case OBJ_CRUX3:
		case OBJ_BARREL:
		case OBJ_BARRELEX:
		case OBJ_SHRINEL:
		case OBJ_SHRINER:
			Obj_StopAnim(oi);
			break;
		case OBJ_L1LDOOR:
		case OBJ_L1RDOOR:
		case OBJ_L2LDOOR:
		case OBJ_L2RDOOR:
		case OBJ_L3LDOOR:
		case OBJ_L3RDOOR:
			Obj_Door(oi);
			break;
		case OBJ_TORCHL:
		case OBJ_TORCHR:
		case OBJ_TORCHL2:
		case OBJ_TORCHR2:
			Obj_Light(oi, 8);
			break;
		case OBJ_SARC:
			Obj_Sarc(oi);
			break;
		case OBJ_FLAMEHOLE:
			Obj_FlameTrap(oi);
			break;
		case OBJ_TRAPL:
		case OBJ_TRAPR:
			Obj_Trap(oi);
			break;
		case OBJ_MCIRCLE1:
		case OBJ_MCIRCLE2:
			Obj_Circle(oi);
			break;
		case OBJ_BCROSS:
		case OBJ_TBCROSS:
			Obj_Light(oi, 10);
			Obj_BCrossDamage(oi);
			break;
		default:
			break;
		}
		if (object[oi]._oAnimFlag == 0)
			continue;

		object[oi]._oAnimCnt++;

		if (object[oi]._oAnimCnt < object[oi]._oAnimDelay)
			continue;

		object[oi]._oAnimCnt = 0;
		object[oi]._oAnimFrame++;
		if (object[oi]._oAnimFrame > object[oi]._oAnimLen)
			object[oi]._oAnimFrame = 1;
	}
	i = 0;
	while (i < nobjects) {
		oi = objectactive[i];
		if (object[oi]._oDelFlag) {
			DeleteObject_(oi, i);
			i = 0;
		} else {
			i++;
		}
	}
}

void ObjSetMicro(int dx, int dy, int pn)
{
	uint16_t *v;
	MICROS *defs;
	int i;

	dPiece[dx][dy] = pn;
	pn--;
	defs = &dpiece_defs_map_2[dx][dy];
	if (leveltype != DTYPE_HELL) {
		v = (uint16_t *)pLevelPieces.get() + 10 * pn;
		for (i = 0; i < 10; i++) {
			defs->mt[i] = SDL_SwapLE16(v[(i & 1) - (i & 0xE) + 8]);
		}
	} else {
		v = (uint16_t *)pLevelPieces.get() + 16 * pn;
		for (i = 0; i < 16; i++) {
			defs->mt[i] = SDL_SwapLE16(v[(i & 1) - (i & 0xE) + 14]);
		}
	}
}

void objects_set_door_piece(int x, int y)
{
	int pn;
	long v1, v2;

	pn = dPiece[x][y] - 1;

	v1 = *((uint16_t *)pLevelPieces.get() + 10 * pn + 8);
	v2 = *((uint16_t *)pLevelPieces.get() + 10 * pn + 9);
	dpiece_defs_map_2[x][y].mt[0] = SDL_SwapLE16(v1);
	dpiece_defs_map_2[x][y].mt[1] = SDL_SwapLE16(v2);
}

void ObjSetMini(int x, int y, int v)
{
	int xx, yy;
	long v1, v2, v3, v4;

	uint16_t *MegaTiles = (uint16_t *)&pMegaTiles[((uint16_t)v - 1) * 8];
	v1 = SDL_SwapLE16(*(MegaTiles + 0)) + 1;
	v2 = SDL_SwapLE16(*(MegaTiles + 1)) + 1;
	v3 = SDL_SwapLE16(*(MegaTiles + 2)) + 1;
	v4 = SDL_SwapLE16(*(MegaTiles + 3)) + 1;

	xx = 2 * x + 16;
	yy = 2 * y + 16;
	ObjSetMicro(xx, yy, v1);
	ObjSetMicro(xx + 1, yy, v2);
	ObjSetMicro(xx, yy + 1, v3);
	ObjSetMicro(xx + 1, yy + 1, v4);
}

void ObjL1Special(int x1, int y1, int x2, int y2)
{
	int i, j;

	for (i = y1; i <= y2; ++i) {
		for (j = x1; j <= x2; ++j) {
			dSpecial[j][i] = 0;
			if (dPiece[j][i] == 12)
				dSpecial[j][i] = 1;
			if (dPiece[j][i] == 11)
				dSpecial[j][i] = 2;
			if (dPiece[j][i] == 71)
				dSpecial[j][i] = 1;
			if (dPiece[j][i] == 253)
				dSpecial[j][i] = 3;
			if (dPiece[j][i] == 267)
				dSpecial[j][i] = 6;
			if (dPiece[j][i] == 259)
				dSpecial[j][i] = 5;
			if (dPiece[j][i] == 249)
				dSpecial[j][i] = 2;
			if (dPiece[j][i] == 325)
				dSpecial[j][i] = 2;
			if (dPiece[j][i] == 321)
				dSpecial[j][i] = 1;
			if (dPiece[j][i] == 255)
				dSpecial[j][i] = 4;
			if (dPiece[j][i] == 211)
				dSpecial[j][i] = 1;
			if (dPiece[j][i] == 344)
				dSpecial[j][i] = 2;
			if (dPiece[j][i] == 341)
				dSpecial[j][i] = 1;
			if (dPiece[j][i] == 331)
				dSpecial[j][i] = 2;
			if (dPiece[j][i] == 418)
				dSpecial[j][i] = 1;
			if (dPiece[j][i] == 421)
				dSpecial[j][i] = 2;
		}
	}
}

void ObjL2Special(int x1, int y1, int x2, int y2)
{
	int i, j;

	for (j = y1; j <= y2; j++) {
		for (i = x1; i <= x2; i++) {
			dSpecial[i][j] = 0;
			if (dPiece[i][j] == 541)
				dSpecial[i][j] = 5;
			if (dPiece[i][j] == 178)
				dSpecial[i][j] = 5;
			if (dPiece[i][j] == 551)
				dSpecial[i][j] = 5;
			if (dPiece[i][j] == 542)
				dSpecial[i][j] = 6;
			if (dPiece[i][j] == 553)
				dSpecial[i][j] = 6;
		}
	}
	for (j = y1; j <= y2; j++) {
		for (i = x1; i <= x2; i++) {
			if (dPiece[i][j] == 132) {
				dSpecial[i][j + 1] = 2;
				dSpecial[i][j + 2] = 1;
			}
			if (dPiece[i][j] == 135 || dPiece[i][j] == 139) {
				dSpecial[i + 1][j] = 3;
				dSpecial[i + 2][j] = 4;
			}
		}
	}
}

void DoorSet(int oi, int dx, int dy)
{
	int pn;

	pn = dPiece[dx][dy];
	if (currlevel < 17) {
		if (pn == 43)
			ObjSetMicro(dx, dy, 392);
		if (pn == 45)
			ObjSetMicro(dx, dy, 394);
		if (pn == 50 && object[oi]._otype == OBJ_L1LDOOR)
			ObjSetMicro(dx, dy, 411);
		if (pn == 50 && object[oi]._otype == OBJ_L1RDOOR)
			ObjSetMicro(dx, dy, 412);
		if (pn == 54)
			ObjSetMicro(dx, dy, 397);
		if (pn == 55)
			ObjSetMicro(dx, dy, 398);
		if (pn == 61)
			ObjSetMicro(dx, dy, 399);
		if (pn == 67)
			ObjSetMicro(dx, dy, 400);
		if (pn == 68)
			ObjSetMicro(dx, dy, 401);
		if (pn == 69)
			ObjSetMicro(dx, dy, 403);
		if (pn == 70)
			ObjSetMicro(dx, dy, 404);
		if (pn == 72)
			ObjSetMicro(dx, dy, 406);
		if (pn == 212)
			ObjSetMicro(dx, dy, 407);
		if (pn == 354)
			ObjSetMicro(dx, dy, 409);
		if (pn == 355)
			ObjSetMicro(dx, dy, 410);
		if (pn == 411)
			ObjSetMicro(dx, dy, 396);
		if (pn == 412)
			ObjSetMicro(dx, dy, 396);
	} else {
		if (pn == 75)
			ObjSetMicro(dx, dy, 204);
		if (pn == 79)
			ObjSetMicro(dx, dy, 208);
		if (pn == 86 && object[oi]._otype == OBJ_L1LDOOR) {
			ObjSetMicro(dx, dy, 232);
		}
		if (pn == 86 && object[oi]._otype == OBJ_L1RDOOR) {
			ObjSetMicro(dx, dy, 234);
		}
		if (pn == 91)
			ObjSetMicro(dx, dy, 215);
		if (pn == 93)
			ObjSetMicro(dx, dy, 218);
		if (pn == 99)
			ObjSetMicro(dx, dy, 220);
		if (pn == 111)
			ObjSetMicro(dx, dy, 222);
		if (pn == 113)
			ObjSetMicro(dx, dy, 224);
		if (pn == 115)
			ObjSetMicro(dx, dy, 226);
		if (pn == 117)
			ObjSetMicro(dx, dy, 228);
		if (pn == 119)
			ObjSetMicro(dx, dy, 230);
		if (pn == 232)
			ObjSetMicro(dx, dy, 212);
		if (pn == 234)
			ObjSetMicro(dx, dy, 212);
	}
}

void RedoPlayerVision()
{
	int p;

	for (p = 0; p < MAX_PLRS; p++) {
		if (plr[p].plractive && currlevel == plr[p].plrlevel) {
			ChangeVisionXY(plr[p]._pvid, plr[p].position.tile.x, plr[p].position.tile.y);
		}
	}
}

void OperateL1RDoor(int pnum, int oi, bool sendflag)
{
	int xp, yp;

	if (object[oi]._oVar4 == 2) {
		if (!deltaload)
			PlaySfxLoc(IS_DOORCLOS, object[oi].position.x, object[oi].position.y);
		return;
	}

	xp = object[oi].position.x;
	yp = object[oi].position.y;
	if (object[oi]._oVar4 == 0) {
		if (pnum == myplr && sendflag)
			NetSendCmdParam1(true, CMD_OPENDOOR, oi);
		if (currlevel < 21) {
			if (!deltaload)
				PlaySfxLoc(IS_DOOROPEN, object[oi].position.x, object[oi].position.y);
			ObjSetMicro(xp, yp, 395);
		} else {
			if (!deltaload)
				PlaySfxLoc(IS_CROPEN, object[oi].position.x, object[oi].position.y);
			ObjSetMicro(xp, yp, 209);
		}
		if (currlevel < 17) {
			dSpecial[xp][yp] = 8;
		} else {
			dSpecial[xp][yp] = 2;
		}
		objects_set_door_piece(xp, yp - 1);
		object[oi]._oAnimFrame += 2;
		object[oi]._oPreFlag = true;
		DoorSet(oi, xp - 1, yp);
		object[oi]._oVar4 = 1;
		object[oi]._oSelFlag = 2;
		RedoPlayerVision();
		return;
	}

	if (currlevel < 21) {
		if (!deltaload)
			PlaySfxLoc(IS_DOORCLOS, xp, object[oi].position.y);
	} else {
		if (!deltaload)
			PlaySfxLoc(IS_CRCLOS, xp, object[oi].position.y);
	}
	if (!deltaload && dDead[xp][yp] == 0 && dMonster[xp][yp] == 0 && dItem[xp][yp] == 0) {
		if (pnum == myplr && sendflag)
			NetSendCmdParam1(true, CMD_CLOSEDOOR, oi);
		object[oi]._oVar4 = 0;
		object[oi]._oSelFlag = 3;
		ObjSetMicro(xp, yp, object[oi]._oVar1);
		if (currlevel < 17) {
			if (object[oi]._oVar2 != 50) {
				ObjSetMicro(xp - 1, yp, object[oi]._oVar2);
			} else {
				if (dPiece[xp - 1][yp] == 396)
					ObjSetMicro(xp - 1, yp, 411);
				else
					ObjSetMicro(xp - 1, yp, 50);
			}
		} else {
			if (object[oi]._oVar2 != 86) {
				ObjSetMicro(xp - 1, yp, object[oi]._oVar2);
			} else {
				if (dPiece[xp - 1][yp] == 210)
					ObjSetMicro(xp - 1, yp, 232);
				else
					ObjSetMicro(xp - 1, yp, 86);
			}
		}
		dSpecial[xp][yp] = 0;
		object[oi]._oAnimFrame -= 2;
		object[oi]._oPreFlag = false;
		RedoPlayerVision();
	} else {
		object[oi]._oVar4 = 2;
	}
}

void OperateL1LDoor(int pnum, int oi, bool sendflag)
{
	int xp, yp;

	if (object[oi]._oVar4 == 2) {
		if (!deltaload)
			PlaySfxLoc(IS_DOORCLOS, object[oi].position.x, object[oi].position.y);
		return;
	}

	xp = object[oi].position.x;
	yp = object[oi].position.y;
	if (object[oi]._oVar4 == 0) {
		if (pnum == myplr && sendflag)
			NetSendCmdParam1(true, CMD_OPENDOOR, oi);
		if (currlevel < 21) {
			if (!deltaload)
				PlaySfxLoc(IS_DOOROPEN, object[oi].position.x, object[oi].position.y);
			if (object[oi]._oVar1 == 214)
				ObjSetMicro(xp, yp, 408);
			else
				ObjSetMicro(xp, yp, 393);
		} else {
			if (!deltaload)
				PlaySfxLoc(IS_CROPEN, object[oi].position.x, object[oi].position.y);
			ObjSetMicro(xp, yp, 206);
		}
		if (currlevel < 17) {
			dSpecial[xp][yp] = 7;
		} else {
			dSpecial[xp][yp] = 1;
		}
		objects_set_door_piece(xp - 1, yp);
		object[oi]._oAnimFrame += 2;
		object[oi]._oPreFlag = true;
		DoorSet(oi, xp, yp - 1);
		object[oi]._oVar4 = 1;
		object[oi]._oSelFlag = 2;
		RedoPlayerVision();
		return;
	}

	if (currlevel < 21) {
		if (!deltaload)
			PlaySfxLoc(IS_DOORCLOS, xp, object[oi].position.y);
	} else {
		if (!deltaload)
			PlaySfxLoc(IS_CRCLOS, xp, object[oi].position.y);
	}
	if (dDead[xp][yp] == 0 && dMonster[xp][yp] == 0 && dItem[xp][yp] == 0) {
		if (pnum == myplr && sendflag)
			NetSendCmdParam1(true, CMD_CLOSEDOOR, oi);
		object[oi]._oVar4 = 0;
		object[oi]._oSelFlag = 3;
		ObjSetMicro(xp, yp, object[oi]._oVar1);
		if (currlevel < 17) {
			if (object[oi]._oVar2 != 50) {
				ObjSetMicro(xp, yp - 1, object[oi]._oVar2);
			} else {
				if (dPiece[xp][yp - 1] == 396)
					ObjSetMicro(xp, yp - 1, 412);
				else
					ObjSetMicro(xp, yp - 1, 50);
			}
		} else {
			if (object[oi]._oVar2 != 86) {
				ObjSetMicro(xp, yp - 1, object[oi]._oVar2);
			} else {
				if (dPiece[xp][yp - 1] == 210)
					ObjSetMicro(xp, yp - 1, 234);
				else
					ObjSetMicro(xp, yp - 1, 86);
			}
		}
		dSpecial[xp][yp] = 0;
		object[oi]._oAnimFrame -= 2;
		object[oi]._oPreFlag = false;
		RedoPlayerVision();
	} else {
		object[oi]._oVar4 = 2;
	}
}

void OperateL2RDoor(int pnum, int oi, bool sendflag)
{
	int xp, yp;
	bool dok;

	if (object[oi]._oVar4 == 2) {
		if (!deltaload)
			PlaySfxLoc(IS_DOORCLOS, object[oi].position.x, object[oi].position.y);
		return;
	}
	xp = object[oi].position.x;
	yp = object[oi].position.y;
	if (object[oi]._oVar4 == 0) {
		if (pnum == myplr && sendflag)
			NetSendCmdParam1(true, CMD_OPENDOOR, oi);
		if (!deltaload)
			PlaySfxLoc(IS_DOOROPEN, object[oi].position.x, object[oi].position.y);
		ObjSetMicro(xp, yp, 17);
		dSpecial[xp][yp] = 6;
		object[oi]._oAnimFrame += 2;
		object[oi]._oPreFlag = true;
		object[oi]._oVar4 = 1;
		object[oi]._oSelFlag = 2;
		RedoPlayerVision();
		return;
	}

	if (!deltaload)
		PlaySfxLoc(IS_DOORCLOS, object[oi].position.x, yp);
	dok = dMonster[xp][yp] == 0;
	dok = dok && dItem[xp][yp] == 0;
	dok = dok && dDead[xp][yp] == 0;
	if (dok) {
		if (pnum == myplr && sendflag)
			NetSendCmdParam1(true, CMD_CLOSEDOOR, oi);
		object[oi]._oVar4 = 0;
		object[oi]._oSelFlag = 3;
		ObjSetMicro(xp, yp, 540);
		dSpecial[xp][yp] = 0;
		object[oi]._oAnimFrame -= 2;
		object[oi]._oPreFlag = false;
		RedoPlayerVision();
	} else {
		object[oi]._oVar4 = 2;
	}
}

void OperateL2LDoor(int pnum, int oi, bool sendflag)
{
	int xp, yp;
	bool dok;

	if (object[oi]._oVar4 == 2) {
		if (!deltaload)
			PlaySfxLoc(IS_DOORCLOS, object[oi].position.x, object[oi].position.y);
		return;
	}
	xp = object[oi].position.x;
	yp = object[oi].position.y;
	if (object[oi]._oVar4 == 0) {
		if (pnum == myplr && sendflag)
			NetSendCmdParam1(true, CMD_OPENDOOR, oi);
		if (!deltaload)
			PlaySfxLoc(IS_DOOROPEN, object[oi].position.x, object[oi].position.y);
		ObjSetMicro(xp, yp, 13);
		dSpecial[xp][yp] = 5;
		object[oi]._oAnimFrame += 2;
		object[oi]._oPreFlag = true;
		object[oi]._oVar4 = 1;
		object[oi]._oSelFlag = 2;
		RedoPlayerVision();
		return;
	}

	if (!deltaload)
		PlaySfxLoc(IS_DOORCLOS, object[oi].position.x, yp);
	dok = dMonster[xp][yp] == 0;
	dok = dok && dItem[xp][yp] == 0;
	dok = dok && dDead[xp][yp] == 0;
	if (dok) {
		if (pnum == myplr && sendflag)
			NetSendCmdParam1(true, CMD_CLOSEDOOR, oi);
		object[oi]._oVar4 = 0;
		object[oi]._oSelFlag = 3;
		ObjSetMicro(xp, yp, 538);
		dSpecial[xp][yp] = 0;
		object[oi]._oAnimFrame -= 2;
		object[oi]._oPreFlag = false;
		RedoPlayerVision();
	} else {
		object[oi]._oVar4 = 2;
	}
}

void OperateL3RDoor(int pnum, int oi, bool sendflag)
{
	int xp, yp;
	bool dok;

	if (object[oi]._oVar4 == 2) {
		if (!deltaload)
			PlaySfxLoc(IS_DOORCLOS, object[oi].position.x, object[oi].position.y);
		return;
	}

	xp = object[oi].position.x;
	yp = object[oi].position.y;
	if (object[oi]._oVar4 == 0) {
		if (pnum == myplr && sendflag)
			NetSendCmdParam1(true, CMD_OPENDOOR, oi);
		if (!deltaload)
			PlaySfxLoc(IS_DOOROPEN, object[oi].position.x, object[oi].position.y);
		ObjSetMicro(xp, yp, 541);
		object[oi]._oAnimFrame += 2;
		object[oi]._oPreFlag = true;
		object[oi]._oVar4 = 1;
		object[oi]._oSelFlag = 2;
		RedoPlayerVision();
		return;
	}

	if (!deltaload)
		PlaySfxLoc(IS_DOORCLOS, object[oi].position.x, yp);
	dok = dMonster[xp][yp] == 0;
	dok = dok && dItem[xp][yp] == 0;
	dok = dok && dDead[xp][yp] == 0;
	if (dok) {
		if (pnum == myplr && sendflag)
			NetSendCmdParam1(true, CMD_CLOSEDOOR, oi);
		object[oi]._oVar4 = 0;
		object[oi]._oSelFlag = 3;
		ObjSetMicro(xp, yp, 534);
		object[oi]._oAnimFrame -= 2;
		object[oi]._oPreFlag = false;
		RedoPlayerVision();
	} else {
		object[oi]._oVar4 = 2;
	}
}

void OperateL3LDoor(int pnum, int oi, bool sendflag)
{
	int xp, yp;
	bool dok;

	if (object[oi]._oVar4 == 2) {
		if (!deltaload)
			PlaySfxLoc(IS_DOORCLOS, object[oi].position.x, object[oi].position.y);
		return;
	}

	xp = object[oi].position.x;
	yp = object[oi].position.y;
	if (object[oi]._oVar4 == 0) {
		if (pnum == myplr && sendflag)
			NetSendCmdParam1(true, CMD_OPENDOOR, oi);
		if (!deltaload)
			PlaySfxLoc(IS_DOOROPEN, object[oi].position.x, object[oi].position.y);
		ObjSetMicro(xp, yp, 538);
		object[oi]._oAnimFrame += 2;
		object[oi]._oPreFlag = true;
		object[oi]._oVar4 = 1;
		object[oi]._oSelFlag = 2;
		RedoPlayerVision();
		return;
	}

	if (!deltaload)
		PlaySfxLoc(IS_DOORCLOS, object[oi].position.x, yp);
	dok = dMonster[xp][yp] == 0;
	dok = dok && dItem[xp][yp] == 0;
	dok = dok && dDead[xp][yp] == 0;
	if (dok) {
		if (pnum == myplr && sendflag)
			NetSendCmdParam1(true, CMD_CLOSEDOOR, oi);
		object[oi]._oVar4 = 0;
		object[oi]._oSelFlag = 3;
		ObjSetMicro(xp, yp, 531);
		object[oi]._oAnimFrame -= 2;
		object[oi]._oPreFlag = false;
		RedoPlayerVision();
	} else {
		object[oi]._oVar4 = 2;
	}
}

void MonstCheckDoors(int m)
{
	int i, oi;
	int dpx, dpy, mx, my;

	mx = monster[m].position.tile.x;
	my = monster[m].position.tile.y;
	if (dObject[mx - 1][my - 1] != 0
	    || dObject[mx][my - 1] != 0
	    || dObject[mx + 1][my - 1] != 0
	    || dObject[mx - 1][my] != 0
	    || dObject[mx + 1][my] != 0
	    || dObject[mx - 1][my + 1] != 0
	    || dObject[mx][my + 1] != 0
	    || dObject[mx + 1][my + 1] != 0) {
		for (i = 0; i < nobjects; ++i) {
			oi = objectactive[i];
			if ((object[oi]._otype == OBJ_L1LDOOR || object[oi]._otype == OBJ_L1RDOOR) && object[oi]._oVar4 == 0) {
				dpx = abs(object[oi].position.x - mx);
				dpy = abs(object[oi].position.y - my);
				if (dpx == 1 && dpy <= 1 && object[oi]._otype == OBJ_L1LDOOR)
					OperateL1LDoor(myplr, oi, true);
				if (dpx <= 1 && dpy == 1 && object[oi]._otype == OBJ_L1RDOOR)
					OperateL1RDoor(myplr, oi, true);
			}
			if ((object[oi]._otype == OBJ_L2LDOOR || object[oi]._otype == OBJ_L2RDOOR) && object[oi]._oVar4 == 0) {
				dpx = abs(object[oi].position.x - mx);
				dpy = abs(object[oi].position.y - my);
				if (dpx == 1 && dpy <= 1 && object[oi]._otype == OBJ_L2LDOOR)
					OperateL2LDoor(myplr, oi, true);
				if (dpx <= 1 && dpy == 1 && object[oi]._otype == OBJ_L2RDOOR)
					OperateL2RDoor(myplr, oi, true);
			}
			if ((object[oi]._otype == OBJ_L3LDOOR || object[oi]._otype == OBJ_L3RDOOR) && object[oi]._oVar4 == 0) {
				dpx = abs(object[oi].position.x - mx);
				dpy = abs(object[oi].position.y - my);
				if (dpx == 1 && dpy <= 1 && object[oi]._otype == OBJ_L3RDOOR)
					OperateL3RDoor(myplr, oi, true);
				if (dpx <= 1 && dpy == 1 && object[oi]._otype == OBJ_L3LDOOR)
					OperateL3LDoor(myplr, oi, true);
			}
		}
	}
}

void ObjChangeMap(int x1, int y1, int x2, int y2)
{
	int i, j;

	for (j = y1; j <= y2; j++) {
		for (i = x1; i <= x2; i++) {
			ObjSetMini(i, j, pdungeon[i][j]);
			dungeon[i][j] = pdungeon[i][j];
		}
	}
	if (leveltype == DTYPE_CATHEDRAL && currlevel < 17) {
		ObjL1Special(2 * x1 + 16, 2 * y1 + 16, 2 * x2 + 17, 2 * y2 + 17);
		AddL1Objs(2 * x1 + 16, 2 * y1 + 16, 2 * x2 + 17, 2 * y2 + 17);
	}
	if (leveltype == DTYPE_CATACOMBS) {
		ObjL2Special(2 * x1 + 16, 2 * y1 + 16, 2 * x2 + 17, 2 * y2 + 17);
		AddL2Objs(2 * x1 + 16, 2 * y1 + 16, 2 * x2 + 17, 2 * y2 + 17);
	}
}

void ObjChangeMapResync(int x1, int y1, int x2, int y2)
{
	int i, j;

	for (j = y1; j <= y2; j++) {
		for (i = x1; i <= x2; i++) {
			ObjSetMini(i, j, pdungeon[i][j]);
			dungeon[i][j] = pdungeon[i][j];
		}
	}
	if (leveltype == DTYPE_CATHEDRAL && currlevel < 17) {
		ObjL1Special(2 * x1 + 16, 2 * y1 + 16, 2 * x2 + 17, 2 * y2 + 17);
	}
	if (leveltype == DTYPE_CATACOMBS) {
		ObjL2Special(2 * x1 + 16, 2 * y1 + 16, 2 * x2 + 17, 2 * y2 + 17);
	}
}

void OperateL1Door(int pnum, int i, bool sendflag)
{
	int dpx, dpy;

	dpx = abs(object[i].position.x - plr[pnum].position.tile.x);
	dpy = abs(object[i].position.y - plr[pnum].position.tile.y);
	if (dpx == 1 && dpy <= 1 && object[i]._otype == OBJ_L1LDOOR)
		OperateL1LDoor(pnum, i, sendflag);
	if (dpx <= 1 && dpy == 1 && object[i]._otype == OBJ_L1RDOOR)
		OperateL1RDoor(pnum, i, sendflag);
}

void OperateLever(int pnum, int i)
{
	int j, oi;
	bool mapflag;

	if (object[i]._oSelFlag != 0) {
		if (!deltaload)
			PlaySfxLoc(IS_LEVER, object[i].position.x, object[i].position.y);
		object[i]._oSelFlag = 0;
		object[i]._oAnimFrame++;
		mapflag = true;
		if (currlevel == 16) {
			for (j = 0; j < nobjects; j++) {
				oi = objectactive[j];
				if (object[oi]._otype == OBJ_SWITCHSKL
				    && object[i]._oVar8 == object[oi]._oVar8
				    && object[oi]._oSelFlag != 0) {
					mapflag = false;
				}
			}
		}
		if (currlevel == 24) {
			operate_lv24_lever();
			IsUberLeverActivated = true;
			mapflag = false;
			quests[Q_NAKRUL]._qactive = QUEST_DONE;
		}
		if (mapflag)
			ObjChangeMap(object[i]._oVar1, object[i]._oVar2, object[i]._oVar3, object[i]._oVar4);
		if (pnum == myplr)
			NetSendCmdParam1(false, CMD_OPERATEOBJ, i);
	}
}

void OperateBook(int pnum, int i)
{
	int j, oi;
	int dx, dy;
	int otype;
	bool do_add_missile, missile_added;

	if (object[i]._oSelFlag == 0)
		return;
	if (setlevel && setlvlnum == SL_VILEBETRAYER) {
		do_add_missile = false;
		missile_added = false;
		for (j = 0; j < nobjects; j++) {
			oi = objectactive[j];
			otype = object[oi]._otype;
			if (otype == OBJ_MCIRCLE2 && object[oi]._oVar6 == 1) {
				dx = 27;
				dy = 29;
				object[oi]._oVar6 = 4;
				do_add_missile = true;
			}
			if (otype == OBJ_MCIRCLE2 && object[oi]._oVar6 == 2) {
				dx = 43;
				dy = 29;
				object[oi]._oVar6 = 4;
				do_add_missile = true;
			}
			if (do_add_missile) {
				object[dObject[35][36] - 1]._oVar5++;
				AddMissile(plr[pnum].position.tile.x, plr[pnum].position.tile.y, dx, dy, plr[pnum]._pdir, MIS_RNDTELEPORT, TARGET_MONSTERS, pnum, 0, 0);
				missile_added = true;
				do_add_missile = false;
			}
		}
		if (!missile_added)
			return;
	}
	object[i]._oSelFlag = 0;
	object[i]._oAnimFrame++;
	if (!setlevel)
		return;

	if (setlvlnum == SL_BONECHAMB) {
		plr[pnum]._pMemSpells |= GetSpellBitmask(SPL_GUARDIAN);
		if (plr[pnum]._pSplLvl[SPL_GUARDIAN] < MAX_SPELL_LEVEL)
			plr[pnum]._pSplLvl[SPL_GUARDIAN]++;
		quests[Q_SCHAMB]._qactive = QUEST_DONE;
		if (!deltaload)
			PlaySfxLoc(IS_QUESTDN, object[i].position.x, object[i].position.y);
		InitDiabloMsg(EMSG_BONECHAMB);
		AddMissile(
		    plr[pnum].position.tile.x,
		    plr[pnum].position.tile.y,
		    object[i].position.x - 2,
		    object[i].position.y - 4,
		    plr[pnum]._pdir,
		    MIS_GUARDIAN,
		    TARGET_MONSTERS,
		    pnum,
		    0,
		    0);
	}
	if (setlvlnum == SL_VILEBETRAYER) {
		ObjChangeMapResync(
		    object[i]._oVar1,
		    object[i]._oVar2,
		    object[i]._oVar3,
		    object[i]._oVar4);
		for (j = 0; j < nobjects; j++)
			SyncObjectAnim(objectactive[j]);
	}
}

void OperateBookLever(int pnum, int i)
{
	int x, y, tren;

	x = 2 * setpc_x + 16;
	y = 2 * setpc_y + 16;
	if (numitems >= MAXITEMS) {
		return;
	}
	if (object[i]._oSelFlag != 0 && !qtextflag) {
		if (object[i]._otype == OBJ_BLINDBOOK && quests[Q_BLIND]._qvar1 == 0) {
			quests[Q_BLIND]._qactive = QUEST_ACTIVE;
			quests[Q_BLIND]._qlog = true;
			quests[Q_BLIND]._qvar1 = 1;
		}
		if (object[i]._otype == OBJ_BLOODBOOK && quests[Q_BLOOD]._qvar1 == 0) {
			quests[Q_BLOOD]._qactive = QUEST_ACTIVE;
			quests[Q_BLOOD]._qlog = true;
			quests[Q_BLOOD]._qvar1 = 1;
			SpawnQuestItem(IDI_BLDSTONE, 2 * setpc_x + 25, 2 * setpc_y + 33, 0, true);
		}
		object[i]._otype = object[i]._otype;
		if (object[i]._otype == OBJ_STEELTOME && quests[Q_WARLORD]._qvar1 == 0) {
			quests[Q_WARLORD]._qactive = QUEST_ACTIVE;
			quests[Q_WARLORD]._qlog = true;
			quests[Q_WARLORD]._qvar1 = 1;
		}
		if (object[i]._oAnimFrame != object[i]._oVar6) {
			if (object[i]._otype != OBJ_BLOODBOOK)
				ObjChangeMap(object[i]._oVar1, object[i]._oVar2, object[i]._oVar3, object[i]._oVar4);
			if (object[i]._otype == OBJ_BLINDBOOK) {
				SpawnUnique(UITEM_OPTAMULET, x + 5, y + 5);
				tren = TransVal;
				TransVal = 9;
				DRLG_MRectTrans(object[i]._oVar1, object[i]._oVar2, object[i]._oVar3, object[i]._oVar4);
				TransVal = tren;
			}
		}
		object[i]._oAnimFrame = object[i]._oVar6;
		InitQTextMsg(object[i]._oVar7);
		if (pnum == myplr)
			NetSendCmdParam1(false, CMD_OPERATEOBJ, i);
	}
}

void OperateSChambBk(int i)
{
	int j;

	if (object[i]._oSelFlag != 0 && !qtextflag) {
		if (object[i]._oAnimFrame != object[i]._oVar6) {
			ObjChangeMapResync(object[i]._oVar1, object[i]._oVar2, object[i]._oVar3, object[i]._oVar4);
			for (j = 0; j < nobjects; j++)
				SyncObjectAnim(objectactive[j]);
		}
		object[i]._oAnimFrame = object[i]._oVar6;
		if (quests[Q_SCHAMB]._qactive == QUEST_INIT) {
			quests[Q_SCHAMB]._qactive = QUEST_ACTIVE;
			quests[Q_SCHAMB]._qlog = true;
		}

		_speech_id textdef;
		switch (plr[myplr]._pClass) {
		case HeroClass::Warrior:
			textdef = TEXT_BONER;
			break;
		case HeroClass::Rogue:
			textdef = TEXT_RBONER;
			break;
		case HeroClass::Sorcerer:
			textdef = TEXT_MBONER;
			break;
		case HeroClass::Monk:
			textdef = TEXT_HBONER;
			break;
		case HeroClass::Bard:
			textdef = TEXT_BBONER;
			break;
		case HeroClass::Barbarian:
			textdef = TEXT_BONER;
			break;
		}
		quests[Q_SCHAMB]._qmsg = textdef;
		InitQTextMsg(textdef);
	}
}

void OperateChest(int pnum, int i, bool sendmsg)
{
	int j, mtype;

	if (object[i]._oSelFlag != 0) {
		if (!deltaload)
			PlaySfxLoc(IS_CHEST, object[i].position.x, object[i].position.y);
		object[i]._oSelFlag = 0;
		object[i]._oAnimFrame += 2;
		if (!deltaload) {
			SetRndSeed(object[i]._oRndSeed);
			if (setlevel) {
				for (j = 0; j < object[i]._oVar1; j++) {
					CreateRndItem(object[i].position.x, object[i].position.y, true, sendmsg, false);
				}
			} else {
				for (j = 0; j < object[i]._oVar1; j++) {
					if (object[i]._oVar2 != 0)
						CreateRndItem(object[i].position.x, object[i].position.y, false, sendmsg, false);
					else
						CreateRndUseful(object[i].position.x, object[i].position.y, sendmsg);
				}
			}
			if (object[i]._oTrapFlag && object[i]._otype >= OBJ_TCHEST1 && object[i]._otype <= OBJ_TCHEST3) {
				direction mdir = GetDirection(object[i].position, plr[pnum].position.tile);
				switch (object[i]._oVar4) {
				case 0:
					mtype = MIS_ARROW;
					break;
				case 1:
					mtype = MIS_FARROW;
					break;
				case 2:
					mtype = MIS_NOVA;
					break;
				case 3:
					mtype = MIS_FIRERING;
					break;
				case 4:
					mtype = MIS_STEALPOTS;
					break;
				case 5:
					mtype = MIS_MANATRAP;
					break;
				default:
					mtype = MIS_ARROW;
				}
				AddMissile(object[i].position.x, object[i].position.y, plr[pnum].position.tile.x, plr[pnum].position.tile.y, mdir, mtype, TARGET_PLAYERS, -1, 0, 0);
				object[i]._oTrapFlag = false;
			}
			if (pnum == myplr)
				NetSendCmdParam2(false, CMD_PLROPOBJ, pnum, i);
			return;
		}
	}
}

void OperateMushPatch(int pnum, int i)
{
	int x, y;

	if (numitems >= MAXITEMS) {
		return;
	}

	if (quests[Q_MUSHROOM]._qactive != QUEST_ACTIVE || quests[Q_MUSHROOM]._qvar1 < QS_TOMEGIVEN) {
		if (!deltaload && pnum == myplr) {
			plr[myplr].PlaySpeach(13);
		}
	} else {
		if (object[i]._oSelFlag != 0) {
			if (!deltaload)
				PlaySfxLoc(IS_CHEST, object[i].position.x, object[i].position.y);
			object[i]._oSelFlag = 0;
			object[i]._oAnimFrame++;
			if (!deltaload) {
				GetSuperItemLoc(object[i].position.x, object[i].position.y, &x, &y);
				SpawnQuestItem(IDI_MUSHROOM, x, y, 0, false);
				quests[Q_MUSHROOM]._qvar1 = QS_MUSHSPAWNED;
			}
		}
	}
}

void OperateInnSignChest(int pnum, int i)
{
	int x, y;

	if (numitems >= MAXITEMS) {
		return;
	}

	if (quests[Q_LTBANNER]._qvar1 != 2) {
		if (!deltaload && pnum == myplr) {
			plr[myplr].PlaySpeach(24);
		}
	} else {
		if (object[i]._oSelFlag != 0) {
			if (!deltaload)
				PlaySfxLoc(IS_CHEST, object[i].position.x, object[i].position.y);
			object[i]._oSelFlag = 0;
			object[i]._oAnimFrame += 2;
			if (!deltaload) {
				GetSuperItemLoc(object[i].position.x, object[i].position.y, &x, &y);
				SpawnQuestItem(IDI_BANNER, x, y, 0, false);
			}
		}
	}
}

void OperateSlainHero(int pnum, int i)
{
	if (object[i]._oSelFlag != 0) {
		object[i]._oSelFlag = 0;
		if (!deltaload) {
			if (plr[pnum]._pClass == HeroClass::Warrior) {
				CreateMagicArmor(object[i].position.x, object[i].position.y, ITYPE_HARMOR, ICURS_BREAST_PLATE, false, true);
			} else if (plr[pnum]._pClass == HeroClass::Rogue) {
				CreateMagicWeapon(object[i].position.x, object[i].position.y, ITYPE_BOW, ICURS_LONG_WAR_BOW, false, true);
			} else if (plr[pnum]._pClass == HeroClass::Sorcerer) {
				CreateSpellBook(object[i].position.x, object[i].position.y, SPL_LIGHTNING, false, true);
			} else if (plr[pnum]._pClass == HeroClass::Monk) {
				CreateMagicWeapon(object[i].position.x, object[i].position.y, ITYPE_STAFF, ICURS_WAR_STAFF, false, true);
			} else if (plr[pnum]._pClass == HeroClass::Bard) {
				CreateMagicWeapon(object[i].position.x, object[i].position.y, ITYPE_SWORD, ICURS_BASTARD_SWORD, false, true);
			} else if (plr[pnum]._pClass == HeroClass::Barbarian) {
				CreateMagicWeapon(object[i].position.x, object[i].position.y, ITYPE_AXE, ICURS_BATTLE_AXE, false, true);
			}
			plr[myplr].PlaySpeach(9);
			if (pnum == myplr)
				NetSendCmdParam1(false, CMD_OPERATEOBJ, i);
		}
	}
}

void OperateTrapLvr(int i)
{
	int frame, j, oi;

	frame = object[i]._oAnimFrame;
	j = 0;

	if (!deltaload)
		PlaySfxLoc(IS_LEVER, object[i].position.x, object[i].position.y);

	if (frame == 1) {
		object[i]._oAnimFrame = 2;
		for (; j < nobjects; j++) {
			oi = objectactive[j];
			if (object[oi]._otype == object[i]._oVar2 && object[oi]._oVar1 == object[i]._oVar1) {
				object[oi]._oVar2 = 1;
				object[oi]._oAnimFlag = 0;
			}
		}
		return;
	}

	object[i]._oAnimFrame = frame - 1;
	for (; j < nobjects; j++) {
		oi = objectactive[j];
		if (object[oi]._otype == object[i]._oVar2 && object[oi]._oVar1 == object[i]._oVar1) {
			object[oi]._oVar2 = 0;
			if (object[oi]._oVar4 != 0)
				object[oi]._oAnimFlag = 1;
		}
	}
}

void OperateSarc(int pnum, int i, bool sendmsg)
{
	if (object[i]._oSelFlag != 0) {
		if (!deltaload)
			PlaySfxLoc(IS_SARC, object[i].position.x, object[i].position.y);
		object[i]._oSelFlag = 0;
		if (deltaload) {
			object[i]._oAnimFrame = object[i]._oAnimLen;
		} else {
			object[i]._oAnimFlag = 1;
			object[i]._oAnimDelay = 3;
			SetRndSeed(object[i]._oRndSeed);
			if (object[i]._oVar1 <= 2)
				CreateRndItem(object[i].position.x, object[i].position.y, false, sendmsg, false);
			if (object[i]._oVar1 >= 8)
				SpawnSkeleton(object[i]._oVar2, object[i].position.x, object[i].position.y);
			if (pnum == myplr)
				NetSendCmdParam1(false, CMD_OPERATEOBJ, i);
		}
	}
}

void OperateL2Door(int pnum, int i, bool sendflag)
{
	int dpx, dpy;

	dpx = abs(object[i].position.x - plr[pnum].position.tile.x);
	dpy = abs(object[i].position.y - plr[pnum].position.tile.y);
	if (dpx == 1 && dpy <= 1 && object[i]._otype == OBJ_L2LDOOR)
		OperateL2LDoor(pnum, i, sendflag);
	if (dpx <= 1 && dpy == 1 && object[i]._otype == OBJ_L2RDOOR)
		OperateL2RDoor(pnum, i, sendflag);
}

void OperateL3Door(int pnum, int i, bool sendflag)
{
	int dpx, dpy;

	dpx = abs(object[i].position.x - plr[pnum].position.tile.x);
	dpy = abs(object[i].position.y - plr[pnum].position.tile.y);
	if (dpx == 1 && dpy <= 1 && object[i]._otype == OBJ_L3RDOOR)
		OperateL3RDoor(pnum, i, sendflag);
	if (dpx <= 1 && dpy == 1 && object[i]._otype == OBJ_L3LDOOR)
		OperateL3LDoor(pnum, i, sendflag);
}

void OperatePedistal(int pnum, int i)
{
	int iv;

	if (numitems >= MAXITEMS) {
		return;
	}

	if (object[i]._oVar6 != 3 && PlrHasItem(pnum, IDI_BLDSTONE, &iv) != nullptr) {
		RemoveInvItem(pnum, iv);
		object[i]._oAnimFrame++;
		object[i]._oVar6++;
		if (object[i]._oVar6 == 1) {
			if (!deltaload)
				PlaySfxLoc(LS_PUDDLE, object[i].position.x, object[i].position.y);
			ObjChangeMap(setpc_x, setpc_y + 3, setpc_x + 2, setpc_y + 7);
			SpawnQuestItem(IDI_BLDSTONE, 2 * setpc_x + 19, 2 * setpc_y + 26, 0, true);
		}
		if (object[i]._oVar6 == 2) {
			if (!deltaload)
				PlaySfxLoc(LS_PUDDLE, object[i].position.x, object[i].position.y);
			ObjChangeMap(setpc_x + 6, setpc_y + 3, setpc_x + setpc_w, setpc_y + 7);
			SpawnQuestItem(IDI_BLDSTONE, 2 * setpc_x + 31, 2 * setpc_y + 26, 0, true);
		}
		if (object[i]._oVar6 == 3) {
			if (!deltaload)
				PlaySfxLoc(LS_BLODSTAR, object[i].position.x, object[i].position.y);
			ObjChangeMap(object[i]._oVar1, object[i]._oVar2, object[i]._oVar3, object[i]._oVar4);
			LoadMapObjs("Levels\\L2Data\\Blood2.DUN", 2 * setpc_x, 2 * setpc_y);
			SpawnUnique(UITEM_ARMOFVAL, 2 * setpc_x + 25, 2 * setpc_y + 19);
			object[i]._oSelFlag = 0;
		}
	}
}

void TryDisarm(int pnum, int i)
{
	int j, oi, oti, trapdisper;
	bool checkflag;

	if (pnum == myplr)
		NewCursor(CURSOR_HAND);
	if (object[i]._oTrapFlag) {
		trapdisper = 2 * plr[pnum]._pDexterity - 5 * currlevel;
		if (GenerateRnd(100) <= trapdisper) {
			for (j = 0; j < nobjects; j++) {
				checkflag = false;
				oi = objectactive[j];
				oti = object[oi]._otype;
				if (oti == OBJ_TRAPL)
					checkflag = true;
				if (oti == OBJ_TRAPR)
					checkflag = true;
				if (checkflag && dObject[object[oi]._oVar1][object[oi]._oVar2] - 1 == i) {
					object[oi]._oVar4 = 1;
					object[i]._oTrapFlag = false;
				}
			}
			oti = object[i]._otype;
			if (oti >= OBJ_TCHEST1 && oti <= OBJ_TCHEST3)
				object[i]._oTrapFlag = false;
		}
	}
}

int ItemMiscIdIdx(item_misc_id imiscid)
{
	int i;

	i = IDI_GOLD;
	while (AllItemsList[i].iRnd == IDROP_NEVER || AllItemsList[i].iMiscId != imiscid) {
		i++;
	}

	return i;
}

bool OperateShrineMysterious(int pnum)
{
	if (deltaload)
		return false;
	if (pnum != myplr)
		return false;

	ModifyPlrStr(pnum, -1);
	ModifyPlrMag(pnum, -1);
	ModifyPlrDex(pnum, -1);
	ModifyPlrVit(pnum, -1);

	switch (static_cast<CharacterAttribute>(GenerateRnd(4))) {
	case CharacterAttribute::Strength:
		ModifyPlrStr(pnum, 6);
		break;
	case CharacterAttribute::Magic:
		ModifyPlrMag(pnum, 6);
		break;
	case CharacterAttribute::Dexterity:
		ModifyPlrDex(pnum, 6);
		break;
	case CharacterAttribute::Vitality:
		ModifyPlrVit(pnum, 6);
		break;
	}

	CheckStats(pnum);

	InitDiabloMsg(EMSG_SHRINE_MYSTERIOUS);

	return true;
}

bool OperateShrineHidden(int pnum)
{
	if (deltaload)
		return false;
	if (pnum != myplr)
		return false;

	int cnt = 0;
	for (const auto &item : plr[pnum].InvBody) {
		if (!item.isEmpty())
			cnt++;
	}
	if (cnt > 0) {
		for (auto &item : plr[pnum].InvBody) {
			if (!item.isEmpty()
			    && item._iMaxDur != DUR_INDESTRUCTIBLE
			    && item._iMaxDur != 0) {
				item._iDurability += 10;
				item._iMaxDur += 10;
				if (item._iDurability > item._iMaxDur)
					item._iDurability = item._iMaxDur;
			}
		}
		while (true) {
			cnt = 0;
			for (auto &item : plr[pnum].InvBody) {
				if (!item.isEmpty() && item._iMaxDur != DUR_INDESTRUCTIBLE && item._iMaxDur != 0) {
					cnt++;
				}
			}
			if (cnt == 0)
				break;
			int r = GenerateRnd(NUM_INVLOC);
			if (plr[pnum].InvBody[r].isEmpty() || plr[pnum].InvBody[r]._iMaxDur == DUR_INDESTRUCTIBLE || plr[pnum].InvBody[r]._iMaxDur == 0)
				continue;

			plr[pnum].InvBody[r]._iDurability -= 20;
			plr[pnum].InvBody[r]._iMaxDur -= 20;
			if (plr[pnum].InvBody[r]._iDurability <= 0)
				plr[pnum].InvBody[r]._iDurability = 1;
			if (plr[pnum].InvBody[r]._iMaxDur <= 0)
				plr[pnum].InvBody[r]._iMaxDur = 1;
			break;
		}
	}

	InitDiabloMsg(EMSG_SHRINE_HIDDEN);

	return true;
}

bool OperateShrineGloomy(int pnum)
{
	if (deltaload)
		return false;
	if (pnum != myplr)
		return true;

	if (!plr[pnum].InvBody[INVLOC_HEAD].isEmpty())
		plr[pnum].InvBody[INVLOC_HEAD]._iAC += 2;
	if (!plr[pnum].InvBody[INVLOC_CHEST].isEmpty())
		plr[pnum].InvBody[INVLOC_CHEST]._iAC += 2;
	if (!plr[pnum].InvBody[INVLOC_HAND_LEFT].isEmpty()) {
		if (plr[pnum].InvBody[INVLOC_HAND_LEFT]._itype == ITYPE_SHIELD) {
			plr[pnum].InvBody[INVLOC_HAND_LEFT]._iAC += 2;
		} else {
			plr[pnum].InvBody[INVLOC_HAND_LEFT]._iMaxDam--;
			if (plr[pnum].InvBody[INVLOC_HAND_LEFT]._iMaxDam < plr[pnum].InvBody[INVLOC_HAND_LEFT]._iMinDam)
				plr[pnum].InvBody[INVLOC_HAND_LEFT]._iMaxDam = plr[pnum].InvBody[INVLOC_HAND_LEFT]._iMinDam;
		}
	}
	if (!plr[pnum].InvBody[INVLOC_HAND_RIGHT].isEmpty()) {
		if (plr[pnum].InvBody[INVLOC_HAND_RIGHT]._itype == ITYPE_SHIELD) {
			plr[pnum].InvBody[INVLOC_HAND_RIGHT]._iAC += 2;
		} else {
			plr[pnum].InvBody[INVLOC_HAND_RIGHT]._iMaxDam--;
			if (plr[pnum].InvBody[INVLOC_HAND_RIGHT]._iMaxDam < plr[pnum].InvBody[INVLOC_HAND_RIGHT]._iMinDam)
				plr[pnum].InvBody[INVLOC_HAND_RIGHT]._iMaxDam = plr[pnum].InvBody[INVLOC_HAND_RIGHT]._iMinDam;
		}
	}

	for (int j = 0; j < plr[pnum]._pNumInv; j++) {
		switch (plr[pnum].InvList[j]._itype) {
		case ITYPE_SWORD:
		case ITYPE_AXE:
		case ITYPE_BOW:
		case ITYPE_MACE:
		case ITYPE_STAFF:
			plr[pnum].InvList[j]._iMaxDam--;
			if (plr[pnum].InvList[j]._iMaxDam < plr[pnum].InvList[j]._iMinDam)
				plr[pnum].InvList[j]._iMaxDam = plr[pnum].InvList[j]._iMinDam;
			break;
		case ITYPE_SHIELD:
		case ITYPE_HELM:
		case ITYPE_LARMOR:
		case ITYPE_MARMOR:
		case ITYPE_HARMOR:
			plr[pnum].InvList[j]._iAC += 2;
			break;
		default:
			break;
		}
	}

	InitDiabloMsg(EMSG_SHRINE_GLOOMY);

	return true;
}

bool OperateShrineWeird(int pnum)
{
	if (deltaload)
		return false;
	if (pnum != myplr)
		return true;

	if (!plr[pnum].InvBody[INVLOC_HAND_LEFT].isEmpty() && plr[pnum].InvBody[INVLOC_HAND_LEFT]._itype != ITYPE_SHIELD)
		plr[pnum].InvBody[INVLOC_HAND_LEFT]._iMaxDam++;
	if (!plr[pnum].InvBody[INVLOC_HAND_RIGHT].isEmpty() && plr[pnum].InvBody[INVLOC_HAND_RIGHT]._itype != ITYPE_SHIELD)
		plr[pnum].InvBody[INVLOC_HAND_RIGHT]._iMaxDam++;

	for (int j = 0; j < plr[pnum]._pNumInv; j++) {
		switch (plr[pnum].InvList[j]._itype) {
		case ITYPE_SWORD:
		case ITYPE_AXE:
		case ITYPE_BOW:
		case ITYPE_MACE:
		case ITYPE_STAFF:
			plr[pnum].InvList[j]._iMaxDam++;
			break;
		default:
			break;
		}
	}

	InitDiabloMsg(EMSG_SHRINE_WEIRD);

	return true;
}

bool OperateShrineMagical(int pnum)
{
	if (deltaload)
		return false;

	AddMissile(
	    plr[pnum].position.tile.x,
	    plr[pnum].position.tile.y,
	    plr[pnum].position.tile.x,
	    plr[pnum].position.tile.y,
	    plr[pnum]._pdir,
	    MIS_MANASHIELD,
	    -1,
	    pnum,
	    0,
	    2 * leveltype);

	if (pnum != myplr)
		return false;

	InitDiabloMsg(EMSG_SHRINE_MAGICAL);

	return true;
}

bool OperateShrineStone(int pnum)
{
	if (deltaload)
		return false;
	if (pnum != myplr)
		return true;

	for (auto &item : plr[pnum].InvBody) {
		if (item._itype == ITYPE_STAFF)
			item._iCharges = item._iMaxCharges;
	}
	for (int j = 0; j < plr[pnum]._pNumInv; j++) {
		if (plr[pnum].InvList[j]._itype == ITYPE_STAFF)
			plr[pnum].InvList[j]._iCharges = plr[pnum].InvList[j]._iMaxCharges;
	}
	for (auto &item : plr[pnum].SpdList) {
		if (item._itype == ITYPE_STAFF)
			item._iCharges = item._iMaxCharges; // belt items don't have charges?
	}

	InitDiabloMsg(EMSG_SHRINE_STONE);

	return true;
}

bool OperateShrineReligious(int pnum)
{
	if (deltaload)
		return false;
	if (pnum != myplr)
		return true;

	for (auto &item : plr[pnum].InvBody)
		item._iDurability = item._iMaxDur;
	for (int j = 0; j < plr[pnum]._pNumInv; j++)
		plr[pnum].InvList[j]._iDurability = plr[pnum].InvList[j]._iMaxDur;
	for (auto &item : plr[pnum].SpdList)
		item._iDurability = item._iMaxDur; // belt items don't have durability?

	InitDiabloMsg(EMSG_SHRINE_RELIGIOUS);

	return true;
}

bool OperateShrineEnchanted(int pnum)
{
	if (deltaload)
		return false;
	if (pnum != myplr)
		return false;

	int cnt = 0;
	uint64_t spell = 1;
	int maxSpells = gbIsHellfire ? MAX_SPELLS : 37;
	uint64_t spells = plr[pnum]._pMemSpells;
	for (int j = 0; j < maxSpells; j++) {
		if ((spell & spells) != 0)
			cnt++;
		spell *= 2;
	}
	if (cnt > 1) {
		spell = 1;
		for (int j = SPL_FIREBOLT; j < maxSpells; j++) { // BUGFIX: < MAX_SPELLS, there is no spell with MAX_SPELLS index (fixed)
			if ((plr[pnum]._pMemSpells & spell) != 0) {
				if (plr[pnum]._pSplLvl[j] < MAX_SPELL_LEVEL)
					plr[pnum]._pSplLvl[j]++;
			}
			spell *= 2;
		}
		int r;
		do {
			r = GenerateRnd(maxSpells);
		} while (!(plr[pnum]._pMemSpells & GetSpellBitmask(r + 1)));
		if (plr[pnum]._pSplLvl[r + 1] >= 2)
			plr[pnum]._pSplLvl[r + 1] -= 2;
		else
			plr[pnum]._pSplLvl[r + 1] = 0;
	}

	InitDiabloMsg(EMSG_SHRINE_ENCHANTED);

	return true;
}

bool OperateShrineThaumaturgic(int pnum)
{
	for (int j = 0; j < nobjects; j++) {
		int v1 = objectactive[j];
		assert((DWORD)v1 < MAXOBJECTS);
		if ((object[v1]._otype == OBJ_CHEST1
		        || object[v1]._otype == OBJ_CHEST2
		        || object[v1]._otype == OBJ_CHEST3)
		    && object[v1]._oSelFlag == 0) {
			object[v1]._oRndSeed = AdvanceRndSeed();
			object[v1]._oSelFlag = 1;
			object[v1]._oAnimFrame -= 2;
		}
	}

	if (deltaload)
		return false;

	if (pnum != myplr)
		return true;

	InitDiabloMsg(EMSG_SHRINE_THAUMATURGIC);

	return true;
}

bool OperateShrineFascinating(int pnum)
{
	if (deltaload)
		return false;
	if (pnum != myplr)
		return false;

	plr[pnum]._pMemSpells |= GetSpellBitmask(SPL_FIREBOLT);

	if (plr[pnum]._pSplLvl[SPL_FIREBOLT] < MAX_SPELL_LEVEL)
		plr[pnum]._pSplLvl[SPL_FIREBOLT]++;
	if (plr[pnum]._pSplLvl[SPL_FIREBOLT] < MAX_SPELL_LEVEL)
		plr[pnum]._pSplLvl[SPL_FIREBOLT]++;

	DWORD t = plr[pnum]._pMaxManaBase / 10;
	int v1 = plr[pnum]._pMana - plr[pnum]._pManaBase;
	int v2 = plr[pnum]._pMaxMana - plr[pnum]._pMaxManaBase;
	plr[pnum]._pManaBase -= t;
	plr[pnum]._pMana -= t;
	plr[pnum]._pMaxMana -= t;
	plr[pnum]._pMaxManaBase -= t;
	if (plr[pnum]._pMana >> 6 <= 0) {
		plr[pnum]._pMana = v1;
		plr[pnum]._pManaBase = 0;
	}
	if (plr[pnum]._pMaxMana >> 6 <= 0) {
		plr[pnum]._pMaxMana = v2;
		plr[pnum]._pMaxManaBase = 0;
	}

	InitDiabloMsg(EMSG_SHRINE_FASCINATING);

	return true;
}

bool OperateShrineCryptic(int pnum)
{
	if (deltaload)
		return false;

	AddMissile(
	    plr[pnum].position.tile.x,
	    plr[pnum].position.tile.y,
	    plr[pnum].position.tile.x,
	    plr[pnum].position.tile.y,
	    plr[pnum]._pdir,
	    MIS_NOVA,
	    -1,
	    pnum,
	    0,
	    2 * leveltype);

	if (pnum != myplr)
		return false;

	plr[pnum]._pMana = plr[pnum]._pMaxMana;
	plr[pnum]._pManaBase = plr[pnum]._pMaxManaBase;

	InitDiabloMsg(EMSG_SHRINE_CRYPTIC);

	return true;
}

bool OperateShrineEldritch(int pnum)
{
	/// BUGFIX: change `plr[pnum].HoldItem` to use a temporary buffer to prevent deleting item in hand
	if (deltaload)
		return false;
	if (pnum != myplr)
		return true;

	for (int j = 0; j < plr[pnum]._pNumInv; j++) {
		if (plr[pnum].InvList[j]._itype == ITYPE_MISC) {
			if (plr[pnum].InvList[j]._iMiscId == IMISC_HEAL
			    || plr[pnum].InvList[j]._iMiscId == IMISC_MANA) {
				SetPlrHandItem(&plr[pnum].HoldItem, ItemMiscIdIdx(IMISC_REJUV));
				GetPlrHandSeed(&plr[pnum].HoldItem);
				plr[pnum].HoldItem._iStatFlag = true;
				plr[pnum].InvList[j] = plr[pnum].HoldItem;
			}
			if (plr[pnum].InvList[j]._iMiscId == IMISC_FULLHEAL
			    || plr[pnum].InvList[j]._iMiscId == IMISC_FULLMANA) {
				SetPlrHandItem(&plr[pnum].HoldItem, ItemMiscIdIdx(IMISC_FULLREJUV));
				GetPlrHandSeed(&plr[pnum].HoldItem);
				plr[pnum].HoldItem._iStatFlag = true;
				plr[pnum].InvList[j] = plr[pnum].HoldItem;
			}
		}
	}
	for (auto &item : plr[pnum].SpdList) {
		if (item._itype == ITYPE_MISC) {
			if (item._iMiscId == IMISC_HEAL
			    || item._iMiscId == IMISC_MANA) {
				SetPlrHandItem(&plr[pnum].HoldItem, ItemMiscIdIdx(IMISC_REJUV));
				GetPlrHandSeed(&plr[pnum].HoldItem);
				plr[pnum].HoldItem._iStatFlag = true;
				item = plr[pnum].HoldItem;
			}
			if (item._iMiscId == IMISC_FULLHEAL
			    || item._iMiscId == IMISC_FULLMANA) {
				SetPlrHandItem(&plr[pnum].HoldItem, ItemMiscIdIdx(IMISC_FULLREJUV));
				GetPlrHandSeed(&plr[pnum].HoldItem);
				plr[pnum].HoldItem._iStatFlag = true;
				item = plr[pnum].HoldItem;
			}
		}
	}

	InitDiabloMsg(EMSG_SHRINE_ELDRITCH);

	return true;
}

bool OperateShrineEerie(int pnum)
{
	if (deltaload)
		return false;
	if (pnum != myplr)
		return false;

	ModifyPlrMag(pnum, 2);
	CheckStats(pnum);

	InitDiabloMsg(EMSG_SHRINE_EERIE);

	return true;
}

bool OperateShrineDivine(int pnum, int x, int y)
{
	if (deltaload)
		return false;
	if (pnum != myplr)
		return false;

	if (currlevel < 4) {
		CreateTypeItem(x, y, false, ITYPE_MISC, IMISC_FULLMANA, false, true);
		CreateTypeItem(x, y, false, ITYPE_MISC, IMISC_FULLHEAL, false, true);
	} else {
		CreateTypeItem(x, y, false, ITYPE_MISC, IMISC_FULLREJUV, false, true);
		CreateTypeItem(x, y, false, ITYPE_MISC, IMISC_FULLREJUV, false, true);
	}

	plr[pnum]._pMana = plr[pnum]._pMaxMana;
	plr[pnum]._pManaBase = plr[pnum]._pMaxManaBase;
	plr[pnum]._pHitPoints = plr[pnum]._pMaxHP;
	plr[pnum]._pHPBase = plr[pnum]._pMaxHPBase;

	InitDiabloMsg(EMSG_SHRINE_DIVINE);

	return true;
}

bool OperateShrineHoly(int pnum)
{
	if (deltaload)
		return false;

	int j = 0;
	int xx, yy;
	DWORD lv;
	do {
		xx = GenerateRnd(MAXDUNX);
		yy = GenerateRnd(MAXDUNY);
		lv = dPiece[xx][yy];
		j++;
		if (j > MAXDUNX * MAXDUNY)
			break;
	} while (nSolidTable[lv] || dObject[xx][yy] != 0 || dMonster[xx][yy] != 0);

	AddMissile(plr[pnum].position.tile.x, plr[pnum].position.tile.y, xx, yy, plr[pnum]._pdir, MIS_RNDTELEPORT, -1, pnum, 0, 2 * leveltype);

	if (pnum != myplr)
		return false;

	InitDiabloMsg(EMSG_SHRINE_HOLY);

	return true;
}

bool OperateShrineSacred(int pnum)
{
	if (deltaload || pnum != myplr)
		return false;

	plr[pnum]._pMemSpells |= GetSpellBitmask(SPL_CBOLT);

	if (plr[pnum]._pSplLvl[SPL_CBOLT] < MAX_SPELL_LEVEL)
		plr[pnum]._pSplLvl[SPL_CBOLT]++;
	if (plr[pnum]._pSplLvl[SPL_CBOLT] < MAX_SPELL_LEVEL)
		plr[pnum]._pSplLvl[SPL_CBOLT]++;

	DWORD t = plr[pnum]._pMaxManaBase / 10;
	int v1 = plr[pnum]._pMana - plr[pnum]._pManaBase;
	int v2 = plr[pnum]._pMaxMana - plr[pnum]._pMaxManaBase;
	plr[pnum]._pManaBase -= t;
	plr[pnum]._pMana -= t;
	plr[pnum]._pMaxMana -= t;
	plr[pnum]._pMaxManaBase -= t;
	if (plr[pnum]._pMana >> 6 <= 0) {
		plr[pnum]._pMana = v1;
		plr[pnum]._pManaBase = 0;
	}
	if (plr[pnum]._pMaxMana >> 6 <= 0) {
		plr[pnum]._pMaxMana = v2;
		plr[pnum]._pMaxManaBase = 0;
	}

	InitDiabloMsg(EMSG_SHRINE_SACRED);

	return true;
}

bool OperateShrineSpiritual(int pnum)
{
	if (deltaload)
		return false;
	if (pnum != myplr)
		return false;

	for (int8_t &gridItem : plr[pnum].InvGrid) {
		if (gridItem == 0) {
			int r = 5 * leveltype + GenerateRnd(10 * leveltype);
			DWORD t = plr[pnum]._pNumInv; // check
			plr[pnum].InvList[t] = golditem;
			plr[pnum].InvList[t]._iSeed = AdvanceRndSeed();
			plr[pnum]._pNumInv++;
			gridItem = plr[pnum]._pNumInv;
			plr[pnum].InvList[t]._ivalue = r;
			plr[pnum]._pGold += r;
			SetGoldCurs(pnum, t);
		}
	}

	InitDiabloMsg(EMSG_SHRINE_SPIRITUAL);

	return true;
}

bool OperateShrineSpooky(int pnum)
{
	if (deltaload)
		return false;

	if (pnum == myplr) {
		InitDiabloMsg(EMSG_SHRINE_SPOOKY1);
		return true;
	}

	plr[myplr]._pHitPoints = plr[myplr]._pMaxHP;
	plr[myplr]._pHPBase = plr[myplr]._pMaxHPBase;
	plr[myplr]._pMana = plr[myplr]._pMaxMana;
	plr[myplr]._pManaBase = plr[myplr]._pMaxManaBase;

	InitDiabloMsg(EMSG_SHRINE_SPOOKY2);

	return true;
}

bool OperateShrineAbandoned(int pnum)
{
	if (deltaload)
		return false;
	if (pnum != myplr)
		return false;

	ModifyPlrDex(pnum, 2);
	CheckStats(pnum);

	if (pnum != myplr)
		return true;

	InitDiabloMsg(EMSG_SHRINE_ABANDONED);

	return true;
}

bool OperateShrineCreepy(int pnum)
{
	if (deltaload)
		return false;
	if (pnum != myplr)
		return false;

	ModifyPlrStr(pnum, 2);
	CheckStats(pnum);

	if (pnum != myplr)
		return true;

	InitDiabloMsg(EMSG_SHRINE_CREEPY);

	return true;
}

bool OperateShrineQuiet(int pnum)
{
	if (deltaload)
		return false;
	if (pnum != myplr)
		return false;

	ModifyPlrVit(pnum, 2);
	CheckStats(pnum);

	if (pnum != myplr)
		return true;

	InitDiabloMsg(EMSG_SHRINE_QUIET);

	return true;
}

bool OperateShrineSecluded(int pnum)
{
	if (deltaload)
		return false;
	if (pnum != myplr)
		return true;

	std::fill(&AutomapView[0][0], &AutomapView[DMAXX - 1][DMAXX - 1], true);

	InitDiabloMsg(EMSG_SHRINE_SECLUDED);

	return true;
}

bool OperateShrineOrnate(int pnum)
{
	if (deltaload)
		return false;
	if (pnum != myplr)
		return false;

	plr[pnum]._pMemSpells |= GetSpellBitmask(SPL_HBOLT);
	if (plr[pnum]._pSplLvl[SPL_HBOLT] < MAX_SPELL_LEVEL)
		plr[pnum]._pSplLvl[SPL_HBOLT]++;
	if (plr[pnum]._pSplLvl[SPL_HBOLT] < MAX_SPELL_LEVEL)
		plr[pnum]._pSplLvl[SPL_HBOLT]++;

	DWORD t = plr[pnum]._pMaxManaBase / 10;
	int v1 = plr[pnum]._pMana - plr[pnum]._pManaBase;
	int v2 = plr[pnum]._pMaxMana - plr[pnum]._pMaxManaBase;
	plr[pnum]._pManaBase -= t;
	plr[pnum]._pMana -= t;
	plr[pnum]._pMaxMana -= t;
	plr[pnum]._pMaxManaBase -= t;
	if (plr[pnum]._pMana >> 6 <= 0) {
		plr[pnum]._pMana = v1;
		plr[pnum]._pManaBase = 0;
	}
	if (plr[pnum]._pMaxMana >> 6 <= 0) {
		plr[pnum]._pMaxMana = v2;
		plr[pnum]._pMaxManaBase = 0;
	}

	InitDiabloMsg(EMSG_SHRINE_ORNATE);

	return true;
}

bool OperateShrineGlimmering(int pnum)
{
	if (deltaload)
		return false;
	if (pnum != myplr)
		return false;

	for (auto &item : plr[pnum].InvBody) {
		if (item._iMagical && !item._iIdentified)
			item._iIdentified = true;
	}
	for (int j = 0; j < plr[pnum]._pNumInv; j++) {
		if (plr[pnum].InvList[j]._iMagical && !plr[pnum].InvList[j]._iIdentified)
			plr[pnum].InvList[j]._iIdentified = true;
	}
	for (auto &item : plr[pnum].SpdList) {
		if (item._iMagical && !item._iIdentified)
			item._iIdentified = true; // belt items can't be magical?
	}

	InitDiabloMsg(EMSG_SHRINE_GLIMMERING);

	return true;
}

bool OperateShrineTainted(int pnum)
{
	if (deltaload)
		return false;

	if (pnum == myplr) {
		InitDiabloMsg(EMSG_SHRINE_TAINTED1);
		return true;
	}

	int r = GenerateRnd(4);

	int v1 = r == 0 ? 1 : -1;
	int v2 = r == 1 ? 1 : -1;
	int v3 = r == 2 ? 1 : -1;
	int v4 = r == 3 ? 1 : -1;

	ModifyPlrStr(myplr, v1);
	ModifyPlrMag(myplr, v2);
	ModifyPlrDex(myplr, v3);
	ModifyPlrVit(myplr, v4);

	CheckStats(myplr);

	InitDiabloMsg(EMSG_SHRINE_TAINTED2);

	return true;
}

bool OperateShrineOily(int pnum, int x, int y)
{
	if (deltaload)
		return false;
	if (pnum != myplr)
		return false;

	switch (plr[myplr]._pClass) {
	case HeroClass::Warrior:
		ModifyPlrStr(myplr, 2);
		break;
	case HeroClass::Rogue:
		ModifyPlrDex(myplr, 2);
		break;
	case HeroClass::Sorcerer:
		ModifyPlrMag(myplr, 2);
		break;
	case HeroClass::Barbarian:
		ModifyPlrVit(myplr, 2);
		break;
	case HeroClass::Monk:
		ModifyPlrStr(myplr, 1);
		ModifyPlrDex(myplr, 1);
		break;
	case HeroClass::Bard:
		ModifyPlrDex(myplr, 1);
		ModifyPlrMag(myplr, 1);
		break;
	}

	CheckStats(pnum);

	AddMissile(
	    x,
	    y,
	    plr[myplr].position.tile.x,
	    plr[myplr].position.tile.y,
	    plr[myplr]._pdir,
	    MIS_FIREWALL,
	    TARGET_PLAYERS,
	    -1,
	    2 * currlevel + 2,
	    0);

	InitDiabloMsg(EMSG_SHRINE_OILY);

	return true;
}

bool OperateShrineGlowing(int pnum)
{
	if (deltaload)
		return false;
	if (pnum != myplr)
		return false;

	int playerXP = plr[myplr]._pExperience;
	int magicGain = playerXP / 1000;
	int xpLoss = 0;
	if (playerXP > 5000) {
		magicGain = 5;
		xpLoss = ((double)playerXP * 0.95);
	}
	ModifyPlrMag(myplr, magicGain);
	plr[myplr]._pExperience = xpLoss;

	if (sgOptions.Gameplay.bExperienceBar) {
		force_redraw = 255;
	}

	CheckStats(pnum);

	InitDiabloMsg(EMSG_SHRINE_GLOWING);

	return true;
}

bool OperateShrineMendicant(int pnum)
{
	if (deltaload)
		return false;
	if (pnum != myplr)
		return false;

	int gold = plr[myplr]._pGold / 2;
	AddPlrExperience(myplr, plr[myplr]._pLevel, gold);
	TakePlrsMoney(gold);

	CheckStats(pnum);

	InitDiabloMsg(EMSG_SHRINE_MENDICANT);

	return true;
}

bool OperateShrineSparkling(int pnum, int x, int y)
{
	if (deltaload)
		return false;
	if (pnum != myplr)
		return false;

	AddPlrExperience(myplr, plr[myplr]._pLevel, 1000 * currlevel);

	AddMissile(
	    x,
	    y,
	    plr[myplr].position.tile.x,
	    plr[myplr].position.tile.y,
	    plr[myplr]._pdir,
	    MIS_FLASH,
	    TARGET_PLAYERS,
	    -1,
	    3 * currlevel + 2,
	    0);

	CheckStats(pnum);

	InitDiabloMsg(EMSG_SHRINE_SPARKLING);

	return true;
}

bool OperateShrineTown(int pnum, int x, int y)
{
	if (deltaload)
		return false;
	if (pnum != myplr)
		return false;

	AddMissile(
	    x,
	    y,
	    plr[myplr].position.tile.x,
	    plr[myplr].position.tile.y,
	    plr[myplr]._pdir,
	    MIS_TOWN,
	    TARGET_PLAYERS,
	    pnum,
	    0,
	    0);

	InitDiabloMsg(EMSG_SHRINE_TOWN);

	return true;
}

bool OperateShrineShimmering(int pnum)
{
	if (deltaload)
		return false;
	if (pnum != myplr)
		return false;

	plr[pnum]._pMana = plr[pnum]._pMaxMana;
	plr[pnum]._pManaBase = plr[pnum]._pMaxManaBase;

	InitDiabloMsg(EMSG_SHRINE_SHIMMERING);

	return true;
}

bool OperateShrineSolar(int pnum)
{
	if (deltaload)
		return false;
	if (pnum != myplr)
		return false;

	time_t tm = time(nullptr);
	int hour = localtime(&tm)->tm_hour;
	if (hour >= 20 || hour < 4) {
		InitDiabloMsg(EMSG_SHRINE_SOLAR4);
		ModifyPlrVit(myplr, 2);
	} else if (hour >= 18) {
		InitDiabloMsg(EMSG_SHRINE_SOLAR3);
		ModifyPlrMag(myplr, 2);
	} else if (hour >= 12) {
		InitDiabloMsg(EMSG_SHRINE_SOLAR2);
		ModifyPlrStr(myplr, 2);
	} else /* 4:00 to 11:59 */ {
		InitDiabloMsg(EMSG_SHRINE_SOLAR1);
		ModifyPlrDex(myplr, 2);
	}

	CheckStats(pnum);

	return true;
}

bool OperateShrineMurphys(int pnum)
{
	if (deltaload)
		return false;
	if (pnum != myplr)
		return false;

	bool broke = false;
	for (auto &item : plr[myplr].InvBody) {
		if (!item.isEmpty() && GenerateRnd(3) == 0) {
			if (item._iDurability != DUR_INDESTRUCTIBLE) {
				if (item._iDurability) {
					item._iDurability /= 2;
					broke = true;
					break;
				}
			}
		}
	}
	if (!broke) {
		TakePlrsMoney(plr[myplr]._pGold / 3);
	}

	InitDiabloMsg(EMSG_SHRINE_MURPHYS);

	return true;
}

void OperateShrine(int pnum, int i, _sfx_id sType)
{
	if (dropGoldFlag) {
		dropGoldFlag = false;
		dropGoldValue = 0;
	}

	assert((DWORD)i < MAXOBJECTS);

	if (object[i]._oSelFlag == 0)
		return;

	SetRndSeed(object[i]._oRndSeed);
	object[i]._oSelFlag = 0;

	if (!deltaload) {
		PlaySfxLoc(sType, object[i].position.x, object[i].position.y);
		object[i]._oAnimFlag = 1;
		object[i]._oAnimDelay = 1;
	} else {
		object[i]._oAnimFrame = object[i]._oAnimLen;
		object[i]._oAnimFlag = 0;
	}

	switch (object[i]._oVar1) {
	case SHRINE_MYSTERIOUS:
		if (!OperateShrineMysterious(pnum))
			return;
		break;
	case SHRINE_HIDDEN:
		if (!OperateShrineHidden(pnum))
			return;
		break;
	case SHRINE_GLOOMY:
		if (!OperateShrineGloomy(pnum))
			return;
		break;
	case SHRINE_WEIRD:
		if (!OperateShrineWeird(pnum))
			return;
		break;
	case SHRINE_MAGICAL:
	case SHRINE_MAGICAL2:
		if (!OperateShrineMagical(pnum))
			return;
		break;
	case SHRINE_STONE:
		if (!OperateShrineStone(pnum))
			return;
		break;
	case SHRINE_RELIGIOUS:
		if (!OperateShrineReligious(pnum))
			return;
		break;
	case SHRINE_ENCHANTED:
		if (!OperateShrineEnchanted(pnum))
			return;
		break;
	case SHRINE_THAUMATURGIC:
		if (!OperateShrineThaumaturgic(pnum))
			return;
		break;
	case SHRINE_FASCINATING:
		if (!OperateShrineFascinating(pnum))
			return;
		break;
	case SHRINE_CRYPTIC:
		if (!OperateShrineCryptic(pnum))
			return;
		break;
	case SHRINE_ELDRITCH:
		if (!OperateShrineEldritch(pnum))
			return;
		break;
	case SHRINE_EERIE:
		if (!OperateShrineEerie(pnum))
			return;
		break;
	case SHRINE_DIVINE:
		if (!OperateShrineDivine(pnum, object[i].position.x, object[i].position.y))
			return;
		break;
	case SHRINE_HOLY:
		if (!OperateShrineHoly(pnum))
			return;
		break;
	case SHRINE_SACRED:
		if (!OperateShrineSacred(pnum))
			return;
		break;
	case SHRINE_SPIRITUAL:
		if (!OperateShrineSpiritual(pnum))
			return;
		break;
	case SHRINE_SPOOKY:
		if (!OperateShrineSpooky(pnum))
			return;
		break;
	case SHRINE_ABANDONED:
		if (!OperateShrineAbandoned(pnum))
			return;
		break;
	case SHRINE_CREEPY:
		if (!OperateShrineCreepy(pnum))
			return;
		break;
	case SHRINE_QUIET:
		if (!OperateShrineQuiet(pnum))
			return;
		break;
	case SHRINE_SECLUDED:
		if (!OperateShrineSecluded(pnum))
			return;
		break;
	case SHRINE_ORNATE:
		if (!OperateShrineOrnate(pnum))
			return;
		break;
	case SHRINE_GLIMMERING:
		if (!OperateShrineGlimmering(pnum))
			return;
		break;
	case SHRINE_TAINTED:
		if (!OperateShrineTainted(pnum))
			return;
		break;
	case SHRINE_OILY:
		if (!OperateShrineOily(pnum, object[i].position.x, object[i].position.y))
			return;
		break;
	case SHRINE_GLOWING:
		if (!OperateShrineGlowing(pnum))
			return;
		break;
	case SHRINE_MENDICANT:
		if (!OperateShrineMendicant(pnum))
			return;
		break;
	case SHRINE_SPARKLING:
		if (!OperateShrineSparkling(pnum, object[i].position.x, object[i].position.y))
			return;
		break;
	case SHRINE_TOWN:
		if (!OperateShrineTown(pnum, object[i].position.x, object[i].position.y))
			return;
		break;
	case SHRINE_SHIMMERING:
		if (!OperateShrineShimmering(pnum))
			return;
		break;
	case SHRINE_SOLAR:
		if (!OperateShrineSolar(pnum))
			return;
		break;
	case SHRINE_MURPHYS:
		if (!OperateShrineMurphys(pnum))
			return;
		break;
	}

	CalcPlrInv(pnum, true);
	force_redraw = 255;

	if (pnum == myplr)
		NetSendCmdParam2(false, CMD_PLROPOBJ, pnum, i);
}

void OperateSkelBook(int pnum, int i, bool sendmsg)
{
	if (object[i]._oSelFlag != 0) {
		if (!deltaload)
			PlaySfxLoc(IS_ISCROL, object[i].position.x, object[i].position.y);
		object[i]._oSelFlag = 0;
		object[i]._oAnimFrame += 2;
		if (!deltaload) {
			SetRndSeed(object[i]._oRndSeed);
			if (GenerateRnd(5) != 0)
				CreateTypeItem(object[i].position.x, object[i].position.y, false, ITYPE_MISC, IMISC_SCROLL, sendmsg, false);
			else
				CreateTypeItem(object[i].position.x, object[i].position.y, false, ITYPE_MISC, IMISC_BOOK, sendmsg, false);
			if (pnum == myplr)
				NetSendCmdParam1(false, CMD_OPERATEOBJ, i);
		}
	}
}

void OperateBookCase(int pnum, int i, bool sendmsg)
{
	if (object[i]._oSelFlag != 0) {
		if (!deltaload)
			PlaySfxLoc(IS_ISCROL, object[i].position.x, object[i].position.y);
		object[i]._oSelFlag = 0;
		object[i]._oAnimFrame -= 2;
		if (!deltaload) {
			SetRndSeed(object[i]._oRndSeed);
			CreateTypeItem(object[i].position.x, object[i].position.y, false, ITYPE_MISC, IMISC_BOOK, sendmsg, false);
			if (QuestStatus(Q_ZHAR)
			    && monster[MAX_PLRS]._mmode == MM_STAND // prevents playing the "angry" message for the second time if zhar got aggroed by losing vision and talking again
			    && monster[MAX_PLRS]._uniqtype - 1 == UMT_ZHAR
			    && monster[MAX_PLRS]._msquelch == UINT8_MAX
			    && monster[MAX_PLRS]._mhitpoints) {
				monster[MAX_PLRS].mtalkmsg = TEXT_ZHAR2;
				M_StartStand(0, monster[MAX_PLRS]._mdir);
				monster[MAX_PLRS]._mgoal = MGOAL_ATTACK2;
				monster[MAX_PLRS]._mmode = MM_TALK;
			}
			if (pnum == myplr)
				NetSendCmdParam1(false, CMD_OPERATEOBJ, i);
		}
	}
}

void OperateDecap(int pnum, int i, bool sendmsg)
{
	if (object[i]._oSelFlag != 0) {
		object[i]._oSelFlag = 0;
		if (!deltaload) {
			SetRndSeed(object[i]._oRndSeed);
			CreateRndItem(object[i].position.x, object[i].position.y, false, sendmsg, false);
			if (pnum == myplr)
				NetSendCmdParam1(false, CMD_OPERATEOBJ, i);
		}
	}
}

void OperateArmorStand(int pnum, int i, bool sendmsg)
{
	bool uniqueRnd;

	if (object[i]._oSelFlag != 0) {
		object[i]._oSelFlag = 0;
		object[i]._oAnimFrame++;
		if (!deltaload) {
			SetRndSeed(object[i]._oRndSeed);
			uniqueRnd = (GenerateRnd(2) != 0);
			if (currlevel <= 5) {
				CreateTypeItem(object[i].position.x, object[i].position.y, true, ITYPE_LARMOR, IMISC_NONE, sendmsg, false);
			} else if (currlevel >= 6 && currlevel <= 9) {
				CreateTypeItem(object[i].position.x, object[i].position.y, uniqueRnd, ITYPE_MARMOR, IMISC_NONE, sendmsg, false);
			} else if (currlevel >= 10 && currlevel <= 12) {
				CreateTypeItem(object[i].position.x, object[i].position.y, false, ITYPE_HARMOR, IMISC_NONE, sendmsg, false);
			} else if (currlevel >= 13 && currlevel <= 16) {
				CreateTypeItem(object[i].position.x, object[i].position.y, true, ITYPE_HARMOR, IMISC_NONE, sendmsg, false);
			} else if (currlevel >= 17) {
				CreateTypeItem(object[i].position.x, object[i].position.y, true, ITYPE_HARMOR, IMISC_NONE, sendmsg, false);
			}
			if (pnum == myplr)
				NetSendCmdParam1(false, CMD_OPERATEOBJ, i);
			return;
		}
	}
}

int FindValidShrine()
{
	int rv;
	bool done;

	done = false;
	do {
		rv = GenerateRnd(gbIsHellfire ? NUM_SHRINETYPE : 26);
		if (currlevel >= shrinemin[rv] && currlevel <= shrinemax[rv] && rv != SHRINE_THAUMATURGIC) {
			done = true;
		}
		if (done) {
			if (gbIsMultiplayer) {
				if (shrineavail[rv] == SHRINETYPE_SINGLE) {
					done = false;
					continue;
				}
			}
			if (!gbIsMultiplayer) {
				if (shrineavail[rv] == SHRINETYPE_MULTI) {
					done = false;
					continue;
				}
			}
			done = true;
		}
	} while (!done);
	return rv;
}

void OperateGoatShrine(int pnum, int i, _sfx_id sType)
{
	SetRndSeed(object[i]._oRndSeed);
	object[i]._oVar1 = FindValidShrine();
	OperateShrine(pnum, i, sType);
	object[i]._oAnimDelay = 2;
	force_redraw = 255;
}

void OperateCauldron(int pnum, int i, _sfx_id sType)
{
	SetRndSeed(object[i]._oRndSeed);
	object[i]._oVar1 = FindValidShrine();
	OperateShrine(pnum, i, sType);
	object[i]._oAnimFrame = 3;
	object[i]._oAnimFlag = 0;
	force_redraw = 255;
}

bool OperateFountains(int pnum, int i)
{
	int prev, add, rnd, cnt;
	bool applied;
	bool done;

	applied = false;
	SetRndSeed(object[i]._oRndSeed);
	switch (object[i]._otype) {
	case OBJ_BLOODFTN:
		if (deltaload)
			return false;
		if (pnum != myplr)
			return false;

		if (plr[pnum]._pHitPoints < plr[pnum]._pMaxHP) {
			PlaySfxLoc(LS_FOUNTAIN, object[i].position.x, object[i].position.y);
			plr[pnum]._pHitPoints += 64;
			plr[pnum]._pHPBase += 64;
			if (plr[pnum]._pHitPoints > plr[pnum]._pMaxHP) {
				plr[pnum]._pHitPoints = plr[pnum]._pMaxHP;
				plr[pnum]._pHPBase = plr[pnum]._pMaxHPBase;
			}
			applied = true;
		} else
			PlaySfxLoc(LS_FOUNTAIN, object[i].position.x, object[i].position.y);
		break;
	case OBJ_PURIFYINGFTN:
		if (deltaload)
			return false;
		if (pnum != myplr)
			return false;

		if (plr[pnum]._pMana < plr[pnum]._pMaxMana) {
			PlaySfxLoc(LS_FOUNTAIN, object[i].position.x, object[i].position.y);

			plr[pnum]._pMana += 64;
			plr[pnum]._pManaBase += 64;
			if (plr[pnum]._pMana > plr[pnum]._pMaxMana) {
				plr[pnum]._pMana = plr[pnum]._pMaxMana;
				plr[pnum]._pManaBase = plr[pnum]._pMaxManaBase;
			}

			applied = true;
		} else
			PlaySfxLoc(LS_FOUNTAIN, object[i].position.x, object[i].position.y);
		break;
	case OBJ_MURKYFTN:
		if (object[i]._oSelFlag == 0)
			break;
		if (!deltaload)
			PlaySfxLoc(LS_FOUNTAIN, object[i].position.x, object[i].position.y);
		object[i]._oSelFlag = 0;
		if (deltaload)
			return false;
		AddMissile(
		    plr[pnum].position.tile.x,
		    plr[pnum].position.tile.y,
		    plr[pnum].position.tile.x,
		    plr[pnum].position.tile.y,
		    plr[pnum]._pdir,
		    MIS_INFRA,
		    -1,
		    pnum,
		    0,
		    2 * leveltype);
		applied = true;
		if (pnum == myplr)
			NetSendCmdParam1(false, CMD_OPERATEOBJ, i);
		break;
	case OBJ_TEARFTN:
		if (object[i]._oSelFlag == 0)
			break;
		prev = -1;
		add = -1;
		done = false;
		cnt = 0;
		if (!deltaload)
			PlaySfxLoc(LS_FOUNTAIN, object[i].position.x, object[i].position.y);
		object[i]._oSelFlag = 0;
		if (deltaload)
			return false;
		if (pnum != myplr)
			return false;
		while (!done) {
			rnd = GenerateRnd(4);
			if (rnd != prev) {
				switch (rnd) {
				case 0:
					ModifyPlrStr(pnum, add);
					break;
				case 1:
					ModifyPlrMag(pnum, add);
					break;
				case 2:
					ModifyPlrDex(pnum, add);
					break;
				case 3:
					ModifyPlrVit(pnum, add);
					break;
				}
				prev = rnd;
				add = 1;
				cnt++;
			}
			if (cnt <= 1)
				continue;

			done = true;
		}
		CheckStats(pnum);
		applied = true;
		if (pnum == myplr)
			NetSendCmdParam1(false, CMD_OPERATEOBJ, i);
		break;
	default:
		break;
	}
	force_redraw = 255;
	return applied;
}

void OperateWeaponRack(int pnum, int i, bool sendmsg)
{
	int weaponType;

	if (object[i]._oSelFlag == 0)
		return;
	SetRndSeed(object[i]._oRndSeed);

	switch (GenerateRnd(4) + ITYPE_SWORD) {
	case ITYPE_SWORD:
		weaponType = ITYPE_SWORD;
		break;
	case ITYPE_AXE:
		weaponType = ITYPE_AXE;
		break;
	case ITYPE_BOW:
		weaponType = ITYPE_BOW;
		break;
	case ITYPE_MACE:
		weaponType = ITYPE_MACE;
		break;
	}

	object[i]._oSelFlag = 0;
	object[i]._oAnimFrame++;
	if (deltaload)
		return;

	CreateTypeItem(object[i].position.x, object[i].position.y, leveltype > 1, weaponType, IMISC_NONE, sendmsg, false);

	if (pnum == myplr)
		NetSendCmdParam1(false, CMD_OPERATEOBJ, i);
}

void OperateStoryBook(int pnum, int i)
{
	if (object[i]._oSelFlag != 0 && !deltaload && !qtextflag && pnum == myplr) {
		object[i]._oAnimFrame = object[i]._oVar4;
		PlaySfxLoc(IS_ISCROL, object[i].position.x, object[i].position.y);
		if (object[i]._oVar8 != 0 && currlevel == 24) {
			if (IsUberLeverActivated != 1 && quests[Q_NAKRUL]._qactive != QUEST_DONE && objects_lv_24_454B04(object[i]._oVar8)) {
				NetSendCmd(false, CMD_NAKRUL);
				return;
			}
		} else if (currlevel >= 21) {
			quests[Q_NAKRUL]._qactive = QUEST_ACTIVE;
			quests[Q_NAKRUL]._qlog = true;
			quests[Q_NAKRUL]._qmsg = static_cast<_speech_id>(object[i]._oVar2);
		}
		InitQTextMsg(object[i]._oVar2);
		NetSendCmdParam1(false, CMD_OPERATEOBJ, i);
	}
}

void OperateLazStand(int pnum, int i)
{
	int xx, yy;

	if (numitems >= MAXITEMS) {
		return;
	}

	if (object[i]._oSelFlag != 0 && !deltaload && !qtextflag && pnum == myplr) {
		object[i]._oAnimFrame++;
		object[i]._oSelFlag = 0;
		GetSuperItemLoc(object[i].position.x, object[i].position.y, &xx, &yy);
		SpawnQuestItem(IDI_LAZSTAFF, xx, yy, 0, false);
	}
}

bool objectIsDisabled(int i)
{
	if (!sgOptions.Gameplay.bDisableCripplingShrines)
		return false;
	if ((object[i]._otype == OBJ_GOATSHRINE) || (object[i]._otype == OBJ_CAULDRON))
		return true;
	if ((object[i]._otype != OBJ_SHRINEL) && (object[i]._otype != OBJ_SHRINER))
		return false;
	if ((object[i]._oVar1 == SHRINE_FASCINATING)
	    || (object[i]._oVar1 == SHRINE_ORNATE)
	    || (object[i]._oVar1 == SHRINE_SACRED))
		return true;
	return false;
}

void OperateObject(int pnum, int i, bool TeleFlag)
{
	bool sendmsg;

	sendmsg = (pnum == myplr);
	switch (object[i]._otype) {
	case OBJ_L1LDOOR:
	case OBJ_L1RDOOR:
		if (TeleFlag) {
			if (object[i]._otype == OBJ_L1LDOOR)
				OperateL1LDoor(pnum, i, true);
			if (object[i]._otype == OBJ_L1RDOOR)
				OperateL1RDoor(pnum, i, true);
			break;
		}
		if (pnum == myplr)
			OperateL1Door(pnum, i, true);
		break;
	case OBJ_L2LDOOR:
	case OBJ_L2RDOOR:
		if (TeleFlag) {
			if (object[i]._otype == OBJ_L2LDOOR)
				OperateL2LDoor(pnum, i, true);
			if (object[i]._otype == OBJ_L2RDOOR)
				OperateL2RDoor(pnum, i, true);
			break;
		}
		if (pnum == myplr)
			OperateL2Door(pnum, i, true);
		break;
	case OBJ_L3LDOOR:
	case OBJ_L3RDOOR:
		if (TeleFlag) {
			if (object[i]._otype == OBJ_L3LDOOR)
				OperateL3LDoor(pnum, i, true);
			if (object[i]._otype == OBJ_L3RDOOR)
				OperateL3RDoor(pnum, i, true);
			break;
		}
		if (pnum == myplr)
			OperateL3Door(pnum, i, true);
		break;
	case OBJ_LEVER:
	case OBJ_SWITCHSKL:
		OperateLever(pnum, i);
		break;
	case OBJ_BOOK2L:
		OperateBook(pnum, i);
		break;
	case OBJ_BOOK2R:
		OperateSChambBk(i);
		break;
	case OBJ_CHEST1:
	case OBJ_CHEST2:
	case OBJ_CHEST3:
	case OBJ_TCHEST1:
	case OBJ_TCHEST2:
	case OBJ_TCHEST3:
		OperateChest(pnum, i, sendmsg);
		break;
	case OBJ_SARC:
		OperateSarc(pnum, i, sendmsg);
		break;
	case OBJ_FLAMELVR:
		OperateTrapLvr(i);
		break;
	case OBJ_BLINDBOOK:
	case OBJ_BLOODBOOK:
	case OBJ_STEELTOME:
		OperateBookLever(pnum, i);
		break;
	case OBJ_SHRINEL:
	case OBJ_SHRINER:
		OperateShrine(pnum, i, IS_MAGIC);
		break;
	case OBJ_SKELBOOK:
	case OBJ_BOOKSTAND:
		OperateSkelBook(pnum, i, sendmsg);
		break;
	case OBJ_BOOKCASEL:
	case OBJ_BOOKCASER:
		OperateBookCase(pnum, i, sendmsg);
		break;
	case OBJ_DECAP:
		OperateDecap(pnum, i, sendmsg);
		break;
	case OBJ_ARMORSTAND:
	case OBJ_WARARMOR:
		OperateArmorStand(pnum, i, sendmsg);
		break;
	case OBJ_GOATSHRINE:
		OperateGoatShrine(pnum, i, LS_GSHRINE);
		break;
	case OBJ_CAULDRON:
		OperateCauldron(pnum, i, LS_CALDRON);
		break;
	case OBJ_BLOODFTN:
	case OBJ_PURIFYINGFTN:
	case OBJ_MURKYFTN:
	case OBJ_TEARFTN:
		OperateFountains(pnum, i);
		break;
	case OBJ_STORYBOOK:
		OperateStoryBook(pnum, i);
		break;
	case OBJ_PEDISTAL:
		OperatePedistal(pnum, i);
		break;
	case OBJ_WARWEAP:
	case OBJ_WEAPONRACK:
		OperateWeaponRack(pnum, i, sendmsg);
		break;
	case OBJ_MUSHPATCH:
		OperateMushPatch(pnum, i);
		break;
	case OBJ_LAZSTAND:
		OperateLazStand(pnum, i);
		break;
	case OBJ_SLAINHERO:
		OperateSlainHero(pnum, i);
		break;
	case OBJ_SIGNCHEST:
		OperateInnSignChest(pnum, i);
		break;
	default:
		break;
	}
}

void SyncOpL1Door(int pnum, int cmd, int i)
{
	bool do_sync;

	if (pnum == myplr)
		return;

	do_sync = false;
	if (cmd == CMD_OPENDOOR && object[i]._oVar4 == 0) {
		do_sync = true;
	}
	if (cmd == CMD_CLOSEDOOR && object[i]._oVar4 == 1)
		do_sync = true;
	if (do_sync) {
		if (object[i]._otype == OBJ_L1LDOOR)
			OperateL1LDoor(-1, i, false);
		if (object[i]._otype == OBJ_L1RDOOR)
			OperateL1RDoor(-1, i, false);
	}
}

void SyncOpL2Door(int pnum, int cmd, int i)
{
	bool do_sync;

	if (pnum == myplr)
		return;

	do_sync = false;
	if (cmd == CMD_OPENDOOR && object[i]._oVar4 == 0) {
		do_sync = true;
	}
	if (cmd == CMD_CLOSEDOOR && object[i]._oVar4 == 1)
		do_sync = true;
	if (do_sync) {
		if (object[i]._otype == OBJ_L2LDOOR)
			OperateL2LDoor(-1, i, false);
		if (object[i]._otype == OBJ_L2RDOOR)
			OperateL2RDoor(-1, i, false);
	}
}

void SyncOpL3Door(int pnum, int cmd, int i)
{
	bool do_sync;

	if (pnum == myplr)
		return;

	do_sync = false;
	if (cmd == CMD_OPENDOOR && object[i]._oVar4 == 0) {
		do_sync = true;
	}
	if (cmd == CMD_CLOSEDOOR && object[i]._oVar4 == 1)
		do_sync = true;
	if (do_sync) {
		if (object[i]._otype == OBJ_L3LDOOR)
			OperateL3LDoor(-1, i, false);
		if (object[i]._otype == OBJ_L3RDOOR)
			OperateL3RDoor(-1, i, false);
	}
}

void SyncOpObject(int pnum, int cmd, int i)
{
	switch (object[i]._otype) {
	case OBJ_L1LDOOR:
	case OBJ_L1RDOOR:
		SyncOpL1Door(pnum, cmd, i);
		break;
	case OBJ_L2LDOOR:
	case OBJ_L2RDOOR:
		SyncOpL2Door(pnum, cmd, i);
		break;
	case OBJ_L3LDOOR:
	case OBJ_L3RDOOR:
		SyncOpL3Door(pnum, cmd, i);
		break;
	case OBJ_LEVER:
	case OBJ_SWITCHSKL:
		OperateLever(pnum, i);
		break;
	case OBJ_CHEST1:
	case OBJ_CHEST2:
	case OBJ_CHEST3:
	case OBJ_TCHEST1:
	case OBJ_TCHEST2:
	case OBJ_TCHEST3:
		OperateChest(pnum, i, false);
		break;
	case OBJ_SARC:
		OperateSarc(pnum, i, false);
		break;
	case OBJ_BLINDBOOK:
	case OBJ_BLOODBOOK:
	case OBJ_STEELTOME:
		OperateBookLever(pnum, i);
		break;
	case OBJ_SHRINEL:
	case OBJ_SHRINER:
		OperateShrine(pnum, i, IS_MAGIC);
		break;
	case OBJ_SKELBOOK:
	case OBJ_BOOKSTAND:
		OperateSkelBook(pnum, i, false);
		break;
	case OBJ_BOOKCASEL:
	case OBJ_BOOKCASER:
		OperateBookCase(pnum, i, false);
		break;
	case OBJ_DECAP:
		OperateDecap(pnum, i, false);
		break;
	case OBJ_ARMORSTAND:
	case OBJ_WARARMOR:
		OperateArmorStand(pnum, i, false);
		break;
	case OBJ_GOATSHRINE:
		OperateGoatShrine(pnum, i, LS_GSHRINE);
		break;
	case OBJ_CAULDRON:
		OperateCauldron(pnum, i, LS_CALDRON);
		break;
	case OBJ_MURKYFTN:
	case OBJ_TEARFTN:
		OperateFountains(pnum, i);
		break;
	case OBJ_STORYBOOK:
		OperateStoryBook(pnum, i);
		break;
	case OBJ_PEDISTAL:
		OperatePedistal(pnum, i);
		break;
	case OBJ_WARWEAP:
	case OBJ_WEAPONRACK:
		OperateWeaponRack(pnum, i, false);
		break;
	case OBJ_MUSHPATCH:
		OperateMushPatch(pnum, i);
		break;
	case OBJ_SLAINHERO:
		OperateSlainHero(pnum, i);
		break;
	case OBJ_SIGNCHEST:
		OperateInnSignChest(pnum, i);
		break;
	default:
		break;
	}
}

void BreakCrux(int i)
{
	int j, oi;
	bool triggered;

	object[i]._oAnimFlag = 1;
	object[i]._oAnimFrame = 1;
	object[i]._oAnimDelay = 1;
	object[i]._oSolidFlag = true;
	object[i]._oMissFlag = true;
	object[i]._oBreak = -1;
	object[i]._oSelFlag = 0;
	triggered = true;
	for (j = 0; j < nobjects; j++) {
		oi = objectactive[j];
		if (object[oi]._otype != OBJ_CRUX1 && object[oi]._otype != OBJ_CRUX2 && object[oi]._otype != OBJ_CRUX3)
			continue;
		if (object[i]._oVar8 != object[oi]._oVar8 || object[oi]._oBreak == -1)
			continue;
		triggered = false;
	}
	if (!triggered)
		return;
	if (!deltaload)
		PlaySfxLoc(IS_LEVER, object[i].position.x, object[i].position.y);
	ObjChangeMap(object[i]._oVar1, object[i]._oVar2, object[i]._oVar3, object[i]._oVar4);
}

void BreakBarrel(int pnum, int i, int dam, bool forcebreak, bool sendmsg)
{
	int oi;
	int xp, yp;

	if (object[i]._oSelFlag == 0)
		return;
	if (forcebreak) {
		object[i]._oVar1 = 0;
	} else {
		object[i]._oVar1 -= dam;
		if (pnum != myplr && object[i]._oVar1 <= 0)
			object[i]._oVar1 = 1;
	}
	if (object[i]._oVar1 > 0) {
		if (deltaload)
			return;

		PlaySfxLoc(IS_IBOW, object[i].position.x, object[i].position.y);
		return;
	}

	object[i]._oVar1 = 0;
	object[i]._oAnimFlag = 1;
	object[i]._oAnimFrame = 1;
	object[i]._oAnimDelay = 1;
	object[i]._oSolidFlag = false;
	object[i]._oMissFlag = true;
	object[i]._oBreak = -1;
	object[i]._oSelFlag = 0;
	object[i]._oPreFlag = true;
	if (deltaload) {
		object[i]._oAnimFrame = object[i]._oAnimLen;
		object[i]._oAnimCnt = 0;
		object[i]._oAnimDelay = 1000;
		return;
	}

	if (object[i]._otype == OBJ_BARRELEX) {
		if (currlevel >= 21 && currlevel <= 24)
			PlaySfxLoc(IS_POPPOP3, object[i].position.x, object[i].position.y);
		else if (currlevel >= 17 && currlevel <= 20)
			PlaySfxLoc(IS_POPPOP8, object[i].position.x, object[i].position.y);
		else
			PlaySfxLoc(IS_BARLFIRE, object[i].position.x, object[i].position.y);
		for (yp = object[i].position.y - 1; yp <= object[i].position.y + 1; yp++) {
			for (xp = object[i].position.x - 1; xp <= object[i].position.x + 1; xp++) {
				if (dMonster[xp][yp] > 0)
					MonsterTrapHit(dMonster[xp][yp] - 1, 1, 4, 0, MIS_FIREBOLT, false);
				bool unused;
				if (dPlayer[xp][yp] > 0)
					PlayerMHit(dPlayer[xp][yp] - 1, -1, 0, 8, 16, MIS_FIREBOLT, false, 0, &unused);
				if (dObject[xp][yp] > 0) {
					oi = dObject[xp][yp] - 1;
					if (object[oi]._otype == OBJ_BARRELEX && object[oi]._oBreak != -1)
						BreakBarrel(pnum, oi, dam, true, sendmsg);
				}
			}
		}
	} else {
		if (currlevel >= 21 && currlevel <= 24)
			PlaySfxLoc(IS_POPPOP2, object[i].position.x, object[i].position.y);
		else if (currlevel >= 17 && currlevel <= 20)
			PlaySfxLoc(IS_POPPOP5, object[i].position.x, object[i].position.y);
		else
			PlaySfxLoc(IS_BARREL, object[i].position.x, object[i].position.y);
		SetRndSeed(object[i]._oRndSeed);
		if (object[i]._oVar2 <= 1) {
			if (object[i]._oVar3 == 0)
				CreateRndUseful(object[i].position.x, object[i].position.y, sendmsg);
			else
				CreateRndItem(object[i].position.x, object[i].position.y, false, sendmsg, false);
		}
		if (object[i]._oVar2 >= 8)
			SpawnSkeleton(object[i]._oVar4, object[i].position.x, object[i].position.y);
	}
	if (pnum == myplr)
		NetSendCmdParam2(false, CMD_BREAKOBJ, pnum, i);
}

void BreakObject(int pnum, int oi)
{
	int objdam, mind, maxd;

	if (pnum != -1) {
		mind = plr[pnum]._pIMinDam;
		maxd = plr[pnum]._pIMaxDam;
		objdam = GenerateRnd(maxd - mind + 1) + mind;
		objdam += plr[pnum]._pDamageMod + plr[pnum]._pIBonusDamMod + objdam * plr[pnum]._pIBonusDam / 100;
	} else {
		objdam = 10;
	}
	switch (object[oi]._otype) {
	case OBJ_CRUX1:
	case OBJ_CRUX2:
	case OBJ_CRUX3:
		BreakCrux(oi);
		break;
	case OBJ_BARREL:
	case OBJ_BARRELEX:
		BreakBarrel(pnum, oi, objdam, false, true);
		break;
	default:
		break;
	}
}

void SyncBreakObj(int pnum, int oi)
{
	if (object[oi]._otype >= OBJ_BARREL && object[oi]._otype <= OBJ_BARRELEX)
		BreakBarrel(pnum, oi, 0, true, false);
}

void SyncL1Doors(int i)
{
	int x, y;

	if (object[i]._oVar4 == 0) {
		object[i]._oMissFlag = false;
		return;
	}

	object[i]._oMissFlag = true;

	x = object[i].position.x;
	y = object[i].position.y;
	object[i]._oSelFlag = 2;
	if (currlevel < 17) {
		if (object[i]._otype == OBJ_L1LDOOR) {
			if (object[i]._oVar1 == 214)
				ObjSetMicro(x, y, 408);
			else
				ObjSetMicro(x, y, 393);
			dSpecial[x][y] = 7;
			objects_set_door_piece(x - 1, y);
			y--;
		} else {
			ObjSetMicro(x, y, 395);
			dSpecial[x][y] = 8;
			objects_set_door_piece(x, y - 1);
			x--;
		}
	} else {
		if (object[i]._otype == OBJ_L1LDOOR) {
			ObjSetMicro(x, y, 206);
			dSpecial[x][y] = 1;
			objects_set_door_piece(x - 1, y);
			y--;
		} else {
			ObjSetMicro(x, y, 209);
			dSpecial[x][y] = 2;
			objects_set_door_piece(x, y - 1);
			x--;
		}
	}
	DoorSet(i, x, y);
}

void SyncCrux(int i)
{
	bool found;
	int j, oi, type;

	found = true;
	for (j = 0; j < nobjects; j++) {
		oi = objectactive[j];
		type = object[oi]._otype;
		if (type != OBJ_CRUX1 && type != OBJ_CRUX2 && type != OBJ_CRUX3)
			continue;
		if (object[i]._oVar8 != object[oi]._oVar8 || object[oi]._oBreak == -1)
			continue;
		found = false;
	}
	if (found)
		ObjChangeMap(object[i]._oVar1, object[i]._oVar2, object[i]._oVar3, object[i]._oVar4);
}

void SyncLever(int i)
{
	if (object[i]._oSelFlag == 0)
		ObjChangeMap(object[i]._oVar1, object[i]._oVar2, object[i]._oVar3, object[i]._oVar4);
}

void SyncQSTLever(int i)
{
	int tren;

	if (object[i]._oAnimFrame == object[i]._oVar6) {
		ObjChangeMapResync(object[i]._oVar1, object[i]._oVar2, object[i]._oVar3, object[i]._oVar4);
		if (object[i]._otype == OBJ_BLINDBOOK) {
			tren = TransVal;
			TransVal = 9;
			DRLG_MRectTrans(object[i]._oVar1, object[i]._oVar2, object[i]._oVar3, object[i]._oVar4);
			TransVal = tren;
		}
	}
}

void SyncPedistal(int i)
{
	if (object[i]._oVar6 == 1)
		ObjChangeMapResync(setpc_x, setpc_y + 3, setpc_x + 2, setpc_y + 7);
	if (object[i]._oVar6 == 2) {
		ObjChangeMapResync(setpc_x, setpc_y + 3, setpc_x + 2, setpc_y + 7);
		ObjChangeMapResync(setpc_x + 6, setpc_y + 3, setpc_x + setpc_w, setpc_y + 7);
	}
	if (object[i]._oVar6 == 3) {
		ObjChangeMapResync(object[i]._oVar1, object[i]._oVar2, object[i]._oVar3, object[i]._oVar4);
		LoadMapObjs("Levels\\L2Data\\Blood2.DUN", 2 * setpc_x, 2 * setpc_y);
	}
}

void SyncL2Doors(int i)
{
	int x, y;

	if (object[i]._oVar4 == 0)
		object[i]._oMissFlag = false;
	else
		object[i]._oMissFlag = true;
	x = object[i].position.x;
	y = object[i].position.y;
	object[i]._oSelFlag = 2;
	if (object[i]._otype == OBJ_L2LDOOR && object[i]._oVar4 == 0) {
		ObjSetMicro(x, y, 538);
		dSpecial[x][y] = 0;
	} else if (object[i]._otype == OBJ_L2LDOOR && (object[i]._oVar4 == 1 || object[i]._oVar4 == 2)) {
		ObjSetMicro(x, y, 13);
		dSpecial[x][y] = 5;
	} else if (object[i]._otype == OBJ_L2RDOOR && object[i]._oVar4 == 0) {
		ObjSetMicro(x, y, 540);
		dSpecial[x][y] = 0;
	} else if (object[i]._otype == OBJ_L2RDOOR && (object[i]._oVar4 == 1 || object[i]._oVar4 == 2)) {
		ObjSetMicro(x, y, 17);
		dSpecial[x][y] = 6;
	}
}

void SyncL3Doors(int i)
{
	int x, y;

	object[i]._oMissFlag = true;
	x = object[i].position.x;
	y = object[i].position.y;
	object[i]._oSelFlag = 2;
	if (object[i]._otype == OBJ_L3LDOOR && object[i]._oVar4 == 0) {
		ObjSetMicro(x, y, 531);
	} else if (object[i]._otype == OBJ_L3LDOOR && (object[i]._oVar4 == 1 || object[i]._oVar4 == 2)) {
		ObjSetMicro(x, y, 538);
	} else if (object[i]._otype == OBJ_L3RDOOR && object[i]._oVar4 == 0) {
		ObjSetMicro(x, y, 534);
	} else if (object[i]._otype == OBJ_L3RDOOR && (object[i]._oVar4 == 1 || object[i]._oVar4 == 2)) {
		ObjSetMicro(x, y, 541);
	}
}

void SyncObjectAnim(int o)
{
	object_graphic_id index = AllObjects[object[o]._otype].ofindex;

	const auto &found = std::find(std::begin(ObjFileList), std::end(ObjFileList), index);
	if (found == std::end(ObjFileList)) {
		LogCritical("Unable to find object_graphic_id {} in list of objects to load, level generation error.", index);
		return;
	}

	const int i = std::distance(std::begin(ObjFileList), found);

	object[o]._oAnimData = pObjCels[i].get();
	switch (object[o]._otype) {
	case OBJ_L1LDOOR:
	case OBJ_L1RDOOR:
		SyncL1Doors(o);
		break;
	case OBJ_L2LDOOR:
	case OBJ_L2RDOOR:
		SyncL2Doors(o);
		break;
	case OBJ_L3LDOOR:
	case OBJ_L3RDOOR:
		SyncL3Doors(o);
		break;
	case OBJ_CRUX1:
	case OBJ_CRUX2:
	case OBJ_CRUX3:
		SyncCrux(o);
		break;
	case OBJ_LEVER:
	case OBJ_BOOK2L:
	case OBJ_SWITCHSKL:
		SyncLever(o);
		break;
	case OBJ_BOOK2R:
	case OBJ_BLINDBOOK:
	case OBJ_STEELTOME:
		SyncQSTLever(o);
		break;
	case OBJ_PEDISTAL:
		SyncPedistal(o);
		break;
	default:
		break;
	}
}

void GetObjectStr(int i)
{
	switch (object[i]._otype) {
	case OBJ_CRUX1:
	case OBJ_CRUX2:
	case OBJ_CRUX3:
		strcpy(infostr, _("Crucified Skeleton"));
		break;
	case OBJ_LEVER:
	case OBJ_FLAMELVR:
		strcpy(infostr, _("Lever"));
		break;
	case OBJ_L1LDOOR:
	case OBJ_L1RDOOR:
	case OBJ_L2LDOOR:
	case OBJ_L2RDOOR:
	case OBJ_L3LDOOR:
	case OBJ_L3RDOOR:
		if (object[i]._oVar4 == 1)
			strcpy(infostr, _("Open Door"));
		if (object[i]._oVar4 == 0)
			strcpy(infostr, _("Closed Door"));
		if (object[i]._oVar4 == 2)
			strcpy(infostr, _("Blocked Door"));
		break;
	case OBJ_BOOK2L:
		if (setlevel) {
			if (setlvlnum == SL_BONECHAMB) {
				strcpy(infostr, _("Ancient Tome"));
			} else if (setlvlnum == SL_VILEBETRAYER) {
				strcpy(infostr, _("Book of Vileness"));
			}
		}
		break;
	case OBJ_SWITCHSKL:
		strcpy(infostr, _("Skull Lever"));
		break;
	case OBJ_BOOK2R:
		strcpy(infostr, _("Mythical Book"));
		break;
	case OBJ_CHEST1:
	case OBJ_TCHEST1:
		strcpy(infostr, _("Small Chest"));
		break;
	case OBJ_CHEST2:
	case OBJ_TCHEST2:
		strcpy(infostr, _("Chest"));
		break;
	case OBJ_CHEST3:
	case OBJ_TCHEST3:
	case OBJ_SIGNCHEST:
		strcpy(infostr, _("Large Chest"));
		break;
	case OBJ_SARC:
		strcpy(infostr, _("Sarcophagus"));
		break;
	case OBJ_BOOKSHELF:
		strcpy(infostr, _("Bookshelf"));
		break;
	case OBJ_BOOKCASEL:
	case OBJ_BOOKCASER:
		strcpy(infostr, _("Bookcase"));
		break;
	case OBJ_BARREL:
	case OBJ_BARRELEX:
		if (currlevel >= 17 && currlevel <= 20)      // for hive levels
			strcpy(infostr, _("Pod"));               //Then a barrel is called a pod
		else if (currlevel >= 21 && currlevel <= 24) // for crypt levels
			strcpy(infostr, _("Urn"));               //Then a barrel is called an urn
		else
			strcpy(infostr, _("Barrel"));
		break;
	case OBJ_SHRINEL:
	case OBJ_SHRINER:
		sprintf(tempstr, _("%s Shrine"), _(shrinestrs[object[i]._oVar1]));
		strcpy(infostr, tempstr);
		break;
	case OBJ_SKELBOOK:
		strcpy(infostr, _("Skeleton Tome"));
		break;
	case OBJ_BOOKSTAND:
		strcpy(infostr, _("Library Book"));
		break;
	case OBJ_BLOODFTN:
		strcpy(infostr, _("Blood Fountain"));
		break;
	case OBJ_DECAP:
		strcpy(infostr, _("Decapitated Body"));
		break;
	case OBJ_BLINDBOOK:
		strcpy(infostr, _("Book of the Blind"));
		break;
	case OBJ_BLOODBOOK:
		strcpy(infostr, _("Book of Blood"));
		break;
	case OBJ_PURIFYINGFTN:
		strcpy(infostr, _("Purifying Spring"));
		break;
	case OBJ_ARMORSTAND:
	case OBJ_WARARMOR:
		strcpy(infostr, _("Armor"));
		break;
	case OBJ_WARWEAP:
		strcpy(infostr, _("Weapon Rack"));
		break;
	case OBJ_GOATSHRINE:
		strcpy(infostr, _("Goat Shrine"));
		break;
	case OBJ_CAULDRON:
		strcpy(infostr, _("Cauldron"));
		break;
	case OBJ_MURKYFTN:
		strcpy(infostr, _("Murky Pool"));
		break;
	case OBJ_TEARFTN:
		strcpy(infostr, _("Fountain of Tears"));
		break;
	case OBJ_STEELTOME:
		strcpy(infostr, _("Steel Tome"));
		break;
	case OBJ_PEDISTAL:
		strcpy(infostr, _("Pedestal of Blood"));
		break;
	case OBJ_STORYBOOK:
		strcpy(infostr, _(StoryBookName[object[i]._oVar3]));
		break;
	case OBJ_WEAPONRACK:
		strcpy(infostr, _("Weapon Rack"));
		break;
	case OBJ_MUSHPATCH:
		strcpy(infostr, _("Mushroom Patch"));
		break;
	case OBJ_LAZSTAND:
		strcpy(infostr, _("Vile Stand"));
		break;
	case OBJ_SLAINHERO:
		strcpy(infostr, _("Slain Hero"));
		break;
	default:
		break;
	}
	if (plr[myplr]._pClass == HeroClass::Rogue) {
		if (object[i]._oTrapFlag) {
			sprintf(tempstr, _("Trapped %s"), infostr);
			strcpy(infostr, tempstr);
			infoclr = COL_RED;
		}
	}
	if (objectIsDisabled(i)) {
		sprintf(tempstr, _("%s (disabled)"), infostr);
		strcpy(infostr, tempstr);
		infoclr = COL_RED;
	}
}

void operate_lv24_lever()
{
	if (currlevel == 24) {
		PlaySfxLoc(IS_CROPEN, UberRow, UberCol);
		//the part below is the same as objects_454BA8
		dPiece[UberRow][UberCol] = 298;
		dPiece[UberRow][UberCol - 1] = 301;
		dPiece[UberRow][UberCol - 2] = 300;
		dPiece[UberRow][UberCol + 1] = 299;
		SetDungeonMicros();
	}
}

void objects_454BA8()
{
	dPiece[UberRow][UberCol] = 298;
	dPiece[UberRow][UberCol - 1] = 301;
	dPiece[UberRow][UberCol - 2] = 300;
	dPiece[UberRow][UberCol + 1] = 299;

	SetDungeonMicros();
}

void objects_rnd_454BEA()
{
	int xp, yp;

	while (true) {
		xp = GenerateRnd(80) + 16;
		yp = GenerateRnd(80) + 16;
		if (RndLocOk(xp - 1, yp - 1)
		    && RndLocOk(xp, yp - 1)
		    && RndLocOk(xp + 1, yp - 1)
		    && RndLocOk(xp - 1, yp)
		    && RndLocOk(xp, yp)
		    && RndLocOk(xp + 1, yp)
		    && RndLocOk(xp - 1, yp + 1)
		    && RndLocOk(xp, yp + 1)
		    && RndLocOk(xp + 1, yp + 1)) {
			break;
		}
	}
	UberLeverRow = UberRow + 3;
	UberLeverCol = UberCol - 1;
	AddObject(OBJ_LEVER, UberRow + 3, UberCol - 1);
}

bool objects_lv_24_454B04(int s)
{
	switch (s) {
	case 6:
		dword_6DE0E0 = 1;
		break;
	case 7:
		if (dword_6DE0E0 == 1) {
			dword_6DE0E0 = 2;
		} else {
			dword_6DE0E0 = 0;
		}
		break;
	case 8:
		if (dword_6DE0E0 == 2)
			return true;
		dword_6DE0E0 = 0;
		break;
	}
	return false;
}

} // namespace devilution
