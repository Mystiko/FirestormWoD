////////////////////////////////////////////////////////////////////////////////
///
///  MILLENIUM-STUDIO
///  Copyright 2016 Millenium-studio SARL
///  All Rights Reserved.
///
////////////////////////////////////////////////////////////////////////////////

# include "boss_iron_maidens.hpp"

/// Admiral Gar'an - 77557
class boss_admiral_garan : public CreatureScript
{
    public:
        boss_admiral_garan() : CreatureScript("boss_admiral_garan") { }

        enum eSpells
        {
            IronMaidenIntroConversation = 172658,
            AfterTrashesConversation    = 172686,
            RideLoadingChain            = 158646,
            /// Combat spells
            /// Iron Shot
            SpellIronShotSearcher       = 156666,
            SpellIronShotDamage         = 156669,
            /// Rapid Fire
            SpellRapidFireRedArrow      = 156631,
            SpellRapideFirePeriodic     = 156626,
            SpellRapidFireTargetVisual  = 156649,
            /// Penetrating Shot
            SpellPenetratingShotAura    = 164271,
            /// Deploy Turret
            SpellDeployTurretJump       = 159585,
            SpellDeployTurretSummon     = 158599,
            SpellDominatorBlastDoT      = 158601
        };

        enum eEvents
        {
            EventRapidFire = 1,
            EventRegenIronFury,
            EventPenetratingShot,
            EventDeployTurret,
            EventDreadnaughtPhase,
            EventJumpToShip
        };

        enum eTimers
        {
            TimerRapidFire          = 19 * TimeConstants::IN_MILLISECONDS,
            TimerRapidFireCD        = 30 * TimeConstants::IN_MILLISECONDS,
            TimerEnergyRegen        = 6 * TimeConstants::IN_MILLISECONDS + 333,
            TimerPenetratingShotCD  = 30 * TimeConstants::IN_MILLISECONDS,
            TimerDeployTurretCD     = 21 * TimeConstants::IN_MILLISECONDS + 300,
            TimerDreadnaughtPhase   = 60 * TimeConstants::IN_MILLISECONDS,
            TimerDreadnaughtPhaseCD = 198 * TimeConstants::IN_MILLISECONDS
        };

        enum eTalks
        {
        };

        enum eMoves
        {
            MoveJump = 1,
            MoveDown,
            MoveLast,
            MoveToZipline,
            MoveExitZipline = 50
        };

        enum eVisual
        {
            IntroVisual = 6636
        };

        struct boss_admiral_garanAI : public BossAI
        {
            boss_admiral_garanAI(Creature* p_Creature) : BossAI(p_Creature, eFoundryDatas::DataIronMaidens)
            {
                m_Instance  = p_Creature->GetInstanceScript();
                m_IntroDone = false;

                p_Creature->AddUnitMovementFlag(MovementFlags::MOVEMENTFLAG_DISABLE_COLLISION);

                for (uint8 l_I = 0; l_I < eIronMaidensDatas::MaxLoadingChains; ++l_I)
                    m_LoadingChains[l_I] = 0;
            }

            InstanceScript* m_Instance;

            EventMap m_CustomEvent;
            EventMap m_Events;

            std::array<uint64, eIronMaidensDatas::MaxLoadingChains> m_LoadingChains;

            bool m_IntroDone;
            std::set<uint64> m_TrashesGuids;

            bool m_DeployTurret;

            bool m_IronWillTriggered;
            bool m_BossDisabled;
            bool m_CanJumpToShip;
            bool m_IsOnBoat;

            void Reset() override
            {
                me->setPowerType(Powers::POWER_ENERGY);
                me->SetMaxPower(Powers::POWER_ENERGY, 100);
                me->SetPower(Powers::POWER_ENERGY, 0);

                me->SetCanFly(false);
                me->SetDisableGravity(false);
                me->SetHover(false);

                me->SetReactState(ReactStates::REACT_AGGRESSIVE);

                me->CastSpell(me, eIronMaidensSpells::ZeroPowerZeroRegen, true);

                me->RemoveAura(eIronMaidensSpells::IronWill);
                me->RemoveAura(eIronMaidensSpells::PermanentFeignDeath);
                me->RemoveAura(eIronMaidensSpells::WarmingUpAura);

                me->RemoveFlag(EUnitFields::UNIT_FIELD_FLAGS, eUnitFlags::UNIT_FLAG_DISABLE_MOVE | eUnitFlags::UNIT_FLAG_NOT_SELECTABLE | eUnitFlags::UNIT_FLAG_UNK_29);
                me->RemoveFlag(EUnitFields::UNIT_FIELD_FLAGS_2, eUnitFlags2::UNIT_FLAG2_FEIGN_DEATH | eUnitFlags2::UNIT_FLAG2_DISABLE_TURN | eUnitFlags2::UNIT_FLAG2_UNK5);

                m_CustomEvent.Reset();
                m_Events.Reset();

                RespawnMaidens(m_Instance, me);

                _Reset();

                if (!m_IntroDone)
                {
                    AddTimedDelayedOperation(1 * TimeConstants::IN_MILLISECONDS, [this]() -> void
                    {
                        std::list<Creature*> l_TrashesList;

                        me->GetCreatureListWithEntryInGrid(l_TrashesList, eIronMaidensCreatures::AquaticTechnician, 150.0f);
                        me->GetCreatureListWithEntryInGridAppend(l_TrashesList, eIronMaidensCreatures::IronDockworker, 150.0f);
                        me->GetCreatureListWithEntryInGridAppend(l_TrashesList, eIronMaidensCreatures::IronEarthbinder, 150.0f);
                        me->GetCreatureListWithEntryInGridAppend(l_TrashesList, eIronMaidensCreatures::IronMauler, 150.0f);
                        me->GetCreatureListWithEntryInGridAppend(l_TrashesList, eIronMaidensCreatures::IronCleaver, 150.0f);

                        for (Creature* l_Trash : l_TrashesList)
                        {
                            if (l_Trash->isAlive())
                                m_TrashesGuids.insert(l_Trash->GetGUID());
                        }

                        if (m_TrashesGuids.empty())
                        {
                            m_IntroDone = true;

                            AddTimedDelayedOperation(1 * TimeConstants::IN_MILLISECONDS, [this]() -> void
                            {
                                if (Creature* l_Sorka = Creature::GetCreature(*me, m_Instance->GetData64(eFoundryCreatures::BossEnforcerSorka)))
                                {
                                    if (l_Sorka->IsAIEnabled)
                                        l_Sorka->AI()->DoAction(eIronMaidensActions::ActionAfterTrashesIntro);
                                }

                                if (Creature* l_Marak = Creature::GetCreature(*me, m_Instance->GetData64(eFoundryCreatures::BossMarakTheBlooded)))
                                {
                                    if (l_Marak->IsAIEnabled)
                                        l_Marak->AI()->DoAction(eIronMaidensActions::ActionAfterTrashesIntro);
                                }

                                DoAction(eIronMaidensActions::ActionAfterTrashesIntro);
                            });
                        }
                    });

                    AddTimedDelayedOperation(2 * TimeConstants::IN_MILLISECONDS, [this]() -> void
                    {
                        std::list<Creature*> l_LoadingChains;
                        me->GetCreatureListWithEntryInGrid(l_LoadingChains, eIronMaidensCreatures::LoadingChain, 150.0f);

                        uint8 l_Count = 0;
                        for (Creature* l_Creature : l_LoadingChains)
                            m_LoadingChains[l_Count++] = l_Creature->GetGUID();
                    });
                }

                m_DeployTurret      = false;

                m_IronWillTriggered = false;
                m_BossDisabled      = false;
                m_CanJumpToShip     = true;
                m_IsOnBoat          = false;

                for (uint64 l_Guid : m_LoadingChains)
                {
                    if (Creature* l_Chain = Creature::GetCreature(*me, l_Guid))
                    {
                        l_Chain->RemoveFlag(EUnitFields::UNIT_FIELD_NPC_FLAGS, NPCFlags::UNIT_NPC_FLAG_SPELLCLICK);
                        l_Chain->NearTeleportTo(l_Chain->GetHomePosition());
                    }
                }

                if (m_Instance != nullptr)
                {
                    if (Creature* l_Zipline = Creature::GetCreature(*me, m_Instance->GetData64(eFoundryCreatures::ZiplineStalker)))
                    {
                        if (l_Zipline->IsAIEnabled)
                            l_Zipline->AI()->Reset();
                    }
                }
            }

            void SetGUID(uint64 p_Guid, int32 p_ID /*= 0*/) override
            {
                m_TrashesGuids.erase(p_Guid);

                if (!m_IntroDone && m_TrashesGuids.empty() && m_Instance != nullptr)
                {
                    m_IntroDone = true;

                    AddTimedDelayedOperation(1 * TimeConstants::IN_MILLISECONDS, [this]() -> void
                    {
                        if (Creature* l_Sorka = Creature::GetCreature(*me, m_Instance->GetData64(eFoundryCreatures::BossEnforcerSorka)))
                        {
                            if (l_Sorka->IsAIEnabled)
                                l_Sorka->AI()->DoAction(eIronMaidensActions::ActionAfterTrashesIntro);
                        }

                        if (Creature* l_Marak = Creature::GetCreature(*me, m_Instance->GetData64(eFoundryCreatures::BossMarakTheBlooded)))
                        {
                            if (l_Marak->IsAIEnabled)
                                l_Marak->AI()->DoAction(eIronMaidensActions::ActionAfterTrashesIntro);
                        }

                        DoAction(eIronMaidensActions::ActionAfterTrashesIntro);
                    });
                }
            }

            uint32 GetData(uint32 p_ID) override
            {
                switch (p_ID)
                {
                    case eIronMaidensDatas::IsAvailableForShip:
                        return uint32(m_CanJumpToShip);
                    default:
                        break;
                }

                return 0;
            }

            void SetData(uint32 p_ID, uint32 p_Value) override
            {
                switch (p_ID)
                {
                    case eIronMaidensDatas::IsAvailableForShip:
                    {
                        m_CanJumpToShip = p_Value != 0;
                        break;
                    }
                    default:
                        break;
                }
            }

            void KilledUnit(Unit* p_Killed) override
            {
                /*if (p_Killed->IsPlayer())
                    Talk(eThogarTalks::TalkSlay);*/
            }

            void EnterCombat(Unit* p_Attacker) override
            {
                StartMaidens(m_Instance, me, p_Attacker);

                _EnterCombat();

                m_Events.ScheduleEvent(eEvents::EventRapidFire, eTimers::TimerRapidFire);

                m_CustomEvent.ScheduleEvent(eEvents::EventRegenIronFury, eTimers::TimerEnergyRegen);
                m_CustomEvent.ScheduleEvent(eEvents::EventDreadnaughtPhase, eTimers::TimerDreadnaughtPhase);
            }

            void JustDied(Unit* /*p_Killer*/) override
            {
                me->RemoveAllAreasTrigger();

                summons.DespawnAll();

                _JustDied();

                if (m_Instance != nullptr)
                    m_Instance->SendEncounterUnit(EncounterFrameType::ENCOUNTER_FRAME_DISENGAGE, me);

                RemoveCombatAuras();
            }

            void EnterEvadeMode() override
            {
                me->ExitVehicle();

                me->NearTeleportTo(me->GetHomePosition());

                WipeMaidens(m_Instance);

                CreatureAI::EnterEvadeMode();

                me->RemoveAllAreasTrigger();

                summons.DespawnAll();

                RemoveCombatAuras();
            }

            void DoAction(int32 const p_Action) override
            {
                switch (p_Action)
                {
                    case eIronMaidensActions::ActionIntro:
                    {
                        if (m_IntroDone)
                            break;

                        me->CastSpell(me, eSpells::IronMaidenIntroConversation, false);
                        break;
                    }
                    case eIronMaidensActions::ActionAfterTrashesIntro:
                    {
                        me->CastSpell(me, eSpells::AfterTrashesConversation, false);

                        std::list<Creature*> l_CosmeticMobs;
                        me->GetCreatureListWithEntryInGrid(l_CosmeticMobs, eIronMaidensCreatures::Ukurogg, 150.0f);
                        me->GetCreatureListWithEntryInGridAppend(l_CosmeticMobs, eIronMaidensCreatures::Uktar, 150.0f);
                        me->GetCreatureListWithEntryInGridAppend(l_CosmeticMobs, eIronMaidensCreatures::BattleMedicRogg, 150.0f);
                        me->GetCreatureListWithEntryInGridAppend(l_CosmeticMobs, eIronMaidensCreatures::Gorak, 150.0f);
                        me->GetCreatureListWithEntryInGridAppend(l_CosmeticMobs, eIronMaidensCreatures::IronEviscerator, 150.0f);

                        for (Creature* l_Mob : l_CosmeticMobs)
                            l_Mob->DespawnOrUnsummon();

                        AddTimedDelayedOperation(9 * TimeConstants::IN_MILLISECONDS, [this]() -> void
                        {
                            me->GetMotionMaster()->MoveJump(g_BoatBossFirstJumpPos, 30.0f, 20.0f, eMoves::MoveJump);
                        });

                        break;
                    }
                    case eIronMaidensActions::ActionJumpToShip:
                    {
                        m_Events.ScheduleEvent(eEvents::EventJumpToShip, 1);
                        break;
                    }
                    default:
                        break;
                }
            }

