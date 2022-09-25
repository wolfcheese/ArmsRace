#include "framework.h"
#include "tr5_roman_statue.h"
#include "Game/items.h"
#include "Game/effects/tomb4fx.h"
#include "Game/effects/effects.h"
#include "Game/collision/collide_room.h"
#include "Game/control/box.h"
#include "Game/people.h"
#include "Game/effects/debris.h"
#include "Game/animation.h"
#include "Game/effects/lightning.h"
#include "Game/itemdata/creature_info.h"
#include "Game/Lara/lara.h"
#include "Game/misc.h"
#include "Sound/sound.h"
#include "Specific/setup.h"
#include "Specific/level.h"
#include <Game/health.h>
#include <weather.h>

using namespace TEN::Effects::Lightning;
using namespace TEN::Effects::Environment;

struct RomanStatueInfo
{
	Vector3Int Position;
	LIGHTNING_INFO* EnergyArcs[8];
	unsigned int Count;
};

RomanStatueInfo RomanStatueData;
BITE_INFO RomanStatueBite{ 0, 0, 0, 15 };

// TODO
enum RomanStatueState
{
	STATUE_STATE_IDLE = 1,
	STATUE_STATE_SCREAM = 2,
	STATUE_STATE_ATTACK_1 = 3,
	STATUE_STATE_ATTACK_2 = 4,
	STATUE_STATE_ATTACK_3 = 5,
	STATUE_STATE_HIT = 6,
	STATUE_STATE_WALK = 7,

	STATUE_STATE_ATTACK_4 = 9,
	STATUE_STATE_TURN_180 = 10,
	STATUE_STATE_DEATH = 11,
	STATUE_STATE_ENERGY_ATTACK = 12
};

// TODO
enum RomanStatueAnim
{
	STATUE_ANIM_HIT = 5,
	STATUE_ANIM_DEATH = 14,
	STATUE_ANIM_START_JUMP_DOWN = 16
};

short flameEmitters[8];
short teleportObject[2];

static void GetTargetPosition(ItemInfo* item, PHD_3DPOS* target)
{
	CreatureInfo* creature = (CreatureInfo*)item->Data;
	Vector3Int start, end;
	start.x = 0;
	start.y = -96;
	start.z = 144;
	GetJointAbsPosition(item, &start, 14);
	end.x = creature->Enemy->Pose.Position.x;
	end.y = creature->Enemy->Pose.Position.y;
	end.z = creature->Enemy->Pose.Position.z;
	auto angles = GetVectorAngles(end.x - start.x, end.y - start.y, end.z - start.z);
	target->Position.x = start.x;
	target->Position.y = start.y;
	target->Position.z = start.z;
	target->Orientation = angles;
}

static void TriggerFireAttack(ItemInfo* item)
{
	PHD_3DPOS src;
	GetTargetPosition(item, &src);
	FX_INFO* fx;
	short fxNumber = NO_ITEM;

	fxNumber = CreateNewEffect(item->RoomNumber);
	if (fxNumber != NO_ITEM)
	{
		fx = &EffectList[fxNumber];
		fx->pos.Position.x = src.Position.x;
		fx->pos.Position.y = src.Position.y - (GetRandomControl() & 0x3F) - 32;
		fx->pos.Position.z = src.Position.z;
		fx->pos.Orientation.x = src.Orientation.x;
		fx->pos.Orientation.y = src.Orientation.y;
		fx->pos.Orientation.z = 0;
		fx->roomNumber = item->RoomNumber;
		fx->counter = 31;
		fx->objectNumber = ID_ENERGY_BUBBLES;
		fx->frameNumber = Objects[fx->objectNumber].meshIndex + 5;
		fx->speed = (GetRandomControl() & 0x1F) + 300;
		fx->flag1 = 6;
	}
	for (int i = 0;i < 8;i++)
	{
		ItemInfo* flameItem = &g_Level.Items[flameEmitters[i]];
		if (flameItem->Status != ITEM_ACTIVE)
		{
			flameItem->Status = ITEM_ACTIVE;
			flameItem->Flags |= IFLAG_ACTIVATION_MASK;
			AddActiveItem(flameEmitters[i]);
		}
		flameItem->ItemFlags[7] = 1;
	}
	bool flipped = FlipStats[g_Level.Rooms[item->ItemFlags[4]].flipNumber] == 1;
	if (flipped)
	{
		Weather.Flash(242, 161, 22, 0.06f);
		DoFlipMap(g_Level.Rooms[item->ItemFlags[4]].flipNumber);
	}
}

