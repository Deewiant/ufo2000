/*
This file is part of "UFO 2000" aka "X-COM: Gladiators"
                    http://ufo2000.sourceforge.net/
Copyright (C) 2000-2001  Alexander Ivanov aka Sanami
Copyright (C) 2002-2004  ufo2000 development team

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
*/

#include "stdafx.h"

#include "global.h"

#include "pck.h"
#include "item.h"
#include "video.h"
#include "map.h"
#include "multiplay.h"
#include "sound.h"
#include "colors.h"
#include "text.h"
#include "explo.h"

char* damage_names[7] = {"AP", "IN", "HE", "LS", "PL", "ST",""};

IMPLEMENT_PERSISTENCE(Item, "Item");

int Item::obdata_get_int(int item_index, const char *property_name)
{
	int stack_top = lua_gettop(L);
    // Enter 'ItemsTable' table
	lua_pushstring(L, "ItemsTable");
	lua_gettable(L, LUA_GLOBALSINDEX);
	ASSERT(lua_istable(L, -1)); 
    // Enter [item_index] table
	lua_pushnumber(L, item_index);
	lua_gettable(L, -2);
	ASSERT(lua_istable(L, -1));
    // Get property value
	lua_pushstring(L, property_name);
	lua_gettable(L, -2);
	int result = 0;
	if (lua_isnumber(L, -1)) result = (int)lua_tonumber(L, -1);
	lua_settop(L, stack_top);
	return result;
}

BITMAP *Item::obdata_get_bitmap(int item_index, const char *property_name, int bitmap_index)
{
	int stack_top = lua_gettop(L);
    // Enter 'ItemsTable' table
	lua_pushstring(L, "ItemsTable");
	lua_gettable(L, LUA_GLOBALSINDEX);
	ASSERT(lua_istable(L, -1)); 
    // Enter [item_index] table
	lua_pushnumber(L, item_index);
	lua_gettable(L, -2);
	ASSERT(lua_istable(L, -1));
    // Get property value
	lua_pushstring(L, property_name);
	lua_gettable(L, -2);
	BITMAP *result = NULL;
    if (lua_istable(L, -1)) {
        lua_pushnumber(L, bitmap_index);
        lua_gettable(L, -2);
    }
	if (lua_islightuserdata(L, -1)) result = (BITMAP *)lua_topointer(L, -1);
	lua_settop(L, stack_top);
	return result;
}

int Item::obdata_get_array_int(int item_index, const char *property_name, int index)
{
	int stack_top = lua_gettop(L);
    // Enter 'ItemsTable' table
	lua_pushstring(L, "ItemsTable");
	lua_gettable(L, LUA_GLOBALSINDEX);
	ASSERT(lua_istable(L, -1)); 
    // Enter [item_index] table
	lua_pushnumber(L, item_index);
	lua_gettable(L, -2);
	ASSERT(lua_istable(L, -1));
    // Get property value
	lua_pushstring(L, property_name);
	lua_gettable(L, -2);
	if (!lua_istable(L, -1)) {
		lua_settop(L, stack_top);
		return 0;
	}
    // Index a value inside of property
	lua_pushnumber(L, index + 1);
	lua_gettable(L, -2);

	int result = 0;
	if (lua_isnumber(L, -1)) result = (int)lua_tonumber(L, -1);

	lua_settop(L, stack_top);
	return result;
}

std::string Item::obdata_get_string(int item_index, const char *property_name)
{
	int stack_top = lua_gettop(L);
    // Enter 'ItemsTable' table
	lua_pushstring(L, "ItemsTable");
	lua_gettable(L, LUA_GLOBALSINDEX);
	ASSERT(lua_istable(L, -1)); 
    // Enter [item_index] table
	lua_pushnumber(L, item_index);
	lua_gettable(L, -2);
	std::string result = "";
	if (lua_istable(L, -1)) {
	    // Get property value
		lua_pushstring(L, property_name);
		lua_gettable(L, -2);
		if (lua_isstring(L, -1)) {
			result = lua_tostring(L, -1);
		}
	}
	lua_settop(L, stack_top);
	return result;
}

