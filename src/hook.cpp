#include "hook.h"
#include "ClibUtil/editorID.hpp"

namespace hooks
{
	void OnMeleeHitHook::Set_iFrames(RE::Actor* actor)
	{
		actor->SetGraphVariableBool("bIframeActive", true);
		actor->SetGraphVariableBool("bInIframe", true);
	}

	void OnMeleeHitHook::Reset_iFrames(RE::Actor* actor)
	{
		actor->SetGraphVariableBool("bIframeActive", false);
		actor->SetGraphVariableBool("bInIframe", false);
	}

	void OnMeleeHitHook::dispelEffect(RE::MagicItem* spellForm, RE::Actor* a_target)
	{
		const auto targetActor = a_target->AsMagicTarget();
		if (targetActor->HasMagicEffect(spellForm->effects[0]->baseEffect)) {
			auto activeEffects = targetActor->GetActiveEffectList();
			for (const auto& effect : *activeEffects) {
				if (effect->spell == spellForm) {
					effect->Dispel(true);
				}
			}
		}
	}

	bool OnMeleeHitHook::Can_Transform(RE::Actor* a_actor)
	{
		auto  tolerant_teammates = true;
		auto  adequate_threat = false;
		float MyTeam_total_threat = 0.0f;
		float EnemyTeam_total_threat = 0.0f;
		auto  combatGroup = a_actor->GetCombatGroup();
		if (combatGroup) {
			for (auto it = combatGroup->members.begin(); it != combatGroup->members.end(); ++it) {
				if (it->memberHandle && it->memberHandle.get().get()) {
					auto Teammate = it->memberHandle.get().get();
					if (Teammate != a_actor && (Teammate->IsGuard() || Teammate->IsInFaction(RE::TESForm::LookupByEditorID<RE::TESFaction>("DLC1DawnguardExteriorGuardFaction")) || Teammate->IsInFaction(RE::TESForm::LookupByEditorID<RE::TESFaction>("DLC1DawnguardFaction")) || Teammate->IsInFaction(RE::TESForm::LookupByEditorID<RE::TESFaction>("VigilantOfStendarrFaction")))) {
						tolerant_teammates = false;
						break;
					}
				}
				continue;
			}
			for (auto it = combatGroup->members.begin(); it != combatGroup->members.end(); ++it) {
				if (it->memberHandle && it->memberHandle.get().get()) {
					//auto Teammate = it->memberHandle.get().get();
					MyTeam_total_threat += it->threatValue;
				}
				continue;
			}
		}
		auto CTarget = a_actor->GetActorRuntimeData().currentCombatTarget.get().get();
		if (CTarget) {
			auto EnemyGroup = CTarget->GetCombatGroup();
			if (EnemyGroup) {
				for (auto it = EnemyGroup->members.begin(); it != EnemyGroup->members.end(); ++it) {
					if (it->memberHandle && it->memberHandle.get().get()) {
						//auto Teammate = it->memberHandle.get().get();
						EnemyTeam_total_threat += it->threatValue;
					}
					continue;
				}
			}
		}

		if (MyTeam_total_threat > 0 && EnemyTeam_total_threat > 0) {
			logger::info("Name {} TeamThreatLVL {}"sv, CTarget->GetName(), (MyTeam_total_threat / EnemyTeam_total_threat));
			if ((MyTeam_total_threat / EnemyTeam_total_threat) <= 0.625f) {
				adequate_threat = true;
			}
		}

		return tolerant_teammates && adequate_threat;
	}

	bool OnMeleeHitHook::isHumanoid(RE::Actor* a_actor)
	{
		auto bodyPartData = a_actor->GetRace() ? a_actor->GetRace()->bodyPartData : nullptr;
		return bodyPartData && bodyPartData->GetFormID() == 0x1d;
	}

	bool OnMeleeHitHook::is_valid_actor(RE::Actor* a_actor)
	{
		bool result = true;
		if (a_actor->IsPlayerRef() || a_actor->IsDead() || !isHumanoid(a_actor) 
		|| !(a_actor->HasKeywordString("ActorTypeNPC") || a_actor->HasKeywordString("DLC2ActorTypeMiraak")) 
		|| a_actor->HasKeywordString("PCG_ExcludeSprintAttacks")){
			result = false;
		}
		return result;
	}