            void MovementInform(uint32 p_Type, uint32 p_ID) override
            {
                if (p_Type != MovementGeneratorType::EFFECT_MOTION_TYPE &&
                    p_Type != MovementGeneratorType::POINT_MOTION_TYPE)
                    return;

                switch (p_ID)
                {
                    case eMoves::MoveJump:
                    {
                        me->SetAIAnimKitId(eVisual::IntroVisual);

                        AddTimedDelayedOperation(10, [this]() -> void
                        {
                            me->AddUnitMovementFlag(MovementFlags::MOVEMENTFLAG_FLYING);
                            me->SetSpeed(UnitMoveType::MOVE_FLIGHT, 4.0f);
                            me->GetMotionMaster()->MoveSmoothFlyPath(eMoves::MoveDown, g_BoatBossFlyingMoves.data(), g_BoatBossFlyingMoves.size());
                        });

                        break;
                    }
                    case eMoves::MoveDown:
                    {
                        me->SetAIAnimKitId(0);

                        AddTimedDelayedOperation(10, [this]() -> void
                        {
                            me->RemoveUnitMovementFlag(MovementFlags::MOVEMENTFLAG_FLYING);
                            me->SetSpeed(UnitMoveType::MOVE_FLIGHT, me->GetCreatureTemplate()->speed_fly);
                            me->GetMotionMaster()->MoveJump(g_GaranHomePos, 30.0f, 20.0f, eMoves::MoveLast);
                        });

                        break;
                    }
                    case eMoves::MoveLast:
                    {
                        for (uint8 l_I = 0; l_I < MAX_EQUIPMENT_ITEMS * 2; ++l_I)
                            me->SetUInt32Value(EUnitFields::UNIT_FIELD_VIRTUAL_ITEMS + l_I, 0);

                        me->SetHomePosition(g_GaranHomePos);

                        AddTimedDelayedOperation(10, [this]() -> void
                        {
                            me->SetFacingTo(g_GaranFinalFacing);
                        });

                        AddTimedDelayedOperation(1 * TimeConstants::IN_MILLISECONDS, [this]() -> void
                        {
                            me->LoadEquipment();

                            me->RemoveFlag(EUnitFields::UNIT_FIELD_FLAGS, eUnitFlags::UNIT_FLAG_IMMUNE_TO_PC);

                            if (Creature* l_Sorka = Creature::GetCreature(*me, m_Instance->GetData64(eFoundryCreatures::BossEnforcerSorka)))
                                l_Sorka->RemoveFlag(EUnitFields::UNIT_FIELD_FLAGS, eUnitFlags::UNIT_FLAG_IMMUNE_TO_PC);

                            if (Creature* l_Marak = Creature::GetCreature(*me, m_Instance->GetData64(eFoundryCreatures::BossMarakTheBlooded)))
                                l_Marak->RemoveFlag(EUnitFields::UNIT_FIELD_FLAGS, eUnitFlags::UNIT_FLAG_IMMUNE_TO_PC);
                        });

                        break;
                    }
                    case eSpells::SpellDeployTurretJump:
                    {
                        me->CastSpell(*me, eSpells::SpellDeployTurretSummon, false);
                        break;
                    }
                    case eMoves::MoveToZipline:
                    {
                        AddTimedDelayedOperation(10, [this]() -> void
                        {
                            if (m_Instance == nullptr)
                                return;

                            if (Creature* l_Zipline = Creature::GetCreature(*me, m_Instance->GetData64(eFoundryCreatures::ZiplineStalker)))
                                me->CastSpell(l_Zipline, eSpells::RideLoadingChain, true);
                        });

                        break;
                    }
                    default:
                        break;
                }
            }

            void RegeneratePower(Powers /*p_Power*/, int32& p_Value) override
            {
                /// Iron Maidens only regens by script
                p_Value = 0;
            }

            void SetPower(Powers p_Power, int32 p_Value) override
            {
                if (p_Value >= eIronMaidensDatas::FirstIronFuryAbility)
                {
                    if (!m_Events.HasEvent(eEvents::EventPenetratingShot))
                        m_Events.ScheduleEvent(eEvents::EventPenetratingShot, 1);
                }

                if (p_Value >= eIronMaidensDatas::SecondIronFuryAbility)
                {
                    if (!m_Events.HasEvent(eEvents::EventDeployTurret))
                        m_Events.ScheduleEvent(eEvents::EventDeployTurret, 1);
                }
            }

            void SpellHitTarget(Unit* p_Target, SpellInfo const* p_SpellInfo) override
            {
                if (p_Target == nullptr)
                    return;

                switch (p_SpellInfo->Id)
                {
                    case eSpells::SpellIronShotSearcher:
                    {
                        me->CastSpell(p_Target, eSpells::SpellIronShotDamage, false);
                        break;
                    }
                    case eIronMaidensSpells::IronWill:
                    {
                        me->SetPower(Powers::POWER_ENERGY, me->GetMaxPower(Powers::POWER_ENERGY));
                        m_CustomEvent.CancelEvent(eEvents::EventRegenIronFury);
                        break;
                    }
                    default:
                        break;
                }
            }

            void OnSpellCasted(SpellInfo const* p_SpellInfo) override
            {
                switch (p_SpellInfo->Id)
                {
                    case eSpells::SpellDeployTurretSummon:
                    {
                        if (Player* l_Victim = SelectMainTank())
                            me->GetMotionMaster()->MoveChase(l_Victim);

                        m_DeployTurret = false;
                        break;
                    }
                    default:
                        break;
                }
            }

            void SpellHit(Unit* p_Caster, SpellInfo const* p_SpellInfo) override
            {
                switch (p_SpellInfo->Id)
                {
                    case eIronMaidensSpells::IronWill:
                    {
                        /// If any boss's health drops below 20%, all bosses will ignore the next scheduled Dreadnaught phase
                        m_CustomEvent.DelayEvent(eEvents::EventDreadnaughtPhase, eTimers::TimerDreadnaughtPhase);
                        break;
                    }
                    default:
                        break;
                }
            }

            void DamageTaken(Unit* /*p_Attacker*/, uint32& p_Damage, SpellInfo const* /*p_SpellInfo*/) override
            {
                if (m_BossDisabled)
                {
                    p_Damage = 0;
                    return;
                }

                if (!m_IronWillTriggered && me->HealthBelowPctDamaged(int32(eIronMaidensDatas::MaxHealthForIronWill), p_Damage))
                {
                    m_IronWillTriggered = true;
                    TriggerIronWill(m_Instance);
                }
                else if (p_Damage >= me->GetHealth())
                {
                    m_BossDisabled = true;
                    m_CanJumpToShip = false;

                    p_Damage = 0;

                    me->SetHealth(1);

                    me->CastSpell(me, eIronMaidensSpells::PermanentFeignDeath, true);

                    me->SetReactState(ReactStates::REACT_PASSIVE);

                    /// From retail
                    me->SetFlag(EUnitFields::UNIT_FIELD_FLAGS, eUnitFlags::UNIT_FLAG_DISABLE_MOVE | eUnitFlags::UNIT_FLAG_NOT_SELECTABLE | eUnitFlags::UNIT_FLAG_UNK_29);
                    me->SetFlag(EUnitFields::UNIT_FIELD_FLAGS_2, eUnitFlags2::UNIT_FLAG2_FEIGN_DEATH | eUnitFlags2::UNIT_FLAG2_DISABLE_TURN | eUnitFlags2::UNIT_FLAG2_UNK5);
                }
            }

            void UpdateAI(uint32 const p_Diff) override
            {
                UpdateOperations(p_Diff);

                if (!UpdateVictim())
                    return;

                if (!m_IsOnBoat && me->GetDistance(me->GetHomePosition()) >= 80.0f)
                {
                    EnterEvadeMode();
                    return;
                }

                /// Temp disable boss
                if (m_IsOnBoat)
                    return;

                m_CustomEvent.Update(p_Diff);
                m_Events.Update(p_Diff);

                switch (m_CustomEvent.ExecuteEvent())
                {
                    case eEvents::EventRegenIronFury:
                    {
                        me->ModifyPower(Powers::POWER_ENERGY, 1);
                        m_CustomEvent.ScheduleEvent(eEvents::EventRegenIronFury, eTimers::TimerEnergyRegen);
                        break;
                    }
                    case eEvents::EventDreadnaughtPhase:
                    {
                        if (m_Instance == nullptr)
                            break;

                        Creature* l_Boss    = nullptr;
                        float l_HealthPct   = 100.0f;

                        for (uint8 l_I = 0; l_I < 3; ++l_I)
                        {
                            if (Creature* l_Maiden = Creature::GetCreature(*me, m_Instance->GetData64(g_IronMaidensEntries[l_I])))
                            {
                                /// Boss cannot go to ship
                                if (!l_Maiden->IsAIEnabled || !l_Maiden->AI()->GetData(eIronMaidensDatas::IsAvailableForShip))
                                    continue;

                                if (l_Maiden->GetHealthPct() < l_HealthPct)
                                {
                                    l_Boss      = l_Maiden;
                                    l_HealthPct = l_Boss->GetHealthPct();
                                }
                            }
                        }

                        if (l_Boss == nullptr)
                            break;

                        /// Prevent boss to go to ship again
                        l_Boss->AI()->SetData(eIronMaidensDatas::IsAvailableForShip, uint32(false));
                        l_Boss->AI()->DoAction(eIronMaidensActions::ActionJumpToShip);

                        /// Enable interaction with Loading Chains
                        for (uint64 l_Guid : m_LoadingChains)
                        {
                            if (Creature* l_Chain = Creature::GetCreature(*me, l_Guid))
                                l_Chain->SetFlag(EUnitFields::UNIT_FIELD_NPC_FLAGS, NPCFlags::UNIT_NPC_FLAG_SPELLCLICK);
                        }

                        m_CustomEvent.ScheduleEvent(eEvents::EventDreadnaughtPhase, eTimers::TimerDreadnaughtPhaseCD);
                        break;
                    }
                    default:
                        break;
                }

                if (me->HasUnitState(UnitState::UNIT_STATE_CASTING))
                    return;

                switch (m_Events.ExecuteEvent())
                {
                    case eEvents::EventRapidFire:
                    {
                        if (Creature* l_Stalker = me->SummonCreature(eIronMaidensCreatures::RapidFireStalker, *me))
                        {
                            l_Stalker->DespawnOrUnsummon(11 * TimeConstants::IN_MILLISECONDS);

                            me->CastSpell(l_Stalker, eSpells::SpellRapideFirePeriodic, false);

                            if (Unit* l_Target = SelectTarget(SelectAggroTarget::SELECT_TARGET_RANDOM))
                            {
                                me->CastSpell(l_Target, eSpells::SpellRapidFireRedArrow, true);

                                uint64 l_TargetGUID = l_Target->GetGUID();
                                uint64 l_Guid       = l_Stalker->GetGUID();
                                AddTimedDelayedOperation(3 * TimeConstants::IN_MILLISECONDS, [this, l_Guid, l_TargetGUID]() -> void
                                {
                                    if (Creature* l_Stalker = Creature::GetCreature(*me, l_Guid))
                                    {
                                        if (l_Stalker->IsAIEnabled)
                                            l_Stalker->AI()->SetGUID(l_TargetGUID);

                                        l_Stalker->CastSpell(l_Stalker, eSpells::SpellRapidFireTargetVisual, true);
                                    }
                                });
                            }
                        }

                        m_Events.ScheduleEvent(eEvents::EventRapidFire, eTimers::TimerRapidFireCD);
                        break;
                    }
                    case eEvents::EventPenetratingShot:
                    {
                        if (Player* l_Target = SelectRangedTarget())
                            me->CastSpell(l_Target, eSpells::SpellPenetratingShotAura, false);

                        m_Events.ScheduleEvent(eEvents::EventPenetratingShot, eTimers::TimerPenetratingShotCD);
                        break;
                    }
                    case eEvents::EventDeployTurret:
                    {
                        m_DeployTurret = true;

                        float l_Range = frand(10.0f, 25.0f);
                        float l_Angle = frand(0.0f, 2 * M_PI);

                        Position l_JumpPos;

                        l_JumpPos.m_positionX = g_RoomCenterPos.m_positionX + l_Range * cos(l_Angle);
                        l_JumpPos.m_positionY = g_RoomCenterPos.m_positionY + l_Range * sin(l_Angle);
                        l_JumpPos.m_positionZ = g_RoomCenterPos.m_positionZ;

                        me->CastSpell(l_JumpPos, eSpells::SpellDeployTurretJump, true);

                        m_Events.ScheduleEvent(eEvents::EventDeployTurret, eTimers::TimerDeployTurretCD);
                        break;
                    }
                    case eEvents::EventJumpToShip:
                    {
                        m_IsOnBoat = true;

                        me->SetReactState(ReactStates::REACT_PASSIVE);

                        me->AttackStop();

                        me->GetMotionMaster()->MovePoint(eMoves::MoveToZipline, g_EnterZiplinePos);

                        me->SummonCreature(eIronMaidensCreatures::Uktar, *g_ShipSpawnPos[eIronMaidensCreatures::Uktar].begin());
                        me->SummonCreature(eIronMaidensCreatures::BattleMedicRogg, *g_ShipSpawnPos[eIronMaidensCreatures::BattleMedicRogg].begin());
                        break;
                    }
                    default:
                        break;
                }

                /// Must be checked again, before using Iron Shot
                if (me->HasUnitState(UnitState::UNIT_STATE_CASTING) || m_DeployTurret)
                    return;

                me->CastSpell(me, eSpells::SpellIronShotSearcher, true);
            }

