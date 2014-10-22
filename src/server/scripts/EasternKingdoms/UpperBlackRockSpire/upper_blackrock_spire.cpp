/*
* Copyright (C) 2012-2014 JadeCore <http://www.pandashan.com>
* Copyright (C) 2008-2012 TrinityCore <http://www.trinitycore.org/>
* Copyright (C) 2006-2009 ScriptDev2 <https://scriptdev2.svn.sourceforge.net/>
*
* This program is free software; you can redistribute it and/or modify it
* under the terms of the GNU General Public License as published by the
* Free Software Foundation; either version 2 of the License, or (at your
* option) any later version.
*
* This program is distributed in the hope that it will be useful, but WITHOUT
* ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
* FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for
* more details.
*
* You should have received a copy of the GNU General Public License along
* with this program. If not, see <http://www.gnu.org/licenses/>.
*/

#include "ScriptPCH.h"
#include "upper_blackrock_spire.h"
#include "Language.h"

enum eSpells
{
    // Black Iron Grunt
    SPELL_DEVASTATE                 = 153832,
    SPELL_RALLYING_BANNER           = 153792,
    // Black Iron Leadbelcher
    SPELL_INCENDIARY_SHELL          = 153981,
    SPELL_RIFLE_SHOT                = 153974,
    // Sentry Cannon
    SPELL_CANNON_SHOT               = 154178,
    SPELL_SAFETY_PROTOCOLS          = 154894,
    // Ragemaw Worg
    SPELL_BLACKROCK_RABIES          = 154017,
    SPELL_FRANTIC_MAULING           = 154039,
    // Black Iron Warcaster
    SPELL_BOLT_OF_STEEL             = 153642,
    SPELL_SHRAPNEL_STORM            = 153942,
    SPELL_SHRAPNEL_STORM_MISSILE    = 153941
};

enum eEvents
{
    // Black Iron Grunt
    EVENT_DEVASTATE = 1,
    EVENT_RALLYING_BANNER,
    // Black Iron Leadbelcher
    EVENT_INCENDIARY_SHELL,
    EVENT_RIFLE_SHOT,
    // Sentry Cannon
    EVENT_CANNON_SHOT,
    EVENT_SAFETY_PROTOCOLS,
    // Ragemaw Worg
    EVENT_FRANTIC_MAULING,
    // Black Iron Warcaster
    EVENT_BOLT_OF_STEEL,
    EVENT_SHRAPNEL_STORM
};

enum eActions
{
};

enum eTalks
{
    TALK_IRON_GRUNT_AGGRO,          // Iron Horde, we have unwanted visitors! -- Help! We have intruders!
    TALK_IRON_GRUNT_NEAR_DEATH      // We need backup!
};

// Black Iron Grunt - 76179
class mob_black_iron_grunt : public CreatureScript
{
    public:
        mob_black_iron_grunt() : CreatureScript("mob_black_iron_grunt") { }

        struct mob_black_iron_gruntAI : public ScriptedAI
        {
            mob_black_iron_gruntAI(Creature* p_Creature) : ScriptedAI(p_Creature) { }

            EventMap m_Events;
            bool m_DeathEventDone;

            void Reset()
            {
                me->ReenableEvadeMode();

                m_Events.Reset();

                m_DeathEventDone = false;
            }

            void EnterCombat(Unit* p_Attacker)
            {
                Talk(TALK_IRON_GRUNT_AGGRO);

                m_Events.ScheduleEvent(EVENT_DEVASTATE, 3000);
                m_Events.ScheduleEvent(EVENT_RALLYING_BANNER, 10000);
            }

            void DamageTaken(Unit* p_Attacker, uint32& p_Damage)
            {
                if (me->HealthBelowPctDamaged(20, p_Damage) && !m_DeathEventDone)
                {
                    m_DeathEventDone = true;
                    Talk(TALK_IRON_GRUNT_NEAR_DEATH);
                }
            }

            void UpdateAI(const uint32 p_Diff)
            {
                if (!UpdateVictim())
                    return;

                if (me->HasUnitState(UNIT_STATE_CASTING))
                    return;

                m_Events.Update(p_Diff);

                switch (m_Events.ExecuteEvent())
                {
                    case EVENT_DEVASTATE:
                        if (Unit* l_Target = SelectTarget(SELECT_TARGET_TOPAGGRO))
                            me->CastSpell(l_Target, SPELL_DEVASTATE, false);
                        m_Events.ScheduleEvent(EVENT_DEVASTATE, 8000);
                        break;
                    case EVENT_RALLYING_BANNER:
                        me->MonsterTextEmote(LANG_RALLYING_BANNER_SUMMONED, 0, true);
                        me->CastSpell(me, SPELL_RALLYING_BANNER, false);
                        /*
                        @TODO:
                            - Script for Rallying Banner
                            - 153799 : Create Areatrigger, which grows up
                            - And aggro nearby allies
                        */
                        m_Events.ScheduleEvent(EVENT_RALLYING_BANNER, 15000);
                        break;
                    default:
                        break;
                }

                DoMeleeAttackIfReady();
            }
        };