bool Item::get_ammo_list(const std::string item_name, std::vector<std::string> &ammo)
{
	ammo.clear();

	int stack_top = lua_gettop(L);
    // Enter 'ItemsTable' table
	lua_pushstring(L, "ItemsTable");
	lua_gettable(L, LUA_GLOBALSINDEX);
	ASSERT(lua_istable(L, -1)); 
    // Enter [item_index] table
	lua_pushstring(L, item_name.c_str());
	lua_gettable(L, -2);
	if (!lua_istable(L, -1)) {
		lua_settop(L, stack_top);
		return false;
	}
	lua_pushstring(L, "ammo");
	lua_gettable(L, -2);
	if (!lua_istable(L, -1)) {
		lua_settop(L, stack_top);
		return true;
	}

	int i = 1;

	while (true) {
		lua_pushnumber(L, i);
		lua_gettable(L, -2);
		if (!lua_isstring(L, -1)) {
			lua_settop(L, stack_top);
			return true;
		}
		ammo.push_back(lua_tostring(L, -1));
		lua_pop(L, 1);
		i++;
	}
}

/**
 * Show object stats information in the armoury
 */
void Item::od_info(int type, int gx, int gy, int gcol)
{
	text_mode(-1);

	textprintf(screen2, font, gx, gy, gcol, "%s", obdata_name(type).c_str());
	gy += 15;

	if (obdata_wayPoints(type) || obdata_isGun(type)) {
      //textprintf(screen2, font, gx + 5, gy, gcol, "%13s %-8s %s",
        textprintf(screen2, font, gx + 0, gy, gcol, "%13s %-8s %10s",
                       _("Firing-Type"), _("Accuracy"), _("TU cost") );
		gy += 10;
		if (obdata_accuracy(type, 0)) {
            textprintf(screen2, font, gx, gy, gcol, "%10s(%d)     %3d%%    %5d%%",
                       _("Auto"), obdata_autoShots(type), obdata_accuracy(type, 0), 
					   obdata_time(type, 0) * obdata_autoShots(type));
			gy += 10;
		}
		if (obdata_accuracy(type, 1)) {
            textprintf(screen2, font, gx, gy, gcol, "%13s     %3d%%    %5d%%",
                       _("Snap"),
                       obdata_accuracy(type, 1), obdata_time(type, 1));
			gy += 10;
		}
		if (obdata_accuracy(type, 2)) {
            textprintf(screen2, font, gx, gy, gcol, "%13s     %3d%%    %5d%%",
                       _("Aimed"),
                       obdata_accuracy(type, 2), obdata_time(type, 2));
			gy += 10;
		}
		gy += 5;
	}

	if (obdata_twoHanded(type)) {
        textprintf(screen2, font, gx, gy, gcol, _("Two-handed weapon") );
		gy += 15;
	}

	if (obdata_damage(type) > 0) {
        textprintf(screen2, font, gx, gy, gcol, "%13s: %3d (%2d%%) %8s: %d",
                   _("Damage"),     obdata_damage(type), obdata_dDeviation(type),
                   _("Ammo-Type"),   obdata_damageType(type));
		gy += 10;
	}

	if (obdata_isAmmo(type)) {
        textprintf(screen2, font, gx, gy, gcol, "%13s: %3d", 
                   _("Rounds"), obdata_rounds(type));
		gy += 10;
	}
	
	if (obdata_exploRange(type) > 0) {
        textprintf(screen2, font, gx, gy, gcol, "%13s: %3d",
                   _("Blast range"), obdata_exploRange(type));
		gy += 10;
	}
	
	if (obdata_smokeRange(type) > 0) {
        textprintf(screen2, font, gx, gy, gcol, "%13s: %3d",
                   _("Smoke range"), obdata_smokeRange(type));
		gy += 10;
	}
	
	if (obdata_smokeTime(type) > 0) {
        textprintf(screen2, font, gx, gy, gcol, "%13s: %3d",
                   _("Smoke time"), obdata_smokeTime(type));
		gy += 10;
	}

	std::vector<std::string> ammo;
	get_ammo_list(obdata_name(type), ammo);

	for (int i = 0; i < (int)ammo.size(); i++) {
        textprintf(screen2, font, gx, gy, gcol, "%12s%d: %s", 
                   _("Ammo#"), i + 1, ammo[i].c_str());
		gy += 10;
	}

    textprintf(screen2, font, gx, gy, gcol, "%13s: %3d", 
               _("Weight"), obdata_weight(type));
	gy += 10;
    textprintf(screen2, font, gx, gy, gcol, "%13s: %3d", 
               _("Cost"), obdata_cost(type));
	gy += 10;
}

