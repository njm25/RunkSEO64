#include "freecam.hpp"

#include "../cfg.hpp"

bool Freecam::onLevelInit()
{
	m_flying = false;

	return true;
}

bool Freecam::overrideView(CViewSetup *const setup)
{
	C_TFPlayer *const local{ ec->getLocal() };

	if (!cfg::freecam_active || !setup || !local || !local->deadflag())
	{
		m_flying = false;
		return false;
	}

	//starts where the death cam was looking from
	if (!m_flying) {
		m_pos = setup->origin;
		m_flying = true;
	}

	const QAngle angles{ i::engine->GetViewAngles() };

	vec3 forward{};
	vec3 right{};
	vec3 up{};

	math::angleVectors(angles, &forward, &right, &up);

	vec3 move{};

	if (input_game->getKeyInGame('W').held) {
		move += forward;
	}

	if (input_game->getKeyInGame('S').held) {
		move -= forward;
	}

	if (input_game->getKeyInGame('D').held) {
		move += right;
	}

	if (input_game->getKeyInGame('A').held) {
		move -= right;
	}

	if (input_game->getKeyInGame(VK_SPACE).held) {
		move += vec3{ 0.0f, 0.0f, 1.0f };
	}

	if (input_game->getKeyInGame(VK_CONTROL).held) {
		move -= vec3{ 0.0f, 0.0f, 1.0f };
	}

	if (!move.isZero())
	{
		move.normalize();

		const float speed{ cfg::freecam_speed * (input_game->getKeyInGame(VK_SHIFT).held ? 3.0f : 1.0f) };

		m_pos += move * (speed * i::global_vars->absoluteframetime);
	}

	setup->origin = m_pos;
	setup->angles = angles;

	return true;
}

bool Freecam::flying() const
{
	return m_flying;
}