void TriggerIceAttack(ItemInfo* item)
{
	/*PHD_3DPOS src;
	GetTargetPosition(item, &src);
	FX_INFO* fx;
	short fxNumber = NO_ITEM;

	fxNumber = CreateNewEffect(item->RoomNumber);
	if (fxNumber != NO_ITEM)
	{
		fx = &EffectList[fxNumber];
		fx->pos.Position.x = src.Position.x;
		fx->pos.Position.y = src.Position.y - (GetRandomControl() & 0x3F) - 64;
		fx->pos.Position.z = src.Position.z;
		fx->pos.Orientation.x = src.Orientation.x;
		fx->pos.Orientation.y = src.Orientation.y;
		fx->pos.Orientation.z = 0;
		fx->roomNumber = item->RoomNumber;
		fx->counter = 31;
		fx->objectNumber = ID_ENERGY_BUBBLES;
		fx->frameNumber = Objects[fx->objectNumber].meshIndex + 5;
		fx->speed = (GetRandomControl() & 0x1F) + 140;
		fx->flag1 = 3;
	}*/
	
	bool flipped = FlipStats[g_Level.Rooms[item->ItemFlags[4]].flipNumber] == 1;
	if (!flipped)
	{
		Weather.Flash(7, 215, 247, 0.06f);
		DoFlipMap(g_Level.Rooms[item->ItemFlags[4]].flipNumber);
	}
}

static void RomanStatueHitEffect(ItemInfo* item, Vector3Int* pos, int joint)
{
	GetJointAbsPosition(item, pos, joint);

	/*wc - don't want to worry about bubbles
	if (!(GetRandomControl() & 0x1F))
	{
		short fxNumber = CreateNewEffect(item->RoomNumber);
		if (fxNumber != -1)
		{
			auto* fx = &EffectList[fxNumber];

			fx->pos.Position.x = pos->x;
			fx->pos.Position.y = pos->y;
			fx->pos.Position.z = pos->z;
			fx->RoomNumber = item->RoomNumber;
			fx->pos.Orientation.z = 0;
			fx->pos.Orientation.x = 0;
			fx->pos.Orientation.y = 2 * GetRandomControl();
			fx->speed = 1;
			fx->fallspeed = 0;
			fx->ObjectNumber = ID_BODY_PART;
			fx->shade = 16912;
			fx->flag2 = 9729;
			fx->frameNumber = Objects[ID_BUBBLES].meshIndex + (GetRandomControl() & 7);
			fx->counter = 0;
			fx->flag1 = 0;
		}
	}*/

	if (!(GetRandomControl() & 0xF))
	{
		auto* spark = &SmokeSparks[GetFreeSmokeSpark()];

		spark->on = 1;
		spark->sShade = 0;
		spark->colFadeSpeed = 4;
		spark->fadeToBlack = 32;
		spark->dShade = (GetRandomControl() & 0xF) + 64;
		spark->blendMode = BLEND_MODES::BLENDMODE_ADDITIVE;
		spark->life = spark->sLife = (GetRandomControl() & 3) + 64;
		spark->x = (GetRandomControl() & 0x1F) + pos->x - 16;
		spark->y = (GetRandomControl() & 0x1F) + pos->y - 16;
		spark->z = (GetRandomControl() & 0x1F) + pos->z - 16;
		spark->xVel = (GetRandomControl() & 0x7F) - 64;
		spark->yVel = 0;
		spark->zVel = (GetRandomControl() & 0x7F) - 64;
		spark->friction = 4;
		spark->flags = SP_ROTATE;
		spark->rotAng = GetRandomControl() & 0xFFF;
		spark->rotAdd = (GetRandomControl() & 0x1F) - 16;
		spark->maxYvel = 0;
		spark->gravity = (GetRandomControl() & 7) + 8;
		spark->mirror = 0;
		spark->sSize = spark->size = (GetRandomControl() & 7) + 8;
		spark->dSize = spark->size * 2;
	}
}

static void TriggerRomanStatueShockwaveAttackSparks(int x, int y, int z, byte r, byte g, byte b, byte size)
{
	auto* spark = GetFreeParticle();

	spark->dG = g;
	spark->sG = g;
	spark->colFadeSpeed = 2;
	spark->dR = r;
	spark->sR = r;
	spark->blendMode = BLEND_MODES::BLENDMODE_ADDITIVE;
	spark->life = 16;
	spark->sLife = 16;
	spark->x = x;
	spark->on = 1;
	spark->dB = b;
	spark->sB = b;
	spark->fadeToBlack = 4;
	spark->y = y;
	spark->z = z;
	spark->zVel = 0;
	spark->yVel = 0;
	spark->xVel = 0;
	spark->flags = SP_SCALE | SP_DEF;
	spark->scalar = 3;
	spark->maxYvel = 0;
	spark->spriteIndex = Objects[ID_DEFAULT_SPRITES].meshIndex + 11;
	spark->gravity = 0;
	spark->dSize = spark->sSize = spark->size = size + (GetRandomControl() & 3);
}

static void TriggerRomanStatueScreamingSparks(int x, int y, int z, short xv, short yv, short zv, int flags)
{
	auto* spark = GetFreeParticle();

	spark->on = 1;
	spark->sR = 0;
	spark->sG = 0;
	spark->sB = 0;
	spark->dR = 64;

	if (flags)
	{
		spark->dG = (GetRandomControl() & 0x3F) - 64;
		spark->dB = spark->dG / 2;
	}
	else
	{
		spark->dB = (GetRandomControl() & 0x3F) - 64;
		spark->dG = spark->dB / 2;
	}

	spark->colFadeSpeed = 4;
	spark->fadeToBlack = 4;
	spark->life = 16;
	spark->sLife = 16;
	spark->x = x;
	spark->y = y;
	spark->z = z;
	spark->xVel = xv;
	spark->yVel = yv;
	spark->zVel = zv;
	spark->blendMode = BLEND_MODES::BLENDMODE_ADDITIVE;
	spark->friction = 34;
	spark->maxYvel = 0;
	spark->gravity = 0;
	spark->flags = SP_NONE;
}

