/*
	VPPAdminTools integration:
	- permission PvPLog:ChatHistory (VPP admin menu, Permissions tab) for the chat command /pvplog [count]
	- grenade / trap activators come from VPP (Grenade_Base / TrapBase GetActivator).
*/
class PvPLogVPP
{
	const static string PERM_CHAT = "PvPLog:ChatHistory";

	// private line in the player's chat
	static void SendChat(PlayerBase player, string text)
	{
		if (!player || !player.GetIdentity())
			return;

		GetGame().RPCSingleParam(player, ERPCs.RPC_USER_ACTION_MESSAGE, new Param1<string>(text), true, player.GetIdentity());
	}
};

modded class ChatCommandManager
{
	override void OnInit()
	{
		super.OnInit();
		// the ChatCommand constructor registers PERM_CHAT itself
		AddChatCommand(new PvPLogChatModule("/pvplog", 0, PvPLogVPP.PERM_CHAT));
	}
};

// /pvplog      - last 5 PvP events
// /pvplog 15   - last 15 (max 20)
class PvPLogChatModule extends ChatCommand
{
	override void ExecuteCommand(PlayerBase caller, array<Man> targets, array<string> args)
	{
		if (!caller)
			return;

		int count = 5;
		if (args && args.Count() > 0)
		{
			string arg = args[0];
			arg = arg.Trim();
			if (arg.ToInt() > 0)
				count = arg.ToInt();
		}

		if (count > 20)
			count = 20;

		array<string> lines = PvPLog.Get().GetHistory(count);
		if (lines.Count() == 0)
		{
			PvPLogVPP.SendChat(caller, "[PvPLog] PvP событий пока нет");
			return;
		}

		PvPLogVPP.SendChat(caller, "[PvPLog] последние " + lines.Count() + ":");
		foreach (string line : lines)
		{
			PvPLogVPP.SendChat(caller, line);
		}
	}
};
