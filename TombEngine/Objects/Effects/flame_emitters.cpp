#include "framework.h"
#include "Objects/Effects/flame_emitters.h"

#include "Game/animation.h"
#include "Game/collision/collide_item.h"
#include "Game/collision/collide_room.h"
#include "Game/collision/sphere.h"
#include "Game/effects/effects.h"
#include "Game/effects/lightning.h"
#include "Game/effects/lara_fx.h"
#include "Game/effects/tomb4fx.h"
#include "Renderer11.h"
#include "Game/effects/weather.h"
#include "Game/items.h"
#include "Game/Lara/lara.h"
#include "Sound/sound.h"
#include "Specific/level.h"
#include "Specific/setup.h"
#include "Specific/trmath.h"
#include "input.h"

using namespace TEN::Input;
using namespace TEN::Effects::Lara;
using namespace TEN::Effects::Lightning;
using namespace TEN::Effects::Environment;
using namespace TEN::Renderer;

namespace TEN::Entities::Effects
{
	byte Flame3xzoffs[16][2] =
	{
		{ 9, 9 },
		{ 24, 9 },
		{ 40, 9	},
		{ 55, 9 },
		{ 9, 24 },
		{ 24, 24 },
		{ 40, 24 },
		{ 55, 24 },
		{ 9, 40 },
		{ 24, 40 },
		{ 40, 40 },
		{ 55, 40 },
		{ 9, 55	 },
		{ 24, 55 },
		{ 40, 55 },
		{ 55, 55 }
	};

	OBJECT_COLLISION_BOUNDS FireBounds =
	{
		0, 0, 
		0, 0, 
		0, 0, 
		-ANGLE(10.0f), ANGLE(10.0f),
		-ANGLE(30.0f), ANGLE(30.0f),
		-ANGLE(10.0f), ANGLE(10.0f)
	};

	bool FlameEmitterFlags[8];