            void RemoveCombatAuras()
            {
                if (m_Instance == nullptr)
                    return;

                m_Instance->DoRemoveAurasDueToSpellOnPlayers(eSpells::SpellRapidFireRedArrow);
                m_Instance->DoRemoveAurasDueToSpellOnPlayers(eSpells::SpellDominatorBlastDoT);
            }
        };

        CreatureAI* GetAI(Creature* p_Creature) const override
        {
            return new boss_admiral_garanAI(p_Creature);
        }
};

/// Enforcer Sorka - 77231
class boss_enforcer_sorka : public CreatureScript
{
    public:
        boss_enforcer_sorka() : CreatureScript("boss_enforcer_sorka") { }

        enum eSpells
        {
            /// Combat Spells
            /// Blade Dash
            SpellBladeDashCast          = 155794,
            SpellBladeDashDamage        = 155841,
            /// Convulsive Shadows
            SpellConvulsiveShadowsCast  = 156109,
            SpellConvulsiveShadowsAura  = 156214,
            /// Dark Hunt
            SpellDarkHuntAura           = 158315
        };

        enum eEvents
        {
            EventBladeDash = 1,
            EventRegenIronFury,
            EventConvulsiveShadows,
            EventDarkHunt,
            EventJumpToShip
        };

        enum eTimers
        {
            TimerBladeDash              = 11 * TimeConstants::IN_MILLISECONDS,
            TimerBladeDashCD            = 20 * TimeConstants::IN_MILLISECONDS,
            TimerEnergyRegen            = 6 * TimeConstants::IN_MILLISECONDS + 333,
            TimerConvulsiveShadowsCD    = 56 * TimeConstants::IN_MILLISECONDS,
            TimerDarkHuntCD             = 13 * TimeConstants::IN_MILLISECONDS + 500
        };

        enum eTalks
        {
        };

        enum eMoves
        {
            MoveJump = 1,
            MoveToZipline,
            MoveExitZipline = 50
        };

        struct boss_enforcer_sorkaAI : public BossAI
        {
            boss_enforcer_sorkaAI(Creature* p_Creature) : BossAI(p_Creature, eFoundryDatas::DataIronMaidens)
            {
                m_Instance = p_Creature->GetInstanceScript();

                p_Creature->AddUnitMovementFlag(MovementFlags::MOVEMENTFLAG_DISABLE_COLLISION);
            }

            InstanceScript* m_Instance;

            EventMap m_CustomEvent;
            EventMap m_Events;

            bool m_IsInBladeDash;
            std::set<uint64> m_BladeDashTargets;

            bool m_IronWillTriggered;
            bool m_BossDisabled;
            bool m_CanJumpToShip;
            bool m_IsOnBoat;

            void Reset() override
            {
                me->setPowerType(Powers::POWER_ENERGY);
                me->SetMaxPower(Powers::POWER_ENERGY, 100);
                me->SetPower(Powers::POWER_ENERGY, 0);

                me->SetCanFly(false);
                me->SetDisableGravity(false);
                me->SetHover(false);

                me->SetReactState(ReactStates::REACT_AGGRESSIVE);

                me->CastSpell(me, eIronMaidensSpells::ZeroPowerZeroRegen, true);

                me->RemoveAura(eIronMaidensSpells::IronWill);
                me->RemoveAura(eIronMaidensSpells::PermanentFeignDeath);
                me->RemoveAura(eIronMaidensSpells::WarmingUpAura);

                me->RemoveFlag(EUnitFields::UNIT_FIELD_FLAGS, eUnitFlags::UNIT_FLAG_DISABLE_MOVE | eUnitFlags::UNIT_FLAG_NOT_SELECTABLE | eUnitFlags::UNIT_FLAG_UNK_29);
                me->RemoveFlag(EUnitFields::UNIT_FIELD_FLAGS_2, eUnitFlags2::UNIT_FLAG2_FEIGN_DEATH | eUnitFlags2::UNIT_FLAG2_DISABLE_TURN | eUnitFlags2::UNIT_FLAG2_UNK5);

                m_CustomEvent.Reset();
                m_Events.Reset();

                RespawnMaidens(m_Instance, me);

                _Reset();

                m_IsInBladeDash = false;
                m_BladeDashTargets.clear();

                m_IronWillTriggered = false;
                m_BossDisabled      = false;
                m_CanJumpToShip     = true;
                m_IsOnBoat          = false;
            }

            uint32 GetData(uint32 p_ID) override
            {
                switch (p_ID)
                {
                    case eIronMaidensDatas::IsAvailableForShip:
                        return uint32(m_CanJumpToShip);
                    default:
                        break;
                }

                return 0;
            }

            void SetData(uint32 p_ID, uint32 p_Value) override
            {
                switch (p_ID)
                {
                    case eIronMaidensDatas::IsAvailableForShip:
                    {
                        m_CanJumpToShip = p_Value != 0;
                        break;
                    }
                    default:
                        break;
                }
            }

            void KilledUnit(Unit* p_Killed) override
            {
                /*if (p_Killed->IsPlayer())
                    Talk(eThogarTalks::TalkSlay);*/
            }

            void EnterCombat(Unit* p_Attacker) override
            {
                StartMaidens(m_Instance, me, p_Attacker);

                _EnterCombat();

                m_Events.ScheduleEvent(eEvents::EventBladeDash, eTimers::TimerBladeDash);

                m_CustomEvent.ScheduleEvent(eEvents::EventRegenIronFury, eTimers::TimerEnergyRegen);
            }

            void JustDied(Unit* /*p_Killer*/) override
            {
                me->RemoveAllAreasTrigger();

                summons.DespawnAll();

                _JustDied();

                if (m_Instance != nullptr)
                    m_Instance->SendEncounterUnit(EncounterFrameType::ENCOUNTER_FRAME_DISENGAGE, me);

                RemoveCombatAuras();
            }

            void EnterEvadeMode() override
            {
                me->ExitVehicle();

                me->NearTeleportTo(me->GetHomePosition());

                WipeMaidens(m_Instance);

                CreatureAI::EnterEvadeMode();

                me->RemoveAllAreasTrigger();

                summons.DespawnAll();

                RemoveCombatAuras();
            }

            void DoAction(int32 const p_Action) override
            {
                switch (p_Action)
                {
                    case eIronMaidensActions::ActionAfterTrashesIntro:
                    {
                        AddTimedDelayedOperation(13 * TimeConstants::IN_MILLISECONDS, [this]() -> void
                        {
                            me->GetMotionMaster()->MoveJump(g_SorkaHomePos, 30.0f, 20.0f, eMoves::MoveJump);
                        });

                        break;
                    }
                    case eIronMaidensActions::ActionJumpToShip:
                    {
                        m_Events.ScheduleEvent(eEvents::EventJumpToShip, 1);
                        break;
                    }
                    default:
                        break;
                }
            }

            void MovementInform(uint32 p_Type, uint32 p_ID) override
            {
                if (p_Type != MovementGeneratorType::EFFECT_MOTION_TYPE)
                    return;

                switch (p_ID)
                {
                    case eMoves::MoveJump:
                    {
                        me->SetHomePosition(g_SorkaHomePos);
                        break;
                    }
                    case eMoves::MoveToZipline:
                    {
                        AddTimedDelayedOperation(10, [this]() -> void
                        {
                            if (m_Instance == nullptr)
                                return;

                            m_IsOnBoat = true;

                            if (Creature* l_Zipline = Creature::GetCreature(*me, m_Instance->GetData64(eFoundryCreatures::ZiplineStalker)))
                                me->EnterVehicle(l_Zipline);
                        });

                        break;
                    }
                    default:
                        break;
                }
            }

            void RegeneratePower(Powers /*p_Power*/, int32& p_Value) override
            {
                /// Iron Maidens only regens by script
                p_Value = 0;
            }

            void SetPower(Powers p_Power, int32 p_Value) override
            {
                if (p_Value >= eIronMaidensDatas::FirstIronFuryAbility)
                {
                    if (!m_Events.HasEvent(eEvents::EventConvulsiveShadows))
                        m_Events.ScheduleEvent(eEvents::EventConvulsiveShadows, 1);
                }

                if (p_Value >= eIronMaidensDatas::SecondIronFuryAbility)
                {
                    if (!m_Events.HasEvent(eEvents::EventDarkHunt))
                        m_Events.ScheduleEvent(eEvents::EventDarkHunt, 1);
                }
            }

            void SpellHitTarget(Unit* p_Target, SpellInfo const* p_SpellInfo) override
            {
                if (p_Target == nullptr)
                    return;

                switch (p_SpellInfo->Id)
                {
                    case eSpells::SpellBladeDashCast:
                    {
                        if (!p_Target->IsPlayer())
                            break;

                        DashToTarget(p_Target);

                        /// Just for safety...
                        AddTimedDelayedOperation(12 * TimeConstants::IN_MILLISECONDS, [this]() -> void
                        {
                            m_IsInBladeDash = false;
                        });

                        break;
                    }
                    case eSpells::SpellBladeDashDamage:
                    {
                        if (!m_IsInBladeDash)
                        {
                            if (Player* l_Target = SelectMainTank())
                                me->GetMotionMaster()->MoveChase(l_Target);

                            me->RemoveAura(eSpells::SpellBladeDashCast);
                            break;
                        }

                        /// Cannot select a player twice
                        m_BladeDashTargets.insert(p_Target->GetGUID());

                        std::list<Player*> l_PossibleTargets;
                        me->GetPlayerListInGrid(l_PossibleTargets, 8.0f);

                        if (!l_PossibleTargets.empty())
                        {
                            l_PossibleTargets.remove_if([this](Player* p_Player) -> bool
                            {
                                if (p_Player == nullptr || m_BladeDashTargets.find(p_Player->GetGUID()) != m_BladeDashTargets.end())
                                    return true;

                                return false;
                            });
                        }

                        if (l_PossibleTargets.empty())
                        {
                            m_IsInBladeDash = false;

                            if (Player* l_Target = SelectMainTank())
                                DashToTarget(l_Target);

                            break;
                        }

                        auto l_Iter = l_PossibleTargets.begin();
                        std::advance(l_Iter, urand(0, l_PossibleTargets.size() - 1));

                        DashToTarget(*l_Iter);
                        break;
                    }
                    case eSpells::SpellConvulsiveShadowsCast:
                    {
                        p_Target->CastSpell(p_Target, eSpells::SpellConvulsiveShadowsAura, true, nullptr, nullptr, me->GetGUID());

                        if (Aura* l_Aura = p_Target->GetAura(eSpells::SpellConvulsiveShadowsAura))
                            l_Aura->SetStackAmount(4);

                        break;
                    }
                    case eIronMaidensSpells::IronWill:
                    {
                        me->SetPower(Powers::POWER_ENERGY, me->GetMaxPower(Powers::POWER_ENERGY));
                        m_CustomEvent.CancelEvent(eEvents::EventRegenIronFury);
                        break;
                    }
                    default:
                        break;
                }
            }

            void DamageTaken(Unit* /*p_Attacker*/, uint32& p_Damage, SpellInfo const* /*p_SpellInfo*/) override
            {
                if (m_BossDisabled)
                {
                    p_Damage = 0;
                    return;
                }

                if (!m_IronWillTriggered && me->HealthBelowPctDamaged(int32(eIronMaidensDatas::MaxHealthForIronWill), p_Damage))
                {
                    m_IronWillTriggered = true;
                    TriggerIronWill(m_Instance);
                }
                else if (p_Damage >= me->GetHealth())
                {
                    m_BossDisabled = true;
                    m_CanJumpToShip = false;

                    p_Damage = 0;

                    me->SetHealth(1);

                    me->CastSpell(me, eIronMaidensSpells::PermanentFeignDeath, true);

                    me->SetReactState(ReactStates::REACT_PASSIVE);

                    /// From retail
                    me->SetFlag(EUnitFields::UNIT_FIELD_FLAGS, eUnitFlags::UNIT_FLAG_DISABLE_MOVE | eUnitFlags::UNIT_FLAG_NOT_SELECTABLE | eUnitFlags::UNIT_FLAG_UNK_29);
                    me->SetFlag(EUnitFields::UNIT_FIELD_FLAGS_2, eUnitFlags2::UNIT_FLAG2_FEIGN_DEATH | eUnitFlags2::UNIT_FLAG2_DISABLE_TURN | eUnitFlags2::UNIT_FLAG2_UNK5);
                }
            }

