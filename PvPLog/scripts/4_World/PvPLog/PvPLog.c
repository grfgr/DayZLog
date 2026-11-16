class PvPLogPendingKill
{
	PlayerBase m_Victim;
	string m_VictimName;
	string m_VictimId;
	vector m_VictimPos;

	PlayerBase m_Killer;
	string m_KillerName;
	string m_KillerId;
	vector m_KillerPos;
	string m_Weapon;
	string m_WeaponName;
	string m_Method;

	int m_Time;
};

/*
	Server-side PvP logger.
	- OnHit: every hit a player takes from another player (directly, by grenade / trap they armed, or by the car they drive).
	- OnKilled: the death is finished a moment later (DEATH_DELAY_MS), so the fatal hit's zone and ammo are known
	  whatever order the engine calls EEKilled and EEHitBy in.
	Output: text + JSONL files (hits and kills), Discord webhook (kills only), /pvplog history.
*/
class PvPLog
{
	const static int DEATH_DELAY_MS = 100;

	protected static ref PvPLog s_Instance;

	protected ref PvPLogSettings m_Settings;
	protected ref PvPLogDiscord m_Discord;
	protected ref array<ref PvPLogPendingKill> m_Pending;
	protected ref array<string> m_History;

	static PvPLog Get()
	{
		if (!s_Instance)
			s_Instance = new PvPLog();

		return s_Instance;
	}

	void PvPLog()
	{
		m_Settings = PvPLogSettings.Load();
		m_Discord = new PvPLogDiscord(m_Settings.DiscordUsername);
		m_Pending = new array<ref PvPLogPendingKill>();
		m_History = new array<string>();
		Print("[PvPLog] started, logs: " + PvPLogSettings.LOG_DIR);
	}

	PvPLogSettings GetSettings()
	{
		return m_Settings;
	}

	// newest last
	array<string> GetHistory(int count)
	{
		array<string> result = new array<string>();
		int start = Math.Max(0, m_History.Count() - count);
		for (int i = start; i < m_History.Count(); i++)
		{
			result.Insert(m_History[i]);
		}

		return result;
	}

	// ---------------------------------------------------------------- hits

	void OnHit(PlayerBase victim, TotalDamageResult damageResult, int damageType, EntityAI source, string zone, string ammo)
	{
		if (!victim || victim.m_PvPLog_DeathHandled)
			return;

		PvPLogAttacker attacker = PvPLogUtils.ResolveAttacker(source, damageType);
		if (!attacker || attacker.m_Player == victim)
			return;

		PvPLogHitInfo hit = new PvPLogHitInfo();
		hit.m_Attacker = attacker.m_Player;
		hit.m_AttackerName = PvPLogUtils.NameOf(attacker.m_Player);
		hit.m_AttackerId = PvPLogUtils.IdOf(attacker.m_Player);
		hit.m_AttackerPos = attacker.m_Player.GetPosition();
		hit.m_Weapon = attacker.m_Weapon;
		hit.m_WeaponName = attacker.m_WeaponName;
		hit.m_Ammo = ammo;
		hit.m_Zone = zone;
		hit.m_Method = attacker.m_Method;
		hit.m_Time = GetGame().GetTime();
		victim.m_PvPLog_LastHit = hit;

		if (!m_Settings.LogHits)
			return;

		float damage = 0;
		float blood = 0;
		float shock = 0;
		if (damageResult)
		{
			damage = damageResult.GetDamage("", "Health");
			blood = damageResult.GetDamage("", "Blood");
			shock = damageResult.GetDamage("", "Shock");
		}

		float healthLeft = victim.GetHealth("", "Health");
		string victimName = PvPLogUtils.NameOf(victim);
		string victimId = PvPLogUtils.IdOf(victim);
		vector victimPos = victim.GetPosition();
		float distance = vector.Distance(hit.m_AttackerPos, victimPos);
		string time = PvPLogFmt.Now();

		string text = "[" + time + "] HIT  | ";
		text += hit.m_AttackerName + " (" + hit.m_AttackerId + ") [" + PvPLogFmt.Pos(hit.m_AttackerPos) + "]";
		text += " -> " + victimName + " (" + victimId + ") [" + PvPLogFmt.Pos(victimPos) + "]";
		text += " | " + hit.m_WeaponName + " (" + hit.m_Weapon + ") | " + ammo + " | " + PvPLogUtils.MethodLabel(hit.m_Method);
		text += " | " + PvPLogUtils.ZoneLabel(zone);
		text += " | урон " + PvPLogFmt.F1(damage) + " HP, кровь " + PvPLogFmt.F1(blood) + ", шок " + PvPLogFmt.F1(shock);
		text += " | осталось " + PvPLogFmt.F1(healthLeft) + " HP | " + PvPLogFmt.F1(distance) + " м";

		PvPLogJson json = new PvPLogJson();
		json.Str("kind", "hit");
		json.Str("time", time);
		json.Str("attacker", hit.m_AttackerName);
		json.Str("attackerId", hit.m_AttackerId);
		json.Vec("attackerPos", hit.m_AttackerPos);
		json.Str("victim", victimName);
		json.Str("victimId", victimId);
		json.Vec("victimPos", victimPos);
		json.Str("weapon", hit.m_Weapon);
		json.Str("weaponName", hit.m_WeaponName);
		json.Str("ammo", ammo);
		json.Str("method", hit.m_Method);
		json.Str("zone", zone);
		json.Num("damage", damage);
		json.Num("blood", blood);
		json.Num("shock", shock);
		json.Num("healthLeft", healthLeft);
		json.Num("distance", distance);

		Write(text, json.Build());

		string shortText = hit.m_AttackerName + " попал в " + victimName + " | " + hit.m_WeaponName + " | " + PvPLogUtils.ZoneLabel(zone) + " | " + PvPLogFmt.F1(distance) + " м";
		AddHistory("[" + time.Substring(11, 8) + "] " + shortText);
	}