	bool OnMeleeHitHook::getrace_IsWerewolf(RE::Actor* a_actor)
	{
		bool result = false;
		const auto race = a_actor->GetRace();
		const auto raceEDID = race->formEditorID;
		if (raceEDID == "WerewolfBeastRace") {
			if (a_actor->HasKeywordString("TBW_Farkas") || a_actor->HasKeywordString("TBW_Vilkas") || a_actor->HasKeywordString("TBW_Aela")) {
				result = true;
			}
		}
		return result;
	}

	void OnMeleeHitHook::UnequipAll(RE::Actor* a_actor)
	{
		uniqueLocker lock(mtx_Inventory);
		auto         itt = _Inventory.find(a_actor);
		if (itt == _Inventory.end()) {
			std::vector<RE::TESBoundObject*> Hen;
			_Inventory.insert({ a_actor, Hen });
		}

		for (auto it = _Inventory.begin(); it != _Inventory.end(); ++it) {
			if (it->first == a_actor) {
				auto inv = a_actor->GetInventory();
				for (auto& [item, data] : inv) {
					const auto& [count, entry] = data;
					if (count > 0 && entry->IsWorn()) {
						RE::ActorEquipManager::GetSingleton()->UnequipObject(a_actor, item);
						it->second.push_back(item);
					}
				}
				break;
			}
			continue;
		}
	}

	void OnMeleeHitHook::Re_EquipAll(RE::Actor* a_actor)
	{
		uniqueLocker lock(mtx_Inventory);
		for (auto it = _Inventory.begin(); it != _Inventory.end(); ++it) {
			if (it->first == a_actor) {
				for (auto item : it->second) {
					RE::ActorEquipManager::GetSingleton()->EquipObject(a_actor, item);
				}
				_Inventory.erase(it);
				break;
			}
			continue;
		}
	}

	bool OnMeleeHitHook::isPowerAttacking(RE::Actor* a_actor)
	{
		auto currentProcess = a_actor->GetActorRuntimeData().currentProcess;
		if (currentProcess) {
			auto highProcess = currentProcess->high;
			if (highProcess) {
				auto attackData = highProcess->attackData;
				if (attackData) {
					auto flags = attackData->data.flags;
					return flags.any(RE::AttackData::AttackFlag::kPowerAttack);
				}
			}
		}
		return false;
	}

	void OnMeleeHitHook::UpdateCombatTarget(RE::Actor* a_actor){
		auto CTarget = a_actor->GetActorRuntimeData().currentCombatTarget.get().get();
		if (!CTarget) {
			auto combatGroup = a_actor->GetCombatGroup();
			if (combatGroup) {
				for (auto it = combatGroup->targets.begin(); it != combatGroup->targets.end(); ++it) {
					if (it->targetHandle && it->targetHandle.get().get()) {
						a_actor->GetActorRuntimeData().currentCombatTarget = it->targetHandle.get().get();
						break;
					}
					continue;
				}
			}
		}
		//a_actor->UpdateCombat();
	}


	bool OnMeleeHitHook::IsCasting(RE::Actor* a_actor)
	{
		bool result = false;

		auto IsCastingRight = false;
		auto IsCastingLeft = false;
		auto IsCastingDual = false;

		if ((a_actor->GetGraphVariableBool("IsCastingRight", IsCastingRight) && IsCastingRight) 
		|| (a_actor->GetGraphVariableBool("IsCastingLeft", IsCastingLeft) && IsCastingLeft) 
		|| (a_actor->GetGraphVariableBool("IsCastingDual", IsCastingDual) && IsCastingDual)) {
			result = true;
		}
		
		return result;
	}

	void OnMeleeHitHook::InterruptAttack(RE::Actor* a_actor){
		a_actor->NotifyAnimationGraph("attackStop");
		a_actor->NotifyAnimationGraph("recoilStop");
		a_actor->NotifyAnimationGraph("bashStop");
		a_actor->NotifyAnimationGraph("blockStop");
		a_actor->NotifyAnimationGraph("staggerStop");
	}

