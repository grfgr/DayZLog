modded class PlayerBase
{
	ref PvPLogHitInfo m_PvPLog_LastHit;
	// set once the death was handled: hits on the body after that are not logged
	bool m_PvPLog_DeathHandled;

	override void EEHitBy(TotalDamageResult damageResult, int damageType, EntityAI source, int component, string dmgZone, string ammo, vector modelPos, float speedCoef)
	{
		super.EEHitBy(damageResult, damageType, source, component, dmgZone, ammo, modelPos, speedCoef);

		if (GetGame().IsDedicatedServer())
			PvPLog.Get().OnHit(this, damageResult, damageType, source, dmgZone, ammo);
	}

	override void EEKilled(Object killer)
	{
		// before super: identity and positions are still intact
		if (GetGame().IsDedicatedServer())
			PvPLog.Get().OnKilled(this, killer);

		super.EEKilled(killer);
	}
};