	void FlameEmitterControl(short itemNumber)
	{
		auto* item = &g_Level.Items[itemNumber];

		//wc - added flag check (yeah this is super hacky)
		if (TriggerActive(item) && item->ItemFlags[7] >= 0)
		{
			// Jet flame
			if (item->TriggerFlags < 0)
			{
				short flags = -item->TriggerFlags;
				if ((flags & 7) == 2 || (flags & 7) == 7)
				{
					SoundEffect(SFX_TR4_FLAME_EMITTER, &item->Pose);
					TriggerSuperJetFlame(item, -256 - (3072 * GlobalCounter & 0x1C00), GlobalCounter & 1);
					TriggerDynamicLight(item->Pose.Position.x, item->Pose.Position.y, item->Pose.Position.z,
						(GetRandomControl() & 3) + 20,
						(GetRandomControl() & 0x3F) + 192,
						(GetRandomControl() & 0x1F) + 96, 0);
				}
				else
				{
					if (item->ItemFlags[0])
					{
						if (item->ItemFlags[1])
							item->ItemFlags[1] = item->ItemFlags[1] - (item->ItemFlags[1] >> 2);

						if (item->ItemFlags[2] < 256)
							item->ItemFlags[2] += 8;

						item->ItemFlags[0]--;
						if (item->ItemFlags[0] == 1)
							item->ItemFlags[3] = (GetRandomControl() & 0x3F) + 150;
					}
					else
					{
						if (!--item->ItemFlags[3])
						{
							if (flags >> 3)
								item->ItemFlags[0] = (GetRandomControl() & 0x1F) + 30 * (flags >> 3);
							else
								item->ItemFlags[0] = (GetRandomControl() & 0x3F) + 60;
						}

						if (item->ItemFlags[2])
							item->ItemFlags[2] -= 8;

						if (item->ItemFlags[1] > -8192)
							item->ItemFlags[1] -= 512;
					}

					if (item->ItemFlags[2])
						AddFire(item->Pose.Position.x, item->Pose.Position.y, item->Pose.Position.z, item->RoomNumber, 0.5f, item->ItemFlags[2]);

					if (item->ItemFlags[1])
					{
						SoundEffect(SFX_TR4_FLAME_EMITTER, &item->Pose);

						if (item->ItemFlags[1] <= -8192)
							TriggerSuperJetFlame(item, -256 - (3072 * GlobalCounter & 0x1C00), GlobalCounter & 1);
						else
							TriggerSuperJetFlame(item, item->ItemFlags[1], GlobalCounter & 1);

						TriggerDynamicLight(item->Pose.Position.x, item->Pose.Position.y, item->Pose.Position.z,
							(-item->ItemFlags[1] >> 10) - (GetRandomControl() & 1) + 16,
							(GetRandomControl() & 0x3F) + 192,
							(GetRandomControl() & 0x1F) + 96, 0);
					}
					else
					{
						byte r = (GetRandomControl() & 0x3F) + 192;
						byte g = (GetRandomControl() & 0x1F) + 96;
						byte falloff = 10 - (GetRandomControl() & 1);

						TriggerDynamicLight(item->Pose.Position.x, item->Pose.Position.y, item->Pose.Position.z,
							10 - (GetRandomControl() & 1),
							(GetRandomControl() & 0x3F) + 192,
							(GetRandomControl() & 0x1F) + 96, 0);
					}
				}

				SoundEffect(SFX_TR4_LOOP_FOR_SMALL_FIRES, &item->Pose);
			}
			else
			{
				if (item->TriggerFlags < 8)
					FlameEmitterFlags[item->TriggerFlags] = true;

				AddFire(item->Pose.Position.x, item->Pose.Position.y, item->Pose.Position.z, item->RoomNumber, 1.0f, 0);

				TriggerDynamicLight(item->Pose.Position.x, item->Pose.Position.y, item->Pose.Position.z,
					16 - (GetRandomControl() & 1),
					(GetRandomControl() & 0x3F) + 192,
					(GetRandomControl() & 0x1F) + 96, 0);

				SoundEffect(SFX_TR4_LOOP_FOR_SMALL_FIRES, &item->Pose);

				if (!Lara.Burn &&
					ItemNearLara(&item->Pose, 600) &&
					(pow(LaraItem->Pose.Position.x - item->Pose.Position.x, 2) +
						pow(LaraItem->Pose.Position.z - item->Pose.Position.z, 2) < pow(SECTOR(0.5f), 2)) &&
					Lara.Control.WaterStatus != WaterStatus::FlyCheat)
				{
					LaraBurn(LaraItem);
				}
			}
		}
		else
		{
			if (item->TriggerFlags > 0 && item->TriggerFlags < 8)
				FlameEmitterFlags[item->TriggerFlags] = false;
			item->ItemFlags[7] = -1;
			if (!TriggerActive(item))
				AddActiveItem(itemNumber);
		}
	}