	std::vector<RE::TESForm*> OnMeleeHitHook::GetEquippedForm(RE::Actor* actor)
	{
		std::vector<RE::TESForm*> Hen;

		if (actor->GetEquippedObject(true)) {
			Hen.push_back(actor->GetEquippedObject(true));
		}
		if (actor->GetEquippedObject(false)) {
			Hen.push_back(actor->GetEquippedObject(false));
		}

		return Hen;
	}

	bool OnMeleeHitHook::GetEquippedType_IsMelee(RE::Actor* actor)
	{
		bool result = false;
		auto form_list = GetEquippedForm(actor);

		if (!form_list.empty()) {
			for (auto form : form_list) {
				if (form) {
					switch (*form->formType) {
					case RE::FormType::Weapon:
						if (const auto equippedWeapon = form->As<RE::TESObjectWEAP>()) {
							switch (equippedWeapon->GetWeaponType()) {
							case RE::WEAPON_TYPE::kHandToHandMelee:
							case RE::WEAPON_TYPE::kOneHandSword:
							case RE::WEAPON_TYPE::kOneHandDagger:
							case RE::WEAPON_TYPE::kOneHandAxe:
							case RE::WEAPON_TYPE::kOneHandMace:
							case RE::WEAPON_TYPE::kTwoHandSword:
							case RE::WEAPON_TYPE::kTwoHandAxe:
								result = true;
								break;
							default:
								break;
							}
						}
						break;
					case RE::FormType::Armor:
						if (auto equippedShield = form->As<RE::TESObjectARMO>()) {
							result = true;
						}
						break;
					default:
						break;
					}
					if (result) {
						break;
					}
				}
				continue;
			}
		}
		return result;
	}

	bool OnMeleeHitHook::is_melee(RE::Actor* actor)
	{
		return GetEquippedType_IsMelee(actor);
	}

	bool OnMeleeHitHook::IsMeleeOnly(RE::Actor* a_actor)
	{
		using TYPE = RE::CombatInventoryItem::TYPE;

		auto result = false;

		auto combatCtrl = a_actor->GetActorRuntimeData().combatController;
		auto CombatInv = combatCtrl ? combatCtrl->inventory : nullptr;
		if (CombatInv) {
			for (const auto item : CombatInv->equippedItems) {
				if (item.item) {
					switch (item.item->GetType()) {
					case TYPE::kMelee:
						result = true;
						break;
					default:
						break;
					}
				}
				if (result){
					break;
				}
			}
		}

		return result;
	}

	void OnMeleeHitHook::EquipfromInvent(RE::Actor* a_actor, RE::FormID a_formID)
	{
		auto inv = a_actor->GetInventory();
		for (auto& [item, data] : inv) {
			const auto& [count, entry] = data;
			if (count > 0 && entry->object->formID == a_formID) {
				RE::ActorEquipManager::GetSingleton()->EquipObject(a_actor, entry->object);
				break;
			}
			continue;
		}
	}

	bool OnMeleeHitHook::TBW_SendTransformation(STATIC_ARGS, RE::Actor* a_actor)
	{
		if (getrace_IsWerewolf(a_actor) || !Can_Transform(a_actor)) {
			return false;
		}

		a_actor->SetGraphVariableBool("bIsDodging", true);
		logger::info("Began Transformation");

		InterruptAttack(a_actor);
		a_actor->AddSpell(RE::TESForm::LookupByEditorID<RE::SpellItem>("TBW_WolvenForm_Ability"));
		auto data = RE::TESDataHandler::GetSingleton();
		util::playSound(a_actor, (data->LookupForm<RE::BGSSoundDescriptorForm>(0xFF783, "Skyrim.esm")));  // initiate sound
		a_actor->NotifyAnimationGraph("IdleWerewolfTransformation");

		a_actor->AsActorValueOwner()->SetActorValue(RE::ActorValue::kSpeedMult, 225.0f);
		auto were_armour = RE::TESForm::LookupByEditorID<RE::TESObjectARMO>("ArmorFXWerewolfTransitionSkin");
		GetSingleton()->UnequipAll(a_actor);
		a_actor->AddWornItem(were_armour, 1, false, 0, 0);
		EquipfromInvent(a_actor, were_armour->formID);

		Set_iFrames(a_actor);
		//util::playSound(a_actor, (data->LookupForm<RE::BGSSoundDescriptorForm>(0x51936, "Skyrim.esm"))); // transforming sound
		//util::playSound(a_actor, (data->LookupForm<RE::BGSSoundDescriptorForm>(0xFF785, "Skyrim.esm")));  // complete transformation sound
		return true;
	}

