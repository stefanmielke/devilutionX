#include "controls/plrctrls.h"

#include <algorithm>
#include <cstdint>
#include <list>

#include "automap.h"
#include "control.h"
#include "controls/controller.h"
#include "controls/controller_motion.h"
#include "controls/game_controls.h"
#include "cursor.h"
#include "gmenu.h"
#include "help.h"
#include "inv.h"
#include "minitext.h"
#include "missiles.h"
#include "stores.h"
#include "towners.h"
#include "trigs.h"

#define SPLICONLENGTH 56

namespace devilution {

bool sgbControllerActive = false;
coords speedspellscoords[50];
int speedspellcount = 0;

static const direction FaceDir[3][3] = {
	// NONE      UP      DOWN
	{ DIR_OMNI, DIR_N, DIR_S }, // NONE
	{ DIR_W, DIR_NW, DIR_SW },  // LEFT
	{ DIR_E, DIR_NE, DIR_SE },  // RIGHT
};
static const int Offsets[8][2] = {
	{ 1, 1 },   // DIR_S
	{ 0, 1 },   // DIR_SW
	{ -1, 1 },  // DIR_W
	{ -1, 0 },  // DIR_NW
	{ -1, -1 }, // DIR_N
	{ 0, -1 },  // DIR_NE
	{ 1, -1 },  // DIR_E
	{ 1, 0 },   // DIR_SE
};

/**
 * Native game menu, controlled by simulating a keyboard.
 */
bool InGameMenu()
{
	return stextflag != STORE_NONE
	    || helpflag
	    || talkflag
	    || qtextflag
	    || gmenu_is_active()
	    || PauseMode == 2
	    || plr[myplr]._pInvincible;
}

namespace {

int slot = SLOTXY_INV_FIRST;

/**
 * Number of angles to turn to face the coordinate
 * @param x Tile coordinates
 * @param y Tile coordinates
 * @return -1 == down
 */
int GetRotaryDistance(int x, int y)
{
	int d, d1, d2;

	if (plr[myplr].position.future.x == x && plr[myplr].position.future.y == y)
		return -1;

	d1 = plr[myplr]._pdir;
	d2 = GetDirection(plr[myplr].position.future, { x, y });

	d = abs(d1 - d2);
	if (d > 4)
		return 4 - (d % 4);

	return d;
}

/**
 * @brief Get the best case walking steps to coordinates
 * @param dx Tile coordinates
 * @param dy Tile coordinates
 */
int GetMinDistance(int dx, int dy)
{
	return std::max(abs(plr[myplr].position.future.x - dx), abs(plr[myplr].position.future.y - dy));
}

/**
 * @brief Get walking steps to coordinate
 * @param dx Tile coordinates
 * @param dy Tile coordinates
 * @param maxDistance the max number of steps to search
 * @return number of steps, or 0 if not reachable
 */
int GetDistance(int dx, int dy, int maxDistance)
{
	if (GetMinDistance(dx, dy) > maxDistance) {
		return 0;
	}

	int8_t walkpath[MAX_PATH_LENGTH];
	int steps = FindPath(PosOkPlayer, myplr, plr[myplr].position.future.x, plr[myplr].position.future.y, dx, dy, walkpath);
	if (steps > maxDistance)
		return 0;

	return steps;
}

/**
 * @brief Get distance to coordinate
 * @param dx Tile coordinates
 * @param dy Tile coordinates
 */
int GetDistanceRanged(int dx, int dy)
{
	int a = plr[myplr].position.future.x - dx;
	int b = plr[myplr].position.future.y - dy;

	return sqrt(a * a + b * b);
}

void FindItemOrObject()
{
	int mx = plr[myplr].position.future.x;
	int my = plr[myplr].position.future.y;
	int rotations = 5;

	// As the player can not stand on the edge fo the map this is safe from OOB
	for (int xx = -1; xx < 2; xx++) {
		for (int yy = -1; yy < 2; yy++) {
			if (dItem[mx + xx][my + yy] <= 0)
				continue;
			int i = dItem[mx + xx][my + yy] - 1;
			if (items[i].isEmpty()
			    || items[i]._iSelFlag == 0)
				continue;
			int newRotations = GetRotaryDistance(mx + xx, my + yy);
			if (rotations < newRotations)
				continue;
			if (xx != 0 && yy != 0 && GetDistance(mx + xx, my + yy, 1) == 0)
				continue;
			rotations = newRotations;
			pcursitem = i;
			cursmx = mx + xx;
			cursmy = my + yy;
		}
	}

	if (leveltype == DTYPE_TOWN || pcursitem != -1)
		return; // Don't look for objects in town

	for (int xx = -1; xx < 2; xx++) {
		for (int yy = -1; yy < 2; yy++) {
			if (dObject[mx + xx][my + yy] == 0)
				continue;
			int o = dObject[mx + xx][my + yy] > 0 ? dObject[mx + xx][my + yy] - 1 : -(dObject[mx + xx][my + yy] + 1);
			if (object[o]._oSelFlag == 0)
				continue;
			if (xx == 0 && yy == 0 && object[o]._oDoorFlag)
				continue; // Ignore doorway so we don't get stuck behind barrels
			int newRotations = GetRotaryDistance(mx + xx, my + yy);
			if (rotations < newRotations)
				continue;
			if (xx != 0 && yy != 0 && GetDistance(mx + xx, my + yy, 1) == 0)
				continue;
			rotations = newRotations;
			pcursobj = o;
			cursmx = mx + xx;
			cursmy = my + yy;
		}
	}
}

void CheckTownersNearby()
{
	for (int i = 0; i < 16; i++) {
		int distance = GetDistance(towners[i].position.x, towners[i].position.y, 2);
		if (distance == 0)
			continue;
		pcursmonst = i;
	}
}

bool HasRangedSpell()
{
	int spl = plr[myplr]._pRSpell;

	return spl != SPL_INVALID
	    && spl != SPL_TOWN
	    && spl != SPL_TELEPORT
	    && spelldata[spl].sTargeted
	    && !spelldata[spl].sTownSpell;
}

bool CanTargetMonster(int mi)
{
	const MonsterStruct &monst = monster[mi];

	if ((monst._mFlags & (MFLAG_HIDDEN | MFLAG_GOLEM)) != 0)
		return false;
	if (monst._mhitpoints >> 6 <= 0) // dead
		return false;

	const int mx = monst.position.tile.x;
	const int my = monst.position.tile.y;
	if ((dFlags[mx][my] & BFLAG_LIT) == 0) // not visible
		return false;
	if (dMonster[mx][my] == 0)
		return false;

	return true;
}

void FindRangedTarget()
{
	int rotations = 0;
	int distance = 0;
	bool canTalk = false;

	// The first MAX_PLRS monsters are reserved for players' golems.
	for (int mi = MAX_PLRS; mi < MAXMONSTERS; mi++) {
		const MonsterStruct &monst = monster[mi];
		const int mx = monst.position.future.x;
		const int my = monst.position.future.y;
		if (!CanTargetMonster(mi))
			continue;

		const bool newCanTalk = CanTalkToMonst(mi);
		if (pcursmonst != -1 && !canTalk && newCanTalk)
			continue;
		const int newDdistance = GetDistanceRanged(mx, my);
		const int newRotations = GetRotaryDistance(mx, my);
		if (pcursmonst != -1 && canTalk == newCanTalk) {
			if (distance < newDdistance)
				continue;
			if (distance == newDdistance && rotations < newRotations)
				continue;
		}
		distance = newDdistance;
		rotations = newRotations;
		canTalk = newCanTalk;
		pcursmonst = mi;
	}
}

void FindMeleeTarget()
{
	bool visited[MAXDUNX][MAXDUNY] = { {} };
	int maxSteps = 25; // Max steps for FindPath is 25
	int rotations = 0;
	bool canTalk = false;

	struct SearchNode {
		int x, y;
		int steps;
	};
	std::list<SearchNode> queue;

	{
		const int startX = plr[myplr].position.future.x;
		const int startY = plr[myplr].position.future.y;
		visited[startX][startY] = true;
		queue.push_back({ startX, startY, 0 });
	}

	while (!queue.empty()) {
		SearchNode node = queue.front();
		queue.pop_front();

		for (int i = 0; i < 8; i++) {
			const int dx = node.x + pathxdir[i];
			const int dy = node.y + pathydir[i];

			if (visited[dx][dy])
				continue; // already visisted

			if (node.steps > maxSteps) {
				visited[dx][dy] = true;
				continue;
			}

			if (!PosOkPlayer(myplr, dx, dy)) {
				visited[dx][dy] = true;

				if (dMonster[dx][dy] != 0) {
					const int mi = dMonster[dx][dy] > 0 ? dMonster[dx][dy] - 1 : -(dMonster[dx][dy] + 1);
					if (CanTargetMonster(mi)) {
						const bool newCanTalk = CanTalkToMonst(mi);
						if (pcursmonst != -1 && !canTalk && newCanTalk)
							continue;
						const int newRotations = GetRotaryDistance(dx, dy);
						if (pcursmonst != -1 && canTalk == newCanTalk && rotations < newRotations)
							continue;
						rotations = newRotations;
						canTalk = newCanTalk;
						pcursmonst = mi;
						if (!canTalk)
							maxSteps = node.steps; // Monsters found, cap search to current steps
					}
				}

				continue;
			}

			PATHNODE pPath;
			pPath.position = { node.x, node.y };

			if (path_solid_pieces(&pPath, dx, dy)) {
				queue.push_back({ dx, dy, node.steps + 1 });
				visited[dx][dy] = true;
			}
		}
	}
}

void CheckMonstersNearby()
{
	if (plr[myplr]._pwtype == WT_RANGED || HasRangedSpell()) {
		FindRangedTarget();
		return;
	}

	FindMeleeTarget();
}

void CheckPlayerNearby()
{
	int newDdistance;
	int rotations = 0;
	int distance = 0;

	if (pcursmonst != -1)
		return;

	int spl = plr[myplr]._pRSpell;
	if (gbFriendlyMode && spl != SPL_RESURRECT && spl != SPL_HEALOTHER)
		return;

	for (int i = 0; i < MAX_PLRS; i++) {
		if (i == myplr)
			continue;
		const int mx = plr[i].position.future.x;
		const int my = plr[i].position.future.y;
		if (dPlayer[mx][my] == 0
		    || (dFlags[mx][my] & BFLAG_LIT) == 0
		    || (plr[i]._pHitPoints == 0 && spl != SPL_RESURRECT))
			continue;

		if (plr[myplr]._pwtype == WT_RANGED || HasRangedSpell() || spl == SPL_HEALOTHER) {
			newDdistance = GetDistanceRanged(mx, my);
		} else {
			newDdistance = GetDistance(mx, my, distance);
			if (newDdistance == 0)
				continue;
		}

		if (pcursplr != -1 && distance < newDdistance)
			continue;
		const int newRotations = GetRotaryDistance(mx, my);
		if (pcursplr != -1 && distance == newDdistance && rotations < newRotations)
			continue;

		distance = newDdistance;
		rotations = newRotations;
		pcursplr = i;
	}
}

void FindActor()
{
	if (leveltype != DTYPE_TOWN)
		CheckMonstersNearby();
	else
		CheckTownersNearby();

	if (gbIsMultiplayer)
		CheckPlayerNearby();
}

int pcursmissile;
int pcurstrig;
int pcursquest;

void FindTrigger()
{
	int rotations;
	int distance = 0;

	if (pcursitem != -1 || pcursobj != -1)
		return; // Prefer showing items/objects over triggers (use of cursm* conflicts)

	for (int i = 0; i < nummissiles; i++) {
		int mi = missileactive[i];
		if (missile[mi]._mitype == MIS_TOWN || missile[mi]._mitype == MIS_RPORTAL) {
			int mix = missile[mi].position.tile.x;
			int miy = missile[mi].position.tile.y;
			const int newDdistance = GetDistance(mix, miy, 2);
			if (newDdistance == 0)
				continue;
			if (pcursmissile != -1 && distance < newDdistance)
				continue;
			const int newRotations = GetRotaryDistance(mix, miy);
			if (pcursmissile != -1 && distance == newDdistance && rotations < newRotations)
				continue;
			cursmx = mix;
			cursmy = miy;
			pcursmissile = mi;
			distance = newDdistance;
			rotations = newRotations;
		}
	}

	if (pcursmissile == -1) {
		for (int i = 0; i < numtrigs; i++) {
			int tx = trigs[i].position.x;
			int ty = trigs[i].position.y;
			if (trigs[i]._tlvl == 13)
				ty -= 1;
			const int newDdistance = GetDistance(tx, ty, 2);
			if (newDdistance == 0)
				continue;
			cursmx = tx;
			cursmy = ty;
			pcurstrig = i;
		}

		if (pcurstrig == -1) {
			for (int i = 0; i < MAXQUESTS; i++) {
				if (i == Q_BETRAYER || currlevel != quests[i]._qlevel || quests[i]._qslvl == 0)
					continue;
				const int newDdistance = GetDistance(quests[i].position.x, quests[i].position.y, 2);
				if (newDdistance == 0)
					continue;
				cursmx = quests[i].position.x;
				cursmy = quests[i].position.y;
				pcursquest = i;
			}
		}
	}

	if (pcursmonst != -1 || pcursplr != -1 || cursmx == -1 || cursmy == -1)
		return; // Prefer monster/player info text

	CheckTrigForce();
	CheckTown();
	CheckRportal();
}

void Interact()
{
	if (leveltype == DTYPE_TOWN && pcursmonst != -1) {
		NetSendCmdLocParam1(true, CMD_TALKXY, towners[pcursmonst].position, pcursmonst);
	} else if (pcursmonst != -1) {
		if (plr[myplr]._pwtype != WT_RANGED || CanTalkToMonst(pcursmonst)) {
			int distx = abs(plr[myplr].position.tile.x - monster[pcursmonst].position.future.x);
			int disty = abs(plr[myplr].position.tile.y - monster[pcursmonst].position.future.y);
			if (distx > 1 || disty > 1) {
				// attack on the current direction
				const int x = plr[myplr].position.future.x;
				const int y = plr[myplr].position.future.y;
				const int dx = x + Offsets[plr[myplr]._pdir][0];
				const int dy = y + Offsets[plr[myplr]._pdir][1];
				
				NetSendCmdLoc(myplr, true, CMD_SATTACKXY, { dx, dy });
			} else {
				NetSendCmdParam1(true, CMD_ATTACKID, pcursmonst);
			}
		} else {
			NetSendCmdParam1(true, CMD_RATTACKID, pcursmonst);
		}
	} else if (leveltype != DTYPE_TOWN) {
		if (pcursplr != -1 && !gbFriendlyMode) {
			NetSendCmdParam1(true, plr[myplr]._pwtype == WT_RANGED ? CMD_RATTACKPID : CMD_ATTACKPID, pcursplr);
		} else {
			// attack on the current direction
			const int x = plr[myplr].position.future.x;
			const int y = plr[myplr].position.future.y;
			const int dx = x + Offsets[plr[myplr]._pdir][0];
			const int dy = y + Offsets[plr[myplr]._pdir][1];

			NetSendCmdLoc(myplr, true, plr[myplr]._pwtype == WT_RANGED ? CMD_RATTACKXY : CMD_SATTACKXY, { dx, dy });
		}
	}
}

void AttrIncBtnSnap(AxisDirection dir)
{
	static AxisDirectionRepeater repeater;
	dir = repeater.Get(dir);
	if (dir.y == AxisDirectionY_NONE)
		return;

	if (chrbtnactive && plr[myplr]._pStatPts <= 0)
		return;

	// first, find our cursor location
	int slot = 0;
	for (int i = 0; i < 4; i++) {
		if (MouseX >= ChrBtnsRect[i].x
		    && MouseX <= ChrBtnsRect[i].x + ChrBtnsRect[i].w
		    && MouseY >= ChrBtnsRect[i].y
		    && MouseY <= ChrBtnsRect[i].h + ChrBtnsRect[i].y) {
			slot = i;
			break;
		}
	}

	if (dir.y == AxisDirectionY_UP) {
		if (slot > 0)
			--slot;
	} else if (dir.y == AxisDirectionY_DOWN) {
		if (slot < 3)
			++slot;
	}

	// move cursor to our new location
	int x = ChrBtnsRect[slot].x + (ChrBtnsRect[slot].w / 2);
	int y = ChrBtnsRect[slot].y + (ChrBtnsRect[slot].h / 2);
	SetCursorPos(x, y);
}

coords InvGetInvSlotCoord(const inv_body_loc inv_slot, bool center = true)
{
	coords result { RIGHT_PANEL, 0 };
	result.x -= (icursW28 - 1) * (INV_SLOT_SIZE_PX / 2);
	switch (inv_slot)
	{
	case INVLOC_HEAD:
		result.x += ((InvRect[SLOTXY_HEAD_FIRST].X + InvRect[SLOTXY_HEAD_LAST].X) / 2);
		result.y += ((InvRect[SLOTXY_HEAD_FIRST].Y + InvRect[SLOTXY_HEAD_LAST].Y) / 2);
		break;
	case INVLOC_RING_LEFT:
		result.x += InvRect[SLOTXY_RING_LEFT].X;
		result.y += InvRect[SLOTXY_RING_LEFT].Y;
		break;
	case INVLOC_RING_RIGHT:
		result.x += InvRect[SLOTXY_RING_RIGHT].X;
		result.y += InvRect[SLOTXY_RING_RIGHT].Y;
		break;
	case INVLOC_AMULET:
		result.x += InvRect[SLOTXY_AMULET].X;
		result.y += InvRect[SLOTXY_AMULET].Y;
		break;
	case INVLOC_HAND_LEFT:
		result.x += ((InvRect[SLOTXY_HAND_LEFT_FIRST].X + InvRect[SLOTXY_HAND_LEFT_LAST].X) / 2);
		result.y += ((InvRect[SLOTXY_HAND_LEFT_FIRST].Y + InvRect[SLOTXY_HAND_LEFT_LAST].Y) / 2);
		break;
	case INVLOC_HAND_RIGHT:
		result.x += ((InvRect[SLOTXY_HAND_RIGHT_FIRST].X + InvRect[SLOTXY_HAND_RIGHT_LAST].X) / 2);
		result.y += ((InvRect[SLOTXY_HAND_RIGHT_FIRST].Y + InvRect[SLOTXY_HAND_RIGHT_LAST].Y) / 2);
		break;
	case INVLOC_CHEST:
		result.x += ((InvRect[SLOTXY_CHEST_FIRST].X + InvRect[SLOTXY_CHEST_LAST].X) / 2);
		result.y += ((InvRect[SLOTXY_CHEST_FIRST].Y + InvRect[SLOTXY_CHEST_LAST].Y) / 2);
		break;
	default:
		break;
	}

	return result;
}

/**
 * Get coordinates for a given inventory slot (for belt use BeltGetSlotCoord)
 */
coords InvGetSlotCoord(int slot)
{
    assert(slot <= SLOTXY_INV_LAST);
    return {InvRect[slot].X + RIGHT_PANEL, InvRect[slot].Y };
}

/**
 * Get coordinates for a given belt slot (for normal inventory use InvGetSlotCoord)
 */
coords BeltGetSlotCoord(int slot)
{
    assert(slot >= SLOTXY_BELT_FIRST && slot <= SLOTXY_BELT_LAST);
    return {InvRect[slot].X + PANEL_LEFT, InvRect[slot].Y + PANEL_TOP };
}

/**
 * Move the cursor around in our inventory
 * If mouse coords are at SLOTXY_CHEST_LAST, consider this center of equipment
 * small inventory squares are 29x29 (roughly)
 */
void InvMove(AxisDirection dir)
{
	static AxisDirectionRepeater repeater(/*min_interval_ms=*/150);
	dir = repeater.Get(dir);
	if (dir.x == AxisDirectionX_NONE && dir.y == AxisDirectionY_NONE)
		return;

	coords mousePos { MouseX, MouseY };

	const bool isHoldingItem = pcurs > 1;

	// normalize slots
	if (slot < 0)
		slot = 0;
	else if (slot >= SLOTXY_HEAD_FIRST && slot <= SLOTXY_HEAD_LAST)
		slot = SLOTXY_HEAD_FIRST;
	else if (slot >= SLOTXY_HAND_LEFT_FIRST && slot <= SLOTXY_HAND_LEFT_LAST)
		slot = SLOTXY_HAND_LEFT_FIRST;
	else if (slot >= SLOTXY_CHEST_FIRST && slot <= SLOTXY_CHEST_LAST)
		slot = SLOTXY_CHEST_FIRST;
	else if (slot >= SLOTXY_HAND_RIGHT_FIRST && slot <= SLOTXY_HAND_RIGHT_LAST)
		slot = SLOTXY_HAND_RIGHT_FIRST;
	else if (slot > SLOTXY_BELT_LAST)
		slot = SLOTXY_BELT_LAST;

	const int initialSlot = slot;

	// when item is on cursor (pcurs > 1), this is the real cursor XY
	if (dir.x == AxisDirectionX_LEFT) {
		if (isHoldingItem) {
			if (slot >= SLOTXY_INV_FIRST && slot <= SLOTXY_INV_LAST) {
                if (slot == SLOTXY_INV_ROW1_FIRST || slot == SLOTXY_INV_ROW2_FIRST || slot == SLOTXY_INV_ROW3_FIRST || slot == SLOTXY_INV_ROW4_FIRST) {
					slot += INV_ROW_SLOT_SIZE - icursW28;
				} else {
					slot -= 1;
				}
				mousePos = InvGetSlotCoord(slot);
			} else if (slot > SLOTXY_BELT_FIRST && slot <= SLOTXY_BELT_LAST) {
                slot -= 1;
                mousePos = BeltGetSlotCoord(slot);
            } else if (plr[myplr].HoldItem._itype == ITYPE_RING) {
                slot = SLOTXY_RING_LEFT;
				mousePos = InvGetInvSlotCoord(INVLOC_RING_LEFT);
			} else if (plr[myplr].HoldItem.isWeapon() || plr[myplr].HoldItem.isShield()) {
			    if (slot == SLOTXY_HAND_LEFT_FIRST) {
                    slot = SLOTXY_HAND_RIGHT_FIRST;
                    mousePos = InvGetInvSlotCoord(INVLOC_HAND_RIGHT);
                } else if (slot == SLOTXY_HAND_RIGHT_FIRST) {
                    slot = SLOTXY_HAND_LEFT_FIRST;
                    mousePos = InvGetInvSlotCoord(INVLOC_HAND_LEFT);
                }
            }
		} else {
			if (slot == SLOTXY_HAND_RIGHT_FIRST) {
                slot = SLOTXY_CHEST_FIRST;
				mousePos = InvGetInvSlotCoord(INVLOC_CHEST);
			} else if (slot == SLOTXY_CHEST_FIRST) {
                slot = SLOTXY_HAND_LEFT_FIRST;
				mousePos = InvGetInvSlotCoord(INVLOC_HAND_LEFT);
			} else if (slot == SLOTXY_AMULET) {
                slot = SLOTXY_HEAD_FIRST;
				mousePos = InvGetInvSlotCoord(INVLOC_HEAD);
			} else if (slot == SLOTXY_RING_RIGHT) {
                slot = SLOTXY_RING_LEFT;
				mousePos = InvGetInvSlotCoord(INVLOC_RING_LEFT);
			} else if (slot >= SLOTXY_INV_FIRST && slot <= SLOTXY_INV_LAST) {
				if (slot == SLOTXY_INV_ROW1_FIRST || slot == SLOTXY_INV_ROW2_FIRST || slot == SLOTXY_INV_ROW3_FIRST || slot == SLOTXY_INV_ROW4_FIRST) {
					slot += INV_ROW_SLOT_SIZE - 1;
				} else {
					slot -= 1;
				}
                mousePos = InvGetSlotCoord(slot);
			} else if (slot > SLOTXY_BELT_FIRST && slot <= SLOTXY_BELT_LAST) {
				slot -= 1;
                mousePos = BeltGetSlotCoord(slot);
			}
		}
	} else if (dir.x == AxisDirectionX_RIGHT) {
		if (isHoldingItem) {
			if (slot >= SLOTXY_INV_FIRST && slot <= SLOTXY_INV_LAST) {
				if (
                    slot == SLOTXY_INV_ROW1_LAST + 1 - icursW28 || slot == SLOTXY_INV_ROW2_LAST + 1 - icursW28 ||
				    slot == SLOTXY_INV_ROW3_LAST + 1 - icursW28 || slot == SLOTXY_INV_ROW4_LAST + 1 - icursW28
                ) {
					slot -= INV_ROW_SLOT_SIZE - icursW28;
				} else {
					slot += 1;
				}
                mousePos = InvGetSlotCoord(slot);
			} else if (slot >= SLOTXY_BELT_FIRST && slot < SLOTXY_BELT_LAST) {
                slot += 1;
                mousePos = BeltGetSlotCoord(slot);
            } else if (plr[myplr].HoldItem._itype == ITYPE_RING) {
                slot = SLOTXY_RING_RIGHT;
				mousePos = InvGetInvSlotCoord(INVLOC_RING_RIGHT);
			} else if (plr[myplr].HoldItem.isWeapon() || plr[myplr].HoldItem.isShield()) {
                if (slot == SLOTXY_HAND_LEFT_FIRST) {
                    slot = SLOTXY_HAND_RIGHT_FIRST;
                    mousePos = InvGetInvSlotCoord(INVLOC_HAND_RIGHT);
                } else if (slot == SLOTXY_HAND_RIGHT_FIRST) {
                    slot = SLOTXY_HAND_LEFT_FIRST;
                    mousePos = InvGetInvSlotCoord(INVLOC_HAND_LEFT);
                }
            }
		} else {
			if (slot == SLOTXY_RING_LEFT) {
                slot = SLOTXY_RING_RIGHT;
				mousePos = InvGetInvSlotCoord(INVLOC_RING_RIGHT);
			} else if (slot == SLOTXY_HAND_LEFT_FIRST) {
                slot = SLOTXY_CHEST_FIRST;
				mousePos = InvGetInvSlotCoord(INVLOC_CHEST);
			} else if (slot == SLOTXY_CHEST_FIRST) {
                slot = SLOTXY_HAND_RIGHT_FIRST;
				mousePos = InvGetInvSlotCoord(INVLOC_HAND_RIGHT);
			} else if (slot == SLOTXY_HEAD_FIRST) {
                slot = SLOTXY_AMULET;
				mousePos = InvGetInvSlotCoord(INVLOC_AMULET);
			} else if (slot >= SLOTXY_INV_FIRST && slot <= SLOTXY_INV_LAST) {
				if (slot == SLOTXY_INV_ROW1_LAST || slot == SLOTXY_INV_ROW2_LAST || slot == SLOTXY_INV_ROW3_LAST || slot == SLOTXY_INV_ROW4_LAST) {
					slot -= INV_ROW_SLOT_SIZE - 1;
				} else {
					slot += 1;
				}
                mousePos = InvGetSlotCoord(slot);
			} else if (slot >= SLOTXY_BELT_FIRST && slot < SLOTXY_BELT_LAST) {
				slot += 1;
                mousePos = BeltGetSlotCoord(slot);
			}
		}
	}
	if (dir.y == AxisDirectionY_UP) {
		if (isHoldingItem) {
			if (slot >= SLOTXY_INV_ROW2_FIRST) { // general inventory
				slot -= INV_ROW_SLOT_SIZE;
                mousePos = InvGetSlotCoord(slot);
			} else if (slot >= SLOTXY_INV_FIRST) {
				if (plr[myplr].HoldItem._itype == ITYPE_RING) {
					if (slot >= SLOTXY_INV_ROW1_FIRST && slot <= SLOTXY_INV_ROW1_FIRST + (INV_ROW_SLOT_SIZE / 2) - 1) {
                        slot = SLOTXY_RING_LEFT;
                        mousePos = InvGetInvSlotCoord(INVLOC_RING_LEFT);
                    } else {
                        slot = SLOTXY_RING_RIGHT;
                        mousePos = InvGetInvSlotCoord(INVLOC_RING_RIGHT);
                    }
				} else if (plr[myplr].HoldItem.isWeapon()) {
                    slot = SLOTXY_HAND_LEFT_FIRST;
					mousePos = InvGetInvSlotCoord(INVLOC_HAND_LEFT);
				} else if (plr[myplr].HoldItem.isShield()) {
                    slot = SLOTXY_HAND_RIGHT_FIRST;
					mousePos = InvGetInvSlotCoord(INVLOC_HAND_RIGHT);
				} else if (plr[myplr].HoldItem.isHelm()) {
                    slot = SLOTXY_HEAD_FIRST;
					mousePos = InvGetInvSlotCoord(INVLOC_HEAD);
				} else if (plr[myplr].HoldItem.isArmor()) {
                    slot = SLOTXY_CHEST_FIRST;
					mousePos = InvGetInvSlotCoord(INVLOC_CHEST);
				} else if (plr[myplr].HoldItem._itype == ITYPE_AMULET) {
                    slot = SLOTXY_AMULET;
					mousePos = InvGetInvSlotCoord(INVLOC_AMULET);
				}
			}
		} else {
			if (slot >= SLOTXY_INV_ROW1_FIRST && slot < SLOTXY_INV_ROW1_FIRST + 3) { // first 3 general slots
                slot = SLOTXY_RING_LEFT;
				mousePos = InvGetInvSlotCoord(INVLOC_RING_LEFT);
			} else if (slot >= SLOTXY_INV_ROW1_FIRST + 3 && slot < SLOTXY_INV_ROW1_FIRST + 7) { // middle 4 general slots
                slot = SLOTXY_CHEST_FIRST;
				mousePos = InvGetInvSlotCoord(INVLOC_CHEST);
			} else if (slot >= SLOTXY_INV_ROW1_FIRST + 7 && slot < SLOTXY_INV_ROW1_LAST) { // last 3 general slots
                slot = SLOTXY_RING_RIGHT;
				mousePos = InvGetInvSlotCoord(INVLOC_RING_RIGHT);
			} else if (slot == SLOTXY_CHEST_FIRST || slot == SLOTXY_HAND_LEFT_FIRST) {
                slot = SLOTXY_HEAD_FIRST;
				mousePos = InvGetInvSlotCoord(INVLOC_HEAD);
			} else if (slot == SLOTXY_RING_LEFT) {
                slot = SLOTXY_HAND_LEFT_FIRST;
				mousePos = InvGetInvSlotCoord(INVLOC_HAND_LEFT);
			} else if (slot == SLOTXY_RING_RIGHT) {
                slot = SLOTXY_HAND_RIGHT_FIRST;
				mousePos = InvGetInvSlotCoord(INVLOC_HAND_RIGHT);
			} else if (slot == SLOTXY_HAND_RIGHT_FIRST) {
                slot = SLOTXY_AMULET;
				mousePos = InvGetInvSlotCoord(INVLOC_AMULET);
			} else if (slot >= SLOTXY_INV_ROW2_FIRST) {
				slot -= INV_ROW_SLOT_SIZE;
                mousePos = InvGetSlotCoord(slot);
			}
		}
	} else if (dir.y == AxisDirectionY_DOWN) {
		if (isHoldingItem) {
			if (slot == SLOTXY_HEAD_FIRST || slot == SLOTXY_CHEST_FIRST) {
			    slot = SLOTXY_INV_ROW1_FIRST + 4;
                mousePos = InvGetSlotCoord(slot);
			} else if (slot == SLOTXY_RING_LEFT || slot == SLOTXY_HAND_LEFT_FIRST) {
                slot = SLOTXY_INV_ROW1_FIRST + 1;
                mousePos = InvGetSlotCoord(slot);
			} else if (slot == SLOTXY_RING_RIGHT || slot == SLOTXY_HAND_RIGHT_FIRST || slot == SLOTXY_AMULET) {
                slot = SLOTXY_INV_ROW1_LAST - 1;
                mousePos = InvGetSlotCoord(slot);
			} else if (slot <= (SLOTXY_INV_ROW3_LAST)) {
				slot += INV_ROW_SLOT_SIZE;
                mousePos = InvGetSlotCoord(slot);
			} else if (slot <= SLOTXY_INV_LAST && plr[myplr].HoldItem._itype == ITYPE_MISC && icursW28 == 1 && icursH28 == 1) { // forcing only 1x1 misc items
				slot += INV_ROW_SLOT_SIZE;
				if (slot > SLOTXY_BELT_LAST)
				    slot = SLOTXY_BELT_LAST;
                mousePos = BeltGetSlotCoord(slot);
			}
		} else {
			if (slot == SLOTXY_HEAD_FIRST) {
                slot = SLOTXY_CHEST_FIRST;
				mousePos = InvGetInvSlotCoord(INVLOC_CHEST);
			} else if (slot == SLOTXY_CHEST_FIRST) {
                slot = SLOTXY_INV_ROW1_FIRST + (INV_ROW_SLOT_SIZE / 2);
                mousePos = InvGetSlotCoord(slot);
			} else if (slot == SLOTXY_HAND_LEFT_FIRST) {
                slot = SLOTXY_RING_LEFT;
				mousePos = InvGetInvSlotCoord(INVLOC_RING_LEFT);
			} else if (slot == SLOTXY_RING_LEFT) {
                slot = SLOTXY_INV_ROW1_FIRST + 1;
                mousePos = InvGetSlotCoord(slot);
			} else if (slot == SLOTXY_RING_RIGHT) {
                slot = SLOTXY_INV_ROW1_LAST - 1;
                mousePos = InvGetSlotCoord(slot);
			} else if (slot == SLOTXY_AMULET) {
                slot = SLOTXY_HAND_RIGHT_FIRST;
				mousePos = InvGetInvSlotCoord(INVLOC_HAND_RIGHT);
			} else if (slot == SLOTXY_HAND_RIGHT_FIRST) {
                slot = SLOTXY_RING_RIGHT;
				mousePos = InvGetInvSlotCoord(INVLOC_RING_RIGHT);
			} else if (slot <= (SLOTXY_INV_ROW3_LAST)) {
				slot += INV_ROW_SLOT_SIZE;
                mousePos = InvGetSlotCoord(slot);
			} else if (slot <= SLOTXY_INV_LAST) {
				slot += INV_ROW_SLOT_SIZE;
                if (slot > SLOTXY_BELT_LAST)
                    slot = SLOTXY_BELT_LAST;
                mousePos = BeltGetSlotCoord(slot);
			}
		}
	}

	// not movement was made
	if (slot == initialSlot)
	    return;

    // move cursor to the center of the object or slot
    mousePos.y -= (INV_SLOT_SIZE_PX / 2);
    if (isHoldingItem) {
        mousePos.y -= (int)((icursH28 / 2.f) * INV_SLOT_SIZE_PX);
    } else {
        mousePos.x += (INV_SLOT_SIZE_PX / 2);
    }

	if (mousePos.x == MouseX && mousePos.y == MouseY) {
		return; // Avoid wobeling when scalled
	}

	SetCursorPos(mousePos.x, mousePos.y);
}

/**
 * check if hot spell at X Y exists
 */
bool HSExists(int x, int y)
{
	for (int r = 0; r < speedspellcount; r++) {
		if (x >= speedspellscoords[r].x - SPLICONLENGTH / 2
		    && x < speedspellscoords[r].x + SPLICONLENGTH / 2
		    && y >= speedspellscoords[r].y - SPLICONLENGTH / 2
		    && y < speedspellscoords[r].y + SPLICONLENGTH / 2) {
			return true;
		}
	}
	return false;
}

void HotSpellMove(AxisDirection dir)
{
	static AxisDirectionRepeater repeater;
	dir = repeater.Get(dir);
	if (dir.x == AxisDirectionX_NONE && dir.y == AxisDirectionY_NONE)
		return;

	int spbslot = plr[myplr]._pRSpell;
	for (int r = 0; r < speedspellcount; r++) {
		if (MouseX >= speedspellscoords[r].x - SPLICONLENGTH / 2
		    && MouseX < speedspellscoords[r].x + SPLICONLENGTH / 2
		    && MouseY >= speedspellscoords[r].y - SPLICONLENGTH / 2
		    && MouseY < speedspellscoords[r].y + SPLICONLENGTH / 2) {
			spbslot = r;
			break;
		}
	}

	int x = speedspellscoords[spbslot].x;
	int y = speedspellscoords[spbslot].y;

	if (dir.x == AxisDirectionX_LEFT) {
		if (spbslot < speedspellcount - 1) {
			x = speedspellscoords[spbslot + 1].x;
			y = speedspellscoords[spbslot + 1].y;
		}
	} else if (dir.x == AxisDirectionX_RIGHT) {
		if (spbslot > 0) {
			x = speedspellscoords[spbslot - 1].x;
			y = speedspellscoords[spbslot - 1].y;
		}
	}

	if (dir.y == AxisDirectionY_UP) {
		if (HSExists(x, y - SPLICONLENGTH)) {
			y -= SPLICONLENGTH;
		}
	} else if (dir.y == AxisDirectionY_DOWN) {
		if (HSExists(x, y + SPLICONLENGTH)) {
			y += SPLICONLENGTH;
		}
	}

	if (x != MouseX || y != MouseY) {
		SetCursorPos(x, y);
	}
}

void SpellBookMove(AxisDirection dir)
{
	static AxisDirectionRepeater repeater;
	dir = repeater.Get(dir);

	if (dir.x == AxisDirectionX_LEFT) {
		if (sbooktab > 0)
			sbooktab--;
	} else if (dir.x == AxisDirectionX_RIGHT) {
		if ((gbIsHellfire && sbooktab < 4) || (!gbIsHellfire && sbooktab < 3))
			sbooktab++;
	}
}

/**
 * @brief check if stepping in direction (dir) from x, y is blocked.
 *
 * If you step from A to B, at leat one of the Xs need to be clear:
 *
 *  AX
 *  XB
 *
 *  @return true if step is blocked
 */
bool IsPathBlocked(int x, int y, int dir)
{
	int d1, d2, d1x, d1y, d2x, d2y;

	switch (dir) {
	case DIR_N:
		d1 = DIR_NW;
		d2 = DIR_NE;
		break;
	case DIR_E:
		d1 = DIR_NE;
		d2 = DIR_SE;
		break;
	case DIR_S:
		d1 = DIR_SE;
		d2 = DIR_SW;
		break;
	case DIR_W:
		d1 = DIR_SW;
		d2 = DIR_NW;
		break;
	default:
		return false;
	}

	d1x = x + Offsets[d1][0];
	d1y = y + Offsets[d1][1];
	d2x = x + Offsets[d2][0];
	d2y = y + Offsets[d2][1];

	if (!nSolidTable[dPiece[d1x][d1y]] && !nSolidTable[dPiece[d2x][d2y]])
		return false;

	return !PosOkPlayer(myplr, d1x, d1y) && !PosOkPlayer(myplr, d2x, d2y);
}

direction GetNonBlockedDirection(int x, int y, int dir)
{
	int d1, d2;

	switch (dir) {
	case DIR_N:
		d1 = DIR_NW;
		d2 = DIR_NE;
		break;
	case DIR_E:
		d1 = DIR_NE;
		d2 = DIR_SE;
		break;
	case DIR_S:
		d1 = DIR_SE;
		d2 = DIR_SW;
		break;
	case DIR_W:
		d1 = DIR_SW;
		d2 = DIR_NW;
		break;
	default:
		return DIR_OMNI;
	}

	const int d1x = x + Offsets[d1][0];
	const int d1y = y + Offsets[d1][1];
	const int d2x = x + Offsets[d2][0];
	const int d2y = y + Offsets[d2][1];

	if (!nSolidTable[dPiece[d1x][d1y]])
		return (direction)d1;
	if (!nSolidTable[dPiece[d2x][d2y]])
		return (direction)d2;

	return DIR_OMNI;
}

void WalkInDirection(int playerId, const direction dir)
{
	const int x = plr[playerId].position.future.x;
	const int y = plr[playerId].position.future.y;
	
	const int dx = x + Offsets[dir][0];
	const int dy = y + Offsets[dir][1];
	plr[playerId]._pdir = dir;

	if (nSolidTable[dPiece[dx][dy]]) {
		const direction new_dir = GetNonBlockedDirection(x, y, dir);
		if (new_dir != DIR_OMNI) {
			WalkInDirection(playerId, new_dir);
			return;
		}
	}

	if (PosOkPlayer(playerId, dx, dy) && IsPathBlocked(x, y, dir)) {
		return; // Don't start backtrack around obstacles
	}

	NetSendCmdLoc(playerId, true, CMD_WALKXY, { dx, dy });
}

void WalkInAxisDirection(int playerId, const AxisDirection dir)
{
	const int x = plr[playerId].position.future.x;
	const int y = plr[playerId].position.future.y;

	if (dir.x == AxisDirectionX_NONE && dir.y == AxisDirectionY_NONE) {
		if (sgbControllerActive && plr[playerId].walkpath[0] != WALK_NONE && plr[playerId].destAction == ACTION_NONE)
			NetSendCmdLoc(playerId, true, CMD_WALKXY, { x, y }); // Stop walking
		return;
	}

	const direction pdir = FaceDir[static_cast<std::size_t>(dir.x)][static_cast<std::size_t>(dir.y)];

	WalkInDirection(playerId, pdir);
}

void QuestLogMove(AxisDirection moveDir)
{
	static AxisDirectionRepeater repeater;
	moveDir = repeater.Get(moveDir);
	if (moveDir.y == AxisDirectionY_UP)
		QuestlogUp();
	else if (moveDir.y == AxisDirectionY_DOWN)
		QuestlogDown();
}

void StoreMove(AxisDirection moveDir)
{
	static AxisDirectionRepeater repeater;
	moveDir = repeater.Get(moveDir);
	if (moveDir.y == AxisDirectionY_UP)
		STextUp();
	else if (moveDir.y == AxisDirectionY_DOWN)
		STextDown();
}

using HandleLeftStickOrDPadFn = void (*)(devilution::AxisDirection);

HandleLeftStickOrDPadFn GetLeftStickOrDPadGameUIHandler()
{
	if (invflag) {
		return &InvMove;
	}
	if (chrflag && plr[myplr]._pStatPts > 0) {
		return &AttrIncBtnSnap;
	} else if (spselflag) {
		return &HotSpellMove;
	} else if (sbookflag) {
		return &SpellBookMove;
	} else if (questlog) {
		return &QuestLogMove;
	} else if (stextflag != STORE_NONE) {
		return &StoreMove;
	}
	return nullptr;
}

void ProcessLeftStickOrDPadGameUI()
{
	HandleLeftStickOrDPadFn handler = GetLeftStickOrDPadGameUIHandler();
	if (handler != nullptr)
		handler(GetLeftStickOrDpadDirection(true));
}

void Movement(int playerId)
{
	if (InGameMenu()
	    || IsControllerButtonPressed(ControllerButton_BUTTON_START)
	    || IsControllerButtonPressed(ControllerButton_BUTTON_BACK))
		return;

	AxisDirection moveDir = GetMoveDirection();
	if (moveDir.x != AxisDirectionX_NONE || moveDir.y != AxisDirectionY_NONE) {
		sgbControllerActive = true;
	}

	if (GetLeftStickOrDPadGameUIHandler() == nullptr) {
		WalkInAxisDirection(playerId, moveDir);
	}
}

struct RightStickAccumulator {

