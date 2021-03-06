/*
This file is part of UFO2000 (http://ufo2000.sourceforge.net)

Copyright (C) 2000-2001  Alexander Ivanov aka Sanami
Copyright (C) 2002-2003  ufo2000 development team

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
#ifndef PLATOON_H
#define PLATOON_H

#include "global.h"

#include "soldier.h"
#include "position.h"

class Units;
class Statistics;

/**
 * Class for handling the squad controlled by one of the opposing sides.
 *
 * @ingroup battlescape
 * @ingroup planner
 */
class Platoon: public persist::BaseObject
{
    DECLARE_PERSISTENCE(Platoon);
private:
    friend class Soldier;
    //! Pointer to current selected soldier in the squad
    Soldier *man;
    //! Squad identifier
    long     ID;
    //! Number of soldiers in the squad
    long     size;

    //! Each bit in the vision_matrix is set to 1 if the soldier
    //! with the corresponding vision_mask can see that map cell.
    int32 m_vision_matrix[MAP_LEVEL_LIMIT * 10 * 6 * 10 * 6];
        
    //! Map cell that has been explored
    int m_seen[MAP_LEVEL_LIMIT][10 * 6][10 * 6];
        
    //! Seen items remain visible until you look at them again
    int m_seen_item_index[MAP_LEVEL_LIMIT][10 * 6][10 * 6];
        
    //! All visible enemy soldiers
    int32 m_visible_enemies;
        
    Statistics *m_stats;

public:
    Platoon() { memset(m_seen, 0, sizeof(m_seen)); }
    Platoon(int PID, int num);
    Platoon(int PID, PLAYERDATA *pd, DeployType dep_type);
    virtual ~Platoon();
    void destroy();

    Statistics *get_stats() {return m_stats;}

    void move(int ISLOCAL);
        void restore_moved();
    void restore();
    void bullmove();
    void bulldraw();

    int belong(Soldier *some);
    int belong(Bullet *some);
    Soldier *findman(int SID);
    Soldier *findman(char *sname);
    Soldier *findnum(int N);
    Soldier *next_not_moved_man(Soldier *sel_man);
    Place *find_item(Item *it, int &lev, int &col, int &row);
    int find_place_coords(Place *pl, int &lev, int &col, int &row);
    int check_for_hit(int z, int x, int y, Soldier* no_test = NULL);
    void apply_hit(int sniper, int z, int x, int y, int type, int hitdir);
    int dist_to_nearest(Soldier *some);
        
    int nobullfly();
    int nomoves();

    int realsize();

    Soldier *captain()
    {
        if (man == NULL) return NULL;
        return man->is_active() ? man : man->next_active_soldier();
    }
    int num_of_men() { ASSERT(size == realsize()); return size; }
    int calc_platoon_cost();

    int check_reaction_fire(Soldier *target);
    
    void change_morale(int delta, bool send_to_remote);
    void check_morale();

    void save_FULLDATA(const char *fn);
    void load_FULLDATA(const char *fn);

    void build_Units(Units &local);
    void send_Units(Units &local);

    int eot_save(char *buf, int &buf_size);

    //! Visibility calculations for the platoon
    void initialize_vision_matrix();
    int32 *get_vision_matrix(){return m_vision_matrix;}
    void set_seen(int lev, int col, int row, int value) { m_seen[lev][col][row] = value; }
    int is_seen(int lev, int col, int row);
    int is_visible(int lev, int col, int row);
        
    //! Platoon's visible items and enemies
    int get_seen_item_index(int lev, int col, int row) { return m_seen_item_index[lev][col][row]; }
    void set_seen_item_index(int lev, int col, int row, int value) { m_seen_item_index[lev][col][row] = value; }
    int32 get_visible_enemies(){return m_visible_enemies;}
    void set_visible_enemies(int32 visible_enemies){m_visible_enemies = visible_enemies;}
    int32 update_visible_enemies();
    void draw_enemy_indicators(bool draw_indicators, bool draw_markers); 
    int center_enemy_seen();
        
    void soldier_moved(Soldier* const target);
        
    void sit_on_start();
    void save_to_string(std::string &str);

    virtual bool Write(persist::Engine &archive) const;
    virtual bool Read(persist::Engine &archive);
};

#endif
