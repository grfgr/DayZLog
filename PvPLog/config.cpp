class CfgPatches
{
	class PvPLog
	{
		units[] = {};
		weapons[] = {};
		requiredVersion = 0.1;
		// VPPAdminTools is a hard dependency: permissions, notifications, chat command, grenade/trap activators
		requiredAddons[] = {"DZ_Data", "DZ_Scripts", "DZM_VPPAdminToolsScripts"};
	};
};

class CfgMods
{
	class PvPLog
	{
		dir = "PvPLog";
		name = "PvPLog";
		credits = "";
		author = "";
		authorID = "0";
		version = "1.0.0";
		extra = 0;
		type = "mod";
		hideName = 1;
		hidePicture = 1;
		picture = "";
		action = "";
		dependencies[] = {"Game", "World", "Mission"};

		class defs
		{
			class gameScriptModule
			{
				value = "";
				files[] = {"PvPLog/scripts/3_Game"};
			};
			class worldScriptModule
			{
				value = "";
				files[] = {"PvPLog/scripts/4_World"};
			};
			class missionScriptModule
			{
				value = "";
				files[] = {"PvPLog/scripts/5_Mission"};
			};
		};
	};
};