static void TriggerRomanStatueAttackEffect1(short itemNum, int factor)
{
	auto* spark = GetFreeParticle();

	spark->on = 1;
	spark->sR = 0;
	spark->sB = (GetRandomControl() & 0x3F) - 96;
	spark->dB = (GetRandomControl() & 0x3F) - 96;
	spark->dR = 0;

	if (factor < 16)
	{
		spark->sB = (factor * spark->sB) / 16;
		spark->dB = (factor * spark->dB) / 16;
	}

	spark->sG = spark->sB / 2;
	spark->dG = spark->dB / 2;
	spark->fadeToBlack = 4;
	spark->colFadeSpeed = (GetRandomControl() & 3) + 8;
	spark->blendMode = BLEND_MODES::BLENDMODE_ADDITIVE;
	spark->dynamic = -1;
	spark->life = spark->sLife = (GetRandomControl() & 3) + 32;
	spark->y = 0;
	spark->x = (GetRandomControl() & 0x1F) - 16;
	spark->z = (GetRandomControl() & 0x1F) - 16;
	spark->yVel = 0;
	spark->xVel = (byte)GetRandomControl() - 128;
	spark->friction = 4;
	spark->zVel = (byte)GetRandomControl() - 128;
	spark->flags = SP_NODEATTACH | SP_EXPDEF | SP_ITEM | SP_ROTATE | SP_DEF | SP_SCALE; // 4762;
	spark->fxObj = itemNum;
	spark->nodeNumber = 6;
	spark->rotAng = GetRandomControl() & 0xFFF;
	spark->rotAdd = (GetRandomControl() & 0x3F) - 32;
	spark->maxYvel = 0;
	spark->gravity = -8 - (GetRandomControl() & 7);
	spark->scalar = 2;
	spark->dSize = 4;
	spark->sSize = (spark->size = factor * ((GetRandomControl() & 0x1F) + 64)) / 16;
}

static void RomanStatueAttack(PHD_3DPOS* pos, short RoomNumber, short count)
{
	//wc - don't want to worry about bubbles
	return;
	short fxNumber = CreateNewEffect(RoomNumber);

	if (fxNumber != NO_ITEM)
	{
		auto* fx = &EffectList[fxNumber];

		fx->pos.Position.x = pos->Position.x;
		fx->pos.Position.y = pos->Position.y;
		fx->pos.Position.z = pos->Position.z;
		fx->pos.Orientation.x = pos->Orientation.x;
		fx->pos.Orientation.y = pos->Orientation.y;
		fx->pos.Orientation.z = 0;
		fx->roomNumber = RoomNumber;
		fx->counter = 16 * count + 15;
		fx->flag1 = 1;
		fx->objectNumber = ID_BUBBLES;
		fx->speed = (GetRandomControl() & 0x1F) + 64;
		fx->frameNumber = Objects[ID_BUBBLES].meshIndex + 8;
	}
}

void TriggerRomanStatueMissileSparks(Vector3Int* pos, char fxObject)
{
	auto* spark = GetFreeParticle();

	spark->on = 1;
	spark->sR = 0;
	spark->sG = (GetRandomControl() & 0x3F) - 96;
	spark->sB = spark->sG / 2;
	spark->dR = 0;
	spark->dG = (GetRandomControl() & 0x3F) - 96;
	spark->dB = spark->dG / 2;
	spark->fadeToBlack = 8;
	spark->colFadeSpeed = (GetRandomControl() & 3) + 8;
	spark->blendMode = BLEND_MODES::BLENDMODE_ADDITIVE;
	spark->dynamic = -1;
	spark->life = spark->sLife = (GetRandomControl() & 3) + 20;
	spark->x = (GetRandomControl() & 0xF) - 8;
	spark->y = (GetRandomControl() & 0xF) - 8;
	spark->z = (GetRandomControl() & 0xF) - 8;
	spark->xVel = (GetRandomControl() & 0x3FF) - 512;
	spark->yVel = (GetRandomControl() & 0x3FF) - 512;
	spark->zVel = (GetRandomControl() & 0x3FF) - 512;
	spark->friction = 68;
	spark->flags = SP_ROTATE | SP_FX | SP_ROTATE | SP_DEF | SP_SCALE;
	spark->rotAng = GetRandomControl() & 0xFFF;
	spark->gravity = 0;
	spark->maxYvel = 0;
	spark->rotAdd = (GetRandomControl() & 0x3F) - 32;
	spark->fxObj = fxObject;
	spark->scalar = 2;
	spark->sSize = spark->size = (GetRandomControl() & 0xF) + 96;
	spark->dSize = spark->size / 4;
}

