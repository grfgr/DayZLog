class PvPLogPendingKill
{
	PlayerBase Victim;
	string VictimName;
	string VictimId;
	vector VictimPos;

	PlayerBase Killer;
	string KillerName;
	string KillerId;
	vector KillerPos;
	string Weapon;
	string WeaponName;
	string Method;

	int Time;
};

/*
	Server-side PvP logger.
	- OnHit: every hit a player takes from another player (directly, by grenade / trap they armed, or by the car they drive).
	- OnKilled: the death is finished a moment later (DEATH_DELAY_MS), so the fatal hit's zone and ammo are known
	  whatever order the engine calls EEKilled and EEHitBy in.
	Output: text + JSONL files, Discord webhooks, VPP admin notifications, /pvplog history.
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
		if (!attacker || attacker.Player == victim)
			return;

		PvPLogHitInfo hit = new PvPLogHitInfo();
		hit.Attacker = attacker.Player;
		hit.AttackerName = PvPLogUtils.NameOf(attacker.Player);
		hit.AttackerId = PvPLogUtils.IdOf(attacker.Player);
		hit.AttackerPos = attacker.Player.GetPosition();
		hit.Weapon = attacker.Weapon;
		hit.WeaponName = attacker.WeaponName;
		hit.Ammo = ammo;
		hit.Zone = zone;
		hit.Method = attacker.Method;
		hit.Time = GetGame().GetTime();
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
		float distance = vector.Distance(hit.AttackerPos, victimPos);
		string time = PvPLogFmt.Now();

		string text = "[" + time + "] HIT  | ";
		text += hit.AttackerName + " (" + hit.AttackerId + ") [" + PvPLogFmt.Pos(hit.AttackerPos) + "]";
		text += " -> " + victimName + " (" + victimId + ") [" + PvPLogFmt.Pos(victimPos) + "]";
		text += " | " + hit.WeaponName + " (" + hit.Weapon + ") | " + ammo + " | " + PvPLogUtils.MethodLabel(hit.Method);
		text += " | " + PvPLogUtils.ZoneLabel(zone);
		text += " | урон " + PvPLogFmt.F1(damage) + " HP, кровь " + PvPLogFmt.F1(blood) + ", шок " + PvPLogFmt.F1(shock);
		text += " | осталось " + PvPLogFmt.F1(healthLeft) + " HP | " + PvPLogFmt.F1(distance) + " м";

		PvPLogJson json = new PvPLogJson();
		json.Str("kind", "hit");
		json.Str("time", time);
		json.Str("attacker", hit.AttackerName);
		json.Str("attackerId", hit.AttackerId);
		json.Vec("attackerPos", hit.AttackerPos);
		json.Str("victim", victimName);
		json.Str("victimId", victimId);
		json.Vec("victimPos", victimPos);
		json.Str("weapon", hit.Weapon);
		json.Str("weaponName", hit.WeaponName);
		json.Str("ammo", ammo);
		json.Str("method", hit.Method);
		json.Str("zone", zone);
		json.Num("damage", damage);
		json.Num("blood", blood);
		json.Num("shock", shock);
		json.Num("healthLeft", healthLeft);
		json.Num("distance", distance);

		Write(text, json.Build());

		string shortText = hit.AttackerName + " попал в " + victimName + " | " + hit.WeaponName + " | " + PvPLogUtils.ZoneLabel(zone) + " | " + PvPLogFmt.F1(distance) + " м";
		AddHistory("[" + time.Substring(11, 8) + "] " + shortText);

		if (m_Settings.DiscordHitsWebhook != "")
		{
			string fields = PvPLogDiscord.Field("Оружие", hit.WeaponName + " (" + hit.Weapon + ")");
			fields += "," + PvPLogDiscord.Field("Патрон", ammo);
			fields += "," + PvPLogDiscord.Field("Куда", PvPLogUtils.ZoneLabel(zone));
			fields += "," + PvPLogDiscord.Field("Урон", PvPLogFmt.F1(damage) + " HP (осталось " + PvPLogFmt.F1(healthLeft) + ")");
			fields += "," + PvPLogDiscord.Field("Дистанция", PvPLogFmt.F1(distance) + " м");
			fields += "," + PvPLogDiscord.Field("Способ", PvPLogUtils.MethodLabel(hit.Method));
			fields += "," + PvPLogDiscord.Field("Атакующий", hit.AttackerName + "\n" + hit.AttackerId + "\n" + PvPLogFmt.Pos(hit.AttackerPos), false);
			fields += "," + PvPLogDiscord.Field("Цель", victimName + "\n" + victimId + "\n" + PvPLogFmt.Pos(victimPos), false);
			m_Discord.Enqueue(m_Settings.DiscordHitsWebhook, Embed("Попадание", "**" + hit.AttackerName + "** → **" + victimName + "**", 15105570, fields));
		}

		if (m_Settings.VPPHitAlerts)
			PvPLogVPP.NotifyAdmins(PvPLogVPP.PERM_HIT_ALERTS, "PvP: попадание", shortText);
	}

	// ---------------------------------------------------------------- deaths

	void OnKilled(PlayerBase victim, Object killerObject)
	{
		if (!victim || victim.m_PvPLog_DeathHandled || !m_Settings.LogKills)
			return;

		PvPLogPendingKill kill = new PvPLogPendingKill();
		kill.Victim = victim;
		kill.VictimName = PvPLogUtils.NameOf(victim);
		kill.VictimId = PvPLogUtils.IdOf(victim);
		kill.VictimPos = victim.GetPosition();
		kill.Time = GetGame().GetTime();

		PvPLogAttacker attacker = PvPLogUtils.ResolveAttacker(EntityAI.Cast(killerObject), -1);
		if (attacker && attacker.Player != victim)
		{
			kill.Killer = attacker.Player;
			kill.KillerName = PvPLogUtils.NameOf(attacker.Player);
			kill.KillerId = PvPLogUtils.IdOf(attacker.Player);
			kill.KillerPos = attacker.Player.GetPosition();
			kill.Weapon = attacker.Weapon;
			kill.WeaponName = attacker.WeaponName;
			kill.Method = attacker.Method;
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
			if (now - kill.Time < DEATH_DELAY_MS)
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
		if (kill.Victim)
		{
			kill.Victim.m_PvPLog_DeathHandled = true;
			lastHit = kill.Victim.m_PvPLog_LastHit;
		}

		bool indirect = false;
		int secondsSinceHit = -1;
		if (lastHit)
			secondsSinceHit = (kill.Time - lastHit.Time) / 1000;

		// no player killer (bled out, fell, ...): blame the last player who hit the victim, if recent enough
		if (kill.KillerId == "" && kill.KillerName == "")
		{
			if (!lastHit || m_Settings.KillAttributionWindowSec <= 0 || secondsSinceHit > m_Settings.KillAttributionWindowSec)
				return;

			indirect = true;
			kill.Killer = lastHit.Attacker;
			kill.KillerName = lastHit.AttackerName;
			kill.KillerId = lastHit.AttackerId;
			kill.KillerPos = lastHit.AttackerPos;
			if (lastHit.Attacker && lastHit.Attacker.IsAlive())
				kill.KillerPos = lastHit.Attacker.GetPosition();
		}

		// the fatal hit tells the zone and ammo; it also names the weapon better than the killer object does
		string zone = "";
		string ammo = "";
		if (lastHit && lastHit.AttackerId == kill.KillerId && lastHit.AttackerName == kill.KillerName)
		{
			zone = lastHit.Zone;
			ammo = lastHit.Ammo;
			kill.Weapon = lastHit.Weapon;
			kill.WeaponName = lastHit.WeaponName;
			kill.Method = lastHit.Method;
		}

		float distance = vector.Distance(kill.KillerPos, kill.VictimPos);
		string time = PvPLogFmt.Now();

		string text = "[" + time + "] KILL | Убийца: " + kill.KillerName + " (" + kill.KillerId + ") [" + PvPLogFmt.Pos(kill.KillerPos) + "]";
		text += " | Жертва: " + kill.VictimName + " (" + kill.VictimId + ") [" + PvPLogFmt.Pos(kill.VictimPos) + "]";
		text += " | " + kill.WeaponName + " (" + kill.Weapon + ") | " + ammo + " | " + PvPLogUtils.MethodLabel(kill.Method);
		text += " | " + PvPLogUtils.ZoneLabel(zone) + " | " + PvPLogFmt.F1(distance) + " м";
		if (indirect)
			text += " | косвенно: умер через " + secondsSinceHit + " с после попадания";

		PvPLogJson json = new PvPLogJson();
		json.Str("kind", "kill");
		json.Str("time", time);
		json.Str("killer", kill.KillerName);
		json.Str("killerId", kill.KillerId);
		json.Vec("killerPos", kill.KillerPos);
		json.Str("victim", kill.VictimName);
		json.Str("victimId", kill.VictimId);
		json.Vec("victimPos", kill.VictimPos);
		json.Str("weapon", kill.Weapon);
		json.Str("weaponName", kill.WeaponName);
		json.Str("ammo", ammo);
		json.Str("method", kill.Method);
		json.Str("zone", zone);
		json.Num("distance", distance);
		json.Bool("indirect", indirect);
		if (indirect)
			json.Int("secondsSinceHit", secondsSinceHit);

		Write(text, json.Build());

		string shortText = kill.KillerName + " убил " + kill.VictimName + " | " + kill.WeaponName + " | " + PvPLogUtils.ZoneLabel(zone) + " | " + PvPLogFmt.F1(distance) + " м";
		if (indirect)
			shortText += " (косвенно)";

		AddHistory("[" + time.Substring(11, 8) + "] " + shortText);

		if (m_Settings.DiscordKillsWebhook != "")
		{
			string fields = PvPLogDiscord.Field("Оружие", kill.WeaponName + " (" + kill.Weapon + ")");
			fields += "," + PvPLogDiscord.Field("Патрон", ammo);
			fields += "," + PvPLogDiscord.Field("Куда", PvPLogUtils.ZoneLabel(zone));
			fields += "," + PvPLogDiscord.Field("Дистанция", PvPLogFmt.F1(distance) + " м");
			fields += "," + PvPLogDiscord.Field("Способ", PvPLogUtils.MethodLabel(kill.Method));
			if (indirect)
				fields += "," + PvPLogDiscord.Field("Косвенно", "умер через " + secondsSinceHit + " с после попадания");

			fields += "," + PvPLogDiscord.Field("Убийца", kill.KillerName + "\n" + kill.KillerId + "\nКоординаты: " + PvPLogFmt.Pos(kill.KillerPos), false);
			fields += "," + PvPLogDiscord.Field("Жертва", kill.VictimName + "\n" + kill.VictimId + "\nКоординаты: " + PvPLogFmt.Pos(kill.VictimPos), false);
			m_Discord.Enqueue(m_Settings.DiscordKillsWebhook, Embed("Убийство", "**" + kill.KillerName + "** убил **" + kill.VictimName + "**", 15158332, fields));
		}

		if (m_Settings.VPPKillAlerts)
			PvPLogVPP.NotifyAdmins(PvPLogVPP.PERM_KILL_ALERTS, "PvP: убийство", shortText);
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