	void OnMeleeHitHook::TBW_CompleteTransformation(RE::Actor* a_actor)
	{
		logger::info("completing Transformation");

		auto were_armour = RE::TESForm::LookupByEditorID<RE::TESObjectARMO>("ArmorFXWerewolfTransitionSkin");
		a_actor->RemoveItem(were_armour, 1, RE::ITEM_REMOVE_REASON::kRemove, nullptr, nullptr);

		a_actor->SetGraphVariableBool("bIsDodging", false);

		logger::info("Wolven form succesful");
	}

	bool OnMeleeHitHook::TBW_RevertTransformation(STATIC_ARGS, RE::Actor* a_actor)
	{
		if (!getrace_IsWerewolf(a_actor)) {
			return false;
		}
		a_actor->SetGraphVariableBool("bIsDodging", true);
		auto data = RE::TESDataHandler::GetSingleton();
		util::playSound(a_actor, (data->LookupForm<RE::BGSSoundDescriptorForm>(0xFF783, "Skyrim.esm")));  // initiate sound

		a_actor->AsActorValueOwner()->SetActorValue(RE::ActorValue::kSpeedMult, 100.0f);
		a_actor->RemoveSpell(RE::TESForm::LookupByEditorID<RE::SpellItem>("TBW_WolvenForm_Ability"));

		return true;
	}

	void OnMeleeHitHook::TBW_CompleteReversion(RE::Actor* a_actor)
	{
		logger::info("completing Reversion");

		auto were_armour = RE::TESForm::LookupByEditorID<RE::TESObjectARMO>("ArmorFXWerewolfTransitionSkin");
		a_actor->RemoveItem(were_armour, 1, RE::ITEM_REMOVE_REASON::kRemove, nullptr, nullptr);

		GetSingleton()->Re_EquipAll(a_actor);

		const auto cooldown = RE::TESForm::LookupByEditorID<RE::MagicItem>("TBW_WolvenForm_Cooldown");
		const auto caster = a_actor->GetMagicCaster(RE::MagicSystem::CastingSource::kInstant);
		caster->CastSpellImmediate(cooldown, true, a_actor, 1, false, 0.0, a_actor);

		a_actor->SetGraphVariableBool("bIsDodging", false);

		logger::info("Were form succesful");
		auto spellList = a_actor->As<RE::TESNPC>()->GetSpellList();

		auto numSpells = spellList->numSpells;

		auto spells = spellList->spells;

		std::vector<RE::SpellItem*> copiedData{ spells, spells + numSpells };

		for (auto it : copiedData) {
			if (it) {
				for (auto itt : it->effects){
					itt->effectItem.duration;

				}
			}
			continue;
		}

		//spellList->GetIndex();
	}

	void OnMeleeHitHook::TBW_TriggerWolvenForm(STATIC_ARGS, RE::Actor* a_actor)
	{
		const auto transform = RE::TESForm::LookupByEditorID<RE::MagicItem>("TBW_BecomeTheBeast_Spell");
		const auto caster = a_actor->GetMagicCaster(RE::MagicSystem::CastingSource::kInstant);
		caster->CastSpellImmediate(transform, true, a_actor, 1, false, 0.0, a_actor);
	}

	float OnMeleeHitHook::AV_Mod(RE::Actor* a_actor, int actor_value, float input, float mod)
	{
		if (actor_value > 0){
			int k;
			for (k = 0; k <= actor_value; k += 1) {
				input += mod;
			}
		}

		return input;
	}

	