        CreatureAI* GetAI(Creature* p_Creature) const
        {
            return new mob_black_iron_gruntAI(p_Creature);
        }
};

// Black Iron Leadbelcher - 76157
class mob_black_iron_leadbelcher : public CreatureScript
{
    public:
        mob_black_iron_leadbelcher() : CreatureScript("mob_black_iron_leadbelcher") { }

        struct mob_black_iron_leadbelcherAI : public ScriptedAI
        {
            mob_black_iron_leadbelcherAI(Creature* p_Creature) : ScriptedAI(p_Creature) { }

            EventMap m_Events;

            void Reset()
            {
                me->ReenableEvadeMode();

                m_Events.Reset();
            }

            void EnterCombat(Unit* p_Attacker)
            {
                m_Events.ScheduleEvent(EVENT_INCENDIARY_SHELL, 4000);
                m_Events.ScheduleEvent(EVENT_RIFLE_SHOT, 8000);
            }

            void UpdateAI(const uint32 p_Diff)
            {
                if (!UpdateVictim())
                    return;

                if (me->HasUnitState(UNIT_STATE_CASTING))
                    return;

                m_Events.Update(p_Diff);

                switch (m_Events.ExecuteEvent())
                {
                    case EVENT_INCENDIARY_SHELL:
                        if (Unit* l_Target = SelectTarget(SELECT_TARGET_RANDOM))
                            me->CastSpell(l_Target, SPELL_INCENDIARY_SHELL, false);
                        m_Events.ScheduleEvent(EVENT_INCENDIARY_SHELL, 10000);
                        break;
                    case EVENT_RIFLE_SHOT:
                        if (Unit* l_Target = SelectTarget(SELECT_TARGET_RANDOM))
                            me->CastSpell(l_Target, SPELL_RIFLE_SHOT, false);
                        m_Events.ScheduleEvent(EVENT_RIFLE_SHOT, 10000);
                        break;
                    default:
                        break;
                }

                DoMeleeAttackIfReady();
            }
        };

        CreatureAI* GetAI(Creature* p_Creature) const
        {
            return new mob_black_iron_leadbelcherAI(p_Creature);
        }
};

// Sentry Cannon - 76314
class mob_sentry_cannon : public CreatureScript
{
    public:
        mob_sentry_cannon() : CreatureScript("mob_sentry_cannon") { }

        struct mob_sentry_cannonAI : public ScriptedAI
        {
            mob_sentry_cannonAI(Creature* p_Creature) : ScriptedAI(p_Creature) { }

            EventMap m_Events;

            void Reset()
            {
                me->ReenableEvadeMode();

                m_Events.Reset();

                me->AddUnitState(UNIT_STATE_ROOT);
            }

            void EnterCombat(Unit* p_Attacker)
            {
                m_Events.ScheduleEvent(EVENT_CANNON_SHOT, 5000);
            }

            void DamageTaken(Unit* p_Attacker, uint32& p_Damage)
            {
                if (me->HealthBelowPctDamaged(50, p_Damage) && !me->HasAura(SPELL_SAFETY_PROTOCOLS))
                    me->CastSpell(me, SPELL_SAFETY_PROTOCOLS, true);
            }

            void UpdateAI(const uint32 p_Diff)
            {
                if (!UpdateVictim())
                    return;

                if (me->HasUnitState(UNIT_STATE_CASTING))
                    return;

                m_Events.Update(p_Diff);

                switch (m_Events.ExecuteEvent())
                {
                    case EVENT_CANNON_SHOT:
                        if (Unit* l_Target = SelectTarget(SELECT_TARGET_RANDOM))
                            me->CastSpell(l_Target, SPELL_CANNON_SHOT, false);
                        m_Events.ScheduleEvent(EVENT_CANNON_SHOT, 15000);
                        break;
                    default:
                        break;
                }

                DoMeleeAttackIfReady();
            }
        };

        CreatureAI* GetAI(Creature* p_Creature) const
        {
            return new mob_sentry_cannonAI(p_Creature);
        }
};

// Ragemaw Worg - 76181
class mob_ragemaw_worg : public CreatureScript
{
    public:
        mob_ragemaw_worg() : CreatureScript("mob_ragemaw_worg") { }

