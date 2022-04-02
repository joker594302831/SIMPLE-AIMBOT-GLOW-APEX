#include <iostream>
#include <Windows.h>
#include <TlHelp32.h>
#include <memory>
#include <string_view>
#include <cstdint>
#include <vector>
#include <string>
#include <iostream>
#include <ctime>
#include <random>
#include "../Header Files/offsets.h"
#include "../Header Files/Driver.h"

int screenWeight = 1920; // In-game resolution
int screenHeight = 1080;
int xFOV = 200; //Aimbot horizontal FOV (square)
int yFOV = 200; //Aimbot vertical FOV (square)
int aSmoothAmount = 1; // Aimbot smoothness

uintptr_t localPlayer;
uintptr_t entList;
uintptr_t viewRenderer;
uintptr_t viewMatrix;

struct GlowMode
{
	int8_t GeneralGlowMode, BorderGlowMode, BorderSize, TransparentLevel;
};

DWORD64 GetEntityById(int Ent, DWORD64 Base)
{
	DWORD64 EntityList = Base + OFFSET_ENTITYLIST; //updated
	DWORD64 BaseEntity = read<DWORD64>(EntityList);
	if (!BaseEntity)
		return NULL;
	return  read<DWORD64>(EntityList + (Ent << 5));
}

int crosshairX = screenWeight / 2;
int crosshairY = screenHeight / 2;

int entX = 0;
int entY = 0;

int closestX = 0;
int closestY = 0;

int aX = 0;
int aY = 0;

float entNewVisTime = 0;
float entOldVisTime[100];
int visCooldownTime[100];

struct Vector3 {
	float x, y, z;
};

struct Matrix {
	float matrix[16];
};

struct Vector3 _WorldToScreen(const struct Vector3 pos, struct Matrix matrix) {
	struct Vector3 out;
	float _x = matrix.matrix[0] * pos.x + matrix.matrix[1] * pos.y + matrix.matrix[2] * pos.z + matrix.matrix[3];
	float _y = matrix.matrix[4] * pos.x + matrix.matrix[5] * pos.y + matrix.matrix[6] * pos.z + matrix.matrix[7];
	out.z = matrix.matrix[12] * pos.x + matrix.matrix[13] * pos.y + matrix.matrix[14] * pos.z + matrix.matrix[15];

	_x *= 1.f / out.z;
	_y *= 1.f / out.z;

	int width = screenWeight;
	int height = screenHeight;

	out.x = width * .5f;
	out.y = height * .5f;

	out.x += 0.5f * _x * width + 0.5f;
	out.y -= 0.5f * _y * height + 0.5f;

	return out;
}

uintptr_t GetEntityBoneArray(uintptr_t ent)
{
	return read<uintptr_t>(ent + OFFSET_BONES);
}

Vector3 GetEntityBonePosition(uintptr_t ent, uint32_t BoneId, Vector3 BasePosition)
{
	unsigned long long pBoneArray = GetEntityBoneArray(ent);

	Vector3 EntityHead = Vector3();

	EntityHead.x = read<float>(pBoneArray + 0xCC + (BoneId * 0x30)) + BasePosition.x;
	EntityHead.y = read<float>(pBoneArray + 0xDC + (BoneId * 0x30)) + BasePosition.y;
	EntityHead.z = read<float>(pBoneArray + 0xEC + (BoneId * 0x30)) + BasePosition.z;

	return EntityHead;
}

Vector3 GetEntityBasePosition(uintptr_t ent)
{
	return read<Vector3>(ent + OFFSET_ORIGIN);
}