	class OurEventSink :
		public RE::BSTEventSink<RE::TESSwitchRaceCompleteEvent>,
		public RE::BSTEventSink<RE::TESEquipEvent>,
		public RE::BSTEventSink<RE::TESCombatEvent>,
		public RE::BSTEventSink<RE::TESActorLocationChangeEvent>,
		public RE::BSTEventSink<RE::TESSpellCastEvent>,
		public RE::BSTEventSink<SKSE::ModCallbackEvent>
	{
		OurEventSink() = default;
		OurEventSink(const OurEventSink&) = delete;
		OurEventSink(OurEventSink&&) = delete;
		OurEventSink& operator=(const OurEventSink&) = delete;
		OurEventSink& operator=(OurEventSink&&) = delete;

	public:
		static OurEventSink* GetSingleton()
		{
			static OurEventSink singleton;
			return &singleton;
		}

		RE::BSEventNotifyControl ProcessEvent(const RE::TESSwitchRaceCompleteEvent* event, RE::BSTEventSource<RE::TESSwitchRaceCompleteEvent>*)
		{
			auto a_actor = event->subject->As<RE::Actor>();

			if (!a_actor) {
				return RE::BSEventNotifyControl::kContinue;
			}

			if (!OnMeleeHitHook::getrace_IsWerewolf(a_actor)) {
				OnMeleeHitHook::Reset_iFrames(a_actor);
				OnMeleeHitHook::TBW_CompleteReversion(a_actor);
			}else{
				OnMeleeHitHook::Reset_iFrames(a_actor);
				OnMeleeHitHook::TBW_CompleteTransformation(a_actor);
			}

			return RE::BSEventNotifyControl::kContinue;
		}

		RE::BSEventNotifyControl ProcessEvent(const RE::TESEquipEvent* event, RE::BSTEventSource<RE::TESEquipEvent>*){
			auto a_actor = event->actor->As<RE::Actor>();

			if (!a_actor) {
				return RE::BSEventNotifyControl::kContinue;
			}

			return RE::BSEventNotifyControl::kContinue;
		}

		RE::BSEventNotifyControl ProcessEvent(const SKSE::ModCallbackEvent* event, RE::BSTEventSource<SKSE::ModCallbackEvent>*)
		{
			
			return RE::BSEventNotifyControl::kContinue;
		}

		RE::BSEventNotifyControl ProcessEvent(const RE::TESCombatEvent* event, RE::BSTEventSource<RE::TESCombatEvent>*){
			auto a_actor = event->actor->As<RE::Actor>();

			if (!a_actor || a_actor->IsPlayerRef()) {
				return RE::BSEventNotifyControl::kContinue;
			}
			// if (!OnMeleeHitHook::getrace_IsWerewolf(a_actor)) {
			// 	return RE::BSEventNotifyControl::kContinue;
			// }

			switch (event->newState.get()) {
			case RE::ACTOR_COMBAT_STATE::kCombat:
			case RE::ACTOR_COMBAT_STATE::kSearching:
			    break;

			case RE::ACTOR_COMBAT_STATE::kNone:
				if (OnMeleeHitHook::getrace_IsWerewolf(a_actor)) {
					const auto Revert = RE::TESForm::LookupByEditorID<RE::MagicItem>("TBW_RevertFormSpell");
					const auto caster = a_actor->GetMagicCaster(RE::MagicSystem::CastingSource::kInstant);
					caster->CastSpellImmediate(Revert, true, a_actor, 1, false, 0.0, a_actor);
				}
				break;

			default:
				break;
			}

			return RE::BSEventNotifyControl::kContinue;
		}

		RE::BSEventNotifyControl ProcessEvent(const RE::TESActorLocationChangeEvent* event, RE::BSTEventSource<RE::TESActorLocationChangeEvent>*)
		{
			auto a_actor = event->actor->As<RE::Actor>();

			if (!a_actor) {
				return RE::BSEventNotifyControl::kContinue;
			}

			if (!OnMeleeHitHook::getrace_IsWerewolf(a_actor)) {
				return RE::BSEventNotifyControl::kContinue;
			}

			auto Playerhandle = RE::PlayerCharacter::GetSingleton();

			if (Playerhandle->IsSneaking() || event->newLoc && event->newLoc->HasKeyword(RE::TESForm::LookupByEditorID<RE::BGSKeyword>("LocTypeHabitation"))) {
				const auto Revert = RE::TESForm::LookupByEditorID<RE::MagicItem>("TBW_RevertFormSpell");
				const auto caster = a_actor->GetMagicCaster(RE::MagicSystem::CastingSource::kInstant);
				caster->CastSpellImmediate(Revert, true, a_actor, 1, false, 0.0, a_actor);
			}

			return RE::BSEventNotifyControl::kContinue;
		}

		RE::BSEventNotifyControl ProcessEvent(const RE::TESSpellCastEvent* event, RE::BSTEventSource<RE::TESSpellCastEvent>*)
		{
			auto a_actor = event->object->As<RE::Actor>();

			if (!a_actor) {
				return RE::BSEventNotifyControl::kContinue;
			}

			auto eSpell = RE::TESForm::LookupByID(event->spell);

			if (eSpell && eSpell->Is(RE::FormType::Spell)) {
				auto rSpell = eSpell->As<RE::SpellItem>();
				if (rSpell->GetSpellType() == RE::MagicSystem::SpellType::kSpell) {
					std::string Lsht = (clib_util::editorID::get_editorID(rSpell));
					switch (hash(Lsht.c_str(), Lsht.size())) {
					case "PCG_SprintAttack_Execute_Spell"_h:
						break;
					default:
						break;
					}
				}
			}
			return RE::BSEventNotifyControl::kContinue;
		}
	};

