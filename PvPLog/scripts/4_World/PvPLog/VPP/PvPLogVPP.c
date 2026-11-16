/*
	VPPAdminTools integration:
	- permissions (they appear in the VPP admin menu, Permissions tab, after the first start):
		PvPLog:KillAlerts   - in-game notification on every PvP kill
		PvPLog:HitAlerts    - in-game notification on every PvP hit (noisy; also needs VPPHitAlerts in settings.json)
		PvPLog:ChatHistory  - chat command /pvplog [count]
	- grenade / trap activators come from VPP (Grenade_Base / TrapBase GetActivator).
*/
class PvPLogVPP
{
	const static string PERM_KILL_ALERTS = "PvPLog:KillAlerts";
	const static string PERM_HIT_ALERTS = "PvPLog:HitAlerts";
	const static string PERM_CHAT = "PvPLog:ChatHistory";

	static void NotifyAdmins(string permission, string title, string text)
	{
		PermissionManager permissions = GetPermissionManager();
		if (!permissions)
			return;

		array<Man> players = new array<Man>();
		GetGame().GetPlayers(players);
		foreach (Man man : players)
		{
			PlayerIdentity identity = man.GetIdentity();
			if (!identity)
				continue;

			string steamId = identity.GetPlainId();
			// regular players: skip before VerifyPermission, which logs every rejection
			if (!permissions.HasUserGroup(steamId) && !permissions.IsSuperAdmin(steamId))
				continue;

			if (!permissions.VerifyPermission(steamId, permission, "", false))
				continue;

			NotificationSystem.SendNotificationToPlayerIdentityExtended(identity, 10.0, title, text, "set:ccgui_enforce image:MapUserMarker");
		}
	}

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
		GetPermissionManager().AddPermissionType({PvPLogVPP.PERM_KILL_ALERTS, PvPLogVPP.PERM_HIT_ALERTS});
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
