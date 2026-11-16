/*
	$profile:PvPLog/settings.json, created with defaults on the first start.
	The file is saved back after loading, so options added in newer versions appear in it automatically.
*/
class PvPLogSettings
{
	int ConfigVersion = 1;

	// what to log
	bool LogHits = true;
	bool LogKills = true;

	// files in $profile:PvPLog/logs/
	bool WriteTextLog = true;   // PvP_YYYY-MM-DD.log   - human readable
	bool WriteJsonLog = true;   // PvP_YYYY-MM-DD.jsonl - one JSON object per line, for tools / parsers

	// A death without a player killer (bleeding out, fall, etc.) is still counted as a PvP kill
	// when the victim was hit by a player within this many seconds. 0 = off.
	int KillAttributionWindowSec = 60;

	// Discord webhook for kills ("" = off). Hits are only written to the log files.
	string DiscordKillsWebhook = "";
	string DiscordUsername = "PvP Log";

	// how many recent events the /pvplog chat command can show
	int HistorySize = 50;

	const static string DIR = "$profile:PvPLog";
	const static string LOG_DIR = "$profile:PvPLog/logs";
	const static string FILE = "$profile:PvPLog/settings.json";

	static PvPLogSettings Load()
	{
		if (!FileExist(DIR))
			MakeDirectory(DIR);

		if (!FileExist(LOG_DIR))
			MakeDirectory(LOG_DIR);

		PvPLogSettings settings = new PvPLogSettings();
		if (FileExist(FILE))
			JsonFileLoader<PvPLogSettings>.JsonLoadFile(FILE, settings);

		JsonFileLoader<PvPLogSettings>.JsonSaveFile(FILE, settings);
		return settings;
	}
};