	RE::BSEventNotifyControl animEventHandler::HookedProcessEvent(RE::BSAnimationGraphEvent& a_event, RE::BSTEventSource<RE::BSAnimationGraphEvent>* src)
	{
		FnProcessEvent fn = fnHash.at(*(uint64_t*)this);

		if (!a_event.holder) {
			return fn ? (this->*fn)(a_event, src) : RE::BSEventNotifyControl::kContinue;
		}

		RE::Actor* actor = const_cast<RE::TESObjectREFR*>(a_event.holder)->As<RE::Actor>();
		switch (hash(a_event.tag.c_str(), a_event.tag.size())) {
		case "tailSprint"_h:
			if (OnMeleeHitHook::IsMeleeOnly(actor)) {
			}
			break;

		default:
			break;
		}

		return fn ? (this->*fn)(a_event, src) : RE::BSEventNotifyControl::kContinue;
	}

	std::unordered_map<uint64_t, animEventHandler::FnProcessEvent> animEventHandler::fnHash;

	void OnMeleeHitHook::install(){

		auto eventSink = OurEventSink::GetSingleton();

		// ScriptSource
		auto* eventSourceHolder = RE::ScriptEventSourceHolder::GetSingleton();
		eventSourceHolder->AddEventSink<RE::TESSwitchRaceCompleteEvent>(eventSink);
		eventSourceHolder->AddEventSink<RE::TESEquipEvent>(eventSink);
		eventSourceHolder->AddEventSink<RE::TESCombatEvent>(eventSink);
		eventSourceHolder->AddEventSink<RE::TESActorLocationChangeEvent>(eventSink);
		eventSourceHolder->AddEventSink<RE::TESSpellCastEvent>(eventSink);
		SKSE::GetModCallbackEventSource()->AddEventSink(eventSink);
	}

	bool OnMeleeHitHook::BindPapyrusFunctions(VM* vm)
	{
		vm->RegisterFunction("TBW_SendTransformation", "TBW_NativeFunctions", TBW_SendTransformation);
		vm->RegisterFunction("TBW_RevertTransformation", "TBW_NativeFunctions", TBW_RevertTransformation);
		vm->RegisterFunction("TBW_TriggerWolvenForm", "TBW_NativeFunctions", TBW_TriggerWolvenForm);
		return true;
	}

