#include <iostream>
#include <windows.h>
#include <time.h>
using namespace std;

#define MaxPlayers 10

struct Bayum {
    long health = 9000000000; // здоровье
    int resist = 44; // сопротивление атакам
    int damage = 73843; // урон обычной атакой
    int specialDamage = 150000; // урон спецатакой
    int attackCooldown = 5; // время между обычными атаками (сек)
    int specialCooldown = 10; // время между спецатаками (сек)
};

struct Player {
    long health = 500000; // здоровье игрока
    int damage = 12000; // урон обычной атакой
    int specialDamage = 30000; // урон спецприёмом (опционально)
    int attackCooldown = 2; // время между обычными атаками (сек)
    int specialCooldown = 5; // время между спецатаками (сек)
    int defense = 20; // защита игрока (уменьшает урон босса)
    int dodgeChance = 15; // шанс уклониться от атаки босса (%)
    char name[64]; // имя игрока (для логов)
};

//функция для потокобезопасного логиравания
void Log(const char* messege) {
    WaitForSingleObject(hConsolwMutex, INFINITE);
    printf("%s\n", messege);
    ReleaseMutex(hConsolwMutex);
}

Bayum boss;

Player players[MaxPlayers];
int playersCount;

HANDLE hMutex;
HANDLE hConsolwMutex;

HANDLE hBossSpecialAttackEvent;
HANDLE HGameOverEvent;

bool bossAlive;
bool gameOver;

char logBuff[256];

DWORD WINAPI BossThread(LPVOID lpParam) {
    DWORD lastSpecialAttack = GetTickCount(); 
    DWORD lastAttack = GetTickCount();

    while (bossAlive) {
        //ожиданеие следующей атаки
        DWORD now = GetTickCount();
        DWORD nextAttack = lastAttack + boss.attackCooldown * 1000;
        DWORD nextSpecialAttack = lastSpecialAttack + boss.specialCooldown * 1000;
        DWORD waitAttack;
        if (nextAttack < nextSpecialAttack) {
            waitAttack = nextAttack;
        }
        else {
            waitAttack = nextSpecialAttack;
        }

        if (waitAttack < now) {
            Sleep(waitAttack - now);
        }

        WaitForSingleObject(hMutex, INFINITE);

        if (!bossAlive || gameOver) {
            ReleaseMutex(hMutex);
            break;
        }
        //подсчет живых игроков
        int aliveCount = 0;
        int aliveIndex[MaxPlayers];
        for (int i = 0; i < playersCount; i++) {
            if (players[i].health > 0) {
                aliveIndex[aliveCount] = i;
                aliveCount++;
            }
        }
        if (aliveCount == 0) {
            bossAlive = true;
            gameOver = true;
            SetEvent(HGameOverEvent);
            ReleaseMutex(hMutex);
            break;
        }
        //атака
        now = GetTickCount();
        if (now >= lastSpecialAttack + boss.specialCooldown * 1000) {
            double baseSpecialAttack;
            if (aliveCount > 1) {
                baseSpecialAttack = (boss.specialDamage * (1 - 0.05 * (aliveCount - 1)));
                sprintf(logBuff,"[БОСС] Спецатака по всем игрокам! Урон: %d", (int)baseSpecialAttack);
                Log(logBuff);

                for (int i = 0; i < aliveCount; i++) {
                    int aindx = aliveIndex[i];
                    if (rand() % 100 < players[aindx].dodgeChance) {
                        sprintf(logBuff, "[БОСС] Игрок %s уклонился от спецатаки", players[aindx].name);
                        Log(logBuff);
                        continue;
                    }
                    else {
                        int reciaved = baseSpecialAttack * (100 - players[aindx].defense) / 100;
                        players[aindx].health -= reciaved;
                        if (players[aindx].health < 0)
                        {
                            players[aindx].health = 0;
                        }
                        sprintf(logBuff, "[БОСС] Игрок %s получил %d урона (осталось HP: %lld)", players[aindx].name, reciaved, players[aindx].health);
                        Log(logBuff);
                    }
                    lastSpecialAttack = now;
                    SetEvent(hBossSpecialAttackEvent);
                }
            }
            else {
                int target = aliveIndex[rand() % aliveCount]; //случайный игрок
                if (rand() % 100 < players[target].dodgeChance) {
                    sprintf(logBuff, "[БОСС] Игрок %s уклонился от обычной атаки", players[target].name);
                    Log(logBuff);
                }
                else {
                    int reciaved = boss.damage * (100 - players[target].defense) / 100;
                    players[target].health -= reciaved;
                    if (players[target].health < 0)
                    {
                        players[target].health = 0;
                    }
                    sprintf(logBuff, "[БОСС] Игрок %s получил %d урона (осталось HP: %lld)", players[target].name, reciaved, players[target].health);
                    Log(logBuff);
                }
                lastAttack = now;
            }
            if (boss.health <= 0) {
                bossAlive = false;
                gameOver = true;
                SetEvent(HGameOverEvent);
                sprintf(logBuff, "[БОСС] Босс повержен! Победили игроки.");
                Log(logBuff);
            }
            ReleaseMutex(hMutex);
            Log("[БОСС] Ход босса завершён");
            return 0;
        }
    }
}

int main()
{
}