            void UpdateAI(uint32 const p_Diff) override
            {
                UpdateOperations(p_Diff);

                if (!UpdateVictim())
                    return;

                if (!m_IsOnBoat && me->GetDistance(me->GetHomePosition()) >= 80.0f)
                {
                    EnterEvadeMode();
                    return;
                }

                /// Temp disable boss
                if (m_IsOnBoat)
                    return;

                m_CustomEvent.Update(p_Diff);
                m_Events.Update(p_Diff);

                switch (m_CustomEvent.ExecuteEvent())
                {
                    case eEvents::EventRegenIronFury:
                    {
                        me->ModifyPower(Powers::POWER_ENERGY, 1);
                        m_CustomEvent.ScheduleEvent(eEvents::EventRegenIronFury, eTimers::TimerEnergyRegen);
                        break;
                    }
                    default:
                        break;
                }

                if (me->HasUnitState(UnitState::UNIT_STATE_CASTING) || m_IsInBladeDash)
                    return;

                switch (m_Events.ExecuteEvent())
                {
                    case eEvents::EventBladeDash:
                    {
                        m_IsInBladeDash = true;
                        m_BladeDashTargets.clear();

                        if (Unit* l_Target = SelectTarget(SelectAggroTarget::SELECT_TARGET_RANDOM, 0, 45.0f, true, -int32(eIronMaidensSpells::OnABoatPeriodic)))
                        {
                            me->SetFacingTo(me->GetAngle(l_Target));

                            uint64 l_Guid = l_Target->GetGUID();
                            AddTimedDelayedOperation(10, [this, l_Guid]() -> void
                            {
                                if (Unit* l_Target = Unit::GetUnit(*me, l_Guid))
                                    me->CastSpell(l_Target, eSpells::SpellBladeDashCast, false);
                            });
                        }

                        m_Events.ScheduleEvent(eEvents::EventBladeDash, eTimers::TimerBladeDashCD);
                        break;
                    }
                    case eEvents::EventConvulsiveShadows:
                    {
                        if (Unit* l_Target = SelectTarget(SelectAggroTarget::SELECT_TARGET_RANDOM, 0, 0.0f, true))
                            me->CastSpell(l_Target, eSpells::SpellConvulsiveShadowsCast, false);

                        m_Events.ScheduleEvent(eEvents::EventConvulsiveShadows, eTimers::TimerConvulsiveShadowsCD);
                        break;
                    }
                    case eEvents::EventDarkHunt:
                    {
                        if (Unit* l_Target = SelectTarget(SelectAggroTarget::SELECT_TARGET_RANDOM, 0, 0.0f, true))
                            me->CastSpell(l_Target, eSpells::SpellDarkHuntAura, true);

                        m_Events.ScheduleEvent(eEvents::EventDarkHunt, eTimers::TimerDarkHuntCD);
                        break;
                    }
                    case eEvents::EventJumpToShip:
                    {
                        m_IsOnBoat = true;

                        me->SetReactState(ReactStates::REACT_PASSIVE);

                        me->AttackStop();

                        me->GetMotionMaster()->MovePoint(eMoves::MoveToZipline, g_EnterZiplinePos);

                        me->SummonCreature(eIronMaidensCreatures::Gorak, *g_ShipSpawnPos[eIronMaidensCreatures::Gorak].begin());
                        me->SummonCreature(eIronMaidensCreatures::IronEviscerator, g_ShipSpawnPos[eIronMaidensCreatures::Gorak][0]);
                        me->SummonCreature(eIronMaidensCreatures::IronEviscerator, g_ShipSpawnPos[eIronMaidensCreatures::Gorak][1]);
                        break;
                    }
                    default:
                        break;
                }

                DoMeleeAttackIfReady();
            }

            void DashToTarget(Unit* p_Target)
            {
                me->NearTeleportTo(*p_Target);

                uint64 l_Guid = p_Target->GetGUID();
                AddTimedDelayedOperation(10, [this, l_Guid]() -> void
                {
                    if (Unit* l_Target = Unit::GetUnit(*me, l_Guid))
                        me->CastSpell(l_Target, eSpells::SpellBladeDashDamage, true);
                });
            }

            void RemoveCombatAuras()
            {
                if (m_Instance == nullptr)
                    return;

                m_Instance->DoRemoveAurasDueToSpellOnPlayers(eSpells::SpellConvulsiveShadowsAura);
                m_Instance->DoRemoveAurasDueToSpellOnPlayers(eSpells::SpellDarkHuntAura);
            }
        };

        CreatureAI* GetAI(Creature* p_Creature) const override
        {
            return new boss_enforcer_sorkaAI(p_Creature);
        }
};

/// Marak the Blooded - 77477
class boss_marak_the_blooded : public CreatureScript
{
    public:
        boss_marak_the_blooded() : CreatureScript("boss_marak_the_blooded") { }

        enum eSpells
        {
            /// Combat Spells
            /// Blood Ritual
            SpellBloodRitualAura                = 159724,
            SpellBloodRitualCast                = 158078,
            /// Bloodsoaked Heartseeker
            SpellBloodsoakedHeartseekerMarker   = 158010,
            SpellBloodsoakedHeartseekerCast     = 158008,
            SpellBloodsoakedHeartseekerDamage   = 158009,
            SpellBloodsoakedHeartseekerBounce   = 157950,
            /// Sanguine Strikes
            SpellSanguineStrikesAura            = 156601
        };

        enum eEvents
        {
            EventBloodRitual = 1,
            EventRegenIronFury,
            EventBloodsoakedHeartseeker,
            EventJumpToShip
        };

        enum eTimers
        {
            TimerBloodRitual                = 5 * TimeConstants::IN_MILLISECONDS,
            TimerBloodRitualCD              = 20 * TimeConstants::IN_MILLISECONDS,
            TimerEnergyRegen                = 6 * TimeConstants::IN_MILLISECONDS + 333,
            TimerBloodsoakedHeartseekerCD   = 70 * TimeConstants::IN_MILLISECONDS
        };

        enum eTalks
        {
        };

        enum eVisual
        {
            IntroVisual = 6636
        };

        enum eMoves
        {
            MoveJump = 1,
            MoveDown,
            MoveLast,
            MoveToZipline,
            MoveExitZipline = 50
        };

        struct boss_marak_the_bloodedAI : public BossAI
        {
            boss_marak_the_bloodedAI(Creature* p_Creature) : BossAI(p_Creature, eFoundryDatas::DataIronMaidens)
            {
                m_Instance = p_Creature->GetInstanceScript();

                p_Creature->AddUnitMovementFlag(MovementFlags::MOVEMENTFLAG_DISABLE_COLLISION);
            }

            InstanceScript* m_Instance;

            EventMap m_CustomEvent;
            EventMap m_Events;

            uint8 m_HeartseekerID;
            std::array<uint64, eIronMaidensDatas::MaxHeartseekerTargets> m_HeartseekerTargets;

            bool m_IronWillTriggered;
            bool m_BossDisabled;
            bool m_CanJumpToShip;
            bool m_IsOnBoat;

            void Reset() override
            {
                me->setPowerType(Powers::POWER_ENERGY);
                me->SetMaxPower(Powers::POWER_ENERGY, 100);
                me->SetPower(Powers::POWER_ENERGY, 0);

                me->SetCanFly(false);
                me->SetDisableGravity(false);
                me->SetHover(false);

                me->SetReactState(ReactStates::REACT_AGGRESSIVE);

                me->CastSpell(me, eIronMaidensSpells::ZeroPowerZeroRegen, true);

                me->RemoveAura(eSpells::SpellSanguineStrikesAura);

                me->RemoveAura(eIronMaidensSpells::IronWill);
                me->RemoveAura(eIronMaidensSpells::PermanentFeignDeath);
                me->RemoveAura(eIronMaidensSpells::WarmingUpAura);

                me->RemoveFlag(EUnitFields::UNIT_FIELD_FLAGS, eUnitFlags::UNIT_FLAG_DISABLE_MOVE | eUnitFlags::UNIT_FLAG_NOT_SELECTABLE | eUnitFlags::UNIT_FLAG_UNK_29);
                me->RemoveFlag(EUnitFields::UNIT_FIELD_FLAGS_2, eUnitFlags2::UNIT_FLAG2_FEIGN_DEATH | eUnitFlags2::UNIT_FLAG2_DISABLE_TURN | eUnitFlags2::UNIT_FLAG2_UNK5);

                m_CustomEvent.Reset();
                m_Events.Reset();

                RespawnMaidens(m_Instance, me);

                _Reset();

                ResetHeartseekerTargets();

                m_IronWillTriggered = false;
                m_BossDisabled      = false;
                m_CanJumpToShip     = true;
                m_IsOnBoat          = false;
            }

            uint32 GetData(uint32 p_ID) override
            {
                switch (p_ID)
                {
                    case eIronMaidensDatas::IsAvailableForShip:
                        return uint32(m_CanJumpToShip);
                    default:
                        break;
                }

                return 0;
            }

            void SetData(uint32 p_ID, uint32 p_Value) override
            {
                switch (p_ID)
                {
                    case eIronMaidensDatas::IsAvailableForShip:
                    {
                        m_CanJumpToShip = p_Value != 0;
                        break;
                    }
                    default:
                        break;
                }
            }

            void KilledUnit(Unit* p_Killed) override
            {
                /*if (p_Killed->IsPlayer())
                    Talk(eThogarTalks::TalkSlay);*/
            }

            void EnterCombat(Unit* p_Attacker) override
            {
                StartMaidens(m_Instance, me, p_Attacker);

                _EnterCombat();

                m_Events.ScheduleEvent(eEvents::EventBloodRitual, eTimers::TimerBloodRitual);

                m_CustomEvent.ScheduleEvent(eEvents::EventRegenIronFury, eTimers::TimerEnergyRegen);
            }

            void JustDied(Unit* /*p_Killer*/) override
            {
                me->RemoveAllAreasTrigger();

                summons.DespawnAll();

                _JustDied();

                if (m_Instance != nullptr)
                    m_Instance->SendEncounterUnit(EncounterFrameType::ENCOUNTER_FRAME_DISENGAGE, me);

                RemoveCombatAuras();
            }

            void EnterEvadeMode() override
            {
                me->ExitVehicle();

                me->NearTeleportTo(me->GetHomePosition());

                WipeMaidens(m_Instance);

                CreatureAI::EnterEvadeMode();

                me->RemoveAllAreasTrigger();

                summons.DespawnAll();

                RemoveCombatAuras();
            }

            void DoAction(int32 const p_Action) override
            {
                switch (p_Action)
                {
                    case eIronMaidensActions::ActionAfterTrashesIntro:
                    {
                        me->GetMotionMaster()->MoveJump(g_BoatBossFirstJumpPos, 30.0f, 20.0f, eMoves::MoveJump);
                        break;
                    }
                    case eIronMaidensActions::ActionJumpToShip:
                    {
                        m_Events.ScheduleEvent(eEvents::EventJumpToShip, 1);
                        break;
                    }
                    default:
                        break;
                }
            }

            void MovementInform(uint32 p_Type, uint32 p_ID) override
            {
                if (p_Type != MovementGeneratorType::EFFECT_MOTION_TYPE &&
                    p_Type != MovementGeneratorType::POINT_MOTION_TYPE)
                    return;

                switch (p_ID)
                {
                    case eMoves::MoveJump:
                    {
                        me->SetAIAnimKitId(eVisual::IntroVisual);

                        AddTimedDelayedOperation(10, [this]() -> void
                        {
                            me->AddUnitMovementFlag(MovementFlags::MOVEMENTFLAG_FLYING);
                            me->SetSpeed(UnitMoveType::MOVE_FLIGHT, 4.0f);
                            me->GetMotionMaster()->MoveSmoothFlyPath(eMoves::MoveDown, g_BoatBossFlyingMoves.data(), g_BoatBossFlyingMoves.size());
                        });

                        break;
                    }
                    case eMoves::MoveDown:
                    {
                        me->SetAIAnimKitId(0);

                        AddTimedDelayedOperation(10, [this]() -> void
                        {
                            me->RemoveUnitMovementFlag(MovementFlags::MOVEMENTFLAG_FLYING);
                            me->SetSpeed(UnitMoveType::MOVE_FLIGHT, me->GetCreatureTemplate()->speed_fly);
                            me->GetMotionMaster()->MoveJump(g_MarakHomePos, 30.0f, 20.0f, eMoves::MoveLast);
                        });

                        break;
                    }
                    case eMoves::MoveLast:
                    {
                        me->SetHomePosition(g_MarakHomePos);

                        AddTimedDelayedOperation(10, [this]() -> void
                        {
                            me->SetFacingTo(g_MarakFinalFacing);
                        });

                        break;
                    }
                    case eMoves::MoveToZipline:
                    {
                        AddTimedDelayedOperation(10, [this]() -> void
                        {
                            if (m_Instance == nullptr)
                                return;

                            m_IsOnBoat = true;

                            if (Creature* l_Zipline = Creature::GetCreature(*me, m_Instance->GetData64(eFoundryCreatures::ZiplineStalker)))
                                me->EnterVehicle(l_Zipline);
                        });

                        break;
                    }
                    default:
                        break;
                }
            }

            void RegeneratePower(Powers /*p_Power*/, int32& p_Value) override
            {
                /// Iron Maidens only regens by script
                p_Value = 0;
            }

            void SetPower(Powers p_Power, int32 p_Value) override
            {
                if (p_Value >= eIronMaidensDatas::FirstIronFuryAbility)
                {
                    if (!m_Events.HasEvent(eEvents::EventBloodsoakedHeartseeker))
                        m_Events.ScheduleEvent(eEvents::EventBloodsoakedHeartseeker, 1);
                }

                if (p_Value >= eIronMaidensDatas::SecondIronFuryAbility)
                    me->CastSpell(me, eSpells::SpellSanguineStrikesAura, true);
            }

