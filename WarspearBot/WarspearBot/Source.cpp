#include<iostream>
#include <Windows.h>
#include <TlHelp32.h>
#include <vector>
#include <conio.h>
#include <ctime> 

#define K_ENTER 0x0D
#define K_LEFT 0x25
#define K_UP 0x26
#define K_RIGHT 0x27
#define K_DOWN 0x28
#define K_1 0x31
#define K_2 0x32
#define K_3 0x33
#define K_4 0x34


using namespace std;


HWND hwnd;
DWORD pId;
HANDLE handle;
class Param
{
	uintptr_t address;
	uintptr_t c_address;
	std::vector<unsigned int> offsets;
	bool is_2bytes;
public:
	int value;
	void Set(uintptr_t new_address, std::vector<unsigned int> new_offsets, bool new_is_2bytes)
	{
		address = new_address;
		offsets = new_offsets;
		is_2bytes = new_is_2bytes;
		ParamFind();
	}
	void ParamFind()
	{
		uintptr_t addr = address;
		for (int i = 0; i < offsets.size(); ++i)
		{
			//printf("[*] address%i: %08x and %08x\n ", i, addr, offsets[i]);
			addr += offsets[i];
			//printf("%08x\n", addr);
			if (i != offsets.size() - 1)
				ReadProcessMemory(handle, (BYTE*)addr, &addr, sizeof(addr), 0);
		}
		c_address = addr;
		//printf("Address: [%08x]  ", addr);
		ParamUpdate();
	}
	void ParamUpdate()
	{
		short int short_value;
		if (is_2bytes)
		{
			ReadProcessMemory(handle, (PBYTE*)c_address, &short_value, sizeof(short_value), NULL);
			value = short_value;
		}
		else
		{
			ReadProcessMemory(handle, (PBYTE*)c_address, &value, sizeof(value), NULL);
		}
	}
};
Param cursorX;
Param cursorY;
Param cursorFlag;
Param playerMaxHP;
Param playerHP;
Param playerX;
Param playerY;
Param isMoving;

// Статические адресса (Нужно менять при перезапуске игры)
uintptr_t address_curosr; // Cursor
uintptr_t address_player; // Player




// Доп параметры
int lastHP;
int skill_reload_1=0; // 10
int skill_reload_2=0; // 16
int skill_reload_4 = 0; //
bool changer = true;

// Дейл Шаман Кот sp {26,1} fr {16,24,1,3} KAS true;
// Дейл (коты, наездники на драконах) Рога Для Питья sp { 1,26 } fr KAS true
// Стандартные коты sp { 11,2 } fr {1,9,4,7}

//{ 20,22 }; // x,y
// {23,26,23,25}; 
bool KILL_AND_SAVE = true;
int save_point[2] = { 25,18 }; // x,y
int find_region[4] = { 7,9,13,16 }; // x1,x2,y1,y2
int playerMinHP=2200;
int hit_counter = 0;