	float OnMeleeHitHook::get_group_threatRatio(RE::Actor* protagonist, RE::Actor* combat_target)
	{
		float result = 0.0f;
		float MyTeam_total_threat = 0.0f;
		float EnemyTeam_total_threat = 0.0f;
		auto combatGroup = protagonist->GetCombatGroup();
		if (combatGroup) {
			
			for (auto it = combatGroup->members.begin(); it != combatGroup->members.end(); ++it) {
				if (it->memberHandle && it->memberHandle.get().get()) {
					MyTeam_total_threat += it->threatValue;
				}
				continue;
			}
		}
		auto EnemyGroup = combat_target->GetCombatGroup();
		if (EnemyGroup) {
			for (auto it = EnemyGroup->members.begin(); it != EnemyGroup->members.end(); ++it) {
				if (it->memberHandle && it->memberHandle.get().get()) {
					EnemyTeam_total_threat += it->threatValue;
				}
				continue;
			}
		}

		if (MyTeam_total_threat > 0 && EnemyTeam_total_threat > 0) {
			result = MyTeam_total_threat / EnemyTeam_total_threat;
		}

		return result;
	}

	float OnMeleeHitHook::get_personal_survivalRatio(RE::Actor* protagonist, RE::Actor* combat_target)
	{
		float result = 0.0f;
		float MyTeam_total_threat = 0.0f;
		float EnemyTeam_total_threat = 0.0f;
		float personal_threat = 0.0f;
		auto  combatGroup = protagonist->GetCombatGroup();
		if (combatGroup) {
			for (auto it = combatGroup->members.begin(); it != combatGroup->members.end(); ++it) {
				if (it->memberHandle && it->memberHandle.get().get()) {
					MyTeam_total_threat += it->threatValue;
					if (it->memberHandle.get().get() == protagonist){
						personal_threat += it->threatValue;
					}
				}
				continue;
			}
		}
		auto EnemyGroup = combat_target->GetCombatGroup();
		if (EnemyGroup) {
			for (auto it = EnemyGroup->members.begin(); it != EnemyGroup->members.end(); ++it) {
				if (it->memberHandle && it->memberHandle.get().get()) {
					EnemyTeam_total_threat += it->threatValue;
				}
				continue;
			}
		}

		if (MyTeam_total_threat > 0 && EnemyTeam_total_threat > 0 && personal_threat > 0) {

			auto personal_survival = personal_threat/EnemyTeam_total_threat;
			auto Enemy_groupSurvival = EnemyTeam_total_threat/MyTeam_total_threat;

			result = personal_survival/Enemy_groupSurvival;
		}

		return result;
	}

	float OnMeleeHitHook::get_personal_threatRatio(RE::Actor* protagonist, RE::Actor* combat_target)
	{
		float result = 0.0f;
		float personal_threat = 0.0f;
		float CTarget_threat = 0.0f;

		auto  combatGroup = protagonist->GetCombatGroup();
		if (combatGroup) {
			for (auto it = combatGroup->members.begin(); it != combatGroup->members.end(); ++it) {
				if (it->memberHandle && it->memberHandle.get().get() && it->memberHandle.get().get() == protagonist) {
					personal_threat += it->threatValue;
					break;
				}
				continue;
			}
		}
		auto EnemyGroup = combat_target->GetCombatGroup();
		if (EnemyGroup) {
			for (auto it = EnemyGroup->members.begin(); it != EnemyGroup->members.end(); ++it) {
				if (it->memberHandle && it->memberHandle.get().get() && it->memberHandle.get().get() == combat_target) {
					CTarget_threat += it->threatValue;
					break;
				}
				continue;
			}
		}

		if (personal_threat > 0 && CTarget_threat > 0) {
			result = personal_threat / CTarget_threat;
		}

		return result;
	}

	int OnMeleeHitHook::GenerateRandomInt(int value_a, int value_b)
	{
		std::mt19937 generator(rd());
		std::uniform_int_distribution<int> dist(value_a, value_b);
		return dist(generator);
	}

	float OnMeleeHitHook::GenerateRandomFloat(float value_a, float value_b)
	{
		std::mt19937 generator(rd());
		std::uniform_real_distribution<float> dist(value_a, value_b);
		return dist(generator);
	}

