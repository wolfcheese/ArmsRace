#include "framework.h"
#include "tr5_twoblockplatform.h"
#include "Specific/level.h"
#include "Game/control/control.h"
#include "Game/items.h"
#include "Game/Lara/lara.h"
#include "Sound/sound.h"
#include "Game/collision/collide_room.h"
#include "Game/collision/floordata.h"
#include "Renderer/Renderer11.h"
using namespace TEN::Renderer;

using namespace TEN::Floordata;

void InitialiseTwoBlocksPlatform(short itemNumber)
{
	auto* item = &g_Level.Items[itemNumber];

	item->ItemFlags[0] = item->Pose.Position.y;
	item->ItemFlags[1] = 1;
	item->ItemFlags[6] = NO_ITEM;
	UpdateBridgeItem(itemNumber);

	//wc - collect skelly in same room
	for (int i = 0; i < g_Level.NumItems; i++)
	{
		ItemInfo* item2 = &g_Level.Items[i];
		if (item2->ObjectNumber == ID_SKELETON && item->RoomNumber == item2->RoomNumber)
		{
			item->ItemFlags[6] = i;
			break;
		}
	}
}

void TwoBlocksPlatformControl(short itemNumber)
{
	auto* item = &g_Level.Items[itemNumber];

	if (TriggerActive(item))
	{
		if (item->TriggerFlags)
		{
			if (item->Pose.Position.y > (item->ItemFlags[0] - 16 * (int) (item->TriggerFlags & 0xFFFFFFF0)))
				item->Pose.Position.y -= item->TriggerFlags & 0xF;

			auto probe = GetCollision(item);

			item->Floor = probe.Position.Floor;

			if (probe.RoomNumber != item->RoomNumber)
			{
				UpdateBridgeItem(itemNumber, true);
				ItemNewRoom(itemNumber, probe.RoomNumber);
				UpdateBridgeItem(itemNumber);
			}
		}
		else
		{
			bool onObject = false;
			bool laraOnObject = false;

			int height = LaraItem->Pose.Position.y + 1;
			if (GetBridgeItemIntersect(itemNumber, LaraItem->Pose.Position.x, LaraItem->Pose.Position.y, LaraItem->Pose.Position.z, false).has_value())
			{
				if (LaraItem->Pose.Position.y <= item->Pose.Position.y + 32)
				{
					if (item->Pose.Position.y < height)
						onObject = true;
						laraOnObject = true;
				}
			}

			//wc - skelly can lower the platform too
			if (!onObject && item->ItemFlags[6] != NO_ITEM)
			{
				ItemInfo* skellyItem = &g_Level.Items[item->ItemFlags[6]];
				if (GetBridgeItemIntersect(itemNumber, skellyItem->Pose.Position.x, item->Pose.Position.y, skellyItem->Pose.Position.z, false).has_value())
				{
					onObject = true;
				}
			}

			if (onObject)
				item->ItemFlags[1] = 1;
			else
				item->ItemFlags[1] = -1;

			//wc - platform acts as switch
			if(!laraOnObject)
				TestTriggers(item, false);

			if (item->ItemFlags[1] < 0)
			{
				if (item->Pose.Position.y <= item->ItemFlags[0])
					item->ItemFlags[1] = 1;
				else
				{
					SoundEffect(SFX_TR4_RUMBLE_NEXTDOOR, &item->Pose);
					item->Pose.Position.y -= 4;
				}
			}
			else if (item->ItemFlags[1] > 0)
			{
				if (item->Pose.Position.y >= item->ItemFlags[0] + 128)
					item->ItemFlags[1] = -1;
				else
				{
					SoundEffect(SFX_TR4_RUMBLE_NEXTDOOR, &item->Pose);
					item->Pose.Position.y += 4;
				}
			}
		}
	}
}

std::optional<int> TwoBlocksPlatformFloor(short itemNumber, int x, int y, int z)
{
	auto* item = &g_Level.Items[itemNumber];

	if (!item->MeshBits)
		return std::nullopt;

	return GetBridgeItemIntersect(itemNumber, x, y, z, false);
}

std::optional<int> TwoBlocksPlatformCeiling(short itemNumber, int x, int y, int z)
{
	auto* item = &g_Level.Items[itemNumber];

	if (!item->MeshBits)
		return std::nullopt;

	return GetBridgeItemIntersect(itemNumber, x, y, z, true);
}

int TwoBlocksPlatformFloorBorder(short itemNumber)
{
	return GetBridgeBorder(itemNumber, false);
}

int TwoBlocksPlatformCeilingBorder(short itemNumber)
{
	return GetBridgeBorder(itemNumber, true);
}