	void FlameEmitter2Control(short itemNumber)
	{
		auto* item = &g_Level.Items[itemNumber];

		//wc - added flag check (yeah this is super hacky)
		if (TriggerActive(item) && item->ItemFlags[7] >= 0)
		{
			// If not an emitter for flipmaps
			if (item->TriggerFlags >= 0)
			{
				// If not a moving flame
				if (item->TriggerFlags != 2)
				{
					if (item->TriggerFlags == 123)
					{
						// Middle of the block
						AddFire(item->Pose.Position.x, item->Pose.Position.y, item->Pose.Position.z, item->RoomNumber, 0.25f, item->ItemFlags[3]);
					}
					else
					{
						float size = 1.0f;
						switch (item->TriggerFlags)
						{
						default:
						case 0:
							size = 2.0f;
							break;

						case 1:
							size = 1.0f;
							break;

						case 3:
							size = 0.5f;
							break;
						}

						AddFire(item->Pose.Position.x, item->Pose.Position.y, item->Pose.Position.z, item->RoomNumber, size, item->ItemFlags[3]);
					}
				}

				if (item->TriggerFlags == 0 || item->TriggerFlags == 2)
				{
					if (item->ItemFlags[3])
					{
						TriggerDynamicLight(item->Pose.Position.x, item->Pose.Position.y, item->Pose.Position.z,
							10,
							((GetRandomControl() & 0x3F) + 192) * item->ItemFlags[3] >> 8,
							(GetRandomControl() & 0x1F) + 96 * item->ItemFlags[3] >> 8,
							0);
					}
					else
					{
						TriggerDynamicLight(item->Pose.Position.x, item->Pose.Position.y, item->Pose.Position.z,
							10,
							(GetRandomControl() & 0x3F) + 192,
							(GetRandomControl() & 0x1F) + 96,
							0);
					}
				}

				if (item->TriggerFlags == 2)
				{
					item->Pose.Position.x += phd_sin(item->Pose.Orientation.y - ANGLE(180)) * (CLICK(1) / FPS);
					item->Pose.Position.z += phd_cos(item->Pose.Orientation.y - ANGLE(180)) * (CLICK(1) / FPS);

					auto probe = GetCollision(item);
					
					if (TestEnvironment(ENV_FLAG_WATER, probe.RoomNumber) ||
						probe.Position.Floor - item->Pose.Position.y > CLICK(2) ||
						probe.Position.Floor == NO_HEIGHT)
					{
						Weather.Flash(255, 128, 0, 0.03f);
						KillItem(itemNumber);
						return;
					}

					if (item->RoomNumber != probe.RoomNumber)
						ItemNewRoom(itemNumber, probe.RoomNumber);

					item->Pose.Position.y = probe.Position.Floor;

					if (Wibble & 7)
						TriggerFireFlame(item->Pose.Position.x, item->Pose.Position.y - 32, item->Pose.Position.z, -1, 1);
				}

				SoundEffect(SFX_TR4_LOOP_FOR_SMALL_FIRES, &item->Pose);
			}
			else if (item->ItemFlags[0] == 0)
			{
				DoFlipMap(-item->TriggerFlags);
				FlipMap[-item->TriggerFlags] ^= 0x3E00u;
				item->ItemFlags[0] = 1;
			}
		}
		else
		{
			item->ItemFlags[7] = -1;
			if (!TriggerActive(item))
				AddActiveItem(itemNumber);
		}
	}

	void FlameControl(short fxNumber)
	{
		auto* fx = &EffectList[fxNumber];

		for (int i = 0; i < 14; i++)
		{
			if (!(Wibble & 0xC))
			{
				fx->pos.Position.x = 0;
				fx->pos.Position.y = 0;
				fx->pos.Position.z = 0;

				GetLaraJointPosition((Vector3Int*)&fx->pos, i);

				// TR5 code?
				if (Lara.BurnCount)
				{
					Lara.BurnCount--;
					if (!Lara.BurnCount)
						Lara.BurnSmoke = true;
				}

				TriggerFireFlame(fx->pos.Position.x, fx->pos.Position.y, fx->pos.Position.z, -1, 255 - Lara.BurnSmoke);
			}
		}

		byte r = (GetRandomControl() & 0x3F) + 192;
		byte g = (GetRandomControl() & 0x1F) + 96;

		auto pos = Vector3Int();
		GetLaraJointPosition(&pos, LM_HIPS);

		if (!Lara.BurnSmoke)
		{
			if (Lara.BurnBlue == 0)
			{
				TriggerDynamicLight(
					pos.x,
					pos.y,
					pos.z,
					13,
					(GetRandomControl() & 0x3F) + 192,
					(GetRandomControl() & 0x1F) + 96,
					0);
			}
			else if (Lara.BurnBlue == 1)
			{
				TriggerDynamicLight(
					pos.x, 
					pos.y, 
					pos.z, 
					13, 
					0, 
					(GetRandomControl() & 0x1F) + 96,
					(GetRandomControl() & 0x3F) + 192);
			}
			else if (Lara.BurnBlue == 2)
			{
				TriggerDynamicLight(
					pos.x,
					pos.y,
					pos.z,
					13,
					0,
					(GetRandomControl() & 0x3F) + 192,
					(GetRandomControl() & 0x1F) + 96);
			}
		}
		else
		{
			TriggerDynamicLight(
				pos.x,
				pos.y,
				pos.z, 
				13,
				GetRandomControl() & 0x3F,
				(GetRandomControl() & 0x3F) + 192,
				(GetRandomControl() & 0x1F) + 96);
		}

		if (LaraItem->RoomNumber != fx->roomNumber)
			EffectNewRoom(fxNumber, LaraItem->RoomNumber);

		int waterHeight = GetWaterHeight(fx->pos.Position.x, fx->pos.Position.y, fx->pos.Position.z, fx->roomNumber);
		if (waterHeight == NO_HEIGHT || fx->pos.Position.y <= waterHeight || Lara.BurnBlue)
		{
			SoundEffect(SFX_TR4_LOOP_FOR_SMALL_FIRES, &fx->pos);
			DoDamage(LaraItem, 7);
		}
		else
		{
			KillEffect(fxNumber);
			Lara.Burn = false;
		}

		if (Lara.Control.WaterStatus == WaterStatus::FlyCheat)
		{
			KillEffect(fxNumber);
			Lara.Burn = false;
		}
	}