	// ---------------------------------------------------------------- deaths

	void OnKilled(PlayerBase victim, Object killerObject)
	{
		if (!victim || victim.m_PvPLog_DeathHandled || !m_Settings.LogKills)
			return;

		PvPLogPendingKill kill = new PvPLogPendingKill();
		kill.m_Victim = victim;
		kill.m_VictimName = PvPLogUtils.NameOf(victim);
		kill.m_VictimId = PvPLogUtils.IdOf(victim);
		kill.m_VictimPos = victim.GetPosition();
		kill.m_Time = GetGame().GetTime();

		PvPLogAttacker attacker = PvPLogUtils.ResolveAttacker(EntityAI.Cast(killerObject), -1);
		if (attacker && attacker.m_Player != victim)
		{
			kill.m_Killer = attacker.m_Player;
			kill.m_KillerName = PvPLogUtils.NameOf(attacker.m_Player);
			kill.m_KillerId = PvPLogUtils.IdOf(attacker.m_Player);
			kill.m_KillerPos = attacker.m_Player.GetPosition();
			kill.m_Weapon = attacker.m_Weapon;
			kill.m_WeaponName = attacker.m_WeaponName;
			kill.m_Method = attacker.m_Method;
		}

		m_Pending.Insert(kill);
		GetGame().GetCallQueue(CALL_CATEGORY_SYSTEM).CallLater(ProcessPending, DEATH_DELAY_MS, false);
	}

	protected void ProcessPending()
	{
		int now = GetGame().GetTime();
		for (int i = m_Pending.Count() - 1; i >= 0; i--)
		{
			PvPLogPendingKill kill = m_Pending[i];
			if (now - kill.m_Time < DEATH_DELAY_MS)
				continue;

			FinishKill(kill);
			m_Pending.Remove(i);
		}

		if (m_Pending.Count() > 0)
			GetGame().GetCallQueue(CALL_CATEGORY_SYSTEM).CallLater(ProcessPending, DEATH_DELAY_MS, false);
	}

