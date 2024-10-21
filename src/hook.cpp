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

	bool OnMeleeHitHook::Patch_Spell_List(RE::Actor* a_actor, RE::SpellItem* equipped_spell)
	{
		bool result = false;
		auto spelldata = a_actor->As<RE::TESNPC>()->GetSpellList();

		auto numSpells = spelldata->numSpells;

		auto spells = spelldata->spells;

		static auto fireKeyword = RE::TESForm::LookupByEditorID<RE::BGSKeyword>("MagicDamageFire");
		static auto frostKeyword = RE::TESForm::LookupByEditorID<RE::BGSKeyword>("MagicDamageFrost");
		static auto shockKeyword = RE::TESForm::LookupByEditorID<RE::BGSKeyword>("MagicDamageShock");
		static auto healKeyword = RE::TESForm::LookupByEditorID<RE::BGSKeyword>("MagicRestoreHealth");
		static auto PatchedSpell = RE::TESForm::LookupByEditorID<RE::BGSKeyword>("NSV_Patched_Key");

		const auto BE_hostile_ff_aimed = RE::TESForm::LookupByEditorID("NSV_Aimed_FireForget_Hostile_Effect")->As<RE::EffectSetting>();
		const auto BE_nonhostile_ff_aimed = RE::TESForm::LookupByEditorID("NSV_Aimed_FireForget_Hostile_Effect")->As<RE::EffectSetting>();
		const auto BE_heal_ff_aimed = RE::TESForm::LookupByEditorID("NSV_Aimed_FireForget_Hostile_Effect")->As<RE::EffectSetting>();

		const auto BE_hostile_ff_self = RE::TESForm::LookupByEditorID("NSV_Aimed_FireForget_Hostile_Effect")->As<RE::EffectSetting>();
		const auto BE_nonhostile_ff_self = RE::TESForm::LookupByEditorID("NSV_Aimed_FireForget_Hostile_Effect")->As<RE::EffectSetting>();
		const auto BE_heal_ff_self = RE::TESForm::LookupByEditorID("NSV_Aimed_FireForget_Hostile_Effect")->As<RE::EffectSetting>();

		const auto BE_hostile_ff_TA = RE::TESForm::LookupByEditorID("NSV_Aimed_FireForget_Hostile_Effect")->As<RE::EffectSetting>();
		const auto BE_nonhostile_ff_TA = RE::TESForm::LookupByEditorID("NSV_Aimed_FireForget_Hostile_Effect")->As<RE::EffectSetting>();
		const auto BE_heal_ff_TA = RE::TESForm::LookupByEditorID("NSV_Aimed_FireForget_Hostile_Effect")->As<RE::EffectSetting>();

		const auto BE_hostile_ff_TL = RE::TESForm::LookupByEditorID("NSV_Aimed_FireForget_Hostile_Effect")->As<RE::EffectSetting>();
		const auto BE_nonhostile_ff_TL = RE::TESForm::LookupByEditorID("NSV_Aimed_FireForget_Hostile_Effect")->As<RE::EffectSetting>();
		const auto BE_heal_ff_TL = RE::TESForm::LookupByEditorID("NSV_Aimed_FireForget_Hostile_Effect")->As<RE::EffectSetting>();


		const auto BE_hostile_cc_aimed = RE::TESForm::LookupByEditorID("NSV_Aimed_FireForget_Hostile_Effect")->As<RE::EffectSetting>();
		const auto BE_nonhostile_cc_aimed = RE::TESForm::LookupByEditorID("NSV_Aimed_FireForget_Hostile_Effect")->As<RE::EffectSetting>();
		const auto BE_heal_cc_aimed = RE::TESForm::LookupByEditorID("NSV_Aimed_FireForget_Hostile_Effect")->As<RE::EffectSetting>();

		const auto BE_hostile_cc_self = RE::TESForm::LookupByEditorID("NSV_Aimed_FireForget_Hostile_Effect")->As<RE::EffectSetting>();
		const auto BE_nonhostile_cc_self = RE::TESForm::LookupByEditorID("NSV_Aimed_FireForget_Hostile_Effect")->As<RE::EffectSetting>();
		const auto BE_heal_cc_self = RE::TESForm::LookupByEditorID("NSV_Aimed_FireForget_Hostile_Effect")->As<RE::EffectSetting>();

		const auto BE_hostile_cc_TA = RE::TESForm::LookupByEditorID("NSV_Aimed_FireForget_Hostile_Effect")->As<RE::EffectSetting>();
		const auto BE_nonhostile_cc_TA = RE::TESForm::LookupByEditorID("NSV_Aimed_FireForget_Hostile_Effect")->As<RE::EffectSetting>();
		const auto BE_heal_cc_TA = RE::TESForm::LookupByEditorID("NSV_Aimed_FireForget_Hostile_Effect")->As<RE::EffectSetting>();

		const auto BE_hostile_cc_TL = RE::TESForm::LookupByEditorID("NSV_Aimed_FireForget_Hostile_Effect")->As<RE::EffectSetting>();
		const auto BE_nonhostile_cc_TL = RE::TESForm::LookupByEditorID("NSV_Aimed_FireForget_Hostile_Effect")->As<RE::EffectSetting>();
		const auto BE_heal_cc_TL = RE::TESForm::LookupByEditorID("NSV_Aimed_FireForget_Hostile_Effect")->As<RE::EffectSetting>();

		bool hostile_flag = false;
		bool det_flag = false;
		bool fire_flag = false;
		bool frost_flag = false;
		bool shock_flag = false;
		bool heal_flag = false;

		std::vector<RE::SpellItem*> spellList{ spells, spells + numSpells };

		for (auto indv_spell : spellList) {
			if (indv_spell && !indv_spell->HasKeyword(PatchedSpell) && indv_spell != equipped_spell && indv_spell->GetDelivery() != RE::MagicSystem::Delivery::kTouch 
			&& indv_spell->GetCastingType() != RE::MagicSystem::CastingType::kScroll && indv_spell->GetCastingType() != RE::MagicSystem::CastingType::kConstantEffect) {
				for (auto indv_effect : indv_spell->effects){
					if (indv_effect->baseEffect->data.flags.all(RE::EffectSetting::EffectSettingData::Flag::kHostile)) {
						hostile_flag = true;
					}
					if (indv_effect->baseEffect->data.flags.all(RE::EffectSetting::EffectSettingData::Flag::kDetrimental)) {
						det_flag = true;
					}
					if (indv_effect->baseEffect->HasKeyword(fireKeyword)){
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
				}

				RE::Effect* effect;
				effect->cost = 0.0f;
				effect->conditions.head = nullptr;
				effect->conditions.head->next = nullptr;
				effect->effectItem.area = 0;
				effect->effectItem.duration = 0;
				effect->effectItem.magnitude = 0;

				switch (indv_spell->GetCastingType()) {
				case RE::MagicSystem::CastingType::kFireAndForget:

					switch (indv_spell->GetDelivery()) {
					case RE::MagicSystem::Delivery::kAimed:
						if(hostile_flag || fire_flag || frost_flag || shock_flag){
							effect->baseEffect = BE_hostile_ff_aimed;
						}
						if(heal_flag){
							effect->baseEffect = BE_heal_ff_aimed;
						}
						if (!heal_flag && !(hostile_flag|| det_flag || fire_flag || frost_flag || shock_flag)){
							effect->baseEffect = BE_nonhostile_ff_aimed;
						}
						break;

					case RE::MagicSystem::Delivery::kSelf:
						if (hostile_flag || fire_flag || frost_flag || shock_flag) {
							effect->baseEffect = BE_hostile_ff_self;
						}
						if (heal_flag) {
							effect->baseEffect = BE_heal_ff_self;
						}
						if (!heal_flag && !(hostile_flag || det_flag || fire_flag || frost_flag || shock_flag)) {
							effect->baseEffect = BE_nonhostile_ff_self;
						}
						break;

					case RE::MagicSystem::Delivery::kTargetActor:
						if (hostile_flag || fire_flag || frost_flag || shock_flag) {
							effect->baseEffect = BE_hostile_ff_TA;
						}
						if (heal_flag) {
							effect->baseEffect = BE_heal_ff_TA;
						}
						if (!heal_flag && !(hostile_flag || det_flag || fire_flag || frost_flag || shock_flag)) {
							effect->baseEffect = BE_nonhostile_ff_TA;
						}
						break;

					case RE::MagicSystem::Delivery::kTargetLocation:
						if (hostile_flag || fire_flag || frost_flag || shock_flag) {
							effect->baseEffect = BE_hostile_ff_TL;
						}
						if (heal_flag) {
							effect->baseEffect = BE_heal_ff_TL;
						}
						if (!heal_flag && !(hostile_flag || det_flag || fire_flag || frost_flag || shock_flag)) {
							effect->baseEffect = BE_nonhostile_ff_TL;
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
							effect->baseEffect = BE_hostile_cc_aimed;
						}
						if (heal_flag) {
							effect->baseEffect = BE_heal_cc_aimed;
						}
						if (!heal_flag && !(hostile_flag || det_flag || fire_flag || frost_flag || shock_flag)) {
							effect->baseEffect = BE_nonhostile_cc_aimed;
						}
						break;

					case RE::MagicSystem::Delivery::kSelf:
						if (hostile_flag || fire_flag || frost_flag || shock_flag) {
							effect->baseEffect = BE_hostile_cc_self;
						}
						if (heal_flag) {
							effect->baseEffect = BE_heal_cc_self;
						}
						if (!heal_flag && !(hostile_flag || det_flag || fire_flag || frost_flag || shock_flag)) {
							effect->baseEffect = BE_nonhostile_cc_self;
						}
						break;

					case RE::MagicSystem::Delivery::kTargetActor:
						if (hostile_flag || fire_flag || frost_flag || shock_flag) {
							effect->baseEffect = BE_hostile_cc_TA;
						}
						if (heal_flag) {
							effect->baseEffect = BE_heal_cc_TA;
						}
						if (!heal_flag && !(hostile_flag || det_flag || fire_flag || frost_flag || shock_flag)) {
							effect->baseEffect = BE_nonhostile_cc_TA;
						}
						break;

					case RE::MagicSystem::Delivery::kTargetLocation:
						if (hostile_flag || fire_flag || frost_flag || shock_flag) {
							effect->baseEffect = BE_hostile_cc_TL;
						}
						if (heal_flag) {
							effect->baseEffect = BE_heal_cc_TL;
						}
						if (!heal_flag && !(hostile_flag || det_flag || fire_flag || frost_flag || shock_flag)) {
							effect->baseEffect = BE_nonhostile_cc_TL;
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
				    OnMeleeHitHook::Patch_Spell_List(a_actor, rSpell);
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
		using EventType = RE::INPUT_EVENT_TYPE;
		using DeviceType = RE::INPUT_DEVICE;

		if (!a_event) {
			return RE::BSEventNotifyControl::kContinue;
		}

		for (auto event = *a_event; event; event = event->next) {
			if (event->eventType != EventType::kButton) {
				continue;
			}

			auto button = static_cast<RE::ButtonEvent*>(event);
			if (!button) {
				continue;
			}
			auto key = button->idCode;
			if (!key) {
				continue;
			}

			switch (button->device.get()) {
			case DeviceType::kMouse:
				key += kMouseOffset;
				break;
			case DeviceType::kKeyboard:
				key += kKeyboardOffset;
				break;
			case DeviceType::kGamepad:
				key = GetGamepadIndex((RE::BSWin32GamepadDevice::Key)key);
				break;
			default:
				continue;
			}

			if (key == 274)
			{
				auto Playerhandle = RE::PlayerCharacter::GetSingleton();
				if (button->IsDown() || button->IsHeld() || button->IsPressed()){
					Playerhandle->SetGraphVariableBool("bPSV_Toggle_PowerSprintAttack", true);
				}
				if(button->IsUp()){
					Playerhandle->SetGraphVariableBool("bPSV_Toggle_PowerSprintAttack", false);
				}
				break;
			}
		}

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
		return;
	}
}

// auto player = RE::PlayerCharacter::GetSingleton();
// a_actor->AddToFaction(RE::TESForm::LookupByEditorID<RE::TESFaction>("WerewolfFaction"), 1);
// player->AddToFaction(RE::TESForm::LookupByEditorID<RE::TESFaction>("WerewolfFaction"), 1);
// player->AddToFaction(RE::TESForm::LookupByEditorID<RE::TESFaction>("WerewolfFaction"), 1);