////////////////////// Работа с параметрами
// Создание параметров
void CreateParams()
{
	cursorX.Set(address_curosr, { 0x24, 0x10, 0x4, 0x21C, 0x8 },true);
	cursorY.Set(address_curosr, { 0x24, 0x10, 0x4, 0x21C, 0xA },true);
	cursorFlag.Set(address_curosr, { 0x24, 0x10, 0x4, 0x21C, 0x6C },true);
	playerMaxHP.Set(address_player, { 0x0,0x10,0xD4,0x4,0xB8,0x4C,0xA0 },false);
	playerHP.Set(address_player, { 0x0,0x10,0xD4,0x4,0xB8,0x4C,0x9C },false);
	lastHP = playerHP.value;
	playerX.Set(address_player, { 0x0,0x10,0xD4,0x4,0xB8,0x4C,0xB0 },true);
	playerY.Set(address_player, { 0x0, 0x10, 0xD4, 0x4, 0xB8, 0x4C, 0xB2 },true);
	isMoving.Set(address_player, { 0x0, 0x10, 0xD4, 0x4, 0xB8, 0x4C, 0xD0 },true);
}
// Обновление значений в параметрах
void UpdateParams() {
	cursorX.ParamUpdate();
	cursorY.ParamUpdate();
	cursorFlag.ParamUpdate();
	playerMaxHP.ParamUpdate();
	lastHP = playerHP.value;
	playerHP.ParamUpdate();
	playerX.ParamUpdate();
	playerY.ParamUpdate();
	isMoving.ParamUpdate();
}
// Вывод параметров
void PrintParams()
{
	printf("CursorX: [%i]\n", cursorX.value);
	printf("CursorY: [%i]\n", cursorY.value);
	printf("CursorFlag: [%i]\n", cursorFlag.value);
	printf("PlayerMaxHP: [%i]\n", playerMaxHP.value);
	printf("PlayerHP: [%i]\n", playerHP.value);
	printf("PlayerX: [%i]\n", playerX.value);
	printf("PlayerY: [%i]\n", playerY.value);
	printf("PlayerIsMoving: [%i]\n\n", isMoving.value);
}
//////////////////////

uintptr_t GetModuleBaseAddress(DWORD procId, const TCHAR* modName)
{
	uintptr_t modBaseAddr = 0;
	HANDLE hSnap = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE | TH32CS_SNAPMODULE32, procId);
	if (hSnap != INVALID_HANDLE_VALUE)
	{
		MODULEENTRY32 modEntry;
		modEntry.dwSize = sizeof(modEntry);
		if (Module32First(hSnap, &modEntry))
		{
			do
			{
				if (!lstrcmpi(modEntry.szModule, modName))
				{
					modBaseAddr = (uintptr_t)modEntry.modBaseAddr;
					break;
				}
				printf("0x%x - %s\n", (int)modEntry.modBaseAddr, modEntry.szModule);
			} while (Module32Next(hSnap, &modEntry));
		}
	}
	CloseHandle(hSnap);
	return modBaseAddr;
}


// Отправка нажатий клавишь в окно
void keyPress(uintptr_t key,int sleep=0) {
	LPARAM lParam;
	int ScanCode = MapVirtualKey(key, 0);
	lParam = ScanCode << 16;
	lParam |= 1;
	PostMessage(hwnd, WM_KEYDOWN, key, lParam);
	Sleep(sleep);
	PostMessage(hwnd, WM_KEYUP, key, lParam);

	//Задержка при передвижении
	if (key == K_UP || key == K_LEFT || key == K_RIGHT || key == K_DOWN)
		Sleep(200);
	//
}

// Перемещение курсора на нужную точку
void CursorToPoint(int x, int y)
{	
	int start = clock();
	while (cursorX.value != x || cursorY.value != y)
	{
		//printf("X %i == %i  Y %i == %i\n", cursorX, x, cursorY, y);
		if (cursorX.value < x)
			keyPress(K_RIGHT);
		if (cursorX.value > x)
			keyPress(K_LEFT);
		if (cursorY.value > y)
			keyPress(K_UP);
		if (cursorY.value < y)
			keyPress(K_DOWN);
		UpdateParams();
		if((clock() - start) / CLOCKS_PER_SEC > 6)
		{
			keyPress(K_ENTER,5);
		}
	}
}

