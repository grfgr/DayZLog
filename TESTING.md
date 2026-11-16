# Локальный сервер для теста PvPLog

Для Windows. Пути ниже даны для стандартной установки Steam, поправьте под свои.

## 1. Что поставить из Steam

1. **DayZ Server**: Steam → Библиотека → фильтр «Инструменты» → *DayZ Server*. Бесплатно, если у вас есть DayZ.
   Папка: `C:\Program Files (x86)\Steam\steamapps\common\DayZServer`
2. **DayZ Tools**: там же, в «Инструментах». Нужен только **Addon Builder**.
3. Подпишитесь в Мастерской на:
   - **Community Framework**: https://steamcommunity.com/sharedfiles/filedetails/?id=1559212036
   - **VPPAdminTools**: https://steamcommunity.com/sharedfiles/filedetails/?id=1828439124

   Запустите DayZ Launcher, чтобы моды скачались. Они окажутся в
   `C:\Program Files (x86)\Steam\steamapps\common\DayZ\!Workshop\` (папки `@CF` и `@VPPAdminTools`).

## 2. Моды на сервер

1. Скопируйте `@CF` и `@VPPAdminTools` из `DayZ\!Workshop\` в папку `DayZServer\`.
2. Скопируйте ключи: всё из `@CF\keys\` и `@VPPAdminTools\keys\` (файлы `*.bikey`) положите в `DayZServer\keys\`.

## 3. Сборка PvPLog в PBO

1. Скачайте мод: на GitHub откройте ветку `claude/pvp-logging-mod-ugsq1y` → **Code → Download ZIP** и распакуйте.
2. Создайте папку `DayZServer\@PvPLog\Addons\`.
3. Запустите **DayZ Tools → Addon Builder**:
   - **Addon source directory**: распакованная папка `...\DayZLog\PvPLog` (та, где лежит `config.cpp`);
   - **Destination directory**: `DayZServer\@PvPLog\Addons`;
   - **Options**:
     - **Addon prefix**: `PvPLog`;
     - **Binarize**: снять галочку;
     - в **List of files to copy directly** добавьте `*.c;*.cpp` (если там уже что-то есть, допишите через `;`);
   - нажмите **Pack**.
4. Должен появиться файл `DayZServer\@PvPLog\Addons\PvPLog.pbo`.

Ключ для PvPLog не нужен: это серверный мод, клиенты его не загружают.

## 4. Настройка сервера

Откройте `DayZServer\serverDZ.cfg` и проверьте или поменяйте строки:
```
hostname = "PvPLog test";
password = "";
passwordAdmin = "admin123";
maxPlayers = 10;
verifySignatures = 2;
BattlEye = 1;
vppDisablePassword = 1;   // VPP не будет спрашивать пароль (только для локального теста!)
```
В конце файла должна быть миссия:
```
class Missions
{
    class DayZ
    {
        template = "dayzOffline.chernarusplus";
    };
};
```

### Права суперадмина в VPP

1. Узнайте свой Steam64 ID: это 17-значное число, начинается с `7656119`. Посмотреть можно на https://steamid.io по ссылке на ваш профиль.
2. Создайте файл
   `DayZServer\profiles\VPPAdminTools\Permissions\SuperAdmins\SuperAdmins.txt`
   (папки создайте сами, если их нет) и впишите в него одной строкой свой Steam64 ID.

## 5. Запуск

Создайте `DayZServer\start.bat`:
```bat
@echo off
cd /d "C:\Program Files (x86)\Steam\steamapps\common\DayZServer"
start "" DayZServer_x64.exe -config=serverDZ.cfg -port=2302 -profiles=profiles -dologs -adminlog -netlog -freezecheck "-mod=@CF;@VPPAdminTools" "-servermod=@PvPLog"
```
Запустите его. Первый старт идёт 1–3 минуты.

**Проверка, что мод загрузился.** Откройте последний `DayZServer\profiles\script_*.log` и найдите строку
```
[PvPLog] started, logs: $profile:PvPLog/logs
```
Должен появиться файл `profiles\PvPLog\settings.json`.

Если сервер сразу закрылся или показал окно с ошибкой вроде `Can't compile "World" script module`, мод не скомпилировался. Пришлите эту ошибку из `script_*.log`.

