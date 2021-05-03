/**
 * @file monster.h
 *
 * Interface of monster functionality, AI, actions, spawning, loading, etc.
 */
#pragma once

#include <cstdint>
#include <array>

#include "engine.h"
#include "miniwin/miniwin.h"
#include "monstdat.h"

#ifndef NOSOUND
#include "sound.h"
#endif

namespace devilution {

#define MAXMONSTERS 200
#define MAX_LVLMTYPES 24

enum monster_flag : uint16_t {
	// clang-format off
	MFLAG_HIDDEN          = 1 << 0,
	MFLAG_LOCK_ANIMATION  = 1 << 1,
	MFLAG_ALLOW_SPECIAL   = 1 << 2,
	MFLAG_NOHEAL          = 1 << 3,
	MFLAG_TARGETS_MONSTER = 1 << 4,
	MFLAG_GOLEM           = 1 << 5,
	MFLAG_QUEST_COMPLETE  = 1 << 6,
	MFLAG_KNOCKBACK       = 1 << 7,
	MFLAG_SEARCH          = 1 << 8,
	MFLAG_CAN_OPEN_DOOR   = 1 << 9,
	MFLAG_NO_ENEMY        = 1 << 10,
	MFLAG_BERSERK         = 1 << 11,
	MFLAG_NOLIFESTEAL     = 1 << 12,
	// clang-format on
};

/** this enum contains indexes from UniqMonst array for special unique monsters (usually quest related) */
enum : uint8_t {
	UMT_GARBUD,
	UMT_SKELKING,
	UMT_ZHAR,
	UMT_SNOTSPIL,
	UMT_LAZURUS,
	UMT_RED_VEX,
	UMT_BLACKJADE,
	UMT_LACHDAN,
	UMT_WARLORD,
	UMT_BUTCHER,
	UMT_HORKDMN,
	UMT_DEFILER,
	UMT_NAKRUL,
};

enum MON_MODE : uint8_t {
	MM_STAND,
	/** Movement towards N, NW, or NE */
	MM_WALK,
	/** Movement towards S, SW, or SE */
	MM_WALK2,
	/** Movement towards W or E */
	MM_WALK3,
	MM_ATTACK,
	MM_GOTHIT,
	MM_DEATH,
	MM_SATTACK,
	MM_FADEIN,
	MM_FADEOUT,
	MM_RATTACK,
	MM_SPSTAND,
	MM_RSPATTACK,
	MM_DELAY,
	MM_CHARGE,
	MM_STONE,
	MM_HEAL,
	MM_TALK,
};

enum {
	MA_STAND,
	MA_WALK,
	MA_ATTACK,
	MA_GOTHIT,
	MA_DEATH,
	MA_SPECIAL,
};

enum monster_goal : uint8_t {
	MGOAL_NONE,
	MGOAL_NORMAL,
	MGOAL_RETREAT,
	MGOAL_HEALING,
	MGOAL_MOVE,
	MGOAL_ATTACK2,
	MGOAL_INQUIRING,
	MGOAL_TALKING,
};

enum placeflag : uint8_t {
	// clang-format off
	PLACE_SCATTER = 1 << 0,
	PLACE_SPECIAL = 1 << 1,
	PLACE_UNIQUE  = 1 << 2,
	// clang-format on
};

struct AnimStruct {
	std::unique_ptr<uint8_t[]> CMem;
	std::array<uint8_t *, 8> Data;
	int Frames;
	int Rate;
};

struct CMonster {
	_monster_id mtype;
	/** placeflag enum as a flags*/
	uint8_t mPlaceFlags;
	AnimStruct Anims[6];
#ifndef NOSOUND
	std::unique_ptr<TSnd> Snds[4][2];
#endif
	int width;
	uint16_t mMinHP;
	uint16_t mMaxHP;
	bool has_special;
	uint8_t mAFNum;
	int8_t mdeadval;
	const MonsterData *MData;
	/**
	 * A TRN file contains a sequence of color transitions, represented
	 * as indexes into a palette. (a 256 byte array of palette indices)
	 */
	uint8_t *trans_file;
};

struct MonsterStruct { // note: missing field _mAFNum
	int _mMTidx;
	MON_MODE _mmode;
	monster_goal _mgoal;
	int _mgoalvar1;
	int _mgoalvar2;
	int _mgoalvar3;
	uint8_t _pathcount;
	ActorPosition position;
	/** Direction faced by monster (direction enum) */
	direction _mdir;
	/** The current target of the mosnter. An index in to either the plr or monster array based on the _meflag value. */
	int _menemy;
	/** Usually correspond's to the enemy's future position */
	Point enemyPosition;
	uint8_t *_mAnimData;
	/** Tick length of each frame in the current animation */
	int _mAnimDelay;
	/** Increases by one each game tick, counting how close we are to _pAnimDelay */
	int _mAnimCnt;
	/** Number of frames in current animation */
	int _mAnimLen;
	/** Current frame of animation. */
	int _mAnimFrame;
	bool _mDelFlag;
	int _mVar1;
	int _mVar2;
	int _mVar3;
	/** Value used to measure progress for moving from one tile to another */
	int actionFrame;
	int _mmaxhp;
	int _mhitpoints;
	_mai_id _mAi;
	uint8_t _mint;
	uint32_t _mFlags;
	uint8_t _msquelch;
	int _mRndSeed;
	int _mAISeed;
	uint8_t _uniqtype;
	uint8_t _uniqtrans;
	int8_t _udeadval;
	int8_t mWhoHit;
	int8_t mLevel;
	uint16_t mExp;
	uint16_t mHit;
	uint8_t mMinDamage;
	uint8_t mMaxDamage;
	uint16_t mHit2;
	uint8_t mMinDamage2;
	uint8_t mMaxDamage2;
	uint8_t mArmorClass;
	uint16_t mMagicRes;
	int mtalkmsg;
	uint8_t leader;
	uint8_t leaderflag;
	uint8_t packsize;
	int8_t mlid; // BUGFIX -1 is used when not emitting light this should be signed (fixed)
	const char *mName;
	CMonster *MType;
	const MonsterData *MData;
};

extern int monstkills[MAXMONSTERS];
extern int monstactive[MAXMONSTERS];
extern int nummonsters;
extern bool sgbSaveSoundOn;
extern MonsterStruct monster[MAXMONSTERS];
extern CMonster Monsters[MAX_LVLMTYPES];
extern int nummtypes;

void InitLevelMonsters();
void GetLevelMTypes();
void InitMonsterGFX(int monst);
void InitMonster(int i, direction rd, int mtype, int x, int y);
void ClrAllMonsters();
void monster_some_crypt();
void PlaceGroup(int mtype, int num, int leaderf, int leader);
void InitMonsters();
void SetMapMonsters(const uint16_t *dunData, int startx, int starty);
void DeleteMonster(int i);
int AddMonster(int x, int y, direction dir, int mtype, bool InMap);
void monster_43C785(int i);
bool M_Talker(int i);
void M_StartStand(int i, direction md);
void M_ClearSquares(int i);
void M_GetKnockback(int i);
void M_StartHit(int i, int pnum, int dam);
void M_StartKill(int i, int pnum);
void M_SyncStartKill(int i, int x, int y, int pnum);
void M_Teleport(int i);
void M_UpdateLeader(int i);
void DoEnding();
void PrepDoEnding();
void M_WalkDir(int i, direction md);
void MAI_Zombie(int i);
void MAI_SkelSd(int i);
void MAI_Snake(int i);
void MAI_Bat(int i);
void MAI_SkelBow(int i);
void MAI_Fat(int i);
void MAI_Sneak(int i);
void MAI_Fireman(int i);
void MAI_Fallen(int i);
void MAI_Cleaver(int i);
void MAI_Round(int i, bool special);
void MAI_GoatMc(int i);
void MAI_Ranged(int i, int missile_type, bool special);
void MAI_GoatBow(int i);
void MAI_Succ(int i);
void MAI_Lich(int i);
void MAI_ArchLich(int i);
void MAI_Psychorb(int i);
void MAI_Necromorb(int i);
void MAI_AcidUniq(int i);
void MAI_Firebat(int i);
void MAI_Torchant(int i);
void MAI_Scav(int i);
void MAI_Garg(int i);
void MAI_RoundRanged(int i, int missile_type, bool checkdoors, int dam, int lessmissiles);
void MAI_Magma(int i);
void MAI_Storm(int i);
void MAI_BoneDemon(int i);
void MAI_Acid(int i);
void MAI_Diablo(int i);
void MAI_Mega(int i);
void MAI_Golum(int i);
void MAI_SkelKing(int i);
void MAI_Rhino(int i);
void MAI_HorkDemon(int i);
void MAI_Counselor(int i);
void MAI_Garbud(int i);
void MAI_Zhar(int i);
void MAI_SnotSpil(int i);
void MAI_Lazurus(int i);
void MAI_Lazhelp(int i);
void MAI_Lachdanan(int i);
void MAI_Warlord(int i);
void DeleteMonsterList();
void ProcessMonsters();
void FreeMonsters();
bool DirOK(int i, direction mdir);
bool PosOkMissile(int entity, int x, int y);
bool LineClearSolid(int x1, int y1, int x2, int y2);
bool LineClearMissile(int x1, int y1, int x2, int y2);
bool LineClear(int x1, int y1, int x2, int y2);
bool LineClear(bool (*Clear)(int, int, int), int entity, int x1, int y1, int x2, int y2);
void SyncMonsterAnim(int i);
void M_FallenFear(int x, int y);
void PrintMonstHistory(int mt);
void PrintUniqueHistory();
void MissToMonst(int i, int x, int y);
bool PosOkMonst(int i, int x, int y);
bool monster_posok(int i, int x, int y);
bool PosOkMonst2(int i, int x, int y);
bool PosOkMonst3(int i, int x, int y);
bool IsSkel(int mt);
bool IsGoat(int mt);
int M_SpawnSkel(int x, int y, direction dir);
bool SpawnSkeleton(int ii, int x, int y);
int PreSpawnSkeleton();
void TalktoMonster(int i);
void SpawnGolum(int i, int x, int y, int mi);
bool CanTalkToMonst(int m);
bool CheckMonsterHit(int m, bool *ret);
int encode_enemy(int m);
void decode_enemy(int m, int enemy);

/* data */

extern direction left[8];
extern direction right[8];
extern direction opposite[8];
extern int offset_x[8];
extern int offset_y[8];

} // namespace devilution