// Проверка на врага в точке
bool isEnemy()
{
	if (cursorFlag.value == 8 || cursorFlag.value == 0)
		return true;
	else
		return false;
}
// Проверка на лут в точке
bool isLoot()
{
	if (cursorFlag.value == 10)
		return true;
	else
		return false;
}
// Сделать удар по точке и использовать скиллы
void attackEnemy() {
	keyPress(K_ENTER,5);
	//if ((clock() - skill_reload_4) > 30)
	//{
	//	keyPress(K_4);
	//	skill_reload_2 = clock();
	//	Sleep(100);
	//}
	UpdateParams();
	if ((clock() - skill_reload_2) > 16 && isEnemy())
	{
		keyPress(K_2);
		skill_reload_2 = clock();
		Sleep(150);
	}
	UpdateParams();
	if ((clock() - skill_reload_1) > 10 && isEnemy())
	{
		keyPress(K_1);
		skill_reload_1 = clock();
		Sleep(150);
	}
	while (isMoving.value == 1)
		UpdateParams();
	Sleep(500);
}
// Залутать точку
void lootEnemy()
{
	keyPress(K_ENTER,5);
	while (isMoving.value == 1)
		UpdateParams();
	while (isLoot())
	{
		keyPress(K_ENTER, 5);
		Sleep(250);
		UpdateParams();
	}
}


// Проверка на удар
bool isHit()
{
	if (lastHP <= playerHP.value)
	{
		return false;
	}
	if (lastHP > playerHP.value)
	{
		return true;
	}
	return false;
}
// Действия при ударе
bool action()
{
	if (isEnemy())
	{
		attackEnemy();
		while (isEnemy())
		{
			attackEnemy();
			//cout << "fight!" << endl;
			UpdateParams();
		}
		UpdateParams();
		if (isLoot())
		{
			cout << "Loot" << endl;
			lootEnemy();
			return true;
		}
		return true;
	}
	if (isLoot())
	{
		cout << "Loot" << endl;
		lootEnemy();
		return true;
	}
}
bool hit_reaction(int range)
{
	cout << "hit!" << endl;
	//bool gox = true;
	//int find_range = 1;

	int n = range;
	int x = playerX.value, y = playerY.value;
	int go = 1;
	bool ret = false;
	CursorToPoint(x, y);
	if (action())
		ret = true;
	//int i = n;
	for (int i = 1; i <= n; i++)
	{
		int k = x - i, j = y - i;
		switch (go)
		{
		case 1:
			for (k; k < x + i; ++k)
			{
				CursorToPoint(k,j);
				if (action())
					ret = true;
			}
			go = 2;
			//break;
		case 2:
			for (j; j < y + i; ++j)
			{
				CursorToPoint(k, j);
				if (action())
					ret = true;
			}
			go = 3;
			//break;
		case 3:
			for (k; k > x - i; --k)
			{
				CursorToPoint(k, j);
				if (action())
					ret = true;
			}
			go = 4;
			//break;
		case 4:
			for (j; j > y - i; --j)
			{
				CursorToPoint(k, j);
				if (action())
					ret = true;
			}
			go = 1;
			//break;
		}
	}
	return ret;
}
// Получаем ли урон в течении 3 секунд (пока не используем)
bool fight()
{
	int start = clock();
	while ((clock() - start) / CLOCKS_PER_SEC < 3)
	{
		UpdateParams();
		if (isHit())
			return true;
	}
	return false;
}
// Действия при малом здоровье
bool PlayerHealth(int x= save_point[0],int y= save_point[1], int minHP = playerMinHP)
{
	if (playerHP.value < minHP)
	{
		CursorToPoint(x,y);
		if (cursorFlag.value != 15)
		{
			keyPress(K_ENTER);
			while (playerX.value != cursorX.value || playerY.value != cursorY.value)
				UpdateParams();
			while (playerHP.value != playerMaxHP.value)
			{
				UpdateParams();
				if (isHit())
				{
					hit_counter++;
					if (hit_reaction(hit_counter))
						hit_counter = 0;
					else
						continue;
				}
			}
		}
		else
		{
			printf("\n		[*]Cursor Block!\n");
		}
		return false;
	}
	else
		return true;
}

