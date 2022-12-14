#include "framework.h"
#include "Game/people.h"

#include "Game/animation.h"
#include "Game/control/los.h"
#include "Game/collision/sphere.h"
#include "Game/effects/effects.h"
#include "Game/effects/debris.h"
#include "Game/itemdata/creature_info.h"
#include "Game/items.h"
#include "Game/Lara/lara.h"
#include "Game/misc.h"
#include <Game/health.h>
#include "Sound/sound.h"
#include <weather.h>
using namespace TEN::Effects::Environment;

bool ShotLara(ItemInfo* item, AI_INFO* AI, BITE_INFO* gun, short extraRotation, int damage) 
{
	auto* creature = GetCreatureInfo(item);
	auto* enemy = creature->Enemy;

	bool hit = false;
	bool targetable = false;

	if (AI->distance <= pow(MAX_VISIBILITY_DISTANCE, 2) && Targetable(item, AI))
	{
		int distance = phd_sin(AI->enemyFacing) * enemy->Animation.Velocity * pow(MAX_VISIBILITY_DISTANCE, 2) / 300;
		distance = pow(distance, 2) + AI->distance;
		if (distance <= pow(MAX_VISIBILITY_DISTANCE, 2))
		{
			int random = (pow(MAX_VISIBILITY_DISTANCE, 2) - AI->distance) / (pow(MAX_VISIBILITY_DISTANCE, 2) / 0x5000) + 8192;
			hit = GetRandomControl() < random;
		}
		else
			hit = false;
		
		targetable = true;
	}
	else
	{
		hit = false;
		targetable = false;
	}

	if (damage)
	{
		if (enemy == LaraItem)
		{
			if (hit)
			{
				CreatureEffect(item, gun, &GunHit);
				DoDamage(LaraItem, damage);
				//wc - freeze enemy freezes Lara
				if (item->ObjectNumber == ID_BADDY1 && item->TriggerFlags == 999 && FreezeTimer <= 0 && item->ItemFlags[7] <= 0)
				{
					LaraItem->AfterDeath = -127;
					FreezeTimer = 127;
					item->ItemFlags[7] = 127 * 2;
					Weather.Flash(2, 37, 179, 0.06f);
				}
			}
			else if (targetable)
				CreatureEffect(item, gun, &GunMiss);
		}
		else
		{
			CreatureEffect(item, gun, &GunShot);
			if (hit)
			{
				//wc - telporting sas enemy
				if (enemy->ObjectNumber == ID_LARA_DOUBLE && enemy->TriggerFlags == 3)
				{
					item->Pose.Position.y = item->StartPose.Position.y = enemy->Pose.Position.y;
					item->Pose.Position.x = item->StartPose.Position.x = enemy->Pose.Position.x;
					item->Pose.Position.z = item->StartPose.Position.z = enemy->Pose.Position.z;
					item->Location.roomNumber = enemy->RoomNumber;
					item->Location.yNumber = enemy->Pose.Position.y;
					item->ItemFlags[7] = 0;
					Weather.Flash(153, 7, 186, 0.06f);
					creature->Enemy = LaraItem;
				}
				else
				{
					enemy->HitStatus = true;
					enemy->HitPoints += damage / -10;

					int random = GetRandomControl() & 0xF;
					if (random > 14)
						random = 0;

				Vector3Int pos = { 0, 0, 0 };
					GetJointAbsPosition(enemy, &pos, random);
				DoBloodSplat(pos.x, pos.y, pos.z, (GetRandomControl() & 3) + 4, enemy->Pose.Orientation.y, enemy->RoomNumber);
				}
			}
		}
	}

	// TODO: smash objects

	return targetable;
}

short GunMiss(int x, int y, int z, short velocity, short yRot, short roomNumber)
{
	GameVector pos;
	pos.x = LaraItem->Pose.Position.x + ((GetRandomControl() - 0x4000) << 9) / 0x7FFF;
	pos.y = LaraItem->Floor;
	pos.z = LaraItem->Pose.Position.z + ((GetRandomControl() - 0x4000) << 9) / 0x7FFF;
	pos.roomNumber = LaraItem->RoomNumber;

	Richochet((PHD_3DPOS*)&pos);

	return GunShot(x, y, z, velocity, yRot, roomNumber);
}

short GunHit(int x, int y, int z, short velocity, short yRot, short roomNumber)
{
	Vector3Int pos = { 0, 0, 0 };
	GetLaraJointPosition(&pos, (25 * GetRandomControl()) >> 15);
	DoBloodSplat(pos.x, pos.y, pos.z, (GetRandomControl() & 3) + 3, LaraItem->Pose.Orientation.y, LaraItem->RoomNumber);
	return GunShot(x, y, z, velocity, yRot, roomNumber);
}

short GunShot(int x, int y, int z, short velocity, short yRot, short roomNumber)
{
	return -1;
}

bool Targetable(ItemInfo* item, AI_INFO* AI) 
{
	auto* creature = GetCreatureInfo(item);
	auto* enemy = creature->Enemy;

	if (enemy == NULL || enemy->HitPoints <= 0 || !AI->ahead || AI->distance >= pow(MAX_VISIBILITY_DISTANCE, 2))
		return false;

	if (!enemy->Data.is<CreatureInfo>() && !enemy->IsLara())
		return false;

	auto* bounds = (BOUNDING_BOX*)GetBestFrame(item);

	GameVector start;
	start.x = item->Pose.Position.x;
	start.y = (item->ObjectNumber == ID_SNIPER) ? (item->Pose.Position.y - CLICK(3)) : (item->Pose.Position.y + ((bounds->Y2 + 3 * bounds->Y1) / 4));
	start.z = item->Pose.Position.z;
	start.roomNumber = item->RoomNumber;

	bounds = (BOUNDING_BOX*)GetBestFrame(enemy);

	GameVector target;
	target.x = enemy->Pose.Position.x;
	target.y = enemy->Pose.Position.y + ((bounds->Y2 + 3 * bounds->Y1) / 4);
	target.z = enemy->Pose.Position.z;

	return LOS(&start, &target);
}

bool TargetVisible(ItemInfo* item, AI_INFO* AI) 
{
	auto* creature = GetCreatureInfo(item);
	auto* enemy = creature->Enemy;

	if (enemy != NULL)
	{
		short angle = AI->angle - creature->JointRotation[2];
		if (enemy->HitPoints != 0 && angle > -ANGLE(45.0f) && angle < ANGLE(45.0f) && AI->distance < pow(MAX_VISIBILITY_DISTANCE, 2))
		{
			GameVector start;
			start.x = item->Pose.Position.x;
			start.y = item->Pose.Position.y - CLICK(3);
			start.z = item->Pose.Position.z;
			start.roomNumber = item->RoomNumber;

			GameVector target;
			target.x = enemy->Pose.Position.x;
			auto* bounds = (BOUNDING_BOX*)GetBestFrame(enemy);
			target.y = enemy->Pose.Position.y + ((((bounds->Y1 * 2) + bounds->Y1) + bounds->Y2) / 4);
			target.z = enemy->Pose.Position.z;

			return LOS(&start, &target);
		}
	}

	return false;
}
