#include "framework.h"
#include "Objects/TR2/Entity/tr2_worker_flamethrower.h"

#include "Game/animation.h"
#include "Game/camera.h"
#include "Game/control/box.h"
#include "Game/control/control.h"
#include "Game/effects/effects.h"
#include "Game/effects/tomb4fx.h"
#include "Game/items.h"
#include "Game/itemdata/creature_info.h"
#include "Game/missile.h"
#include "Game/misc.h"
#include "Game/people.h"
#include "Specific/setup.h"
#include "Specific/level.h"
#include "Specific/trmath.h"
#include <Game/health.h>

BITE_INFO WorkerFlamethrowerBite = { 0, 250, 32, 9 };
Vector3Int WorkerFlamethrowerOffset = { 0, 140, 0 };

// TODO
enum WorkerFlamethrowerState
{

};

// TODO
enum WorkerFlamethrowerAnim
{

};

static void Flame(int x, int y, int z, int velocity, short yRot, short roomNumber)
{
	short fxNumber = CreateNewEffect(roomNumber);
	if (fxNumber != NO_ITEM)
	{
		auto* fx = &EffectList[fxNumber];

		fx->pos.Position.x = x;
		fx->pos.Position.y = y;
		fx->pos.Position.z = z;
		fx->roomNumber = roomNumber;
		//TODO: complete fx parameters
		fx->shade = 14 * 256;
		fx->counter = 40;
		ShootAtLara(fx);
	}
}

static void GetTargetPosition(ItemInfo* item, PHD_3DPOS* target)
{
	CreatureInfo* creature = (CreatureInfo*)item->Data;
	Vector3Int start, end;
	start.x = 0;
	start.y = -96;
	start.z = 144;
	GetJointAbsPosition(item, &start, 9);
	end.x = creature->Enemy->Pose.Position.x;
	end.y = creature->Enemy->Pose.Position.y;
	end.z = creature->Enemy->Pose.Position.z;
	Vector3Shrt angles = GetVectorAngles(end.x - start.x, end.y - start.y, end.z - start.z);
	target->Position = start;
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
		fx->speed = (GetRandomControl() & 0x1F) + 128;
		fx->flag1 = 6;
	}
}

void InitialiseWorkerFlamethrower(short itemNumber)
{
	auto* item = &g_Level.Items[itemNumber];

	item->Animation.AnimNumber = Objects[item->ObjectNumber].animIndex + 12;

	ClearItem(itemNumber);

	auto* anim = &g_Level.Anims[item->Animation.AnimNumber];
	item->Animation.FrameNumber = anim->frameBase;
	item->Animation.ActiveState = anim->ActiveState;
}

