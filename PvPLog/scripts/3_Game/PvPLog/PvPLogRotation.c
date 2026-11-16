/*
	Log rotation: one file per day (PvP_YYYY-MM-DD.log / .jsonl), files older than LogRetentionDays are deleted.
	Runs at server start and whenever the date changes while the server is running.
	Only files named PvP_YYYY-MM-DD.* are ever touched.
*/
class PvPLogRotation
{
	const static string PREFIX = "PvP_";

	// returns how many files were deleted
	static int DeleteOlderThan(string dir, int keepDays)
	{
		if (keepDays <= 0)
			return 0;

		int year, month, day;
		GetYearMonthDay(year, month, day);
		int today = DayNumber(year, month, day);

		array<string> names = new array<string>();
		string fileName;
		FileAttr fileAttr;
		FindFileHandle handle = FindFile(dir + "/" + PREFIX + "*", fileName, fileAttr, FindFileFlags.ALL);
		if (handle)
		{
			names.Insert(fileName);
			while (FindNextFile(handle, fileName, fileAttr))
			{
				names.Insert(fileName);
			}

			CloseFindFile(handle);
		}

		int deleted = 0;
		foreach (string name : names)
		{
			string baseName = BaseName(name);
			int fileDay = DayOfFile(baseName);
			if (fileDay < 0)
				continue;

			if (today - fileDay > keepDays)
			{
				if (DeleteFile(dir + "/" + baseName))
					deleted++;
			}
		}

		if (deleted > 0)
			Print("[PvPLog] rotation: deleted " + deleted + " log file(s) older than " + keepDays + " days");

		return deleted;
	}

	// "PvP_2026-11-16.log" -> day number, -1 when the name does not match
	static int DayOfFile(string baseName)
	{
		if (baseName.IndexOf(PREFIX) != 0 || baseName.Length() < PREFIX.Length() + 10)
			return -1;

		string date = baseName.Substring(PREFIX.Length(), 10);
		if (date.Get(4) != "-" || date.Get(7) != "-")
			return -1;

		int year = date.Substring(0, 4).ToInt();
		int month = date.Substring(5, 2).ToInt();
		int day = date.Substring(8, 2).ToInt();
		if (year < 2000 || month < 1 || month > 12 || day < 1 || day > 31)
			return -1;

		return DayNumber(year, month, day);
	}

	// FindFile may return a bare name or a path depending on the game version
	static string BaseName(string path)
	{
		string result = path;
		result.Replace("\\", "/");
		int slash = result.LastIndexOf("/");
		if (slash >= 0)
			result = result.Substring(slash + 1, result.Length() - slash - 1);

		return result;
	}

	// days since 1970-01-01 (proleptic Gregorian calendar)
	static int DayNumber(int year, int month, int day)
	{
		if (month <= 2)
			year--;

		int era = year / 400;
		int yearOfEra = year - era * 400;
		int monthIndex = month - 3;
		if (month <= 2)
			monthIndex = month + 9;

		int dayOfYear = (153 * monthIndex + 2) / 5 + day - 1;
		int dayOfEra = yearOfEra * 365 + yearOfEra / 4 - yearOfEra / 100 + dayOfYear;
		return era * 146097 + dayOfEra - 719468;
	}
};