            void SpellHitTarget(Unit* p_Target, SpellInfo const* p_SpellInfo) override
            {
                if (p_Target == nullptr)
                    return;

                switch (p_SpellInfo->Id)
                {
                    case eSpells::SpellBloodsoakedHeartseekerDamage:
                    {
                        HandleNextHeartseeker(false);
                        break;
                    }
                    case eIronMaidensSpells::IronWill:
                    {
                        me->SetPower(Powers::POWER_ENERGY, me->GetMaxPower(Powers::POWER_ENERGY));
                        m_CustomEvent.CancelEvent(eEvents::EventRegenIronFury);
                        break;
                    }
                    default:
                        break;
                }
            }

            void OnSpellCasted(SpellInfo const* p_SpellInfo) override
            {
                switch (p_SpellInfo->Id)
                {
                    case eSpells::SpellBloodRitualCast:
                    {
                        std::list<Player*> l_PlayerList;
                        me->GetPlayerListInGrid(l_PlayerList, 45.0f);

                        if (!l_PlayerList.empty())
                        {
                            l_PlayerList.remove_if([this](Player* p_Player) -> bool
                            {
                                if (p_Player == nullptr || !p_Player->isInFront(me))
                                    return true;

                                return false;
                            });
                        }

                        break;
                    }
                    case eSpells::SpellBloodsoakedHeartseekerCast:
                    {
                        HandleNextHeartseeker();
                        break;
                    }
                    default:
                        break;
                }
            }

            void DamageTaken(Unit* /*p_Attacker*/, uint32& p_Damage, SpellInfo const* /*p_SpellInfo*/) override
            {
                if (m_BossDisabled)
                {
                    p_Damage = 0;
                    return;
                }

                if (!m_IronWillTriggered && me->HealthBelowPctDamaged(int32(eIronMaidensDatas::MaxHealthForIronWill), p_Damage))
                {
                    m_IronWillTriggered = true;
                    TriggerIronWill(m_Instance);
                }
                else if (p_Damage >= me->GetHealth())
                {
                    m_BossDisabled = true;
                    m_CanJumpToShip = false;

                    p_Damage = 0;

                    me->SetHealth(1);

                    me->CastSpell(me, eIronMaidensSpells::PermanentFeignDeath, true);

                    me->SetReactState(ReactStates::REACT_PASSIVE);

                    /// From retail
                    me->SetFlag(EUnitFields::UNIT_FIELD_FLAGS, eUnitFlags::UNIT_FLAG_DISABLE_MOVE | eUnitFlags::UNIT_FLAG_NOT_SELECTABLE | eUnitFlags::UNIT_FLAG_UNK_29);
                    me->SetFlag(EUnitFields::UNIT_FIELD_FLAGS_2, eUnitFlags2::UNIT_FLAG2_FEIGN_DEATH | eUnitFlags2::UNIT_FLAG2_DISABLE_TURN | eUnitFlags2::UNIT_FLAG2_UNK5);
                }
            }

            void UpdateAI(uint32 const p_Diff) override
            {
                UpdateOperations(p_Diff);

                if (!UpdateVictim())
                    return;

                if (!m_IsOnBoat && me->GetDistance(me->GetHomePosition()) >= 80.0f)
                {
                    EnterEvadeMode();
                    return;
                }

                /// Temp disable boss
                if (m_IsOnBoat)
                    return;

                m_CustomEvent.Update(p_Diff);
                m_Events.Update(p_Diff);

                switch (m_CustomEvent.ExecuteEvent())
                {
                    case eEvents::EventRegenIronFury:
                    {
                        me->ModifyPower(Powers::POWER_ENERGY, 1);
                        m_CustomEvent.ScheduleEvent(eEvents::EventRegenIronFury, eTimers::TimerEnergyRegen);
                        break;
                    }
                    default:
                        break;
                }

                if (me->HasUnitState(UnitState::UNIT_STATE_CASTING))
                    return;

                switch (m_Events.ExecuteEvent())
                {
                    case eEvents::EventBloodRitual:
                    {
                        if (Unit* l_Target = SelectTarget(SelectAggroTarget::SELECT_TARGET_RANDOM, 0, 45.0f, true))
                        {
                            me->CastSpell(l_Target, eSpells::SpellBloodRitualAura, true);
                            me->CastSpell(l_Target, eSpells::SpellBloodRitualCast, false);
                        }

                        m_Events.ScheduleEvent(eEvents::EventBloodRitual, eTimers::TimerBloodRitualCD);
                        break;
                    }
                    case eEvents::EventBloodsoakedHeartseeker:
                    {
                        ResetHeartseekerTargets();

                        for (uint8 l_I = 0; l_I < eIronMaidensDatas::MaxHeartseekerTargets; ++l_I)
                        {
                            if (Unit* l_Target = SelectTarget(SelectAggroTarget::SELECT_TARGET_RANDOM, 0, 0.0f, true, -int32(eSpells::SpellBloodsoakedHeartseekerMarker)))
                            {
                                m_HeartseekerTargets[l_I] = l_Target->GetGUID();

                                me->CastSpell(l_Target, eSpells::SpellBloodsoakedHeartseekerMarker, true);
                            }
                        }

                        me->CastSpell(me, eSpells::SpellBloodsoakedHeartseekerCast, false);

                        m_Events.ScheduleEvent(eEvents::EventBloodsoakedHeartseeker, eTimers::TimerBloodsoakedHeartseekerCD);
                        break;
                    }
                    case eEvents::EventJumpToShip:
                    {
                        m_IsOnBoat = true;

                        me->SetReactState(ReactStates::REACT_PASSIVE);

                        me->AttackStop();

                        me->GetMotionMaster()->MovePoint(eMoves::MoveToZipline, g_EnterZiplinePos);

                        me->SummonCreature(eIronMaidensCreatures::Ukurogg, *g_ShipSpawnPos[eIronMaidensCreatures::Ukurogg].begin());
                        break;
                    }
                    default:
                        break;
                }

                DoMeleeAttackIfReady();
            }

            void ResetHeartseekerTargets()
            {
                m_HeartseekerID = 0;

                for (uint8 l_I = 0; l_I < eIronMaidensDatas::MaxHeartseekerTargets; ++l_I)
                    m_HeartseekerTargets[l_I] = 0;
            }

            void HandleNextHeartseeker(bool p_FromBoss = true)
            {
                if (m_HeartseekerID >= eIronMaidensDatas::MaxHeartseekerTargets)
                    return;

                Unit* l_Caster = p_FromBoss ? me : Unit::GetUnit(*me, m_HeartseekerTargets[m_HeartseekerID > 0 ? m_HeartseekerID - 1 : 0]);
                if (l_Caster == nullptr)
                    return;

                if (Unit* l_Target = Unit::GetUnit(*me, m_HeartseekerTargets[m_HeartseekerID++]))
                {
                    l_Caster->CastSpell(l_Target, eSpells::SpellBloodsoakedHeartseekerDamage, true, nullptr, nullptr, me->GetGUID());
                    l_Caster->CastSpell(l_Target, eSpells::SpellBloodsoakedHeartseekerBounce, true, nullptr, nullptr, me->GetGUID());
                }
            }

            void RemoveCombatAuras()
            {
                if (m_Instance == nullptr)
                    return;

                m_Instance->DoRemoveAurasDueToSpellOnPlayers(eSpells::SpellBloodRitualAura);
                m_Instance->DoRemoveAurasDueToSpellOnPlayers(eSpells::SpellBloodsoakedHeartseekerMarker);
            }
        };

        CreatureAI* GetAI(Creature* p_Creature) const override
        {
            return new boss_marak_the_bloodedAI(p_Creature);
        }
};

/// Loading Chain - 78767
class npc_foundry_loading_chain : public CreatureScript
{
    public:
        npc_foundry_loading_chain() : CreatureScript("npc_foundry_loading_chain") { }

        enum eSpells
        {
            RideLoadingChain    = 158646,
            LoadingChainVisual  = 159086,
            LoadCrate           = 171209
        };

        enum eMoves
        {
            MoveBoss = 1,
            MoveBoat,
            MovePlayerBoat,
            MovePlayerBoss
        };

        struct npc_foundry_loading_chainAI : public ScriptedAI
        {
            npc_foundry_loading_chainAI(Creature* p_Creature) : ScriptedAI(p_Creature), m_ChainID(0), m_Vehicle(p_Creature->GetVehicleKit())
            {
                m_Instance = p_Creature->GetInstanceScript();
            }

            uint8 m_ChainID;
            bool m_IsAvailable;

            bool m_IsOnBoat;

            Vehicle* m_Vehicle;

            InstanceScript* m_Instance;

            void Reset() override
            {
                m_IsAvailable   = true;
                m_IsOnBoat      = false;

                me->AddUnitMovementFlag(MovementFlags::MOVEMENTFLAG_FLYING);

                me->CastSpell(me, eSpells::LoadingChainVisual, true);

                me->SetReactState(ReactStates::REACT_PASSIVE);

                me->SetUInt32Value(EUnitFields::UNIT_FIELD_INTERACT_SPELL_ID, eSpells::RideLoadingChain);

                me->SetFlag(EUnitFields::UNIT_FIELD_FLAGS, eUnitFlags::UNIT_FLAG_NON_ATTACKABLE | eUnitFlags::UNIT_FLAG_IMMUNE_TO_NPC);

                /// Init chain ID
                for (Position const l_Pos : g_LoadingChainsSpawnPos)
                {
                    if (me->IsNearPosition(&l_Pos, 0.5f))
                        break;

                    ++m_ChainID;
                }
            }

            void SpellHit(Unit* p_Attacker, SpellInfo const* p_SpellInfo) override
            {
                if (p_SpellInfo->Id == eSpells::LoadCrate && m_ChainID < eIronMaidensDatas::MaxLoadingChains)
                    me->GetMotionMaster()->MovePoint(eMoves::MoveBoat, g_LoadingChainsMoveBoatPos[m_ChainID]);
            }

            void MovementInform(uint32 p_Type, uint32 p_ID) override
            {
                if (p_Type != MovementGeneratorType::POINT_MOTION_TYPE)
                    return;

                switch (p_ID)
                {
                    case eMoves::MoveBoss:
                    {
                        m_IsAvailable   = true;
                        m_IsOnBoat      = false;
                        break;
                    }
                    case eMoves::MoveBoat:
                    {
                        m_IsOnBoat = true;

                        AddTimedDelayedOperation(1 * TimeConstants::IN_MILLISECONDS, [this]() -> void
                        {
                            me->RemoveAura(eSpells::LoadCrate);

                            me->GetMotionMaster()->MovePoint(eMoves::MoveBoss, g_LoadingChainsSpawnPos[m_ChainID]);
                        });

                        break;
                    }
                    case eMoves::MovePlayerBoat:
                    {
                        m_IsOnBoat = true;

                        if (m_Vehicle == nullptr || m_Vehicle->GetPassenger(0) == nullptr)
                            break;

                        if (Player* l_Passenger = m_Vehicle->GetPassenger(0)->ToPlayer())
                            l_Passenger->ExitVehicle();

                        break;
                    }
                    case eMoves::MovePlayerBoss:
                    {
                        m_IsOnBoat      = false;
                        m_IsAvailable   = true;

                        if (m_Vehicle == nullptr || m_Vehicle->GetPassenger(0) == nullptr)
                            break;

                        if (Player* l_Passenger = m_Vehicle->GetPassenger(0)->ToPlayer())
                            l_Passenger->ExitVehicle();

                        break;
                    }
                    default:
                        break;
                }
            }

            void SetData(uint32 p_ID, uint32 p_Value) override
            {
                switch (p_ID)
                {
                    case eIronMaidensDatas::LoadingChainAvailable:
                        m_IsAvailable = p_Value != 0;
                        break;
                    default:
                        break;
                }
            }

            uint32 GetData(uint32 p_ID = 0) override
            {
                switch (p_ID)
                {
                    case eIronMaidensDatas::LoadingChainID:
                        return uint32(m_ChainID);
                    case eIronMaidensDatas::LoadingChainAvailable:
                        return uint32(m_IsAvailable);
                    default:
                        return 0;
                }
            }

            void OnSpellClick(Unit* p_Clicker) override
            {
                if (!m_IsAvailable)
                    return;

                m_IsAvailable = false;

                p_Clicker->CastSpell(me, eSpells::RideLoadingChain, true);
            }

            void PassengerBoarded(Unit* p_Passenger, int8 p_SeatID, bool p_Apply) override
            {
                if (p_Apply && m_Instance != nullptr)
                {
                    /// Reset threat to prevent EnterEvadeMode
                    for (uint8 l_I = 0; l_I < 3; ++l_I)
                    {
                        if (Creature* l_Maiden = Creature::GetCreature(*me, m_Instance->GetData64(g_IronMaidensEntries[l_I])))
                        {
                            if (ScriptedAI* l_AI = CAST_AI(ScriptedAI, l_Maiden->AI()))
                                l_AI->DoModifyThreatPercent(p_Passenger, -100);
                        }
                    }

                    p_Passenger->SetCanFly(true);

                    AddTimedDelayedOperation(10, [this]() -> void
                    {
                        if (m_ChainID < eIronMaidensDatas::MaxLoadingChains)
                        {
                            if (m_IsOnBoat)
                                me->GetMotionMaster()->MovePoint(eMoves::MovePlayerBoss, g_LoadingChainsSpawnPos[m_ChainID]);
                            else
                                me->GetMotionMaster()->MovePoint(eMoves::MovePlayerBoat, g_LoadingChainsMoveBoatPos[m_ChainID]);
                        }
                    });
                }
                else
                    p_Passenger->SetCanFly(false);
            }

