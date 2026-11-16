class PvPLogRestCallback : RestCallback
{
	override void OnError(int errorCode)
	{
		Print("[PvPLog] Discord webhook error: " + errorCode);
	}

	// Discord answers 204 without a body, which the engine reports as a timeout: not an error.
	override void OnTimeout()
	{
	}

	override void OnSuccess(string data, int dataSize)
	{
	}
};

/*
	Queues embeds per webhook URL and sends up to EMBEDS_PER_MESSAGE of them in one message every FLUSH_MS,
	which keeps a webhook under Discord's limit (30 messages / minute) even during a big fight.
*/
class PvPLogDiscord
{
	const static int FLUSH_MS = 2500;
	const static int EMBEDS_PER_MESSAGE = 5;
	const static int MAX_QUEUE = 200;

	protected ref map<string, ref array<string>> m_Queue;
	protected ref PvPLogRestCallback m_Callback;
	protected string m_Username;

	void PvPLogDiscord(string username)
	{
		m_Queue = new map<string, ref array<string>>();
		m_Callback = new PvPLogRestCallback();
		m_Username = username;
		GetGame().GetCallQueue(CALL_CATEGORY_SYSTEM).CallLater(Flush, FLUSH_MS, true);
	}

	void ~PvPLogDiscord()
	{
		if (GetGame())
			GetGame().GetCallQueue(CALL_CATEGORY_SYSTEM).Remove(Flush);
	}

	void Enqueue(string url, string embedJson)
	{
		if (url == "")
			return;

		array<string> queue = m_Queue.Get(url);
		if (!queue)
		{
			queue = new array<string>();
			m_Queue.Set(url, queue);
		}

		if (queue.Count() >= MAX_QUEUE)
			queue.RemoveOrdered(0);

		queue.Insert(embedJson);
	}

	void Flush()
	{
		foreach (string url, array<string> queue : m_Queue)
		{
			if (!queue || queue.Count() == 0)
				continue;

			string embeds = "";
			int count = Math.Min(EMBEDS_PER_MESSAGE, queue.Count());
			for (int i = 0; i < count; i++)
			{
				if (i > 0)
					embeds += ",";

				embeds += queue[0];
				queue.RemoveOrdered(0);
			}

			PvPLogJson body = new PvPLogJson();
			body.Str("username", m_Username);
			body.Raw("embeds", "[" + embeds + "]");
			Send(url, body.Build());
		}
	}

	protected void Send(string url, string body)
	{
		RestApi api = GetRestApi();
		if (!api)
			api = CreateRestApi();

		if (!api)
			return;

		RestContext ctx = api.GetRestContext(url);
		if (!ctx)
			return;

		ctx.SetHeader("application/json");
		ctx.POST(m_Callback, "", body);
	}

	// {"name":..,"value":..,"inline":..}
	static string Field(string name, string value, bool isInline = true)
	{
		if (value == "")
			value = "-";

		PvPLogJson field = new PvPLogJson();
		field.Str("name", name);
		field.Str("value", value);
		field.Bool("inline", isInline);
		return field.Build();
	}
};
