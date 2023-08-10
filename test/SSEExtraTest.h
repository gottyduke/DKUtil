#pragma once


#include "DKUtil/Extra.hpp"


namespace Test::Extra
{
	struct ActorDeathInfo
	{
		int actor;
		std::string reason;
	};


	void Run()
	{
		/**/
		dku::serializable<std::map<RE::FormID, ActorDeathInfo>, "MapInfo"> actorDeathInfoMapSerial({
			{ 0, { 11, "testA" } },
			{ 1, { 22, "testB" } },
			{ 2, { 33, "testC" } },
		}); /**/
		dku::serializable<ActorDeathInfo, "SingleInfo"> actorDeathInfoSerial({ 1234, "test" });

		/**
		dku::serializable<std::map<int, int>, "MapInfo"> actorDeathInfoMapSerial({
			{ 0, 1 },
			{ 1, 2 },
			{ 2, 3 },
		}); /**/

		std::tuple<void*, std::string> testTuple(nullptr, "test");
		std::pair<const int, int> as = std::make_tuple(1, 2);

		for (auto& [f, s] : *actorDeathInfoMapSerial) {
			INFO("{} {}", f, s.reason);
		}
		INFO("{}", actorDeathInfoSerial->reason);

		dku::serialization::api::detail::save_all(nullptr);
		dku::serialization::api::detail::revert_all(nullptr);
		dku::serialization::api::detail::load_all(nullptr);

		for (auto& [f, s] : *actorDeathInfoMapSerial) {
			INFO("{} {}", f, s.reason);
		}
		INFO("{}", actorDeathInfoSerial->reason);

		constexpr auto ss = dku::model::concepts::dku_aggregate<ActorDeathInfo>;
	}
} // namespace Test::Extra