            void DamageTaken(Unit* /*p_Attacker*/, uint32& p_Damage, SpellInfo const* /*p_SpellInfo*/) override
            {
                p_Damage = 0;
            }

            void UpdateAI(uint32 const p_Diff)
            {
                UpdateOperations(p_Diff);
            }
        };

        CreatureAI* GetAI(Creature* p_Creature) const override
        {
            return new npc_foundry_loading_chainAI(p_Creature);
        }
};

/// Uk'urogg <Deckhand of Marak the Blooded> - 78341
class npc_foundry_ukurogg : public CreatureScript
{
    public:
        npc_foundry_ukurogg() : CreatureScript("npc_foundry_ukurogg") { }

        enum eSpells
        {
            CarryingCrate = 171198
        };

        enum eEvents
        {
        };

        enum eMoves
        {
            MoveCarryCrate = 1,
            MoveThrowCrate
        };

        struct npc_foundry_ukuroggAI : public ScriptedAI
        {
            npc_foundry_ukuroggAI(Creature* p_Creature) : ScriptedAI(p_Creature) { }

            Position const m_UkuroggThrowCratePos = { 495.576f, 3273.3f, 141.388f, 0.0f };
            Position const m_UkuroggCarryCratePos = { 478.2083f, 3280.669f, 141.388f, 0.0f };

            float const m_UkuroggFinalFacing = 2.859696f;

            EventMap m_Events;

            void Reset() override
            {
                m_Events.Reset();
            }

            void EnterCombat(Unit* /*p_Attacker*/) override
            {
                me->SetWalk(false);

                ClearDelayedOperations();
            }

            void SpellHit(Unit* p_Attacker, SpellInfo const* p_SpellInfo)
            {
                if (p_SpellInfo->Id == eSpells::CarryingCrate)
                {
                    AddTimedDelayedOperation(1 * TimeConstants::IN_MILLISECONDS, [this]() -> void
                    {
                        me->SetWalk(true);

                        me->GetMotionMaster()->MovePoint(eMoves::MoveThrowCrate, m_UkuroggThrowCratePos);
                    });
                }
            }

            void MovementInform(uint32 p_Type, uint32 p_ID) override
            {
                if (p_Type != MovementGeneratorType::POINT_MOTION_TYPE)
                    return;

                switch (p_ID)
                {
                    case eMoves::MoveCarryCrate:
                    {
                        AddTimedDelayedOperation(1 * TimeConstants::IN_MILLISECONDS, [this]() -> void
                        {
                            me->SetFacingTo(m_UkuroggFinalFacing);
                        });

                        break;
                    }
                    case eMoves::MoveThrowCrate:
                    {
                        AddTimedDelayedOperation(1 * TimeConstants::IN_MILLISECONDS, [this]() -> void
                        {
                            me->RemoveAura(eSpells::CarryingCrate);

                            me->GetMotionMaster()->MovePoint(eMoves::MoveCarryCrate, m_UkuroggCarryCratePos);
                        });

                        break;
                    }
                    default:
                        break;
                }
            }

            void UpdateAI(uint32 const p_Diff) override
            {
                UpdateOperations(p_Diff);

                if (!UpdateVictim())
                    return;

                m_Events.Update(p_Diff);

                if (me->HasUnitState(UnitState::UNIT_STATE_CASTING))
                    return;

                /*switch (m_Events.ExecuteEvent())
                {
                    default:
                        break;
                }*/

                DoMeleeAttackIfReady();
            }
        };

        CreatureAI* GetAI(Creature* p_Creature) const override
        {
            return new npc_foundry_ukuroggAI(p_Creature);
        }
};

/// Zipline Stalker - 82538
class npc_foundry_zipline_stalker : public CreatureScript
{
    public:
        npc_foundry_zipline_stalker() : CreatureScript("npc_foundry_zipline_stalker") { }

        enum eSpells
        {
            ZiplineStalkerVisual    = 166239,
            RideLoadingChain        = 158646
        };

        enum eMoves
        {
            MoveToShip,
            MoveExitZipline = 50
        };

        struct npc_foundry_zipline_stalkerAI : public ScriptedAI
        {
            npc_foundry_zipline_stalkerAI(Creature* p_Creature) : ScriptedAI(p_Creature), m_Vehicle(p_Creature->GetVehicleKit())
            {
                m_Instance = p_Creature->GetInstanceScript();
            }

            InstanceScript* m_Instance;

            Vehicle* m_Vehicle;

            void Reset() override
            {
                me->SetFlag(EUnitFields::UNIT_FIELD_FLAGS_2, eUnitFlags2::UNIT_FLAG2_DISABLE_TURN);

                me->SetReactState(ReactStates::REACT_PASSIVE);

                me->CastSpell(me, eSpells::ZiplineStalkerVisual, true);

                if (!me->IsNearPosition(&me->GetHomePosition(), 1.0f))
                    me->NearTeleportTo(me->GetHomePosition());
            }

            void MovementInform(uint32 p_Type, uint32 p_ID) override
            {
                if (p_Type != MovementGeneratorType::EFFECT_MOTION_TYPE)
                    return;

                switch (p_ID)
                {
                    case eMoves::MoveToShip:
                    {
                        if (m_Vehicle == nullptr)
                            break;

                        if (Unit* l_Passenger = m_Vehicle->GetPassenger(0))
                        {
                            l_Passenger->ExitVehicle();

                            uint64 l_Guid = l_Passenger->GetGUID();
                            AddTimedDelayedOperation(10, [this, l_Guid]() -> void
                            {
                                if (Unit* l_Passenger = Unit::GetUnit(*me, l_Guid))
                                    l_Passenger->GetMotionMaster()->MoveJump(g_ExitZiplinePos, 30.0f, 10.0f, eMoves::MoveExitZipline);
                            });
                        }

                        break;
                    }
                    default:
                        break;
                }
            }

            void PassengerBoarded(Unit* p_Passenger, int8 p_SeatID, bool p_Apply) override
            {
                if (p_Apply)
                {
                    p_Passenger->SetCanFly(true);
                    p_Passenger->SetDisableGravity(true);
                    p_Passenger->SetHover(true);

                    AddTimedDelayedOperation(10, [this]() -> void
                    {
                        me->AddUnitMovementFlag(MovementFlags::MOVEMENTFLAG_FLYING);
                        me->GetMotionMaster()->MoveSmoothFlyPath(eMoves::MoveToShip, g_ZiplineFlyingMoves.data(), g_ZiplineFlyingMoves.size());
                    });
                }
                else
                {
                    if (m_Instance == nullptr)
                        return;

                    uint64 l_Guid = p_Passenger->GetGUID();
                    AddTimedDelayedOperation(10, [this, l_Guid]() -> void
                    {
                        if (Unit* l_Passenger = Unit::GetUnit(*me, l_Guid))
                        {
                            if (Creature* l_Cannon = Creature::GetCreature(*l_Passenger, m_Instance->GetData64(eFoundryCreatures::IronCannon)))
                                l_Passenger->CastSpell(l_Cannon, eSpells::RideLoadingChain, true);
                        }
                    });
                }
            }

            void UpdateAI(uint32 const p_Diff) override
            {
                UpdateOperations(p_Diff);
            }
        };

        CreatureAI* GetAI(Creature* p_Creature) const override
        {
            return new npc_foundry_zipline_stalkerAI(p_Creature);
        }
};

/// Rapid Fire Stalker - 77636
class npc_foundry_rapid_fire_stalker : public CreatureScript
{
    public:
        npc_foundry_rapid_fire_stalker() : CreatureScript("npc_foundry_rapid_fire_stalker") { }

        struct npc_foundry_rapid_fire_stalkerAI : public ScriptedAI
        {
            npc_foundry_rapid_fire_stalkerAI(Creature* p_Creature) : ScriptedAI(p_Creature), m_TargetGUID(0) { }

            uint64 m_TargetGUID;

            void SetGUID(uint64 p_Guid, int32 p_ID /*= 0*/) override
            {
                m_TargetGUID = p_Guid;
            }

            void UpdateAI(uint32 const p_Diff) override
            {
                if (Unit* l_Target = Unit::GetUnit(*me, m_TargetGUID))
                {
                    me->GetMotionMaster()->Clear();
                    me->GetMotionMaster()->MovePoint(0, *l_Target, false);
                }
            }
        };

        CreatureAI* GetAI(Creature* p_Creature) const override
        {
            return new npc_foundry_rapid_fire_stalkerAI(p_Creature);
        }
};

/// Dominator Turret - 78583
class npc_foundry_dominator_turret : public CreatureScript
{
    public:
        npc_foundry_dominator_turret() : CreatureScript("npc_foundry_dominator_turret") { }

        enum eSpells
        {
            DominatorBlastTimingAura    = 158598,
            DominatorTurretDeathVisual  = 158640
        };

        struct npc_foundry_dominator_turretAI : public ScriptedAI
        {
            npc_foundry_dominator_turretAI(Creature* p_Creature) : ScriptedAI(p_Creature) { }

            void Reset() override
            {
                me->SetReactState(ReactStates::REACT_PASSIVE);

                me->CastSpell(me, eSpells::DominatorBlastTimingAura, true);
            }

            void JustDied(Unit* /*p_Killer*/) override
            {
                me->CastSpell(me, eSpells::DominatorTurretDeathVisual, true);
            }

            void UpdateAI(uint32 const p_Diff) override
            {
                me->SetFacingTo(me->m_orientation + M_PI / 32.0f);
            }
        };

        CreatureAI* GetAI(Creature* p_Creature) const override
        {
            return new npc_foundry_dominator_turretAI(p_Creature);
        }
};

/// Iron Cannon - 78152
class npc_foundry_iron_cannon : public CreatureScript
{
    public:
        npc_foundry_iron_cannon() : CreatureScript("npc_foundry_iron_cannon") { }

        enum eSpells
        {
            BombardmentPatternAlpha         = 157854,
            BombardmentPatternAlphaMissile  = 157856
        };

        struct npc_foundry_iron_cannonAI : public ScriptedAI
        {
            npc_foundry_iron_cannonAI(Creature* p_Creature) : ScriptedAI(p_Creature), m_Vehicle(p_Creature->GetVehicleKit()) { }

            Vehicle* m_Vehicle;

            void Reset() override
            {
                me->SetReactState(ReactStates::REACT_PASSIVE);

                me->SetFlag(EUnitFields::UNIT_FIELD_FLAGS, eUnitFlags::UNIT_FLAG_NOT_SELECTABLE | eUnitFlags::UNIT_FLAG_IMMUNE_TO_PC);
            }

            void PassengerBoarded(Unit* p_Passenger, int8 p_SeatID, bool p_Apply) override
            {
                if (p_Apply)
                {
                    me->CastSpell(me, eSpells::BombardmentPatternAlpha, false);

                    p_Passenger->CastSpell(p_Passenger, eIronMaidensSpells::WarmingUpAura, false);
                }
            }
        };

        CreatureAI* GetAI(Creature* p_Creature) const override
        {
            return new npc_foundry_iron_cannonAI(p_Creature);
        }
};

/// Uktar <Deckhand of Admiral Gar'an> - 78351
class npc_foundry_uktar : public CreatureScript
{
    public:
        npc_foundry_uktar() : CreatureScript("npc_foundry_uktar") { }

        enum eSpell
        {
            /// Grapeshot Blast
            SpellGrapeshotBlastCast = 158695
        };

        struct npc_foundry_uktarAI : public ScriptedAI
        {
            npc_foundry_uktarAI(Creature* p_Creature) : ScriptedAI(p_Creature)
            {
                m_Instance = p_Creature->GetInstanceScript();
            }

            InstanceScript* m_Instance;

            void Reset() override
            {
                me->SetReactState(ReactStates::REACT_PASSIVE);

                me->SetSheath(SheathState::SHEATH_STATE_RANGED);
            }

            void DamageTaken(Unit* /*p_Attacker*/, uint32& /*p_Damage*/, SpellInfo const* /*p_SpellInfo*/) override
            {
                me->SetReactState(ReactStates::REACT_AGGRESSIVE);
            }

            void JustDied(Unit* /*p_Killer*/) override
            {
                if (m_Instance == nullptr)
                    return;

                if (GameObject* l_AmmoLoader = GameObject::GetGameObject(*me, m_Instance->GetData64(eFoundryGameObjects::AmmoLoader)))
                    l_AmmoLoader->RemoveFlag(EGameObjectFields::GAMEOBJECT_FIELD_FLAGS, GameObjectFlags::GO_FLAG_NOT_SELECTABLE);
            }

