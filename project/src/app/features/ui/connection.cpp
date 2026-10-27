#include "connection.hpp"
#include "widgets.hpp"

#include "../cfg.hpp"

static void pushHistory(std::vector<float> &history, const float val)
{
	if (history.size() >= Connection::history_size) {
		history.erase(history.begin());
	}

	history.push_back(val);
}

void Connection::update(const float now)
{
	if (now < m_next_sample) {
		return;
	}

	m_next_sample = now + 0.25f;

	addSample(currentConnection());
}

void Connection::addSample(const ConnectionSample &sample)
{
	//a different server starts the graph over
	if (!sample.connected || sample.address != m_last.address || sample.map != m_last.map) {
		m_ping.clear();
		m_loss.clear();
		m_choke.clear();
	}

	m_last = sample;

	if (!sample.connected || sample.loopback) {
		return;
	}

	pushHistory(m_ping, static_cast<float>(sample.ping));
	pushHistory(m_loss, std::max(sample.loss_in, sample.loss_out) * 100.0f);
	pushHistory(m_choke, std::max(sample.choke_in, sample.choke_out) * 100.0f);
}

const ConnectionSample &Connection::last() const
{
	return m_last;
}

const std::vector<float> &Connection::pingHistory() const
{
	return m_ping;
}

const std::vector<float> &Connection::lossHistory() const
{
	return m_loss;
}

const std::vector<float> &Connection::chokeHistory() const
{
	return m_choke;
}

std::string Connection::summary() const
{
	if (!m_last.connected) {
		return {};
	}

	if (m_last.loopback) {
		return std::format("local  {:.0f} tick", m_last.tickrate);
	}

	const float loss{ std::max(m_last.loss_in, m_last.loss_out) * 100.0f };
	const float choke{ std::max(m_last.choke_in, m_last.choke_out) * 100.0f };

	return std::format("{} ms  {:.0f} tick  {:.0f}% loss  {:.0f}% choke", m_last.ping, m_last.tickrate, loss, choke);
}

bool Connection::bad() const
{
	if (!m_last.connected || m_last.loopback) {
		return false;
	}

	return m_last.ping >= 150 || std::max(m_last.loss_in, m_last.loss_out) >= 0.02f || std::max(m_last.choke_in, m_last.choke_out) >= 0.1f;
}

static std::string formatDuration(const float seconds)
{
	const int total{ static_cast<int>(seconds) };

	if (total >= 3600) {
		return std::format("{}h {}m", total / 3600, total % 3600 / 60);
	}

	return std::format("{}m {}s", total / 60, total % 60);
}

static void graph(const char *const id, const char *const name, const std::vector<float> &history, const float min_max, const char *const unit)
{
	if (history.empty()) {
		return;
	}

	const float max{ std::max(*std::max_element(history.begin(), history.end()), min_max) };

	ImGui::PlotLines(id, history.data(), static_cast<int>(history.size()), 0, std::format("{} {:.0f}{}", name, history.back(), unit).c_str(),
					 0.0f, max * 1.2f, { ImGui::GetFontSize() * 18.0f, ImGui::GetFontSize() * 3.0f });
}

void Connection::drawFooter(const float right_edge)
{
	const std::string text{ summary() };

	if (text.empty()) {
		return;
	}

	ImGui::SameLine(right_edge - ImGui::CalcTextSize(text.c_str()).x);

	if (bad()) {
		ImGui::TextColored(warningColor(), "%s", text.c_str());
	}

	else {
		ImGui::TextDisabled("%s", text.c_str());
	}

	if (!ImGui::IsItemHovered(ImGuiHoveredFlags_ForTooltip)) {
		return;
	}

	ImGui::BeginTooltip();
	{
		const ConnectionSample &s{ m_last };

		if (!s.map.empty()) {
			ImGui::Text("map: %s", s.map.c_str());
		}

		if (!s.server.empty()) {
			ImGui::Text("server: %s", s.server.c_str());
		}

		if (!s.address.empty()) {
			ImGui::Text("address: %s", s.address.c_str());
		}

		ImGui::Text("tickrate: %.0f", s.tickrate);

		if (!s.loopback)
		{
			ImGui::Text("connected for %s", formatDuration(s.time_connected).c_str());

			ImGui::Separator();

			ImGui::Text("ping: %d ms (latency out %.0f ms, in %.0f ms)", s.ping, s.latency_out * 1000.0f, s.latency_in * 1000.0f);
			ImGui::Text("loss: out %.1f%%, in %.1f%%", s.loss_out * 100.0f, s.loss_in * 100.0f);
			ImGui::Text("choke: out %.1f%%, in %.1f%%", s.choke_out * 100.0f, s.choke_in * 100.0f);
			ImGui::Text("data: out %.1f KB/s, in %.1f KB/s", s.data_out / 1024.0f, s.data_in / 1024.0f);

			ImGui::Separator();

			graph("##ping_graph", "ping", m_ping, 50.0f, " ms");
			graph("##loss_graph", "loss", m_loss, 5.0f, "%");
			graph("##choke_graph", "choke", m_choke, 5.0f, "%");
		}
	}
	ImGui::EndTooltip();
}
