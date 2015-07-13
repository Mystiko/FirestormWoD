/*
* Copyright (C) 2008-2012 TrinityCore <http://www.trinitycore.org/>
* Copyright (C) 2005-2009 MaNGOS <http://getmangos.com/>
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

#ifndef _PLAYER_DUMP_H
#define _PLAYER_DUMP_H

#include <string>
#include <map>
#include <set>

enum DumpTableType
{
    DTT_CHARACTER,      //                                  // characters

    DTT_CHAR_TABLE,     //                                  // character_achievement, character_achievement_progress,
    // character_action, character_aura, character_homebind,
    // character_queststatus, character_queststatus_rewarded, character_reputation,
    // character_spell, character_spell_cooldown, character_ticket, character_talent.
    // character_cuf_profiles, character_currency
    DTT_VS_TABLE,       // Void Storage Table <- playerGuid

    DTT_EQSET_TABLE,    // <- guid                          // character_equipmentsets

    DTT_INVENTORY,      //    -> item guids collection      // character_inventory

    DTT_MAIL,           //    -> mail ids collection        // mail
    //    -> item_text

    DTT_MAIL_ITEM,      // <- mail ids                      // mail_items
    //    -> item guids collection

    DTT_ITEM,           // <- item guids                    // item_instance
    //    -> item_text

    DTT_ITEM_GIFT,      // <- item guids                    // character_gifts

    DTT_PET,            //    -> pet guids collection       // character_pet
    DTT_PET_TABLE,      // <- pet guids                     // pet_aura, pet_spell, pet_spell_cooldown

    DTT_ACC_ACH_PRO,    //   account_achievement_progress
    DTT_ACC_ACH,        //   account_achievement
    DTT_ACC_SPELL,      ///< account_spell
    DTT_GARR_MAIN,      ///< Garrison main table
    DTT_GARR_B,         ///< Garrison building
    DTT_GARR_F,         ///< Garrison followers
    DTT_GARR_M,         ///< Garrison missions
    DTT_GARR_WO,        ///< Garrison work orders
};

enum DumpReturn
{
    DUMP_SUCCESS,
    DUMP_FILE_OPEN_ERROR,
    DUMP_TOO_MANY_CHARS,
    DUMP_UNEXPECTED_END,
    DUMP_FILE_BROKEN,
    DUMP_CHARACTER_DELETED,
    DUMP_SYSTEM_LOCKED
};

class PlayerDump
{
    public:
        static void LoadColumnsName();
    protected:
        PlayerDump() {}
};

class PlayerDumpWriter : public PlayerDump
{
public:
    PlayerDumpWriter() {}

    bool GetDump(uint32 guid, uint32 account, std::string& dump, bool ashran);
    DumpReturn WriteDump(const std::string& file, uint32 guid, uint32 account, bool ashran = false);

private:
    typedef std::set<uint32> GUIDs;

    bool DumpTable(std::string& dump, uint32 guid, uint32 account, char const*tableFrom, char const*tableTo, DumpTableType type, std::vector<std::string> const& columns, bool ashran, uint32& p_GarrisonID);
    std::string GenerateWhereStr(char const* field, GUIDs const& guids, GUIDs::const_iterator& itr);
    std::string GenerateWhereStr(char const* field, uint32 guid);

    GUIDs pets;
    GUIDs mails;
    GUIDs items;
};

class PlayerDumpReader : public PlayerDump
{
public:
    PlayerDumpReader() {}

    DumpReturn LoadDump(const std::string& p_File, uint32 p_Account, std::string p_Name, uint32 p_Guid, bool p_OnlyBoundedItems = false, uint32 p_AtLogin = 0, bool p_Premade = false);
};

#define sInterRealmTransfertReader ACE_Singleton<PlayerDumpReader, ACE_Thread_Mutex>::instance()
#define sInterRealmTransfertWriter ACE_Singleton<PlayerDumpWriter, ACE_Thread_Mutex>::instance()

#endif