            void UpdateAI(uint32 const /*p_Diff*/) override
            {
                if (!UpdateVictim())
                    return;

                if (me->HasUnitState(UnitState::UNIT_STATE_CASTING))
                    return;

                if (Unit* l_Target = SelectTarget(SelectAggroTarget::SELECT_TARGET_RANDOM, 0, 0.0f, true, eIronMaidensSpells::OnABoatPeriodic))
                    me->CastSpell(l_Target, eSpell::SpellGrapeshotBlastCast, false);
            }
        };

        CreatureAI* GetAI(Creature* p_Creature) const override
        {
            return new npc_foundry_uktarAI(p_Creature);
        }
};

/// Battle Medic Rogg - 78352
class npc_foundry_battle_medic_rogg : public CreatureScript
{
    public:
        npc_foundry_battle_medic_rogg() : CreatureScript("npc_foundry_battle_medic_rogg") { }

        enum eSpells
        {
            /// Earthen Barrier
            SpellEarthenBarrier         = 158708,
            /// Protective Earth
            SpellProtectiveEarthCast    = 158707,
            /// Chain Lightning
            SpellChainLightning         = 158710
        };

        enum eEvents
        {
            EventEarthenBarrier = 1,
            EventProtectiveEarth,
            EventChainLightning
        };

        struct npc_foundry_battle_medic_roggAI : public ScriptedAI
        {
            npc_foundry_battle_medic_roggAI(Creature* p_Creature) : ScriptedAI(p_Creature) { }

            EventMap m_Events;

            void Reset() override
            {
                me->SetReactState(ReactStates::REACT_PASSIVE);

                me->SetSheath(SheathState::SHEATH_STATE_MELEE);

                m_Events.Reset();
            }

            void EnterCombat(Unit* /*p_Attacker*/) override
            {
                m_Events.ScheduleEvent(eEvents::EventEarthenBarrier, 5 * TimeConstants::IN_MILLISECONDS);
                m_Events.ScheduleEvent(eEvents::EventProtectiveEarth, 10 * TimeConstants::IN_MILLISECONDS);
                m_Events.ScheduleEvent(eEvents::EventChainLightning, 3 * TimeConstants::IN_MILLISECONDS);
            }

            void DamageTaken(Unit* /*p_Attacker*/, uint32& /*p_Damage*/, SpellInfo const* /*p_SpellInfo*/) override
            {
                me->SetReactState(ReactStates::REACT_AGGRESSIVE);
            }

            void UpdateAI(uint32 const p_Diff)
            {
                if (!UpdateVictim())
                    return;

                m_Events.Update(p_Diff);

                if (me->HasUnitState(UnitState::UNIT_STATE_CASTING))
                    return;

                switch (m_Events.ExecuteEvent())
                {
                    case eEvents::EventEarthenBarrier:
                    {
                        if (Unit* l_Ally = me->SelectNearbyAlly(me))
                            me->CastSpell(l_Ally, eSpells::SpellEarthenBarrier, false);
                        m_Events.ScheduleEvent(eEvents::EventEarthenBarrier, 11 * TimeConstants::IN_MILLISECONDS);
                        break;
                    }
                    case eEvents::EventProtectiveEarth:
                    {
                        float l_Range = frand(0.0f, 10.0f);
                        float l_Angle = frand(0.0f, 2.0f * M_PI);

                        Position l_Pos;
                        l_Pos.m_positionX = me->m_positionX + l_Range * cos(l_Angle);
                        l_Pos.m_positionY = me->m_positionY + l_Range * sin(l_Angle);
                        l_Pos.m_positionZ = me->m_positionZ;

                        me->CastSpell(l_Pos, eSpells::SpellProtectiveEarthCast, false);
                        m_Events.ScheduleEvent(eEvents::EventProtectiveEarth, 15 * TimeConstants::IN_MILLISECONDS);
                        break;
                    }
                    case eEvents::EventChainLightning:
                    {
                        if (Unit* l_Target = SelectTarget(SelectAggroTarget::SELECT_TARGET_RANDOM, 0, 0.0f, true, eIronMaidensSpells::OnABoatPeriodic))
                            me->CastSpell(l_Target, eSpells::SpellChainLightning, false);
                        m_Events.ScheduleEvent(eEvents::EventChainLightning, 10 * TimeConstants::IN_MILLISECONDS);
                        break;
                    }
                    default:
                        break;
                }

                DoMeleeAttackIfReady();
            }
        };

        CreatureAI* GetAI(Creature* p_Creature) const override
        {
            return new npc_foundry_battle_medic_roggAI(p_Creature);
        }
};

/// Gorak <Deckhand of Enforcer Sorka> - 78343
class npc_foundry_gorak : public CreatureScript
{
    public:
        npc_foundry_gorak() : CreatureScript("npc_foundry_gorak") { }

        enum eSpell
        {
            DeadlyThrow = 158692
        };

        enum eEvent
        {
            EventDeadlyThrow = 1
        };

        struct npc_foundry_gorakAI : public ScriptedAI
        {
            npc_foundry_gorakAI(Creature* p_Creature) : ScriptedAI(p_Creature)
            {
                m_Instance = p_Creature->GetInstanceScript();
            }

            InstanceScript* m_Instance;

            EventMap m_Events;

            void Reset() override
            {
                me->SetReactState(ReactStates::REACT_PASSIVE);

                m_Events.Reset();
            }

            void EnterCombat(Unit* p_Attacker) override
            {
                m_Events.ScheduleEvent(eEvent::EventDeadlyThrow, 6 * TimeConstants::IN_MILLISECONDS);
            }

            void JustDied(Unit* p_Killer) override
            {
                if (m_Instance == nullptr)
                    return;

                if (GameObject* l_AmmoLoader = GameObject::GetGameObject(*me, m_Instance->GetData64(eFoundryGameObjects::AmmoLoader)))
                    l_AmmoLoader->RemoveFlag(EGameObjectFields::GAMEOBJECT_FIELD_FLAGS, GameObjectFlags::GO_FLAG_NOT_SELECTABLE);
            }

            void UpdateAI(uint32 const p_Diff) override
            {
                if (!UpdateVictim())
                    return;

                m_Events.Update(p_Diff);

                if (me->HasUnitState(UnitState::UNIT_STATE_CASTING))
                    return;

                switch (m_Events.ExecuteEvent())
                {
                    case eEvent::EventDeadlyThrow:
                    {
                        if (Unit* l_Target = SelectTarget(SelectAggroTarget::SELECT_TARGET_RANDOM, 0, 0.0f, true, eIronMaidensSpells::OnABoatPeriodic))
                            me->CastSpell(l_Target, eSpell::DeadlyThrow, false);
                        m_Events.ScheduleEvent(eEvent::EventDeadlyThrow, 14 * TimeConstants::IN_MILLISECONDS);
                        break;
                    }
                    default:
                        break;
                }

                DoMeleeAttackIfReady();
            }
        };

        CreatureAI* GetAI(Creature* p_Creature) const override
        {
            return new npc_foundry_gorakAI(p_Creature);
        }
};

/// Iron Eviscerator - 78347
class npc_foundry_iron_eviscerator : public CreatureScript
{
    public:
        npc_foundry_iron_eviscerator() : CreatureScript("npc_foundry_iron_eviscerator") { }

        enum eSpell
        {
        };

        struct npc_foundry_iron_evisceratorAI : public ScriptedAI
        {
            npc_foundry_iron_evisceratorAI(Creature* p_Creature) : ScriptedAI(p_Creature)
            {
                m_Instance = p_Creature->GetInstanceScript();
            }

            InstanceScript* m_Instance;

            void Reset() override
            {
                me->SetReactState(ReactStates::REACT_PASSIVE);
            }

            void UpdateAI(uint32 const p_Diff) override
            {
                if (!UpdateVictim())
                    return;

                if (me->HasUnitState(UnitState::UNIT_STATE_CASTING))
                    return;
            }
        };

        CreatureAI* GetAI(Creature* p_Creature) const override
        {
            return new npc_foundry_iron_evisceratorAI(p_Creature);
        }
};

/// Cluster Bomb Alpha - 78177
class npc_foundry_cluster_bomb_alpha : public CreatureScript
{
    public:
        npc_foundry_cluster_bomb_alpha() : CreatureScript("npc_foundry_cluster_bomb_alpha") { }

        enum eSpell
        {
            DetonationSequence = 157867
        };

        struct npc_foundry_cluster_bomb_alphaAI : public ScriptedAI
        {
            npc_foundry_cluster_bomb_alphaAI(Creature* p_Creature) : ScriptedAI(p_Creature)
            {
                m_Instance = p_Creature->GetInstanceScript();
            }

            InstanceScript* m_Instance;

            void Reset() override
            {
                me->SetReactState(ReactStates::REACT_PASSIVE);
            }

            void UpdateAI(uint32 const p_Diff) override
            {
                if (!UpdateVictim())
                    return;

                if (me->HasUnitState(UnitState::UNIT_STATE_CASTING))
                    return;
            }
        };

        CreatureAI* GetAI(Creature* p_Creature) const override
        {
            return new npc_foundry_cluster_bomb_alphaAI(p_Creature);
        }
};

/// Blood Ritual - 158078
class spell_foundry_blood_ritual : public SpellScriptLoader
{
    public:
        spell_foundry_blood_ritual() : SpellScriptLoader("spell_foundry_blood_ritual") { }

        enum eSpell
        {
            CrystallizedBlood = 158080
        };

        class spell_foundry_blood_ritual_SpellScript : public SpellScript
        {
            PrepareSpellScript(spell_foundry_blood_ritual_SpellScript)

            void CorrectTargets(std::list<WorldObject*>& p_Targets)
            {
                if (p_Targets.empty())
                    return;

                Unit* l_Caster = GetCaster();
                p_Targets.sort(JadeCore::ObjectDistanceOrderPred(l_Caster));

                if (Unit* l_Target = (*p_Targets.begin())->ToUnit())
                    l_Caster->CastSpell(l_Target, eSpell::CrystallizedBlood, true);
            }

            void Register() override
            {
                OnObjectAreaTargetSelect += SpellObjectAreaTargetSelectFn(spell_foundry_blood_ritual_SpellScript::CorrectTargets, EFFECT_1, TARGET_UNIT_CONE_ENEMY_104);
            }
        };

        SpellScript* GetSpellScript() const override
        {
            return new spell_foundry_blood_ritual_SpellScript();
        }
};

/// Penetrating Shot (aura) - 164271
class spell_foundry_penetrating_shot : public SpellScriptLoader
{
    public:
        spell_foundry_penetrating_shot() : SpellScriptLoader("spell_foundry_penetrating_shot") { }

        class spell_foundry_penetrating_shot_AuraScript : public AuraScript
        {
            PrepareAuraScript(spell_foundry_penetrating_shot_AuraScript)

            enum eSpell
            {
                PenetratingShotDamage = 164279
            };

            void AfterRemove(AuraEffect const* /*p_AurEff*/, AuraEffectHandleModes /*p_Mode*/)
            {
                Unit* l_Caster = GetCaster();
                Unit* l_Target = GetTarget();
                if (l_Caster == nullptr || l_Target == nullptr)
                    return;

                l_Caster->SetFacingTo(l_Caster->GetAngle(l_Target));
                l_Caster->CastSpell(*l_Target, eSpell::PenetratingShotDamage, true);
            }

            void Register() override
            {
                AfterEffectRemove += AuraEffectRemoveFn(spell_foundry_penetrating_shot_AuraScript::AfterRemove, EFFECT_0, SPELL_AURA_DUMMY, AURA_EFFECT_HANDLE_REAL);
            }
        };

        AuraScript* GetAuraScript() const override
        {
            return new spell_foundry_penetrating_shot_AuraScript();
        }
};

/// Penetrating Shot (damage) - 164279
class spell_foundry_penetrating_shot_damage : public SpellScriptLoader
{
    public:
        spell_foundry_penetrating_shot_damage() : SpellScriptLoader("spell_foundry_penetrating_shot_damage") { }

        class spell_foundry_penetrating_shot_damage_SpellScript : public SpellScript
        {
            PrepareSpellScript(spell_foundry_penetrating_shot_damage_SpellScript)

            uint32 m_TargetCount;

            bool Load() override
            {
                m_TargetCount = 0;
                return true;
            }

            void CorrectTargets(std::list<WorldObject*>& p_Targets)
            {
                m_TargetCount = uint32(p_Targets.size());
            }

            void HandleDamage(SpellEffIndex p_EffIndex)
            {
                if (!m_TargetCount)
                    return;

                SetHitDamage(GetHitDamage() / m_TargetCount);
            }

            void Register() override
            {
                OnObjectAreaTargetSelect += SpellObjectAreaTargetSelectFn(spell_foundry_penetrating_shot_damage_SpellScript::CorrectTargets, EFFECT_0, TARGET_ENNEMIES_IN_CYLINDER);
                OnEffectHitTarget += SpellEffectFn(spell_foundry_penetrating_shot_damage_SpellScript::HandleDamage, EFFECT_0, SPELL_EFFECT_SCHOOL_DAMAGE);
            }
        };

        SpellScript* GetSpellScript() const override
        {
            return new spell_foundry_penetrating_shot_damage_SpellScript();
        }
};

/// Convulsive Shadows - 156214
class spell_foundry_convulsive_shadows : public SpellScriptLoader
{
    public:
        spell_foundry_convulsive_shadows() : SpellScriptLoader("spell_foundry_convulsive_shadows") { }

