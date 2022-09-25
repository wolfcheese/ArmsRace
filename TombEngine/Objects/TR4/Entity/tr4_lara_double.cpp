#include "framework.h"
#include "tr4_lara_double.h"
#include "Game/items.h"
#include "Specific/level.h"
#include "Sound/sound.h"
#include "Game/control/box.h"
#include "Game/Lara/lara.h"
#include "Game/animation.h"
#include <Game/camera.h>
#include <Specific/input.h>
#include <Game/health.h>
#include "effects/weather.h"
#include <Game/effects/effects.h>
#include <Objects/TR4/Object/tr4_senet.h>
#include "Renderer11.h"
using namespace TEN::Effects::Environment;
using namespace TEN::Renderer;
namespace TEN::Entities::TR4
{
	void InitialiseLaraDouble(short itemNum)
	{
		ClearItem(itemNum);
		ItemInfo* item = &g_Level.Items[itemNum];
		//wc - initial teleporter setup (active but not really "active")
		if (item->TriggerFlags == 3)
		{
			item->TriggerFlags = -3;
		}
	}

	void LaraDoubleControl(short itemNumber)
	{
		auto* item = &g_Level.Items[itemNumber];

		//SoundEffect(SFX_TR4_METAL_SCRAPE_LOOP1, &item->pos, 0);

		if (CreatureActive(itemNumber))
		{
			//render shockwave effects if drawn
			if (item->ItemFlags[4] == 1)
			{
				if (item->TriggerFlags == -3)
				{
					item->ItemFlags[7]++;
					if (item->ItemFlags[7] >= 64)
					{
						SenetPieceExplosionEffect(item, 0xffffff, 4);
						item->ItemFlags[7] = 0;
					}
					//TriggerDynamicLight(item->Pose.Position.x, item->Pose.Position.y, item->Pose.Position.z, 8, 255, 255, 255);
				}
				else
					if (item->TriggerFlags == 3)
					{
						item->ItemFlags[7]++;
						if (item->ItemFlags[7] >= 64)
						{
							SenetPieceExplosionEffect(item, 0x9907ba, 8);
							item->ItemFlags[7] = 0;
						}
						//TriggerDynamicLight(item->Pose.Position.x, item->Pose.Position.y, item->Pose.Position.z, 8, 153, 7, 186);
					}
					else if (item->TriggerFlags == 5)
					{
						item->ItemFlags[7]++;
						if (item->ItemFlags[7] >= 64)
						{
							SenetPieceExplosionEffect(item, 0x1cf218, 2);
							item->ItemFlags[7] = 0;
						}
						//TriggerDynamicLight(item->Pose.Position.x, item->Pose.Position.y, item->Pose.Position.z, 8, 28, 242, 24);
					}
			}
			if (item->HitStatus)
			{
				item->HitPoints = 1000;
				//wc - weapon effects
				int OCB = item->TriggerFlags;
				switch (OCB)
				{
				case 3:
					if (Lara.Control.Weapon.GunType == LaraWeaponType::Revolver)
					{
						LaraItem->Pose.Position.y = LaraItem->StartPose.Position.y = item->Pose.Position.y;
						LaraItem->Pose.Position.x = LaraItem->StartPose.Position.x = item->Pose.Position.x;
						LaraItem->Pose.Position.z = LaraItem->StartPose.Position.z = item->Pose.Position.z;
						ItemNewRoom(FindItem(LaraItem), item->RoomNumber);
						LaraItem->Location.yNumber = LaraItem->Pose.Position.y;
						LaraItem->Location.roomNumber = item->RoomNumber;
						Weather.Flash(153, 7, 186, 0.06f);
					}
					break;
				case 4:
				{
					short group = g_Level.Rooms[item->RoomNumber].flipNumber;
					bool flipped = FlipStats[group] == 1;
					if ((!flipped && Lara.Control.Weapon.GunType == LaraWeaponType::Uzi) || (flipped && Lara.Control.Weapon.GunType == LaraWeaponType::Crossbow))
					{
						DoFlipMap(group);
						if (Lara.Control.Weapon.GunType == LaraWeaponType::Uzi)
							Weather.Flash(7, 215, 247, 0.06f);
						else
							Weather.Flash(242, 161, 22, 0.06f);
					}
					break;
				}
				case 5:
					if (Lara.Control.Weapon.GunType == LaraWeaponType::RocketLauncher)
					{
						AntiGravTimer = 800;
						Weather.Flash(28, 242, 24, 0.06f);
					}
					break;
				}
			}
			AnimateItem(item);
		}
	}
}