void InitialiseRomanStatue(short itemNumber)
{
	auto* item = &g_Level.Items[itemNumber];
	
	ClearItem(itemNumber);
	
	item->Animation.AnimNumber = Objects[item->ObjectNumber].animIndex + STATUE_ANIM_START_JUMP_DOWN;
	item->Animation.TargetState = 13;
	item->Animation.ActiveState = 13;
	item->Animation.FrameNumber = g_Level.Anims[item->Animation.AnimNumber].frameBase;
	item->Status = ITEM_NOT_ACTIVE;
	item->Pose.Position.x += 486 * phd_sin(item->Pose.Orientation.y + ANGLE(90.0f));
	item->Pose.Position.z += 486 * phd_cos(item->Pose.Orientation.y + ANGLE(90.0f));

	item->ItemFlags[5] = 1;
	item->ItemFlags[4] = item->RoomNumber;

	ZeroMemory(&RomanStatueData, sizeof(RomanStatueInfo));
}

void RomanStatueControl(short itemNumber)
{
	if (!CreatureActive(itemNumber))
		return;


	short angle = 0;
	short joint0 = 0;
	short joint1 = 0;
	short joint2 = 0;
	
	auto* item = &g_Level.Items[itemNumber];
	auto* creature = GetCreatureInfo(item);
	
	int oldMeshSwapBits = item->MeshSwapBits;

	//wc - collect items in boss's area
	if (teleportObject[0] == 0)
	{
		int j = 0;
		for (int i = 0; i < g_Level.NumItems; i++)
		{
			ItemInfo* item2 = &g_Level.Items[i];
			if (item2->ObjectNumber == ID_FLAME_EMITTER && item->ItemFlags[4] == item2->RoomNumber)
			{
				flameEmitters[j++] = i;
			}
			else if (item2->ObjectNumber == ID_CAMERA_TARGET && item2->TriggerFlags == 999)
				teleportObject[0] = i;
			else if (item2->ObjectNumber == ID_CAMERA_TARGET && item2->TriggerFlags == 998)
				teleportObject[1] = i;
			if (j > 8 && teleportObject[0] > 0 && teleportObject[1] > 0)
				break;
		}
	}

	// At determined HP values, roman statues sheds material.
	if (item->HitPoints < 1 && !(item->MeshSwapBits & 0x10000))
	{
		ExplodeItemNode(item, 16, 0, 8);
		item->MeshBits |= 0x10000;
		item->MeshSwapBits |= 0x10000;
	}
	else if (item->HitPoints < 125 && !(item->MeshSwapBits & 0x100))
	{
		ExplodeItemNode(item, 8, 0, 8);
		item->MeshBits |= 0x100;
		item->MeshSwapBits |= 0x100;
	}
	else if (item->HitPoints < 300 && !(item->MeshSwapBits & 0x400))
	{
		ExplodeItemNode(item, 10, 0, 32);
		ExplodeItemNode(item, 11, 0, 32);
		item->MeshBits |= 0x400u;
		item->MeshSwapBits |= 0x400;
		//wc - boss jumps up a level and switches to fire mode
		ItemInfo* teleportItem = &g_Level.Items[teleportObject[0]];
		item->Animation.AnimNumber = Objects[item->ObjectNumber].animIndex + 17;
		item->Animation.FrameNumber = g_Level.Anims[item->Animation.AnimNumber].frameBase;
		item->Pose.Position = teleportItem->Pose.Position;
		item->Pose.Orientation.y = ANGLE(180);
		item->ItemFlags[5] = 0;
		Weather.Flash(28, 242, 24, 0.06f);
	}
	else if (item->HitPoints < 450 && !(item->MeshSwapBits & 0x10))
	{
		ExplodeItemNode(item, 4, 0, 8);
		item->MeshBits |= 0x10;
		item->MeshSwapBits |= 0x10;
	}

	// Play hit animation.
	/*if (oldSwapMeshFlags != item->SwapMeshFlags)
	{
		//wc - removing this, don't want flinching
		item->Animation.ActiveState = STATUE_STATE_HIT;
		item->Animation.AnimNumber = Objects[item->ObjectNumber].animIndex + STATUE_ANIM_HIT;
		item->Animation.FrameNumber = g_Level.Anims[item->Animation.AnimNumber].frameBase;
		item->Animation.TargetState = STATUE_STATE_HIT;
		item->currentAnimState = STATUE_STATE_HIT;
		item->Animation.AnimNumber = Objects[item->ObjectNumber].animIndex + ANIMATION_ROMAN_STATUE_HIT;
		item->Animation.FrameNumber = g_Level.Anims[item->Animation.AnimNumber].frameBase;
		
	}*/

	if (item->HitPoints > 0)
	{
		creature->Enemy = LaraItem;

		BaddyHealthItemNumber = itemNumber;

		AI_INFO AI;
		CreatureAIInfo(item,&AI);
		
		GetCreatureMood(item, &AI, VIOLENT);
		CreatureMood(item, &AI, VIOLENT);
		
		angle = CreatureTurn(item, creature->MaxTurn);
		
		if (AI.ahead)
		{
			joint0 = AI.angle / 2;
			joint2 = AI.angle / 2;
			joint1 = AI.xAngle;
		}

		creature->MaxTurn = 0;

		Vector3Int pos, pos1, pos2;
		byte color;
		int deltaFrame;
		bool unknown;

		//wc - lava floor harms boss
		auto roomNumber = item->RoomNumber;
		auto floor = GetFloor(item->Pose.Position.x, item->Pose.Position.y, item->Pose.Position.z, &roomNumber);
		if (floor->Flags.Death)
		{
			item->HitPoints--;
			item->ItemFlags[5] = 1;
			item->Animation.TargetState = STATUE_STATE_SCREAM;
		}

		switch (item->Animation.ActiveState)
		{
		case STATUE_STATE_IDLE:
			creature->Flags = 0;
			joint2 = AI.angle;

			if (creature->Mood == MoodType::Attack)
				creature->MaxTurn = ANGLE(2.0f);
			else
			{
				creature->MaxTurn = 0;
				//wc - only walk if room is iced (boss can reach Lara safely)
				bool flipped = FlipStats[g_Level.Rooms[item->ItemFlags[4]].flipNumber] == 1;
				if (flipped && item->ItemFlags[5] == 1)
					item->Animation.TargetState = STATUE_STATE_WALK;
				else
					item->Animation.TargetState = STATUE_STATE_SCREAM;
			}

			if (item->AIBits ||

				(creature->Mood != MoodType::Attack))
			{
				joint2 = AIGuard((CreatureInfo*)creature);
			}
			else if (AI.angle > ANGLE(112.5f) || AI.angle < -ANGLE(112.5f))
				item->Animation.TargetState = STATUE_STATE_TURN_180;
			else if (AI.ahead && AI.distance < pow(SECTOR(1), 2))
			{
				item->Animation.TargetState = STATUE_STATE_ATTACK_1;
			}
			else
			{
				//wc - only walk if room is iced (boss can reach Lara)
				bool flipped = FlipStats[g_Level.Rooms[item->ItemFlags[4]].flipNumber] == 1;
				if (flipped && item->ItemFlags[5] == 1)
					item->Animation.TargetState = STATUE_STATE_WALK;
				else
					item->Animation.TargetState = STATUE_STATE_SCREAM;
			}

			break;

		case STATUE_STATE_SCREAM:
			unknown = false;

			pos1 = { -32, 48, 64 };
			GetJointAbsPosition(item, &pos1, 14);

			pos2 = { -48, 48, 490 };
			GetJointAbsPosition(item, &pos2, 14);

			pos = { (pos1.x + pos2.x) / 2, (pos1.y + pos2.y) / 2, (pos1.z + pos2.z) / 2 };

			deltaFrame = item->Animation.FrameNumber - g_Level.Anims[item->Animation.AnimNumber].frameBase;

			//wc - custom attacks during charging
			switch (item->ItemFlags[5])
			{
			case 0:
				if (deltaFrame >= 105 && deltaFrame <= 110)
					TriggerFireAttack(item);
				break;
			case 1:
				if (deltaFrame >= 105 && deltaFrame <= 110)
					TriggerIceAttack(item);
				break;
			}

			if (deltaFrame > 68 && deltaFrame < 130)
			{
				int deltaFrame2 = deltaFrame - 68;
				if (deltaFrame2 <= 58)
				{
					if (deltaFrame2 > 16)
						deltaFrame2 = 16;
				}
				else
					deltaFrame2 = 4 * (62 - deltaFrame);

				if (item->ItemFlags[5] == 1)
				{
					color = (deltaFrame2 * ((GetRandomControl() & 0x3F) + 128)) / 16;

					if (item->TriggerFlags)
						TriggerDynamicLight(pos.x, pos.y, pos.z, 16, 0, color, color / 2);
					else
						TriggerDynamicLight(pos.x, pos.y, pos.z, 16, 0, color / 2, color);
				}
				else if (item->ItemFlags[5] == 0)
				{
					TriggerDynamicLight(pos.x, pos.y, pos.z, 16, 235, 100, 52);
				}

				for (int i = 0; i < 2; i++)
				{
					short random = GetRandomControl();

					int x = (GetRandomControl() & 0x7FF) + pos.x - SECTOR(1);
					int y = (GetRandomControl() & 0x7FF) + pos.y - SECTOR(1);
					int z = (random & 0x7FF) + pos.z - SECTOR(1);

					TriggerRomanStatueScreamingSparks(
						x,
						y,
						z,
						8 * (pos.x - x),
						8 * (pos.y - y),
						8 * (SECTOR(1) - (random & 0x7FF)),
						item->TriggerFlags);
				}
			}

			if (deltaFrame <= 90 || deltaFrame >= 130)
				break;

			if (item->TriggerFlags)
				pos = { -48, 48, GetRandomControl() % 480 };
			else
				pos = { -40, 64, GetRandomControl() % 360 };

			GetJointAbsPosition(item, &pos, 14);

			color = (GetRandomControl() & 0x3F) + 128;

			pos1.x = (GetRandomControl() & 0xFFF) + item->Pose.Position.x - SECTOR(2);
			pos1.y = item->Pose.Position.y - (GetRandomControl() & 0x3FF) - SECTOR(4);
			pos1.z = (GetRandomControl() & 0xFFF) + item->Pose.Position.z - SECTOR(2);

			for (int i = 0; i < 8; i++)
			{
				auto* arc = RomanStatueData.EnergyArcs[i];

				if (arc && arc->life)
				{
					arc->pos4.x = pos2.x;
					arc->pos4.y = pos2.y;
					arc->pos4.z = pos2.z;

					if (item->TriggerFlags)
						TriggerLightningGlow(pos1.x, pos1.y, pos1.z, 16, 0, color, color / 2);
					else
						TriggerLightningGlow(pos1.x, pos1.y, pos1.z, 16, 0, color / 2, color);

					continue;
				}

				if (!(GlobalCounter & 3) || unknown)
				{
					unknown = true;
					continue;
				}

				if (item->TriggerFlags)
				{
					/*RomanStatueData.energyArcs[i] = TriggerEnergyArc(
						(Vector3Int*)& dest.Orientation.x,
						(Vector3Int*)& dest,
						(GetRandomControl() & 0x3F) + 16,
						(color >> 1) | ((color | 0x180000) << 8),
						15,
						48,
						5);*/

					TriggerLightningGlow(pos.x, pos.y, pos.z, 16, 0, color, color / 2);
					unknown = 1;
					continue;
				}

				/*RomanStatueData.energyArcs[i] = TriggerEnergyArc(
					&pos2,
					&pos1,
					(GetRandomControl() & 0x3F) + 16,
					color | (((color >> 1) | 0x180000) << 8),
					15,
					48,
					5);*/

				TriggerLightningGlow(pos.x, pos.y, pos.z, 16, 0, color / 2, color);
				unknown = true;
			}

			break;

		case STATUE_STATE_ATTACK_1:
		case STATUE_STATE_ATTACK_2:
		case STATUE_STATE_ATTACK_3:
		case STATUE_STATE_ATTACK_4:
			creature->MaxTurn = 0;

			if (abs(AI.angle) >= ANGLE(2.0f))
			{
				if (AI.angle >= 0)
					item->Pose.Orientation.y += ANGLE(2.0f);
				else
					item->Pose.Orientation.y -= ANGLE(2.0f);
			}
			else
				item->Pose.Orientation.y += AI.angle;

			if (item->Animation.FrameNumber > g_Level.Anims[item->Animation.AnimNumber].frameBase + 10)
			{
				pos = { 0, 0, 0 };
				GetJointAbsPosition(item, &pos, 16);

				auto* room = &g_Level.Rooms[item->RoomNumber];
				FloorInfo* floor = GetSector(room, pos.x - room->x, pos.z - room->z);

				// If floor is stopped, then try to find static meshes and shatter them, activating heavy triggers below
				if (floor->Stopper)
				{
					for (int i = 0; i < room->mesh.size(); i++)
					{
						auto* mesh = &room->mesh[i];

						if (!((mesh->pos.Position.z ^ pos.z) & 0xFFFFFC00) && !((mesh->pos.Position.x ^ pos.x) & 0xFFFFFC00))
						{
							if (StaticObjects[mesh->staticNumber].shatterType != SHT_NONE)
							{
								ShatterObject(0, mesh, -64, LaraItem->RoomNumber, 0);
								SoundEffect(GetShatterSound(mesh->staticNumber), (PHD_3DPOS*)mesh);

								mesh->flags &= ~StaticMeshFlags::SM_VISIBLE;
								floor->Stopper = false;

								TestTriggers(pos.x, pos.y, pos.z, item->RoomNumber, true);
							}
						}
					}
				}

				if (!creature->Flags)
				{
					if (item->TouchBits & 0xC000)
					{
						DoDamage(creature->Enemy, 200);
						CreatureEffect2(item, &RomanStatueBite, 20, item->Pose.Orientation.y, DoBloodSplat);
						SoundEffect(SFX_TR4_LARA_THUD, &item->Pose);
						creature->Flags = 1;
						//wc - weaker timestop effect on Lara
						if (AI.distance < SQUARE(1024 + CLICK(4)) && FreezeTimer <= 0)
						{
							LaraItem->AfterDeath = -60;
							FreezeTimer = 127;
							Weather.Flash(2, 37, 179, 0.06f);
						}
					}
				}

				if (!item->TriggerFlags)
				{
					pos1 = { -40, 64, 360 };
					GetJointAbsPosition(item, &pos1, 14);

					pos1.y = item->Pose.Position.y - 64;

					if (item->Animation.FrameNumber == g_Level.Anims[item->Animation.AnimNumber].frameBase + 34 && item->Animation.ActiveState == 3)
					{
						if (item->ItemFlags[0])
							item->ItemFlags[0]--;
						TriggerShockwave((PHD_3DPOS*)&pos1, 16, 160, 96, 0, 64, 128, 48, 0, 1);
						TriggerRomanStatueShockwaveAttackSparks(pos1.x, pos1.y, pos1.z, 128, 64, 0, 128);
						pos1.y -= 64;
						TriggerShockwave((PHD_3DPOS*)&pos1, 16, 160, 64, 0, 64, 128, 48, 0, 1);

						//wc - timestop effect on Lara
						if (AI.distance < SQUARE(1024 + CLICK(4)) && FreezeTimer <= 0)
						{
							LaraItem->AfterDeath = -127;
							FreezeTimer = 127;
							Weather.Flash(2, 37, 179, 0.06f);
						}
					}

					deltaFrame = item->Animation.FrameNumber - g_Level.Anims[item->Animation.AnimNumber].frameBase;
					int deltaFrame2 = g_Level.Anims[item->Animation.AnimNumber].frameEnd - item->Animation.FrameNumber;

					if (deltaFrame2 >= 16)
					{
						if (deltaFrame > 16)
							deltaFrame = 16;
						TriggerRomanStatueAttackEffect1(itemNumber, deltaFrame);
					}
					else
						TriggerRomanStatueAttackEffect1(itemNumber, deltaFrame2);
				}
			}

			break;

		case STATUE_STATE_WALK:
		{
			creature->Flags = 0;
			joint2 = AI.angle;

			if (creature->Mood == MoodType::Attack)
				creature->MaxTurn = ANGLE(7.0f);
			else
			{
				creature->MaxTurn = 0;
				if (abs(AI.angle) >= ANGLE(2.0f))
				{
					if (AI.angle > 0)
						item->Pose.Orientation.y += ANGLE(2.0f);
					else
						item->Pose.Orientation.y -= ANGLE(2.0f);
				}
				else
					item->Pose.Orientation.y += AI.angle;
			}

			if (AI.distance < pow(SECTOR(1), 2))
			{
				item->Animation.TargetState = STATUE_STATE_IDLE;
				break;
			}

			if (AI.bite && AI.distance < pow(SECTOR(1.75f), 2))
			{
				item->Animation.TargetState = 9;
				break;
			}

			if (item->TriggerFlags == 1)
			{
				if (Targetable(item, &AI) && !(GetRandomControl() & 3))
				{
					item->Animation.TargetState = STATUE_STATE_IDLE;
					break;
				}
			}
			bool flipped = FlipStats[g_Level.Rooms[item->ItemFlags[4]].flipNumber] == 1;
			if (!flipped)
			{
				if (item->TriggerFlags || AI.distance >= pow(SECTOR(2.5f), 2))
					item->Animation.TargetState = STATUE_STATE_WALK;
				else
					item->Animation.TargetState = STATUE_STATE_IDLE;
			}
		}
		break;

		case STATUE_STATE_TURN_180: 
			creature->MaxTurn = 0;
			creature->Flags = 0;

			if (AI.angle > 0)
				item->Pose.Orientation.y -= ANGLE(2.0f);
			else
				item->Pose.Orientation.y += ANGLE(2.0f);

			if (item->Animation.FrameNumber == g_Level.Anims[item->Animation.AnimNumber].frameEnd)
				item->Pose.Orientation.y += -ANGLE(180.0f);
		
			break;

		case STATUE_STATE_ENERGY_ATTACK:
		{
			creature->MaxTurn = 0;
			creature->Flags = 0;

			if (RomanStatueData.Count)
			{
				RomanStatueData.Count--;
				color = (RomanStatueData.Count * ((GetRandomControl() & 0x3F) + 128)) / 16;
				TriggerDynamicLight(RomanStatueData.Position.x, RomanStatueData.Position.y, RomanStatueData.Position.z, 16, 0, color, color / 2);
			}

			deltaFrame = item->Animation.FrameNumber - g_Level.Anims[item->Animation.AnimNumber].frameBase;

			if (deltaFrame == 34)
			{
				pos1 = Vector3Int(-48, 48, SECTOR(1));
				GetJointAbsPosition(item, &pos1, 14);

				pos2 = Vector3Int(-48, 48, 450);
				GetJointAbsPosition(item, &pos2, 14);

				auto angles = GetVectorAngles(pos1.x - pos2.x, pos1.y - pos2.y, pos1.z - pos2.z);
				auto attackPos = PHD_3DPOS(pos2, angles);

				short RoomNumber = item->RoomNumber;
				GetFloor(pos2.x, pos2.y, pos2.z, &RoomNumber);

				RomanStatueAttack(&attackPos, RoomNumber, 1);

				TriggerRomanStatueShockwaveAttackSparks(
					attackPos.Position.x,
					attackPos.Position.y,
					attackPos.Position.z,
					0,
					(((GetRandomControl() & 0x3F) + 128) / 2),
					(((GetRandomControl() & 0x3F) + 128)),
					64);

				RomanStatueData.Count = 16;
				RomanStatueData.Position.x = attackPos.Position.x;
				RomanStatueData.Position.y = attackPos.Position.y;
				RomanStatueData.Position.z = attackPos.Position.z;
				if (item->ItemFlags[0])
					item->ItemFlags[0]--;
			}
			else if (deltaFrame < 10 || deltaFrame > 49)
				break;

			deltaFrame -= 10;
			if (deltaFrame < 32)
			{
				pos1 = { -32, 48, 64 };
				GetJointAbsPosition(item, &pos1, 14);

				pos2 = { -48, 48, 490 };
				GetJointAbsPosition(item, &pos2, 14);

				for (int i = 0; i < 4; i++)
				{
					byte r = (deltaFrame * ((GetRandomControl() & 0x3F) + 128)) / 32;
					byte g = (deltaFrame * ((GetRandomControl() & 0x3F) + 128)) / 16;
					byte b = (deltaFrame * ((GetRandomControl() & 0x3F) + 128)) / 32;

					if (i == 0)
					{
						TriggerDynamicLight(
							pos2.x,
							pos2.y,
							pos2.z,
							8,
							0,
							(deltaFrame * ((GetRandomControl() & 0x3F) + 128)) / 32,
							(deltaFrame * ((GetRandomControl() & 0x3F) + 128)) / 64);
					}

					auto* arc = RomanStatueData.EnergyArcs[i];

					if (arc && deltaFrame && deltaFrame != 24)
					{
						if (deltaFrame < 16)
						{
							arc->life = 56;
							arc->r = 0;
							arc->g = g;
							arc->b = b;
						}

						arc->pos1.x = pos1.x;
						arc->pos1.y = pos1.y;
						arc->pos1.z = pos1.z;
						arc->pos4.x = pos2.x;
						arc->pos4.y = pos2.y;
						arc->pos4.z = pos2.z;
					}
					else if (deltaFrame >= 16)
					{
						if (deltaFrame == 24)
						{
							/*TriggerEnergyArc(&pos1, &pos2, 0, ((GetRandomControl() & 0x3F) + 128),
								(((GetRandomControl() & 0x3F) + 128) / 2), 256, 32, 32, ENERGY_ARC_NO_RANDOMIZE,
								ENERGY_ARC_STRAIGHT_LINE);*/
						}
					}
					else
					{
						/*TriggerEnergyArc(&pos1, &pos2, 0, g, b, 256, 24, 32, ENERGY_ARC_NO_RANDOMIZE,
							ENERGY_ARC_STRAIGHT_LINE);*/

							/*RomanStatueData.energyArcs[i] = TriggerEnergyArc(
								&pos1,
								&pos2,
								(GetRandomControl() & 7) + 8,
								cb | ((cg | 0x180000) << 8),
								12,
								64,
								4);*/
					}
				}
			}
		}
		break;
		}
		if (item->Animation.AnimNumber == Objects[item->ObjectNumber].animIndex + 18 || item->Animation.AnimNumber == Objects[item->ObjectNumber].animIndex + 17)
		{
			item->Pose.Orientation.y = ANGLE(180);
			if (item->Animation.AnimNumber == Objects[item->ObjectNumber].animIndex + 18 && item->Animation.FrameNumber == g_Level.Anims[item->Animation.AnimNumber].frameEnd)
			{
				//wc - boss jumps up a level and switches to fire mode
				ItemInfo* teleportItem = &g_Level.Items[teleportObject[1]];
				item->Pose.Position.y = teleportItem->Pose.Position.y;
				item->Pose.Position.x = teleportItem->Pose.Position.x;
				item->Pose.Position.z = teleportItem->Pose.Position.z;
				ItemNewRoom(itemNumber, teleportItem->RoomNumber);
				item->ItemFlags[5] = 0;
			}
		}
	}
	else
	{
		item->HitPoints = 0;

		if (item->Animation.ActiveState == STATUE_STATE_DEATH)
		{
			if (item->Animation.FrameNumber > g_Level.Anims[item->Animation.AnimNumber].frameBase + 54 &&
				item->Animation.FrameNumber < g_Level.Anims[item->Animation.AnimNumber].frameBase + 74 &&
				item->TouchBits)
			{
				DoDamage(creature->Enemy, 40);
			}
			else if (item->Animation.FrameNumber == g_Level.Anims[item->Animation.AnimNumber].frameEnd)
			{
				// Activate trigger on death
				short RoomNumber = item->ItemFlags[2] & 0xFF;
				short floorHeight = item->ItemFlags[2] & 0xFF00;
				auto* room = &g_Level.Rooms[RoomNumber];

				int x = room->x + (creature->Tosspad / 256 & 0xFF) * SECTOR(1) + 512;
				int y = room->minfloor + floorHeight;
				int z = room->z + (creature->Tosspad & 0xFF) * SECTOR(1) + 512;

				TestTriggers(x, y, z, RoomNumber, true);
			}
		}
		else
		{
			item->Animation.AnimNumber = Objects[item->ObjectNumber].animIndex + STATUE_ANIM_DEATH;
			item->Animation.ActiveState = STATUE_STATE_DEATH;
			item->Animation.FrameNumber = g_Level.Anims[item->Animation.AnimNumber].frameBase;
		}
	}

	CreatureTilt(item, 0);
	CreatureJoint(item, 0, joint0);
	CreatureJoint(item, 1, joint1);
	CreatureJoint(item, 2, joint2);

	if (item->MeshSwapBits & 0x400)
	{
		Vector3Int pos;
		pos.x = (GetRandomControl() & 0x1F) - 16;
		pos.y = 86;
		pos.z = (GetRandomControl() & 0x1F) - 16;
		RomanStatueHitEffect(item, &pos, 10);
	}

	if (item->MeshSwapBits & 0x10)
	{
		Vector3Int pos;
		pos.x = -40;
		pos.y = (GetRandomControl() & 0x7F) + 148;
		pos.z = (GetRandomControl() & 0x3F) - 32;
		RomanStatueHitEffect(item, &pos, 4);
	}

	if (item->MeshSwapBits & 0x100)
	{
		Vector3Int pos;
		pos.x = (GetRandomControl() & 0x3F) + 54;
		pos.y = -170;
		pos.z = (GetRandomControl() & 0x1F) + 27;
		RomanStatueHitEffect(item, &pos, 8);
	}

	CreatureAnimation(itemNumber, angle, 0);
}