        enum eSpell
        {
            ShadowExplosion = 156280
        };

        class spell_foundry_convulsive_shadows_SpellScript : public SpellScript
        {
            PrepareSpellScript(spell_foundry_convulsive_shadows_SpellScript)

            void HandleLingeringShadow(SpellEffIndex p_EffIndex)
            {
                Unit* l_Caster = GetCaster();
                if (l_Caster == nullptr)
                    return;

                if (!l_Caster->GetMap()->IsMythic())
                    PreventHitEffect(p_EffIndex);
            }

            void Register() override
            {
                OnEffectLaunch += SpellEffectFn(spell_foundry_convulsive_shadows_SpellScript::HandleLingeringShadow, SpellEffIndex::EFFECT_0, SpellEffects::SPELL_EFFECT_APPLY_AURA);
            }
        };

        SpellScript* GetSpellScript() const override
        {
            return new spell_foundry_convulsive_shadows_SpellScript();
        }

        class spell_foundry_convulsive_shadows_AuraScript : public AuraScript
        {
            PrepareAuraScript(spell_foundry_convulsive_shadows_AuraScript)

            void AfterTick(AuraEffect const* p_AurEff)
            {
                p_AurEff->GetBase()->DropStack();
            }

            void HandleDispel(DispelInfo* p_DispelInfo)
            {
                Unit* l_Target = GetUnitOwner();
                if (l_Target == nullptr)
                    return;

                if (InstanceScript* l_InstanceScript = l_Target->GetInstanceScript())
                {
                    if (Creature* l_Sorka = Creature::GetCreature(*l_Target, l_InstanceScript->GetData64(eFoundryCreatures::BossEnforcerSorka)))
                    {
                        int32 l_Damage = 40000 * p_DispelInfo->GetRemovedCharges();

                        l_Sorka->CastCustomSpell(l_Target, eSpell::ShadowExplosion, &l_Damage, nullptr, nullptr, true);
                    }
                }
            }

            void Register() override
            {
                AfterEffectPeriodic += AuraEffectPeriodicFn(spell_foundry_convulsive_shadows_AuraScript::AfterTick, EFFECT_1, SPELL_AURA_PERIODIC_DAMAGE);
                OnDispel += AuraDispelFn(spell_foundry_convulsive_shadows_AuraScript::HandleDispel);
            }
        };

        AuraScript* GetAuraScript() const override
        {
            return new spell_foundry_convulsive_shadows_AuraScript();
        }
};

/// Dark Hunt - 158315
class spell_foundry_dark_hunt : public SpellScriptLoader
{
    public:
        spell_foundry_dark_hunt() : SpellScriptLoader("spell_foundry_dark_hunt") { }

        class spell_foundry_dark_hunt_AuraScript : public AuraScript
        {
            PrepareAuraScript(spell_foundry_dark_hunt_AuraScript)

            enum eSpell
            {
                DarkHuntExecution = 158321
            };

            void AfterRemove(AuraEffect const* /*p_AurEff*/, AuraEffectHandleModes /*p_Mode*/)
            {
                Unit* l_Caster = GetCaster();
                Unit* l_Target = GetTarget();
                if (l_Caster == nullptr || l_Target == nullptr)
                    return;

                l_Caster->CastSpell(l_Target, eSpell::DarkHuntExecution, true);
            }

            void Register() override
            {
                AfterEffectRemove += AuraEffectRemoveFn(spell_foundry_dark_hunt_AuraScript::AfterRemove, EFFECT_0, SPELL_AURA_DUMMY, AURA_EFFECT_HANDLE_REAL);
            }
        };

        AuraScript* GetAuraScript() const override
        {
            return new spell_foundry_dark_hunt_AuraScript();
        }
};

/// Bloodsoaked Heartseeker - 158009
class spell_foundry_bloodsoaked_heartseeker_damage : public SpellScriptLoader
{
    public:
        spell_foundry_bloodsoaked_heartseeker_damage() : SpellScriptLoader("spell_foundry_bloodsoaked_heartseeker_damage") { }

        class spell_foundry_bloodsoaked_heartseeker_damage_SpellScript : public SpellScript
        {
            PrepareSpellScript(spell_foundry_bloodsoaked_heartseeker_damage_SpellScript)

            void HandleDamage(SpellEffIndex p_EffIndex)
            {
                Unit* l_Caster = GetCaster();
                Unit* l_Target = GetHitUnit();

                if (l_Caster == nullptr || l_Target == nullptr)
                    return;

                float l_ReducedDamage   = 1.6f;
                float l_Damage          = float(GetSpell()->GetDamage());
                float l_Distance        = l_Caster->GetDistance(l_Target);
                float l_NewDamages      = std::max(1.0f, l_Damage - (l_Damage * l_Distance * l_ReducedDamage / 100.0f));

                GetSpell()->SetDamage(uint32(l_NewDamages));
            }

            void Register() override
            {
                OnEffectHitTarget += SpellEffectFn(spell_foundry_bloodsoaked_heartseeker_damage_SpellScript::HandleDamage, EFFECT_0, SPELL_EFFECT_SCHOOL_DAMAGE);
            }
        };

        SpellScript* GetSpellScript() const override
        {
            return new spell_foundry_bloodsoaked_heartseeker_damage_SpellScript();
        }
};

/// Sanguine Strikes - 156601
class spell_foundry_sanguine_strikes_proc : public SpellScriptLoader
{
    public:
        spell_foundry_sanguine_strikes_proc() : SpellScriptLoader("spell_foundry_sanguine_strikes_proc") { }

        enum eSpell
        {
            SanguineStrikesProc = 156610
        };

        class spell_foundry_sanguine_strikes_proc_AuraScript : public AuraScript
        {
            PrepareAuraScript(spell_foundry_sanguine_strikes_proc_AuraScript)

            void OnProc(AuraEffect const* /*p_AurEff*/, ProcEventInfo& p_EventInfo)
            {
                PreventDefaultAction();

                /// Can proc only for melee attacks
                if (p_EventInfo.GetDamageInfo()->GetSpellInfo() != nullptr)
                    return;

                if (Creature* l_Marak = GetTarget()->ToCreature())
                {
                    int32 l_Damage = p_EventInfo.GetDamageInfo()->GetDamage();

                    l_Marak->CastCustomSpell(l_Marak, eSpell::SanguineStrikesProc, &l_Damage, nullptr, nullptr, true);
                }
            }

            void Register() override
            {
                OnEffectProc += AuraEffectProcFn(spell_foundry_sanguine_strikes_proc_AuraScript::OnProc, EFFECT_0, SPELL_AURA_DUMMY);
            }
        };

        AuraScript* GetAuraScript() const override
        {
            return new spell_foundry_sanguine_strikes_proc_AuraScript();
        }
};

/// Sabotage - 158148
class spell_foundry_sabotage : public SpellScriptLoader
{
    public:
        spell_foundry_sabotage() : SpellScriptLoader("spell_foundry_sabotage") { }

        enum eSpell
        {
            EndShipPhase = 158724
        };

        class spell_foundry_sabotage_SpellScript : public SpellScript
        {
            PrepareSpellScript(spell_foundry_sabotage_SpellScript)

            void HandleOpenLock(SpellEffIndex p_EffIndex)
            {
                Unit* l_Caster = GetCaster();
                if (l_Caster == nullptr)
                    return;

                l_Caster->CastSpell(l_Caster, eSpell::EndShipPhase, true);
            }

            void Register() override
            {
                OnEffectHitTarget += SpellEffectFn(spell_foundry_sabotage_SpellScript::HandleOpenLock, EFFECT_0, SPELL_EFFECT_OPEN_LOCK);
            }
        };

        SpellScript* GetSpellScript() const override
        {
            return new spell_foundry_sabotage_SpellScript();
        }
};

/// Dominator Blast - 158602
class areatrigger_foundry_dominator_blast : public AreaTriggerEntityScript
{
    public:
        areatrigger_foundry_dominator_blast() : AreaTriggerEntityScript("areatrigger_foundry_dominator_blast"), m_DamageCooldown(0) { }

        enum eSpell
        {
            DominatorBlastDoT = 158601
        };

        int32 m_DamageCooldown;

        void OnSetCreatePosition(AreaTrigger* p_AreaTrigger, Unit* p_Caster, Position& /*p_SourcePosition*/, Position& p_DestinationPosition, std::list<Position>& /*p_PathToLinearDestination*/) override
        {
            p_AreaTrigger->SetTimeToTarget(15 * TimeConstants::IN_MILLISECONDS);

            p_DestinationPosition.m_positionX = p_Caster->m_positionX + 80.0f * cos(p_Caster->m_orientation);
            p_DestinationPosition.m_positionY = p_Caster->m_positionY + 80.0f * sin(p_Caster->m_orientation);
        }

        void OnUpdate(AreaTrigger* p_AreaTrigger, uint32 p_Time) override
        {
            if (Unit* l_Caster = p_AreaTrigger->GetCaster())
            {
                if (m_DamageCooldown > 0)
                    m_DamageCooldown -= p_Time;
                else
                {
                    std::list<Player*> l_TargetList;
                    float l_Radius = 1.5f;

                    JadeCore::AnyPlayerInObjectRangeCheck l_Check(p_AreaTrigger, l_Radius);
                    JadeCore::PlayerListSearcher<JadeCore::AnyPlayerInObjectRangeCheck> l_Searcher(p_AreaTrigger, l_TargetList, l_Check);
                    p_AreaTrigger->VisitNearbyObject(l_Radius, l_Searcher);

                    for (Player* l_Iter : l_TargetList)
                        l_Iter->CastSpell(l_Iter, eSpell::DominatorBlastDoT, true);

                    m_DamageCooldown = 500;
                }
            }
        }

        AreaTriggerEntityScript* GetAI() const override
        {
            return new areatrigger_foundry_dominator_blast();
        }
};

/// Protective Earth - 158707
class areatrigger_foundry_protective_earth : public AreaTriggerEntityScript
{
    public:
        areatrigger_foundry_protective_earth() : AreaTriggerEntityScript("areatrigger_foundry_protective_earth") { }

        enum eSpell
        {
            SpellEarthenBarrier = 158708
        };

        void OnUpdate(AreaTrigger* p_AreaTrigger, uint32 p_Time) override
        {
            if (Unit* l_Caster = p_AreaTrigger->GetCaster())
            {
                std::list<Unit*> l_Allies;
                float l_Radius = 1.0f;

                JadeCore::AnyFriendlyUnitInObjectRangeCheck l_Check(p_AreaTrigger, l_Caster, l_Radius);
                JadeCore::UnitListSearcher<JadeCore::AnyFriendlyUnitInObjectRangeCheck> l_Searcher(p_AreaTrigger, l_Allies, l_Check);
                p_AreaTrigger->VisitNearbyObject(l_Radius, l_Searcher);

                for (Unit* l_Iter : l_Allies)
                {
                    l_Caster->CastSpell(l_Iter, eSpell::SpellEarthenBarrier, true);
                    p_AreaTrigger->Remove(0);
                    break;
                }
            }
        }

        AreaTriggerEntityScript* GetAI() const override
        {
            return new areatrigger_foundry_protective_earth();
        }
};

/// Iron Maidens Boat - 9945
class areatrigger_at_foundry_iron_maidens_boat : public AreaTriggerScript
{
    public:
        areatrigger_at_foundry_iron_maidens_boat() : AreaTriggerScript("areatrigger_at_foundry_iron_maidens_boat") { }

        void OnEnter(Player* p_Player, AreaTriggerEntry const* /*p_AreaTrigger*/) override
        {
            p_Player->CastSpell(p_Player, eIronMaidensSpells::OnABoatPeriodic, true);
        }

        void OnExit(Player* p_Player, AreaTriggerEntry const* /*p_AreaTrigger*/) override
        {
            p_Player->RemoveAura(eIronMaidensSpells::OnABoatPeriodic);
        }
};

#ifndef __clang_analyzer__
void AddSC_boss_iron_maidens()
{
    /// Bosses
    new boss_admiral_garan();
    new boss_enforcer_sorka();
    new boss_marak_the_blooded();

    /// Creatures
    new npc_foundry_loading_chain();
    new npc_foundry_ukurogg();
    new npc_foundry_zipline_stalker();
    new npc_foundry_rapid_fire_stalker();
    new npc_foundry_dominator_turret();
    new npc_foundry_iron_cannon();
    new npc_foundry_uktar();
    new npc_foundry_battle_medic_rogg();
    new npc_foundry_gorak();
    new npc_foundry_iron_eviscerator();
    new npc_foundry_cluster_bomb_alpha();

    /// Spells
    new spell_foundry_blood_ritual();
    new spell_foundry_penetrating_shot();
    new spell_foundry_penetrating_shot_damage();
    new spell_foundry_convulsive_shadows();
    new spell_foundry_dark_hunt();
    new spell_foundry_bloodsoaked_heartseeker_damage();
    new spell_foundry_sanguine_strikes_proc();
    new spell_foundry_sabotage();

    /// AreaTriggers (spell)
    new areatrigger_foundry_dominator_blast();
    new areatrigger_foundry_protective_earth();

    /// AreaTrigger (world)
    new areatrigger_at_foundry_iron_maidens_boat();
}
#endif