	protected void FinishKill(PvPLogPendingKill kill)
	{
		PvPLogHitInfo lastHit = null;
		if (kill.m_Victim)
		{
			kill.m_Victim.m_PvPLog_DeathHandled = true;
			lastHit = kill.m_Victim.m_PvPLog_LastHit;
		}

		bool indirect = false;
		int secondsSinceHit = -1;
		if (lastHit)
			secondsSinceHit = (kill.m_Time - lastHit.m_Time) / 1000;

		// no player killer (bled out, fell, ...): blame the last player who hit the victim, if recent enough
		if (kill.m_KillerId == "" && kill.m_KillerName == "")
		{
			if (!lastHit || m_Settings.KillAttributionWindowSec <= 0 || secondsSinceHit > m_Settings.KillAttributionWindowSec)
				return;

			indirect = true;
			kill.m_Killer = lastHit.m_Attacker;
			kill.m_KillerName = lastHit.m_AttackerName;
			kill.m_KillerId = lastHit.m_AttackerId;
			kill.m_KillerPos = lastHit.m_AttackerPos;
			if (lastHit.m_Attacker && lastHit.m_Attacker.IsAlive())
				kill.m_KillerPos = lastHit.m_Attacker.GetPosition();
		}

		// the fatal hit tells the zone and ammo; it also names the weapon better than the killer object does
		string zone = "";
		string ammo = "";
		if (lastHit && lastHit.m_AttackerId == kill.m_KillerId && lastHit.m_AttackerName == kill.m_KillerName)
		{
			zone = lastHit.m_Zone;
			ammo = lastHit.m_Ammo;
			kill.m_Weapon = lastHit.m_Weapon;
			kill.m_WeaponName = lastHit.m_WeaponName;
			kill.m_Method = lastHit.m_Method;
		}

		float distance = vector.Distance(kill.m_KillerPos, kill.m_VictimPos);
		string time = PvPLogFmt.Now();

		string text = "[" + time + "] KILL | Убийца: " + kill.m_KillerName + " (" + kill.m_KillerId + ") [" + PvPLogFmt.Pos(kill.m_KillerPos) + "]";
		text += " | Жертва: " + kill.m_VictimName + " (" + kill.m_VictimId + ") [" + PvPLogFmt.Pos(kill.m_VictimPos) + "]";
		text += " | " + kill.m_WeaponName + " (" + kill.m_Weapon + ") | " + ammo + " | " + PvPLogUtils.MethodLabel(kill.m_Method);
		text += " | " + PvPLogUtils.ZoneLabel(zone) + " | " + PvPLogFmt.F1(distance) + " м";
		if (indirect)
			text += " | косвенно: умер через " + secondsSinceHit + " с после попадания";

		PvPLogJson json = new PvPLogJson();
		json.Str("kind", "kill");
		json.Str("time", time);
		json.Str("killer", kill.m_KillerName);
		json.Str("killerId", kill.m_KillerId);
		json.Vec("killerPos", kill.m_KillerPos);
		json.Str("victim", kill.m_VictimName);
		json.Str("victimId", kill.m_VictimId);
		json.Vec("victimPos", kill.m_VictimPos);
		json.Str("weapon", kill.m_Weapon);
		json.Str("weaponName", kill.m_WeaponName);
		json.Str("ammo", ammo);
		json.Str("method", kill.m_Method);
		json.Str("zone", zone);
		json.Num("distance", distance);
		json.Bool("indirect", indirect);
		if (indirect)
			json.Int("secondsSinceHit", secondsSinceHit);

		Write(text, json.Build());

		string shortText = kill.m_KillerName + " убил " + kill.m_VictimName + " | " + kill.m_WeaponName + " | " + PvPLogUtils.ZoneLabel(zone) + " | " + PvPLogFmt.F1(distance) + " м";
		if (indirect)
			shortText += " (косвенно)";

		AddHistory("[" + time.Substring(11, 8) + "] " + shortText);

		if (m_Settings.DiscordKillsWebhook != "")
		{
			string fields = PvPLogDiscord.Field("Оружие", kill.m_WeaponName + " (" + kill.m_Weapon + ")");
			fields += "," + PvPLogDiscord.Field("Патрон", ammo);
			fields += "," + PvPLogDiscord.Field("Куда", PvPLogUtils.ZoneLabel(zone));
			fields += "," + PvPLogDiscord.Field("Дистанция", PvPLogFmt.F1(distance) + " м");
			fields += "," + PvPLogDiscord.Field("Способ", PvPLogUtils.MethodLabel(kill.m_Method));
			if (indirect)
				fields += "," + PvPLogDiscord.Field("Косвенно", "умер через " + secondsSinceHit + " с после попадания");

			fields += "," + PvPLogDiscord.Field("Убийца", kill.m_KillerName + "\n" + kill.m_KillerId + "\nКоординаты: " + PvPLogFmt.Pos(kill.m_KillerPos), false);
			fields += "," + PvPLogDiscord.Field("Жертва", kill.m_VictimName + "\n" + kill.m_VictimId + "\nКоординаты: " + PvPLogFmt.Pos(kill.m_VictimPos), false);
			m_Discord.Enqueue(m_Settings.DiscordKillsWebhook, Embed("Убийство", "**" + kill.m_KillerName + "** убил **" + kill.m_VictimName + "**", 15158332, fields));
		}
	}

	// ---------------------------------------------------------------- output

	protected string Embed(string title, string description, int color, string fieldsJson)
	{
		PvPLogJson footer = new PvPLogJson();
		footer.Str("text", GetGame().GetServerName());

		PvPLogJson embed = new PvPLogJson();
		embed.Str("title", title);
		embed.Str("description", description);
		embed.Int("color", color);
		embed.Raw("fields", "[" + fieldsJson + "]");
		embed.Raw("footer", footer.Build());
		embed.Str("timestamp", PvPLogFmt.IsoUtc());
		return embed.Build();
	}

	protected void Write(string text, string json)
	{
		string baseName = PvPLogSettings.LOG_DIR + "/PvP_" + PvPLogFmt.Today();
		if (m_Settings.WriteTextLog)
			AppendLine(baseName + ".log", text);

		if (m_Settings.WriteJsonLog)
			AppendLine(baseName + ".jsonl", json);
	}

	protected void AppendLine(string path, string line)
	{
		if (!FileExist(PvPLogSettings.LOG_DIR))
			MakeDirectory(PvPLogSettings.LOG_DIR);

		FileHandle file = OpenFile(path, FileMode.APPEND);
		if (file == 0)
		{
			Print("[PvPLog] cannot open " + path);
			return;
		}

		FPrintln(file, line);
		CloseFile(file);
	}

	protected void AddHistory(string line)
	{
		m_History.Insert(line);
		while (m_History.Count() > m_Settings.HistorySize && m_History.Count() > 0)
		{
			m_History.RemoveOrdered(0);
		}
	}
};
