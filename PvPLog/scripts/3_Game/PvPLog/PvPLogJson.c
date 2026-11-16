// Minimal JSON object builder (keeps key names free from Enforce keywords and escapes player names).
class PvPLogJson
{
	protected string m_Body;
	protected bool m_Empty;

	void PvPLogJson()
	{
		m_Body = "";
		m_Empty = true;
	}

	protected void Key(string key)
	{
		if (!m_Empty)
			m_Body += ",";

		m_Body += "\"" + Esc(key) + "\":";
		m_Empty = false;
	}

	PvPLogJson Str(string key, string value)
	{
		Key(key);
		m_Body += "\"" + Esc(value) + "\"";
		return this;
	}

	PvPLogJson Num(string key, float value)
	{
		Key(key);
		m_Body += PvPLogFmt.F1(value);
		return this;
	}

	PvPLogJson Int(string key, int value)
	{
		Key(key);
		m_Body += value.ToString();
		return this;
	}

	PvPLogJson Bool(string key, bool value)
	{
		Key(key);
		if (value)
			m_Body += "true";
		else
			m_Body += "false";

		return this;
	}

	PvPLogJson Vec(string key, vector value)
	{
		Key(key);
		m_Body += "[" + PvPLogFmt.F1(value[0]) + "," + PvPLogFmt.F1(value[1]) + "," + PvPLogFmt.F1(value[2]) + "]";
		return this;
	}

	// value is already valid JSON (object / array)
	PvPLogJson Raw(string key, string json)
	{
		Key(key);
		m_Body += json;
		return this;
	}

	string Build()
	{
		return "{" + m_Body + "}";
	}

	static string Esc(string value)
	{
		string result = value;
		result.Replace("\\", "\\\\");
		result.Replace("\"", "\\\"");
		result.Replace("\r", "");
		result.Replace("\n", "\\n");
		result.Replace("\t", " ");
		return result;
	}
};

class PvPLogFmt
{
	// one decimal: 42.5
	static string F1(float value)
	{
		float rounded = Math.Round(value * 10) / 10;
		return rounded.ToString();
	}

	// "x, y, z" (y = height)
	static string Pos(vector pos)
	{
		return F1(pos[0]) + ", " + F1(pos[1]) + ", " + F1(pos[2]);
	}

	// server local time: 2026-11-16 21:43:12
	static string Now()
	{
		int year, month, day, hour, minute, second;
		GetYearMonthDay(year, month, day);
		GetHourMinuteSecond(hour, minute, second);
		return string.Format("%1-%2-%3 %4:%5:%6", year, month.ToStringLen(2), day.ToStringLen(2), hour.ToStringLen(2), minute.ToStringLen(2), second.ToStringLen(2));
	}

	static string Today()
	{
		int year, month, day;
		GetYearMonthDay(year, month, day);
		return string.Format("%1-%2-%3", year, month.ToStringLen(2), day.ToStringLen(2));
	}

	// UTC ISO 8601 for Discord embeds (Discord shows it in each reader's own time zone)
	static string IsoUtc()
	{
		int year, month, day, hour, minute, second;
		GetYearMonthDayUTC(year, month, day);
		GetHourMinuteSecondUTC(hour, minute, second);
		return string.Format("%1-%2-%3T%4:%5:%6.000Z", year, month.ToStringLen(2), day.ToStringLen(2), hour.ToStringLen(2), minute.ToStringLen(2), second.ToStringLen(2));
	}
};