void WorkerFlamethrower(short itemNumber)
{
	if (!CreatureActive(itemNumber))
		return;

	Vector3Int pos;

	auto* item = &g_Level.Items[itemNumber];
	auto* creature = GetCreatureInfo(item);

	short tilt = 0;
	short angle = 0;
	short headX = 0;
	short headY = 0;
	short torsoX = 0;
	short torsoY = 0;

	pos.x = WorkerFlamethrowerBite.x;
	pos.y = WorkerFlamethrowerBite.y;
	pos.z = WorkerFlamethrowerBite.z;
	GetJointAbsPosition(item, &pos, WorkerFlamethrowerBite.meshNum);

	if (item->HitPoints <= 0)
	{
		if (item->Animation.ActiveState != 7)
		{
			item->Animation.AnimNumber = Objects[item->ObjectNumber].animIndex + 19;
			item->Animation.FrameNumber = g_Level.Anims[item->Animation.AnimNumber].frameBase;
			item->Animation.ActiveState = 7;
		}
	}
	else
	{
		
		if (BaddyHealthItemNumber <= 0)
			BaddyHealthItemNumber = itemNumber;
		if (item->Animation.ActiveState != 5 && item->Animation.ActiveState != 6)
		{
			TriggerDynamicLight(pos.x, pos.y, pos.z, (GetRandomControl() & 4) + 10, (GetRandomControl() & 7) + 128, (GetRandomControl() & 7) + 64, GetRandomControl() & 7);
			TriggerPilotFlame(itemNumber, WorkerFlamethrowerBite.meshNum);
		}
		else
		{
			TriggerDynamicLight(pos.x, pos.y, pos.z, (GetRandomControl() & 4) + 14, (GetRandomControl() & 7) + 128, (GetRandomControl() & 7) + 64, GetRandomControl() & 7);
			ThrowFire(itemNumber, WorkerFlamethrowerBite.meshNum, WorkerFlamethrowerOffset, WorkerFlamethrowerOffset);
		}
			
		AI_INFO AI;
		CreatureAIInfo(item, &AI);

		GetCreatureMood(item, &AI, VIOLENT);
		CreatureMood(item, &AI, VIOLENT);

		angle = CreatureTurn(item, creature->MaxTurn);

		switch (item->Animation.ActiveState)
		{
		case 1:
			creature->MaxTurn = 0;
			creature->Flags = 0;

			if (AI.ahead)
			{
				headX = AI.xAngle;
				headY = AI.angle;
			}

			if (creature->Mood == MoodType::Escape)
				item->Animation.TargetState = 3;
			else if (Targetable(item, &AI))
			{
				if (AI.distance < pow(SECTOR(4), 2) || AI.zoneNumber != AI.enemyZone)
					item->Animation.TargetState = 8;
				else
					item->Animation.TargetState = 2;
			}
			else if (creature->Mood == MoodType::Attack || !AI.ahead)
			{
				if (AI.distance <= pow(SECTOR(2), 2))
					item->Animation.TargetState = 2;
				else
					item->Animation.TargetState = 3;
			}
			else
				item->Animation.TargetState = 4;
			
			break;

		case 2:
			creature->MaxTurn = ANGLE(5.0f);

			if (AI.ahead)
			{
				headX = AI.xAngle;
				headY = AI.angle;
			}

			if (creature->Mood == MoodType::Escape)
				item->Animation.TargetState = 3;
			else if (Targetable(item, &AI))
			{
				if (AI.distance < pow(SECTOR(4), 2) || AI.zoneNumber != AI.enemyZone)
					item->Animation.TargetState = 1;
				else
					item->Animation.TargetState = 6;
			}
			else if (creature->Mood == MoodType::Attack || !AI.ahead)
			{
				if (AI.distance > pow(SECTOR(2), 2))
					item->Animation.TargetState = 3;
			}
			else
				item->Animation.TargetState = 4;
			
			break;

		case 3:
			creature->MaxTurn = ANGLE(10.0f);

			if (AI.ahead)
			{
				headX = AI.xAngle;
				headY = AI.angle;
			}

			if (creature->Mood != MoodType::Escape)
			{
				if (Targetable(item, &AI))
					item->Animation.TargetState = 2;
				else if (creature->Mood == MoodType::Bored || creature->Mood == MoodType::Stalk)
					item->Animation.TargetState = 2;
			}

			break;

		case 4:
			if (AI.ahead)
			{
				headX = AI.xAngle;
				headY = AI.angle;
			}

			if (Targetable(item, &AI))
				item->Animation.TargetState = 5;
			else
			{
				if (creature->Mood == MoodType::Attack)
					item->Animation.TargetState = 1;
				else if (!AI.ahead)
					item->Animation.TargetState = 1;
			}
			
			break;

		case 5:
		case 6:
			if (AI.ahead)
			{
				torsoX = AI.xAngle;
				torsoY = AI.angle;
			}

			if (item->Animation.TargetState != 1 &&
				(creature->Mood == MoodType::Escape || AI.distance > pow(SECTOR(10), 2) || !Targetable(item, &AI)))
			{
				item->Animation.TargetState = 1;
			}

			break;

		case 8:
		case 9:
			creature->Flags = 0;

			if (AI.ahead)
			{
				torsoX = AI.xAngle;
				torsoY = AI.angle;
			}

			if (Targetable(item, &AI))
				item->Animation.TargetState = (item->Animation.ActiveState == 8) ? 5 : 11;
			else
				item->Animation.TargetState = 1;
			
			break;
		}
	}

	CreatureTilt(item, tilt);
	CreatureJoint(item, 0, torsoY);
	CreatureJoint(item, 1, torsoX);
	CreatureJoint(item, 2, headY);
	CreatureJoint(item, 3, headX);
	CreatureAnimation(itemNumber, angle, tilt);
}
