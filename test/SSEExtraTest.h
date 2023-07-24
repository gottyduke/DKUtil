#pragma once


#include "DKUtil/Extra.hpp"


namespace Test::Extra
{
	struct ActorDeathInfo
	{
		RE::Actor* actor;
		std::string reason;
	};


	void Run()
	{
		dku::serializable<std::map<RE::FormID, ActorDeathInfo>, "MapInfo"> actorDeathInfoMapSerial({
			{ 0, { nullptr, "testA" } },
			{ 1, { nullptr, "testB" } },
			{ 2, { nullptr, "testC" } },
		});
		dku::serializable<ActorDeathInfo, "SingleInfo"> actorDeathInfoSerial({ nullptr, "test" });

		dku::serializable<std::set<ActorDeathInfo>, "DeathInfoMap"> actorDeathInfoMap8;

		std::tuple<void*, std::string> testTuple(nullptr, "test");

		dku::serialization::ISerializable::SaveAll(nullptr);
		//dku::serialization::ISerializable::RevertAll(nullptr);
		//dku::serialization::ISerializable::LoadAll(nullptr);

		for (auto& [f, s] : *actorDeathInfoMapSerial) {
			INFO("{} {}", f, s.reason);
		}

		int a[] = { 1, 2, 3, 4, 5 };
		auto as = dku::model::vector_cast(a);
		INFO("{}", as.size());

		constexpr auto ss = dku::model::concepts::dku_aggregate<ActorDeathInfo>;
	}
} // namespace Test::Extra