int main()
{
	while (!hwnd)
	{
		hwnd = FindWindowA(NULL, ("Apex Legends"));
		Sleep(500);
	}

	while (!oPID) // get the process id
	{
		oPID = GetPID("r5apex.exe");
		printf(" [+] Driver Loader\n [+] Status Apex:Detected\n [+] Contact Rakuza#7789");
		Sleep(500);
	}

	while (!oBaseAddress) // request the module base from driver
	{
		oBaseAddress = GetModuleBaseAddress(oPID, "r5apex.exe");
		Sleep(500);
	}
	
	while (true)
	{
		// Matrix set up
		uint64_t viewRenderer = read<uint64_t>(oBaseAddress + OFFSET_RENDER);
		uint64_t viewMatrix = read<uint64_t>(viewRenderer + OFFSET_MATRIX);
		Matrix m = read<Matrix>(viewMatrix);

		// Local player set up
		uintptr_t locPlayer = read<uintptr_t>(oBaseAddress + OFFSET_LOCAL_ENT);

		// Before entity loop starts
		int closestX = 9999;
		int closestY = 9999;

		// Entity loop starts here
		for (int i = 0; i < 64; i++)
		{
			DWORD64 Entity = GetEntityById(i, oBaseAddress);
			if (Entity == 0)
				continue;
			DWORD64 EntityHandle = read<DWORD64>(Entity + OFFSET_NAME);
			std::string Identifier = read<std::string>(EntityHandle);
			LPCSTR IdentifierC = Identifier.c_str();
			if (strcmp(IdentifierC, "player"))
			{

				Vector3 HeadPosition = GetEntityBonePosition(Entity, 8, GetEntityBasePosition(Entity));
				// Convert to screen position

				Vector3 w2sHeadAimPos = _WorldToScreen(HeadPosition, m);

				// Get screen position
				int entX = w2sHeadAimPos.x;
				int entY = w2sHeadAimPos.y;

				// Get entity total visible time
				entNewVisTime = read<float>(Entity + OFFSET_VISIBLE_TIME);

				// Get entity knocked state
				int entKnockedState = read<int>(Entity + OFFSET_BLEED_OUT_STATE);

				// Get player team ID
				int playerTeamID = read<int>(locPlayer + OFFSET_TEAM);

				// Get entity team ID
				int entTeamID = read<int>(Entity + OFFSET_TEAM);

				// Is it an enemy
				if (entTeamID != playerTeamID)
				{
					write<int>(Entity + OFFSET_GLOW_ENABLE, 1); // glow enable: 1 = enabled, 2 = disabled
					write<int>(Entity + OFFSET_GLOW_THROUGH_WALLS, 2); // glow through walls: 2 = enabled, 5 = disabled
					write<GlowMode>(Entity + GLOW_TYPE, { 101,101,46,90 }); // glow type: GeneralGlowMode, BorderGlowMode, BorderSize, TransparentLevel;

					// Is visible
					if (entNewVisTime != entOldVisTime[i])
					{
						visCooldownTime[i] = 0; // low values mean less latency, increase if you observe the color changes on visible enemies

						//見える敵
						if (entKnockedState == 0)
						{
							write<float>(Entity + 0x1D0, 0); // red color/brightness of visible enemies
							write<float>(Entity + 0x1D4, 61); // green							
							write<float>(Entity + 0x1D8, 0); // blue

							// Aimbot fov
							if (abs(crosshairX - entX) < abs(crosshairX - closestX) && abs(crosshairX - entX) < xFOV && abs(crosshairY - entY) < abs(crosshairY - closestY) && abs(crosshairY - entY) < yFOV)
							{
								// Aimbot find closest target
								closestX = entX;
								closestY = entY;
							}
						}
						else
						{
							//ダウンした敵の色
							write<float>(Entity + 0x1D0, 10); // r color/brightness of knocked enemies
							write<float>(Entity + 0x1D4, 10); // g 
							write<float>(Entity + 0x1D8, 10); // b
						}

						entOldVisTime[i] = entNewVisTime;
					}
					else
					{
						if (visCooldownTime[i] <= 0)
						{
							//見えない敵
							if (entKnockedState == 0)
							{
								write<float>(Entity + 0x1D0, 61); // r color/brightness of not visible enemies
								write<float>(Entity + 0x1D4, 0);  // g
								write<float>(Entity + 0x1D8, 0); // b
							}
							else
							{
								//ダウンした敵の色
								write<float>(Entity + 0x1D0, 10); // r color/brightness of knocked not visible enemies
								write<float>(Entity + 0x1D4, 10);  // g
								write<float>(Entity + 0x1D8, 10); // b
							}
						}
					}

					if (visCooldownTime[i] >= 0) visCooldownTime[i] -= 1;
				}
			}
		}
		// After entity loop ends
		if (closestX != 9999 && closestY != 9999)
		{
			// If aimbot key pressed
			if (GetAsyncKeyState(VK_LBUTTON) || GetAsyncKeyState(VK_RBUTTON))
			{
				// If mouse cursor shown
				CURSORINFO ci = { sizeof(CURSORINFO) };
				if (GetCursorInfo(&ci))
				{
					if (ci.flags == 0)
						aX = (closestX - crosshairX) / aSmoothAmount;
					aY = (closestY - crosshairY) / aSmoothAmount;
					mouse_event(MOUSEEVENTF_MOVE, aX, aY, 0, 0); // enable aimbot when mouse cursor is hidden
				}
			}
		}
		//Sleep(100);
	}
}