	void InitialiseFlameEmitter(short itemNumber)
	{
		auto* item = &g_Level.Items[itemNumber];

		if (item->TriggerFlags < 0)
		{
			item->ItemFlags[0] = (GetRandomControl() & 0x3F) + 90;
			item->ItemFlags[2] = 256;

			if (((-item->TriggerFlags) & 7) == 7)
			{
				switch (item->Pose.Orientation.y)
				{
				case 0:
					item->Pose.Position.z += 512;
					break;

				case 0x4000:
					item->Pose.Position.x += 512;
					break;

				case -0x8000:
					item->Pose.Position.z -= 512;
					break;

				case -0x4000:
					item->Pose.Position.x -= 512;
					break;
				}
			}
		}
	}

	void InitialiseFlameEmitter2(short itemNumber)
	{
		auto* item = &g_Level.Items[itemNumber];

		item->Pose.Position.y -= 64;

		if (item->TriggerFlags != 123)
		{
			switch (item->Pose.Orientation.y)
			{
			case 0:
				if (item->TriggerFlags == 2)
					item->Pose.Position.z += 80;
				else
					item->Pose.Position.z += 256;

				break;

			case 0x4000:
				if (item->TriggerFlags == 2)
					item->Pose.Position.x += 80;
				else
					item->Pose.Position.x += 256;

				break;

			case -0x8000:
				if (item->TriggerFlags == 2)
					item->Pose.Position.z -= 80;
				else
					item->Pose.Position.z -= 256;

				break;

			case -0x4000:
				if (item->TriggerFlags == 2)
					item->Pose.Position.x -= 80;
				else
					item->Pose.Position.x -= 256;

				break;
			}
		}
	}

	void InitialiseFlameEmitter3(short itemNumber)
	{
		auto* item = &g_Level.Items[itemNumber];

		if (item->TriggerFlags >= 3)
		{
			for (int i = 0; i < g_Level.NumItems; i++)
			{
				auto* currentItem = &g_Level.Items[i];

				if (currentItem->ObjectNumber == ID_ANIMATING3)
				{
					if (currentItem->TriggerFlags == item->TriggerFlags)
						item->ItemFlags[2] = i;
					else if (currentItem->TriggerFlags == 0)
						item->ItemFlags[3] = i;
				}
			}
		}
	}

	void FlameEmitter3Laser(int x1, int x2, int y, int z1, int z2)
	{
		g_Renderer.AddLine3D(Vector3(x1, y, z1), Vector3(x2, y, z2), Vector4(1, 0, 0, 1));
	}

