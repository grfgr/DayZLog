// One request's answer. The engine keeps every callback it is given, so answered callbacks go back to a pool
// and are reused; each request in flight has its own (one shared callback lost every request after the first).
class PvPLogRestCallback : RestCallback
{
	protected PvPLogDiscord m_Owner;

	void Bind(PvPLogDiscord owner)
	{
		m_Owner = owner;
	}

	protected void Done()
	{
		PvPLogDiscord owner = m_Owner;
		m_Owner = null;
		if (owner)
			owner.Release(this);
	}

	override void OnError(int errorCode)
	{
		Print("[PvPLog] Discord webhook error, code " + errorCode + " (check the URL in settings.json)");
		Done();
	}

	// with ?wait=true Discord answers with a body, so this is a real timeout
	override void OnTimeout()
	{
		Print("[PvPLog] Discord webhook timeout");
		Done();
	}

	override void OnSuccess(string data, int dataSize)
	{
		Done();
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
	protected ref array<ref PvPLogRestCallback> m_FreeCallbacks;
	protected ref array<ref PvPLogRestCallback> m_BusyCallbacks;
	protected string m_Username;

	void PvPLogDiscord(string username)
	{
		m_Queue = new map<string, ref array<string>>();
		m_FreeCallbacks = new array<ref PvPLogRestCallback>();
		m_BusyCallbacks = new array<ref PvPLogRestCallback>();
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

		RestContext ctx = api.GetRestContext(SendUrl(url));
		if (!ctx)
			return;

		PvPLogRestCallback callback;
		int last = m_FreeCallbacks.Count() - 1;
		if (last >= 0)
		{
			callback = m_FreeCallbacks[last];
			m_FreeCallbacks.Remove(last);
		}
		else
		{
			callback = new PvPLogRestCallback();
		}

		callback.Bind(this);
		m_BusyCallbacks.Insert(callback);
		ctx.SetHeader("application/json");
		ctx.POST(callback, "", body);
	}

	void Release(PvPLogRestCallback callback)
	{
		int index = m_BusyCallbacks.Find(callback);
		if (index < 0)
			return;

		m_FreeCallbacks.Insert(callback);
		m_BusyCallbacks.Remove(index);
	}

	// Discord answers 204 (no body) by default, which the engine treats as no answer and keeps the request hanging
	// until it times out; with wait=true it answers 200 + the message.
	static string SendUrl(string url)
	{
		if (url.Contains("wait=true"))
			return url;

		if (url.Contains("?"))
			return url + "&wait=true";

		return url + "?wait=true";
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