        struct mob_ragemaw_worgAI : public ScriptedAI
        {
            mob_ragemaw_worgAI(Creature* p_Creature) : ScriptedAI(p_Creature) { }

            EventMap m_Events;

            void Reset()
            {
                me->ReenableEvadeMode();

                m_Events.Reset();
            }

            void EnterCombat(Unit* p_Attacker)
            {
                m_Events.ScheduleEvent(EVENT_FRANTIC_MAULING, 10000);
            }

            void DamageTaken(Unit* p_Attacker, uint32& p_Damage)
            {
                if (me->HealthBelowPctDamaged(50, p_Damage) && !me->HasAura(SPELL_BLACKROCK_RABIES))
                    me->CastSpell(me, SPELL_BLACKROCK_RABIES, true);
            }

            void UpdateAI(const uint32 p_Diff)
            {
                if (!UpdateVictim())
                    return;

                if (me->HasUnitState(UNIT_STATE_CASTING))
                    return;

                m_Events.Update(p_Diff);

                switch (m_Events.ExecuteEvent())
                {
                    case EVENT_FRANTIC_MAULING:
                        me->CastSpell(me, SPELL_FRANTIC_MAULING, false);
                        m_Events.ScheduleEvent(EVENT_FRANTIC_MAULING, 15000);
                        break;
                    default:
                        break;
                }

                DoMeleeAttackIfReady();
            }
        };

        CreatureAI* GetAI(Creature* p_Creature) const
        {
            return new mob_ragemaw_worgAI(p_Creature);
        }
};

// Black Iron Warcaster - 76151
class mob_black_iron_warcaster : public CreatureScript
{
    public:
        mob_black_iron_warcaster() : CreatureScript("mob_black_iron_warcaster") { }

        struct mob_black_iron_warcasterAI : public ScriptedAI
        {
            mob_black_iron_warcasterAI(Creature* p_Creature) : ScriptedAI(p_Creature) { }

            EventMap m_Events;

            void Reset()
            {
                me->ReenableEvadeMode();

                m_Events.Reset();
            }

            void EnterCombat(Unit* p_Attacker)
            {
                m_Events.ScheduleEvent(EVENT_BOLT_OF_STEEL, 8000);
                m_Events.ScheduleEvent(EVENT_SHRAPNEL_STORM, 15000);
            }

            void UpdateAI(const uint32 p_Diff)
            {
                if (!UpdateVictim())
                    return;

                if (me->HasUnitState(UNIT_STATE_CASTING))
                    return;

                m_Events.Update(p_Diff);

                switch (m_Events.ExecuteEvent())
                {
                    case EVENT_BOLT_OF_STEEL:
                        if (Unit* l_Target = SelectTarget(SELECT_TARGET_RANDOM))
                            me->CastSpell(l_Target, SPELL_BOLT_OF_STEEL, false);
                        m_Events.ScheduleEvent(EVENT_BOLT_OF_STEEL, 10000);
                        break;
                    case EVENT_SHRAPNEL_STORM:
                        m_Events.ScheduleEvent(EVENT_SHRAPNEL_STORM, 15000);
                        break;
                    default:
                        break;
                }

                DoMeleeAttackIfReady();
            }
        };

        CreatureAI* GetAI(Creature* p_Creature) const
        {
            return new mob_black_iron_warcasterAI(p_Creature);
        }
};

// Shrapnel Storm - 153942
class spell_shrapnel_storm : public SpellScriptLoader
{
    public:
        spell_shrapnel_storm() : SpellScriptLoader("spell_shrapnel_storm") { }

        class spell_shrapnel_storm_SpellScript : public SpellScript
        {
            PrepareSpellScript(spell_shrapnel_storm_SpellScript);

            void HandleDummy(SpellEffIndex /*p_EffIndex*/)
            {
                if (Position const* l_Pos = GetExplTargetDest())
                {
                    if (Unit* l_Caster = GetCaster())
                        l_Caster->CastSpell(l_Pos->GetPositionX(), l_Pos->GetPositionY(), l_Pos->GetPositionZ(), SPELL_SHRAPNEL_STORM_MISSILE, true);
                }
            }

            void Register()
            {
                OnEffectHit += SpellEffectFn(spell_shrapnel_storm_SpellScript::HandleDummy, EFFECT_0, SPELL_EFFECT_DUMMY);
            }
        };

        SpellScript* GetSpellScript() const
        {
            return new spell_shrapnel_storm_SpellScript();
        }
};

void AddSC_upper_blackrock_spire()
{
    new mob_black_iron_grunt();
    new mob_black_iron_leadbelcher();
    new mob_sentry_cannon();
    new mob_ragemaw_worg();
    new mob_black_iron_warcaster();
    new spell_shrapnel_storm();
}