	void FlameEmitter3Control(short itemNumber)
	{
		auto* item = &g_Level.Items[itemNumber];

		if (TriggerActive(item))
		{
			//wc - puzzle laser beam
			if (item->TriggerFlags == 998)
			{
				return;
			}
			if (item->TriggerFlags == 999)
			{
				int pushableX[10];
				int pushableZ[10];
				short pushableRot[10];
				int endpointX = -1;
				int endpointZ = -1;
				int i = 0;
				short itemNum = g_Level.Rooms[item->RoomNumber].itemNumber;
				while (itemNum != NO_ITEM)
				{
					auto item2 = &g_Level.Items[itemNum];
					if (item2->ObjectNumber == ID_PUSHABLE_OBJECT2 && item->RoomNumber == item2->RoomNumber)
					{
						pushableX[i] = item2->Pose.Position.x;
						pushableZ[i] = item2->Pose.Position.z;
						pushableRot[i++] = item2->Pose.Orientation.y;
					}
					else if (item2->ObjectNumber == ID_FLAME_EMITTER3 && item2->TriggerFlags == 998 && item->RoomNumber == item2->RoomNumber)
					{
						endpointX = item2->Pose.Position.x;
						endpointZ = item2->Pose.Position.z;
					}
					if (i >= 10 && endpointX != -1)
						break;
					itemNum = item2->NextItem;
				}
				short angle = item->Pose.Orientation.y;
				int xPos = item->Pose.Position.x;
				int zPos = item->Pose.Position.z;
				int xPosLast = item->Pose.Position.x;
				int zPosLast = item->Pose.Position.z;
				i = 0;
				//get laser out of the wall first
				if (angle == ANGLE(0))
				{
					zPos -= CLICK(2);
				}
				else if (angle == ANGLE(180))
				{
					zPos += CLICK(2);
				}
				else if (angle == ANGLE(90))
				{
					xPos -= CLICK(2);
				}
				else
				{
					xPos += CLICK(2);
				}
				for (int i = 0;i < 200;i++)
				{
					bool reflected = false;
					bool blocked = false;
					bool complete = false;
					if (angle == ANGLE(0))
					{
						zPos -= CLICK(1);
					}
					else if(angle == ANGLE(180))
					{
						zPos += CLICK(1);
					}
					else if (angle == ANGLE(90))
					{
						xPos -= CLICK(1);
					}
					else
					{
						xPos += CLICK(1);
					}
					if (xPos == endpointX && zPos == endpointZ)
					{
						//puzzle is complete!
						complete = true;
					}
					for (int j = 0;j < 10;j++)
					{
						if (xPos == pushableX[j] && zPos == pushableZ[j])
						{
							short testAngle = pushableRot[j] - angle;
							//test angle check must be within 180 degrees to work
							if (testAngle > ANGLE(180))
								testAngle -= ANGLE(90) * 4;
							else if (testAngle < ANGLE(180) * -1)
								testAngle += ANGLE(90) * 4;

							if(testAngle == ANGLE(270))
								angle += ANGLE(90);
							else if (testAngle == ANGLE(0))
								angle -= ANGLE(90);
							else
							{
								//failed to reflect, it is blocked instead
								blocked = true;
								break;
							}
							FlameEmitter3Laser(xPosLast, xPos, item->Pose.Position.y, zPosLast, zPos);
							xPosLast = xPos;
							zPosLast = zPos;
							reflected = true;
							break;
						}
					}
					if (complete)
					{
						TestTriggers(item, true);
						//removing for performance reasons
						Weather.Flash(255, 0, 0, 0.06f);
						RemoveActiveItem(itemNumber);
					}
					if (!reflected)
					{
						short room = item->RoomNumber;
						auto floor = GetFloor(xPos, item->Pose.Position.y, zPos, &room);
						//make sure we only check height if it was the same room; don't update the flameemitter's room!
						if (room == item->RoomNumber)
						{
							int height = GetFloorHeight(floor, xPos, item->Pose.Position.y, zPos);
							if (blocked || item->Pose.Position.y - height > CLICK(1))
							{
								FlameEmitter3Laser(xPosLast, xPos, item->Pose.Position.y, zPosLast, zPos);
								TriggerDynamicLight(xPos, item->Pose.Position.y, zPos, 8, 255, 0, 0);
								break;
							}
						}
						else
						//different room returned means it was blocked
						{
							FlameEmitter3Laser(xPosLast, xPos, item->Pose.Position.y, zPosLast, zPos);
							TriggerDynamicLight(xPos, item->Pose.Position.y, zPos, 8, 255, 0, 0);
							break;
						}
					}
				}
				return;
			}
			if (item->TriggerFlags)
			{
				SoundEffect(SFX_TR4_ELECTRIC_ARCING_LOOP, &item->Pose);

				byte g = (GetRandomControl() & 0x3F) + 192;
				byte b = (GetRandomControl() & 0x3F) + 192;

				auto src = item->Pose.Position;
				Vector3Int dest;

				if (!(GlobalCounter & 3))
				{
					if (item->TriggerFlags == 2 || item->TriggerFlags == 4)
					{
						dest.x = item->Pose.Position.x + 2048 * phd_sin(item->Pose.Orientation.y + ANGLE(180));
						dest.y = item->Pose.Position.y;
						dest.z = item->Pose.Position.z + 2048 * phd_cos(item->Pose.Orientation.y + ANGLE(180));

						if (GetRandomControl() & 3)
						{
							TriggerLightning(
								&src, 
								&dest, 
								(GetRandomControl() & 0x1F) + 64, 
								0, 
								g, 
								b, 
								24, 
								0, 
								32, 
								3);
						}
						else
						{
							TriggerLightning(
								&src, 
								&dest, 
								(GetRandomControl() & 0x1F) + 96,
								0,
								g, 
								b,
								32,
								LI_SPLINE,
								32, 
								3);
						}
					}
				}

				if (item->TriggerFlags >= 3 && !(GlobalCounter & 1))
				{
					short targetItemNumber = item->ItemFlags[((GlobalCounter >> 2) & 1) + 2];
					auto* targetItem = &g_Level.Items[targetItemNumber];

					dest = Vector3Int(0, -64, 20);
					GetJointAbsPosition(targetItem, &dest, 0);

					if (!(GlobalCounter & 3))
					{
						if (GetRandomControl() & 3)
						{
							TriggerLightning(
								&src, 
								&dest,
								(GetRandomControl() & 0x1F) + 64,
								0,
								g,
								b,
								24, 
								0, 
								32,
								5);
						}
						else
						{
							TriggerLightning(
								&src,
								&dest,
								(GetRandomControl() & 0x1F) + 96,
								0,
								g,
								b,
								32,
								LI_SPLINE,
								32,
								5);
						}
					}

					if (item->TriggerFlags != 3 || targetItem->TriggerFlags)
						TriggerLightningGlow(dest.x, dest.y, dest.z, 64, 0, g, b);
				}

				if ((GlobalCounter & 3) == 2)
				{
					src = item->Pose.Position;

					dest = Vector3Int(
						(GetRandomControl() & 0x1FF) + src.x - 256,
						(GetRandomControl() & 0x1FF) + src.y - 256,
						(GetRandomControl() & 0x1FF) + src.z - 256
					);

					TriggerLightning(
						&src, 
						&dest, 
						(GetRandomControl() & 0xF) + 16,
						0,
						g,
						b,
						24, 
						LI_SPLINE | LI_MOVEEND,
						32,
						3);
					TriggerLightningGlow(dest.x, dest.y, dest.z, 64, 0, g, b);
				}
			}
			else
			{
				// Small fires
				if (item->ItemFlags[0] != 0)
					item->ItemFlags[0]--;
				else
				{
					item->ItemFlags[0] = (GetRandomControl() & 3) + 8;
					short random = GetRandomControl() & 0x3F;
					if (item->ItemFlags[1] == random)
						random = (random + 13) & 0x3F;
					item->ItemFlags[1] = random;
				}

				int x, z, i;

				if (!(Wibble & 4))
				{
					i = item->ItemFlags[1] & 7;
					x = 16 * (Flame3xzoffs[i][0] - 32);
					z = 16 * (Flame3xzoffs[i][1] - 32);
					TriggerFireFlame(x + item->Pose.Position.x, item->Pose.Position.y, z + item->Pose.Position.z, -1, 2);
				}
				else
				{
					i = item->ItemFlags[1] >> 3;
					x = 16 * (Flame3xzoffs[i + 8][0] - 32);
					z = 16 * (Flame3xzoffs[i + 8][1] - 32);
					TriggerFireFlame(x + item->Pose.Position.x, item->Pose.Position.y, z + item->Pose.Position.z, -1, 2);
				}

				SoundEffect(SFX_TR4_LOOP_FOR_SMALL_FIRES, &item->Pose);

				TriggerDynamicLight(x, item->Pose.Position.y, z, 12, (GetRandomControl() & 0x3F) + 192, ((GetRandomControl() >> 4) & 0x1F) + 96, 0);

				auto pos = PHD_3DPOS(item->Pose.Position);

				if (ItemNearLara(&pos, 600))
				{
					if ((!Lara.Burn) && Lara.Control.WaterStatus != WaterStatus::FlyCheat)
					{
						DoDamage(LaraItem, 5);

						int dx = LaraItem->Pose.Position.x - item->Pose.Position.x;
						int dz = LaraItem->Pose.Position.z - item->Pose.Position.z;

						if (SQUARE(dx) + SQUARE(dz) < SQUARE(450))
							LaraBurn(LaraItem);
					}
				}
			}
		}
	}