## 6. Подключение клиента

1. DayZ Launcher → **Моды**: включите `CF` и `VPPAdminTools`.
2. **Серверы → LAN** (или **Direct Connect**: `127.0.0.1`, порт `2302`) → подключиться.
3. В игре:
   - **Home** открывает меню VPP;
   - **End** включает и выключает режим админки.

## 7. Тест в одиночку

Для PvP нужен второй игрок, но в одиночку его заменяет **манекен**: заспавненный персонаж без игрока. Для мода это обычный `PlayerBase`, так что попадания и смерть по нему логируются так же, как по живому игроку. В логе он записывается как `Bot (SurvivorM_Mirek)`.

1. Откройте VPP (**Home**) → **Item Manager**.
2. Найдите `SurvivorM_Mirek` (подойдёт любой `SurvivorM_*` или `SurvivorF_*`) и заспавните рядом с собой.
3. Там же заспавните оружие и патроны, например `M4A1` + `Mag_STANAG_30Rnd`, или `Mosin9130` + `Ammo_762x54`.
4. Отойдите на 20–50 м и стреляйте: в ногу, в торс, потом в голову.

### Что проверить

| Тест | Где смотреть | Что должно быть |
|---|---|---|
| Попадание | `profiles\PvPLog\logs\PvP_<дата>.log` | строка `HIT` с вашим ником, `Bot (...)`, оружием, зоной, уроном и дистанцией |
| Убийство | тот же файл | строка `KILL` с вашими координатами и координатами манекена |
| JSON | `PvP_<дата>.jsonl` | те же события, по одному JSON-объекту на строку |
| Уведомление VPP | в игре | всплывашка «PvP: убийство» (суперадмину приходит без настройки прав) |
| Чат-команда | в игре нажмите Enter, введите `/pvplog` | последние события в чате |
| Права | VPP → **Permissions** | в списке есть `PvPLog:KillAlerts`, `PvPLog:HitAlerts`, `PvPLog:ChatHistory` |
| Рукопашка | ударьте манекена кулаком или ножом | `HIT` с `Кулаки` или названием ножа, способ «ближний бой» |
| Граната | киньте `M67Grenade` к манекену | `HIT`/`KILL` со способом «взрыв» и вашим ником |
| Discord | см. ниже | сообщение в канале |

### Discord

1. Discord → настройки канала → **Интеграция → Вебхуки → Новый вебхук** → **Копировать URL**.
2. Выключите сервер. Вставьте URL в `profiles\PvPLog\settings.json`:
   ```json
   "DiscordKillsWebhook": "https://discord.com/api/webhooks/....",
   "DiscordHitsWebhook": "https://discord.com/api/webhooks/....",
   ```
   (для теста можно один и тот же URL в оба поля)
3. Запустите сервер и убейте манекена. Сообщение придёт в течение 2–3 секунд.

## 8. Настоящий PvP-тест

Манекен проверяет почти всё. Чтобы увидеть настоящие ники и Steam ID жертвы, нужен второй игрок. Второй клиент на том же ПК под тем же Steam-аккаунтом запустить нельзя, поэтому позовите друга. Для подключения извне откройте порты `2302–2305 UDP` и `27016 UDP` на роутере и в брандмауэре Windows, а друг должен подключаться с теми же модами CF и VPPAdminTools.

## 9. Как прислать проблему

Приложите:
- `profiles\script_*.log`, последний по времени;
- `profiles\PvPLog\logs\PvP_<дата>.log`, если он появился;
- что делали и что ожидали увидеть.
