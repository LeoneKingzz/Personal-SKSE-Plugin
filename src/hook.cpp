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

	bool OnMeleeHitHook::isHumanoid(RE::Actor* a_actor)
	{
		auto bodyPartData = a_actor->GetRace() ? a_actor->GetRace()->bodyPartData : nullptr;
		return bodyPartData && bodyPartData->GetFormID() == 0x1d;
	}

	void OnMeleeHitHook::Patch_Spell_List()
	{
		auto spellList = get_valid_spellList<RE::SpellItem>(LookupMods(Settings::GetSingleton()->exclude_spells_mods.exc_mods), LookupKeywords(Settings::GetSingleton()->exclude_spells_keywords.exc_keywords));

		static auto fireKeyword = RE::TESForm::LookupByEditorID<RE::BGSKeyword>("MagicDamageFire");
		static auto frostKeyword = RE::TESForm::LookupByEditorID<RE::BGSKeyword>("MagicDamageFrost");
		static auto shockKeyword = RE::TESForm::LookupByEditorID<RE::BGSKeyword>("MagicDamageShock");
		static auto healKeyword = RE::TESForm::LookupByEditorID<RE::BGSKeyword>("MagicRestoreHealth");
		static auto PatchedSpell = RE::TESForm::LookupByEditorID<RE::BGSKeyword>("NSV_Patched_Key");

		const auto NSV_Aimed_FF_Hostile_Effect = RE::TESForm::LookupByEditorID("NSV_Aimed_FF_Hostile_Effect")->As<RE::EffectSetting>();
		const auto NSV_Self_FF_Hostile_Effect = RE::TESForm::LookupByEditorID("NSV_Self_FF_Hostile_Effect")->As<RE::EffectSetting>();
		const auto NSV_TA_FF_Hostile_Effect = RE::TESForm::LookupByEditorID("NSV_TA_FF_Hostile_Effect")->As<RE::EffectSetting>();
		const auto NSV_TL_FF_Hostile_Effect = RE::TESForm::LookupByEditorID("NSV_TL_FF_Hostile_Effect")->As<RE::EffectSetting>();
		const auto NSV_Aimed_FF_nonHostile_Effect = RE::TESForm::LookupByEditorID("NSV_Aimed_FF_nonHostile_Effect")->As<RE::EffectSetting>();
		const auto NSV_Self_FF_nonHostile_Effect = RE::TESForm::LookupByEditorID("NSV_Self_FF_nonHostile_Effect")->As<RE::EffectSetting>();
		const auto NSV_TA_FF_nonHostile_Effect = RE::TESForm::LookupByEditorID("NSV_TA_FF_nonHostile_Effect")->As<RE::EffectSetting>();
		const auto NSV_TL_FF_nonHostile_Effect = RE::TESForm::LookupByEditorID("NSV_TL_FF_nonHostile_Effect")->As<RE::EffectSetting>();

		const auto NSV_Aimed_CC_Hostile_Effect = RE::TESForm::LookupByEditorID("NSV_Aimed_CC_Hostile_Effect")->As<RE::EffectSetting>();
		const auto NSV_Self_CC_Hostile_Effect = RE::TESForm::LookupByEditorID("NSV_Self_CC_Hostile_Effect")->As<RE::EffectSetting>();
		const auto NSV_TA_CC_Hostile_Effect = RE::TESForm::LookupByEditorID("NSV_TA_CC_Hostile_Effect")->As<RE::EffectSetting>();
		const auto NSV_TL_CC_Hostile_Effect = RE::TESForm::LookupByEditorID("NSV_TL_CC_Hostile_Effect")->As<RE::EffectSetting>();
		const auto NSV_Aimed_CC_nonHostile_Effect = RE::TESForm::LookupByEditorID("NSV_Aimed_CC_nonHostile_Effect")->As<RE::EffectSetting>();
		const auto NSV_Self_CC_nonHostile_Effect = RE::TESForm::LookupByEditorID("NSV_Self_CC_nonHostile_Effect")->As<RE::EffectSetting>();
		const auto NSV_TA_CC_nonHostile_Effect = RE::TESForm::LookupByEditorID("NSV_TA_CC_nonHostile_Effect")->As<RE::EffectSetting>();
		const auto NSV_TL_CC_nonHostile_Effect = RE::TESForm::LookupByEditorID("NSV_TL_CC_nonHostile_Effect")->As<RE::EffectSetting>();

		const auto NSV_Aimed_FF_Heal_Effect = RE::TESForm::LookupByEditorID("NSV_Aimed_FF_Heal_Effect")->As<RE::EffectSetting>();
		const auto NSV_Self_FF_Heal_Effect = RE::TESForm::LookupByEditorID("NSV_Self_FF_Heal_Effect")->As<RE::EffectSetting>();
		const auto NSV_TA_FF_Heal_Effect = RE::TESForm::LookupByEditorID("NSV_TA_FF_Heal_Effect")->As<RE::EffectSetting>();
		const auto NSV_TL_FF_Heal_Effect = RE::TESForm::LookupByEditorID("NSV_TL_FF_Heal_Effect")->As<RE::EffectSetting>();
		const auto NSV_Aimed_CC_Heal_Effect = RE::TESForm::LookupByEditorID("NSV_Aimed_CC_Heal_Effect")->As<RE::EffectSetting>();
		const auto NSV_Self_CC_Heal_Effect = RE::TESForm::LookupByEditorID("NSV_Self_CC_Heal_Effect")->As<RE::EffectSetting>();
		const auto NSV_TA_CC_Heal_Effect = RE::TESForm::LookupByEditorID("NSV_TA_CC_Heal_Effect")->As<RE::EffectSetting>();
		const auto NSV_TL_CC_Heal_Effect = RE::TESForm::LookupByEditorID("NSV_TL_CC_Heal_Effect")->As<RE::EffectSetting>();

		for (auto indv_spell : spellList) {
			if (indv_spell && indv_spell->GetSpellType() == RE::MagicSystem::SpellType::kSpell && !indv_spell->HasKeyword(PatchedSpell) && indv_spell->GetDelivery() != RE::MagicSystem::Delivery::kTouch 
			&& indv_spell->GetCastingType() != RE::MagicSystem::CastingType::kScroll && indv_spell->GetCastingType() != RE::MagicSystem::CastingType::kConstantEffect) {

				bool invalid_spell = false;
				bool hostile_flag = false;
				bool det_flag = false;
				bool fire_flag = false;
				bool frost_flag = false;
				bool shock_flag = false;
				bool heal_flag = false;
				bool valuemod_flag = false;
				bool health_AV_flag = false;

				for (auto indv_effect : indv_spell->effects) {
					if (indv_effect && indv_effect->baseEffect) {
						auto Archy = indv_effect->baseEffect->data.archetype;
						auto pAV = indv_effect->baseEffect->data.primaryAV;
						auto sAV = indv_effect->baseEffect->data.secondaryAV;

						if (Archy == RE::EffectSetting::Archetype::kValueAndParts || Archy == RE::EffectSetting::Archetype::kAccumulateMagnitude 
						|| Archy == RE::EffectSetting::Archetype::kSlowTime || Archy == RE::EffectSetting::Archetype::kDisguise || Archy == RE::EffectSetting::Archetype::kVampireLord 
						|| Archy == RE::EffectSetting::Archetype::kGrabActor || Archy == RE::EffectSetting::Archetype::kWerewolfFeed|| Archy == RE::EffectSetting::Archetype::kCureAddiction 
						|| Archy == RE::EffectSetting::Archetype::kDispel || Archy == RE::EffectSetting::Archetype::kTelekinesis || Archy == RE::EffectSetting::Archetype::kConcussion
						|| Archy == RE::EffectSetting::Archetype::kLock|| Archy == RE::EffectSetting::Archetype::kOpen || Archy == RE::EffectSetting::Archetype::kWerewolf
						|| Archy == RE::EffectSetting::Archetype::kSpawnScriptedRef || Archy == RE::EffectSetting::Archetype::kCureDisease|| Archy == RE::EffectSetting::Archetype::kNightEye
						|| Archy == RE::EffectSetting::Archetype::kGuide || Archy == RE::EffectSetting::Archetype::kLight || Archy == RE::EffectSetting::Archetype::kDarkness 
						|| Archy == RE::EffectSetting::Archetype::kDetectLife) {
							invalid_spell = true;
						}

						if (indv_effect->baseEffect->data.flags.all(RE::EffectSetting::EffectSettingData::Flag::kHostile)) {
							hostile_flag = true;
						}
						if (indv_effect->baseEffect->data.flags.all(RE::EffectSetting::EffectSettingData::Flag::kDetrimental)) {
							det_flag = true;
						}
						if (indv_effect->baseEffect->HasKeyword(fireKeyword)) {
							fire_flag = true;
						}
						if (indv_effect->baseEffect->HasKeyword(frostKeyword)) {
							frost_flag = true;
						}
						if (indv_effect->baseEffect->HasKeyword(shockKeyword)) {
							shock_flag = true;
						}
						if (indv_effect->baseEffect->HasKeyword(healKeyword)) {
							heal_flag = true;
						}
						if (Archy == RE::EffectSetting::Archetype::kValueModifier || Archy == RE::EffectSetting::Archetype::kDualValueModifier 
						|| Archy == RE::EffectSetting::Archetype::kPeakValueModifier) {
							valuemod_flag = true;
						}
						if (pAV == RE::ActorValue::kHealth || sAV == RE::ActorValue::kHealth) {
							health_AV_flag = true;
						}
					}
				}

				if (invalid_spell){
					continue;
				}

				RE::Effect* effect = new RE::Effect;
				effect->cost = 0.0f;
				effect->effectItem.area = 0;
				effect->effectItem.duration = 0;
				effect->effectItem.magnitude = 0;

				switch (indv_spell->GetCastingType()) {
				case RE::MagicSystem::CastingType::kFireAndForget:

					switch (indv_spell->GetDelivery()) {
					case RE::MagicSystem::Delivery::kAimed:
						if (hostile_flag || fire_flag || frost_flag || shock_flag) {
							effect->baseEffect = NSV_Aimed_FF_Hostile_Effect;

						}else if (!det_flag && (heal_flag || (valuemod_flag && health_AV_flag) )){
							effect->baseEffect = NSV_Aimed_FF_Heal_Effect;

						}else{
							effect->baseEffect = NSV_Aimed_FF_nonHostile_Effect;
						}
						break;

					case RE::MagicSystem::Delivery::kSelf:
						if (hostile_flag || fire_flag || frost_flag || shock_flag) {
							effect->baseEffect = NSV_Self_FF_Hostile_Effect;

						} else if (!det_flag && (heal_flag || (valuemod_flag && health_AV_flag))) {
							effect->baseEffect = NSV_Self_FF_Heal_Effect;

						} else {
							effect->baseEffect = NSV_Self_FF_nonHostile_Effect;
						}
						break;

					case RE::MagicSystem::Delivery::kTargetActor:
						if (hostile_flag || fire_flag || frost_flag || shock_flag) {
							effect->baseEffect = NSV_TA_FF_Hostile_Effect;

						} else if (!det_flag && (heal_flag || (valuemod_flag && health_AV_flag))) {
							effect->baseEffect = NSV_TA_FF_Heal_Effect;

						} else {
							effect->baseEffect = NSV_TA_FF_nonHostile_Effect;
						}
						break;

					case RE::MagicSystem::Delivery::kTargetLocation:
						if (hostile_flag || fire_flag || frost_flag || shock_flag) {
							effect->baseEffect = NSV_TL_FF_Hostile_Effect;

						} else if (!det_flag && (heal_flag || (valuemod_flag && health_AV_flag))) {
							effect->baseEffect = NSV_TL_FF_Heal_Effect;

						} else {
							effect->baseEffect = NSV_TL_FF_nonHostile_Effect;
						}
						break;

					default:
						break;
					}

					break;

				case RE::MagicSystem::CastingType::kConcentration:

					switch (indv_spell->GetDelivery()) {
					case RE::MagicSystem::Delivery::kAimed:
						if (hostile_flag || fire_flag || frost_flag || shock_flag) {
							effect->baseEffect = NSV_Aimed_CC_Hostile_Effect;

						} else if (!det_flag && (heal_flag || (valuemod_flag && health_AV_flag))) {
							effect->baseEffect = NSV_Aimed_CC_Heal_Effect;

						} else {
							effect->baseEffect = NSV_Aimed_CC_nonHostile_Effect;
						}
						break;

					case RE::MagicSystem::Delivery::kSelf:
						if (hostile_flag || fire_flag || frost_flag || shock_flag) {
							effect->baseEffect = NSV_Self_CC_Hostile_Effect;

						} else if (!det_flag && (heal_flag || (valuemod_flag && health_AV_flag))) {
							effect->baseEffect = NSV_Self_CC_Heal_Effect;

						} else {
							effect->baseEffect = NSV_Self_CC_nonHostile_Effect;
						}
						break;

					case RE::MagicSystem::Delivery::kTargetActor:
						if (hostile_flag || fire_flag || frost_flag || shock_flag) {
							effect->baseEffect = NSV_TA_CC_Hostile_Effect;

						} else if (!det_flag && (heal_flag || (valuemod_flag && health_AV_flag))) {
							effect->baseEffect = NSV_TA_CC_Heal_Effect;

						} else {
							effect->baseEffect = NSV_TA_CC_nonHostile_Effect;
						}
						break;

					case RE::MagicSystem::Delivery::kTargetLocation:
						if (hostile_flag || fire_flag || frost_flag || shock_flag) {
							effect->baseEffect = NSV_TL_CC_Hostile_Effect;

						} else if (!det_flag && (heal_flag || (valuemod_flag && health_AV_flag))) {
							effect->baseEffect = NSV_TL_CC_Heal_Effect;

						} else {
							effect->baseEffect = NSV_TL_CC_nonHostile_Effect;
						}
						break;

					default:
						break;
					}

					break;

				default:
					break;
				}

				indv_spell->AddKeyword(PatchedSpell);
				indv_spell->effects.push_back(effect);
			}
			continue;
		}
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

			switch (event->newState.get()) {
			case RE::ACTOR_COMBAT_STATE::kCombat:
			case RE::ACTOR_COMBAT_STATE::kSearching:
			    break;

			case RE::ACTOR_COMBAT_STATE::kNone:
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

			return RE::BSEventNotifyControl::kContinue;
		}

		RE::BSEventNotifyControl ProcessEvent(const RE::TESSpellCastEvent* event, RE::BSTEventSource<RE::TESSpellCastEvent>*)
		{
			auto a_actor = event->object->As<RE::Actor>();

			if (!a_actor) {
				return RE::BSEventNotifyControl::kContinue;
			}

			if (a_actor->IsPlayerRef() || a_actor->HasKeywordString("NSV_ExcludeActor_Key")) {
				return RE::BSEventNotifyControl::kContinue;
			}

			auto eSpell = RE::TESForm::LookupByID(event->spell);

			if (eSpell && eSpell->Is(RE::FormType::Spell)) {
				auto rSpell = eSpell->As<RE::SpellItem>();
				switch (rSpell->GetSpellType()) {
				case RE::MagicSystem::SpellType::kSpell:
				    //OnMeleeHitHook::Patch_Spell_List();
					break;

				default:
					break;
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

	InputEventHandler* InputEventHandler::GetSingleton()
	{
		static InputEventHandler singleton;
		return std::addressof(singleton);
	}

	RE::BSEventNotifyControl InputEventHandler::ProcessEvent(RE::InputEvent* const* a_event, [[maybe_unused]] RE::BSTEventSource<RE::InputEvent*>* a_eventSource)
	{
		// using EventType = RE::INPUT_EVENT_TYPE;
		// using DeviceType = RE::INPUT_DEVICE;

		if (!a_event) {
			return RE::BSEventNotifyControl::kContinue;
		}

		// for (auto event = *a_event; event; event = event->next) {
		// 	if (event->eventType != EventType::kButton) {
		// 		continue;
		// 	}

		// 	auto button = static_cast<RE::ButtonEvent*>(event);
		// 	if (!button) {
		// 		continue;
		// 	}
		// 	auto key = button->idCode;
		// 	if (!key) {
		// 		continue;
		// 	}

		// 	switch (button->device.get()) {
		// 	case DeviceType::kMouse:
		// 		key += kMouseOffset;
		// 		break;
		// 	case DeviceType::kKeyboard:
		// 		key += kKeyboardOffset;
		// 		break;
		// 	case DeviceType::kGamepad:
		// 		key = GetGamepadIndex((RE::BSWin32GamepadDevice::Key)key);
		// 		break;
		// 	default:
		// 		continue;
		// 	}

		// 	if (key == 274)
		// 	{
		// 		auto Playerhandle = RE::PlayerCharacter::GetSingleton();
		// 		if (button->IsDown() || button->IsHeld() || button->IsPressed()){
		// 			Playerhandle->SetGraphVariableBool("bPSV_Toggle_PowerSprintAttack", true);
		// 		}
		// 		if(button->IsUp()){
		// 			Playerhandle->SetGraphVariableBool("bPSV_Toggle_PowerSprintAttack", false);
		// 		}
		// 		break;
		// 	}
		// }

		return RE::BSEventNotifyControl::kContinue;
	}

	std::uint32_t InputEventHandler::GetGamepadIndex(RE::BSWin32GamepadDevice::Key a_key)
	{
		using Key = RE::BSWin32GamepadDevice::Key;

		std::uint32_t index;
		switch (a_key) {
		case Key::kUp:
			index = 0;
			break;
		case Key::kDown:
			index = 1;
			break;
		case Key::kLeft:
			index = 2;
			break;
		case Key::kRight:
			index = 3;
			break;
		case Key::kStart:
			index = 4;
			break;
		case Key::kBack:
			index = 5;
			break;
		case Key::kLeftThumb:
			index = 6;
			break;
		case Key::kRightThumb:
			index = 7;
			break;
		case Key::kLeftShoulder:
			index = 8;
			break;
		case Key::kRightShoulder:
			index = 9;
			break;
		case Key::kA:
			index = 10;
			break;
		case Key::kB:
			index = 11;
			break;
		case Key::kX:
			index = 12;
			break;
		case Key::kY:
			index = 13;
			break;
		case Key::kLeftTrigger:
			index = 14;
			break;
		case Key::kRightTrigger:
			index = 15;
			break;
		default:
			index = kInvalid;
			break;
		}

		return index != kInvalid ? index + kGamepadOffset : kInvalid;
	}

	void InputEventHandler::SinkEventHandlers()
	{
		auto deviceManager = RE::BSInputDeviceManager::GetSingleton();
		deviceManager->AddEventSink(InputEventHandler::GetSingleton());
		logger::info("Added input event sink");
	}

	bool OnMeleeHitHook::BindPapyrusFunctions(VM* vm)
	{
		//vm->RegisterFunction("XXXX", "XXXXX", XXXX);
		return true;
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
		return;
	}

	void Settings::Load(){
		constexpr auto path = "Data\\SKSE\\Plugins\\NPCSpellVariance.ini";

		CSimpleIniA ini;
		ini.SetUnicode();

		ini.LoadFile(path);

		exclude_spells_mods.Load(ini);
		exclude_spells_keywords.Load(ini);

		ini.SaveFile(path);
	}

	void Settings::Exclude_AllSpells_inMods::Load(CSimpleIniA& a_ini){
		static const char* section = "Exclude_AllSpells_inMods";

		auto DS = GetSingleton();

		DS->exclude_spells_mods.exc_mods = detail::get_value(a_ini, DS->exclude_spells_mods.exc_mods, section, "exc_mods",
		";Enter Mod Names of which all spells within are excluded.");
	}

	void Settings::Exclude_AllSpells_withKeywords::Load(CSimpleIniA& a_ini)
	{
		static const char* section = "Exclude_AllSpells_withKeywords";

		auto DS = GetSingleton();

		DS->exclude_spells_keywords.exc_keywords = detail::get_value(a_ini, DS->exclude_spells_keywords.exc_keywords, section, "exc_keywords", 
		";Enter keywords for which all associated spells are excluded.");
	}
}

// Exclude_AllSpells_inMods::exc_mods.push_back("Heroes of Yore.esp");
// Exclude_AllSpells_inMods::exc_mods.push_back("VampireLordSeranaAssets.esp");
// Exclude_AllSpells_inMods::exc_mods.push_back("VampireLordSerana.esp");
// Exclude_AllSpells_inMods::exc_mods.push_back("TheBeastWithin.esp");
// Exclude_AllSpells_inMods::exc_mods.push_back("TheBeastWithinHowls.esp");
// Exclude_AllSpells_withKeywords::exc_keywords.push_back("HoY_MagicShoutSpell");
// Exclude_AllSpells_withKeywords::exc_keywords.push_back("LDP_MagicShoutSpell");

// bool OnMeleeHitHook::AddCondition(BGSKeyword* a_keyword)
// {
// 	if (!GetKeywordIndex(a_keyword)) {
// 		std::vector<BGSKeyword*> copiedData{ keywords, keywords + numKeywords };
// 		copiedData.push_back(a_keyword);
// 		CopyKeywords(copiedData);
// 		return true;
// 	}
// 	return false;
// }

// void OnMeleeHitHook::CopyEffect(const std::vector<RE::Effect*>& a_copiedData)
// {
// 	const auto oldData = keywords;

// 	const auto newSize = a_copiedData.size();
// 	const auto newData = calloc<RE::Effect*>(newSize);
// 	std::ranges::copy(a_copiedData, newData);

// 	numKeywords = static_cast<std::uint32_t>(newSize);
// 	keywords = newData;

// 	free(oldData);
// }

// auto spelldata = a_actor->As<RE::TESNPC>()->GetSpellList();
// auto numSpells = spelldata->numSpells;
// auto spells = spelldata->spells;
// std::vector<RE::SpellItem*> spellList{ spells, spells + numSpells };