	void FlameEmitterCollision(short itemNumber, ItemInfo* laraItem, CollisionInfo* coll)
	{
		auto* item = &g_Level.Items[itemNumber];

		if (Lara.Control.Weapon.GunType != LaraWeaponType::Torch ||
			Lara.Control.HandStatus != HandStatus::WeaponReady ||
			Lara.LeftArm.Locked ||
			Lara.Torch.IsLit == (item->Status & 1) ||
			item->Timer == -1 ||
			!(TrInput & IN_ACTION) ||
			laraItem->Animation.ActiveState != LS_IDLE ||
			laraItem->Animation.AnimNumber != LA_STAND_IDLE ||
			laraItem->Animation.IsAirborne)
		{
			if (item->ObjectNumber == ID_BURNING_ROOTS)
				ObjectCollision(itemNumber, laraItem, coll);
		}
		else
		{
			switch (item->ObjectNumber)
			{
			case ID_FLAME_EMITTER:
				FireBounds.boundingBox.X1 = -256;
				FireBounds.boundingBox.X2 = 256;
				FireBounds.boundingBox.Y1 = 0;
				FireBounds.boundingBox.Y2 = 1024;
				FireBounds.boundingBox.Z1 = -800;
				FireBounds.boundingBox.Z2 = 800;
				break;

			case ID_FLAME_EMITTER2:
				FireBounds.boundingBox.X1 = -256;
				FireBounds.boundingBox.X2 = 256;
				FireBounds.boundingBox.Y1 = 0;
				FireBounds.boundingBox.Y2 = 1024;
				FireBounds.boundingBox.Z1 = -600;
				FireBounds.boundingBox.Z2 = 600;
				break;

			case ID_BURNING_ROOTS:
				FireBounds.boundingBox.X1 = -384;
				FireBounds.boundingBox.X2 = 384;
				FireBounds.boundingBox.Y1 = 0;
				FireBounds.boundingBox.Y2 = 2048;
				FireBounds.boundingBox.Z1 = -384;
				FireBounds.boundingBox.Z2 = 384;
				break;
			}

			short oldYrot = item->Pose.Orientation.y;
			item->Pose.Orientation.y = laraItem->Pose.Orientation.y;

			if (TestLaraPosition(&FireBounds, item, laraItem))
			{
				if (item->ObjectNumber == ID_BURNING_ROOTS)
					laraItem->Animation.AnimNumber = LA_TORCH_LIGHT_5;
				else
				{
					Lara.Torch.State = TorchState::JustLit;
					int dy = abs(laraItem->Pose.Position.y - item->Pose.Position.y);
					laraItem->ItemFlags[3] = 1;
					laraItem->Animation.AnimNumber = (dy >> 8) + LA_TORCH_LIGHT_1;
				}

				laraItem->Animation.ActiveState = LS_MISC_CONTROL;
				laraItem->Animation.FrameNumber = g_Level.Anims[laraItem->Animation.AnimNumber].frameBase;
				Lara.Flare.ControlLeft = false;
				Lara.LeftArm.Locked = true;
				Lara.InteractedItem = itemNumber;
			}

			item->Pose.Orientation.y = oldYrot;
		}

		if (Lara.InteractedItem == itemNumber &&
			item->Status != ITEM_ACTIVE &&
			laraItem->Animation.ActiveState == LS_MISC_CONTROL)
		{
			if (laraItem->Animation.AnimNumber >= LA_TORCH_LIGHT_1 && laraItem->Animation.AnimNumber <= LA_TORCH_LIGHT_5)
			{
				if (laraItem->Animation.FrameNumber - g_Level.Anims[laraItem->Animation.AnimNumber].frameBase == 40)
				{
					TestTriggers(item, true, item->Flags & IFLAG_ACTIVATION_MASK);

					item->Flags |= 0x3E00;
					item->ItemFlags[3] = 0;
					item->Status = ITEM_ACTIVE;

					AddActiveItem(itemNumber);
				}
			}
		}
	}
}
