modded class MissionServer
{
	override void OnInit()
	{
		super.OnInit();
		// load settings.json and start the Discord queue at boot instead of on the first hit
		PvPLog.Get();
	}
};