	float OnMeleeHitHook::confidence_threshold(RE::Actor* a_actor, int confidence, bool inverse)
	{
		float result = 0.0f;

		if (inverse){
			switch (confidence) {
			case 0:
				result = 0.1f;
				break;

			case 1:
				result = 0.3f;
				break;

			case 2:
				result = 0.5f;
				break;

			case 3:
				result = 0.7f;
				break;

			case 4:
				result = 0.9f;
				break;

			default:
				break;
			}
		}else{
			switch (confidence) {
			case 0:
				result = 1.25f;
				break;

			case 1:
				result = 1.0f;
				break;

			case 2:
				result = 0.75f;
				break;

			case 3:
				result = 0.5f;
				break;

			case 4:
				result = 0.25f;
				break;

			default:
				break;
			}
		}
		return result;
	}

	void OnMeleeHitHook::Update(RE::Actor* a_actor, [[maybe_unused]] float a_delta)
	{
		if (a_actor->GetActorRuntimeData().currentProcess && a_actor->GetActorRuntimeData().currentProcess->InHighProcess() && a_actor->Is3DLoaded()){
			///
		}
	}

	void OnMeleeHitHook::init()
	{
		_precision_API = reinterpret_cast<PRECISION_API::IVPrecision1*>(PRECISION_API::RequestPluginAPI());
		if (_precision_API) {
			_precision_API->AddPostHitCallback(SKSE::GetPluginHandle(), PrecisionWeaponsCallback_Post);
			logger::info("Enabled compatibility with Precision");
		}
	}

	void OnMeleeHitHook::PrecisionWeaponsCallback_Post(const PRECISION_API::PrecisionHitData& a_precisionHitData, const RE::HitData& a_hitdata)
	{
		if (!a_precisionHitData.target || !a_precisionHitData.target->Is(RE::FormType::ActorCharacter)) {
			return;
		}
		auto hit_causer = a_precisionHitData.attacker;
		//auto hit_target = a_precisionHitData.target->As<RE::Actor>();

		if (getrace_IsWerewolf(hit_causer) || hit_causer->HasKeywordString("TBW_IsPackMember_Key")) {
			auto magicTarget = hit_causer->AsMagicTarget();
			const auto magicEffect = RE::TESForm::LookupByEditorID<RE::EffectSetting>("TBW_FeralRageFastAttacksCrit_1st");
			if (magicTarget->HasMagicEffect(magicEffect)) {
				auto intensity = 0;
				hit_causer->GetGraphVariableInt("iTBW_FeralRage_intensity", intensity);
				if (intensity <= 19){
					hit_causer->SetGraphVariableInt("iTBW_FeralRage_intensity", intensity += 1);
				}
				auto health = hit_causer->AsActorValueOwner()->GetPermanentActorValue(RE::ActorValue::kHealth) * 0.01f;
				hit_causer->AsActorValueOwner()->RestoreActorValue(RE::ACTOR_VALUE_MODIFIER::kDamage, RE::ActorValue::kHealth, -health);
				if (hit_causer->AsActorValueOwner()->GetActorValue(RE::ActorValue::kHealth) / hit_causer->AsActorValueOwner()->GetPermanentActorValue(RE::ActorValue::kHealth) <= 0.3f){
					dispelEffect(RE::TESForm::LookupByEditorID<RE::MagicItem>("TBW_HowlOfTheHunt_Spell"), hit_causer);
				}
			}else{
				hit_causer->SetGraphVariableInt("iTBW_FeralRage_intensity", 0);
			}
		}
		return;
	}
}

// auto player = RE::PlayerCharacter::GetSingleton();
// a_actor->AddToFaction(RE::TESForm::LookupByEditorID<RE::TESFaction>("WerewolfFaction"), 1);
// player->AddToFaction(RE::TESForm::LookupByEditorID<RE::TESFaction>("WerewolfFaction"), 1);
// player->AddToFaction(RE::TESForm::LookupByEditorID<RE::TESFaction>("WerewolfFaction"), 1);