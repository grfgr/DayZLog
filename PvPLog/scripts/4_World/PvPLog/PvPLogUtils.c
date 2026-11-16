// Who is behind a hit: the player, what they used and how.
class PvPLogAttacker
{
	PlayerBase Player;
	string Weapon;       // class name: M4A1, KitchenKnife, M67Grenade ...
	string WeaponName;   // display name
	string Method;       // firearm / melee / explosion / trap / vehicle / other
};

// Last player hit a victim took; used for the kill (zone, ammo) and for deaths without a direct killer.
class PvPLogHitInfo
{
	PlayerBase Attacker;
	string AttackerName;
	string AttackerId;
	vector AttackerPos;
	string Weapon;
	string WeaponName;
	string Ammo;
	string Zone;
	string Method;
	int Time;            // GetGame().GetTime(), ms
};

class PvPLogUtils
{
	static PvPLogAttacker ResolveAttacker(EntityAI source, int damageType)
	{
		if (!source)
			return null;

		PvPLogAttacker attacker = new PvPLogAttacker();
		attacker.Method = MethodOf(damageType);
		attacker.Weapon = source.GetType();
		attacker.WeaponName = source.GetDisplayName();

		PlayerBase player = PlayerBase.Cast(source);
		Grenade_Base grenade = Grenade_Base.Cast(source);
		TrapBase trap = TrapBase.Cast(source);
		Transport vehicle = Transport.Cast(source);

		if (player)
		{
			// bare hands, or a melee hit made with whatever is in hands (rifle butt etc.)
			attacker.Player = player;
			EntityAI inHands = player.GetHumanInventory().GetEntityInHands();
			if (inHands)
			{
				attacker.Weapon = inHands.GetType();
				attacker.WeaponName = inHands.GetDisplayName();
			}
			else
			{
				attacker.Weapon = "Fists";
				attacker.WeaponName = "Кулаки";
			}
		}
		else if (grenade)
		{
			// VPP remembers who pulled the pin
			attacker.Player = grenade.GetActivator();
			attacker.Method = "explosion";
		}
		else if (trap)
		{
			// VPP remembers who set the trap / mine
			attacker.Player = trap.GetActivator();
			attacker.Method = "trap";
		}
		else if (vehicle)
		{
			attacker.Player = PlayerBase.Cast(vehicle.CrewMember(0));
			attacker.Method = "vehicle";
		}
		else
		{
			// weapon, knife, projectile: whoever holds it
			attacker.Player = PlayerBase.Cast(source.GetHierarchyRootPlayer());
		}

		if (!attacker.Player)
			return null;

		return attacker;
	}

	static string MethodOf(int damageType)
	{
		if (damageType == DT_FIRE_ARM)
			return "firearm";

		if (damageType == DT_CLOSE_COMBAT)
			return "melee";

		if (damageType == DT_EXPLOSION)
			return "explosion";

		return "other";
	}

	static string MethodLabel(string method)
	{
		switch (method)
		{
			case "firearm": return "огнестрел";
			case "melee": return "ближний бой";
			case "explosion": return "взрыв";
			case "trap": return "ловушка";
			case "vehicle": return "транспорт";
		}

		return "другое";
	}

	static string NameOf(PlayerBase player)
	{
		if (!player)
			return "?";

		PlayerIdentity identity = player.GetIdentity();
		if (identity)
			return identity.GetName();

		// cached by VPP on connect, still there after the identity is gone
		string cached = player.VPlayerGetName();
		if (cached != "")
			return cached;

		// no player behind it: a spawned survivor dummy or an AI
		return "Bot (" + player.GetType() + ")";
	}

	static string IdOf(PlayerBase player)
	{
		if (!player)
			return "";

		PlayerIdentity identity = player.GetIdentity();
		if (identity)
			return identity.GetPlainId();

		return player.VPlayerGetSteamId();
	}

	static string ZoneLabel(string zone)
	{
		string key = zone;
		key.ToLower();
		switch (key)
		{
			case "head": return "Голова";
			case "brain": return "Голова (мозг)";
			case "torso": return "Торс";
			case "leftarm": return "Левая рука";
			case "rightarm": return "Правая рука";
			case "lefthand": return "Левая кисть";
			case "righthand": return "Правая кисть";
			case "leftleg": return "Левая нога";
			case "rightleg": return "Правая нога";
			case "leftfoot": return "Левая стопа";
			case "rightfoot": return "Правая стопа";
			case "": return "-";
		}

		return zone;
	}
};