bool findEnemy(int xs = find_region[0],int xe = find_region[1],int ys = find_region[2],int ye = find_region[3])
{
	bool gox = true;
	for (int k = xe; k >= xs; k--) //for (int k = xs; k <= xe; k++)
	{
		if(gox)
			for (int j = ye; j >= ys; j--)
			{
				CursorToPoint(k, j);
				if (isEnemy())
					return true;
			}
		else
			for (int j = ys; j <= ye; j++)
			{
				CursorToPoint(k, j);
				if (isEnemy())
					return true;
			}
		gox = !gox;
	}
	return false;
}





int main()
{
	SetConsoleTitle("Warspear Cheat");

	hwnd = FindWindow(NULL, "Warspear Online");
	if (hwnd == NULL)
	{
		cout << "[-] Please Open Warspear Online" << endl;
		Sleep(1000);
		return 1;
	}
	cout << "	[+] Window is Opened!" << endl;

	GetWindowThreadProcessId(hwnd, &pId);
	handle = OpenProcess(PROCESS_ALL_ACCESS, FALSE, pId);
	if (pId == NULL)
	{
		cout << "\n\n [-] Process ID not Found" << endl;
		Sleep(1000);
		return 1;
	}
	cout << "	[+] Process is Opened!" << endl;

	DWORD dwBaseOffset = GetModuleBaseAddress(pId, "warspear.exe");


	ReadProcessMemory(handle, (PBYTE*)(dwBaseOffset + 0x05F0330), &address_curosr, sizeof(address_curosr), NULL);
	ReadProcessMemory(handle, (PBYTE*)(dwBaseOffset + 0x05F49B8), &address_player, sizeof(address_player), NULL);
	printf("address 0x%x\n ", dwBaseOffset);
	printf("address_curosr 0x%x\n ", address_curosr);
	printf("address_player 0x%x\n ", address_player);

	cout << "\n[+] We are Ready!\n" << endl;

	CreateParams();
	PrintParams();
	srand(clock());
	int hit_time=clock();
	bool changer=true;
	int game_mode = 1;
	while (game_mode==1)
	{
		UpdateParams();
		PlayerHealth(13, 5);

		if (isHit())
		{
			hit_counter++;
			if (hit_reaction(hit_counter))
				hit_counter = 0;
			else
				continue;
			Sleep((1 + rand() % (4 - 1 + 1)) * 1000);
			CursorToPoint(17 + rand() % (19 - 17 + 1), 14 + rand() % (16 - 14 + 1));
			keyPress(K_ENTER, 5);
		}
	}
	while (game_mode==2) {
		UpdateParams();
		PlayerHealth(13,5);
		if (changer)
		{
			CursorToPoint(16, 15);
			keyPress(K_ENTER, 5);
			while (!findEnemy(17, 19, 14, 16))
			{
				UpdateParams();
				if (isHit())
				{
					hit_counter++;
					if (hit_reaction(hit_counter))
						hit_counter = 0;
					else
						continue;
				}
			}
			while (findEnemy(17, 19, 14, 16))
			{
				action();
			}
			changer = !changer;
			hit_time = clock();
			while ((clock() - hit_time) / CLOCKS_PER_SEC < 5)
			{
				UpdateParams();
				if (isHit())
				{
					hit_counter++;
					if (hit_reaction(hit_counter))
						hit_counter = 0;
					else
						continue;
				}
			}
		}
		else
		{
			CursorToPoint(11, 15);
			keyPress(K_ENTER, 5);
			while (!findEnemy(7, 9, 13, 16))
			{
				UpdateParams();
				if (isHit())
				{
					hit_counter++;
					if (hit_reaction(hit_counter))
						hit_counter = 0;
					else
						continue;
				}
			}
			while (findEnemy(7, 9, 13, 16))
			{
				action();
			}
			changer = !changer;
			hit_time = clock();
			while ((clock() - hit_time) / CLOCKS_PER_SEC < 5)
			{
				UpdateParams();
				if (isHit())
				{
					hit_counter++;
					if (hit_reaction(hit_counter))
						hit_counter = 0;
					else
						continue;
				}
			}
		}
	}
	return 0;
}