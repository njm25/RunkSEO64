#pragma once

#include "../../../game/game.hpp"

struct ConnectionSample final
{
	bool connected{};
	bool loopback{}; //listen server or demo, nothing worth showing
	int ping{}; //what the scoreboard shows, from the player resource
	float latency_out{}; //seconds
	float latency_in{};
	float loss_out{}; //0 to 1
	float loss_in{};
	float choke_out{};
	float choke_in{};
	float data_out{}; //bytes a second
	float data_in{};
	float tickrate{};
	float time_connected{}; //seconds
	std::string map{};
	std::string address{};
	std::string server{};
};

//game side, in game_info.cpp
ConnectionSample currentConnection();

//ping, tickrate, loss and choke for the menu footer, with a short history for the graph
class Connection final
{
public:
	static constexpr size_t history_size{ 120 }; //30 seconds at 4 samples a second

private:
	ConnectionSample m_last{};
	std::vector<float> m_ping{};
	std::vector<float> m_loss{};
	std::vector<float> m_choke{};
	float m_next_sample{};

public:
	//samples every quarter second, now is in seconds
	void update(const float now);
	void addSample(const ConnectionSample &sample);

	const ConnectionSample &last() const;
	const std::vector<float> &pingHistory() const;
	const std::vector<float> &lossHistory() const;
	const std::vector<float> &chokeHistory() const;

	std::string summary() const; //"45 ms  66 tick  0% loss  0% choke"
	bool bad() const; //worth drawing in the warning color

	void drawFooter(const float right_edge);
};

MAKE_UNIQUE(Connection, connection);