	RightStickAccumulator()
	{
		lastTc = SDL_GetTicks();
		hiresDX = 0;
		hiresDY = 0;
	}

	void Pool(int *x, int *y, int slowdown)
	{
		const Uint32 tc = SDL_GetTicks();
		const int dtc = tc - lastTc;
		hiresDX += rightStickX * dtc;
		hiresDY += rightStickY * dtc;
		const int dx = hiresDX / slowdown;
		const int dy = hiresDY / slowdown;
		*x += dx;
		*y -= dy;
		lastTc = tc;
		// keep track of remainder for sub-pixel motion
		hiresDX -= dx * slowdown;
		hiresDY -= dy * slowdown;
	}

	void Clear()
	{
		lastTc = SDL_GetTicks();
	}

	DWORD lastTc;
	float hiresDX;
	float hiresDY;
};

} // namespace

void StoreSpellCoords()
{
	const int startX = PANEL_LEFT + 12 + SPLICONLENGTH / 2;
	const int endX = startX + SPLICONLENGTH * 10;
	const int endY = PANEL_TOP - 17 - SPLICONLENGTH / 2;
	speedspellcount = 0;
	int xo = endX;
	int yo = endY;
	for (int i = RSPLTYPE_SKILL; i <= RSPLTYPE_CHARGES; i++) {
		std::uint64_t spells;
		switch (i) {
		case RSPLTYPE_SKILL:
			spells = plr[myplr]._pAblSpells;
			break;
		case RSPLTYPE_SPELL:
			spells = plr[myplr]._pMemSpells;
			break;
		case RSPLTYPE_SCROLL:
			spells = plr[myplr]._pScrlSpells;
			break;
		case RSPLTYPE_CHARGES:
			spells = plr[myplr]._pISpells;
			break;
		}
		std::uint64_t spell = 1;
		for (int j = 1; j < MAX_SPELLS; j++) {
			if ((spell & spells) != 0) {
				speedspellscoords[speedspellcount] = { xo, yo };
				++speedspellcount;
				xo -= SPLICONLENGTH;
				if (xo < startX) {
					xo = endX;
					yo -= SPLICONLENGTH;
				}
			}
			spell <<= 1;
		}
		if (spells != 0 && xo != endX)
			xo -= SPLICONLENGTH;
		if (xo < startX) {
			xo = endX;
			yo -= SPLICONLENGTH;
		}
	}
}

bool IsAutomapActive()
{
	return AutomapActive && leveltype != DTYPE_TOWN;
}

bool IsMovingMouseCursorWithController()
{
	return rightStickX != 0 || rightStickY != 0;
}

void HandleRightStickMotion()
{
	static RightStickAccumulator acc;
	// deadzone is handled in ScaleJoystickAxes() already
	if (rightStickX == 0 && rightStickY == 0) {
		acc.Clear();
		return;
	}

	if (IsAutomapActive()) { // move map
		int dx = 0, dy = 0;
		acc.Pool(&dx, &dy, 32);
		AutomapOffset.x += dy + dx;
		AutomapOffset.y += dy - dx;
		return;
	}

	{ // move cursor
		sgbControllerActive = false;
		int x = MouseX;
		int y = MouseY;
		acc.Pool(&x, &y, 2);
		x = std::min(std::max(x, 0), gnScreenWidth - 1);
		y = std::min(std::max(y, 0), gnScreenHeight - 1);

		// We avoid calling `SetCursorPos` within the same SDL tick because
		// that can cause all stick motion events to arrive before all
		// cursor position events.
		static int lastMouseSetTick = 0;
		const int now = SDL_GetTicks();
		if (now - lastMouseSetTick > 0) {
			SetCursorPos(x, y);
			lastMouseSetTick = now;
		}
	}
}

/**
 * @brief Moves the mouse to the first inventory slot.
 */
void FocusOnInventory()
{
    slot = SLOTXY_INV_FIRST;
	SetCursorPos(InvRect[SLOTXY_INV_FIRST].X + RIGHT_PANEL + (INV_SLOT_SIZE_PX / 2), InvRect[SLOTXY_INV_FIRST].Y - (INV_SLOT_SIZE_PX / 2));
}

void plrctrls_after_check_curs_move()
{
	// check for monsters first, then items, then towners.
	if (sgbControllerActive) {
		// Clear focuse set by cursor
		pcursplr = -1;
		pcursmonst = -1;
		pcursitem = -1;
		pcursobj = -1;
		pcursmissile = -1;
		pcurstrig = -1;
		pcursquest = -1;
		cursmx = -1;
		cursmy = -1;
		if (!invflag) {
			*infostr = '\0';
			ClearPanel();
			FindActor();
			FindItemOrObject();
			FindTrigger();
		}
	}
}

void plrctrls_every_frame()
{
	ProcessLeftStickOrDPadGameUI();
	HandleRightStickMotion();
}

void plrctrls_after_game_logic()
{
	Movement(myplr);
}

void UseBeltItem(int type)
{
	for (int i = 0; i < MAXBELTITEMS; i++) {
		const int id = AllItemsList[plr[myplr].SpdList[i].IDidx].iMiscId;
		const int spellId = AllItemsList[plr[myplr].SpdList[i].IDidx].iSpell;
		if ((type == BLT_HEALING && (id == IMISC_HEAL || id == IMISC_FULLHEAL || (id == IMISC_SCROLL && spellId == SPL_HEAL)))
		    || (type == BLT_MANA && (id == IMISC_MANA || id == IMISC_FULLMANA))
		    || id == IMISC_REJUV || id == IMISC_FULLREJUV) {
			if (plr[myplr].SpdList[i]._itype > -1) {
				UseInvItem(myplr, INVITEM_BELT_FIRST + i);
				break;
			}
		}
	}
}

void PerformPrimaryAction()
{
	if (invflag) { // inventory is open
		if (pcurs > CURSOR_HAND && pcurs < CURSOR_FIRSTITEM) {
			TryIconCurs();
			NewCursor(CURSOR_HAND);
		} else {
			CheckInvItem();
		}
		return;
	}

	if (spselflag) {
		SetSpell();
		return;
	}

	if (chrflag && !chrbtnactive && plr[myplr]._pStatPts > 0) {
		CheckChrBtns();
		for (int i = 0; i < 4; i++) {
			if (MouseX >= ChrBtnsRect[i].x
			    && MouseX <= ChrBtnsRect[i].x + ChrBtnsRect[i].w
			    && MouseY >= ChrBtnsRect[i].y
			    && MouseY <= ChrBtnsRect[i].h + ChrBtnsRect[i].y) {
				chrbtn[i] = true;
				chrbtnactive = true;
				ReleaseChrBtns(false);
			}
		}
		return;
	}

	Interact();
}

bool SpellHasActorTarget()
{
	int spl = plr[myplr]._pRSpell;
	if (spl == SPL_TOWN || spl == SPL_TELEPORT)
		return false;

	if (spl == SPL_FIREWALL && pcursmonst != -1) {
		cursmx = monster[pcursmonst].position.tile.x;
		cursmy = monster[pcursmonst].position.tile.y;
	}

	return pcursplr != -1 || pcursmonst != -1;
}

void UpdateSpellTarget()
{
	if (SpellHasActorTarget())
		return;

	pcursplr = -1;
	pcursmonst = -1;

	const PlayerStruct &player = plr[myplr];

	int range = 1;
	if (plr[myplr]._pRSpell == SPL_TELEPORT)
		range = 4;

	cursmx = player.position.future.x + Offsets[player._pdir][0] * range;
	cursmy = player.position.future.y + Offsets[player._pdir][1] * range;
}

/**
 * @brief Try dropping item in all 9 possible places
 */
bool TryDropItem()
{
	cursmx = plr[myplr].position.future.x + 1;
	cursmy = plr[myplr].position.future.y;
	if (!DropItemBeforeTrig()) {
		// Try to drop on the other side
		cursmx = plr[myplr].position.future.x;
		cursmy = plr[myplr].position.future.y + 1;
		DropItemBeforeTrig();
	}

	return pcurs == CURSOR_HAND;
}

void PerformSpellAction()
{
	if (InGameMenu() || questlog || sbookflag)
		return;

	if (invflag) {
		if (pcurs >= CURSOR_FIRSTITEM)
			TryDropItem();
		else if (pcurs > CURSOR_HAND) {
			TryIconCurs();
			NewCursor(CURSOR_HAND);
		}
		return;
	}

	if (pcurs >= CURSOR_FIRSTITEM && !TryDropItem())
		return;
	if (pcurs > CURSOR_HAND)
		NewCursor(CURSOR_HAND);

	if (spselflag) {
		SetSpell();
		return;
	}

	int spl = plr[myplr]._pRSpell;
	if ((pcursplr == -1 && (spl == SPL_RESURRECT || spl == SPL_HEALOTHER))
	    || (pcursobj == -1 && spl == SPL_DISARM)) {
		plr[myplr].PlaySpeach(27);
		return;
	}

	UpdateSpellTarget();
	CheckPlrSpell();
}

void CtrlUseInvItem()
{
	ItemStruct *item;

	if (pcursinvitem == -1)
		return;

	if (pcursinvitem <= INVITEM_INV_LAST)
		item = &plr[myplr].InvList[pcursinvitem - INVITEM_INV_FIRST];
	else
		item = &plr[myplr].SpdList[pcursinvitem - INVITEM_BELT_FIRST];

	if ((item->_iMiscId == IMISC_SCROLLT || item->_iMiscId == IMISC_SCROLL) && spelldata[item->_iSpell].sTargeted) {
		return;
	}

	UseInvItem(myplr, pcursinvitem);
}

void PerformSecondaryAction()
{
	if (invflag) {
		CtrlUseInvItem();
		return;
	}

	if (pcurs >= CURSOR_FIRSTITEM && !TryDropItem())
		return;
	if (pcurs > CURSOR_HAND)
		NewCursor(CURSOR_HAND);

	if (pcursitem != -1) {
		NetSendCmdLocParam1(true, CMD_GOTOAGETITEM, { cursmx, cursmy }, pcursitem);
	} else if (pcursobj != -1) {
		NetSendCmdLocParam1(true, CMD_OPOBJXY, { cursmx, cursmy }, pcursobj);
	} else if (pcursmissile != -1) {
		MakePlrPath(myplr, missile[pcursmissile].position.tile.x, missile[pcursmissile].position.tile.y, true);
		plr[myplr].destAction = ACTION_WALK;
	} else if (pcurstrig != -1) {
		MakePlrPath(myplr, trigs[pcurstrig].position.x, trigs[pcurstrig].position.y, true);
		plr[myplr].destAction = ACTION_WALK;
	} else if (pcursquest != -1) {
		MakePlrPath(myplr, quests[pcursquest].position.x, quests[pcursquest].position.y, true);
		plr[myplr].destAction = ACTION_WALK;
	}
}

} // namespace devilution