Item::Item(int _type)
{
	m_type = _type;
	m_x = 0; m_y = 0;
	m_next = NULL; m_prev = NULL; m_place = NULL;
	m_rounds = obdata_rounds(_type);
	m_delay_time = 0;
	m_ammo = NULL;

	m_health = obdata_maxHealth();

	m_pInv = obdata_get_bitmap(m_type, "pInv");	// Picture for inventory
	ASSERT(m_pInv);
	m_pMap = obdata_get_bitmap(m_type, "pMap");	// Picture for battlemap
	ASSERT(m_pMap);
    for (int i = 0; i < 8; i++) {
        // Pictures for items in hands
        m_pHeld[i] = obdata_get_bitmap(m_type, "pHeld", i + 1);
        ASSERT(m_pHeld[i]);
    }

	std::string sound = obdata_get_string(m_type, "sound");
	m_sound = getSymCode(sound.c_str());
	if (!sound.empty() && m_sound == SS_UNKNOWN) {
		lua_safe_dostring(L, 
			(std::string("Warning([[%s]], [[Invalid sound effect identifier - '") + 
			sound + std::string("']])")).c_str());
	}
}

Item::~Item()
{
	if (m_ammo != NULL) {
		ASSERT(m_ammo->m_next == NULL);
		ASSERT(m_ammo->m_prev == NULL);
		delete m_ammo;
	}
}

void Item::unlink()
{
	if (m_prev != NULL) m_prev->m_next = m_next;
	if (m_next != NULL) m_next->m_prev = m_prev;
        if (m_place != NULL && m_place->m_item == this) m_place->m_item = m_next;
	m_prev = NULL; m_next = NULL; m_place = NULL;
}

int Item::loadclip(Item *&clip)
{
	ASSERT(clip != NULL);
	if (can_use_ammo_type(obdata_name(clip->m_type))) {
		clip->unlink();
		Item *it = m_ammo;
		m_ammo = clip;
		clip = it;
		return 1;
	}
	return 0;
}

Item *Item::unload()
{
	Item *t;
	t = m_ammo;
	m_ammo = NULL;
	return t;
}

int Item::haveclip()
{
	if (m_ammo != NULL)
		return 1;
	return 0;
}

void Item::shot()
{
	if (m_ammo->m_rounds > 0)
		m_ammo->m_rounds--;
	if (m_ammo->m_rounds == 0 && m_ammo->obdata_disappear()) {
		if (m_ammo != NULL) {
            elist->remove(m_ammo);
			delete m_ammo;
			m_ammo = NULL;
		}
	}
}

int Item::inside(int _x, int _y)
{
	if ((m_x <= _x) && (_x < m_x + obdata_width()) && (m_y <= _y) && (_y < m_y + obdata_height()))
		return 1;
	return 0;
}

void Item::draw_health(BITMAP *dest, int GRAPH, int gx, int gy)
{
	int color = COLOR_GREEN;		//gr
	if (m_health < obdata_maxHealth() * 2 / 3)
		color = COLOR_YELLOW01;		//yel
	if (m_health < obdata_maxHealth() / 3)
		color = COLOR_RED01;		//red
	if (GRAPH) {
		int len;     //=m_health;
		len = 14 * m_health / obdata_maxHealth();
		hline(dest, gx, gy, gx + len, color);
	} else
		printsmall_x(dest, gx, gy + 1, color, m_health);
}

int Item::damage(int dam)
{
	m_health -= dam;
	if (m_health <= 0)
		return 1;
	return 0;
}

char* Item::get_damage_name()
{
    int damage_type = m_ammo->obdata_damageType(m_ammo->m_type);
    if (haveclip() && damage_type >= 0 && damage_type < 8)
        return damage_names[damage_type];
    return damage_names[7];
}

bool Item::Write(persist::Engine &archive) const
{
	PersistWriteBinary(archive, *this);

	PersistWriteObject(archive, m_next);
	PersistWriteObject(archive, m_prev);
	PersistWriteObject(archive, m_place);
	PersistWriteObject(archive, m_ammo);

	return true;
}

bool Item::Read(persist::Engine &archive)
{
	PersistReadBinary(archive, *this);

	PersistReadObject(archive, m_next);
	PersistReadObject(archive, m_prev);
	PersistReadObject(archive, m_place);
	PersistReadObject(archive, m_ammo);

    m_pInv = obdata_get_bitmap(m_type, "pInv");
    ASSERT(m_pInv);
    m_pMap = obdata_get_bitmap(m_type, "pMap");
    ASSERT(m_pMap);
    for (int i = 0; i < 8; i++) {
        m_pHeld[i] = obdata_get_bitmap(m_type, "pHeld", i + 1);
        ASSERT(m_pHeld[i]);
    }

    return true;
}
